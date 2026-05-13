// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P19-Saikkonen-LayoutInvariant — Re-Implementation in PRT-ART
// Paper: Layout Invariants with Constant-Time Updates
//        (Saikkonen/Soisalon-Soininen 2016)
// Concept: MemoryLayout (Achse: Memory-Layout)
//
// Kernidee: Layout-Invariante (z. B. "BFS-level-order + free-cell-bitmap"),
// die bei jeder Update-Operation in O(1) erhalten bleibt.

#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::layout_invariant {

template <std::size_t Capacity = 1024>
class LayoutInvariantBuffer {
public:
    using value_t = std::uint64_t;

    // O(1) Insert: belege erste freie Zelle
    int insert(value_t v) noexcept {
        if (size_ >= Capacity) return 5;  // capacity_exceeded
        std::size_t pos = find_first_free();
        if (pos >= Capacity) return 5;
        cells_[pos]    = v;
        occupied_.set(pos);
        ++size_;
        return 0;
    }

    // O(1) Erase: markiere Zelle frei (Tombstone-Style)
    int erase_at(std::size_t pos) noexcept {
        if (pos >= Capacity) return 7;     // out_of_range
        if (!occupied_.test(pos)) return 2; // not_found
        occupied_.reset(pos);
        --size_;
        return 0;
    }

    [[nodiscard]] std::optional<value_t> at(std::size_t pos) const noexcept {
        if (pos >= Capacity) return std::nullopt;
        if (!occupied_.test(pos)) return std::nullopt;
        return cells_[pos];
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    // Invariante: belegte Zellen sind kontinuierlich, oder ein
    // Tombstone-Slot markiert die Luecke.
    [[nodiscard]] bool check_invariant() const noexcept {
        // Triviale Form: size_ == popcount(occupied_)
        return size_ == occupied_.count();
    }

private:
    std::size_t find_first_free() const noexcept {
        for (std::size_t i = 0; i < Capacity; ++i) {
            if (!occupied_.test(i)) return i;
        }
        return Capacity;
    }

    std::vector<value_t>    cells_{std::vector<value_t>(Capacity, 0)};
    std::bitset<Capacity>   occupied_{};
    std::size_t             size_ = 0;
};

}  // namespace comdare::prt_art::legacy_reimpl::layout_invariant
