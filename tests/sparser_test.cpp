#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "cascade_builder.h"
#include "cascade_evaluator.h"
#include "json_facade.h"
#include "node.h"

TEST(JsonQuery, ToString_ReturnsCorrectFormat) {
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

TEST(RawFilterQueryGenerator, GenerateRawFilters_ProperlyGeneratesRawFilters) {
    const Predicate pred_1{.key = "title", .value = "Lord of the Rings"};
    const Predicate pred_2{.key = "title", .value = "Harry Potter"};
    const Predicate pred_3{.key = "title", .value = "The Hobbit"};

    const PredicateConjunction conj_1{{pred_1, pred_2}};
    const PredicateConjunction conj_2{{pred_3}};

    const PredicateDisjunction disj{{conj_1, conj_2}};

    auto expected = std::array<std::array<std::array<std::string_view, kMaxRfsInPred>, kMaxPred>, kMaxConj>();

    auto expected_conj_1_pred_1 = std::array<std::string_view, kMaxRfsInPred>{
        "Lord", "ord ", "rd o", "d of", " of ", "of t", "f th", " the", "the ", "he R", "e Ri", " Rin", "Ring", "ings"};

    auto expected_conj_1_pred_2 = std::array<std::string_view, kMaxRfsInPred>{"Harr", "arry", "rry ", "ry P", "y Po",
                                                                              " Pot", "Pott", "otte", "tter"};

    auto expected_conj_2_pred_1 =
        std::array<std::string_view, kMaxRfsInPred>{"The ", "he H", "e Ho", " Hob", "Hobb", "obbi", "bbit"};

    expected[0][0] = expected_conj_1_pred_1;
    expected[0][1] = expected_conj_1_pred_2;
    expected[1][0] = expected_conj_2_pred_1;

    const RawFilterData expected_rf_data{
        .data = expected,
        .rf_count = {{{14, 9}, {7}}},
        .pred_count = {2, 1},
        .conj_count = 2,
    };

    const auto actual = RawFilterQueryGenerator::GenerateRawFilters(disj);

    ASSERT_EQ(expected_rf_data.conj_count, actual.conj_count);
    for (size_t conj_idx = 0; conj_idx < expected_rf_data.conj_count; ++conj_idx) {
        ASSERT_EQ(expected_rf_data.pred_count[conj_idx], actual.pred_count[conj_idx]);
        for (size_t pred_idx = 0; pred_idx < expected_rf_data.pred_count[conj_idx]; ++pred_idx) {
            ASSERT_EQ(expected_rf_data.rf_count[conj_idx][pred_idx], actual.rf_count[conj_idx][pred_idx]);
            for (size_t rf_idx = 0; rf_idx < expected_rf_data.rf_count[conj_idx][pred_idx]; ++rf_idx) {
                ASSERT_EQ(expected_rf_data.data[conj_idx][pred_idx][rf_idx], actual.data[conj_idx][pred_idx][rf_idx]);
            }
        }
    }
}

TEST(RawFilterQueryGenerator, GenerateRawFilters_GenerateRawFiltersForSinglePredicate) {
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

TEST(RapidJsonFacade, Parse_ValidJson_SuccessfulParsing) {
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

TEST(RapidJsonFacade, Parse_InvalidJson_ThrowsException) {
    RapidJsonFacade facade;
    std::string_view invalidJson = R"({invalid json})";  // Missing quotes, braces, etc.
    EXPECT_ANY_THROW(facade.Parse(invalidJson)) << "Should fail to parse invalid JSON";
    EXPECT_FALSE(facade.GetString("randomKey").has_value());
}

TEST(RapidJsonFacade, GetString_ExistingAndMissingKeys_BehavesCorrectly) {
    RapidJsonFacade facade;
    std::string_view json = R"({"fruit":"apple"})";
    facade.Parse(json);

    auto fruitVal = facade.GetString("fruit");
    ASSERT_TRUE(fruitVal.has_value());
    EXPECT_EQ(*fruitVal, "apple");

    auto colorVal = facade.GetString("color");
    EXPECT_FALSE(colorVal.has_value());
}

TEST(JsonQueryDriver, RunQuery_AllPredicatesMatch_ReturnsTrue) {
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

TEST(JsonQueryDriver, RunQuery_NoPredicatesMatch_ReturnsFalse) {
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

TEST(JsonQueryDriver, RunQuery_PartialConjunctionMismatch_ReturnsFalse) {
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

TEST(CascadeBuilder, GenerateValidCascades_ValidDisjunction_ReturnsExpectedCount) {
    Predicate pred1{.key = "name", .value = "John"};
    Predicate pred2{.key = "region", .value = "EMEA"};
    Predicate pred3{.key = "name", .value = "Jane"};

    PredicateConjunction conj1{{pred1, pred2}};
    PredicateConjunction conj2{{
        pred3,
    }};
    PredicateDisjunction disj{{conj1, conj2}};

    RawFilterData raw_filter_data = RawFilterQueryGenerator::GenerateRawFilters(disj);

    CascadeBuilder builder(disj, raw_filter_data);
    auto valid_cascades = builder.GenerateValidCascades();

    ASSERT_EQ(8, valid_cascades.size());
}

void ValidateFailNodePaths(const std::shared_ptr<Node>& root, std::vector<bool>& conj_used,
                           const size_t total_conjunctions, bool& test_failure) {
    if (!root) {
        throw std::runtime_error("Root node is null.");
    }

    if (root->type == NodeType::INTER) {
        if (root->conjunction_idx < total_conjunctions) {
            conj_used[root->conjunction_idx] = true;
        }
    }

    if (root->type == NodeType::FAIL) {
        for (bool used : conj_used) {
            if (!used) {
                test_failure = true;
                return;
            }
        }
    }

    if (root->left) {
        std::vector<bool> conj_used_snapshot = conj_used;
        ValidateFailNodePaths(root->left, conj_used_snapshot, total_conjunctions, test_failure);
    }

    if (root->right) {
        std::vector<bool> conj_used_snapshot = conj_used;
        ValidateFailNodePaths(root->right, conj_used_snapshot, total_conjunctions, test_failure);
    }
}

TEST(CascadeBuilder, FailPaths_IncludeAllConjunctions) {
    Predicate pred1{.key = "name", .value = "John"};
    Predicate pred2{.key = "region", .value = "EMEA"};
    Predicate pred3{.key = "name", .value = "Jane"};

    PredicateConjunction conj1{{pred1, pred2}};
    PredicateConjunction conj2{{pred3}};
    PredicateDisjunction disj{{conj1, conj2}};

    RawFilterData raw_filter_data = RawFilterQueryGenerator::GenerateRawFilters(disj);

    CascadeBuilder builder(disj, raw_filter_data);
    auto valid_cascades = builder.GenerateValidCascades();

    const size_t total_conjunctions = disj.conjunctions.size();

    bool found_failure = false;
    for (auto& root : valid_cascades) {
        std::vector<bool> conj_used(total_conjunctions, false);

        ValidateFailNodePaths(root, conj_used, total_conjunctions, found_failure);

        if (found_failure) {
            PrettyPrint(root, raw_filter_data);
            break;
        }
    }

    ASSERT_FALSE(found_failure) << "A path ending with fail_node did not include all conjunctions.";
}

template <std::size_t N>
void fillArrayWithRandomValues(std::array<unsigned long long, N>& arr, double minValue, double maxValue) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<double> distribution(minValue, maxValue);

    for (auto& element : arr) {
        element = distribution(generator);
    }
}

template <std::size_t M, std::size_t N>
void fillArrayWithRandomValues(std::array<std::bitset<M>, N>& arr) {
    std::random_device rd;
    std::mt19937 generator(rd());
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 1);

    for (auto& element : arr) {
        for (size_t i = 0; i < M; ++i) {
            element[i] = dist(gen);
        }
    }
}

TEST(CascadeEvaluator, EvaluateCascade_ValidCascade_1_Conj_1_Rf_ReturnsExpectedEstimation) {
    /*
     *        (INTER -> rf_0)
     *         /       \
     *      (FAIL)    (PARSE)
     *
     */

    double precision = 1e-6;
    auto estimation_result = EstimationResult{.total_parser_runtime = 100, .total_rf_runtimes = {}, .bitsets = {}};

    fillArrayWithRandomValues(estimation_result.bitsets);
    fillArrayWithRandomValues(estimation_result.total_rf_runtimes, 5.0, 100.0);

    estimation_result.bitsets[0] = 0b1111000000;
    estimation_result.total_rf_runtimes[0] = 2.0;

    std::shared_ptr<Node> left = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::FAIL);
    std::shared_ptr<Node> right = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::PARSE);
    std::shared_ptr<Node> root = std::make_shared<Node>(0, 0, 0, left, right, NodeType::INTER);

    CascadeEvaluator evaluator(estimation_result);
    double result = evaluator.EvaluateCascade(root);

    EXPECT_NEAR(1.0, evaluator.rf_probabilities_[0], precision);
    EXPECT_NEAR(0.4, evaluator.rf_probabilities_[evaluator.parse_idx_], precision);
    EXPECT_NEAR(0.6, evaluator.rf_probabilities_[evaluator.fail_idx_], precision);

    double expected = 1.0 * 2 + 0.4 * 100;  // RF_0 cost + Total parser cost
    EXPECT_NEAR(expected, result, precision);
}

TEST(CascadeEvaluator, EvaluateCascade_ValidCascade_1_Conj_2_Rfs_ReturnsExpectedEstimation) {
    /*
     *           (INTER -> rf_0)
     *              /         \
     *           (FAIL)     (INTER -> rf_1)
     *                        /       \
     *                     (FAIL)    (PARSE)
     *
     */

    double precision = 1e-6;
    auto estimation_result = EstimationResult{
        .total_parser_runtime = 100,
        .total_rf_runtimes = {},
        .bitsets = {},
    };

    fillArrayWithRandomValues(estimation_result.bitsets);
    fillArrayWithRandomValues(estimation_result.total_rf_runtimes, 5.0, 100.0);
    estimation_result.bitsets[0] = 0b1111000000;
    estimation_result.bitsets[1] = 0b1100000001;
    estimation_result.total_rf_runtimes[0] = 2.0;
    estimation_result.total_rf_runtimes[1] = 3.0;

    std::shared_ptr<Node> fail = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::FAIL);
    std::shared_ptr<Node> parse = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::PARSE);
    std::shared_ptr<Node> inter = std::make_shared<Node>(0, 0, 1, fail, parse, NodeType::INTER);
    std::shared_ptr<Node> root = std::make_shared<Node>(0, 0, 0, fail, inter, NodeType::INTER);

    CascadeEvaluator evaluator(estimation_result);
    double result = evaluator.EvaluateCascade(root);

    EXPECT_NEAR(1.0, evaluator.rf_probabilities_[0], precision);
    EXPECT_NEAR(0.4, evaluator.rf_probabilities_[1], precision);
    EXPECT_NEAR(0.2, evaluator.rf_probabilities_[evaluator.parse_idx_], precision);
    EXPECT_NEAR(0.8, evaluator.rf_probabilities_[evaluator.fail_idx_], precision);

    double expected = 1.0 * 2 + 0.4 * 3 + 0.2 * 100;  // RF_0 cost + RF_1 cost + Total parser cost
    EXPECT_NEAR(expected, result, precision);
}

TEST(CascadeEvaluator, EvaluateCascade_ValidCascade_2_conj_2_preds_2_rfs_ReturnsExpectedEstimation) {
    /*
     *                   (1.0.1)
     *                 /        \
     *           (0.0.0)         (1.1.1)
     *           /       \       /       \
     *        (F)   (0.1.2)    (0.0.0)   (PARSE)
     *               /    \     /   \
     *              (F)  (P)  (F)  (P)
     *
     */

    double precision = 1e-6;
    auto estimation_result = EstimationResult{.total_parser_runtime = 100, .total_rf_runtimes = {}, .bitsets = {}};
    fillArrayWithRandomValues(estimation_result.bitsets);
    fillArrayWithRandomValues(estimation_result.total_rf_runtimes, 5.0, 100.0);

    estimation_result.total_rf_runtimes[GetFlatIdx(1, 0, 1)] = 3.0;
    estimation_result.total_rf_runtimes[GetFlatIdx(1, 1, 1)] = 5.0;
    estimation_result.total_rf_runtimes[GetFlatIdx(0, 0, 0)] = 7.0;
    estimation_result.total_rf_runtimes[GetFlatIdx(0, 1, 2)] = 11.0;

    estimation_result.bitsets[GetFlatIdx(1, 0, 1)] = 0b0100101010;
    estimation_result.bitsets[GetFlatIdx(1, 1, 1)] = 0b1100111101;
    estimation_result.bitsets[GetFlatIdx(0, 0, 0)] = 0b1011100110;
    estimation_result.bitsets[GetFlatIdx(0, 1, 2)] = 0b1110000001;

    std::shared_ptr<Node> fail = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::FAIL);
    std::shared_ptr<Node> parse = std::make_shared<Node>(0, 0, 0, nullptr, nullptr, NodeType::PARSE);

    std::shared_ptr<Node> node_0_0_0_right = std::make_shared<Node>(0, 0, 0, fail, parse, NodeType::INTER);
    std::shared_ptr<Node> node_1_1_1_right = std::make_shared<Node>(1, 1, 1, node_0_0_0_right, parse, NodeType::INTER);

    std::shared_ptr<Node> node_0_1_2_left = std::make_shared<Node>(0, 1, 2, fail, parse, NodeType::INTER);
    std::shared_ptr<Node> node_0_0_0_left = std::make_shared<Node>(0, 0, 0, fail, node_0_1_2_left, NodeType::INTER);

    std::shared_ptr<Node> node_1_0_1_root =
        std::make_shared<Node>(1, 0, 1, node_0_0_0_left, node_1_1_1_right, NodeType::INTER);

    CascadeEvaluator evaluator(estimation_result);
    double result = evaluator.EvaluateCascade(node_1_0_1_root);

    EXPECT_NEAR(1.0, evaluator.rf_probabilities_[GetFlatIdx(1, 0, 1)], precision);
    EXPECT_NEAR(0.4, evaluator.rf_probabilities_[GetFlatIdx(1, 1, 1)], precision);
    EXPECT_NEAR(0.7, evaluator.rf_probabilities_[GetFlatIdx(0, 0, 0)], precision);
    EXPECT_NEAR(0.4, evaluator.rf_probabilities_[GetFlatIdx(0, 1, 2)], precision);
    EXPECT_NEAR(0.6, evaluator.rf_probabilities_[evaluator.parse_idx_], precision);
    EXPECT_NEAR(0.4, evaluator.rf_probabilities_[evaluator.fail_idx_], precision);

    double expected = 1.0 * 3 + 0.4 * 5 + 0.7 * 7 + 0.4 * 11 + 0.6 * 100;
    EXPECT_NEAR(expected, result, precision);
}
