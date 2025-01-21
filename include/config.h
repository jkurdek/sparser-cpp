#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstddef>

constexpr size_t kRfSize = 4;
constexpr size_t kSampleSize = 10;
constexpr size_t kMaxDepth = 4;

constexpr size_t kMaxRfsInPred = 32;
constexpr size_t kMaxPred = 10;
constexpr size_t kMaxConj = 10;
constexpr size_t kTotalMaxRfs = kMaxRfsInPred * kMaxPred * kMaxConj;

#endif  // CONFIG_H_
