// SPDX-License-Identifier: Apache-2.0
// V41.A5 (2026-05-24) - 4-bit Radix-Tree (ART-vereinfacht)
//
// Echter Trie statt STL: 4-bit nibbles aus uint32-Keys, 16-Eintrag-Nodes,
// Pfad-Knoten-Kette. Im Vergleich zu std::map (rot-schwarz-baum) hat dieser
// O(log_16 N) statt O(log_2 N) Tiefe und nutzt SIMD-freundliches Layout.

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace comdare::cache_engine::indexes {

class RadixIndex {
public:
    static constexpr std::size_t kFanOut = 16;  // 4-bit nibbles

    RadixIndex() : root_{std::make_unique<Node>()} {}

    void insert(std::uint32_t key) {
        Node* node = root_.get();
        for (int shift = 28; shift >= 0; shift -= 4) {
            std::uint8_t nibble = static_cast<std::uint8_t>((key >> shift) & 0xF);
            if (!node->children[nibble]) {
                node->children[nibble] = std::make_unique<Node>();
            }
            node = node->children[nibble].get();
        }
        if (!node->present) {
            node->present = true;
            ++size_;
        }
    }

    [[nodiscard]] bool contains(std::uint32_t key) const {
        Node const* node = root_.get();
        for (int shift = 28; shift >= 0; shift -= 4) {
            std::uint8_t nibble = static_cast<std::uint8_t>((key >> shift) & 0xF);
            Node const* next = node->children[nibble].get();
            if (!next) return false;
            node = next;
        }
        return node->present;
    }

    [[nodiscard]] std::size_t size() const { return size_; }

private:
    struct Node {
        std::array<std::unique_ptr<Node>, kFanOut> children {};
        bool present {false};
    };
    std::unique_ptr<Node> root_;
    std::size_t           size_ {0};
};

}  // namespace comdare::cache_engine::indexes
