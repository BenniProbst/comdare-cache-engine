#pragma once
// pruef_dock/pruef_only.hpp -- S3 (Section 62-B, COMDARE_PRUEF_ONLY): laedt EINE bereits gebaute Tier-.so und faehrt
// NUR das Konformitaets-Gate (run_conformance_gate) darueber -- KEIN Bau, KEINE Messung. Herausloesung der
// Load+Drive+Gate-Sequenz aus measure_one_binary (cache_engine_builder_iterator.hpp): AnatomyModuleLoader ->
// acquire_search_algorithm_drive -> run_conformance_gate ueber die geladene Tier (IObservableTier IS-A IDriveableTier).
// RAII entlaedt die .so beim Verlassen. Header-only, rein-lesend (kein Compile).

#include "conformance_gate.hpp"      // run_conformance_gate / ConformanceResult
#include "mess_konsistenz_gate.hpp"  // M-1/D-2: pruefe_mess_konsistenz (Vertrag CEB <-> Tier-Binary)
#include "search_algorithm_dock.hpp" // SearchAlgorithmDrive / acquire_search_algorithm_drive / dock_status_ok

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp> // AnatomyModuleLoader / AnatomyModuleHandle

#include <cstdint>
#include <filesystem>

namespace comdare::cache_engine::builder::pruef_dock {

/// Ergebnis eines Pruef-only-Laufs EINER .so. loaded=false => .so nicht ladbar / kein Mess-Interface (== nicht
/// pruefbar = Fail); sonst entscheiden gate.passed() UND mess.passed(). passed() fasst alles zusammen.
///
/// M-1/D-2: `mess` traegt die IDENTITAETS-Pruefung (deklarierte Mess-Ausstattung == von der CEB einkompilierte).
/// Sie ist ein EIGENES Feld und keine zusaetzliche Zusicherung im ConformanceResult des Funktions-Gates -- die
/// beiden Gatter beantworten verschiedene Fragen (KANN die Huelle / IST sie die richtige), und ein Bediener muss
/// am Ergebnis ablesen koennen, welche der beiden gebrochen ist.
struct PruefOutcome {
    bool                   loaded = false;
    ConformanceResult      gate{};
    MessKonsistenzErgebnis mess{}; ///< Default-Status ist erwartung_leer == fail-closed
    [[nodiscard]] bool     passed() const noexcept { return loaded && gate.passed() && mess.passed(); }
};

/// run_so_conformance_gate(so_path, erwartete_mess_zeile) -- laedt die gebaute .so, prueft den Mess-Vertrag,
/// holt den Dock-Antrieb und faehrt run_conformance_gate ueber die geladene Tier. KEIN Bau, KEINE Messung.
/// seed/n_random wie der Dock-Default (deterministisch). Die .so wird beim Verlassen per Handle-RAII entladen.
///
/// REIHENFOLGE (bindend, und der Grund steht hier): laden -> MESS-VERTRAG -> Antrieb -> Funktions-Gate. Der
/// Identitaets-Vertrag steht VOR dem teuren Funktions-Gate, weil eine Binary mit falscher Mess-Ausstattung
/// auch dann nicht gemessen werden darf, wenn ihre std::map-Huelle tadellos ist -- und weil ein 2000-Op-Lauf
/// ueber eine Binary, die ohnehin abgewiesen wird, verschenkte Zeit ist. Das Ergebnis traegt trotzdem BEIDE
/// Befunde, wo sie erhoben wurden; abgebrochen wird nicht, damit der Bediener die volle Diagnose bekommt.
///
/// FAIL-CLOSED: `erwartete_mess_zeile` hat KEINEN Default. Ein Aufrufer muss sagen, was er erwartet -- ein
/// leerer String ist eine gueltige EINGABE mit dem Ergebnis "erwartung_leer" (also Fail), aber er ist kein
/// stillschweigender Standard, den man durch Weglassen bekommt.
[[nodiscard]] inline PruefOutcome run_so_conformance_gate(std::filesystem::path const& so_path,
                                                          std::string_view             erwartete_mess_zeile,
                                                          std::uint64_t seed = 42, std::uint64_t n_random = 2000) {
    PruefOutcome                        out;
    anatomy_loader::AnatomyModuleHandle handle;
    if (anatomy_loader::AnatomyModuleLoader::load(so_path, handle) != anatomy_loader::status_ok)
        return out; // loaded bleibt false
    out.mess = pruefe_mess_konsistenz(handle, erwartete_mess_zeile);
    SearchAlgorithmDrive drive;
    if (acquire_search_algorithm_drive(handle, drive) != dock_status_ok || drive.obs == nullptr) return out;
    out.loaded = true;
    out.gate   = run_conformance_gate(*drive.obs, seed, n_random); // IObservableTier IS-A IDriveableTier
    return out;
}

} // namespace comdare::cache_engine::builder::pruef_dock
