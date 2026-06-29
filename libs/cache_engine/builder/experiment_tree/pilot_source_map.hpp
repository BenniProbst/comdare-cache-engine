#pragma once
// L-LAZY-E2E (gate-frei, 2026-06-03) — pilot_source_map: die TYP→STRING-Brücke zwischen dem string-getriebenen
// BuildOrchestrator (SourceGenFn: binary_id → Source) und dem TYP-getriebenen realen Anatomie-Emitter
// (codegen::adhoc_emitter, Engine::for_each_composition_type). Löst die ZWEI-PFAD-Spannung aus ceb_generator.hpp:
//   • Der BuildOrchestrator kennt nur den Baum-`binary_id`-STRING und kann daraus KEINEN C++-Typ auflösen
//     (Direktive „kein Runtime-Switch", String→Typ verboten).
//   • Die reale, baubare, observierbare Anatomie kommt NUR TYP-getrieben (CompositionFromPermTuple<P> +
//     COMDARE_DEFINE_ANATOMY_MODULE_ADHOC, BR-4).
// Auflösung (BR-2-Prinzip, composition_registry.hpp): EINMAL host-seitig über alle Pilot-Permutationen iterieren
// (compile-time for_each_permutation), je Permutation den serialisierten Pfad (== Baum-`binary_id`,
// axis_path_serialization.hpp = DIE zentrale Konvention) als KEY + die REALE Anatomie-Source
// (render_adhoc_module_source(adhoc_macro_args<Comp>()), BR-4) als VALUE in eine map ablegen. Diese map IST die
// SourceGenFn → der Orchestrator schlägt je binary_id die fertige reale Quelle nach (kein String→Typ-Dispatch).
//
// ⚠️ Dieser Header inkludiert den all_axes_umbrella + die Topic-ConfigSets der Pilot-Achsen → compiler-heap-schwer.
// Er gehört in die HARNESS-/Test-.cpp (opt-in), NICHT in den engine-agnostischen Treiber-Header
// cache_engine_builder_iterator.hpp (der bleibt umbrella-frei). C++23.

#include "axis_path_serialization.hpp"           // serialize_composition_path<P>() (== Baum-binary_id)
#include "../codegen/adhoc_emitter.hpp"          // render_adhoc_module_source / adhoc_macro_args<Comp>
#include "../../anatomy/composition_factory.hpp" // CompositionFromPermTuple<P> (PermTuple → AdHocComposition)

#include <functional>
#include <map>
#include <string>

namespace comdare::cache_engine::builder::experiment {

/// build_pilot_source_map<Engine>() — iteriert die Pilot-Permutationen EINMAL (compile-time for_each_permutation)
/// und baut die Brücken-map: Baum-`binary_id` (serialize_composition_path<P>) → REALE Anatomie-Modul-Quelle
/// (render_adhoc_module_source(adhoc_macro_args<CompositionFromPermTuple<P>>)). Der Index im Modul-Kommentar ist
/// laufend (nur Doku — der Round-Trip-Key ist der Pfad). Die map deckt GENAU die Pilot-Kompositionen ab; ein
/// binary_id außerhalb (z.B. größerer Baum als der Pilot) liefert keine Quelle (→ Orchestrator-Source-Fehler,
/// ehrlich sichtbar). O(count()) Einträge — der Pilot ist klein gehalten (sonst C1060; Doc 27 §6).
template <class Engine>
[[nodiscard]] inline std::map<std::string, std::string> build_pilot_source_map() {
    std::map<std::string, std::string> by_path;
    int                                idx = 0;
    Engine::for_each_permutation([&]<class P>() {
        using Comp = anatomy::CompositionFromPermTuple<P>; // reale AdHocComposition<19> (compile-time materialisiert)
        std::string const path = serialize_composition_path<P>();
        by_path.emplace(path, codegen::render_adhoc_module_source(idx, codegen::adhoc_macro_args<Comp>()));
        ++idx;
    });
    return by_path;
}

/// make_source_gen_from_map — verpackt die Brücken-map als SourceGenFn-kompatibles Callable (binary_id → Source).
/// Unbekannter binary_id → leere Quelle (der BuildOrchestrator markiert die DLL dann als nicht schreibbar/baubar,
/// statt still falschen Code zu erzeugen). Die map wird per Wert kopiert (by_path ist klein/Pilot).
[[nodiscard]] inline std::function<std::string(std::string const&)>
make_source_gen_from_map(std::map<std::string, std::string> by_path) {
    return [by_path = std::move(by_path)](std::string const& binary_id) -> std::string {
        auto it = by_path.find(binary_id);
        return it == by_path.end() ? std::string{} : it->second;
    };
}

} // namespace comdare::cache_engine::builder::experiment
