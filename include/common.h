#ifndef COMMON_H_
#define COMMON_H_

#include <chrono>

using bench_timer_t = std::chrono::time_point<std::chrono::steady_clock>;

inline bench_timer_t benchmark_start() { return std::chrono::steady_clock::now(); }

inline double benchmark_stop(bench_timer_t start) {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

#endif  // COMMON_H_
