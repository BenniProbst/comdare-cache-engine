#pragma once
// V41 Umstufung-A s4 (Task #43) — WormholeLeafListPoolStore: Leaf-Listen-Substrat (erfuellt WormholeLeafListPool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die Wormhole-Korrektheits-Essenz (Wu/Ni/Jiang EuroSys 2019), is_original=
// false ([[pseudocode-papers-fallback]]; wh.c=GPL-3.0, KEIN Code-Copy/Linking — reine Re-Impl aus dem Verstaendnis):
// (1) sortierte DOPPELT-VERKETTETE Leaf-Liste (jeder Leaf = anchor=Minimum-Key + sortierter KV-Block + prev/next),
// (2) geordneter Anchor->Leaf-Index (std::map ersetzt das wh.c-2-Wege-Cuckoo + wormmeta-Meta-Trie semantisch:
// index_lookup_le = groesster Anchor<=key = geordnete Hash-Jump-Semantik). Der Jump-Descent + Leaf-Split/Merge
// lebt im WormholeJumpTraversalOrgan, NICHT hier. NodeRef = Leaf-Index direkt (nur EIN Kind-Typ), kNil-Sentinel.
//
// Performance/Concurrency (Cuckoo+bswap+SIMD, crc32c-Praefix-Hashing, entry13-Tagged-Pointer, wormmeta-Bitmap,
// Wormref/QSBR, slab-Allocator) ist DRAUSSEN (aendert die Map-Semantik nicht). kWhKpn klein (8) -> erzwingt
// Splits/Merges schon bei wenigen Keys (analog B-Baum kT=4); die echte 128er-Kapazitaet ist Performance-Tuning.

#include "wormhole_leaf_list_pool_concept.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

class WormholeLeafListPoolStore {
public:
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
    static constexpr std::size_t kNil   = std::numeric_limits<std::size_t>::max();
    static constexpr int         kWhKpn = 8;   // Keys/Leaf (klein -> erzwingt Split/Merge)
    static constexpr int         kWhMid = 4;   // Split-Punkt
    static constexpr int         kWhMrg = 6;   // Borrow/Merge-Schwelle (~3/4 kWhKpn)
    // Robustheits-Guard (adversariale Verifikation): die Split/Merge-Logik setzt diese Relationen voraus.
    static_assert(kWhMid >= 1 && kWhMid < kWhKpn, "kWhMid muss in [1, kWhKpn) liegen (split_leaf-Korrektheit)");
    static_assert(kWhMrg >= kWhMid && kWhMrg <= kWhKpn, "kWhMrg muss in [kWhMid, kWhKpn] liegen (borrow/merge-Korrektheit)");

    // ── Wurzel (= Listenkopf) + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void set_root(std::size_t r) noexcept { root_ = r; }
    void inc_size()              noexcept { ++size_; }
    void dec_size()              noexcept { --size_; }
    void clear() noexcept {
        leaves_.clear(); fl_leaf_.clear(); anchor_index_.clear();
        root_ = kNil; size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] int        leaf_n(std::size_t i)            const noexcept { return leaves_[i].n; }
    [[nodiscard]] key_type   leaf_key_at(std::size_t i, int j)   const noexcept { return leaves_[i].key[static_cast<std::size_t>(j)]; }
    [[nodiscard]] value_type leaf_value_at(std::size_t i, int j) const noexcept { return leaves_[i].val[static_cast<std::size_t>(j)]; }
    [[nodiscard]] key_type   leaf_anchor(std::size_t i)       const noexcept { return leaves_[i].anchor; }
    [[nodiscard]] std::size_t leaf_prev(std::size_t i)        const noexcept { return leaves_[i].prev; }
    [[nodiscard]] std::size_t leaf_next(std::size_t i)        const noexcept { return leaves_[i].next; }
    void set_leaf_n(std::size_t i, int n)                 noexcept { leaves_[i].n = n; }
    void set_leaf_key_at(std::size_t i, int j, key_type k)     noexcept { leaves_[i].key[static_cast<std::size_t>(j)] = k; }
    void set_leaf_value_at(std::size_t i, int j, value_type v) noexcept { leaves_[i].val[static_cast<std::size_t>(j)] = v; }
    void set_leaf_anchor(std::size_t i, key_type k)       noexcept { leaves_[i].anchor = k; }
    void set_leaf_prev(std::size_t i, std::size_t p)      noexcept { leaves_[i].prev = p; }
    void set_leaf_next(std::size_t i, std::size_t n)      noexcept { leaves_[i].next = n; }
    [[nodiscard]] std::size_t new_leaf() {
        std::size_t idx;
        if (!fl_leaf_.empty()) { idx = fl_leaf_.back(); fl_leaf_.pop_back(); leaves_[idx] = Leaf{}; }
        else { idx = leaves_.size(); leaves_.push_back(Leaf{}); }
        return idx;
    }
    void free_node(std::size_t i) noexcept { fl_leaf_.push_back(i); }

    // ── Anchor-Index (geordnet) ──
    void index_insert(key_type anchor, std::size_t leaf) { anchor_index_[anchor] = leaf; }
    void index_erase(key_type anchor)                    { anchor_index_.erase(anchor); }
    /// Groesster Anchor <= key -> sein Leaf; kNil falls key < allen Ankern (Linear-Fallback im Organ).
    [[nodiscard]] std::size_t index_lookup_le(key_type key) const noexcept {
        auto it = anchor_index_.upper_bound(key);      // erster Anchor > key
        if (it == anchor_index_.begin()) return kNil;  // key < allen Ankern
        --it;
        return it->second;
    }
    void index_clear() noexcept { anchor_index_.clear(); }

private:
    struct Leaf {
        int         n = 0;
        key_type    anchor = 0;
        std::size_t prev = kNil;
        std::size_t next = kNil;
        // Kapazitaet kWhKpn+1: ein Insert darf ein (durch Merge) bis kWhKpn gefuelltes Leaf transient auf
        // kWhKpn+1 bringen, BEVOR der Split greift (sonst Array-Overflow — adversariale Verifikation).
        std::array<key_type,   kWhKpn + 1> key{};
        std::array<value_type, kWhKpn + 1> val{};
    };

    std::vector<Leaf>               leaves_{};
    std::vector<std::size_t>        fl_leaf_{};
    std::map<key_type, std::size_t> anchor_index_{};
    std::size_t                     root_ = kNil;
    std::size_t                     size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das WormholeLeafListPool-Concept.
static_assert(WormholeLeafListPool<WormholeLeafListPoolStore>);

}  // namespace comdare::cache_engine::lookup::composable
