#pragma once

#include <string>
#include <vector>

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
};
