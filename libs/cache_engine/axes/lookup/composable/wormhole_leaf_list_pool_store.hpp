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
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

inline constexpr std::size_t kWormholeNil = std::numeric_limits<std::size_t>::max();
inline constexpr int         kWormholeKpn = 8; // Keys/Leaf (klein -> erzwingt Split/Merge)
inline constexpr int         kWormholeMid = 4; // Split-Punkt
inline constexpr int         kWormholeMrg = 6; // Borrow/Merge-Schwelle (~3/4 kWhKpn)

struct WormholeLeaf {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    int         n      = 0;
    key_type    anchor = 0;
    std::size_t prev   = kWormholeNil;
    std::size_t next   = kWormholeNil;
    // Kapazitaet kWhKpn+1: ein Insert darf ein (durch Merge) bis kWhKpn gefuelltes Leaf transient auf
    // kWhKpn+1 bringen, BEVOR der Split greift (sonst Array-Overflow — adversariale Verifikation).
    std::array<key_type, kWormholeKpn + 1>   key{};
    std::array<value_type, kWormholeKpn + 1> val{};
};

} // namespace detail

template <class A = std::allocator<detail::WormholeLeaf>>
class WormholeLeafListPoolStore {
public:
    using node_type                     = detail::WormholeLeaf;
    using key_type                      = typename node_type::key_type;
    using value_type                    = typename node_type::value_type;
    using allocator_type                = A;
    using leaf_allocator_type           = typename std::allocator_traits<A>::template rebind_alloc<node_type>;
    using index_allocator_type          = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;
    using map_value_type                = std::pair<const key_type, std::size_t>;
    using map_allocator_type            = typename std::allocator_traits<A>::template rebind_alloc<map_value_type>;
    static constexpr std::size_t kNil   = detail::kWormholeNil;
    static constexpr int         kWhKpn = detail::kWormholeKpn;
    static constexpr int         kWhMid = detail::kWormholeMid;
    static constexpr int         kWhMrg = detail::kWormholeMrg;
    // Robustheits-Guard (adversariale Verifikation): die Split/Merge-Logik setzt diese Relationen voraus.
    static_assert(kWhMid >= 1 && kWhMid < kWhKpn, "kWhMid muss in [1, kWhKpn) liegen (split_leaf-Korrektheit)");
    static_assert(kWhMrg >= kWhMid && kWhMrg <= kWhKpn,
                  "kWhMrg muss in [kWhMid, kWhKpn] liegen (borrow/merge-Korrektheit)");

    // ── Wurzel (= Listenkopf) + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void                      set_root(std::size_t r) noexcept { root_ = r; }
    void                      inc_size() noexcept { ++size_; }
    void                      dec_size() noexcept { --size_; }
    void                      clear() noexcept {
        leaves_.clear();
        fl_leaf_.clear();
        anchor_index_.clear();
        root_ = kNil;
        size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] int      leaf_n(std::size_t i) const noexcept { return leaves_[i].n; }
    [[nodiscard]] key_type leaf_key_at(std::size_t i, int j) const noexcept {
        return leaves_[i].key[static_cast<std::size_t>(j)];
    }
    [[nodiscard]] value_type leaf_value_at(std::size_t i, int j) const noexcept {
        return leaves_[i].val[static_cast<std::size_t>(j)];
    }
    [[nodiscard]] key_type    leaf_anchor(std::size_t i) const noexcept { return leaves_[i].anchor; }
    [[nodiscard]] std::size_t leaf_prev(std::size_t i) const noexcept { return leaves_[i].prev; }
    [[nodiscard]] std::size_t leaf_next(std::size_t i) const noexcept { return leaves_[i].next; }
    void                      set_leaf_n(std::size_t i, int n) noexcept { leaves_[i].n = n; }
    void set_leaf_key_at(std::size_t i, int j, key_type k) noexcept { leaves_[i].key[static_cast<std::size_t>(j)] = k; }
    void set_leaf_value_at(std::size_t i, int j, value_type v) noexcept {
        leaves_[i].val[static_cast<std::size_t>(j)] = v;
    }
    void                      set_leaf_anchor(std::size_t i, key_type k) noexcept { leaves_[i].anchor = k; }
    void                      set_leaf_prev(std::size_t i, std::size_t p) noexcept { leaves_[i].prev = p; }
    void                      set_leaf_next(std::size_t i, std::size_t n) noexcept { leaves_[i].next = n; }
    [[nodiscard]] std::size_t new_leaf() {
        std::size_t idx;
        if (!fl_leaf_.empty()) {
            idx = fl_leaf_.back();
            fl_leaf_.pop_back();
            leaves_[idx] = Leaf{};
        } else {
            idx                            = leaves_.size();
            std::size_t const old_capacity = leaves_.capacity();
            leaves_.push_back(node_type{});
            record_capacity_growth_(old_capacity, leaves_.capacity(), sizeof(node_type));
        }
        return idx;
    }
    void free_node(std::size_t i) noexcept {
        std::size_t const old_capacity = fl_leaf_.capacity();
        fl_leaf_.push_back(i);
        record_capacity_growth_(old_capacity, fl_leaf_.capacity(), sizeof(std::size_t));
    }

    // ── Anchor-Index (geordnet) ──
    void index_insert(key_type anchor, std::size_t leaf) {
        auto [it, inserted] = anchor_index_.try_emplace(anchor, leaf);
        if (!inserted) {
            it->second = leaf;
            return;
        }
        record_map_insert_();
    }
    void index_erase(key_type anchor) { anchor_index_.erase(anchor); }
    /// Groesster Anchor <= key -> sein Leaf; kNil falls key < allen Ankern (Linear-Fallback im Organ).
    [[nodiscard]] std::size_t index_lookup_le(key_type key) const noexcept {
        auto it = anchor_index_.upper_bound(key);     // erster Anchor > key
        if (it == anchor_index_.begin()) return kNil; // key < allen Ankern
        --it;
        return it->second;
    }
    void index_clear() noexcept { anchor_index_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{
            alloc_calls_,
            bytes_allocated_,
            static_cast<std::uint64_t>(leaves_.size() - fl_leaf_.size()),
        };
    }
#endif

private:
    using Leaf     = node_type;
    using MapIndex = std::map<key_type, std::size_t, std::less<key_type>, map_allocator_type>;

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Ehrliche Allokator-Metrik: gezaehlt werden nur erfolgreiche vector-capacity-Zuwaechse (als Capacity-Delta
    // mal Elementgroesse) sowie NEU eingefuegte Map-Werte. Reuse/clear ohne Wachstum erzeugt bewusst keine
    // kuenstlichen Werte.
    void record_capacity_growth_(std::size_t old_capacity, std::size_t new_capacity, std::size_t elem_bytes) noexcept {
        if (new_capacity <= old_capacity) return;
        ++alloc_calls_;
        bytes_allocated_ +=
            static_cast<std::uint64_t>(new_capacity - old_capacity) * static_cast<std::uint64_t>(elem_bytes);
    }
    void record_map_insert_() noexcept {
        ++alloc_calls_;
        // konservative Untergrenze (RB-Node-Overhead bewusst nicht fabriziert).
        bytes_allocated_ += static_cast<std::uint64_t>(sizeof(map_value_type));
    }
#else
    static void record_capacity_growth_(std::size_t, std::size_t, std::size_t) noexcept {}
    static void record_map_insert_() noexcept {}
#endif

    std::vector<Leaf, leaf_allocator_type>         leaves_{};
    std::vector<std::size_t, index_allocator_type> fl_leaf_{};
    MapIndex                                       anchor_index_{};
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das WormholeLeafListPool-Concept.
static_assert(WormholeLeafListPool<WormholeLeafListPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
