#pragma once
// V41.F.6.1.R3 — SearchAlgorithmAnatomy<Composition> Skelett
//
// Saeugetier-Anatomie: EINE generische Such-Algorithmus-Klasse, die durch
// Template-Spezialisierung mit jeder Composition zu einem konkreten Algorithmus
// wird. Phase R3 ist Pilot-Skelett mit std::map als Container; echte
// Composition-driven Implementation kommt in R4+R5.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §11.3+§12+§14
// @task V41.F.6.1.R3

#include "composition_concept.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// SearchAlgorithmAnatomy — zentrale Anatomie-Klasse fuer ALLE Suchalgorithmen.
///
/// Template-Parameter Composition liefert die 17 Achsen-Auspraegungen. Konkrete
/// Algorithmen (ART/HOT/Wormhole/SuRF/Masstree/START) sind reine Template-
/// Instantiationen — siehe `anatomy::Art`, `anatomy::Hot` etc. unten.
///
/// Phase R3 (Pilot): interner std::map<uint64_t,uint64_t> als Container.
/// Phase R4+: Container wird durch Composition::node_type + Composition::allocator
/// + Composition::concurrency-getriebene Implementation ersetzt.
template <IsComposition Composition>
class SearchAlgorithmAnatomy {
public:
    using composition_t = Composition;
    using key_type      = std::uint64_t;
    using value_type    = std::uint64_t;

    // Composition-Inspection (statisch — Pflicht fuer Mess-Treiber)
    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id() noexcept { return Composition::paper_id; }
    static constexpr std::size_t organ_count() noexcept { return composition_organ_count<Composition>::value; }

    /// Insert (Pilot Pipeline — kommt in R4 echte Multi-Organ-Sequenz)
    bool insert(key_type k, value_type v) {
        auto [it, inserted] = container_.insert_or_assign(k, v);
        return inserted;
    }

    /// Lookup (Pilot Pipeline)
    std::optional<value_type> lookup(key_type k) const {
        auto it = container_.find(k);
        if (it == container_.end()) return std::nullopt;
        return it->second;
    }

    /// Erase (Pilot Pipeline)
    bool erase(key_type k) {
        return container_.erase(k) > 0;
    }

    /// Clear
    void clear() noexcept { container_.clear(); }

    /// Aktuelle Element-Anzahl
    std::size_t size() const noexcept { return container_.size(); }

    /// Composition-Empty
    bool empty() const noexcept { return container_.empty(); }

private:
    // Pilot R3 Container — wird in R4 durch Composition::node_type/allocator/concurrency ersetzt
    std::map<key_type, value_type> container_;
};

}  // namespace comdare::cache_engine::anatomy
