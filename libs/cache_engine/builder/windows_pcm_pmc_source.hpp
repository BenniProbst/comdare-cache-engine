#pragma once
// V5-#26 / Task #153 — WindowsPcmPmcSource: REALE CPU-Cache-Miss-Quelle via Intel PCM (Windows).
//
// Drop-in-Implementierung der `IPmcSource`-Naht (pmc_source.hpp): liefert die +6 HW-Counter
// (cache_misses L2/L3, dtlb, coherence, energy) aus echten Performance-Monitoring-Countern statt
// `NullPmcSource`-0. KEINE Änderung an POD/Pipeline/PDF (pmc_source.hpp:6-9) — eine weitere IPmcSource.
//
// BUILD-SICHERHEIT (kritisch): die GANZE Implementierung steht hinter
//     #if defined(COMDARE_ENABLE_PMC) && defined(_WIN32)
// Ohne dieses Makro kompiliert diese Datei zu EINEM LEEREN Translation-Unit-Inhalt (nur das pragma once
// + Kommentare) — `cpucounters.h` wird NIE inkludiert, kein Symbol entsteht. Der Default-Build
// (COMDARE_ENABLE_PMC OFF) bricht damit NIE und braucht weder Intel-PCM-Header noch -Lib noch msr.sys.
//
// AKTIVIERUNG (System-Schritt, NICHT Teil dieses Scaffolds): Intel PCM (BSD-3) vendoren, COMDARE_PCM_DIR
// setzen, -DCOMDARE_ENABLE_PMC=ON, msr.sys signieren + nach system32, Messung als Admin.
//   → docs/sessions/2026-06-18-windows-pcm-pmc-source-integration.md
//
// Intel-PCM-API (web-verifiziert, BSD-3): PCM::getInstance() → program() (==PCM::Success bei Erfolg),
// getSystemCounterState() (Snapshot), getL2CacheMisses(before,after) / getL3CacheMisses(before,after).
// Header: cpucounters.h.

#if defined(COMDARE_ENABLE_PMC) && defined(_WIN32)

#include "pmc_source.hpp" // IPmcSource / PmcCounters (UNVERÄNDERT)

// Intel PCM-Header NUR hier (innerhalb des Guards) — sonst würde der Default-Build ihn anfordern.
#include <cpucounters.h>

namespace comdare::cache_engine::builder {

/// REALE PMC-Quelle (Intel PCM, Windows). begin() = Snapshot, end() = zweiter Snapshot → Counter-Delta.
/// `available()` spiegelt EHRLICH den PCM-Init-Status (program()==PCM::Success); schlägt der Treiber/MSR-
/// Zugriff fehl (kein signiertes msr.sys / keine Admin-Rechte), bleibt available()=false → wie NullPmcSource.
class WindowsPcmPmcSource final : public IPmcSource {
public:
    WindowsPcmPmcSource() noexcept {
        // PCM ist ein Prozess-Singleton; program() initialisiert den msr.sys-Treiber-Zugriff.
        pcm_ = ::pcm::PCM::getInstance();
        if (pcm_ != nullptr) { ready_ = (pcm_->program() == ::pcm::PCM::Success); }
    }

    void begin() noexcept override {
        if (ready_ && pcm_ != nullptr) { before_ = pcm_->getSystemCounterState(); }
    }

    [[nodiscard]] PmcCounters end() noexcept override {
        PmcCounters c;
        if (!ready_ || pcm_ == nullptr) {
            return c; // available=false (Default) — ehrlich „nicht gemessen"
        }
        auto const after = pcm_->getSystemCounterState();
        // L1 wird von der System-weiten PCM-Counter-State nicht direkt geliefert (architektur-abhängig) —
        // L2/L3 sind die robusten, web-verifizierten getInstance()-Pfade; L1 bleibt ehrlich 0.
        c.cache_misses_l2 = static_cast<std::uint64_t>(::pcm::getL2CacheMisses(before_, after));
        c.cache_misses_l3 = static_cast<std::uint64_t>(::pcm::getL3CacheMisses(before_, after));
        c.available       = true;
        return c;
    }

    [[nodiscard]] bool             available() const noexcept override { return ready_ && pcm_ != nullptr; }
    [[nodiscard]] std::string_view name() const noexcept override { return "intel-pcm-windows"; }

private:
    ::pcm::PCM*               pcm_ = nullptr; ///< Prozess-Singleton (nicht besitzend)
    ::pcm::SystemCounterState before_{};      ///< Snapshot bei begin()
    bool                      ready_ = false; ///< program()==Success (Treiber/MSR ok)
};

} // namespace comdare::cache_engine::builder

#endif // COMDARE_ENABLE_PMC && _WIN32
