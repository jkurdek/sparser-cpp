#ifndef CASCADE_EVALUATOR_H_
#define CASCADE_EVALUATOR_H_

#include <array>
#include <bitset>
#include <memory>

#include "config.h"
#include "node.h"

struct EstimationResult {
    unsigned long long total_parser_runtime;
    std::array<unsigned long long, kTotalMaxRfs> total_rf_runtimes;
    std::array<std::bitset<kSampleSize>, kTotalMaxRfs> bitsets;
};

class CascadeEvaluator {
   public:
    CascadeEvaluator(const EstimationResult& estimation_result) : estimation_result_(estimation_result) {}

    double EvaluateCascade(std::shared_ptr<Node> cascade);
    std::array<double, kTotalMaxRfs + 2> rf_probabilities_;  // Changes every call to EvaluateCascade
    const size_t parse_idx_ = kTotalMaxRfs + 1;
    const size_t fail_idx_ = kTotalMaxRfs;

   private:
    const EstimationResult& estimation_result_;
    void EvaluateNodeRec(std::shared_ptr<Node> node, std::bitset<kSampleSize> cumulative_bitset);
};

#endif  // CASCADE_EVALUATOR_H_
