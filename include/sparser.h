#ifndef SPARSER_H_
#define SPARSER_H_

#include <array>
#include <bitset>
#include <cstddef>
#include <string_view>
#include <vector>

#include "json_facade.h"

constexpr size_t kRfSize = 4;
constexpr size_t kSampleSize = 10;
constexpr size_t kMaxRfs = 32;

struct EstimationResult {
    std::array<double, kMaxRfs> total_rf_runtimes;
    double total_parser_runtime;
    std::array<std::bitset<kMaxRfs>, kSampleSize> bitsets;
};

struct RawFilterData {
    size_t size;
    std::vector<std::string_view> raw_filters;
    std::vector<size_t> conjunctive_indices;
    std::vector<size_t> predicate_indices;
};

class RawFilterQueryGenerator {
   public:
    static RawFilterData GenerateRawFilters(const PredicateDisjunction& disjunction);
    static std::vector<std::string_view> GenerateRawFiltersFromPredicate(const std::string_view& input);
};

class Sparser {
   public:
    explicit Sparser(std::unique_ptr<JsonQueryDriver>&& json_query_driver = {})
        : json_query_driver_(std::move(json_query_driver)) {}
    void calibrate(const std::vector<std::string_view>& input, RawFilterData raw_filter_data);

   private:
    std::unique_ptr<JsonQueryDriver> json_query_driver_;
};

#endif  // SPARSER_H_
