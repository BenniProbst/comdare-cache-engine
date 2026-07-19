#pragma once
// BR-2-Fundament (2026-06-02, Doc 27 §3+§5-R3) — DIE EINE zentrale Pfad-Serialisierungs-Konvention.
//
// Doc 27 §5 R3: „Pfad-Serialisierung muss BR-1↔BR-2↔BR-4 identisch sein → EINE zentrale serialize_path-Konvention."
// Dieser Header IST diese Konvention. Sowohl BR-1 (registry_to_axis_levels: Baum-AxisLevel-Namen) als auch BR-2
// (composition_registry: PermTuple→Pfad) als auch BR-4 (perm_<id>.cpp-Benennung) leiten ihre Pfad-Strings hierüber
// ab — so ist der Baum-Blatt-`binary_id` GARANTIERT gleich dem CompositionRegistry-Key (Round-Trip-Garant).
//
// Format (identisch zum Baum: experiment_tree.hpp StaticAxisNode::serialize() = "axis=value", "/"-join):
//   "search_algo=<W0::name()>/cache_traversal=<W1::name()>/.../queuing_q2=<W16::name()>"  (17 Slots, INC-2d)
// C++23, header-only, KEINE Achsen-Includes (umbrella-unabhängig; nimmt das PermTuple als Template-Param entgegen).

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::builder::experiment {

namespace mp = boost::mp11;

/// Die 17 Komposition-Achsen-Namen in AdHocComposition-Slot-Reihenfolge T0..T16 (composition_factory.hpp;
/// Doc 30 §8.0 erweitert um queuing_q1/queuing_q2). Bau-INC-2c (F12iii, ABI-5): "telemetry" hat die
/// binary_id-permutierende Komposition verlassen (CEB-System-Achse, H-10-Sidecar; vorher Slot 10 von 19).
/// Bau-INC-2d (ABI-6): "isa" hat die Komposition ebenfalls verlassen (Target-ISA-System-Achse, build-config-
/// gewählter Codegen-Codepfad, +target=-Sidecar; vorher Slot 12 von 19 / T11 von 18). Das isa-ORGAN
/// (Amd64Isa/ObservableIsa) bleibt als Codegen-Träger, permutiert aber nicht mehr (telemetry-treu).
/// TEILMENGE der Achsen aus registry_to_axis_levels.hpp build_all_axis_levels() (BR-1, zentrale Quelle) —
/// aber NICHT deren Präfix: dort stehen zwischen T15 und q1/q2 die 3 build-only-Achsen
/// (page_type/simd_extension/general_hardware), dahinter die 4 node-shape-Achsen (#234-K).
inline constexpr std::array<std::string_view, 17> kCompositionAxisNames = {
    "search_algo", "cache_traversal",  "mapping",     "path_compression", "node_type",    "memory_layout",
    "allocator",   "prefetch",         "concurrency", "serialization",    "value_handle", "index_organization",
    "io_dispatch", "migration_policy", "filter",      "queuing_q1",       "queuing_q2"};

/// Ein Achsen-Pfad-Segment: "axis=value" (= experiment_tree.hpp StaticAxisNode::serialize()).
[[nodiscard]] inline std::string serialize_axis_segment(std::string_view axis, std::string_view value) {
    std::string s{axis};
    s += '=';
    s += value;
    return s;
}

/// serialize_composition_path<P>() — der serialisierte Static-Pfad EINER 17-Achsen-Permutation (PermTuple<V0..V16>; INC-2d).
/// Baut "search_algo=<V0::name()>/.../queuing_q2=<V16::name()>" — exakt der `binary_id`, den der Experiment-Baum für
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

/// serialize_composition_from_slots<C>() — derselbe Pfad, aber aus den 17 NAMED using-Slots einer
/// AdHocComposition<…> (C::search_algo::name() … C::filter::name()). Round-Trip-Beleg (BR-2): wenn
/// serialize_composition_from_slots<CompositionFromPermTuple<P>>() == serialize_composition_path<P>(),
/// hat CompositionFromPermTuple die Slot-Reihenfolge T0..T16 KORREKT erhalten (P → Composition verlustfrei).
/// Bau-INC-2d: C::isa::name() ist raus (isa ist keine Kompositions-Achse mehr — Target-ISA-System-Achse).
template <class C>
[[nodiscard]] inline std::string serialize_composition_from_slots() {
    std::array<std::string_view, 17> const v = {
        C::search_algo::name(), C::cache_traversal::name(),  C::mapping::name(),      C::path_compression::name(),
        C::node_type::name(),   C::memory_layout::name(),    C::allocator::name(),    C::prefetch::name(),
        C::concurrency::name(), C::serialization::name(),    C::value_handle::name(), C::index_organization::name(),
        C::io_dispatch::name(), C::migration_policy::name(), C::filter::name(),       C::queuing_q1::name(),
        C::queuing_q2::name()};
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (!out.empty()) out += '/';
        out += serialize_axis_segment(kCompositionAxisNames[i], v[i]);
    }
    return out;
}

/// with_shape_segment<Shape>(path, shape_axis) — haengt OPTIONAL EIN node-shape-Segment
/// "/<shape_axis>=<Shape::name()>" an einen serialisierten 17-Achsen-binary_id. Default-OFF: Shape=void => path
/// BYTE-IDENTISCH zurueck (golden_fullpilot_320 unberuehrt). Nur bei aktivem Shape-Traeger EINER organ-backed
/// Familie (234-V-b) wird das Segment NACH queuing_q2 emittiert. shape_axis-Name vom Aufrufer (dieser Header
/// bleibt achsen-frei).
template <class Shape>
[[nodiscard]] inline std::string with_shape_segment(std::string path, std::string_view shape_axis) {
    if constexpr (!std::is_same_v<Shape, void>) {
        if (!path.empty()) path += '/';
        path += serialize_axis_segment(shape_axis, Shape::name());
    }
    return path;
}

} // namespace comdare::cache_engine::builder::experiment
