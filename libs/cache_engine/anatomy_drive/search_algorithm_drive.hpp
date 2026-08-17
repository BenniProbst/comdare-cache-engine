#pragma once
// K2 -- DAS STUFEN-NEUTRALE ANTRIEBS-BUENDEL der SearchAlgorithm-Gattung.
//
// ============================================================================================
// WAS HIER STEHT UND WARUM ES HIER STEHT
// ============================================================================================
// SearchAlgorithmDrive + acquire_search_algorithm_drive, WOERTLICH aus
// builder/pruef_dock/search_algorithm_dock.hpp herausgeloest -- unveraendert in Signatur,
// Semantik und Fehlerpfaden. Es ist eine WANDERUNG, keine Neufassung: der alte Header
// re-exportiert beide Namen per using, und die drei Bestands-Konsumenten
// (pruef_only.hpp, cache_engine_builder_iterator.hpp, search_algorithm_dock.hpp selbst)
// bleiben unangetastet.
//
// DER GRUND ist der Owner-Entscheid vom 09.08.2026 zu K2: "der Loader wandert in eine
// stufen-neutrale Bibliothek, weil das Pruefdock der CEB und das Pruefdock der Hybrid-Tier-Binary
// jeweils technisch identisch bei Konfiguration sein muessen."
//
// Genau das ist die Sache: das Ebene-2-Dock (CEB <-> Tier) und das Ebene-3-Dock
// (Hybrid <-> plain Tiers) muessen ihr Ziel auf DIESELBE Art anfassen. Laege der Drive weiterhin
// in der Builder-Lib, muesste die Hybrid-Stufe den Builder linken -- oder ihre eigene, zweite
// Antriebs-Beschaffung schreiben. Die zweite waere die schlimmere Variante: zwei Wege, ein Ziel
// anzufassen, die still auseinanderlaufen.
//
// ============================================================================================
// WAS BEWUSST NICHT MITGEWANDERT IST
// ============================================================================================
// IPruefDock, SearchAlgorithmDock, das Konformitaets-Gate, die Dock-Registry und der Sequencer.
// Sie tragen DOCK-Semantik (Gattungs-Match, Mess-Vertrag, Registrierung) und gehoeren zur
// Builder-Stufe. Die Doktrin-Zeile "IPruefDock lebt nur im Builder-Binary"
// (builder/pruef_dock/pruef_dock.hpp) bleibt damit woertlich wahr.
//
// Die Trennlinie laeuft entlang der Frage: beschreibt es die ABI-KANTE zum geladenen Modul
// (-> neutral) oder das VERHALTEN eines Docks (-> Builder)? Der Drive ist reine ABI-Kante:
// vier dynamic_casts auf Sub-Interfaces, sonst nichts.
//
// @autoritaet Owner-Entscheid 09.08.2026 (K2 = Option (a))
// @herkunft builder/pruef_dock/search_algorithm_dock.hpp (INC-2a/Q4), unveraendert uebernommen

#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>            // IObservableTier (SearchAlgorithm-Gattungs-Antrieb)
#include <anatomy/resource_controllable_tier.hpp> // INC-2a: IResourceControllableTier
#include <anatomy/rollbackable_tier.hpp>          // V5-I6/I7: IRollbackableTier (memento_all)
#include <anatomy/scannable_tier.hpp>             // INC-2a: IScannableTier (YCSB-E Range-Scan)
#include <anatomy_drive/tier_drive_status.hpp>
#include <builder/anatomy_module_loader/anatomy_module_loader.hpp> // AnatomyModuleHandle

namespace comdare::cache_engine::anatomy_drive {

namespace anatomy        = ::comdare::cache_engine::anatomy;
namespace anatomy_loader = ::comdare::cache_engine::builder::anatomy_loader;

/// INC-2a (Q4, Pruef-Dock scharf): das dock-vertragliche Antriebs-Buendel der SearchAlgorithm-Gattung.
/// obs = Mess-Antrieb (Pflicht fuer Messung), ctrl/rbk/scn = optionale Sub-Antriebe (alte DLLs -> nullptr).
struct SearchAlgorithmDrive {
    anatomy::IObservableTier*           obs  = nullptr;
    anatomy::IResourceControllableTier* ctrl = nullptr;
    anatomy::IRollbackableTier*         rbk  = nullptr;
    anatomy::IScannableTier*            scn  = nullptr;
};

/// INC-2a (Q4): die EINE dock-vertragliche Antriebs-Beschaffung -- ersetzt die rohen dynamic_cast-
/// Bypaesse des Lazy-Iterators (cache_engine_builder_iterator). Semantik BEWUSST identisch zum
/// bisherigen Iterator-Verhalten (kein Gattungs-Reject hier -- der scharfe Gattungs-Match kommt mit
/// den Multi-Gattungs-Docks in INC-2d/2e ueber accepts()): base fehlt -> no_anatomy; Mess-Antrieb
/// fehlt -> subinterface_missing; sonst ok mit vollem Buendel.
[[nodiscard]] inline int acquire_search_algorithm_drive(anatomy_loader::AnatomyModuleHandle& h,
                                                        SearchAlgorithmDrive&                out) noexcept {
    anatomy::IAnatomyBase* base = h.anatomy();
    if (base == nullptr) return dock_status_no_anatomy;
    out.obs  = dynamic_cast<anatomy::IObservableTier*>(base);
    out.ctrl = dynamic_cast<anatomy::IResourceControllableTier*>(base);
    out.rbk  = dynamic_cast<anatomy::IRollbackableTier*>(base);
    out.scn  = dynamic_cast<anatomy::IScannableTier*>(base);
    return (out.obs == nullptr) ? dock_status_subinterface_missing : dock_status_ok;
}

} // namespace comdare::cache_engine::anatomy_drive
