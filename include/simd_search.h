// NOLINTBEGIN(*)

#ifndef SIMD_SEARCH_H_
#define SIMD_SEARCH_H_

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <string>

#ifdef __ARM_NEON
#include <arm_neon.h>
#endif

#ifdef __x86_64__
#include <immintrin.h>
#endif

#ifdef __ARM_NEON

// http://0x80.pl/notesen/2016-11-28-simd-strfind.html#arm-neon-32-bit-code
inline size_t neon_strstr_anysize(std::string_view haystack, std::string_view needle) {
    const char* s = haystack.data();
    const size_t n = haystack.size();
    const char* needle_data = needle.data();
    const size_t k = needle.size();

    const uint8x16_t first = vdupq_n_u8(needle_data[0]);
    const uint8x16_t last  = vdupq_n_u8(needle_data[k - 1]);
    const uint8x8_t  half  = vdup_n_u8(0x0f);

    const uint8_t* ptr = reinterpret_cast<const uint8_t*>(s);

    union {
        uint8_t  tmp[8];
        uint32_t word[2];
    };

    for (size_t i = 0; i < n; i += 16) {

        const uint8x16_t block_first = vld1q_u8(ptr + i);
        const uint8x16_t block_last  = vld1q_u8(ptr + i + k - 1);

        const uint8x16_t eq_first = vceqq_u8(first, block_first);
        const uint8x16_t eq_last  = vceqq_u8(last, block_last);
        const uint8x16_t pred_16  = vandq_u8(eq_first, eq_last);
        const uint8x8_t pred_8    = vbsl_u8(half, vget_low_u8(pred_16), vget_high_u8(pred_16));

        vst1_u8(tmp, pred_8);

        if ((word[0] | word[1]) == 0) {
            continue;
        }

        for (int j=0; j < 8; j++) {
            if (tmp[j] & 0x0f) {
                if (memcmp(s + i + j + 1, needle_data + 1, k - 2) == 0) {
                    return i + j;
                }
            }
        }

        for (int j=0; j < 8; j++) {
            if (tmp[j] & 0xf0) {
                if (memcmp(s + i + j + 1 + 8, needle_data + 1, k - 2) == 0) {
                    return i + j + 8;
                }
            }
        }
    }

    return std::string::npos;
}
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
#elif defined(__AVX2__)
__attribute__((target("avx2")))
inline bool simd_search4(std::string_view haystack, std::string_view needle) {
    if (haystack.size() < 4) {
        return false;
    }

    const uint32_t needle_val = *reinterpret_cast<const uint32_t*>(needle.data());
    const __m256i needle_vec = _mm256_set1_epi32(needle_val);

    const char* data = haystack.data();
    const size_t haystack_len = haystack.size();
    const auto* curr = reinterpret_cast<const uint8_t*>(data);
    const uint8_t* end = curr + haystack_len - 3;

    while (curr + 32 <= end) {
        __m256i data0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(curr + 0));
        __m256i data1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(curr + 1));
        __m256i data2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(curr + 2));
        __m256i data3 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(curr + 3));

        __m256i eq0 = _mm256_cmpeq_epi32(data0, needle_vec);
        __m256i eq1 = _mm256_cmpeq_epi32(data1, needle_vec);
        __m256i eq2 = _mm256_cmpeq_epi32(data2, needle_vec);
        __m256i eq3 = _mm256_cmpeq_epi32(data3, needle_vec);

        __m256i combined = _mm256_or_si256(
            _mm256_or_si256(eq0, eq1),
            _mm256_or_si256(eq2, eq3)
        );

        if (_mm256_movemask_epi8(combined)) return true;
        curr += 29;
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
