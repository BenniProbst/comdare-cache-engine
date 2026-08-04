#pragma once
// V41 Umstufung-A s4 (Task #43) — HotPatriciaNodePoolStore: binary crit-bit-Substrat (erfuellt HotPatriciaNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — die HOT-Korrektheits-Basis (Binna et al. SIGMOD 2018), is_original=false
// ([[pseudocode-papers-fallback]]): Leaf (Key+Value inline) + Internal{crit_bit (0..63), child[2]}. Nur 2
// Kind-Typen (kein adaptives Growth wie ART — Single-Bit-Split erzeugt stets binaere Internals). NodeRef =
// std::size_t (Kind in Bits 56-63, Index in Bits 0-55), je Kind ein Vektor + Free-List (Index-Stabilitaet via
// Recycling). Der crit-bit-Descent + Leaf-Split + Erase-Collapse lebt im HotPatriciaTraversalOrgan, NICHT hier.
//
// A8-S5 Familie 01a (2026-08-04): Leaf-, Internal- und Free-List-Speicher kommen REAL aus der Allocator-Achse
// (axis_06), Muster BTreeNodePoolStore. Der Allokator-Template-Kopf bestand schon; geflippt wurde die BINDUNG
// (Default + Rebind ueber den StdAllocatorAdapter der Achse statt ueber std::allocator). COW-Kopie rebindet
// alle vier Vektoren an das eigene allocator_ und verwirft die Kopier-Pollution per restore_statistics-Memento;
// Move ist nicht deklariert -> degradiert sicher zu Copy.

#include "hot_patricia_node_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

struct HotPatriciaLeaf {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    key_type   key{};
    value_type val{};
};

struct HotPatriciaInternal {
    std::uint8_t               crit_bit{};
    std::array<std::size_t, 2> child{};
}; // crit_bit 0..63 passt in uint8

} // namespace detail

template <class Alloc = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class HotPatriciaNodePoolStore {
public:
    using node_type                   = detail::HotPatriciaLeaf;
    using key_type                    = typename node_type::key_type;
    using value_type                  = typename node_type::value_type;
    using allocator_type              = Alloc;
    using leaf_allocator_type         = typename Alloc::template StdAllocatorAdapter<detail::HotPatriciaLeaf>;
    using internal_allocator_type     = typename Alloc::template StdAllocatorAdapter<detail::HotPatriciaInternal>;
    using index_allocator_type        = typename Alloc::template StdAllocatorAdapter<std::size_t>;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    // Default: die vier Vektoren an das eigene allocator_ binden (Adapter nicht default-konstruierbar).
    HotPatriciaNodePoolStore()
        : leaves_(allocator_.template as_std_allocator<detail::HotPatriciaLeaf>()),
          internals_(allocator_.template as_std_allocator<detail::HotPatriciaInternal>()),
          fl_leaf_(allocator_.template as_std_allocator<std::size_t>()),
          fl_internal_(allocator_.template as_std_allocator<std::size_t>()) {}
    // COW-Pflicht (Memento): allocator_ mitkopieren, Vektoren an DAS EIGENE allocator_ rebinden, dann die
    // Kopier-Pollution per restore_statistics verwerfen. Move NICHT deklariert -> degradiert zu Copy.
    HotPatriciaNodePoolStore(HotPatriciaNodePoolStore const& o)
        : allocator_(o.allocator_), leaves_(o.leaves_, allocator_.template as_std_allocator<detail::HotPatriciaLeaf>()),
          internals_(o.internals_, allocator_.template as_std_allocator<detail::HotPatriciaInternal>()),
          fl_leaf_(o.fl_leaf_, allocator_.template as_std_allocator<std::size_t>()),
          fl_internal_(o.fl_internal_, allocator_.template as_std_allocator<std::size_t>()), root_(o.root_),
          size_(o.size_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    HotPatriciaNodePoolStore& operator=(HotPatriciaNodePoolStore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter setzt keine propagate_-Typedefs) -> die Vektoren behalten ihr an
            // this-allocator_ gebundenes Adapter; die Assigns re-allozieren transient ueber this-allocator_.
            leaves_      = o.leaves_;
            internals_   = o.internals_;
            fl_leaf_     = o.fl_leaf_;
            fl_internal_ = o.fl_internal_;
            root_        = o.root_;
            size_        = o.size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~HotPatriciaNodePoolStore() = default;

    enum Kind : std::uint8_t { kLeaf = 0, kInternal = 1 };
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
        internals_.clear();
        fl_leaf_.clear();
        fl_internal_.clear();
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

    // ── Internal (crit-bit) ──
    [[nodiscard]] unsigned    crit_bit(std::size_t r) const noexcept { return internals_[ref_idx(r)].crit_bit; }
    [[nodiscard]] std::size_t child(std::size_t r, unsigned bit) const noexcept {
        return internals_[ref_idx(r)].child[bit & 1U];
    }
    void set_child(std::size_t r, unsigned bit, std::size_t c) noexcept { internals_[ref_idx(r)].child[bit & 1U] = c; }
    [[nodiscard]] std::size_t new_internal(unsigned crit_bit, std::size_t c0, std::size_t c1) {
        std::size_t idx;
        if (!fl_internal_.empty()) {
            idx = fl_internal_.back();
            fl_internal_.pop_back();
            internals_[idx] = detail::HotPatriciaInternal{};
        } else {
            idx = internals_.size();
            internals_.push_back(detail::HotPatriciaInternal{});
        }
        detail::HotPatriciaInternal& x = internals_[idx];
        x.crit_bit                     = static_cast<std::uint8_t>(crit_bit);
        x.child[0]                     = c0;
        x.child[1]                     = c1;
        return make_ref(kInternal, idx);
    }
    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — free_.push_back kann beim Free-List-Wachstum
    // allozieren/werfen ([[allocation-failure-exception]]: werfen statt terminate; Concept verlangt kein noexcept).
    // KAUSALITAET (Posten 64/70): der Wurf kommt seit dem A8-S5-Schnitt vom StdAllocatorAdapter der
    // Allokator-ACHSE (Strategie meldet OOM per nullptr -> Adapter wirft std::bad_alloc), nicht vom Default.
    void free_node(std::size_t r) {
        if (ref_kind(r) == kLeaf) {
            fl_leaf_.push_back(ref_idx(r));
        } else {
            fl_internal_.push_back(ref_idx(r));
        }
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder) statt der
    /// frueheren Store-eigenen Capacity-Delta-Schaetzung (allocator-UNABHAENGIG -> zweite, driftende Wahrheit).
    /// live_nodes speist der ABI-Adapter im Rich-Zweig aus occupied_count() des Organs.
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    using Leaf     = detail::HotPatriciaLeaf;
    using Internal = detail::HotPatriciaInternal;

    // allocator_ VOR den Vektoren (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    Alloc                                          allocator_{};
    std::vector<Leaf, leaf_allocator_type>         leaves_;
    std::vector<Internal, internal_allocator_type> internals_;
    std::vector<std::size_t, index_allocator_type> fl_leaf_;
    std::vector<std::size_t, index_allocator_type> fl_internal_;
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das HotPatriciaNodePool-Concept.
static_assert(HotPatriciaNodePool<HotPatriciaNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
