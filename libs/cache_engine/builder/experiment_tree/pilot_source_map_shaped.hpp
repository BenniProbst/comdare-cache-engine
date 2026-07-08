#pragma once
// 234-V-b (2026-07-08) -- SHAPED-Sibling zu pilot_source_map: Baum->Emitter-Verdrahtung fuer EINE
// Shape-Variante EINER organ-backed Pool-Familie (default-OFF). Iteriert dieselben Pilot-Permutationen wie
// build_pilot_source_map, filtert auf organ-backed (organ_for != void = Adapter-pool_family_), KEY =
// shape-segmentierter binary_id (with_shape_segment<Shape>), VALUE = SHAPED-Quelle
// (render_adhoc_module_source_shaped).
// INVARIANTE (wie 234-V-a): organ-backed Kompositionen des Engine muessen zur FAMILIE von Shape gehoeren --
// sonst ist die EMITTIERTE TU (nicht dieser Header) fuer die falsche Familie nicht kompilierbar
// (Pool-Store-Concept). Aufrufer nutzen Ein-Familien-Engines. Dieser Header INSTANZIIERT den Shaped-Pool NICHT
// (rendert nur Text) -- fuer jeden Engine sicher.
// Sibling-Disziplin: bringt Achsen-/Shaped-Emitter-Wurzeln mit; Plain-pilot_source_map.hpp unberuehrt.

#include "axis_path_serialization.hpp"
#include "pilot_source_map.hpp"
#include "../codegen/adhoc_emitter_shaped.hpp"
#include "../codegen/adhoc_emitter.hpp"
#include "../../anatomy/composition_factory.hpp"

#include <axes/lookup/composable/organ_for_search_algo.hpp>

#include <map>
#include <string>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::builder::experiment {

template <class Engine, class Shape>
[[nodiscard]] inline std::map<std::string, std::string> build_pilot_source_map_shaped(std::string_view shape_axis,
                                                                                      std::string_view shape_include) {
    std::map<std::string, std::string> by_path;
    int                                idx = 0;
    Engine::for_each_permutation([&]<class P>() {
        using Comp   = anatomy::CompositionFromPermTuple<P>;
        using OrganT = ::comdare::cache_engine::lookup::composable::organ_for_search_algo_t<typename Comp::search_algo>;
        if constexpr (!std::is_same_v<OrganT, void>) {
            std::string const path = with_shape_segment<Shape>(serialize_composition_path<P>(), shape_axis);
            by_path.emplace(path,
                            codegen::render_adhoc_module_source_shaped(idx, codegen::type_name<Shape>(), shape_include,
                                                                       codegen::adhoc_macro_args<Comp>()));
            ++idx;
        }
    });
    return by_path;
}

} // namespace comdare::cache_engine::builder::experiment
