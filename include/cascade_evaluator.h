#ifndef CASCADE_EVALUATOR_H_
#define CASCADE_EVALUATOR_H_

#include <array>
#include <bitset>
#include <memory>

#include "config.h"
#include "node.h"

struct EstimationResult {
    double average_parse_time;
    double average_rf_time;
    std::array<std::bitset<kSampleSize>, kTotalMaxRfs> bitsets;
};

class CascadeEvaluator {
   public:
    CascadeEvaluator(const EstimationResult& estimation_result) : estimation_result_(estimation_result) {}

    [[nodiscard]] double EvaluateCascade(std::shared_ptr<Node> cascade);
    std::array<double, kTotalMaxRfs + 2> rf_probabilities_;  // Changes every call to EvaluateCascade
    const size_t parse_idx_ = kTotalMaxRfs + 1;
    const size_t fail_idx_ = kTotalMaxRfs;

   private:
    const EstimationResult& estimation_result_;
    void EvaluateNodeRec(std::shared_ptr<Node> node, std::bitset<kSampleSize> cumulative_bitset);
};

#endif  // CASCADE_EVALUATOR_H_
