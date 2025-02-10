#ifndef RAW_FILTER_H_
#define RAW_FILTER_H_

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

#include "config.h"
#include "json_facade.h"

struct RawFilterData {
    std::array<std::array<std::array<std::string_view, kMaxRfsInPred>, kMaxPred>, kMaxConj> data;
    std::array<std::array<size_t, kMaxPred>, kMaxConj> rf_count = {};
    std::array<size_t, kMaxConj> pred_count = {};
    size_t conj_count = 0;
};

class RawFilterQueryGenerator {
   public:
    [[nodiscard]] static RawFilterData GenerateRawFilters(const PredicateDisjunction& disjunction);
    [[nodiscard]] static std::vector<std::string_view> GenerateRawFiltersFromPredicate(
        const std::string_view& predicate);
};

[[nodiscard]] inline size_t GetFlatIdx(size_t conj_idx, size_t pred_idx, size_t rf_idx) {
    return (conj_idx * kMaxPred * kMaxRfsInPred) + (pred_idx * kMaxRfsInPred) + rf_idx;
}

#endif  // RAW_FILTER_H_
