#include "sparser.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "common.h"

std::string JsonQuery::ToString() const {
    std::ostringstream oss;
    for (const auto& conjunction : disjunction_.conjunctions) {
        if (!conjunction.predicates.empty()) {
            oss << "(";
        }
        for (const auto& predicate : conjunction.predicates) {
            oss << predicate.key << ": " << predicate.value;
            if (&predicate != &conjunction.predicates.back()) {
                oss << " ∧ ";
            }
        }
        if (!conjunction.predicates.empty()) {
            oss << ")";
        }
        if (&conjunction != &disjunction_.conjunctions.back()) {
            oss << " ∨ ";
        }
    }
    oss << "\n";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const JsonQuery& query) {
    os << query.ToString();
    return os;
}

RawFilterData RawFilterQueryGenerator::GenerateRawFilters(const PredicateDisjunction& disjunction) {
    RawFilterData raw_filter_data;
    for (size_t conj_idx = 0; conj_idx < disjunction.conjunctions.size(); ++conj_idx) {
        const auto& conjunction = disjunction.conjunctions[conj_idx];
        for (size_t pred_idx = 0; pred_idx < conjunction.predicates.size(); ++pred_idx) {
            const auto& predicate = conjunction.predicates[pred_idx];
            const auto filters = GenerateRawFiltersFromPredicate(predicate.value);
            for (const auto& filter : filters) {
                raw_filter_data.raw_filters.push_back(filter);
                raw_filter_data.conjunctive_indices.push_back(conj_idx);
                raw_filter_data.predicate_indices.push_back(pred_idx);
            }
        }
    }
    raw_filter_data.size = raw_filter_data.raw_filters.size();
    return raw_filter_data;
}

std::vector<std::string_view> RawFilterQueryGenerator::GenerateRawFiltersFromPredicate(
    const std::string_view& predicate) {
    std::vector<std::string_view> rawFilters;
    for (size_t i = 0; i < predicate.size() - kRfSize + 1; ++i) {
        rawFilters.emplace_back(predicate.substr(i, kRfSize));
    }
    return rawFilters;
}

void Sparser::calibrate(const std::vector<std::string_view>& input, RawFilterData raw_filter_data) {
    auto result = EstimationResult{};

    assert(input.size() >= kSampleSize);

    for (size_t i = 0; i < kSampleSize; i++) {
        auto json_row = input[i];
        for (size_t rf_idx = 0; rf_idx < kMaxRfs; rf_idx++) {
            auto rf = raw_filter_data.raw_filters[rf_idx];
            std::cout << "Grepping... : " << rf << "\n";

            auto grepStart = benchmark_start();
            auto x = json_row.find(rf);
            result.total_rf_runtimes[i] += benchmark_stop(grepStart);
            if (x != std::string_view::npos) {
                std::cout << "Found: " << rf << "\n";
                result.bitsets[rf_idx].set(i);
            } else {
                std::cout << "Not found: " << rf << "\n";
            }

            // Run the parser here
            result.total_parser_runtime += 0;
        }
    }
}
