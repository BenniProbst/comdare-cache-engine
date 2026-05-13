// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P26-Zhang-FGCS — Re-Implementation in PRT-ART
// Paper: A prefetching indexing scheme for in-memory database systems
//        (Q. Zhang et al. FGCS 2024)
// Concept: PrefetchStrategy + Telemetry (Achse: Prefetch / Measurement)
//
// Kernidee: Zwei Prefetcher-Kategorien (Path + Jump-Pointer) je Block, mit
// Read-Counter und Hit/Miss-Statistiken zur dynamischen Wahl.

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::zhang_fgcs {

enum class PrefetchKind : std::uint8_t {
    Path        = 0,
    JumpPointer = 1
};

struct BlockStats {
    std::uint64_t read_count       = 0;
    std::uint64_t path_hits        = 0;
    std::uint64_t path_misses      = 0;
    std::uint64_t jump_hits        = 0;
    std::uint64_t jump_misses      = 0;
};

class PathJumpPointerPrefetcher {
public:
    void record_read(std::uint64_t block_id) {
        ++stats_[block_id].read_count;
    }

    void record_path_hit(std::uint64_t block_id)   { ++stats_[block_id].path_hits; }
    void record_path_miss(std::uint64_t block_id)  { ++stats_[block_id].path_misses; }
    void record_jump_hit(std::uint64_t block_id)   { ++stats_[block_id].jump_hits; }
    void record_jump_miss(std::uint64_t block_id)  { ++stats_[block_id].jump_misses; }

    // Wahl zwischen Path und Jump-Pointer basierend auf Hit-Rate
    [[nodiscard]] PrefetchKind recommend(std::uint64_t block_id) const {
        auto it = stats_.find(block_id);
        if (it == stats_.end()) return PrefetchKind::Path;
        auto const& s = it->second;
        std::uint64_t const path_total = s.path_hits + s.path_misses;
        std::uint64_t const jump_total = s.jump_hits + s.jump_misses;
        double const path_rate = path_total > 0
            ? static_cast<double>(s.path_hits) / static_cast<double>(path_total) : 0.0;
        double const jump_rate = jump_total > 0
            ? static_cast<double>(s.jump_hits) / static_cast<double>(jump_total) : 0.0;
        return path_rate >= jump_rate ? PrefetchKind::Path : PrefetchKind::JumpPointer;
    }

    [[nodiscard]] BlockStats const* stats_for(std::uint64_t block_id) const {
        auto it = stats_.find(block_id);
        return it == stats_.end() ? nullptr : &it->second;
    }

    [[nodiscard]] std::size_t tracked_blocks() const noexcept { return stats_.size(); }

private:
    std::unordered_map<std::uint64_t, BlockStats> stats_{};
};

}  // namespace comdare::prt_art::legacy_reimpl::zhang_fgcs
