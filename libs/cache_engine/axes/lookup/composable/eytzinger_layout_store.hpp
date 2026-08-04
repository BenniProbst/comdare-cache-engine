#pragma once
// #188-4a (2026-07-02) -- EytzingerLayoutStore: sortierte Basis + lazy BFS-Layout in EINEM Store.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Option (b) (User-Entscheid 2026-07-02): sortierte Basis = Quelle der Wahrheit; BFS-Layout wird lazy im ersten
// lookup nach Mutation neu gebaut (O(n) assign + rekursives Fill). Diese Rebuild-Kosten sind eine DOKUMENTIERTE
// Mess-Eigenschaft des Eytzinger-Tiers (erster Lookup nach Mutations-Burst traegt den Rebuild), NICHT
// wegzuoptimieren. Kein thread-safe const-lookup (mutable-Rebuild; is_thread_safe()==false wie der Wrapper).
//
// Eine dirty-Kopie rebuildert ihren eigenen BFS-Puffer beim naechsten Lookup; die Vektoren sind unabhaengig
// und damit Memento-/CoW-tauglich fuer den Adapter-Pfad.
//
// A8-S5 Familie 01a (2026-08-04): alle vier Vektoren (sortierte Basis + BFS-Puffer) beziehen ihren Speicher
// REAL aus der Allocator-Achse (axis_06), Muster BTreeNodePoolStore. Dieser Store hatte wie Masstree gar keine
// Allokator-Naht und hat deshalb einen Template-Kopf BEKOMMEN; Eytzinger ist zwar organ_for-Familie, aber der
// Store ist KEIN Registry-Organ (er wird nur im Organ-Alias genannt) -- die Typ-Namens-Kante entfaellt, die
// Nenn-Stellen schreiben jetzt `EytzingerLayoutStore<>`.
// ALLOKATIONS-VERTRAG: dynamisches Wachstum [[allocation-failure-exception]] -- insert_slot_at/append_slot und
// rebuild_if_dirty koennen std::bad_alloc werfen; der Wurf kommt seit dem Schnitt vom StdAllocatorAdapter der
// ACHSE (Strategie meldet OOM per nullptr, Adapter uebersetzt an EINER Stelle, Posten 64), nicht vom Default.
// COW-Kopie rebindet an das eigene allocator_ + verwirft die Kopier-Pollution per restore_statistics-Memento;
// Move ist nicht deklariert -> degradiert sicher zu Copy (der Adapter haelt &allocator_).

#include "eytzinger_layout_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)

#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Eytzinger-Layout-Substrat: sortierter Primaerzustand plus abgeleiteter 1-indexed BFS-Puffer.
template <class Alloc = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class EytzingerLayoutStore {
public:
    using key_type            = std::uint64_t;
    using value_type          = std::uint64_t;
    using allocator_type      = Alloc;
    using key_allocator_type  = typename Alloc::template StdAllocatorAdapter<key_type>;
    using slot_allocator_type = typename Alloc::template StdAllocatorAdapter<value_type>;

    // Default: die vier Vektoren an das eigene allocator_ binden (Adapter nicht default-konstruierbar).
    EytzingerLayoutStore()
        : keys_(allocator_.template as_std_allocator<key_type>()),
          values_(allocator_.template as_std_allocator<value_type>()),
          eyt_keys_(allocator_.template as_std_allocator<key_type>()),
          eyt_vals_(allocator_.template as_std_allocator<value_type>()) {}
    // COW-Pflicht (Memento): allocator_ mitkopieren, alle vier Vektoren an DAS EIGENE allocator_ rebinden,
    // dann die Kopier-Pollution per restore_statistics verwerfen. Move NICHT deklariert -> degradiert zu Copy.
    EytzingerLayoutStore(EytzingerLayoutStore const& o)
        : allocator_(o.allocator_), keys_(o.keys_, allocator_.template as_std_allocator<key_type>()),
          values_(o.values_, allocator_.template as_std_allocator<value_type>()),
          eyt_keys_(o.eyt_keys_, allocator_.template as_std_allocator<key_type>()),
          eyt_vals_(o.eyt_vals_, allocator_.template as_std_allocator<value_type>()), dirty_(o.dirty_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    EytzingerLayoutStore& operator=(EytzingerLayoutStore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter setzt keine propagate_-Typedefs) -> die Vektoren behalten ihr an
            // this-allocator_ gebundenes Adapter; die Assigns re-allozieren transient ueber this-allocator_.
            keys_     = o.keys_;
            values_   = o.values_;
            eyt_keys_ = o.eyt_keys_;
            eyt_vals_ = o.eyt_vals_;
            dirty_    = o.dirty_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~EytzingerLayoutStore() = default;

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder). Vor dem
    /// Schnitt hatte dieser Store ueberhaupt keine Allokator-Sicht -- der Rebuild-Puffer, die dokumentierte
    /// Mess-Eigenschaft des Eytzinger-Tiers, lief vollstaendig an T6 vorbei.
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

    [[nodiscard]] std::size_t slot_count() const noexcept { return keys_.size(); }
    [[nodiscard]] key_type    key_at(std::size_t i) const noexcept { return keys_[i]; }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return values_[i]; }

    void set_value_at(std::size_t i, value_type v) noexcept {
        values_[i] = v;
        dirty_     = true;
    }
    void insert_slot_at(std::size_t i, key_type k, value_type v) {
        keys_.insert(keys_.begin() + static_cast<std::ptrdiff_t>(i), k);
        values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(i), v);
        dirty_ = true;
    }
    void erase_slot_at(std::size_t i) {
        keys_.erase(keys_.begin() + static_cast<std::ptrdiff_t>(i));
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(i));
        dirty_ = true;
    }
    void append_slot(key_type k, value_type v) {
        keys_.push_back(k);
        values_.push_back(v);
        dirty_ = true;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
        eyt_keys_.clear();
        eyt_vals_.clear();
        dirty_ = true;
    }

    /// Baut bei Bedarf das 1-indizierte Eytzinger-Layout aus der sortierten Quelle.
    void rebuild_if_dirty() const {
        if (!dirty_) return;
        std::size_t const n = keys_.size();
        eyt_keys_.assign(n + 1u, key_type{});
        eyt_vals_.assign(n + 1u, value_type{});
        std::size_t pos = 0;
        fill_eytzinger(1u, n, pos);
        dirty_ = false;
    }

    [[nodiscard]] key_type   eyt_key_at(std::size_t i) const noexcept { return eyt_keys_[i]; }
    [[nodiscard]] value_type eyt_value_at(std::size_t i) const noexcept { return eyt_vals_[i]; }
    [[nodiscard]] bool       dirty() const noexcept { return dirty_; }

private:
    void fill_eytzinger(std::size_t k, std::size_t n, std::size_t& pos) const {
        if (k <= n) {
            fill_eytzinger(2u * k, n, pos);
            eyt_keys_[k] = keys_[pos];
            eyt_vals_[k] = values_[pos];
            ++pos;
            fill_eytzinger(2u * k + 1u, n, pos);
        }
    }

    // allocator_ VOR den Vektoren (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    // mutable, weil der lazy Rebuild in rebuild_if_dirty() const ist und REAL ueber die Achse alloziert.
    mutable Alloc                                        allocator_{};
    std::vector<key_type, key_allocator_type>            keys_;
    std::vector<value_type, slot_allocator_type>         values_;
    mutable std::vector<key_type, key_allocator_type>    eyt_keys_;
    mutable std::vector<value_type, slot_allocator_type> eyt_vals_;
    mutable bool                                         dirty_ = false;
};

static_assert(EytzingerLayoutPool<EytzingerLayoutStore<>>);

} // namespace comdare::cache_engine::lookup::composable
