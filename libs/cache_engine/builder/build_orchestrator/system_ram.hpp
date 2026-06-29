#pragma once
// KF-16b (2026-06-02) — reale OS-Abfrage des freien physischen RAM (FreeRamFn-Fabrik). SEPARAT von
// build_orchestrator.hpp, damit der Kern-Header windows.h-frei bleibt. Opt-in: der reale Build-Treiber
// inkludiert diesen Header; Tests injizieren stattdessen einen Stub-FreeRamFn. C++23.

#include "build_orchestrator.hpp"

#include <cstdint>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/sysinfo.h>
#endif

namespace comdare::cache_engine::builder::experiment {

/// Liefert eine FreeRamFn, die den freien physischen RAM des Systems abfragt (Windows GlobalMemoryStatusEx /
/// POSIX sysinfo). Bei Fehlschlag → free_ram_unlimited (RAM-Gate effektiv aus statt fälschlich zu blockieren).
[[nodiscard]] inline FreeRamFn make_system_free_ram_fn() {
    return []() -> std::uint64_t {
#if defined(_WIN32)
        MEMORYSTATUSEX m{};
        m.dwLength = sizeof(m);
        if (GlobalMemoryStatusEx(&m)) return static_cast<std::uint64_t>(m.ullAvailPhys);
        return free_ram_unlimited();
#else
        struct sysinfo si{};
        if (::sysinfo(&si) == 0)
            return static_cast<std::uint64_t>(si.freeram) * static_cast<std::uint64_t>(si.mem_unit);
        return free_ram_unlimited();
#endif
    };
}

} // namespace comdare::cache_engine::builder::experiment
