// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P16-Bender-TreeLayout — Re-Implementation in PRT-ART
// Paper: Tree Layout in Multilevel Memory (Bender/Demaine/Farach-Colton 2002)
// Concept: MemoryLayout (Achse: Memory-Layout)
//
// Kernidee: Greedy Layout-Algorithmus mit O(N log B), der Knoten nach
// ihrer Zugriffs-Wahrscheinlichkeit packt: oft besuchte Knoten zusammen
// mit ihren Eltern in dieselbe Cache-Line/Block.

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::probability_layout {

struct AccessNode {
    std::uint64_t node_id          = 0;
    double        access_probability = 0.0;
    std::uint64_t parent_id        = 0;
};

// LayoutBlock: ein "Block" (Cache-Line/Memory-Page) mit fester Kapazitaet.
struct LayoutBlock {
    std::uint64_t              block_id   = 0;
    double                     density    = 0.0;
    std::vector<std::uint64_t> node_ids{};
};

// ProbabilityLayout: Greedy Layout-Algorithmus
class ProbabilityLayout {
public:
    explicit ProbabilityLayout(std::size_t block_capacity) noexcept
        : block_capacity_(block_capacity) {}

    void add_node(AccessNode n) {
        nodes_.push_back(n);
    }

    // Greedy: sortiere absteigend nach access_probability,
    // packe in Bloecke bis Kapazitaet erreicht.
    std::vector<LayoutBlock> compute_layout() const {
        std::vector<AccessNode> sorted(nodes_);
        std::sort(sorted.begin(), sorted.end(),
            [](AccessNode const& a, AccessNode const& b) {
                return a.access_probability > b.access_probability;
            });

        std::vector<LayoutBlock> blocks;
        LayoutBlock current{0, 0.0, {}};
        for (auto const& n : sorted) {
            if (current.node_ids.size() >= block_capacity_) {
                blocks.push_back(current);
                current = LayoutBlock{static_cast<std::uint64_t>(blocks.size()), 0.0, {}};
            }
            current.node_ids.push_back(n.node_id);
            current.density += n.access_probability;
        }
        if (!current.node_ids.empty()) blocks.push_back(current);
        return blocks;
    }

    [[nodiscard]] std::size_t node_count() const noexcept { return nodes_.size(); }
    [[nodiscard]] std::size_t block_capacity() const noexcept { return block_capacity_; }

private:
    std::size_t              block_capacity_;
    std::vector<AccessNode>  nodes_{};
};

}  // namespace comdare::prt_art::legacy_reimpl::probability_layout
