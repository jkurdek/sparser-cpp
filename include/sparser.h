#ifndef SPARSER_H_
#define SPARSER_H_

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cascade_evaluator.h"
#include "config.h"
#include "json_facade.h"
#include "node.h"
#include "raw_filter.h"

struct SparserConfig {
    std::string input_path;
    JsonQuery json_query;
    size_t rf_size = kRfSize;
    size_t sample_size = kSampleSize;
    size_t max_depth = kMaxDepth;
    size_t max_rfs_in_pred = kMaxRfsInPred;
    size_t max_pred = kMaxPred;
    size_t max_conj = kMaxConj;

    void PrintConfig() const;
};

struct SparserSearchStats {
    size_t records_processed;
    size_t records_matched;
    size_t callback_passed;
    double fraction_true_positive;
    double fraction_false_positive;

    void PrintStats() const;
};

struct NaiveSearchStats {
    size_t records_processed;
    size_t callback_passed;
};

class Sparser {
   public:
    explicit Sparser(std::unique_ptr<JsonQueryDriver>&& json_query_driver = {})
        : json_query_driver_(std::move(json_query_driver)) {}

    void Run(const std::string& input_path, const JsonQuery& json_query);

    [[nodiscard]] EstimationResult Calibrate(const std::vector<std::string_view>& input, const JsonQuery& json_query,
                                             const RawFilterData& rf_data);
    [[nodiscard]] SparserSearchStats SearchCascade(const std::vector<std::string_view>& input,
                                                   const JsonQuery& json_query, const RawFilterData& rf_data,
                                                   const std::shared_ptr<Node>);
    [[nodiscard]] NaiveSearchStats SearchNaive(const std::vector<std::string_view>& input, const JsonQuery& json_query);

   private:
    std::unique_ptr<JsonQueryDriver> json_query_driver_;
};

#endif  // SPARSER_H_
