#ifndef CASCADE_EVALUATOR_H_
#define CASCADE_EVALUATOR_H_

#include <array>
#include <bitset>
#include <memory>

#include "config.h"
#include "node.h"

struct EstimationResult {
    double average_parse_time{};
    double average_rf_time{};
    std::array<std::bitset<kSampleSize>, kTotalMaxRfs> bitsets;
};

class CascadeEvaluator {
   public:
    explicit CascadeEvaluator(const EstimationResult& estimation_result) : estimation_result_(estimation_result) {}

    [[nodiscard]] double EvaluateCascade(const std::shared_ptr<Node>& cascade);

   private:
    EstimationResult estimation_result_;

    [[nodiscard]] double EvaluateSubtreeCost(const std::shared_ptr<Node>& node,
                                             const std::bitset<kSampleSize>& passed_records);
};

#endif  // CASCADE_EVALUATOR_H_
