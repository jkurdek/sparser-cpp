#include "cascade_evaluator.h"

#include <bitset>
#include <memory>

#include "config.h"
#include "node.h"
#include "raw_filter.h"

double CascadeEvaluator::EvaluateCascade(const std::shared_ptr<Node>& cascade) {
    return EvaluateSubtreeCost(cascade, std::bitset<kSampleSize>().set());
}

double CascadeEvaluator::EvaluateSubtreeCost(const std::shared_ptr<Node>& node,
                                             const std::bitset<kSampleSize>& passed_records) {
    if (!node) {
        return 0.0;
    }

    const double pass_fraction = static_cast<double>(passed_records.count()) / kSampleSize;
    if (node->type == NodeType::PARSE) {
        return pass_fraction * estimation_result_.average_parse_time;
    }
    if (node->type == NodeType::FAIL) {
        return 0.0;
    }

    const double subtree_cost = pass_fraction * estimation_result_.average_rf_time;

    const auto& matching_records =
        estimation_result_.bitsets.at(GetFlatIdx(node->conjunction_idx, node->predicate_idx, node->raw_filter_idx));

    return EvaluateSubtreeCost(node->left, passed_records & ~matching_records) +
           EvaluateSubtreeCost(node->right, passed_records & matching_records) + subtree_cost;

    return subtree_cost;
}
