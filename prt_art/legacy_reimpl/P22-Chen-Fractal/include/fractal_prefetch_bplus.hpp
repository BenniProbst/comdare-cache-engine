// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P22-Chen-Fractal — Re-Implementation in PRT-ART
// Paper: Fractal Prefetching B+-Trees (Chen et al. 2002)
// Concept: Page + PrefetchStrategy (Achse: Page-Type / Prefetch)
//
// Kernidee: fraktale Disk+Cache-Kombination. Eine Disk-Page enthaelt einen
// cache-optimierten B+-Subtree (Innen-B+ + Aussen-B+).

#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::fractal_bplus {

inline constexpr std::size_t kDiskPageBytes = 4096;
inline constexpr std::size_t kCacheLineBytes = 64;
inline constexpr std::size_t kInnerNodesPerPage = kDiskPageBytes / kCacheLineBytes;

// InnerCacheNode: ein Cache-Line-grosser innerer B+-Knoten innerhalb der Disk-Page
struct InnerCacheNode {
    std::uint32_t              key_count = 0;
    std::array<std::uint32_t, 7> keys{};
    std::array<std::uint32_t, 7> children{};  // indices in same disk page
};

// DiskPageWithInnerBPlus: Disk-Page (4096B) mit 64-Cache-Line-Inner-Trees
class DiskPageWithInnerBPlus {
public:
    std::array<InnerCacheNode, kInnerNodesPerPage> inner_nodes{};
    std::uint32_t                                   inner_node_count = 0;

    int allocate_inner_node() noexcept {
        if (inner_node_count >= kInnerNodesPerPage) return 5;
        return static_cast<int>(inner_node_count++);
    }

    // Lookup mit fraktal-Prefetch: erst Inner-Tree-Lookup, dann optional
    // Cross-Page-Prefetch
    [[nodiscard]] std::uint32_t lookup_fractal(std::uint32_t k) const noexcept {
        if (inner_node_count == 0) return 0;
        auto const& root = inner_nodes[0];
        for (std::uint32_t i = 0; i < root.key_count; ++i) {
            if (root.keys[i] == k) return root.children[i];
            if (root.keys[i] > k) break;
        }
        return 0;
    }

    [[nodiscard]] std::size_t inner_count() const noexcept { return inner_node_count; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return kInnerNodesPerPage; }
};

static_assert(sizeof(InnerCacheNode) <= kCacheLineBytes,
              "InnerCacheNode must fit into one cache line");

}  // namespace comdare::prt_art::legacy_reimpl::fractal_bplus
