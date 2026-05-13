// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// P17-Bender-CacheOblivious — Re-Implementation in PRT-ART
// Paper: Cache-Oblivious B-Trees (Bender/Demaine/Farach-Colton 2005)
// Concept: MemoryLayout (Achse: Memory-Layout)
//
// Kernidee: van-Emde-Boas (vEB) Layout — rekursive Aufspaltung der Baumhoehe
// in Top-Subtree (ceil(H/2)) + sqrt(N) viele Bottom-Subtrees (floor(H/2)),
// JEDER davon selbst rekursiv vEB-gelayoutet.
// Resultat: O(log_B N) Cache-Misses OHNE Kenntnis von Cache-Parametern.
//
// Verifikation (Test mit H=4): Index-Reihenfolge ist NICHT BFS, sondern
// vEB — Kind-Bloecke werden lokal zusammengepackt.

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::prt_art::legacy_reimpl::cache_oblivious_layout {

class VebLayout {
public:
    explicit VebLayout(std::size_t tree_height) : height_(tree_height) {
        std::size_t const node_count = (height_ == 0) ? 0 : ((1ULL << height_) - 1);
        veb_index_of_node_.resize(node_count, 0);

        if (node_count == 0) return;
        std::size_t next_index = 0;
        layout_veb(0, height_, next_index);
    }

    [[nodiscard]] std::size_t veb_index(std::size_t node_id) const {
        return node_id < veb_index_of_node_.size()
            ? veb_index_of_node_[node_id] : 0;
    }

    [[nodiscard]] std::size_t height()     const noexcept { return height_; }
    [[nodiscard]] std::size_t node_count() const noexcept { return veb_index_of_node_.size(); }

private:
    // Rekursives van-Emde-Boas-Layout fuer Sub-Tree mit Wurzel root_id und
    // Hoehe sub_height. Knoten werden an konsekutive Indizes ab next_index
    // zugewiesen, next_index wird entsprechend hochgezaehlt.
    void layout_veb(std::size_t root_id, std::size_t sub_height, std::size_t& next_index) {
        if (sub_height == 0 || root_id >= veb_index_of_node_.size()) return;

        if (sub_height == 1) {
            veb_index_of_node_[root_id] = next_index++;
            return;
        }

        std::size_t const top_h = (sub_height + 1) / 2;   // ceiling
        std::size_t const bot_h = sub_height - top_h;     // floor

        // 1) Top-Subtree REKURSIV vEB layouten (NICHT BFS!)
        layout_veb(root_id, top_h, next_index);

        // 2) Sammle alle Bottom-Wurzeln: Kinder der Top-Leaves
        std::vector<std::size_t> bottom_roots = collect_bottom_roots(root_id, top_h);

        // 3) Jeder Bottom-Subtree rekursiv vEB layouten
        for (auto br : bottom_roots) {
            layout_veb(br, bot_h, next_index);
        }
    }

    // Sammelt die Wurzeln aller Bottom-Subtrees:
    // = Kinder der Top-Leaves (Knoten auf Level top_h-1 unter root_id).
    [[nodiscard]] std::vector<std::size_t>
    collect_bottom_roots(std::size_t root_id, std::size_t top_h) const {
        std::vector<std::size_t> current{root_id};
        // BFS durch den Top-Subtree bis Level top_h-1
        for (std::size_t lvl = 1; lvl < top_h; ++lvl) {
            std::vector<std::size_t> next_level;
            for (auto n : current) {
                std::size_t l = n * 2 + 1;
                std::size_t r = n * 2 + 2;
                if (l < veb_index_of_node_.size()) next_level.push_back(l);
                if (r < veb_index_of_node_.size()) next_level.push_back(r);
            }
            current = std::move(next_level);
        }
        // current haelt jetzt die Top-Leaves. Kinder = Bottom-Roots.
        std::vector<std::size_t> bot_roots;
        bot_roots.reserve(current.size() * 2);
        for (auto leaf : current) {
            std::size_t l = leaf * 2 + 1;
            std::size_t r = leaf * 2 + 2;
            if (l < veb_index_of_node_.size()) bot_roots.push_back(l);
            if (r < veb_index_of_node_.size()) bot_roots.push_back(r);
        }
        return bot_roots;
    }

    std::size_t              height_;
    std::vector<std::size_t> veb_index_of_node_;
};

}  // namespace comdare::prt_art::legacy_reimpl::cache_oblivious_layout
