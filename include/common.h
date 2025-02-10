#ifndef COMMON_H_
#define COMMON_H_

#include <chrono>

using bench_timer_t = std::chrono::time_point<std::chrono::steady_clock>;

[[nodiscard]] inline bench_timer_t benchmark_start() { return std::chrono::steady_clock::now(); }

constexpr double MICROSECONDS_IN_SECOND = 1'000'000.0;

[[nodiscard]] inline double benchmark_stop(bench_timer_t start) {
    auto end = std::chrono::steady_clock::now();
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) /
           MICROSECONDS_IN_SECOND;
}

#endif  // COMMON_H_
