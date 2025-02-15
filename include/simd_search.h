// NOLINTBEGIN(*)

#ifndef SIMD_SEARCH_H_
#define SIMD_SEARCH_H_

#include <cstddef>
#include <cstdint>
#include <string_view>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifdef __ARM_NEON
inline bool simd_search4(std::string_view haystack, std::string_view needle) {
    if (haystack.size() < 4) {
        return false;
    }

    const uint32_t needle_val = *reinterpret_cast<const uint32_t*>(needle.data());
    const uint32x4_t needle_vec = vdupq_n_u32(needle_val);

    const char* data = haystack.data();
    const size_t haystack_len = haystack.size();
    const auto* curr = reinterpret_cast<const uint8_t*>(data);
    const uint8_t* end = curr + haystack_len - 3;
    while (curr + 16 <= end) {
        uint8x16_t data0 = vld1q_u8(curr + 0);
        uint8x16_t data1 = vld1q_u8(curr + 1);
        uint8x16_t data2 = vld1q_u8(curr + 2);
        uint8x16_t data3 = vld1q_u8(curr + 3);
        uint32x4_t seq0 = vreinterpretq_u32_u8(data0);
        uint32x4_t seq1 = vreinterpretq_u32_u8(data1);
        uint32x4_t seq2 = vreinterpretq_u32_u8(data2);
        uint32x4_t seq3 = vreinterpretq_u32_u8(data3);
        uint32x4_t eq0 = vceqq_u32(seq0, needle_vec);
        uint32x4_t eq1 = vceqq_u32(seq1, needle_vec);
        uint32x4_t eq2 = vceqq_u32(seq2, needle_vec);
        uint32x4_t eq3 = vceqq_u32(seq3, needle_vec);
        uint32x4_t combined = vorrq_u32(vorrq_u32(eq0, eq1), vorrq_u32(eq2, eq3));
        if (vmaxvq_u32(combined)) return true;
        curr += 13;
    }
    while (curr <= end) {
        if (*reinterpret_cast<const uint32_t*>(curr) == needle_val) {
            return true;
        }
        ++curr;
    }
    return false;
}
#else
#include <string>
inline bool simd_search4(std::string_view haystack, std::string_view needle) {
    return haystack.find(needle) != std::string_view::npos;
}
#endif

#endif  // SIMD_SEARCH_H_

// NOLINTEND(*)
