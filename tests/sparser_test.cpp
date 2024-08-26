#include "sparser.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

TEST(SparserQueryTest, ToString) {
    const Predicate pred_1{"p1"};
    const Predicate pred_2{"p2"};
    const Predicate pred_3{"p3"};
    const Predicate pred_4{"p4"};

    const PredicateConjunction conj_1{{pred_1, pred_2}};
    const PredicateConjunction conj_2{{pred_3, pred_4}};

    const PredicateDisjunction disj{{conj_1, conj_2}};
    const SparserQuery query{disj};

    const std::string expected{"(p1 ∧ p2) ∨ (p3 ∧ p4)\n"};
    auto actual = query.ToString();

    ASSERT_EQ(expected, actual);
}

TEST(SparserQueryTest, GenerateRawFiltersForQueryTest) {
    const Predicate pred_1{"Lord of the Rings"};
    const Predicate pred_2{"Harry Potter"};
    const Predicate pred_3{"The Hobbit"};

    const PredicateConjunction conj_1{{pred_1, pred_2}};
    const PredicateConjunction conj_2{{pred_3}};
    const PredicateDisjunction disj{{conj_1, conj_2}};
    const SparserQuery query{disj};

    const std::vector<std::string_view> expected_filters{"Lord", "ord ", "rd o", "d of", " of ", "of t", "f th", " the",
                                                         "the ", "he R", "e Ri", " Rin", "Ring", "ings", "Harr", "arry",
                                                         "rry ", "ry P", "y Po", " Pot", "Pott", "otte", "tter", "The ",
                                                         "he H", "e Ho", " Hob", "Hobb", "obbi", "bbit"};

    const std::vector<size_t> expected_conjunctive_indices{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                           0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1};

    const std::vector<size_t> expected_predicate_indices{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                                                         1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

    const auto actual = RawFilterQueryGenerator::GenerateRawFilters(query.get_disjunction());

    ASSERT_EQ(expected_filters.size(), actual.raw_filters.size());
    for (size_t i = 0; i < expected_filters.size(); ++i) {
        ASSERT_EQ(expected_filters[i], actual.raw_filters[i]);
    }

    ASSERT_EQ(expected_conjunctive_indices.size(), actual.conjunctive_indices.size());
    for (size_t i = 0; i < expected_conjunctive_indices.size(); ++i) {
        ASSERT_EQ(expected_conjunctive_indices[i], actual.conjunctive_indices[i]);
    }

    ASSERT_EQ(expected_predicate_indices.size(), actual.predicate_indices.size());
    for (size_t i = 0; i < expected_predicate_indices.size(); ++i) {
        ASSERT_EQ(expected_predicate_indices[i], actual.predicate_indices[i]);
    }
}

TEST(SparserQueryTest, GenerateRawFiltersForSinglePredicate) {
    const Predicate pred_1{"Harry Potter"};

    const std::vector<std::string_view> expected{
        "Harr", "arry", "rry ", "ry P", "y Po", " Pot", "Pott", "otte", "tter",
    };

    const auto actual = RawFilterQueryGenerator::GenerateRawFiltersFromPredicate(pred_1.value);

    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(expected[i], actual[i]);
    }
}
