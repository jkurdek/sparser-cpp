#include "cascade_evaluator.h"

#include "raw_filter.h"

double CascadeEvaluator::EvaluateCascade(std::shared_ptr<Node> node) {
    rf_probabilities_.fill(0.0);
    EvaluateNodeRec(node, std::bitset<kSampleSize>().set());

    double cost = 0.0;
    for (size_t idx = 0; idx < kTotalMaxRfs; idx++) {
        cost += rf_probabilities_[idx] * estimation_result_.total_rf_runtimes[idx];
    }

    cost += rf_probabilities_[parse_idx_] * estimation_result_.total_parser_runtime;

    return cost;
}

void CascadeEvaluator::EvaluateNodeRec(std::shared_ptr<Node> node, std::bitset<kSampleSize> cumulative_bitset) {
    if (!node) {
        throw std::runtime_error("Node is nullptr");
    }

    if (node->type == NodeType::FAIL) {
        rf_probabilities_[fail_idx_] +=
            static_cast<double>(cumulative_bitset.count()) / static_cast<double>(kSampleSize);
        return;
    }

    if (node->type == NodeType::PARSE) {
        rf_probabilities_[parse_idx_] +=
            static_cast<double>(cumulative_bitset.count()) / static_cast<double>(kSampleSize);
        return;
    }

    auto current_rf_idx = GetFlatIdx(node->conjunction_idx, node->predicate_idx, node->raw_filter_idx);
    rf_probabilities_[current_rf_idx] +=
        static_cast<double>(cumulative_bitset.count()) / static_cast<double>(kSampleSize);

    auto bitset = estimation_result_.bitsets[current_rf_idx];

    EvaluateNodeRec(node->left, cumulative_bitset & (~bitset));
    EvaluateNodeRec(node->right, (cumulative_bitset & bitset));
}
