#pragma once
// BR-2-Fundament (2026-06-02, Doc 27 §3+§5-R3) — DIE EINE zentrale Pfad-Serialisierungs-Konvention.
//
// Doc 27 §5 R3: „Pfad-Serialisierung muss BR-1↔BR-2↔BR-4 identisch sein → EINE zentrale serialize_path-Konvention."
// Dieser Header IST diese Konvention. Sowohl BR-1 (registry_to_axis_levels: Baum-AxisLevel-Namen) als auch BR-2
// (composition_registry: PermTuple→Pfad) als auch BR-4 (perm_<id>.cpp-Benennung) leiten ihre Pfad-Strings hierüber
// ab — so ist der Baum-Blatt-`binary_id` GARANTIERT gleich dem CompositionRegistry-Key (Round-Trip-Garant).
//
// Format (identisch zum Baum: experiment_tree.hpp StaticAxisNode::serialize() = "axis=value", "/"-join):
//   "search_algo=<W0::name()>/cache_traversal=<W1::name()>/.../queuing_q2=<W18::name()>"
// C++23, header-only, KEINE Achsen-Includes (umbrella-unabhängig; nimmt das PermTuple als Template-Param entgegen).

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

/// Die 19 Komposition-Achsen-Namen in AdHocComposition-Slot-Reihenfolge T0..T18 (composition_factory.hpp:41-68;
/// Doc 30 §8.0 erweitert um queuing_q1/queuing_q2). TEILMENGE der Achsen aus registry_to_axis_levels.hpp
/// build_all_axis_levels() (BR-1, zentrale Quelle) — aber NICHT deren Präfix: dort stehen zwischen T16 und q1/q2
/// die 3 build-only-Achsen (page_type/simd_extension/general_hardware), dahinter die 4 node-shape-Achsen (#234-K).
/// (Kommentar-Korrektur #234-K: die frühere "IDENTISCH zu den ersten 19"-Behauptung war schon im 22er-Stand falsch.)
inline constexpr std::array<std::string_view, 19> kCompositionAxisNames = {
    "search_algo",      "cache_traversal", "mapping",    "path_compression",   "node_type",
    "memory_layout",    "allocator",       "prefetch",   "concurrency",        "serialization",
    "telemetry",        "value_handle",    "isa",        "index_organization", "io_dispatch",
    "migration_policy", "filter",          "queuing_q1", "queuing_q2"};

/// Ein Achsen-Pfad-Segment: "axis=value" (= experiment_tree.hpp StaticAxisNode::serialize()).
[[nodiscard]] inline std::string serialize_axis_segment(std::string_view axis, std::string_view value) {
    std::string s{axis};
    s += '=';
    s += value;
    return s;
}

/// serialize_composition_path<P>() — der serialisierte Static-Pfad EINER 19-Achsen-Permutation (PermTuple<V0..V18>).
/// Baut "search_algo=<V0::name()>/.../queuing_q2=<V18::name()>" — exakt der `binary_id`, den der Experiment-Baum für
/// dasselbe Tupel erzeugt (gleiche Achsen-Namen, gleiche W::name()-Werte, gleiche Reihenfolge, gleiches Format).
template <class P>
[[nodiscard]] inline std::string serialize_composition_path() {
    std::string out;
    std::size_t i = 0;
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, typename P::variants>>([&](auto id) {
        using V = typename decltype(id)::type;
        if (i < kCompositionAxisNames.size()) {
            if (!out.empty()) out += '/';
            out += serialize_axis_segment(kCompositionAxisNames[i], V::name());
        }
        ++i;
    });
    return out;
}

/// serialize_composition_from_slots<C>() — derselbe Pfad, aber aus den 19 NAMED using-Slots einer
/// AdHocComposition<…> (C::search_algo::name() … C::filter::name()). Round-Trip-Beleg (BR-2): wenn
/// serialize_composition_from_slots<CompositionFromPermTuple<P>>() == serialize_composition_path<P>(),
/// hat CompositionFromPermTuple die Slot-Reihenfolge T0..T18 KORREKT erhalten (P → Composition verlustfrei).
template <class C>
[[nodiscard]] inline std::string serialize_composition_from_slots() {
    std::array<std::string_view, 19> const v = {
        C::search_algo::name(), C::cache_traversal::name(),    C::mapping::name(),     C::path_compression::name(),
        C::node_type::name(),   C::memory_layout::name(),      C::allocator::name(),   C::prefetch::name(),
        C::concurrency::name(), C::serialization::name(),      C::telemetry::name(),   C::value_handle::name(),
        C::isa::name(),         C::index_organization::name(), C::io_dispatch::name(), C::migration_policy::name(),
        C::filter::name(),      C::queuing_q1::name(),         C::queuing_q2::name()};
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (!out.empty()) out += '/';
        out += serialize_axis_segment(kCompositionAxisNames[i], v[i]);
    }
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
