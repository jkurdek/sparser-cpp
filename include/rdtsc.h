#ifndef __RDTSC_H_DEFINED__
#define __RDTSC_H_DEFINED__

// NOLINTBEGIN(*)

#include <cstdint>

#if defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

#elif defined(__aarch64__)

static __inline__ uint64_t rdtsc(void) {
    uint64_t val;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(val));
    return val;
}

#else

#error "No tick counter is available!"

#endif

#endif  // __RDTSC_H_DEFINED__

// NOLINTEND(*)
