// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P23-Khan-AdaptivePrefetch — Re-Implementation in PRT-ART
// Paper: Data Cache Prefetching With Dynamic Adaptation (Khan 2010)
// Concept: PrefetchStrategy (Achse: Prefetch)
//
// Kernidee: Code-Specializer adaptiert die Prefetch-Distance zur Laufzeit
// auf Basis von Hit/Miss-Statistiken.

#pragma once

#include <algorithm>
#include <cstdint>

namespace comdare::prt_art::legacy_reimpl::adaptive_prefetch {

class AdaptivePrefetchDistance {
public:
    explicit AdaptivePrefetchDistance(std::uint32_t initial_distance = 4) noexcept
        : distance_(initial_distance) {}

    void record_hit()  noexcept { ++hits_;  rebalance(); }
    void record_miss() noexcept { ++misses_; rebalance(); }

    [[nodiscard]] std::uint32_t current_distance() const noexcept { return distance_; }
    [[nodiscard]] std::uint64_t total_hits()        const noexcept { return hits_; }
    [[nodiscard]] std::uint64_t total_misses()      const noexcept { return misses_; }

private:
    void rebalance() noexcept {
        if (hits_ + misses_ < kAdaptInterval) return;
        // Bei vielen Misses: Distance vergroessern (further ahead)
        // Bei vielen Hits:   Distance verkleinern (less aggressive)
        std::uint64_t const total = hits_ + misses_;
        std::uint64_t const miss_pct = misses_ * 100 / total;
        if (miss_pct > 60 && distance_ < kMaxDistance) {
            ++distance_;
        } else if (miss_pct < 20 && distance_ > kMinDistance) {
            --distance_;
        }
        hits_ = misses_ = 0;
    }

    static constexpr std::uint32_t kMinDistance    = 1;
    static constexpr std::uint32_t kMaxDistance    = 32;
    static constexpr std::uint64_t kAdaptInterval  = 100;

    std::uint32_t distance_;
    std::uint64_t hits_   = 0;
    std::uint64_t misses_ = 0;
};

}  // namespace comdare::prt_art::legacy_reimpl::adaptive_prefetch
