#ifndef SPARSER_H_
#define SPARSER_H_

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

constexpr size_t kRfSize = 4;

struct Predicate {
    std::string value;
};

struct PredicateConjunction {
    std::vector<Predicate> predicates;
};

struct PredicateDisjunction {
    std::vector<PredicateConjunction> conjunctions;
};

class SparserQuery {
   private:
    PredicateDisjunction disjunction_;

   public:
    explicit SparserQuery(const PredicateDisjunction& disjunction) : disjunction_(disjunction) {}

    [[nodiscard]] const PredicateDisjunction& get_disjunction() const { return disjunction_; }

    [[nodiscard]] std::string ToString() const;
    friend std::ostream& operator<<(std::ostream& os, const SparserQuery& query);
};

struct RawFilterData {
    std::vector<std::string_view> raw_filters;
    std::vector<size_t> conjunctive_indices;
    std::vector<size_t> predicate_indices;
};

class RawFilter {
   private:
    std::string_view value_;
    size_t conjunctive_index_;
    size_t predicate_index_;

   public:
    explicit RawFilter(const std::string_view& value, size_t conjunctive_index, size_t predicate_index)
        : value_(value), conjunctive_index_(conjunctive_index), predicate_index_(predicate_index) {}

    [[nodiscard]] std::string_view get_value() const { return value_; }
    [[nodiscard]] size_t get_conjunctive_index() const { return conjunctive_index_; }
    [[nodiscard]] size_t get_predicate_index() const { return predicate_index_; }

    bool operator==(const RawFilter& other) const;

    [[nodiscard]] std::string ToString() const;
    friend std::ostream& operator<<(std::ostream& os, const RawFilter& filter);
};

class RawFilterQueryGenerator {
   public:
    static RawFilterData GenerateRawFilters(const PredicateDisjunction& disjunction);
    static std::vector<std::string_view> GenerateRawFiltersFromPredicate(const std::string_view& input);
};

#endif  // SPARSER_H_
