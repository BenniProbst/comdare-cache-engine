// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P13-Hankins — Re-Implementation in PRT-ART
// Paper: Effect of Node Size on the Performance of Cache-Conscious B+-Trees
//        (Hankins/Patel 2003)
// Concept: Page (Achse: Page-Type)
//
// Kernidee: Wider Nodes (4-16 Cache-Lines pro Knoten) + I/M/B/T-Cost-Modell.

#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace comdare::prt_art::legacy_reimpl::hankins {

inline constexpr std::size_t kCacheLineBytes = 64;

struct CostModel {
    double weight_instructions  = 1.0;
    double weight_memory_stalls = 4.0;
    double weight_branch        = 0.5;
    double weight_tlb           = 2.0;

    [[nodiscard]] double total(std::uint64_t i, std::uint64_t m,
                                std::uint64_t b, std::uint64_t t) const noexcept {
        return weight_instructions  * static_cast<double>(i)
             + weight_memory_stalls * static_cast<double>(m)
             + weight_branch        * static_cast<double>(b)
             + weight_tlb           * static_cast<double>(t);
    }
};

template <std::size_t CacheLinesPerNode = 8>
class WiderBPlusPage {
public:
    static_assert(CacheLinesPerNode >= 1 && CacheLinesPerNode <= 16);
    static constexpr std::size_t kBytesTotal     = CacheLinesPerNode * kCacheLineBytes;
    static constexpr std::size_t kReservedHeader = 8;
    static constexpr std::size_t kFanout = (kBytesTotal - kReservedHeader) / 8;

    std::uint32_t                       key_count = 0;
    std::uint32_t                       reserved  = 0;
    std::array<std::uint32_t, kFanout>  keys{};
    std::array<std::uint32_t, kFanout>  values{};

    int insert(std::uint32_t k, std::uint32_t v) noexcept {
        if (key_count >= kFanout) return 5;
        std::size_t pos = 0;
        while (pos < key_count && keys[pos] < k) ++pos;
        if (pos < key_count && keys[pos] == k) return 1;
        for (std::size_t i = key_count; i > pos; --i) {
            keys[i]   = keys[i - 1];
            values[i] = values[i - 1];
        }
        keys[pos]   = k;
        values[pos] = v;
        ++key_count;
        return 0;
    }

    [[nodiscard]] std::optional<std::uint32_t> lookup(std::uint32_t k) const noexcept {
        for (std::uint32_t i = 0; i < key_count; ++i) {
            if (keys[i] == k) return values[i];
            if (keys[i] > k)  break;
        }
        return std::nullopt;
    }

    [[nodiscard]] static constexpr std::size_t fanout() noexcept { return kFanout; }
    [[nodiscard]] static constexpr std::size_t bytes()  noexcept { return kBytesTotal; }
};

}  // namespace comdare::prt_art::legacy_reimpl::hankins
