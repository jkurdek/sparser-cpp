#include "sparser.h"

#include <sstream>
#include <string>

std::string SparserQuery::ToString() const {
    std::ostringstream oss;
    for (const auto& conjunction : disjunction_.conjunctions) {
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
        if (&conjunction != &disjunction_.conjunctions.back()) {
            oss << " ∨ ";
        }
    }
    oss << "\n";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const SparserQuery& query) {
    os << query.ToString();
    return os;
}

bool RawFilter::operator==(const RawFilter& other) const {
    return value_ == other.value_ && conjunctive_index_ == other.conjunctive_index_ &&
           predicate_index_ == other.predicate_index_;
}

std::string RawFilter::ToString() const {
    std::ostringstream oss;
    oss << "RawFilter(value: " << value_ << ", conjunctiveIndex: " << conjunctive_index_
        << ", predicateIndex: " << predicate_index_ << ")";
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const RawFilter& filter) {
    os << filter.ToString();
    return os;
}

RawFilterData RawFilterQueryGenerator::GenerateRawFilters(const PredicateDisjunction& disjunction) {
    RawFilterData raw_filter_data;
    for (size_t conjunctive_index = 0; conjunctive_index < disjunction.conjunctions.size(); ++conjunctive_index) {
        const auto& conjunction = disjunction.conjunctions[conjunctive_index];
        for (size_t predicate_index = 0; predicate_index < conjunction.predicates.size(); ++predicate_index) {
            const auto& predicate = conjunction.predicates[predicate_index];
            const auto filters = GenerateRawFiltersFromPredicate(predicate.value);
            for (const auto& filter : filters) {
                raw_filter_data.raw_filters.push_back(filter);
                raw_filter_data.conjunctive_indices.push_back(conjunctive_index);
                raw_filter_data.predicate_indices.push_back(predicate_index);
            }
        }
    }
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
