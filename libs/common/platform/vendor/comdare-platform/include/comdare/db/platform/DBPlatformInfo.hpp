#pragma once
/**
 * @file DBPlatformInfo.hpp
 * @brief DB-specific platform information and tuning hints
 *
 * Thin wrapper over comdare::platform::SIMDDetector providing
 * database-relevant platform queries (page sizing, IO alignment,
 * vectorized scan support, NUMA awareness).
 */

#include <comdare/platform/SIMDDetect.hpp>

#include <cstddef>
#include <cstdint>

namespace comdare::db {

/**
 * @brief Database platform information derived from CPU features
 *
 * Provides hardware-aware defaults for buffer pool page sizes,
 * direct-IO alignment, and vectorized scan capabilities.
 */
class DBPlatformInfo {
public:
    /**
     * @brief Get singleton instance (thread-safe, cached)
     */
    static const DBPlatformInfo& instance() {
        static DBPlatformInfo info;
        return info;
    }

    /**
     * @brief Optimal database page size based on L1 data cache
     *
     * Returns 4096 for small caches, 8192 for moderate, 16384 for large.
     * This balances IO amplification against cache utilization.
     */
    [[nodiscard]] std::size_t optimal_page_size() const noexcept {
        return page_size_;
    }

    /**
     * @brief Alignment required for direct IO (O_DIRECT / FILE_FLAG_NO_BUFFERING)
     *
     * Typically 512 or 4096 depending on sector size.
     */
    [[nodiscard]] std::size_t io_alignment() const noexcept {
        return io_alignment_;
    }

    /**
     * @brief Whether vectorized column scans (AVX2+) are available
     */
    [[nodiscard]] bool supports_vectorized_scan() const noexcept {
        return vectorized_scan_;
    }

    /**
     * @brief SIMD vector width in bytes for scan operations
     */
    [[nodiscard]] std::size_t scan_vector_width() const noexcept {
        return platform::SIMDDetector::optimal_vector_width();
    }

    /**
     * @brief Cache line size for padding DB structures
     */
    [[nodiscard]] uint32_t cache_line_size() const noexcept {
        return features_.cache_line_size;
    }

    /**
     * @brief Whether hardware POPCNT is available (for bitmap indexes)
     */
    [[nodiscard]] bool has_popcnt() const noexcept {
        return features_.popcnt;
    }

    /**
     * @brief Whether BMI2 PEXT/PDEP are available (for hash compaction)
     */
    [[nodiscard]] bool has_bmi2() const noexcept {
        return features_.bmi2;
    }

    /**
     * @brief Access underlying CPU features for advanced queries
     */
    [[nodiscard]] const platform::CPUFeatures& cpu_features() const noexcept {
        return features_;
    }

    /**
     * @brief Access best SIMD level
     */
    [[nodiscard]] platform::SIMDLevel simd_level() const noexcept {
        return platform::SIMDDetector::best_level();
    }

private:
    DBPlatformInfo()
        : features_(platform::SIMDDetector::detect())
        , vectorized_scan_(platform::SIMDDetector::supports(platform::SIMDLevel::AVX2))
        , page_size_(compute_page_size(features_))
        , io_alignment_(4096) {}

    static std::size_t compute_page_size(const platform::CPUFeatures& f) {
        if (f.l1_data_cache_kb >= 64) return 16384;
        if (f.l1_data_cache_kb >= 32) return 8192;
        return 4096;
    }

    const platform::CPUFeatures& features_;
    bool vectorized_scan_;
    std::size_t page_size_;
    std::size_t io_alignment_;
};

} // namespace comdare::db
