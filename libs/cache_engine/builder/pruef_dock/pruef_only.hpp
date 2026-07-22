#pragma once
// pruef_dock/pruef_only.hpp -- S3 (Section 62-B, COMDARE_PRUEF_ONLY): laedt EINE bereits gebaute Tier-.so und faehrt
// NUR das Konformitaets-Gate (run_conformance_gate) darueber -- KEIN Bau, KEINE Messung. Herausloesung der
// Load+Drive+Gate-Sequenz aus measure_one_binary (cache_engine_builder_iterator.hpp): AnatomyModuleLoader ->
// acquire_search_algorithm_drive -> run_conformance_gate ueber die geladene Tier (IObservableTier IS-A IDriveableTier).
// RAII entlaedt die .so beim Verlassen. Header-only, rein-lesend (kein Compile).

#include "conformance_gate.hpp"      // run_conformance_gate / ConformanceResult
#include "search_algorithm_dock.hpp" // SearchAlgorithmDrive / acquire_search_algorithm_drive / dock_status_ok

#include <builder/anatomy_module_loader/anatomy_module_loader.hpp> // AnatomyModuleLoader / AnatomyModuleHandle

#include <cstdint>
#include <filesystem>

namespace comdare::cache_engine::builder::pruef_dock {

/// Ergebnis eines Pruef-only-Laufs EINER .so. loaded=false => .so nicht ladbar / kein Mess-Interface (== nicht
/// pruefbar = Fail); sonst entscheidet gate.passed(). passed() fasst beides zusammen.
struct PruefOutcome {
    bool               loaded = false;
    ConformanceResult  gate{};
    [[nodiscard]] bool passed() const noexcept { return loaded && gate.passed(); }
};

/// run_so_conformance_gate(so_path) -- laedt die gebaute .so, holt den Dock-Antrieb und faehrt run_conformance_gate
/// ueber die geladene Tier. KEIN Bau, KEINE Messung. seed/n_random wie der Dock-Default (deterministisch). Die .so wird
/// beim Verlassen per Handle-RAII wieder entladen.
[[nodiscard]] inline PruefOutcome run_so_conformance_gate(std::filesystem::path const& so_path, std::uint64_t seed = 42,
                                                          std::uint64_t n_random = 2000) {
    PruefOutcome                        out;
    anatomy_loader::AnatomyModuleHandle handle;
    if (anatomy_loader::AnatomyModuleLoader::load(so_path, handle) != anatomy_loader::status_ok)
        return out; // loaded bleibt false
    SearchAlgorithmDrive drive;
    if (acquire_search_algorithm_drive(handle, drive) != dock_status_ok || drive.obs == nullptr) return out;
    out.loaded = true;
    out.gate   = run_conformance_gate(*drive.obs, seed, n_random); // IObservableTier IS-A IDriveableTier
    return out;
}

} // namespace comdare::cache_engine::builder::pruef_dock
