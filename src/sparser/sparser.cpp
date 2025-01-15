#include "sparser.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "common.h"
#include "json_facade.h"

RawFilterData RawFilterQueryGenerator::GenerateRawFilters(const PredicateDisjunction& disjunction) {
    RawFilterData rf_data;
    for (size_t conj_idx = 0; conj_idx < disjunction.conjunctions.size(); ++conj_idx) {
        const auto& conjunction = disjunction.conjunctions[conj_idx];
        for (size_t pred_idx = 0; pred_idx < conjunction.predicates.size(); ++pred_idx) {
            const auto& predicate = conjunction.predicates[pred_idx];
            auto raw_filters = GenerateRawFiltersFromPredicate(predicate.value);
            for (size_t rf_idx = 0; rf_idx < raw_filters.size(); ++rf_idx) {
                rf_data.data[conj_idx][pred_idx][rf_idx] = raw_filters[rf_idx];
                rf_data.rf_count[conj_idx][pred_idx]++;
            }
            rf_data.pred_count[conj_idx]++;
        }
        rf_data.conj_count++;
    }
    return rf_data;
}

std::vector<std::string_view> RawFilterQueryGenerator::GenerateRawFiltersFromPredicate(
    const std::string_view& predicate) {
    std::vector<std::string_view> rawFilters;
    for (size_t i = 0; i < predicate.size() - kRfSize + 1; ++i) {
        rawFilters.emplace_back(predicate.substr(i, kRfSize));
    }
    return rawFilters;
}

EstimationResult Sparser::Calibrate(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                                    const RawFilterData& rf_data) {
    auto result = EstimationResult{};

    assert(input.size() >= kSampleSize);

    for (size_t i = 0; i < kSampleSize; i++) {
        auto json_row = input[i];
        for (uint32_t conj_idx = 0; conj_idx < rf_data.conj_count; conj_idx++) {
            for (uint32_t pred_idx = 0; pred_idx < rf_data.pred_count[conj_idx]; pred_idx++) {
                for (uint32_t rf_idx = 0; rf_idx < rf_data.rf_count[conj_idx][pred_idx]; rf_idx++) {
                    auto rf = rf_data.data[conj_idx][pred_idx][rf_idx];
                    std::cout << "Grepping... : " << rf << "\n";

                    auto grepStart = benchmark_start();
                    auto find_result = json_row.find(rf);

                    auto idx = RawFilterData::GetFlatIdx(conj_idx, pred_idx, rf_idx);

                    result.total_rf_runtimes[idx] += benchmark_stop(grepStart);

                    if (find_result != std::string_view::npos) {
                        std::cout << "Found: " << rf << "\n";
                        result.bitsets[idx].set(i);
                    } else {
                        std::cout << "Not found: " << rf << "\n";
                    }
                }
            }
        }
        result.total_parser_runtime += json_query_driver_->RunQuery(input[i], json_query);
    }

    return result;
}

std::vector<std::shared_ptr<Node>> CascadeBuilder::GenerateValidCascades() { return HandleFail(0); }

std::vector<std::shared_ptr<Node>> CascadeBuilder::HandleFail(const size_t current_depth) {
    if (used_conjunctions_.count() == disjunction_.conjunctions.size()) {
        // return vector with a single nullptr
        std::vector<std::shared_ptr<Node>> result;
        result.emplace_back(fail_node);
        return result;
    }

    std::vector<std::shared_ptr<Node>> valid_subtrees;

    for (size_t conj_idx = 0; conj_idx < rf_data_.conj_count; conj_idx++) {
        if (!used_conjunctions_.test(conj_idx)) {
            used_conjunctions_.set(conj_idx);

            for (size_t pred_idx = 0; pred_idx < rf_data_.pred_count[conj_idx]; pred_idx++) {
                for (size_t rf_idx = 0; rf_idx < rf_data_.rf_count[conj_idx][pred_idx]; rf_idx++) {
                    if (!used_predicates_[conj_idx][pred_idx].test(rf_idx)) {
                        used_predicates_[conj_idx][pred_idx].set(rf_idx);

                        auto valid_left_subtrees = HandleFail(current_depth + 1);
                        auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                        for (auto left_subtree : valid_left_subtrees) {
                            for (auto right_subtree : valid_right_subtrees) {
                                auto root =
                                    std::make_shared<Node>(conj_idx, pred_idx, rf_idx, left_subtree, right_subtree);
                                valid_subtrees.emplace_back(root);
                            }
                        }

                        used_predicates_[conj_idx][pred_idx].reset(rf_idx);
                    }
                }
            }

            used_conjunctions_.reset(conj_idx);
        }
    }

    return valid_subtrees;
}

std::vector<std::shared_ptr<Node>> CascadeBuilder::HandleSuccess(const size_t current_depth, const size_t conj_idx) {
    // Check how many levels we have left
    // How many conjunctions we need to visit yet:
    int conj_left = disjunction_.conjunctions.size() - used_conjunctions_.count();
    int free = kMaxDepth - current_depth - conj_left;

    std::vector<std::shared_ptr<Node>> valid_subtrees;
    valid_subtrees.emplace_back(parse_node);  // Finishing here is valid

    if (free > 0) {
        for (size_t pred_idx = 0; pred_idx < rf_data_.pred_count[conj_idx]; pred_idx++) {
            for (size_t rf_idx = 0; rf_idx < rf_data_.rf_count[conj_idx][pred_idx]; rf_idx++) {
                if (!used_predicates_[conj_idx][pred_idx].test(rf_idx)) {
                    used_predicates_[conj_idx][pred_idx].set(rf_idx);

                    auto valid_left_subtrees = HandleFail(current_depth + 1);
                    auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                    for (auto left_subtree : valid_left_subtrees) {
                        for (auto right_subtree : valid_right_subtrees) {
                            auto root = std::make_shared<Node>(conj_idx, pred_idx, rf_idx, left_subtree, right_subtree);
                            valid_subtrees.emplace_back(root);
                        }
                    }

                    used_predicates_[conj_idx][pred_idx].reset(rf_idx);
                }
            }
        }
    }

    return valid_subtrees;
}

void PrettyPrint(const std::shared_ptr<Node>& node, RawFilterData& rf_data, const std::string& prefix, bool isLeft,
                 std::ostream& os) {
    if (!node) {
        // Print "NULL" or some placeholder for an empty child.
        os << prefix << (isLeft ? "├── " : "└── ") << "NULL\n";
        return;
    }

    // Print the data of the current node.
    os << prefix << (isLeft ? "├── " : "└── ")
       << rf_data.data[node->conjunction_idx][node->predicate_idx][node->raw_filter_idx] << "\n";

    // For the left and right children, adjust the prefix
    // (use the extended vertical bar "│" if we're continuing
    // on the left side, or spaces if we've reached the right-most branch).
    auto newPrefix = prefix + (isLeft ? "│   " : "    ");

    // Recursively print left and right subtrees.
    PrettyPrint(node->left, rf_data, newPrefix, true, os);
    PrettyPrint(node->right, rf_data, newPrefix, false, os);
}

double CascadeEvaluator::EvaluateCascade(std::shared_ptr<Node> node) {
    rf_probabilities_.fill(0.0);
    EvaluateParseNodeRec(node, std::bitset<kSampleSize>().set());
    return 0.0;  // TODO: Calculate the cost using probs
}

// TODO: Maybe the split is a bit excessive
void CascadeEvaluator::EvaluateFailNodeRec(std::shared_ptr<Node> node, std::bitset<kSampleSize> cumulative_bitset) {
    if (!node) {
        throw std::runtime_error("Node is nullptr");
    }

    if (node->type == NodeType::PARSE) {
        throw std::runtime_error("Handling fail should not reach PARSE node");
    }

    if (node->type == NodeType::FAIL) {
        rf_probabilities_[fail_idx_] +=
            static_cast<double>(cumulative_bitset.count()) / static_cast<double>(kSampleSize);
        return;
    }

    auto current_rf_idx = RawFilterData::GetFlatIdx(node->conjunction_idx, node->predicate_idx, node->raw_filter_idx);
    auto bitset = estimation_result_.bitsets[current_rf_idx];
    bitset.flip();

    rf_probabilities_[current_rf_idx] += static_cast<double>(bitset.count()) / static_cast<double>(kSampleSize);
    auto new_cumulative_bitset = cumulative_bitset & bitset;

    EvaluateFailNodeRec(node->left, new_cumulative_bitset);
    EvaluateParseNodeRec(node->right, new_cumulative_bitset);
}

void CascadeEvaluator::EvaluateParseNodeRec(std::shared_ptr<Node> node, std::bitset<kSampleSize> cumulative_bitset) {
    if (!node) {
        throw std::runtime_error("Node is nullptr");
    }

    if (node->type == NodeType::FAIL) {
        throw std::runtime_error("Handling parse should not reach FAIL node");
    }

    if (node->type == NodeType::PARSE) {
        rf_probabilities_[parse_idx_] +=
            static_cast<double>(cumulative_bitset.count()) / static_cast<double>(kSampleSize);
        return;
    }

    auto current_rf_idx = RawFilterData::GetFlatIdx(node->conjunction_idx, node->predicate_idx, node->raw_filter_idx);
    auto bitset = estimation_result_.bitsets[current_rf_idx];

    rf_probabilities_[current_rf_idx] += static_cast<double>(bitset.count()) / static_cast<double>(kSampleSize);
    auto new_cumulative_bitset = cumulative_bitset & bitset;

    EvaluateFailNodeRec(node->left, new_cumulative_bitset);
    EvaluateParseNodeRec(node->right, new_cumulative_bitset);
}
