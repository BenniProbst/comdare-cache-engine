#pragma once
// V5-#26 / Task #153 — make_pmc_source(): die EINE Auswahlstelle der PMC-Quelle (Strategy + Factory).
//
// Lehrbuch-Pattern: IPmcSource = Strategy; make_pmc_source() = (Simple-)Factory, die die plattform-/feature-
// abhängige konkrete Quelle wählt. So bleibt der Aufruf-Code (z.B. f15_compare/main.cpp) frei von #ifdef.
//
// BUILD-SICHERHEIT: nur unter `COMDARE_ENABLE_PMC && _WIN32` wird `WindowsPcmPmcSource` gewählt (und ihr
// Header inkludiert — der wiederum guarded `cpucounters.h`). In JEDEM anderen Fall (Default-Build, OFF,
// Nicht-Windows) → `NullPmcSource`. Default-Build zieht KEINEN Intel-PCM-Header/keine -Lib.

#include <memory>

#include "pmc_source.hpp"   // IPmcSource / NullPmcSource (UNVERÄNDERT)

#if defined(COMDARE_ENABLE_PMC) && defined(_WIN32)
#include "windows_pcm_pmc_source.hpp"   // selbst guarded → leer ohne das Makro
#endif

namespace comdare::cache_engine::builder {

/// Wählt die beste verfügbare PMC-Quelle für die aktuelle Build-Konfiguration.
/// COMDARE_ENABLE_PMC + Windows → reale Intel-PCM-Quelle; sonst ehrliche NullPmcSource (available=false).
[[nodiscard]] inline std::unique_ptr<IPmcSource> make_pmc_source() {
#if defined(COMDARE_ENABLE_PMC) && defined(_WIN32)
    return std::make_unique<WindowsPcmPmcSource>();
#else
    return std::make_unique<NullPmcSource>();
#endif
}

}  // namespace comdare::cache_engine::builder
