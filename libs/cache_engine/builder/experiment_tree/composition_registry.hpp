#pragma once
// BR-2 (2026-06-02, Doc 27 §3) — CompositionRegistry: Baum-Blatt-Pfad ↔ reale AdHocComposition<17>.
//
// Bindet den Experiment-Baum an die ECHTEN Kompositionen: per PermutationEngine::for_each_permutation wird
// JEDE Permutation eines PILOT-Engines (klein gehalten — das VOLLE Enabled-Produkt ist C1060-infeasible, Doc 27 §6)
// zu einer realen AdHocComposition<17> materialisiert (CompositionFromPermTuple<P>) und unter ihrem serialisierten
// Pfad (== Baum-`binary_id`, axis_path_serialization.hpp = DIE zentrale Konvention) abgelegt. Der Baum-Blatt-Pfad
// schlägt so genau EINE reale Komposition nach.
//
// Round-Trip-Garant (Doc 27 §3): serialize_composition_path<P>() (aus dem PermTuple) ==
// serialize_composition_from_slots<CompositionFromPermTuple<P>>() (aus den 17 named Slots) == Baum-`binary_id`.
//
// Materialisierungs-Grenze: nur die im (Flag-/Pilot-reduzierten) Engine enthaltenen Permutationen werden
// compile-time materialisiert — NIE der volle 22-Achsen-Typ-Raum. C++23, header-only.

#include "axis_path_serialization.hpp"
#include "anatomy/composition_factory.hpp" // anatomy::AdHocComposition, anatomy::CompositionFromPermTuple

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Die read-only Achsen-Definition einer Komposition: (Achsen-Name, Wrapper-Name) je der 17 Slots T0..T16
/// (15 Such-Achsen + queuing q1/q2; Doc 30 §8.0 i.V.m. Bau-INC-2c: telemetry / Bau-INC-2d: isa sind System-Achsen). Quelle für BR-3 „Achsen-Definition read-only je Knoten".
template <class C>
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> composition_definition() {
    std::array<std::string_view, 17> const v = {
        C::search_algo::name(), C::cache_traversal::name(),  C::mapping::name(),      C::path_compression::name(),
        C::node_type::name(),   C::memory_layout::name(),    C::allocator::name(),    C::prefetch::name(),
        C::concurrency::name(), C::serialization::name(),    C::value_handle::name(), C::index_organization::name(),
        C::io_dispatch::name(), C::migration_policy::name(), C::filter::name(),       C::queuing_q1::name(),
        C::queuing_q2::name()};
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(v.size());
    for (std::size_t i = 0; i < v.size(); ++i)
        out.emplace_back(std::string{kCompositionAxisNames[i]}, std::string{v[i]});
    return out;
}

/// Ein Registry-Eintrag: EIN Baum-Blatt-Pfad → die reale, materialisierte Komposition (+ ihre Definition).
struct CompositionRecord {
    std::string                                      path;      // serialize_composition_path<P>() == binary_id
    std::string                                      slot_path; // serialize_composition_from_slots<Comp>() (== path)
    bool                                             materialized = false; // CompositionFromPermTuple<P> kompilierte
    std::vector<std::pair<std::string, std::string>> definition;           // (achse, wrapper) je Slot (read-only, BR-3)
};

/// CompositionRegistry — keyed über den serialisierten Static-Pfad (= Baum-`binary_id`).
class CompositionRegistry {
public:
    /// Befüllt aus einem PILOT-PermutationEngine: jede Permutation → reale AdHocComposition<17>.
    /// (Der Engine wird durch Flags/Pilot klein gehalten — sonst C1060; Doc 27 §5 R1 + §6.)
    template <class PilotEngine>
    void register_from_engine() {
        PilotEngine::for_each_permutation([this]<class P>() {
            using Comp = anatomy::CompositionFromPermTuple<P>; // materialisiert AdHocComposition<17> (compile-time)
            CompositionRecord r;
            r.path         = serialize_composition_path<P>();
            r.slot_path    = serialize_composition_from_slots<Comp>();
            r.materialized = true;
            r.definition   = composition_definition<Comp>();
            by_path_.insert_or_assign(r.path, std::move(r));
        });
    }

    [[nodiscard]] CompositionRecord const* lookup(std::string const& path) const {
        auto it = by_path_.find(path);
        return it == by_path_.end() ? nullptr : &it->second;
    }
    [[nodiscard]] bool        contains(std::string const& path) const { return by_path_.contains(path); }
    [[nodiscard]] std::size_t size() const noexcept { return by_path_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return by_path_.empty(); }

    template <class F>
    void for_each(F&& f) const {
        for (auto const& [k, v] : by_path_) f(v);
    }

private:
    std::map<std::string, CompositionRecord> by_path_;
};

} // namespace comdare::cache_engine::builder::experiment
