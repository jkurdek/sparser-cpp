#include "sparser.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include "json_facade.h"

TEST(SparserQueryTest, ToString) {
    const Predicate pred_1{.key = "name", .value = "John Doe"};
    const Predicate pred_2{.key = "age", .value = "30"};
    const Predicate pred_3{.key = "city", .value = "New York"};
    const Predicate pred_4{.key = "country", .value = "USA"};

    const PredicateConjunction conj_1{{pred_1, pred_2}};
    const PredicateConjunction conj_2{{pred_3, pred_4}};

    const PredicateDisjunction disj{{conj_1, conj_2}};
    const JsonQuery query{disj};

    const std::string expected{"(name: John Doe ∧ age: 30) ∨ (city: New York ∧ country: USA)\n"};
    auto actual = query.ToString();

    ASSERT_EQ(expected, actual);
}

TEST(SparserQueryTest, GenerateRawFiltersForQueryTests) {
    const Predicate pred_1{.key = "title", .value = "Lord of the Rings"};
    const Predicate pred_2{.key = "title", .value = "Harry Potter"};
    const Predicate pred_3{.key = "title", .value = "The Hobbit"};

    const PredicateConjunction conj_1{{pred_1, pred_2}};
    const PredicateConjunction conj_2{{pred_3}};

    const PredicateDisjunction disj{{conj_1, conj_2}};

    const RawFilterDisjunction expected{
        .conjunctions =
            {
                RawFilterConjunction{
                    .predicates =
                        {
                            RawFilterPredicate{
                                .raw_filters = {"Lord", "ord ", "rd o", "d of", " of ", "of t", "f th", " the", "the ",
                                                "he R", "e Ri", " Rin", "Ring", "ings"},
                            },
                            RawFilterPredicate{
                                .raw_filters = {"Harr", "arry", "rry ", "ry P", "y Po", " Pot", "Pott", "otte", "tter"},
                            },
                        },
                },
                RawFilterConjunction{
                    .predicates =
                        {
                            RawFilterPredicate{
                                .raw_filters = {"The ", "he H", "e Ho", " Hob", "Hobb", "obbi", "bbit"},
                            },
                        },
                },
            },
    };

    const auto actual = RawFilterQueryGenerator::GenerateRawFilters(disj);

    ASSERT_EQ(expected.conjunctions.size(), actual.conjunctions.size());
    for (size_t conj_idx = 0; conj_idx < expected.conjunctions.size(); ++conj_idx) {
        const auto& expected_conjunction = expected.conjunctions[conj_idx];
        const auto& actual_conjunction = actual.conjunctions[conj_idx];

        ASSERT_EQ(expected_conjunction.predicates.size(), actual_conjunction.predicates.size());
        for (size_t pred_idx = 0; pred_idx < expected_conjunction.predicates.size(); ++pred_idx) {
            const auto& expected_predicate = expected_conjunction.predicates[pred_idx];
            const auto& actual_predicate = actual_conjunction.predicates[pred_idx];

            ASSERT_EQ(expected_predicate.raw_filters.size(), actual_predicate.raw_filters.size());
            for (size_t rf_idx = 0; rf_idx < expected_predicate.raw_filters.size(); ++rf_idx) {
                ASSERT_EQ(expected_predicate.raw_filters[rf_idx], actual_predicate.raw_filters[rf_idx]);
            }
        }
    }
}

TEST(SparserQueryTest, GenerateRawFiltersForSinglePredicate) {
    const Predicate pred_1{.key = "title", .value = "Harry Potter"};

    const std::vector<std::string_view> expected{
        "Harr", "arry", "rry ", "ry P", "y Po", " Pot", "Pott", "otte", "tter",
    };

    const auto actual = RawFilterQueryGenerator::GenerateRawFiltersFromPredicate(pred_1.value);

    ASSERT_EQ(expected.size(), actual.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        ASSERT_EQ(expected[i], actual[i]);
    }
}

TEST(RapidJsonFacadeTest, ParseValidJson) {
    RapidJsonFacade facade;
    std::string_view validJson = R"({"name":"John","age":"30"})";
    facade.Parse(validJson);

    auto nameValue = facade.GetString("name");
    ASSERT_TRUE(nameValue.has_value());
    EXPECT_EQ(*nameValue, "John");

    auto ageValue = facade.GetString("age");
    ASSERT_TRUE(ageValue.has_value());
    EXPECT_EQ(*ageValue, "30");
}

TEST(RapidJsonFacadeTest, ParseInvalidJson) {
    RapidJsonFacade facade;
    std::string_view invalidJson = R"({invalid json})";  // Missing quotes, braces, etc.
    EXPECT_ANY_THROW(facade.Parse(invalidJson)) << "Should fail to parse invalid JSON";
    EXPECT_FALSE(facade.GetString("randomKey").has_value());
}

TEST(RapidJsonFacadeTest, HasKeyAndGetString) {
    RapidJsonFacade facade;
    std::string_view json = R"({"fruit":"apple"})";
    facade.Parse(json);

    auto fruitVal = facade.GetString("fruit");
    ASSERT_TRUE(fruitVal.has_value());
    EXPECT_EQ(*fruitVal, "apple");

    auto colorVal = facade.GetString("color");
    EXPECT_FALSE(colorVal.has_value());
}

TEST(JsonQueryDriverTest, RunQuery_AllPredicatesMatch) {
    auto facade = std::make_unique<RapidJsonFacade>();
    JsonQueryDriver driver(std::move(facade));

    std::string_view testJson = R"({"name":"John Doe","age":"30","city":"New York"})";

    Predicate pred1{.key = "name", .value = "John Doe"};
    Predicate pred2{.key = "age", .value = "30"};
    Predicate pred3{.key = "city", .value = "New York"};

    PredicateConjunction conj1{{pred1, pred2}};
    PredicateConjunction conj2{{pred3}};
    PredicateDisjunction disj{{conj1, conj2}};
    JsonQuery query(disj);

    bool result = driver.RunQuery(testJson, query);
    EXPECT_TRUE(result) << "Expected the query to match since the JSON satisfies conjunction 1.";
}

TEST(JsonQueryDriverTest, RunQuery_NoPredicatesMatch) {
    auto facade = std::make_unique<RapidJsonFacade>();
    JsonQueryDriver driver(std::move(facade));

    std::string_view testJson = R"({"name":"Alice","age":"25","city":"Chicago"})";

    Predicate pred1{.key = "name", .value = "John Doe"};
    Predicate pred2{.key = "age", .value = "30"};
    Predicate pred3{.key = "city", .value = "New York"};

    PredicateConjunction conj1{{pred1, pred2}};
    PredicateConjunction conj2{{pred3}};
    PredicateDisjunction disj{{conj1, conj2}};
    JsonQuery query(disj);

    bool result = driver.RunQuery(testJson, query);
    EXPECT_FALSE(result) << "Expected the query NOT to match since none of the predicates match.";
}

TEST(JsonQueryDriverTest, RunQuery_PartialConjunctionFail) {
    auto facade = std::make_unique<RapidJsonFacade>();
    JsonQueryDriver driver(std::move(facade));

    std::string_view testJson = R"({"name":"John Doe","age":"25"})";

    Predicate pred1{.key = "name", .value = "John Doe"};
    Predicate pred2{.key = "age", .value = "30"};

    PredicateConjunction conj1{{pred1, pred2}};
    PredicateDisjunction disj{{conj1}};
    JsonQuery query(disj);

    bool result = driver.RunQuery(testJson, query);
    EXPECT_FALSE(result) << "Expected the query NOT to match because the age mismatch fails the conjunction.";
}

TEST(CascadeBuilderTest, GeneratesCorrectNumberOfCascades) {
    Predicate pred1{.key = "name", .value = "John"};
    Predicate pred2{.key = "region", .value = "EMEA"};
    Predicate pred3{.key = "name", .value = "Jane"};

    PredicateConjunction conj1{{pred1, pred2}};
    PredicateConjunction conj2{{
        pred3,
    }};
    PredicateDisjunction disj{{conj1, conj2}};

    RawFilterDisjunction raw_filter_data = RawFilterQueryGenerator::GenerateRawFilters(disj);

    CascadeBuilder builder(disj, raw_filter_data);
    auto valid_cascades = builder.GenerateValidCascades();

    ASSERT_EQ(8, valid_cascades.size());
}
