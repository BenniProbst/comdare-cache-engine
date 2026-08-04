#pragma once
// E-24 C4 (2026-08-04, Bauplan-Dossier docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C4:
// "Registry-Ausweitung nach Muster f15_compare/main.cpp:220") -- die EINE Voll-Belegung der Dock-Registry.
//
// WARUM DIESE DATEI EXISTIERT (und die Registrierung nicht einfach in f15_compare stehen bleibt): der
// Bauplan verlangt die Ausweitung "nach dem Muster f15_compare:220" -- also an der Stelle, die am Ist der
// EINZIGE produktive register_dock-Aufrufer ist (Paragraf 1.0; OP-4-Rest: "Haupt-Messtreiber Registry vs.
// Direkt-Aufrufe"). Eine Liste, die nur in einer main() steht, ist aber von keinem Test erreichbar: eine
// Wache haette die f15-Registrierung nur ueber einen grep nachbilden koennen, und eine nachgebildete
// Liste laeuft irgendwann auseinander (Memory-Lehre "gruene Tests zementieren alte Ordnung" -- hier waere
// es der umgekehrte Fall: ein gruener Test, der eine ANDERE Liste prueft als die produktive).
// Deshalb: die Voll-Belegung steht EINMAL hier, f15_compare ruft sie auf, und die Test-TU prueft
// DIESELBE Funktion. Es gibt keine zweite Liste, die driften koennte.
//
// LAYER-SCHNITT (bewusst): pruef_dock_registry.hpp bleibt frei von Dock-Abhaengigkeiten -- die Registry
// kennt nur IPruefDock. Erst DIESER Header zieht die fuenf konkreten Docks herein. Wer eine eigene,
// engere Belegung braucht (z.B. ein Test mit genau einem Dock), nimmt weiterhin die Registry direkt.
//
// ABI-NEUTRAL (a-Teil): reine Builder-Seite, header-only.

#include "pruef_dock_registry.hpp"
#include "search_algorithm_dock.hpp"

#include "adapter_dock.hpp"
#include "sequence_dock.hpp"
#include "set_dock.hpp"
#include "view_dock.hpp"

#include <memory>

namespace comdare::cache_engine::builder::pruef_dock {

/// register_all_genus_docks -- registriert je EIN Dock fuer JEDE der fuenf Ebene-2-Gattungen
/// (anatomy_base.hpp:78-84). Die Reihenfolge ist ohne Bedeutung: select_for matcht ueber die IM MODUL
/// deklarierte Gattung (anatomy()->genus()), nicht ueber Dateinamen oder Registrierungs-Reihenfolge.
///
/// Kommt eine sechste Ebene-2-Gattung dazu, ist DIES die Stelle, die sie vergisst -- und die Wache
/// dagegen ist test_e24_c4_genus_pruef_docks, die dock_for_genus() ueber ALLE Enum-Werte laufen laesst.
inline void register_all_genus_docks(PruefDockRegistry& reg) {
    reg.register_dock(std::make_unique<SearchAlgorithmDock>());
    reg.register_dock(std::make_unique<SetPruefDock>());
    reg.register_dock(std::make_unique<SequencePruefDock>());
    reg.register_dock(std::make_unique<AdapterPruefDock>());
    reg.register_dock(std::make_unique<ViewPruefDock>());
}

} // namespace comdare::cache_engine::builder::pruef_dock
