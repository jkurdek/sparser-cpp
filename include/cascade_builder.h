#ifndef CASCADE_BUILDER_H_
#define CASCADE_BUILDER_H_

#include <bitset>

#include "json_facade.h"
#include "node.h"
#include "raw_filter.h"

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

#endif  // CASCADE_BUILDER_H_
