// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P27-Zhang-ASPLOS-Hierarchical — Re-Implementation in PRT-ART
// Paper: Hierarchical Prefetching (T. Zhang et al. ASPLOS 2025)
// Concept: PrefetchStrategy (Achse: Prefetch)
//
// Kernidee: Bundle-Prefetching mit Software/Hardware-Hybrid:
// Bundles aus mehreren Cache-Line-Adressen werden zusammen gepretcht.
// Hierarchisch: L1-Bundles, L2-Bundles, L3-Bundles.

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::zhang_asplos {

inline constexpr std::size_t kBundleSize = 4;   // Adressen pro Bundle

struct Bundle {
    std::array<std::uint64_t, kBundleSize> addresses{};
    std::uint8_t  cache_level   = 1;   // 1=L1, 2=L2, 3=L3
    std::uint8_t  fill_count    = 0;
};

class HierarchicalBundlePrefetcher {
public:
    int add_address_to_bundle(std::uint64_t addr, std::uint8_t cache_level) {
        if (cache_level < 1 || cache_level > 3) return 4;  // invalid_argument
        Bundle* target = find_open_bundle(cache_level);
        if (!target) {
            Bundle nb;
            nb.cache_level = cache_level;
            bundles_.push_back(nb);
            target = &bundles_.back();
        }
        target->addresses[target->fill_count++] = addr;
        if (target->fill_count == kBundleSize) {
            ++completed_bundles_;
        }
        return 0;
    }

    [[nodiscard]] std::size_t bundle_count() const noexcept { return bundles_.size(); }
    [[nodiscard]] std::uint64_t completed_bundles() const noexcept { return completed_bundles_; }
    [[nodiscard]] std::vector<Bundle> const& bundles() const noexcept { return bundles_; }

    [[nodiscard]] std::size_t bundles_at_level(std::uint8_t level) const noexcept {
        std::size_t count = 0;
        for (auto const& b : bundles_) if (b.cache_level == level) ++count;
        return count;
    }

private:
    Bundle* find_open_bundle(std::uint8_t level) noexcept {
        for (auto& b : bundles_) {
            if (b.cache_level == level && b.fill_count < kBundleSize) return &b;
        }
        return nullptr;
    }

    std::vector<Bundle> bundles_{};
    std::uint64_t       completed_bundles_ = 0;
};

}  // namespace comdare::prt_art::legacy_reimpl::zhang_asplos
