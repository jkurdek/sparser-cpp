#ifndef SPARSER_H_
#define SPARSER_H_

#include <array>
#include <bitset>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "json_facade.h"

constexpr size_t kRfSize = 4;
constexpr size_t kSampleSize = 10;
constexpr size_t kMaxRfs = 32;
constexpr size_t kMaxDepth = 4;

struct EstimationResult {
    std::array<double, kMaxRfs> total_rf_runtimes;
    double total_parser_runtime;
    std::array<std::bitset<kMaxRfs>, kSampleSize> bitsets;
};

struct PredicateRawFilters {
    std::vector<std::string_view> raw_filters;
};

struct RawFilterPredicate {
    std::vector<std::string_view> raw_filters;
};

struct RawFilterConjunction {
    std::vector<RawFilterPredicate> predicates;
};

struct RawFilterDisjunction {
    std::vector<RawFilterConjunction> conjunctions;
};

class RawFilterQueryGenerator {
   public:
    static RawFilterDisjunction GenerateRawFilters(const PredicateDisjunction& disjunction);
    static std::vector<std::string_view> GenerateRawFiltersFromPredicate(const std::string_view& input);
};

class Sparser {
   public:
    explicit Sparser(std::unique_ptr<JsonQueryDriver>&& json_query_driver = {})
        : json_query_driver_(std::move(json_query_driver)) {}

    EstimationResult calibrate(const std::vector<std::string_view>& input, JsonQuery json_query);

   private:
    std::unique_ptr<JsonQueryDriver> json_query_driver_;
};

struct Node {
    uint32_t conjunction_idx;
    uint32_t predicate_idx;
    uint32_t raw_filter_idx;
    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;
};

class CascadeBuilder {
   public:
    CascadeBuilder(const PredicateDisjunction& disjunction, const RawFilterDisjunction& raw_filter_data)
        : disjunction_(disjunction), rf_data_(raw_filter_data) {}

    std::vector<std::shared_ptr<Node>> GenerateValidCascades();

   private:
    const PredicateDisjunction& disjunction_;
    const RawFilterDisjunction& rf_data_;
    std::bitset<kMaxDepth> used_conjunctions_;
    std::array<std::array<std::bitset<10>, kMaxDepth>, kMaxDepth> used_predicates_;  // TODO: Add correct dimensions

    std::vector<std::shared_ptr<Node>> HandleFail(const size_t current_depth);
    std::vector<std::shared_ptr<Node>> HandleSuccess(const size_t current_depth, const size_t conjunction_idx);
};

void PrettyPrint(const std::shared_ptr<Node>& node, const RawFilterDisjunction& rf_data, const std::string& prefix = "",
                 bool isLeft = true, std::ostream& os = std::cout);

#endif  // SPARSER_H_
