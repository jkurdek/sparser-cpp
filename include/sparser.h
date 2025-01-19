#ifndef SPARSER_H_
#define SPARSER_H_

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "json_facade.h"

constexpr size_t kRfSize = 4;
constexpr size_t kSampleSize = 10;
constexpr size_t kMaxDepth = 4;

constexpr size_t kMaxRfsInPred = 32;
constexpr size_t kMaxPred = 10;
constexpr size_t kMaxConj = 10;
constexpr size_t kTotalMaxRfs = kMaxRfsInPred * kMaxPred * kMaxConj;

class InputReader {
   public:
    static std::string ReadFile(const std::string& filename);
    static std::vector<std::string_view> ReadRecords(const std::string& input);
};

struct EstimationResult {
    unsigned long long total_parser_runtime;
    std::array<unsigned long long, kTotalMaxRfs> total_rf_runtimes;
    std::array<std::bitset<kSampleSize>, kTotalMaxRfs> bitsets;
};

struct RawFilterData {
    std::array<std::array<std::array<std::string_view, kMaxRfsInPred>, kMaxPred>, kMaxConj> data;
    std::array<std::array<size_t, kMaxPred>, kMaxConj> rf_count = {};
    std::array<size_t, kMaxConj> pred_count = {};
    size_t conj_count = 0;
};

class RawFilterQueryGenerator {
   public:
    static RawFilterData GenerateRawFilters(const PredicateDisjunction& disjunction);
    static std::vector<std::string_view> GenerateRawFiltersFromPredicate(const std::string_view& input);
};

enum class NodeType { INTER, FAIL, PARSE };

struct Node {
    uint32_t conjunction_idx;
    uint32_t predicate_idx;
    uint32_t raw_filter_idx;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
    NodeType type;

    Node(uint32_t conj_idx, uint32_t pred_idx, uint32_t rf_idx, std::shared_ptr<Node> left_subtree,
         std::shared_ptr<Node> right_subtree, NodeType node_type)
        : conjunction_idx(conj_idx),
          predicate_idx(pred_idx),
          raw_filter_idx(rf_idx),
          left(left_subtree),
          right(right_subtree),
          type(node_type) {}
};

class Sparser {
   public:
    explicit Sparser(std::unique_ptr<JsonQueryDriver>&& json_query_driver = {})
        : json_query_driver_(std::move(json_query_driver)) {}

    void Run(const std::string& input_path, const JsonQuery& json_query);

    EstimationResult Calibrate(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                               const RawFilterData& rf_data);
    void SearchCascade(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                       const RawFilterData& rf_data, const std::shared_ptr<Node>);
    void SearchNaive(const std::vector<std::string_view>& input, const JsonQuery& json_query);

   private:
    std::unique_ptr<JsonQueryDriver> json_query_driver_;
};

class CascadeBuilder {
   public:
    CascadeBuilder(const PredicateDisjunction& disjunction, const RawFilterData& raw_filter_data)
        : disjunction_(disjunction), rf_data_(raw_filter_data) {}

    std::vector<std::shared_ptr<Node>> GenerateValidCascades();

   private:
    std::shared_ptr<Node> fail_node = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::FAIL);
    std::shared_ptr<Node> parse_node = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::PARSE);
    const PredicateDisjunction& disjunction_;
    const RawFilterData& rf_data_;
    std::bitset<kMaxConj> used_conjunctions_;
    std::array<std::array<std::bitset<kMaxRfsInPred>, kMaxPred>, kMaxConj> used_rfs_;

    std::vector<std::shared_ptr<Node>> HandleFail(const size_t current_depth);
    std::vector<std::shared_ptr<Node>> HandleSuccess(const size_t current_depth, const size_t conjunction_idx);
};

class CascadeEvaluator {
   public:
    CascadeEvaluator(const EstimationResult& estimation_result) : estimation_result_(estimation_result) {}

    double EvaluateCascade(std::shared_ptr<Node> cascade);
    std::array<double, kTotalMaxRfs + 2> rf_probabilities_;  // Changes every call to EvaluateCascade
    const size_t parse_idx_ = kTotalMaxRfs + 1;
    const size_t fail_idx_ = kTotalMaxRfs;

   private:
    const EstimationResult& estimation_result_;
    void EvaluateNodeRec(std::shared_ptr<Node> node, std::bitset<kSampleSize> cumulative_bitset);
};

static inline size_t GetFlatIdx(size_t conj_idx, size_t pred_idx, size_t rf_idx) {
    return conj_idx * kMaxPred * kMaxRfsInPred + pred_idx * kMaxRfsInPred + rf_idx;
}

void PrettyPrint(const std::shared_ptr<Node>& node, const RawFilterData& rf_data, const std::string& prefix = "",
                 bool isLeft = true, std::ostream& os = std::cout);

#endif  // SPARSER_H_
