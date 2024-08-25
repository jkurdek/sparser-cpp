#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

constexpr size_t RFSZ = 4;

struct Predicate {
    std::string value;
};

struct PredicateConjunction {
    std::vector<Predicate> predicates;
};

struct PredicateDisjunction {
    std::vector<PredicateConjunction> conjunctions;
};

struct SparserQuery {
    PredicateDisjunction disjunction;

    [[nodiscard]] std::string generateOutput() const;
    friend std::ostream& operator<<(std::ostream& os, const SparserQuery& query);
};

struct RawFilter {
    RawFilter(const std::string_view& value, size_t conjunctiveIndex, size_t predicateIndex)
        : value(value), conjunctiveIndex(conjunctiveIndex), predicateIndex(predicateIndex) {}

    std::string_view value;
    size_t conjunctiveIndex;
    size_t predicateIndex;

    bool operator==(const RawFilter& other) const;

    [[nodiscard]] std::string generateOutput() const;
    friend std::ostream& operator<<(std::ostream& os, const RawFilter& filter);
};

struct RawFilterQuery {
    std::unique_ptr<SparserQuery> query;
    std::vector<RawFilter> rawFilters;
};

class RawFilterQueryGenerator {
   public:
    static RawFilterQuery generateRawFilterQuery(std::unique_ptr<SparserQuery> query);

    static std::vector<RawFilter> generateRawFilters(const SparserQuery& query);

    static std::vector<std::string_view> generateRawFilters(const std::string_view& input);
};
