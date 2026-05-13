// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P21-Chen-PrefetchBPlus — Re-Implementation in PRT-ART
// Paper: Improving Index Performance through Prefetching (Chen/Gibbons/Mowry 2001)
// Concept: PrefetchStrategy (Achse: Prefetch)
//
// Kernidee: Wider Nodes mit Software-Prefetch-Instructions vor dem Linear-Scan
// + Leaf-Pointer-Arrays als Skip-Liste fuer Range-Scans.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::prefetch_bplus {

inline constexpr std::size_t kCacheLineBytes = 64;

// PrefetchStrategy: tactical hints, die das BPlus-Searcher waehrend
// Linear-Scan ausgibt. Kein direkter __builtin_prefetch — Strategy-Trace fuer Tests.
struct PrefetchHint {
    std::uint64_t address    = 0;
    std::uint8_t  locality   = 3;   // 0..3
    bool          is_write   = false;
};

class PrefetchTracker {
public:
    void emit(std::uint64_t addr, std::uint8_t locality = 3, bool wr = false) {
        hints_.push_back({addr, locality, wr});
    }

    [[nodiscard]] std::size_t hint_count() const noexcept { return hints_.size(); }
    [[nodiscard]] std::vector<PrefetchHint> const& hints() const noexcept { return hints_; }
    void reset() { hints_.clear(); }

private:
    std::vector<PrefetchHint> hints_{};
};

// Beispiel: Wider Node mit Linear-Scan + Prefetch fuer die naechste Cache-Line
template <std::size_t Lines = 4>
class WiderNodeWithPrefetch {
public:
    static constexpr std::size_t kFanout = (Lines * kCacheLineBytes - 8) / 8;

    std::array<std::uint32_t, kFanout> keys{};
    std::array<std::uint32_t, kFanout> values{};
    std::uint32_t                      key_count = 0;

    int insert(std::uint32_t k, std::uint32_t v) noexcept {
        if (key_count >= kFanout) return 5;
        std::size_t pos = 0;
        while (pos < key_count && keys[pos] < k) ++pos;
        if (pos < key_count && keys[pos] == k) return 1;
        for (std::size_t i = key_count; i > pos; --i) {
            keys[i]   = keys[i - 1];
            values[i] = values[i - 1];
        }
        keys[pos] = k; values[pos] = v;
        ++key_count;
        return 0;
    }

    [[nodiscard]] std::uint32_t scan_with_prefetch(std::uint32_t k, PrefetchTracker& t) const {
        constexpr std::size_t kKeysPerLine = kCacheLineBytes / sizeof(std::uint32_t);
        for (std::uint32_t i = 0; i < key_count; ++i) {
            if ((i % kKeysPerLine) == 0 && i + kKeysPerLine < key_count) {
                t.emit(reinterpret_cast<std::uint64_t>(&keys[i + kKeysPerLine]), 3, false);
            }
            if (keys[i] == k) return values[i];
            if (keys[i] >  k) break;
        }
        return 0;
    }
};

}  // namespace comdare::prt_art::legacy_reimpl::prefetch_bplus
