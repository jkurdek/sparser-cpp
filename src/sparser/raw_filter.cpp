#include "raw_filter.h"

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
