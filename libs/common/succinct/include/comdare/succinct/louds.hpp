// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// comdare::succinct::Louds — Level-Order Unary Degree Sequence (P09)
// Eigene C++23-Implementation, baut auf BitVector auf.
//
// Encoding-Schema (Jacobson 1989):
//   - BFS-Reihenfolge: fuer jeden Knoten N mit d Kindern -> d "1"-Bits + ein "0"-Bit
//   - Bit-Position i in der BitVector entspricht der BFS-Position des Knotens
//
// Beispiel-Baum:           BFS:  A, B, C, D, E
//      A                   degree: 2, 2, 0, 0, 0
//     / \                  LOUDS: 11 0 11 0 0 0 0 0 = "1101100000"
//    B   C
//   / \
//  D   E
//
// Operationen:
//   first_child(v)  -> first child node-id, oder 0 falls keine Kinder
//   parent(v)       -> parent-id (oder 0 fuer root)
//   degree(v)       -> Anzahl Kinder

#pragma once

#include "bit_vector.hpp"

#include <cstdint>
#include <vector>

namespace comdare::succinct {

class Louds {
public:
    Louds() = default;

    // Konstruktion aus BFS-Degree-Sequenz: degrees[i] = Anzahl Kinder des i-ten BFS-Knotens
    void build_from_bfs_degrees(std::vector<std::uint32_t> const& degrees) {
        node_count_            = degrees.size();
        std::size_t total_bits = 0;
        for (auto d : degrees) total_bits += (d + 1); // d Einsen + 1 Null pro Knoten
        bits_.resize(total_bits);
        std::size_t pos = 0;
        for (auto d : degrees) {
            for (std::uint32_t i = 0; i < d; ++i) bits_.set(pos++, true);
            bits_.set(pos++, false);
        }
        bits_.build_rank_index();
    }

    [[nodiscard]] std::size_t      node_count() const noexcept { return node_count_; }
    [[nodiscard]] BitVector const& bits() const noexcept { return bits_; }

    // first_child(v) — gibt BFS-Index des ersten Kindes zurueck.
    // LOUDS-Trick: erstes Kind von v = rank0(select1(v+1)) - 0  (eigene Convention)
    [[nodiscard]] std::size_t first_child(std::size_t v) const noexcept {
        if (v >= node_count_) return 0;
        if (degree(v) == 0) return 0;
        // first child id = (Anzahl bisheriger "1"-bits vor Knotens "0"-Trenner) + 1
        // = total_ones bis zur ersten Position nach v's 0-trenner
        std::size_t zero_pos = nth_zero_position(v);
        // Vor zero_pos sind die "1"-Bits = Kind-IDs (1-based, offset 1 fuer root)
        return bits_.rank1(zero_pos + 1);
    }

    [[nodiscard]] std::uint32_t degree(std::size_t v) const noexcept {
        if (v >= node_count_) return 0;
        std::size_t const zero_pos      = nth_zero_position(v);
        std::size_t const prev_zero_pos = (v == 0) ? 0 : (nth_zero_position(v - 1) + 1);
        return static_cast<std::uint32_t>(zero_pos - prev_zero_pos);
    }

    // parent(v) — root hat parent=0 (self-loop)
    [[nodiscard]] std::size_t parent(std::size_t v) const noexcept {
        if (v == 0) return 0;
        // parent = rank0 der Position vor v's "1"-bit-block
        // Naive: scan bits, weil 1-bit-Position fuer Kind v ueber rank1=v gefunden wird.
        std::size_t const my_bit = bits_.select1(v); // position des v-ten 1-bits
        if (my_bit >= bits_.size()) return 0;
        return bits_.rank0(my_bit); // anzahl 0-bits davor = parent-id
    }

private:
    // Position des (v+1)-ten "0"-Bits in bits_
    [[nodiscard]] std::size_t nth_zero_position(std::size_t v) const noexcept { return bits_.select0(v + 1); }

    BitVector   bits_{};
    std::size_t node_count_ = 0;
};

} // namespace comdare::succinct
