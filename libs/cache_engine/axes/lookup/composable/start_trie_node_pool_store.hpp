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

#include "start_trie_node_pool_concept.hpp"
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

template <class FwdAlloc = std::allocator<std::size_t>>
struct StartTrieInner {
    using disc_allocator_type = typename std::allocator_traits<FwdAlloc>::template rebind_alloc<std::uint32_t>;
    using kids_allocator_type = typename std::allocator_traits<FwdAlloc>::template rebind_alloc<std::size_t>;
    using disc_vector_type    = std::vector<std::uint32_t, disc_allocator_type>;
    using kids_vector_type    = std::vector<std::size_t, kids_allocator_type>;

    StartTriePrefix  prefix{};
    std::uint8_t     span = 1;
    disc_vector_type disc{}; // aufsteigend sortierte Diskriminatoren (span-breit)
    kids_vector_type kids{}; // parallel: Kind-Refs
};

} // namespace detail

template <class A = std::allocator<detail::StartTrieLeaf>>
class StartTrieNodePoolStore {
public:
    using node_type                   = detail::StartTrieLeaf;
    using key_type                    = typename node_type::key_type;
    using value_type                  = typename node_type::value_type;
    using prefix_type                 = detail::StartTriePrefix;
    using allocator_type              = A;
    using leaf_allocator_type         = typename std::allocator_traits<A>::template rebind_alloc<detail::StartTrieLeaf>;
    using forward_allocator_type      = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;
    using inner_type                  = detail::StartTrieInner<forward_allocator_type>;
    using inner_allocator_type        = typename std::allocator_traits<A>::template rebind_alloc<inner_type>;
    using index_allocator_type        = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;
    static constexpr std::size_t kNil = detail::kStartTrieNil;

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
            idx                            = leaves_.size();
            std::size_t const old_capacity = leaves_.capacity();
            leaves_.push_back(node_type{k, v});
            record_capacity_growth_(old_capacity, leaves_.capacity(), sizeof(node_type));
        }
        return make_ref(kLeaf, idx);
    }

    // ── Inner (Multibyte-Span) ──
    [[nodiscard]] std::size_t new_inner(unsigned sp) {
        std::size_t idx;
        std::size_t old_disc_capacity = 0;
        std::size_t old_kids_capacity = 0;
        if (!fl_inner_.empty()) {
            idx = fl_inner_.back();
            fl_inner_.pop_back();
            old_disc_capacity = inners_[idx].disc.capacity();
            old_kids_capacity = inners_[idx].kids.capacity();
            inners_[idx]      = Inner{};
        } else {
            idx                            = inners_.size();
            std::size_t const old_capacity = inners_.capacity();
            inners_.push_back(Inner{});
            record_capacity_growth_(old_capacity, inners_.capacity(), sizeof(Inner));
        }
        inners_[idx].span = static_cast<std::uint8_t>(sp);
        record_inner_vector_capacity_growth_(old_disc_capacity, old_kids_capacity, inners_[idx]);
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
        std::size_t const pos               = static_cast<std::size_t>(it - x.disc.begin());
        std::size_t const old_disc_capacity = x.disc.capacity();
        std::size_t const old_kids_capacity = x.kids.capacity();
        x.disc.insert(x.disc.begin() + static_cast<std::ptrdiff_t>(pos), disc);
        record_capacity_growth_(old_disc_capacity, x.disc.capacity(),
                                sizeof(typename Inner::disc_vector_type::value_type));
        x.kids.insert(x.kids.begin() + static_cast<std::ptrdiff_t>(pos), child);
        record_capacity_growth_(old_kids_capacity, x.kids.capacity(),
                                sizeof(typename Inner::kids_vector_type::value_type));
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

    void free_node(std::size_t r) noexcept {
        if (ref_kind(r) == kLeaf) {
            std::size_t const old_capacity = fl_leaf_.capacity();
            fl_leaf_.push_back(ref_idx(r));
            record_capacity_growth_(old_capacity, fl_leaf_.capacity(), sizeof(std::size_t));
        } else {
            std::size_t const old_capacity = fl_inner_.capacity();
            fl_inner_.push_back(ref_idx(r));
            record_capacity_growth_(old_capacity, fl_inner_.capacity(), sizeof(std::size_t));
        }
    }

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
            static_cast<std::uint64_t>((leaves_.size() + inners_.size()) - (fl_leaf_.size() + fl_inner_.size())),
        };
    }
#endif

private:
    using Leaf  = detail::StartTrieLeaf;
    using Inner = inner_type;

    void record_inner_vector_capacity_growth_(std::size_t old_disc_capacity, std::size_t old_kids_capacity,
                                              Inner const& x) noexcept {
        record_capacity_growth_(old_disc_capacity, x.disc.capacity(),
                                sizeof(typename Inner::disc_vector_type::value_type));
        record_capacity_growth_(old_kids_capacity, x.kids.capacity(),
                                sizeof(typename Inner::kids_vector_type::value_type));
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Ehrliche Allokator-Metrik: gezaehlt werden nur erfolgreiche vector-capacity-Zuwaechse, als Capacity-Delta
    // mal Elementgroesse. Reuse/clear ohne Capacity-Wachstum erzeugt bewusst keine kuenstlichen Werte.
    void record_capacity_growth_(std::size_t old_capacity, std::size_t new_capacity, std::size_t elem_bytes) noexcept {
        if (new_capacity <= old_capacity) return;
        ++alloc_calls_;
        bytes_allocated_ +=
            static_cast<std::uint64_t>(new_capacity - old_capacity) * static_cast<std::uint64_t>(elem_bytes);
    }
#else
    static void record_capacity_growth_(std::size_t, std::size_t, std::size_t) noexcept {}
#endif

    std::vector<Leaf, leaf_allocator_type>         leaves_{};
    std::vector<Inner, inner_allocator_type>       inners_{};
    std::vector<std::size_t, index_allocator_type> fl_leaf_{};
    std::vector<std::size_t, index_allocator_type> fl_inner_{};
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das StartTrieNodePool-Concept.
static_assert(StartTrieNodePool<StartTrieNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
