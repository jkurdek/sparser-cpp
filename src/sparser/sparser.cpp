#include "sparser.h"

#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "common.h"
#include "json_facade.h"
#include "rdtsc.h"

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
#ifndef NDEBUG
                    std::cout << "Grepping... : " << rf << "\n";
#endif

                    auto idx = GetFlatIdx(conj_idx, pred_idx, rf_idx);

                    auto grepStart = rdtsc();
                    auto find_result = json_row.find(rf);
                    result.total_rf_runtimes[idx] += (rdtsc() - grepStart);

                    if (find_result != std::string_view::npos) {
#ifndef NDEBUG
                        std::cout << "Found: " << rf << "\n";
#endif
                        result.bitsets[idx].set(i);
                    } else {
#ifndef NDEBUG
                        std::cout << "Not found: " << rf << "\n";
#endif
                    }
                }
            }
        }

        auto json_query_start = rdtsc();
        json_query_driver_->RunQuery(input[i], json_query);
        result.total_parser_runtime += (rdtsc() - json_query_start);
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
                    if (!used_rfs_[conj_idx][pred_idx].test(rf_idx)) {
                        used_rfs_[conj_idx][pred_idx].set(rf_idx);

                        auto valid_left_subtrees = HandleFail(current_depth + 1);
                        auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                        for (auto left_subtree : valid_left_subtrees) {
                            for (auto right_subtree : valid_right_subtrees) {
                                auto root = std::make_shared<Node>(conj_idx, pred_idx, rf_idx, left_subtree,
                                                                   right_subtree, NodeType::INTER);
                                valid_subtrees.emplace_back(root);
                            }
                        }

                        used_rfs_[conj_idx][pred_idx].reset(rf_idx);
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
                if (!used_rfs_[conj_idx][pred_idx].test(rf_idx)) {
                    used_rfs_[conj_idx][pred_idx].set(rf_idx);

                    auto valid_left_subtrees = HandleFail(current_depth + 1);
                    auto valid_right_subtrees = HandleSuccess(current_depth + 1, conj_idx);

                    for (auto left_subtree : valid_left_subtrees) {
                        for (auto right_subtree : valid_right_subtrees) {
                            auto root = std::make_shared<Node>(conj_idx, pred_idx, rf_idx, left_subtree, right_subtree,
                                                               NodeType::INTER);
                            valid_subtrees.emplace_back(root);
                        }
                    }

                    used_rfs_[conj_idx][pred_idx].reset(rf_idx);
                }
            }
        }
    }

    return valid_subtrees;
}

void PrettyPrint(const std::shared_ptr<Node>& node, const RawFilterData& rf_data, const std::string& prefix,
                 bool isLeft, std::ostream& os) {
    if (!node) {
        // Print "NULL" or some placeholder for an empty child.
        os << prefix << (isLeft ? "├── " : "└── ") << "NULL\n";
        return;
    }

    if (node->type == NodeType::FAIL) {
        os << prefix << (isLeft ? "├── " : "└── ") << "FAIL\n";
        return;
    }

    if (node->type == NodeType::PARSE) {
        os << prefix << (isLeft ? "├── " : "└── ") << "PARSE\n";
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

std::string InputReader::ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Error opening file: " + std::string(filename));
    }

    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string buffer(fileSize, '\0');

    if (!file.read(buffer.data(), fileSize)) {
        throw std::runtime_error("Error reading file: " + std::string(filename));
    }

    return buffer;
}

std::vector<std::string_view> InputReader::ReadRecords(const std::string& input) {
    std::vector<std::string_view> records;
    size_t start = 0;
    size_t end = 0;

    while (end < input.size()) {
        if (input[end] == '\n') {
            records.emplace_back(&input[start], end - start);
            start = end + 1;  // Skip the delimiter
        }
        ++end;
    }

    // Add the last segment if not empty
    if (start < input.size()) {
#ifndef NDEBUG
        std::cout << "Adding last segment" << std::string_view(&input[start], input.size() - start) << "\n";
#endif
        records.emplace_back(&input[start], input.size() - start);
    }

    return records;
}

void Sparser::Run(const std::string& input_path, const JsonQuery& json_query) {
    auto input_reader = InputReader();
    auto file_data = input_reader.ReadFile(input_path);
    auto sparser_time_start = benchmark_start();
    auto sparser_input = input_reader.ReadRecords(file_data);

    auto disjunction = json_query.GetDisjunction();
    auto rf_data = RawFilterQueryGenerator::GenerateRawFilters(disjunction);
    auto estimation_result = Calibrate(sparser_input, json_query, rf_data);

    auto cascade_builder = CascadeBuilder(disjunction, rf_data);
    auto valid_cascades = cascade_builder.GenerateValidCascades();

    auto cascade_evaluator = CascadeEvaluator(estimation_result);
    double min_cost = std::numeric_limits<double>::max();
    std::shared_ptr<Node> best_cascade = nullptr;

    for (auto& cascade : valid_cascades) {
        auto cost = cascade_evaluator.EvaluateCascade(cascade);
        if (cost < min_cost) {
            min_cost = cost;
            best_cascade = cascade;
        }
    }

    std::cout << "Best cascade cost: " << min_cost << "\n\n";
    std::cout << "Best cascade:\n";
    PrettyPrint(best_cascade, rf_data);
    std::cout << "\n";

    SearchCascade(sparser_input, json_query, rf_data, best_cascade);

    std::cout << "Total time: " << benchmark_stop(sparser_time_start) << " s\n";

    auto naive_time_start = benchmark_start();
    auto naive_input = input_reader.ReadRecords(file_data);

    SearchNaive(naive_input, json_query);
    std::cout << "Naive total time: " << benchmark_stop(naive_time_start) << " s\n";
}

void Sparser::SearchCascade(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                            const RawFilterData& rf_data, const std::shared_ptr<Node> node) {
    // start timer here
    int sparser_count = 0;

    for (const auto& record : input) {
        auto root = node;
        while (root->type == NodeType::INTER) {
            auto rf = rf_data.data[root->conjunction_idx][root->predicate_idx][root->raw_filter_idx];

            auto find_result = record.find(rf);
            if (find_result != std::string_view::npos) {
                root = root->right;
            } else {
                root = root->left;
            }
        }

        if (root->type == NodeType::PARSE) {
            if (json_query_driver_->RunQuery(record, json_query)) {
                sparser_count++;
            }
        }
    }

    std::cout << "Sparser found: " << sparser_count << "\n";
}

void Sparser::SearchNaive(const std::vector<std::string_view>& input, const JsonQuery& json_query) {
    int true_count = 0;

    for (const auto& record : input) {
        if (json_query_driver_->RunQuery(record, json_query)) {
            true_count++;
        }
    }

    std::cout << "Naive found: " << true_count << "\n";
}
