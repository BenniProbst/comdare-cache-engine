// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P11-CSS-tree — Re-Implementation in PRT-ART
// Paper: Cache-Sensitive Search Trees (Rao/Ross 1999)
// Concept: Page (Achse: Page-Type)
//
// PRT-ART-eigene C++23-Re-Implementation. Kernidee: Pointer-frei,
// Cache-Line-grosse Knoten, Adressierung der Kinder ueber Index-Arithmetik
// auf einem flachen Array (analog Heap-Array eines binary tree).

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::css_node_page {

// Cache-Line-Groesse als Konstante (default 64 B, MultiLayout konfigurierbar)
inline constexpr std::size_t kCacheLineBytes = 64;

// Schluessel-Typ: 32-bit fuer Pointer-freie Adressierung; Wert: 32-bit Slot-ID
using key_t   = std::uint32_t;
using value_t = std::uint32_t;

// CssNodePage: Pointer-freier Cache-Line-grosser Knoten.
// Fanout = (kCacheLineBytes - 4 Byte Header) / 8 Byte (key+value).
// Adressierung der Kinder via base_index * Fanout + child_offset.
template <std::size_t Fanout = 7>
class CssNodePage {
public:
    static_assert(Fanout >= 2, "CSS-Tree fanout must be >= 2");

    struct Header {
        std::uint16_t key_count   = 0;
        std::uint16_t reserved    = 0;
    };

    Header                     header{};
    std::array<key_t,   Fanout> keys{};
    std::array<value_t, Fanout> values{};

    [[nodiscard]] std::size_t key_count() const noexcept { return header.key_count; }
    [[nodiscard]] bool        full()      const noexcept { return header.key_count == Fanout; }

    // Sortierte Insertion (Binary-Insert), gibt status fuer cache_engine-Konvention
    int insert_sorted(key_t k, value_t v) noexcept {
        if (full()) return 5;  // capacity_exceeded
        std::size_t pos = 0;
        while (pos < header.key_count && keys[pos] < k) ++pos;
        if (pos < header.key_count && keys[pos] == k) return 1;  // already exists
        for (std::size_t i = header.key_count; i > pos; --i) {
            keys[i]   = keys[i - 1];
            values[i] = values[i - 1];
        }
        keys[pos]   = k;
        values[pos] = v;
        ++header.key_count;
        return 0;
    }

    [[nodiscard]] std::optional<value_t> lookup(key_t k) const noexcept {
        for (std::size_t i = 0; i < header.key_count; ++i) {
            if (keys[i] == k) return values[i];
            if (keys[i] > k)  break;
        }
        return std::nullopt;
    }
};

static_assert(sizeof(CssNodePage<7>) <= kCacheLineBytes,
              "CssNodePage<7> must fit into a 64-byte cache line");

// CssTree: flaches Array von CssNodePage. Kinder werden via Index-Arithmetik
// adressiert: child(node, slot) = node_index * (Fanout+1) + slot + 1.
template <std::size_t Fanout = 7>
class CssTree {
public:
    using node_t = CssNodePage<Fanout>;

    [[nodiscard]] std::size_t node_count() const noexcept { return nodes_.size(); }

    int add_root() {
        if (!nodes_.empty()) return 1;  // already exists
        nodes_.emplace_back();
        return 0;
    }

    [[nodiscard]] node_t&       root()       { return nodes_.front(); }
    [[nodiscard]] node_t const& root() const { return nodes_.front(); }

    [[nodiscard]] static constexpr std::size_t
    child_index(std::size_t parent_idx, std::size_t slot) noexcept {
        return parent_idx * (Fanout + 1) + slot + 1;
    }

    [[nodiscard]] static constexpr std::size_t
    parent_index(std::size_t child_idx) noexcept {
        return child_idx == 0 ? 0 : (child_idx - 1) / (Fanout + 1);
    }

private:
    std::vector<node_t> nodes_{};
};

}  // namespace comdare::prt_art::legacy_reimpl::css_node_page
