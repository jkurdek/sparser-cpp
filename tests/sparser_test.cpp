#include "sparser.h"

#include <gtest/gtest.h>

#include <string>

TEST(SparserQueryTest, GenerateOutput) {
    const Predicate pred1 = {"p1"};
    const Predicate pred2 = {"p2"};
    const Predicate pred3 = {"p3"};
    const Predicate pred4 = {"p4"};

    const PredicateConjunction conj1 = {{pred1, pred2}};
    const PredicateConjunction conj2 = {{pred3, pred4}};

    const PredicateDisjunction disj = {{conj1, conj2}};
    const SparserQuery query = {disj};

    const std::string expected = "(p1 ∧ p2) ∨ (p3 ∧ p4)\n";
    auto actual = query.generateOutput();

    ASSERT_EQ(expected, actual);
}
