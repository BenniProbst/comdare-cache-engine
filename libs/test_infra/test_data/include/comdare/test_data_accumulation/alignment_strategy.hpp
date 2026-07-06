#pragma once
// AlignmentStrategy - Cache-Line / HugePage / NUMA-aware Buffer-Alignment (REV 7 §7.3)

#include <cstddef>
#include <cstdint> // Hermetik (CI-2): std::uint8_t fuer AlignmentMode -- bisher nur transitiv bei manchen Konsumenten
#include <cstdlib>
#include <new>
#include <type_traits>

#ifdef _MSC_VER
#include <malloc.h>
#endif

namespace comdare::test_data_accumulation {

inline constexpr std::size_t kCacheLineBytes  = 64;
inline constexpr std::size_t kHugePageBytes2M = 2 * 1024 * 1024;
inline constexpr std::size_t kHugePageBytes1G = 1ULL * 1024 * 1024 * 1024;

enum class AlignmentMode : std::uint8_t {
    Default    = 0,
    CacheLine  = 1,
    HugePage2M = 2,
    HugePage1G = 3,
    NumaLocal  = 4,
};

[[nodiscard]] inline std::size_t alignment_bytes_for(AlignmentMode mode) noexcept {
    switch (mode) {
        case AlignmentMode::CacheLine: return kCacheLineBytes;
        case AlignmentMode::HugePage2M: return kHugePageBytes2M;
        case AlignmentMode::HugePage1G: return kHugePageBytes1G;
        case AlignmentMode::NumaLocal: return kCacheLineBytes; // base align
        case AlignmentMode::Default: return alignof(std::max_align_t);
    }
    return alignof(std::max_align_t);
}

[[nodiscard]] inline void* aligned_alloc_for_mode(std::size_t bytes, AlignmentMode mode) {
    std::size_t const alignment = alignment_bytes_for(mode);
    // Round bytes up to alignment multiple (Pflicht von std::aligned_alloc)
    std::size_t const padded = ((bytes + alignment - 1) / alignment) * alignment;
#ifdef _MSC_VER
    return _aligned_malloc(padded, alignment);
#else
    return std::aligned_alloc(alignment, padded);
#endif
}

inline void aligned_free(void* p) noexcept {
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}

} // namespace comdare::test_data_accumulation
