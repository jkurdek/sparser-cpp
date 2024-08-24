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
