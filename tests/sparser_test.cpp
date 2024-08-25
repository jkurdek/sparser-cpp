#include "sparser.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

RawFilter createRawFilter(const std::string_view& value, size_t conjunctiveIndex, size_t predicateIndex) {
    return RawFilter{value, conjunctiveIndex, predicateIndex};
}

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

TEST(SparserQueryTest, GenerateRawFiltersForQueryTest) {
    const Predicate pred1 = {"Lord of the Rings"};
    const Predicate pred2 = {"Harry Potter"};
    const Predicate pred3 = {"The Hobbit"};

    const PredicateConjunction conj1 = {{pred1, pred2}};
    const PredicateConjunction conj2 = {{pred3}};
    const PredicateDisjunction disj = {{conj1, conj2}};
    const SparserQuery query = {disj};

    const std::vector<RawFilter> expected = {
        createRawFilter("Lord", 0, 0), createRawFilter("ord ", 0, 0), createRawFilter("rd o", 0, 0),
        createRawFilter("d of", 0, 0), createRawFilter(" of ", 0, 0), createRawFilter("of t", 0, 0),
        createRawFilter("f th", 0, 0), createRawFilter(" the", 0, 0), createRawFilter("the ", 0, 0),
        createRawFilter("he R", 0, 0), createRawFilter("e Ri", 0, 0), createRawFilter(" Rin", 0, 0),
        createRawFilter("Ring", 0, 0), createRawFilter("ings", 0, 0), createRawFilter("Harr", 0, 1),
        createRawFilter("arry", 0, 1), createRawFilter("rry ", 0, 1), createRawFilter("ry P", 0, 1),
        createRawFilter("y Po", 0, 1), createRawFilter(" Pot", 0, 1), createRawFilter("Pott", 0, 1),
        createRawFilter("otte", 0, 1), createRawFilter("tter", 0, 1), createRawFilter("The ", 1, 0),
        createRawFilter("he H", 1, 0), createRawFilter("e Ho", 1, 0), createRawFilter(" Hob", 1, 0),
        createRawFilter("Hobb", 1, 0), createRawFilter("obbi", 1, 0), createRawFilter("bbit", 1, 0),
    };

    const auto actual = RawFilterQueryGenerator::generateRawFilters(query);

    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(expected[i], actual[i]);
    }
}

TEST(SparserQueryTest, GenerateRawFiltersForSinglePredicate) {
    const Predicate pred1 = {"Harry Potter"};

    const std::vector<std::string_view> expected = {
        "Harr", "arry", "rry ", "ry P", "y Po", " Pot", "Pott", "otte", "tter",
    };

    const auto actual = RawFilterQueryGenerator::generateRawFilters(pred1.value);

    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(expected[i], actual[i]);
    }
}
