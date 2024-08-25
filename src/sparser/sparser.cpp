#include "sparser.h"

#include <sstream>
#include <string>

std::string SparserQuery::generateOutput() const {
    std::ostringstream oss;
    for (const auto& conjunction : disjunction.conjunctions) {
        if (!conjunction.predicates.empty()) {
            oss << "(";
        }
        for (const auto& predicate : conjunction.predicates) {
            oss << predicate.value;
            if (&predicate != &conjunction.predicates.back()) {
                oss << " ∧ ";
            }
        }
        if (!conjunction.predicates.empty()) {
            oss << ")";
        }
        if (&conjunction != &disjunction.conjunctions.back()) {
            oss << " ∨ ";
        }
    }
    oss << "\n";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const SparserQuery& query) {
    os << query.generateOutput();
    return os;
}

bool RawFilter::operator==(const RawFilter& other) const {
    return value == other.value && conjunctiveIndex == other.conjunctiveIndex && predicateIndex == other.predicateIndex;
}

std::string RawFilter::generateOutput() const {
    std::ostringstream oss;
    oss << "RawFilter(value: " << value << ", conjunctiveIndex: " << conjunctiveIndex
        << ", predicateIndex: " << predicateIndex << ")";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const RawFilter& filter) {
    os << filter.generateOutput();
    return os;
}

RawFilterQuery RawFilterQueryGenerator::generateRawFilterQuery(std::unique_ptr<SparserQuery> query) {
    return RawFilterQuery{std::move(query), generateRawFilters(*query)};
}

std::vector<RawFilter> RawFilterQueryGenerator::generateRawFilters(const SparserQuery& query) {
    std::vector<RawFilter> rawFilters;
    for (size_t conjunctiveIndex = 0; conjunctiveIndex < query.disjunction.conjunctions.size(); ++conjunctiveIndex) {
        for (size_t predicateIndex = 0;
             predicateIndex < query.disjunction.conjunctions[conjunctiveIndex].predicates.size(); ++predicateIndex) {
            const auto& predicate = query.disjunction.conjunctions[conjunctiveIndex].predicates[predicateIndex];
            const auto filters = generateRawFilters(predicate.value);
            for (const auto& filter : filters) {
                rawFilters.emplace_back(filter, conjunctiveIndex, predicateIndex);
            }
        }
    }
    return rawFilters;
}

std::vector<std::string_view> RawFilterQueryGenerator::generateRawFilters(const std::string_view& input) {
    std::vector<std::string_view> rawFilters;
    for (size_t i = 0; i < input.size() - RFSZ + 1; ++i) {
        rawFilters.emplace_back(input.substr(i, RFSZ));
    }
    return rawFilters;
}
