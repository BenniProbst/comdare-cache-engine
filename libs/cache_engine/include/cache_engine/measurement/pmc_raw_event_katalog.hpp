#pragma once
// measurement/pmc_raw_event_katalog.hpp -- #82 (I-PMC-3): DIE MODELL-GEBUNDENEN RAW-EVENTS, EINMAL.
//
// OWNER-GO (KON103 17.08.2026, verbatim): "C-2: Wir lassen nichts weg und machen NICHTS nach der Abgabe,
// es gibt also nur vor der Abgabe, volles Programm bitte." -- die Kernmetrik-Zusage (L3/L2/coherence/
// energy zusaetzlich zu L1D+dTLB) gilt VOLL; die Zuschneide-Empfehlung wurde ABGELEHNT.
//
// GEGENSTAND. Die 16.07.-Synthese (F3i/F4/F5/F9) definiert I-PMC-3: "L2 + coherence via PERF_TYPE_RAW je
// CPU-Modell -- erst damit alle 6 Spec-Felder real." Fuer L2 und coherence_invalidations gibt es KEINEN
// portablen generischen perf-Counter (linux_perf_pmc_source.hpp sagt es seit B5/M-2 woertlich); ein Wert
// existiert nur ueber rohe event/umask-Kodierungen, und die sind eine Eigenschaft der MIKROARCHITEKTUR.
// Deshalb steht hier ein KATALOG: (Vendor, cpuid-Family) -> Belegung. Ein Modell ohne Katalog-Eintrag
// liefert KEINE Belegung -- die Felder bleiben dann ehrlich "n/a" (per-Feld-Flag), NIE eine geratene
// Kodierung (die Kernel-Falle ist am Objekt gemessen: ein unbekannter PMU-Typ wird NICHT abgewiesen,
// sondern faellt still auf einen anderen PMU zurueck -- plausible falsche Zahlen).
//
// ZEN 5 (Family 0x1A / 26), AM OBJEKT BEWIESEN (prod1, AMD Ryzen 9 9950X3D = Granite Ridge, 2 CCX,
// Kernel 6.17.0-35-generic, perf 6.17; 2026-08-21):
//   * perf-JSON amdzen5: l2_cache_req_stat.ic_dc_miss_in_l2 = cpu/event=0x64,umask=0x9/
//     ("Core to L2 cache requests (not including L2 prefetch) for data and instruction cache misses")
//   * perf-JSON amdzen5: ls_dmnd_fills_from_sys.remote_cache = cpu/event=0x43,umask=0x14/
//     ("Demand data cache fills from cache of another CCX"; near_cache 0x04 + far_cache 0x10)
//   * KREUZPROBE raw gegen benannt (perf stat, identischer Pointer-Chase-Lauf):
//       r964  == l2_cache_req_stat.ic_dc_miss_in_l2       -> 17.314.023 == 17.314.023 (identisch)
//       r1443 == ls_dmnd_fills_from_sys.remote_cache      ->     21.503 ==     21.503 (identisch)
//   AMD-RAW-config-Layout (sysfs cpu/format/: event=config:0-7,32-35; umask=config:8-15):
//       0x964 = (umask 0x09 << 8) | event 0x64        0x1443 = (umask 0x14 << 8) | event 0x43
//
// SEMANTIK-OFFENLEGUNG (Pflicht; B-5-Kanon "verschiedene Semantik unter derselben Ueberschrift" --
// Verweis auf die MARKE, nicht auf eine Ledger-Zeilennummer, AnkerWache):
//   * cache_misses_l2 traegt auf Zen 5 die DEMAND-Misses in L2 (IC+DC, ohne L2-Prefetch), per-Task --
//     dieselbe Semantik-Klasse (per-Task, Miss-Zaehler) wie die generischen Nachbarn.
//   * coherence_invalidations traegt auf Zen 5 die DEMAND-Fills aus dem Cache eines ANDEREN CCX
//     (near+far). Das ist der Koharenz-BEOBACHTBARE des Core-PMU: eine Zeile, die ein anderer Kern
//     (typisch nach Invalidierung/Modifikation) haelt, wird per Cache-zu-Cache-Transfer geliefert --
//     das klassische Koharenz-Miss-Mass. Ein woertlicher "empfangene Invalidierungen"-Zaehler existiert
//     im amdzen5-Core-Event-Satz NICHT (perf list am Objekt: 0 Treffer invalid/probe/snoop). Der Anhang
//     fuehrt die Spalte mit genau dieser Zen-5-Belegung, nicht als woertliche Invalidierungszahl.
//
// WARUM FAMILY UND NICHT MODELL: perf selbst bindet die amdzen5-Eventtabelle an die FAMILY 0x1A (die
// Kodierungen sind PPR-Eigenschaften der Zen-5-Kernfamilie, nicht des SKU). Ein feinerer Schluessel
// wuerde Wissen behaupten, das die Quelle (PPR/perf-JSON) nicht so schneidet.
//
// WARUM DIE PLANER-PROBE DIESEN KATALOG NICHT MITENTSCHEIDET: die Lage-Entscheidung (PMC einbauen?)
// muss auf JEDEM Host dieselbe Frage stellen -- sie haengt an der EINEN generischen Liste
// (pmc_event_set.hpp). Die RAW-Belegung ist eine ADDITIVE, modell-gebundene Erweiterung UNTER dem
// bereits entschiedenen Flag, mit eigener per-Feld-Ehrlichkeit zur Laufzeit; ein Host, dessen generische
// Events beissen, dessen Modell aber keinen Katalog-Eintrag hat, misst weiter alles Generische und
// traegt fuer L2/coherence ehrlich "n/a". (Dokumentierte Verschiebung gegenueber dem Satz "die Probe
// prueft GENAU die Events der Quelle" -- er gilt woertlich fuer die ENTSCHEIDUNGS-Events.)
//
// INTEL: heute KEIN Eintrag. prod1 ist AMD; eine Intel-RAW-Belegung ohne Objekt-Kreuzprobe waere ein
// Rateversuch (NIE raten). Der Katalog ist additiv erweiterbar, sobald ein Intel-Host die Kreuzprobe
// liefert (Eintrag + static_assert nach demselben Muster).
//
// header-only, C++23. Reine constexpr-Identitaet; kein Syscall, kein Zustand. ASCII-only.

#include <cstdint>
#include <string_view>

#include <cache_engine/measurement/pmc_event_set.hpp> // PmcEventSpec (+ PERF_TYPE_RAW via linux/perf_event.h)

namespace comdare::cache_engine::measurement {

/// Die RAW-Belegung EINES CPU-Modells: die zwei Felder, die nur roh existieren. `vorhanden == false`
/// heisst "dieses Modell hat keine gepruefte Belegung" -- die Quelle oeffnet dann NICHTS Rohes.
struct PmcRawBelegung {
    bool         vorhanden = false;
    PmcEventSpec l2{};        ///< -> PmcCounters::cache_misses_l2
    PmcEventSpec coherence{}; ///< -> PmcCounters::coherence_invalidations (Belegung s. Kopf)
};

#if defined(__linux__)

/// Zen-5-Konstanten, benannt statt magisch (Herleitung + Kreuzprobe im Kopf).
inline constexpr std::uint16_t    kAmdFamilyZen5         = 26;     // cpuid family 0x1A
inline constexpr std::uint64_t    kZen5RawL2DemandMiss   = 0x964;  // event 0x64, umask 0x09
inline constexpr std::uint64_t    kZen5RawFillsRemoteCcx = 0x1443; // event 0x43, umask 0x14
inline constexpr std::string_view kPmcRawVendorAmdCpuid  = "AuthenticAMD";

/// DER KATALOG. Schluessel = (cpuid-Vendor-String, cpuid-Family). Unbekannt => vorhanden=false.
[[nodiscard]] constexpr PmcRawBelegung pmc_raw_belegung_fuer(std::string_view cpuid_vendor,
                                                             std::uint16_t    family) noexcept {
    if (cpuid_vendor == kPmcRawVendorAmdCpuid && family == kAmdFamilyZen5) {
        PmcRawBelegung b;
        b.vorhanden = true;
        b.l2        = PmcEventSpec{"cache_misses_l2_raw_zen5", PERF_TYPE_RAW, kZen5RawL2DemandMiss};
        b.coherence = PmcEventSpec{"coherence_fills_remote_ccx_raw_zen5", PERF_TYPE_RAW, kZen5RawFillsRemoteCcx};
        return b;
    }
    return {};
}

// -- Selbst-Beweise (compile-time): der Katalog traegt genau das Bewiesene und behauptet nichts weiter. --
static_assert(pmc_raw_belegung_fuer("AuthenticAMD", 26).vorhanden, "Zen 5 (family 26) MUSS belegt sein (#82).");
static_assert(pmc_raw_belegung_fuer("AuthenticAMD", 26).l2.config == 0x964,
              "Zen-5-L2-Kodierung ist die kreuzgeprobte (r964 == ic_dc_miss_in_l2).");
static_assert(pmc_raw_belegung_fuer("AuthenticAMD", 26).coherence.config == 0x1443,
              "Zen-5-Coherence-Kodierung ist die kreuzgeprobte (r1443 == remote_cache).");
static_assert(pmc_raw_belegung_fuer("AuthenticAMD", 26).l2.type == PERF_TYPE_RAW &&
                  pmc_raw_belegung_fuer("AuthenticAMD", 26).coherence.type == PERF_TYPE_RAW,
              "I-PMC-3 ist der PERF_TYPE_RAW-Weg -- eine andere type-Klasse waere ein anderes Paket.");
static_assert(!pmc_raw_belegung_fuer("AuthenticAMD", 25).vorhanden,
              "Zen 3/4 (family 25) ist NICHT kreuzgeprobt -> keine Belegung, kein Rateversuch.");
static_assert(!pmc_raw_belegung_fuer("GenuineIntel", 6).vorhanden,
              "Intel ist NICHT kreuzgeprobt (prod1 ist AMD) -> keine Belegung, kein Rateversuch.");
static_assert(!pmc_raw_belegung_fuer("", 0).vorhanden, "Unbekannt => leer (fail-closed).");

#else // !__linux__

/// Nicht-Linux: es gibt keinen perf_event_open-Weg -- der Katalog ist strukturell leer (fail-closed).
[[nodiscard]] constexpr PmcRawBelegung pmc_raw_belegung_fuer(std::string_view, std::uint16_t) noexcept { return {}; }

#endif // __linux__

} // namespace comdare::cache_engine::measurement
