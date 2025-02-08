#include "sparser.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "cascade_builder.h"
#include "common.h"
#include "input_reader.h"
#include "json_facade.h"
#include "rdtsc.h"

EstimationResult Sparser::Calibrate(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                                    const RawFilterData& rf_data) {
    auto result = EstimationResult{};

    assert(input.size() >= kSampleSize);

    double total_rf_time = 0.0;
    size_t rf_count = 0;
    double total_parser_time = 0.0;

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
                    auto grepEnd = rdtsc();

                    double rf_runtime = grepEnd - grepStart;
                    total_rf_time += rf_runtime;
                    rf_count++;

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
        auto query_result = json_query_driver_->RunQuery(input[i], json_query);
        (void)query_result;  // explicitly ignore the result
        total_parser_time += (rdtsc() - json_query_start);
    }

    result.average_rf_time = total_rf_time / rf_count;
    result.average_parse_time = total_parser_time / kSampleSize;
    std::cout << "Average rf time: " << result.average_rf_time << std::endl;
    std::cout << "Average full parse time: " << result.average_parse_time << std::endl;
    return result;
}

void Sparser::Run(const std::string& input_path, const JsonQuery& json_query) {
    std::cout << "Running Sparser\n";
    SparserConfig config{.input_path = input_path, .json_query = json_query};
    config.PrintConfig();
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

    auto stats = SearchCascade(sparser_input, json_query, rf_data, best_cascade);

    auto sparser_time = benchmark_stop(sparser_time_start);

    std::cout << "Best cascade:\n";
    PrettyPrint(best_cascade, rf_data);
    std::cout << "Best cascade cost: " << min_cost << "\n\n";

    printf("Sparser:\t\x1b[1;33mResult: %ld (Execution Time: %f seconds)\x1b[0m\n", stats.records_matched,
           sparser_time);

    stats.PrintStats();

    auto naive_time_start = benchmark_start();

    auto naive_input = input_reader.ReadRecords(file_data);
    auto naive_stats = SearchNaive(naive_input, json_query);
    auto naive_time = benchmark_stop(naive_time_start);

    printf("Naive:\t\x1b[1;33mResult: %ld (Execution Time: %f seconds)\x1b[0m\n", naive_stats.callback_passed,
           naive_time);
}

SparserSearchStats Sparser::SearchCascade(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                                          const RawFilterData& rf_data, const std::shared_ptr<Node> node) {
    size_t sparser_match = 0;
    size_t sparser_count = 0;

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
            sparser_match++;
            if (json_query_driver_->RunQuery(record, json_query)) {
                sparser_count++;
            }
        }
    }

    return SparserSearchStats{
        .records_processed = input.size(),
        .records_matched = sparser_match,
        .callback_passed = sparser_count,
        .fraction_true_positive = static_cast<double>(sparser_count) / static_cast<double>(sparser_match),
        .fraction_false_positive =
            static_cast<double>(sparser_match - sparser_count) / static_cast<double>(sparser_match),
    };
}

NaiveSearchStats Sparser::SearchNaive(const std::vector<std::string_view>& input, const JsonQuery& json_query) {
    size_t true_count = 0;

    for (const auto& record : input) {
        if (json_query_driver_->RunQuery(record, json_query)) {
            true_count++;
        }
    }

    return NaiveSearchStats{
        .records_processed = input.size(),
        .callback_passed = true_count,
    };
}

void SparserSearchStats::PrintStats() const {
    std::cout << "Records processed: " << records_processed << "\n";
    std::cout << "Records matched: " << records_matched << "\n";
    std::cout << "Records passing callback: " << callback_passed << "\n";
    std::cout << "True positive rate: " << fraction_true_positive << "\n";
    std::cout << "False positive rate: " << fraction_false_positive << "\n\n";
}

void SparserConfig::PrintConfig() const {
    std::cout << "Input path: " << input_path << "\n";
    std::cout << "Json query: " << json_query.ToString() << "\n";
    std::cout << "RF size: " << rf_size << "\n";
    std::cout << "Sample size: " << sample_size << "\n";
    std::cout << "Max depth: " << max_depth << "\n";
    std::cout << "Max rfs in pred: " << max_rfs_in_pred << "\n";
    std::cout << "Max pred: " << max_pred << "\n";
    std::cout << "Max conj: " << max_conj << "\n\n";
}
