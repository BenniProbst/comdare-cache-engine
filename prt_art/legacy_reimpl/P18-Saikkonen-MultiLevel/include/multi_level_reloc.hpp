// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P18-Saikkonen-MultiLevel — Re-Implementation in PRT-ART
// Paper: Multi-Level Block Reorganization (Saikkonen/Soisalon-Soininen 2008)
// Concept: Allocator + Relocation (Achse: Memory-Layout / Allocator)
//
// Kernidee: BFS-Reorganisation ueber Block-Hierarchie. Hot Blocks werden
// in hoehere Cache-Level migriert; Cold Blocks zurueck.

#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::multi_level_reloc {

struct Block {
    std::uint64_t block_id      = 0;
    std::uint32_t level         = 0;        // 0 = L1, 1 = L2, ...
    std::uint64_t access_count  = 0;
    std::uint64_t last_used_ts  = 0;
};

class MultiLevelRelocation {
public:
    explicit MultiLevelRelocation(std::uint32_t max_levels = 4) noexcept
        : max_levels_(max_levels) {}

    int add_block(Block b) {
        if (b.level >= max_levels_) return 4;  // invalid_argument
        blocks_.push_back(b);
        return 0;
    }

    // BFS-Reorganisation: alle Blocks werden Level-fuer-Level besucht.
    // Hot-Threshold = access_count > median -> Migration nach Level-1
    void reorganize_bfs(std::uint64_t hot_threshold) {
        std::queue<std::size_t> queue;
        for (std::size_t i = 0; i < blocks_.size(); ++i) queue.push(i);

        while (!queue.empty()) {
            std::size_t idx = queue.front();
            queue.pop();
            auto& b = blocks_[idx];
            if (b.access_count > hot_threshold && b.level > 0) {
                --b.level;   // Hot -> hoeherer Cache-Level (L1-naeher)
                ++migrations_;
            } else if (b.access_count == 0 && b.level + 1 < max_levels_) {
                ++b.level;   // Cold -> tieferer Level
                ++migrations_;
            }
        }
    }

    [[nodiscard]] std::size_t                block_count()  const noexcept { return blocks_.size(); }
    [[nodiscard]] std::uint64_t              migrations()   const noexcept { return migrations_; }
    [[nodiscard]] std::vector<Block> const& blocks()       const noexcept { return blocks_; }

private:
    std::uint32_t      max_levels_;
    std::vector<Block> blocks_{};
    std::uint64_t      migrations_ = 0;
};

}  // namespace comdare::prt_art::legacy_reimpl::multi_level_reloc
