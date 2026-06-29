#pragma once
// D9 / L-76a (2026-06-02) — SetAnatomy: die SET-GATTUNG (Vogel, genus()==Set, K-only). Hält das ECHTE
// search_algo-Organ der Komposition (Composition::search_algo) und treibt es als MENGE (K=V): insert(k)=
// insert(k,k), contains(k)=lookup(k).has_value(), erase(k). Liefert observe_all → SetObserverSnapshot.
//
// Leichtgewichtig: nutzt NUR Composition::search_algo direkt (kein ComposedStore/Umbrella) — analog zur Art,
// wie SearchAlgorithmAbiAdapter `SearchAlgo search_organ_` hält. Die 15 Set-Achsen sind die Komposition-Identität;
// real getrieben ist das search_algo-Kern-Organ (R5.B-Grenze ehrlich, wie SearchAlgorithm). Doku 14 §27.2/§28/§32.

#include "anatomy_base.hpp"    // AnatomyGenus
#include "set_composition.hpp" // SetComposition / IsSetComposition

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// In-process Set-Observer (reich; der ABI-POD SetObserverSnapshotV1 ist die cross-boundary-Spiegelung).
struct SetObserverSnapshot {
    std::uint64_t insert_count        = 0;
    std::uint64_t contains_count      = 0;
    std::uint64_t contains_hit_count  = 0;
    std::uint64_t contains_miss_count = 0;
    std::uint64_t erase_count         = 0;
    std::uint64_t current_size        = 0;
    std::uint64_t peak_size           = 0;
};

/// SetAnatomy<Composition> — genus()==Set; K-only-Mengen-API über das echte search_algo-Organ (K=V).
template <class Composition>
class SetAnatomy {
public:
    using composition_t = Composition;
    using set_organ_t   = typename Composition::search_algo;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::Set; }             // Vogel
    static constexpr std::size_t      organ_count() noexcept { return Composition::slot_count; } // 15

    // ── Set-Gattungs-API (K-only) — treibt das ECHTE search_algo-Organ als Menge (K=V) ──
    bool insert(std::uint64_t key) {
        auto const before = static_cast<std::uint64_t>(organ_.occupied_count());
        organ_.insert(static_cast<key_t>(key), static_cast<value_t>(key)); // K=V
        auto const after  = static_cast<std::uint64_t>(organ_.occupied_count());
        bool const is_new = after > before;
        if (is_new) ++obs_.insert_count;
        obs_.current_size = after;
        if (after > obs_.peak_size) obs_.peak_size = after;
        return is_new;
    }
    [[nodiscard]] bool contains(std::uint64_t key) const {
        ++obs_.contains_count;
        bool const hit = organ_.lookup(static_cast<key_t>(key)).has_value();
        if (hit)
            ++obs_.contains_hit_count;
        else
            ++obs_.contains_miss_count;
        return hit;
    }
    bool erase(std::uint64_t key) {
        auto const before = static_cast<std::uint64_t>(organ_.occupied_count());
        organ_.erase(static_cast<key_t>(key));
        auto const after   = static_cast<std::uint64_t>(organ_.occupied_count());
        bool const removed = after < before;
        if (removed) ++obs_.erase_count;
        obs_.current_size = after;
        return removed;
    }
    [[nodiscard]] std::size_t size() const noexcept { return static_cast<std::size_t>(organ_.occupied_count()); }
    void                      clear() {
        organ_.clear();
        obs_.current_size = 0;
    }

    [[nodiscard]] SetObserverSnapshot observe_all() const noexcept { return obs_; }

private:
    using key_t   = typename set_organ_t::key_type;
    using value_t = typename set_organ_t::key_type; // Set: K=V (Value-Typ == Key-Typ)
    set_organ_t                 organ_{};
    mutable SetObserverSnapshot obs_{}; // mutable: contains() ist const, zählt aber Abfragen
};

} // namespace comdare::cache_engine::anatomy
