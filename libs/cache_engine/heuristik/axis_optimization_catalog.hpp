#pragma once
// AXIS_ALGO_VERSION: 2
// heuristik/axis_optimization_catalog.hpp -- DER Min/Max-Katalog der Achsen-Optimierungsrichtungen ALS CODE.
//
// QUELLE (autoritativ, verbatim uebertragen -- NICHT erfunden, NICHT interpoliert):
//   super `docs/audits/20260709-axes-optimization-deep-research-BEFUND.md`
//   Abschnitt 1 "Uebersichtstabelle -- Achse -> Min/Max-Groesse -> Parameter-Art -> Last-Abhaengigkeit",
//   Zeilen 81-101 (Kopf :81/:82, Datenzeilen T0..T18 :83-:101). Die Pareto-Markierung stammt aus demselben
//   Dokument, Abschnitt 4 "Ehrliche Rest-Unsicherheiten" :450-453.
//
// WARUM ES DIESEN HEADER GIBT (der Befund, der ihn erzwingt):
//   break_even.hpp trug bis heute die Zeile "KONVENTION 'besser' = KLEINERER y-Wert" -- PAUSCHAL fuer ALLE
//   Achsen. Der Katalog sagt fuer 15 der 19 Achsen-Zeilen mindestens eine MAX-Zielgroesse. Fuer jede solche
//   Zielgroesse meldete der Break-Even-Finder damit die SCHLECHTERE Kurve als die bessere -- exakt umgekehrt.
//   Dieser Header ist die Single-Source, aus der break_even.hpp die Richtung je Zielgroesse bezieht.
//
// DIE ZENTRALE AUSSAGE DES KATALOGS (und der Grund fuer das Datenmodell hier):
//   Die Optimierungsrichtung ist KEINE Eigenschaft der ACHSE, sondern der ZIELGROESSE. Dieselbe Achse traegt
//   gegenlaeufige Groessen: T3 path_compression will MAX Kompressionsrate UND MIN Baumhoehe UND MIN MEM.
//   Eine Tabelle "Achse -> eine Richtung" waere eine Faelschung des Katalogs. Deshalb: flache Zielgroessen-
//   Tabelle (kAxisObjectives) + Achsen-Zeilen mit [begin,count)-Fenster (kAxisOptimizationCatalog).
//
// UEBERTRAGUNGS-REGEL (damit die Zaehlung nachvollziehbar und reproduzierbar ist):
//   (a) Jede mit MIN/MAX ausgezeichnete Groesse der Spalte "Optimierungs-Eigenschaft" wird EINE Zeile in
//       kAxisObjectives, in der Reihenfolge ihrer Nennung.
//   (b) Ein "=" im Katalog ist eine OPERATIONALISIERUNG derselben Groesse, keine zweite Groesse. Beispiel
//       T0 ":83": "MIN Lookup-Latenz = MIN (Baumhoehe x Cache-Lines/Knoten + Vergleiche)" -> EINE Zeile.
//       Ebenso T10 :93, T15 :98, T16 :99.
//   (c) "^" (Konjunktion) und "/" und "vs." trennen ECHTE zweite Groessen -> je eigene Zeile.
//   (d) Ein "->" bezeichnet eine Folge-Groesse, die der Katalog selbst mit MIN/MAX auszeichnet -> eigene
//       Zeile (T3 :86 "-> MIN MEM ^ CM", T12 :95 "-> MAX IPC").
//   Ergebnis dieser Regel am Ist-Katalog: 19 Achsen-Zeilen, 45 Zielgroessen, davon 17 MAX und 28 MIN.
//   15 der 19 Achsen tragen mindestens eine MAX-Groesse; ALLE 19 tragen mindestens eine MIN-Groesse.
//
// PARETO-SONDERBEHANDLUNG (BEFUND :450-453, woertlich): "Die Min/Max-Zuordnung ist bei multi-objektiven
//   Achsen (T6 Durchsatz vs. MEM; T18 Amplification vs. Latenz; T5 Space vs. Speed) KEINE einzelne Groesse,
//   sondern ein Pareto-Punkt ... Die Endauswertung sollte je Achse die *Objective* explizit machen, statt
//   eine falsche Einzel-Extremal-Groesse zu behaupten." Genau das erzwingt dieses Modul: fuer T5/T6/T18
//   verweigert die Achsen-Abkuerzung (find_break_even_points_of_axis) compile-time den Dienst; der Aufrufer
//   MUSS die Zielgroesse nennen.
//
// ABGRENZUNG ZU DEN KOMPOSITIONS-ACHSEN (der Katalog und der Code sind NICHT deckungsgleich -- ehrlich):
//   Der Katalog ist der Stand 2026-07-09 und fuehrt 19 Zeilen T0..T18 inklusive telemetry (T10) und isa (T12).
//   Beide haben die binary_id-permutierende Komposition seither VERLASSEN (Bau-INC-2c/2d, System-Achsen; s.
//   builder/experiment_tree/axis_path_serialization.hpp:22-28). Umgekehrt traegt die Komposition seit
//   STRUKT-R ORG-18 die 18. Achse "persistence_target", fuer die es im Katalog KEINE Zeile gibt.
//   Schnittmenge: 17 Namen. Der Drift-Beleg dieser 17+2+1-Rechnung liegt als static_assert im Test
//   (tests/unit/test_heuristik_spline_break_even.cpp) -- dort, weil er kCompositionAxisNames braucht und
//   dieser Header bewusst boost-frei bleibt.
//   OFFEN, NICHT ERFUNDEN: fuer persistence_target existiert KEINE Katalog-Zeile und damit hier KEINE
//   Richtung. catalog_axis_from_name("persistence_target") liefert honest-empty. Das ist ein Owner-Entscheid
//   (Katalog nachziehen), kein Bau-Rueckstand -- eine hier geratene Richtung waere genau der Fehler, den
//   dieses Modul beseitigt.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (analog
// measurement_tooling_registry.hpp / run_methodology_registry.hpp). Header-only, C++23, keine Fremdbibliothek.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::heuristik {

/// Die Optimierungsrichtung EINER Zielgroesse. "besser" heisst je nach Richtung kleiner ODER groesser.
enum class OptimizationDirection : std::uint8_t {
    Minimize = 0, ///< kleinerer Messwert ist besser (Latenz, Cache-Misses, Speicher, Fragmentierung, ...)
    Maximize = 1, ///< groesserer Messwert ist besser (Durchsatz, CLU, Kompressionsrate, IPC, Fanout, ...)
};

/// Die 19 Katalog-Achsen T0..T18 (BEFUND :83-:101). Enum-Wert == T-Nummer == Index in der Registry.
/// NICHT identisch mit den 18 Kompositions-Achsen (s. Kopf-Kommentar "ABGRENZUNG").
enum class CatalogAxis : std::uint8_t {
    SearchAlgo        = 0,  ///< T0
    CacheTraversal    = 1,  ///< T1
    Mapping           = 2,  ///< T2
    PathCompression   = 3,  ///< T3
    NodeType          = 4,  ///< T4
    MemoryLayout      = 5,  ///< T5  (Pareto)
    Allocator         = 6,  ///< T6  (Pareto)
    Prefetch          = 7,  ///< T7
    Concurrency       = 8,  ///< T8
    Serialization     = 9,  ///< T9
    Telemetry         = 10, ///< T10 (heute CEB-System-Achse, nicht mehr Komposition)
    ValueHandle       = 11, ///< T11
    Isa               = 12, ///< T12 (heute Target-ISA-System-Achse, nicht mehr Komposition)
    IndexOrganization = 13, ///< T13
    IoDispatch        = 14, ///< T14
    MigrationPolicy   = 15, ///< T15
    Filter            = 16, ///< T16
    QueuingQ1         = 17, ///< T17
    QueuingQ2         = 18, ///< T18 (Pareto)
};

// Single-Source: eine 20. Katalog-Zeile bricht hier compile-time (statt still 19 zu bleiben).
inline constexpr std::size_t kCatalogAxisCount = 19;
// Single-Source der Zielgroessen-Gesamtzahl nach der Uebertragungs-Regel (a)-(d) oben.
inline constexpr std::size_t kAxisObjectiveCount = 45;

/// EINE Zielgroesse einer Achse: was gemessen wird, und in welche Richtung "besser" zeigt.
// Default-Initialisierer nach dem Muster des heuristik-Bestands (axis_spline.hpp: `double x = 0.0;`).
// Sie aendern an der Aggregat-Initialisierung der constexpr-Tabellen nichts, machen aber jedes Feld
// auch bei einer kuenftigen Teil-Initialisierung bestimmt -- und halten cppcheck (uninitMemberVarNoCtor,
// Job lint:static) gruen. Die Direction-Vorgabe Minimize ist hier KEINE Semantik-Aussage: jede echte
// Zeile in kAxisObjectives nennt ihre Richtung verbatim, und die Vollzaehligkeits-static_asserts unten
// (17 MAX / 28 MIN / 45) wuerden ein stillschweigend defaultetes Feld sofort brechen.
struct AxisObjective {
    CatalogAxis           axis      = CatalogAxis::SearchAlgo; ///< die Achse der Groesse (Konsistenz-Anker)
    std::string_view      id        = {};                      ///< stabiler ASCII-Token, EINDEUTIG INNERHALB der Achse
    OptimizationDirection direction = OptimizationDirection::Minimize; ///< verbatim aus dem Katalog
    std::string_view      quantity  = {}; ///< die Katalog-Formulierung (ASCII), Rueckverfolgung
};

/// Die flache Zielgroessen-Tabelle. Reihenfolge = Nennungs-Reihenfolge im Katalog; das ERSTE Element je Achse
/// ist die FUEHRENDE Zielgroesse (leading_objective) -- die im Katalog zuerst genannte, nicht eine gewaehlte.
inline constexpr std::array<AxisObjective, kAxisObjectiveCount> kAxisObjectives{{
    // -- T0 search_algo (:83) ----------------------------------------------------------------------
    {CatalogAxis::SearchAlgo, "lookup_latency", OptimizationDirection::Minimize,
     "Lookup-Latenz = Baumhoehe x Cache-Lines/Knoten + Vergleiche"},
    {CatalogAxis::SearchAlgo, "branch_misses", OptimizationDirection::Minimize, "Branch-Miss (SIMD/branch-free)"},
    // -- T1 cache_traversal (:84) ------------------------------------------------------------------
    {CatalogAxis::CacheTraversal, "cache_misses_per_traversal", OptimizationDirection::Minimize,
     "Cache-Misses/Memory-Transfers je Traversal (cache-oblivious O(log_B N))"},
    {CatalogAxis::CacheTraversal, "cache_line_utilization", OptimizationDirection::Maximize, "CLU"},
    // -- T2 mapping (:85) --------------------------------------------------------------------------
    {CatalogAxis::Mapping, "indirection_cache_misses", OptimizationDirection::Minimize,
     "Indirektions-Cache-Misses je Aufloesung (Hash-Redirect O(logN)->O(1))"},
    {CatalogAxis::Mapping, "cache_line_utilization", OptimizationDirection::Maximize,
     "Permuter: CLU + atomarer Reorder"},
    // -- T3 path_compression (:86) -----------------------------------------------------------------
    {CatalogAxis::PathCompression, "compression_ratio", OptimizationDirection::Maximize,
     "Kompressionsrate (Single-Child-Kollaps)"},
    {CatalogAxis::PathCompression, "tree_height", OptimizationDirection::Minimize, "Baumhoehe/Pointer-Indirektionen"},
    {CatalogAxis::PathCompression, "memory_footprint", OptimizationDirection::Minimize, "MEM und CM"},
    // -- T4 node_type (:87) ------------------------------------------------------------------------
    {CatalogAxis::NodeType, "node_memory_waste", OptimizationDirection::Minimize,
     "Knoten-Speicher-Verschnitt (adaptiv ~52 B/Key)"},
    {CatalogAxis::NodeType, "fanout_utilization", OptimizationDirection::Maximize, "Fanout/CLU"},
    // -- T5 memory_layout (:88) -- PARETO (Space vs. Speed, :450-453) ------------------------------
    {CatalogAxis::MemoryLayout, "cache_line_utilization", OptimizationDirection::Maximize, "Cache-Line-Auslastung"},
    {CatalogAxis::MemoryLayout, "cache_misses_per_node_access", OptimizationDirection::Minimize,
     "CM je Knotenzugriff (CSB+: Pointer-Elimination)"},
    {CatalogAxis::MemoryLayout, "memory_footprint", OptimizationDirection::Minimize, "LOUDS: MEM"},
    // -- T6 allocator (:89) -- PARETO (Durchsatz vs. MEM, :450-453) --------------------------------
    {CatalogAxis::Allocator, "alloc_throughput", OptimizationDirection::Maximize,
     "Alloc-Durchsatz/Multicore-Skalierung"},
    {CatalogAxis::Allocator, "fragmentation", OptimizationDirection::Minimize, "Fragmentierung/MEM"},
    {CatalogAxis::Allocator, "p99_latency", OptimizationDirection::Minimize, "p99-Latenz"},
    // -- T7 prefetch (:90) -------------------------------------------------------------------------
    {CatalogAxis::Prefetch, "effective_memory_latency", OptimizationDirection::Minimize,
     "effektive Memory-Latenz (Miss-Latenz verdecken)"},
    {CatalogAxis::Prefetch, "memory_level_parallelism", OptimizationDirection::Maximize, "MLP/IPC"},
    // -- T8 concurrency (:91) ----------------------------------------------------------------------
    {CatalogAxis::Concurrency, "multicore_throughput", OptimizationDirection::Maximize,
     "Multicore-Durchsatz/Skalierung"},
    {CatalogAxis::Concurrency, "coherence_traffic", OptimizationDirection::Minimize, "Kohaerenz-Traffic/Contention"},
    // -- T9 serialization (:92) --------------------------------------------------------------------
    {CatalogAxis::Serialization, "compression_ratio", OptimizationDirection::Maximize, "Kompressionsrate"},
    {CatalogAxis::Serialization, "serialized_size", OptimizationDirection::Minimize,
     "serialisierte Groesse (succinct ~10 bit/Knoten)"},
    {CatalogAxis::Serialization, "decode_latency", OptimizationDirection::Minimize, "Decode-Latenz"},
    // -- T10 telemetry (:93) -----------------------------------------------------------------------
    {CatalogAxis::Telemetry, "observer_perturbation", OptimizationDirection::Minimize,
     "Observer-Perturbation = durch die Messung zugefuegte Kohaerenz-Invalidierungen (False-Sharing)"},
    // -- T11 value_handle (:94) --------------------------------------------------------------------
    {CatalogAxis::ValueHandle, "value_indirection", OptimizationDirection::Minimize,
     "Wert-Indirektion (Inline); Optimum = Schwelle auf Wertgroesse"},
    {CatalogAxis::ValueHandle, "node_density", OptimizationDirection::Maximize,
     "Knoten-Dichte/CLU (External); Optimum = Schwelle auf Wertgroesse"},
    // -- T12 isa (:95) -----------------------------------------------------------------------------
    {CatalogAxis::Isa, "data_parallelism", OptimizationDirection::Maximize,
     "Daten-Parallelitaet/Instruktion (breitere SIMD)"},
    {CatalogAxis::Isa, "ipc", OptimizationDirection::Maximize, "IPC"},
    {CatalogAxis::Isa, "instructions_per_lookup", OptimizationDirection::Minimize, "Instruktionen/Lookup"},
    {CatalogAxis::Isa, "branch_misses", OptimizationDirection::Minimize, "BM"},
    // -- T13 index_organization (:96) --------------------------------------------------------------
    {CatalogAxis::IndexOrganization, "storage_order_locality", OptimizationDirection::Maximize,
     "Scan: Storage-Order-Lokalitaet (clustered)"},
    {CatalogAxis::IndexOrganization, "redundancy", OptimizationDirection::Minimize,
     "Point+Sekundaer: Redundanz (non-clustered)"},
    // -- T14 io_dispatch (:97) ---------------------------------------------------------------------
    {CatalogAxis::IoDispatch, "io_throughput", OptimizationDirection::Maximize,
     "I/O-Durchsatz + Kontrolle (async/evict)"},
    {CatalogAxis::IoDispatch, "page_fault_overhead", OptimizationDirection::Minimize,
     "Page-Fault/TLB-Shootdown-Overhead (mmap-Antipattern)"},
    // -- T15 migration_policy (:98) ----------------------------------------------------------------
    {CatalogAxis::MigrationPolicy, "tier_weighted_latency", OptimizationDirection::Minimize,
     "durchschn. tier-gewichtete Latenz = Cold-Tier-Zugriffe je Speicherbudget"},
    {CatalogAxis::MigrationPolicy, "eviction_regret", OptimizationDirection::Minimize,
     "adaptiv (LeCaR): Eviction-Regret"},
    // -- T16 filter (:99) --------------------------------------------------------------------------
    {CatalogAxis::Filter, "false_positive_rate_per_bit", OptimizationDirection::Minimize,
     "False-Positive-Rate/Bit = bits/Key bei Ziel-FPR"},
    {CatalogAxis::Filter, "probe_memory_accesses", OptimizationDirection::Minimize,
     "Filter-Probe-Memory-Zugriffe (Xor=3, Cuckoo=2)"},
    // -- T17 queuing_q1 (:100) ---------------------------------------------------------------------
    {CatalogAxis::QueuingQ1, "buffer_throughput", OptimizationDirection::Maximize, "Puffer-Durchsatz/Batching"},
    {CatalogAxis::QueuingQ1, "buffer_memory", OptimizationDirection::Minimize, "Puffer-MEM/Drain-Latenz"},
    {CatalogAxis::QueuingQ1, "producer_consumer_parallelism", OptimizationDirection::Maximize,
     "lock-free: Producer/Consumer-Parallelitaet"},
    // -- T18 queuing_q2 (:101) -- PARETO (Amplification vs. Latenz, :450-453; Katalog nennt XOR) ----
    {CatalogAxis::QueuingQ2, "write_amplification", OptimizationDirection::Minimize, "Write-Amplification (lazy)"},
    {CatalogAxis::QueuingQ2, "batching", OptimizationDirection::Maximize, "Batching (lazy)"},
    {CatalogAxis::QueuingQ2, "staleness_tail_latency", OptimizationDirection::Minimize,
     "Staleness/Tail-Latenz (eager)"},
}};

/// Eine Katalog-Achsen-Zeile. `name` ist der Achsen-Token, der -- wo vorhanden -- byte-gleich zum
/// Kompositions-Achsen-Namen ist (kCompositionAxisNames); `source_line` zeigt auf die BEFUND-Zeile.
// Default-Initialisierer wie bei AxisObjective oben -- selbe Begruendung, selbe Unschaedlichkeit:
// die Kachelungs-static_asserts (lueckenlos, ueberlappungsfrei, 19 Achsen) brechen bei einem
// stillschweigend defaulteten Fenster sofort.
struct AxisOptimizationInfo {
    CatalogAxis      axis            = CatalogAxis::SearchAlgo;
    std::string_view t_id            = {};    ///< "T0".."T18" -- die T-Nummer des Katalogs
    std::string_view name            = {};    ///< Achsen-Token ("search_algo", ...)
    std::uint16_t    source_line     = 0;     ///< Zeile in docs/audits/20260709-axes-optimization-...-BEFUND.md
    std::string_view mess_kategorien = {};    ///< Spalte "Haupt-Mess-Kat." (verbatim)
    std::string_view parameter_art   = {};    ///< Spalte "Parameter-Art" (verbatim, ASCII-transliteriert)
    std::size_t      objective_begin = 0;     ///< Index der ersten Zielgroesse in kAxisObjectives
    std::size_t      objective_count = 0;     ///< Anzahl der Zielgroessen dieser Achse
    bool             pareto          = false; ///< BEFUND :450-453: keine EINZELNE Extremal-Groesse
};

/// Die EINE Registry der Katalog-Achsen -- Index == CatalogAxis-Wert (static_assert-gesichert).
inline constexpr std::array<AxisOptimizationInfo, kCatalogAxisCount> kAxisOptimizationCatalog{{
    {CatalogAxis::SearchAlgo, "T0", "search_algo", 83, "LAT, THR, CM, BM, IPC", "CT (Binary-Wahl)", 0, 2, false},
    {CatalogAxis::CacheTraversal, "T1", "cache_traversal", 84, "CM, CLU, dTLB, LAT", "RC-Phantom batch_size", 2, 2,
     false},
    {CatalogAxis::Mapping, "T2", "mapping", 85, "CM, CLU, LAT, BM", "CT", 4, 2, false},
    {CatalogAxis::PathCompression, "T3", "path_compression", 86, "MEM, CM, LAT, CLU", "CT (Binary-Wahl)", 6, 3, false},
    {CatalogAxis::NodeType, "T4", "node_type", 87, "MEM, CLU, CM, LAT", "CT", 9, 2, false},
    {CatalogAxis::MemoryLayout, "T5", "memory_layout", 88, "CLU, CM, MEM, dTLB", "CT (5 RepresentationKind)", 11, 3,
     true},
    {CatalogAxis::Allocator, "T6", "allocator", 89, "THR, LAT(p99), MEM, EN", "RC-Phantom pool_budget_bytes", 14, 3,
     true},
    {CatalogAxis::Prefetch, "T7", "prefetch", 90, "LAT, CM, IPC, EN", "RC verdrahtet prefetch_distance (+MSR hw_pf)",
     17, 2, false},
    {CatalogAxis::Concurrency, "T8", "concurrency", 91, "THR, LAT(tail), CM/CLU", "RC-Phantom thread_count", 19, 2,
     false},
    {CatalogAxis::Serialization, "T9", "serialization", 92, "MEM, LAT, THR", "CT", 21, 3, false},
    {CatalogAxis::Telemetry, "T10", "telemetry", 93, "CM/CLU, THR/LAT, IPC", "CT", 24, 1, false},
    {CatalogAxis::ValueHandle, "T11", "value_handle", 94, "CM, CLU, MEM, LAT", "RC-Phantom inline_threshold_bytes", 25,
     2, false},
    {CatalogAxis::Isa, "T12", "isa", 95, "IPC, THR, BM, EN", "CT (Thesis: Compile-Time-Achse)", 27, 4, false},
    {CatalogAxis::IndexOrganization, "T13", "index_organization", 96, "CM/CLU, LAT, THR, MEM", "CT", 31, 2, false},
    {CatalogAxis::IoDispatch, "T14", "io_dispatch", 97, "LAT, THR, dTLB, CM", "CT", 33, 2, false},
    {CatalogAxis::MigrationPolicy, "T15", "migration_policy", 98, "LAT, CM, MEM, THR, EN",
     "CT+Verhalten (Laufzeit-Entscheidung, keine RC)", 35, 2, false},
    {CatalogAxis::Filter, "T16", "filter", 99, "MEM, CM, LAT, THR", "CT (+bits/Key-Config)", 37, 2, false},
    {CatalogAxis::QueuingQ1, "T17", "queuing_q1", 100, "THR, LAT, MEM, CLU", "CT+asp (Kapazitaet)", 39, 3, false},
    {CatalogAxis::QueuingQ2, "T18", "queuing_q2", 101, "THR, LAT, MEM, EN", "CT+asp (Threshold)", 42, 3, true},
}};

namespace detail {

/// Vollzaehligkeit + Eindeutigkeit der Achsen-Zeilen: Index==Enum, Token nie leer, Namen/T-ids eindeutig.
[[nodiscard]] consteval bool catalog_axes_are_complete_and_unique() {
    for (std::size_t i = 0; i < kCatalogAxisCount; ++i) {
        AxisOptimizationInfo const& a = kAxisOptimizationCatalog[i];
        if (static_cast<std::size_t>(a.axis) != i) return false;
        if (a.t_id.empty() || a.name.empty()) return false;
        if (a.mess_kategorien.empty() || a.parameter_art.empty()) return false;
        if (a.source_line == 0) return false;
        for (std::size_t j = i + 1; j < kCatalogAxisCount; ++j) {
            if (a.name == kAxisOptimizationCatalog[j].name) return false;
            if (a.t_id == kAxisOptimizationCatalog[j].t_id) return false;
        }
    }
    return true;
}

/// LUECKENLOSIGKEIT der [begin,count)-Fenster: sie kacheln kAxisObjectives exakt, ohne Loch und ohne
/// Ueberlappung. Damit kann keine Zielgroesse verwaisen und keine doppelt zugeordnet werden.
[[nodiscard]] consteval bool objective_windows_tile_exactly() {
    std::size_t cursor = 0;
    for (std::size_t i = 0; i < kCatalogAxisCount; ++i) {
        AxisOptimizationInfo const& a = kAxisOptimizationCatalog[i];
        if (a.objective_begin != cursor) return false;
        if (a.objective_count == 0) return false; // jede Achse hat mindestens EINE Zielgroesse
        cursor += a.objective_count;
        if (cursor > kAxisObjectiveCount) return false;
    }
    return cursor == kAxisObjectiveCount;
}

/// Konsistenz + Eindeutigkeit der Zielgroessen: jede sitzt im Fenster ihrer eigenen Achse, ids nie leer,
/// ids EINDEUTIG innerhalb der Achse (ueber Achsen hinweg duerfen sie sich wiederholen -- "compression_ratio"
/// gibt es bei T3 und T9, "cache_line_utilization" bei T1/T2/T5).
[[nodiscard]] consteval bool objectives_are_consistent_and_unique() {
    for (std::size_t i = 0; i < kCatalogAxisCount; ++i) {
        AxisOptimizationInfo const& a = kAxisOptimizationCatalog[i];
        for (std::size_t o = a.objective_begin; o < a.objective_begin + a.objective_count; ++o) {
            if (kAxisObjectives[o].axis != a.axis) return false;
            if (kAxisObjectives[o].id.empty() || kAxisObjectives[o].quantity.empty()) return false;
            for (std::size_t p = o + 1; p < a.objective_begin + a.objective_count; ++p)
                if (kAxisObjectives[o].id == kAxisObjectives[p].id) return false;
        }
    }
    return true;
}

/// Zaehl-Anker: Drift einer Richtung (MIN<->MAX) bricht hier compile-time, auch wenn die Zeilenzahl stimmt.
[[nodiscard]] consteval std::size_t count_direction(OptimizationDirection d) {
    std::size_t n = 0;
    for (AxisObjective const& o : kAxisObjectives)
        if (o.direction == d) ++n;
    return n;
}

/// Wie viele Achsen tragen mindestens eine MAX-Groesse -- die Zahl, die den Befund traegt.
[[nodiscard]] consteval std::size_t count_axes_with_maximize() {
    std::size_t n = 0;
    for (std::size_t i = 0; i < kCatalogAxisCount; ++i) {
        AxisOptimizationInfo const& a = kAxisOptimizationCatalog[i];
        for (std::size_t o = a.objective_begin; o < a.objective_begin + a.objective_count; ++o)
            if (kAxisObjectives[o].direction == OptimizationDirection::Maximize) {
                ++n;
                break;
            }
    }
    return n;
}

[[nodiscard]] consteval std::size_t count_pareto_axes() {
    std::size_t n = 0;
    for (AxisOptimizationInfo const& a : kAxisOptimizationCatalog)
        if (a.pareto) ++n;
    return n;
}

} // namespace detail

static_assert(kAxisOptimizationCatalog.size() == kCatalogAxisCount,
              "kAxisOptimizationCatalog: Array-Groesse == kCatalogAxisCount (Anzahl-Anker).");
static_assert(kAxisObjectives.size() == kAxisObjectiveCount,
              "kAxisObjectives: Array-Groesse == kAxisObjectiveCount (Anzahl-Anker).");
static_assert(detail::catalog_axes_are_complete_and_unique(),
              "kAxisOptimizationCatalog: 19 Zeilen, Index==CatalogAxis, Token nie leer, Name/T-id eindeutig.");
static_assert(detail::objective_windows_tile_exactly(),
              "kAxisOptimizationCatalog: die [begin,count)-Fenster kacheln kAxisObjectives luecken- und "
              "ueberlappungsfrei (keine verwaiste, keine doppelte Zielgroesse).");
static_assert(detail::objectives_are_consistent_and_unique(),
              "kAxisObjectives: jede Zielgroesse sitzt im Fenster ihrer Achse, id nicht leer, id je Achse eindeutig.");
// Zahlen-Anker der Uebertragung (BEFUND :83-:101 nach der Regel (a)-(d) im Kopf): 17 MAX / 28 MIN / 45 gesamt.
// Wer eine Richtung dreht oder eine Zeile ergaenzt, muss diese Zahlen BEWUSST mitziehen.
static_assert(detail::count_direction(OptimizationDirection::Maximize) == 17,
              "kAxisObjectives: 17 MAX-Zielgroessen (BEFUND :83-:101).");
static_assert(detail::count_direction(OptimizationDirection::Minimize) == 28,
              "kAxisObjectives: 28 MIN-Zielgroessen (BEFUND :83-:101).");
static_assert(detail::count_direction(OptimizationDirection::Maximize) +
                      detail::count_direction(OptimizationDirection::Minimize) ==
                  kAxisObjectiveCount,
              "kAxisObjectives: jede Zielgroesse traegt genau eine Richtung.");
// DER BEFUND, der dieses Modul erzwingt: 15 der 19 Achsen tragen eine MAX-Groesse -- fuer sie war die
// pauschale "kleiner ist besser"-Konvention von break_even.hpp falsch herum.
static_assert(detail::count_axes_with_maximize() == 15,
              "kAxisOptimizationCatalog: 15 der 19 Achsen tragen mindestens eine MAX-Zielgroesse.");
static_assert(detail::count_pareto_axes() == 3, "kAxisOptimizationCatalog: genau 3 Pareto-Achsen (BEFUND :450-453).");
static_assert(kAxisOptimizationCatalog[static_cast<std::size_t>(CatalogAxis::MemoryLayout)].pareto &&
                  kAxisOptimizationCatalog[static_cast<std::size_t>(CatalogAxis::Allocator)].pareto &&
                  kAxisOptimizationCatalog[static_cast<std::size_t>(CatalogAxis::QueuingQ2)].pareto,
              "Pareto-Sonderbehandlung gilt genau fuer T5 memory_layout / T6 allocator / T18 queuing_q2.");
// Namen-Anker: eine Umbenennung/Vertauschung der Achsen-Token bricht hier compile-time.
static_assert(kAxisOptimizationCatalog[static_cast<std::size_t>(CatalogAxis::PathCompression)].name ==
                      std::string_view{"path_compression"} &&
                  kAxisOptimizationCatalog[static_cast<std::size_t>(CatalogAxis::Allocator)].name ==
                      std::string_view{"allocator"} &&
                  kAxisOptimizationCatalog[static_cast<std::size_t>(CatalogAxis::QueuingQ2)].name ==
                      std::string_view{"queuing_q2"},
              "kAxisOptimizationCatalog: Achsen-Token sind die Kompositions-Namen (Namen-Anker).");

/// constexpr-Lookup der Achsen-Zeile (Index == CatalogAxis-Wert, per static_assert garantiert).
[[nodiscard]] constexpr AxisOptimizationInfo const& catalog_axis_info(CatalogAxis axis) noexcept {
    return kAxisOptimizationCatalog[static_cast<std::size_t>(axis)];
}

/// Die Zielgroessen EINER Achse, in Katalog-Reihenfolge.
[[nodiscard]] constexpr std::span<AxisObjective const> objectives_of(CatalogAxis axis) noexcept {
    AxisOptimizationInfo const& a = catalog_axis_info(axis);
    return std::span<AxisObjective const>{kAxisObjectives.data() + a.objective_begin, a.objective_count};
}

/// Die FUEHRENDE Zielgroesse: die im Katalog ZUERST genannte. Das ist eine Uebertragungs-Regel, keine
/// Wertung -- fuer Pareto-Achsen ist sie bewusst NICHT als "die" Zielgroesse zu lesen (s. requires_explicit_
/// objective) und die Achsen-Abkuerzung in break_even.hpp verweigert dort compile-time den Dienst.
[[nodiscard]] constexpr AxisObjective const& leading_objective(CatalogAxis axis) noexcept {
    return kAxisObjectives[catalog_axis_info(axis).objective_begin];
}

/// Richtung der fuehrenden Zielgroesse.
[[nodiscard]] constexpr OptimizationDirection leading_direction(CatalogAxis axis) noexcept {
    return leading_objective(axis).direction;
}

/// true, wenn der Katalog fuer diese Achse KEINE einzelne Extremal-Groesse zulaesst (BEFUND :450-453).
[[nodiscard]] constexpr bool requires_explicit_objective(CatalogAxis axis) noexcept {
    return catalog_axis_info(axis).pareto;
}

/// Weiche Suche einer Zielgroesse (honest-empty statt Rateweg).
[[nodiscard]] constexpr std::optional<AxisObjective> find_objective(CatalogAxis      axis,
                                                                    std::string_view objective_id) noexcept {
    for (AxisObjective const& o : objectives_of(axis))
        if (o.id == objective_id) return o;
    return std::nullopt;
}

/// HARTE Aufloesung fuer den Compile-Time-Gebrauch: eine unbekannte Zielgroesse ist ein Uebersetzungs-Fehler,
/// KEIN stiller Default. Genau das verhindert, dass eine nicht katalogisierte Groesse pauschal als MIN faehrt.
[[nodiscard]] consteval OptimizationDirection objective_direction(CatalogAxis axis, std::string_view objective_id) {
    for (AxisObjective const& o : objectives_of(axis))
        if (o.id == objective_id) return o.direction;
    throw "axis_optimization_catalog: unbekannte Zielgroesse fuer diese Achse -- Katalog-Token nennen "
          "(kAxisObjectives) oder den Katalog erst erweitern; NIE eine Richtung raten.";
}

/// Achse aus dem Achsen-Token. honest-empty fuer Namen ohne Katalog-Zeile (z.B. "persistence_target").
[[nodiscard]] constexpr std::optional<CatalogAxis> catalog_axis_from_name(std::string_view name) noexcept {
    for (AxisOptimizationInfo const& a : kAxisOptimizationCatalog)
        if (a.name == name) return a.axis;
    return std::nullopt;
}

/// Compile-time-Iteration ueber die Katalog-Achsen (Metaprogrammierungs-Interface, analog
/// for_each_run_methodology). KEIN Runtime-Switch: der Visitor wird je Zeile statisch instanziiert.
template <class Visitor>
constexpr void for_each_catalog_axis(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kAxisOptimizationCatalog[I]), ...);
    }(std::make_index_sequence<kCatalogAxisCount>{});
}

/// Compile-time-Iteration ueber ALLE Zielgroessen (flach, Achsen-geordnet).
template <class Visitor>
constexpr void for_each_axis_objective(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kAxisObjectives[I]), ...);
    }(std::make_index_sequence<kAxisObjectiveCount>{});
}

} // namespace comdare::cache_engine::heuristik
