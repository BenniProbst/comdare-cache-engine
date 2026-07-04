#pragma once
// V5-#26-SUBSTANZ — Mess-Quellen-Abstraktion für die 6 HW-Counter (PMC) des 16+6-Mess-POD.
//
// Re-Audit-Blocker 2: die „+6"-HW-Spalten (cache_misses L1/L2/L3, dtlb_misses, coherence_invalidations,
// energy) waren hartkodiert 0. Dieses Interface ist die SOFTWARE-Architektur, die sie speist — pluggable
// Mess-Quelle mit EHRLICHER Verfügbarkeits-Meldung (`available()`). Die REALEN Werte brauchen Intel PCM /
// RDPMC / RAPL-MSR (Vendor-Lib + Admin/MSR-Treiber auf der i7-1270P) = Beschaffung/Recht (P4, extern);
// bis dahin liefert `NullPmcSource` available()=false (statt stiller 0). Wenn die Hardware-Quelle verfügbar
// ist, ist sie ein Drop-in (eine weitere IPmcSource-Implementierung) — KEINE Änderung an POD/Pipeline/PDF.
//
// @doku docs/sessions/20260531-mess-abstraktion-cross-platform-architektur-plan.md (I2 PMC) + _v5_i8…/POD #50

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder {

/// Die HW-Counter (Delta über ein Mess-Intervall) + Verfügbarkeits-Flag (= pmc_available im POD).
struct PmcCounters {
    std::uint64_t cache_misses_l1         = 0;
    std::uint64_t cache_misses_l2         = 0;
    std::uint64_t cache_misses_l3         = 0;
    std::uint64_t dtlb_misses             = 0;
    std::uint64_t branch_misses           = 0;
    std::uint64_t coherence_invalidations = 0;
    std::uint64_t energy_micro_joules     = 0;
    bool          available               = false; // false = NICHT real gemessen (ehrlich, kein Schein-0)
};

/// Pluggable HW-Performance-Counter-Quelle. begin()→Op-Lauf→end() liefert das Counter-Delta.
class IPmcSource {
public:
    virtual ~IPmcSource()                                   = default;
    virtual void                           begin() noexcept = 0; ///< Zähler-Start (Intervall-Beginn)
    [[nodiscard]] virtual PmcCounters      end() noexcept   = 0; ///< Intervall-Ende → Delta (available je Quelle)
    [[nodiscard]] virtual bool             available() const noexcept = 0;
    [[nodiscard]] virtual std::string_view name() const noexcept      = 0;
};

/// Default-Quelle, solange kein realer PMC angebunden ist: meldet EHRLICH „nicht verfügbar" (alle 0).
/// Ersetzt die früher hartkodierten 0-HW-Spalten durch ein explizites available=false (#26 / Re-Audit-Blocker 2).
class NullPmcSource final : public IPmcSource {
public:
    void                           begin() noexcept override {}
    [[nodiscard]] PmcCounters      end() noexcept override { return PmcCounters{}; } // available=false (Default)
    [[nodiscard]] bool             available() const noexcept override { return false; }
    [[nodiscard]] std::string_view name() const noexcept override { return "null-pmc (P4/HW-gated)"; }
};

} // namespace comdare::cache_engine::builder
