#pragma once
// V41 Umstufung-A s4 (Task #43) — StartTrieNodePoolStore: Multibyte-Span-Radix-Substrat (erfuellt StartTrieNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die START-Korrektheits-Basis (Fent et al. ICDEW 2020, is_original=false
// [[pseudocode-papers-fallback]]): Leaf (Key+Value inline) + Inner-Knoten mit per-Node SPAN (1/2/3 Bytes
// Diskriminator-Breite) + SPARSE Diskriminator->Kind-Dispatch (sortierte Paare, Binaersuche) + ByteWiseKeyPrefix-
// Path-Compression (axis_02). Der span-1-Fall degeneriert zu ART (Korrektheits-Anker); span-2/3 sind ARTs
// CHARAKTERISTISCHE START-Erweiterung (Rewired64K/16M als portable C++23-Sparse-Dispatch statt Linux-Page-
// Rewiring). Der span-aware Descent lebt im StartTrieTraversalOrgan, NICHT hier.
//
// **Vereinfachung ggue. ART (bewusst, S1):** EINE sparse-sortierte Kind-Liste je Inner-Knoten (statt ARTs
// adaptiver Node4/16/48/256) — die adaptiven Knoten-TYPEN sind eine axis_04-Dimension, die START mit ART
// teilt (Folge-Refinement). Die START-DISTINKTION ist der Multibyte-Span. Cost-DP-Span-Wahl = axis_03t (Folge).
//
// A8-S5 Familie 01a (2026-08-04): Leaf-, Inner- und Free-List-Speicher kommen REAL aus der Allocator-Achse
// (axis_06), Muster BTreeNodePoolStore. BESONDERHEIT dieser Familie (wie Skip-Liste): der Inner-Knoten traegt
// SELBST zwei Container (disc/kids) -- die Bindung liegt deshalb auf BEIDEN Ebenen (Inner-Vektor UND die beiden
// Vektoren IM Inner), sonst allozierten die Kind-Listen still am Default-Allokator vorbei. Weil der
// StdAllocatorAdapter &allocator_ haelt, rebindet die COW-Kopie beide Ebenen explizit (copy_inners_from_) und
// verwirft die Kopier-Pollution per restore_statistics-Memento; Move degradiert sicher zu Copy.

#include "start_trie_node_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp> // ByteWiseKeyPrefix

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

using StartTriePrefix = ::comdare::cache_engine::nodes::axis_02_path_compression::ByteWiseKeyPrefix;

inline constexpr std::size_t kStartTrieNil = std::numeric_limits<std::size_t>::max();

struct StartTrieLeaf {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    key_type   key{};
    value_type val{};
};

/// A8-S5: KEIN Default-Template-Argument mehr -- die beiden Kind-Listen MUESSEN mit dem Achsen-Adapter des
/// besitzenden Stores parametriert werden (ein Default waere genau die stille Default-Allokator-Bindung, die
/// der Schnitt abbaut). Der Adapter ist nicht default-konstruierbar; jeder Inner-Knoten wird deshalb explizit
/// ueber StartTrieNodePoolStore::make_inner_() gebaut, nie per `Inner{}`.
template <class DiscAlloc, class KidsAlloc>
struct StartTrieInner {
    using disc_allocator_type = DiscAlloc;
    using kids_allocator_type = KidsAlloc;
    using disc_vector_type    = std::vector<std::uint32_t, disc_allocator_type>;
    using kids_vector_type    = std::vector<std::size_t, kids_allocator_type>;

    StartTriePrefix  prefix{};
    std::uint8_t     span = 1;
    disc_vector_type disc; // aufsteigend sortierte Diskriminatoren (span-breit)
    kids_vector_type kids; // parallel: Kind-Refs
};

} // namespace detail

template <class Alloc = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class StartTrieNodePoolStore {
public:
    using node_type                   = detail::StartTrieLeaf;
    using key_type                    = typename node_type::key_type;
    using value_type                  = typename node_type::value_type;
    using prefix_type                 = detail::StartTriePrefix;
    using allocator_type              = Alloc;
    using leaf_allocator_type         = typename Alloc::template StdAllocatorAdapter<detail::StartTrieLeaf>;
    using forward_allocator_type      = typename Alloc::template StdAllocatorAdapter<std::size_t>;
    using disc_allocator_type         = typename Alloc::template StdAllocatorAdapter<std::uint32_t>;
    using inner_type                  = detail::StartTrieInner<disc_allocator_type, forward_allocator_type>;
    using inner_allocator_type        = typename Alloc::template StdAllocatorAdapter<inner_type>;
    using index_allocator_type        = typename Alloc::template StdAllocatorAdapter<std::size_t>;
    static constexpr std::size_t kNil = detail::kStartTrieNil;

    // Default: die vier Vektoren an das eigene allocator_ binden (Adapter nicht default-konstruierbar).
    StartTrieNodePoolStore()
        : leaves_(allocator_.template as_std_allocator<detail::StartTrieLeaf>()),
          inners_(allocator_.template as_std_allocator<inner_type>()),
          fl_leaf_(allocator_.template as_std_allocator<std::size_t>()),
          fl_inner_(allocator_.template as_std_allocator<std::size_t>()) {}
    // COW-Pflicht (Memento): allocator_ mitkopieren; die Blatt-/Free-Vektoren rebinden per Vektor-Copy-Ctor,
    // die INNER-Knoten dagegen einzeln (copy_inners_from_) -- sonst haetten disc/kids der Kopie weiterhin am
    // Allokator der QUELLE gehangen (stille Cross-Strategy-Allokation + dangling &allocator_).
    StartTrieNodePoolStore(StartTrieNodePoolStore const& o)
        : allocator_(o.allocator_), leaves_(o.leaves_, allocator_.template as_std_allocator<detail::StartTrieLeaf>()),
          inners_(allocator_.template as_std_allocator<inner_type>()),
          fl_leaf_(o.fl_leaf_, allocator_.template as_std_allocator<std::size_t>()),
          fl_inner_(o.fl_inner_, allocator_.template as_std_allocator<std::size_t>()), root_(o.root_), size_(o.size_) {
        copy_inners_from_(o.inners_);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    StartTrieNodePoolStore& operator=(StartTrieNodePoolStore const& o) {
        if (this != &o) {
            leaves_ = o.leaves_;
            inners_.clear();
            copy_inners_from_(o.inners_); // disc/kids erneut an DAS EIGENE allocator_ gebunden
            fl_leaf_  = o.fl_leaf_;
            fl_inner_ = o.fl_inner_;
            root_     = o.root_;
            size_     = o.size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~StartTrieNodePoolStore() = default;

    enum Kind : std::uint8_t { kLeaf = 0, kInner = 1 };
    [[nodiscard]] static constexpr std::size_t make_ref(Kind kind, std::size_t idx) noexcept {
        return (static_cast<std::size_t>(kind) << 56U) | idx;
    }
    [[nodiscard]] static constexpr Kind        ref_kind(std::size_t r) noexcept { return static_cast<Kind>(r >> 56U); }
    [[nodiscard]] static constexpr std::size_t ref_idx(std::size_t r) noexcept { return r & 0x00FF'FFFF'FFFF'FFFFULL; }

    // ── Wurzel + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void                      set_root(std::size_t r) noexcept { root_ = r; }
    void                      inc_size() noexcept { ++size_; }
    void                      dec_size() noexcept { --size_; }
    void                      clear() noexcept {
        leaves_.clear();
        inners_.clear();
        fl_leaf_.clear();
        fl_inner_.clear();
        root_ = kNil;
        size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] bool        is_leaf(std::size_t r) const noexcept { return ref_kind(r) == kLeaf; }
    [[nodiscard]] key_type    leaf_key(std::size_t r) const noexcept { return leaves_[ref_idx(r)].key; }
    [[nodiscard]] value_type  leaf_value(std::size_t r) const noexcept { return leaves_[ref_idx(r)].val; }
    void                      set_leaf_value(std::size_t r, value_type v) noexcept { leaves_[ref_idx(r)].val = v; }
    [[nodiscard]] std::size_t new_leaf(key_type k, value_type v) {
        std::size_t idx;
        if (!fl_leaf_.empty()) {
            idx = fl_leaf_.back();
            fl_leaf_.pop_back();
            leaves_[idx] = node_type{k, v};
        } else {
            idx = leaves_.size();
            leaves_.push_back(node_type{k, v});
        }
        return make_ref(kLeaf, idx);
    }

    // ── Inner (Multibyte-Span) ──
    [[nodiscard]] std::size_t new_inner(unsigned sp) {
        std::size_t idx;
        if (!fl_inner_.empty()) {
            idx = fl_inner_.back();
            fl_inner_.pop_back();
            inners_[idx] = make_inner_(); // frische, an das eigene allocator_ gebundene Kind-Listen
        } else {
            idx = inners_.size();
            inners_.push_back(make_inner_());
        }
        inners_[idx].span = static_cast<std::uint8_t>(sp);
        return make_ref(kInner, idx);
    }
    [[nodiscard]] unsigned    span(std::size_t r) const noexcept { return inners_[ref_idx(r)].span; }
    [[nodiscard]] prefix_type prefix_of(std::size_t r) const noexcept { return inners_[ref_idx(r)].prefix; }
    void                      set_prefix(std::size_t r, prefix_type p) noexcept { inners_[ref_idx(r)].prefix = p; }
    void                      prefix_cut(std::size_t r, unsigned n) noexcept { inners_[ref_idx(r)].prefix.cut(n); }

    [[nodiscard]] std::size_t find_child(std::size_t r, std::uint32_t disc) const noexcept {
        Inner const& x  = inners_[ref_idx(r)];
        auto         it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        if (it != x.disc.end() && *it == disc) return x.kids[static_cast<std::size_t>(it - x.disc.begin())];
        return kNil;
    }
    // #188-4b-DEG1: const-Iteration ueber die reale sparse Diskriminator->Kind-Struktur (keine 24-Bit-Vollsuche).
    [[nodiscard]] std::size_t child_count(std::size_t r) const noexcept { return inners_[ref_idx(r)].kids.size(); }

    [[nodiscard]] std::size_t child_at(std::size_t r, std::size_t i) const noexcept {
        return inners_[ref_idx(r)].kids[i];
    }
    /// Fuegt (disc, child) sortiert ein (disc ist garantiert NICHT vorhanden) — darf via vector werfen.
    void add_child(std::size_t r, std::uint32_t disc, std::size_t child) {
        Inner& x  = inners_[ref_idx(r)];
        auto   it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        // Praekondition (Concept Z.63): disc NICHT vorhanden. DEBUG-Guard, damit ein kuenftiger
        // axis_03t/Gattungs-Konfigurator-Aufrufer, der das verletzt, sofort auffaellt statt still einen
        // Subtree zu ueberschreiben (adversariale Verifikation w3346v581; heute kein Verhaltenswechsel).
        assert((it == x.disc.end() || *it != disc) && "add_child: disc bereits vorhanden — Praekondition verletzt");
        std::size_t const pos = static_cast<std::size_t>(it - x.disc.begin());
        x.disc.insert(x.disc.begin() + static_cast<std::ptrdiff_t>(pos), disc);
        x.kids.insert(x.kids.begin() + static_cast<std::ptrdiff_t>(pos), child);
    }
    void set_child(std::size_t r, std::uint32_t disc, std::size_t child) noexcept {
        Inner& x  = inners_[ref_idx(r)];
        auto   it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        if (it != x.disc.end() && *it == disc) x.kids[static_cast<std::size_t>(it - x.disc.begin())] = child;
    }
    void remove_child(std::size_t r, std::uint32_t disc) noexcept {
        Inner& x  = inners_[ref_idx(r)];
        auto   it = std::lower_bound(x.disc.begin(), x.disc.end(), disc);
        if (it != x.disc.end() && *it == disc) {
            std::size_t const pos = static_cast<std::size_t>(it - x.disc.begin());
            x.disc.erase(x.disc.begin() + static_cast<std::ptrdiff_t>(pos));
            x.kids.erase(x.kids.begin() + static_cast<std::ptrdiff_t>(pos));
        }
    }

    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — free_.push_back kann beim Free-List-Wachstum
    // allozieren/werfen ([[allocation-failure-exception]]: werfen statt terminate; Concept verlangt kein noexcept).
    // KAUSALITAET (Posten 64/70): der Wurf kommt seit dem A8-S5-Schnitt vom StdAllocatorAdapter der
    // Allokator-ACHSE (Strategie meldet OOM per nullptr -> Adapter wirft std::bad_alloc), nicht vom Default.
    void free_node(std::size_t r) {
        if (ref_kind(r) == kLeaf) {
            fl_leaf_.push_back(ref_idx(r));
        } else {
            fl_inner_.push_back(ref_idx(r));
        }
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder) -- sie
    /// zaehlt jetzt auch die disc/kids-Listen IM Inner-Knoten, weil beide Ebenen ueber dieselbe Strategie
    /// laufen. Die frueheren Store-eigenen Capacity-Delta-Schaetzer waren allocator-UNABHAENGIG und haetten
    /// neben der echten Achsen-Statistik eine zweite, driftende Wahrheit gefuehrt; live_nodes speist der
    /// ABI-Adapter im Rich-Zweig aus occupied_count() des Organs.
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using Leaf  = detail::StartTrieLeaf;
    using Inner = inner_type;

    /// Frischer Inner-Knoten mit leeren, an das EIGENE allocator_ gebundenen Kind-Listen.
    /// (Ein `Inner{}` gaebe es nicht: der Achsen-Adapter ist bestimmungsgemaess nicht default-konstruierbar.)
    [[nodiscard]] Inner make_inner_() {
        return Inner{prefix_type{}, 1u,
                     typename Inner::disc_vector_type(allocator_.template as_std_allocator<std::uint32_t>()),
                     typename Inner::kids_vector_type(allocator_.template as_std_allocator<std::size_t>())};
    }

    /// Uebernimmt die Inner-Knoten einer Quelle und bindet disc/kids neu an das EIGENE allocator_.
    void copy_inners_from_(std::vector<Inner, inner_allocator_type> const& src) {
        inners_.reserve(src.size());
        for (auto const& x : src) {
            inners_.push_back(
                Inner{x.prefix, x.span,
                      typename Inner::disc_vector_type(x.disc.begin(), x.disc.end(),
                                                       allocator_.template as_std_allocator<std::uint32_t>()),
                      typename Inner::kids_vector_type(x.kids.begin(), x.kids.end(),
                                                       allocator_.template as_std_allocator<std::size_t>())});
        }
    }

    // allocator_ VOR den Vektoren (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    Alloc                                          allocator_{};
    std::vector<Leaf, leaf_allocator_type>         leaves_;
    std::vector<Inner, inner_allocator_type>       inners_;
    std::vector<std::size_t, index_allocator_type> fl_leaf_;
    std::vector<std::size_t, index_allocator_type> fl_inner_;
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das StartTrieNodePool-Concept.
static_assert(StartTrieNodePool<StartTrieNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
