#pragma once
// pmc_event_set.hpp -- DIE EINE Liste der Hardware-Events, die im Haus real geoeffnet werden.
//
// WOZU SIE EXISTIERT: die Host-Probe des Planers (profile_facade/planner/pmc_host_probe.hpp) muss GENAU
// die Events pruefen, die die reale Quelle (builder/linux_perf_pmc_source.hpp) spaeter oeffnet -- sonst
// beweist sie etwas anderes, als sie behauptet. Eine ABSCHRIFT der vier Event-Kodierungen in der Probe
// waere die klassische zweite Wahrheit: ein Event-Wechsel an der Quelle liesse die Probe still auf dem
// alten Satz weiterpruefen, und der Planer entschiede die PMC-Permutation an einer Hardware-Eigenschaft,
// die im Messlauf gar nicht mehr abgefragt wird. Deshalb steht der Satz hier EINMAL und beide Seiten
// konsultieren ihn (ABSCHRIFT SCHLAEGT LOESCHUNG: die Quelle behaelt ihre Struktur, sie liest nur ihre
// Konstanten von hier).
//
// ABGRENZUNG SEIT #82 (I-PMC-3, 2026-08-21): diese Liste traegt die ENTSCHEIDUNGS-Events -- die, an denen
// die Planer-Probe die Lage (PMC einbauen ja/nein) festmacht. Die MODELL-GEBUNDENEN RAW-Events fuer
// L2/coherence stehen daneben in pmc_raw_event_katalog.hpp: sie sind eine additive Erweiterung UNTER dem
// bereits entschiedenen Flag, mit eigener per-Feld-Ehrlichkeit zur Laufzeit, und gehen NICHT in die
// Lage-Entscheidung ein (ein Host, dessen Modell keinen Katalog-Eintrag hat, misst weiter alles
// Generische). Der Satz oben ("GENAU die Events") gilt woertlich fuer DIESE Liste.
//
// WARUM ER NICHT UNTER COMDARE_ENABLE_PMC STEHT: die Probe wird IMMER uebersetzt. Sie ist der Grund, aus
// dem das Makro spaeter gesetzt oder nicht gesetzt wird -- stuende sie unter dem Makro, koennte sie ihre
// eigene Voraussetzung nie erzeugen (die Henne-Ei-Falle der bisherigen Lage, in der der Planer das Flag
// UNBEDINGT setzte, weil er nichts hatte, womit er haette entscheiden koennen).
//
// PLATTFORM: die Kodierungen sind Linux-perf-Kodierungen (man7 perf_event_open). Auf Nicht-Linux ist die
// Liste LEER -- und eine leere Liste laesst die Probe fail-closed auf "unbrauchbar" fallen, statt eine
// Verfuegbarkeit zu behaupten, die auf dieser Plattform gar nicht gepruefbar ist.
//
// header-only, C++23. Reine constexpr-Identitaet; kein Syscall, kein Zustand.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#if defined(__linux__)
#include <linux/perf_event.h> // PERF_TYPE_*, PERF_COUNT_*
#endif

namespace comdare::cache_engine::measurement {

/// EIN Hardware-Event in der man7-Anforderungsform: (type, config) plus sein Hausname.
/// Der Hausname ist derselbe, unter dem die Quelle diagnostiziert ("cache_misses_l1", ...) -- ein roter
/// Probe-Lauf und ein [PMC-DIAG]-Log sprechen damit dieselbe Sprache.
struct PmcEventSpec {
    std::string_view name;
    std::uint32_t    type   = 0;
    std::uint64_t    config = 0;
};

/// PERF_TYPE_HW_CACHE config-Kodierung (man7): config = cache_id | (op_id<<8) | (result_id<<16).
/// DIE EINE Kodierung -- detail_linux_perf::cache_cfg leitet hierher weiter.
[[nodiscard]] constexpr std::uint64_t pmc_cache_cfg(std::uint32_t id, std::uint32_t op, std::uint32_t res) noexcept {
    return static_cast<std::uint64_t>(id) | (static_cast<std::uint64_t>(op) << 8) |
           (static_cast<std::uint64_t>(res) << 16);
}

#if defined(__linux__)

/// Die VIER Events, die LinuxPerfPmcSource real oeffnet (Konstruktor, Stand M-3a 07.08.2026).
/// Reihenfolge == Oeffnungs-Reihenfolge der Quelle; die Indizes unten sind ihre Namen.
inline constexpr std::size_t kPmcEventCount = 4;

inline constexpr std::array<PmcEventSpec, kPmcEventCount> kPmcEvents{{
    {"cache_misses_l1", PERF_TYPE_HW_CACHE,
     pmc_cache_cfg(PERF_COUNT_HW_CACHE_L1D, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS)},
    {"cache_misses_l3_ll", PERF_TYPE_HW_CACHE,
     pmc_cache_cfg(PERF_COUNT_HW_CACHE_LL, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS)},
    {"dtlb_misses", PERF_TYPE_HW_CACHE,
     pmc_cache_cfg(PERF_COUNT_HW_CACHE_DTLB, PERF_COUNT_HW_CACHE_OP_READ, PERF_COUNT_HW_CACHE_RESULT_MISS)},
    // M-3a: ANDERE type-Klasse. PERF_COUNT_HW_BRANCH_MISSES ist ein PERF_TYPE_HARDWARE-Event; die config
    // ist der Event-Wert SELBST, keine cache_cfg-Kodierung. Ein pmc_cache_cfg()-Aufruf hier waere ein
    // stiller Fehler (falsche config -> EINVAL oder, schlimmer, ein anderes Event).
    {"branch_misses", PERF_TYPE_HARDWARE, static_cast<std::uint64_t>(PERF_COUNT_HW_BRANCH_MISSES)},
}};

// Index-Anker: die Quelle liest ihre Konstanten ueber DIESE Namen. Eine Umsortierung der Liste bricht hier
// compile-time, statt der Quelle still ein anderes Event unterzuschieben.
inline constexpr std::size_t kPmcEventIndexL1d    = 0;
inline constexpr std::size_t kPmcEventIndexLl     = 1;
inline constexpr std::size_t kPmcEventIndexDtlb   = 2;
inline constexpr std::size_t kPmcEventIndexBranch = 3;

static_assert(kPmcEvents.size() == kPmcEventCount, "kPmcEvents: Array-Groesse == kPmcEventCount (Anzahl-Anker).");
static_assert(kPmcEvents[kPmcEventIndexL1d].name == std::string_view{"cache_misses_l1"},
              "kPmcEvents: Index 0 ist der L1D-Read-Miss-Zaehler (Namen-Anker).");
static_assert(kPmcEvents[kPmcEventIndexLl].name == std::string_view{"cache_misses_l3_ll"},
              "kPmcEvents: Index 1 ist der Last-Level-Read-Miss-Zaehler (Namen-Anker).");
static_assert(kPmcEvents[kPmcEventIndexDtlb].name == std::string_view{"dtlb_misses"},
              "kPmcEvents: Index 2 ist der DTLB-Read-Miss-Zaehler (Namen-Anker).");
static_assert(kPmcEvents[kPmcEventIndexBranch].name == std::string_view{"branch_misses"},
              "kPmcEvents: Index 3 ist der Branch-Miss-Zaehler (Namen-Anker).");
static_assert(kPmcEvents[kPmcEventIndexBranch].type != kPmcEvents[kPmcEventIndexL1d].type,
              "kPmcEvents: der Branch-Zaehler MUSS unter einer anderen type-Klasse stehen als die "
              "Cache-Zaehler (M-3a) -- gleiche Klasse hiesse, jemand hat ihn als cache_cfg kodiert.");

#else // !__linux__

/// Nicht-Linux: LEERE Liste. Die Probe faellt damit fail-closed auf "unbrauchbar" -- sie kann auf dieser
/// Plattform nichts beweisen und behauptet deshalb nichts.
inline constexpr std::size_t                              kPmcEventCount = 0;
inline constexpr std::array<PmcEventSpec, kPmcEventCount> kPmcEvents{};

#endif // __linux__

} // namespace comdare::cache_engine::measurement
