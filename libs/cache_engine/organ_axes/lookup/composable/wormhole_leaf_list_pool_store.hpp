#pragma once
// V41 Umstufung-A s4 (Task #43) — WormholeLeafListPoolStore: Leaf-Listen-Substrat (erfuellt WormholeLeafListPool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die Wormhole-Korrektheits-Essenz (Wu/Ni/Jiang EuroSys 2019), is_original=
// false ([[pseudocode-papers-fallback]]; wh.c=GPL-3.0, KEIN Code-Copy/Linking — reine Re-Impl aus dem Verstaendnis):
// (1) sortierte DOPPELT-VERKETTETE Leaf-Liste (jeder Leaf = anchor=Minimum-Key + sortierter KV-Block + prev/next),
// (2) geordneter Anchor->Leaf-Index (ein geordneter Baum-Index ersetzt das wh.c-2-Wege-Cuckoo + wormmeta-Meta-Trie
// semantisch; er haengt seit dem A8-S5-Schnitt an der Allokator-Achse, s. unten:
// index_lookup_le = groesster Anchor<=key = geordnete Hash-Jump-Semantik). Der Jump-Descent + Leaf-Split/Merge
// lebt im WormholeJumpTraversalOrgan, NICHT hier. NodeRef = Leaf-Index direkt (nur EIN Kind-Typ), kNil-Sentinel.
//
// Performance/Concurrency (Cuckoo+bswap+SIMD, crc32c-Praefix-Hashing, entry13-Tagged-Pointer, wormmeta-Bitmap,
// Wormref/QSBR, slab-Allocator) ist DRAUSSEN (aendert die Map-Semantik nicht). kWhKpn klein (8) -> erzwingt
// Splits/Merges schon bei wenigen Keys (analog B-Baum kT=4); die echte 128er-Kapazitaet ist Performance-Tuning.
//
// A8-S5 Familie 01a (2026-08-04): Leaf-Vektor, Free-Liste UND der geordnete Anchor-Index beziehen ihren
// Speicher REAL aus der Allocator-Achse (axis_06), Muster BTreeNodePoolStore. Der Anchor-Index ist die
// EINZIGE Knoten-basierte Struktur der Familie: sein Allokator wird intern auf den Baum-Knoten rebindet --
// damit laeuft auch jede einzelne Index-Knoten-Allokation ueber die Achse, nicht nur der Leaf-Block.
// COW-Kopie rebindet alle drei an das eigene allocator_ und verwirft die Kopier-Pollution per
// restore_statistics-Memento; Move ist nicht deklariert -> degradiert sicher zu Copy.

#include "wormhole_leaf_list_pool_concept.hpp"
#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)

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

template <class Alloc = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class WormholeLeafListPoolStore {
public:
    using node_type                     = detail::WormholeLeaf;
    using key_type                      = typename node_type::key_type;
    using value_type                    = typename node_type::value_type;
    using allocator_type                = Alloc;
    using leaf_allocator_type           = typename Alloc::template StdAllocatorAdapter<node_type>;
    using index_allocator_type          = typename Alloc::template StdAllocatorAdapter<std::size_t>;
    using map_value_type                = std::pair<const key_type, std::size_t>;
    using map_allocator_type            = typename Alloc::template StdAllocatorAdapter<map_value_type>;
    static constexpr std::size_t kNil   = detail::kWormholeNil;
    static constexpr int         kWhKpn = detail::kWormholeKpn;
    static constexpr int         kWhMid = detail::kWormholeMid;
    static constexpr int         kWhMrg = detail::kWormholeMrg;
    // Robustheits-Guard (adversariale Verifikation): die Split/Merge-Logik setzt diese Relationen voraus.
    static_assert(kWhMid >= 1 && kWhMid < kWhKpn, "kWhMid muss in [1, kWhKpn) liegen (split_leaf-Korrektheit)");
    static_assert(kWhMrg >= kWhMid && kWhMrg <= kWhKpn,
                  "kWhMrg muss in [kWhMid, kWhKpn] liegen (borrow/merge-Korrektheit)");

    // Default: die drei Container an das eigene allocator_ binden (Adapter nicht default-konstruierbar).
    WormholeLeafListPoolStore()
        : leaves_(allocator_.template as_std_allocator<node_type>()),
          fl_leaf_(allocator_.template as_std_allocator<std::size_t>()),
          anchor_index_(std::less<key_type>{}, allocator_.template as_std_allocator<map_value_type>()) {}
    // COW-Pflicht (Memento): allocator_ mitkopieren, alle drei Container an DAS EIGENE allocator_ rebinden,
    // dann die Kopier-Pollution per restore_statistics verwerfen. Move NICHT deklariert -> degradiert zu Copy.
    WormholeLeafListPoolStore(WormholeLeafListPoolStore const& o)
        : allocator_(o.allocator_), leaves_(o.leaves_, allocator_.template as_std_allocator<node_type>()),
          fl_leaf_(o.fl_leaf_, allocator_.template as_std_allocator<std::size_t>()),
          anchor_index_(o.anchor_index_, allocator_.template as_std_allocator<map_value_type>()), root_(o.root_),
          size_(o.size_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    WormholeLeafListPoolStore& operator=(WormholeLeafListPoolStore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter setzt keine propagate_-Typedefs) -> die Container behalten ihr an
            // this-allocator_ gebundenes Adapter; die Assigns re-allozieren transient ueber this-allocator_.
            leaves_       = o.leaves_;
            fl_leaf_      = o.fl_leaf_;
            anchor_index_ = o.anchor_index_;
            root_         = o.root_;
            size_         = o.size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~WormholeLeafListPoolStore() = default;

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
            idx = leaves_.size();
            leaves_.push_back(node_type{});
        }
        return idx;
    }
    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — free_.push_back kann beim Free-List-Wachstum
    // allozieren/werfen ([[allocation-failure-exception]]: werfen statt terminate; Concept verlangt kein noexcept).
    // KAUSALITAET (Posten 64/70): der Wurf kommt seit dem A8-S5-Schnitt vom StdAllocatorAdapter der
    // Allokator-ACHSE (Strategie meldet OOM per nullptr -> Adapter wirft std::bad_alloc), nicht vom Default.
    void free_node(std::size_t i) { fl_leaf_.push_back(i); }

    // ── Anchor-Index (geordnet) ──
    void index_insert(key_type anchor, std::size_t leaf) {
        auto [it, inserted] = anchor_index_.try_emplace(anchor, leaf);
        if (!inserted) it->second = leaf;
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
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder) -- sie
    /// zaehlt jetzt JEDEN Index-Knoten mit, statt ihn wie frueher mit sizeof(map_value_type) zu schaetzen
    /// (der alte Zaehler war allocator-UNABHAENGIG und nannte den RB-Knoten-Overhead selbst "konservative
    /// Untergrenze"). live_nodes speist der ABI-Adapter im Rich-Zweig aus occupied_count() des Organs.
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using Leaf     = node_type;
    using MapIndex = std::map<key_type, std::size_t, std::less<key_type>, map_allocator_type>;

    // allocator_ VOR den Containern (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    Alloc                                          allocator_{};
    std::vector<Leaf, leaf_allocator_type>         leaves_;
    std::vector<std::size_t, index_allocator_type> fl_leaf_;
    MapIndex                                       anchor_index_;
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das WormholeLeafListPool-Concept.
static_assert(WormholeLeafListPool<WormholeLeafListPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
