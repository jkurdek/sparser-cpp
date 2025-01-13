#include "sparser.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include "common.h"
#include "json_facade.h"

RawFilterDisjunction RawFilterQueryGenerator::GenerateRawFilters(const PredicateDisjunction& disjunction) {
    RawFilterDisjunction raw_filter_disjunction;
    for (size_t conj_idx = 0; conj_idx < disjunction.conjunctions.size(); ++conj_idx) {
        const auto& conjunction = disjunction.conjunctions[conj_idx];
        RawFilterConjunction raw_filter_conjunction;
        for (size_t pred_idx = 0; pred_idx < conjunction.predicates.size(); ++pred_idx) {
            const auto& predicate = conjunction.predicates[pred_idx];
            RawFilterPredicate raw_filter_predicate;
            raw_filter_predicate.raw_filters = GenerateRawFiltersFromPredicate(predicate.value);
            raw_filter_conjunction.predicates.push_back(raw_filter_predicate);
        }
        raw_filter_disjunction.conjunctions.push_back(raw_filter_conjunction);
    }
    return raw_filter_disjunction;
}

std::vector<std::string_view> RawFilterQueryGenerator::GenerateRawFiltersFromPredicate(
    const std::string_view& predicate) {
    std::vector<std::string_view> rawFilters;
    for (size_t i = 0; i < predicate.size() - kRfSize + 1; ++i) {
        rawFilters.emplace_back(predicate.substr(i, kRfSize));
    }
    return rawFilters;
}

EstimationResult Sparser::calibrate(const std::vector<std::string_view>& input, JsonQuery json_query) {
    auto raw_filter_data = RawFilterQueryGenerator::GenerateRawFilters(json_query.GetDisjunction());
    auto result = EstimationResult{};

    assert(input.size() >= kSampleSize);

    for (size_t i = 0; i < kSampleSize; i++) {
        auto json_row = input[i];
        for (size_t rf_idx = 0; rf_idx < kMaxRfs; rf_idx++) {
            auto rf = raw_filter_data.conjunctions[0].predicates[0].raw_filters[rf_idx];  // TODO: Fix
            std::cout << "Grepping... : " << rf << "\n";

            auto grepStart = benchmark_start();
            auto find_result = json_row.find(rf);

            result.total_rf_runtimes[i] += benchmark_stop(grepStart);
            if (find_result != std::string_view::npos) {
                std::cout << "Found: " << rf << "\n";
                result.bitsets[rf_idx].set(i);
            } else {
                std::cout << "Not found: " << rf << "\n";
            }

            auto parse_start = benchmark_start();
            json_query_driver_->RunQuery(json_row, json_query);
            result.total_parser_runtime += benchmark_stop(parse_start);
        }
    }

    return result;
}

std::vector<std::shared_ptr<Node>> CascadeBuilder::GenerateValidCascades() { return HandleFail(0); }

std::vector<std::shared_ptr<Node>> CascadeBuilder::HandleFail(const size_t current_depth) {
    if (used_conjunctions_.count() == disjunction_.conjunctions.size()) {
        // return vector with a single nullptr
        std::vector<std::shared_ptr<Node>> result;
        result.emplace_back(nullptr);
        return result;
    }

    std::vector<std::shared_ptr<Node>> valid_subtrees;

    for (size_t conj_idx = 0; conj_idx < disjunction_.conjunctions.size(); conj_idx++) {
        if (!used_conjunctions_.test(conj_idx)) {
            used_conjunctions_.set(conj_idx);

            for (size_t pred_idx = 0; pred_idx < disjunction_.conjunctions[conj_idx].predicates.size(); pred_idx++) {
                for (size_t rf_idx = 0;
                     rf_idx < rf_data_.conjunctions[conj_idx].predicates[pred_idx].raw_filters.size(); rf_idx++) {
                    if (!used_predicates_[conj_idx][pred_idx].test(rf_idx)) {
                        used_predicates_[conj_idx][pred_idx].set(rf_idx);

                        auto valid_left_subtrees = HandleFail(current_depth + 1);
                        auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                        for (const auto& left_subtree : valid_left_subtrees) {
                            for (const auto& right_subtree : valid_right_subtrees) {
                                auto root = std::make_shared<Node>();
                                root->conjunction_idx = conj_idx;
                                root->predicate_idx = pred_idx;
                                root->raw_filter_idx = rf_idx;
                                root->left = left_subtree;
                                root->right = right_subtree;
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
    valid_subtrees.emplace_back(nullptr);  // Finishing here is valid

    if (free > 0) {
        for (size_t pred_idx = 0; pred_idx < disjunction_.conjunctions[conj_idx].predicates.size(); pred_idx++) {
            for (size_t rf_idx = 0; rf_idx < rf_data_.conjunctions[conj_idx].predicates[pred_idx].raw_filters.size();
                 rf_idx++) {
                if (!used_predicates_[conj_idx][pred_idx].test(rf_idx)) {
                    used_predicates_[conj_idx][pred_idx].set(rf_idx);

                    auto valid_left_subtrees = HandleFail(current_depth + 1);
                    auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                    for (const auto& left_subtree : valid_left_subtrees) {
                        for (const auto& right_subtree : valid_right_subtrees) {
                            auto root = std::make_shared<Node>();
                            root->conjunction_idx = conj_idx;
                            root->predicate_idx = pred_idx;
                            root->raw_filter_idx = rf_idx;
                            root->left = left_subtree;
                            root->right = right_subtree;
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

void PrettyPrint(const std::shared_ptr<Node>& node, RawFilterDisjunction& rf_data, const std::string& prefix,
                 bool isLeft, std::ostream& os) {
    if (!node) {
        // Print "NULL" or some placeholder for an empty child.
        os << prefix << (isLeft ? "├── " : "└── ") << "NULL\n";
        return;
    }

    // Print the data of the current node.
    os << prefix << (isLeft ? "├── " : "└── ")
       << rf_data.conjunctions[node->conjunction_idx].predicates[node->predicate_idx].raw_filters[node->raw_filter_idx]
       << "\n";

    // For the left and right children, adjust the prefix
    // (use the extended vertical bar "│" if we're continuing
    // on the left side, or spaces if we've reached the right-most branch).
    auto newPrefix = prefix + (isLeft ? "│   " : "    ");

    // Recursively print left and right subtrees.
    PrettyPrint(node->left, rf_data, newPrefix, true, os);
    PrettyPrint(node->right, rf_data, newPrefix, false, os);
}
