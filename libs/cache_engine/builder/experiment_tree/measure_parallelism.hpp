#pragma once
// measure_parallelism.hpp -- #45 (§16.2-M1/§61-MODI, 2026-07-21): die EINE Runtime-Naht, die aus der aktiven Methodik
// (tp/ep.run_methodology) die Zahl paralleler MESS-Worker (cfg.measure_parallelism) ableitet.
//
// Debug-Modus (measurement_on && !single_thread) => COMDARE_MEASURE_PARALLEL (ein EIGENES Env, KLAR GETRENNT vom
// Compile-Pool COMDARE_BUILD_PARALLEL) oder hardware_concurrency() als nproc-Default. Measure/Release/undeklariert =>
// 0 => sequentieller 1-Thread-Mess-Vollzug (byte-neutral zum Ist). Der Entry (profile_run_entry/experiment_run_entry)
// belegt cfg.measure_parallelism hieraus; der Iterator schaltet daran den sequentiellen bzw. parallelen Dispatch.
// LEICHT (nur run_methodology_registry + stdlib) -- separat testbar ohne die schwere Iterator-/DLL-Include-Kette.

#include <cache_engine/measurement/run_methodology_registry.hpp> // run_methodology_for_ids (debug/measure/release)

#include <cstddef>
#include <cstdlib> // std::getenv (COMDARE_MEASURE_PARALLEL)
#include <string>
#include <thread> // std::thread::hardware_concurrency() (nproc-Default)
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Leitet cfg.measure_parallelism aus der Methodik ab (siehe Datei-Kopf). Env-unparsebar/0 => nproc-Default (nie throw).
[[nodiscard]] inline std::size_t resolve_measure_parallelism(std::vector<std::string> const& run_methodology) {
    auto const& m = ::comdare::cache_engine::measurement::run_methodology_for_ids(run_methodology);
    if (!m.measurement_on || m.single_thread) return 0; // Measure/Release/undeklariert => 1-Thread (byte-neutral)
    if (char const* e = std::getenv("COMDARE_MEASURE_PARALLEL"); e != nullptr && *e != '\0') {
        std::size_t v     = 0;
        bool        digit = false;
        for (char const* p = e; *p != '\0'; ++p) {
            if (*p < '0' || *p > '9') { // unparsebar -> Default (nproc)
                digit = false;
                break;
            }
            v     = v * 10 + static_cast<std::size_t>(*p - '0');
            digit = true;
        }
        if (digit && v > 0) return v;
    }
    unsigned const hw = std::thread::hardware_concurrency();
    return hw > 0 ? static_cast<std::size_t>(hw) : 1; // nproc-Default; hw==0 (unbekannt) => 1
}

} // namespace comdare::cache_engine::builder::experiment
