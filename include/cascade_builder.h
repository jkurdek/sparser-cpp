#ifndef CASCADE_BUILDER_H_
#define CASCADE_BUILDER_H_

#include <array>
#include <bitset>
#include <utility>
#include <vector>
#include <memory>
#include <cstddef>

#include "config.h"
#include "json_facade.h"
#include "node.h"
#include "raw_filter.h"

class CascadeBuilder {
   public:
    CascadeBuilder(PredicateDisjunction  disjunction, const RawFilterData& raw_filter_data)
        : disjunction_(std::move(disjunction)), rf_data_(raw_filter_data) {}

    [[nodiscard]] std::vector<std::shared_ptr<Node>> GenerateValidCascades();

   private:
    std::shared_ptr<Node> fail_node = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::FAIL);
    std::shared_ptr<Node> parse_node = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::PARSE);
    PredicateDisjunction disjunction_;
    RawFilterData rf_data_;
    std::bitset<kMaxConj> used_conjunctions_;
    std::array<std::array<std::bitset<kMaxRfsInPred>, kMaxPred>, kMaxConj> used_rfs_;

    [[nodiscard]] std::vector<std::shared_ptr<Node>> HandleFail(const size_t current_depth);
    [[nodiscard]] std::vector<std::shared_ptr<Node>> HandleSuccess(const size_t current_depth,
                                                                   const size_t conjunction_idx);
};

#endif  // CASCADE_BUILDER_H_
