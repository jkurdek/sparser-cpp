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

TEST(SparserQueryTest, GenerateRawFiltersForQueryTest) {
    const Predicate pred_1{.key = "title", .value = "Lord of the Rings"};
    const Predicate pred_2{.key = "title", .value = "Harry Potter"};
    const Predicate pred_3{.key = "title", .value = "The Hobbit"};

    const PredicateConjunction conj_1{{pred_1, pred_2}};
    const PredicateConjunction conj_2{{pred_3}};
    const PredicateDisjunction disj{{conj_1, conj_2}};
    const JsonQuery query{disj};

    const std::vector<std::string_view> expected_filters{"Lord", "ord ", "rd o", "d of", " of ", "of t", "f th", " the",
                                                         "the ", "he R", "e Ri", " Rin", "Ring", "ings", "Harr", "arry",
                                                         "rry ", "ry P", "y Po", " Pot", "Pott", "otte", "tter", "The ",
                                                         "he H", "e Ho", " Hob", "Hobb", "obbi", "bbit"};

    const std::vector<size_t> expected_conjunctive_indices{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                           0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1};

    const std::vector<size_t> expected_predicate_indices{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
                                                         1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};

    const auto actual = RawFilterQueryGenerator::GenerateRawFilters(query.GetDisjunction());

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
    EXPECT_TRUE(facade.Parse(validJson)) << "Failed to parse a valid JSON string";

    EXPECT_TRUE(facade.HasKey("name"));
    EXPECT_TRUE(facade.HasKey("age"));

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
    EXPECT_FALSE(facade.Parse(invalidJson)) << "Should fail to parse invalid JSON";
    EXPECT_FALSE(facade.HasKey("randomKey"));
    EXPECT_FALSE(facade.GetString("randomKey").has_value());
}

TEST(RapidJsonFacadeTest, HasKeyAndGetString) {
    RapidJsonFacade facade;
    std::string_view json = R"({"fruit":"apple"})";
    EXPECT_TRUE(facade.Parse(json));

    EXPECT_TRUE(facade.HasKey("fruit"));
    auto fruitVal = facade.GetString("fruit");
    ASSERT_TRUE(fruitVal.has_value());
    EXPECT_EQ(*fruitVal, "apple");

    EXPECT_FALSE(facade.HasKey("color"));
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
