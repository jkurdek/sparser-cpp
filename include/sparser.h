#ifndef SPARSER_H_
#define SPARSER_H_

#include <array>
#include <bitset>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

constexpr size_t kRfSize = 4;
constexpr size_t kSampleSize = 10;
constexpr size_t kMaxRfs = 32;

struct EstimationResult {
    std::array<double, kMaxRfs> total_rf_runtimes;
    double total_parser_runtime;
    std::array<std::bitset<kMaxRfs>, kSampleSize> bitsets;
};

struct Predicate {
    std::string value;
    std::string key;
};

struct PredicateConjunction {
    std::vector<Predicate> predicates;
};

struct PredicateDisjunction {
    std::vector<PredicateConjunction> conjunctions;
};

class JsonQuery {
   private:
    PredicateDisjunction disjunction_;

   public:
    explicit JsonQuery(const PredicateDisjunction& disjunction) : disjunction_(disjunction) {}

    [[nodiscard]] const inline PredicateDisjunction& GetDisjunction() const { return disjunction_; }
    [[nodiscard]] std::string ToString() const;

    friend std::ostream& operator<<(std::ostream& os, const JsonQuery& query);
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
    void calibrate(const std::vector<std::string_view>& input, RawFilterData raw_filter_data);
};

#endif  // SPARSER_H_
