// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P12-CSB-tree — Re-Implementation in PRT-ART
// Paper: Making B+-Trees Cache Conscious in Main Memory (Rao/Ross 2000)
// Concept: Page (Achse: Page-Type)
//
// Kernidee: Sibling-Cluster. Parent haelt nur 1 Pointer zum Cluster
// + Offsets (statt N Pointer auf Kinder). Dadurch Fanout-Verdoppelung.

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace comdare::prt_art::legacy_reimpl::csb {

using key_t = std::uint32_t;
using val_t = std::uint32_t;

template <std::size_t Fanout = 14>
struct CsbNodeGroupPage {
    static_assert(Fanout >= 2);

    std::uint32_t                                 group_id      = 0;
    std::uint16_t                                 sibling_count = 0;
    std::uint16_t                                 reserved      = 0;
    std::array<key_t, Fanout>                     separators{};
    std::array<val_t, Fanout + 1>                 child_values{};

    int insert_separator(key_t k, val_t v) noexcept {
        if (sibling_count >= Fanout) return 5;
        std::size_t pos = 0;
        while (pos < sibling_count && separators[pos] < k) ++pos;
        if (pos < sibling_count && separators[pos] == k) return 1;
        for (std::size_t i = sibling_count; i > pos; --i) {
            separators[i]   = separators[i - 1];
            child_values[i] = child_values[i - 1];
        }
        separators[pos]   = k;
        child_values[pos] = v;
        ++sibling_count;
        return 0;
    }

    [[nodiscard]] std::optional<val_t> lookup(key_t k) const noexcept {
        for (std::size_t i = 0; i < sibling_count; ++i) {
            if (separators[i] == k) return child_values[i];
            if (separators[i] >  k) break;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::size_t size() const noexcept { return sibling_count; }
};

}  // namespace comdare::prt_art::legacy_reimpl::csb
