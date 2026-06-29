#pragma once
// V41.F.6.1.R6.A — WorkloadConfig + WorkloadOp Pflicht-Typen
//
// User-Direktive 2026-05-27 (Doku 14 §52.9):
// "Der Workload-Driver wird von der CacheEngineBuilder verwendet, um je
//  Permutations-Binary verschiedene Workloads zu testen und zu dokumentieren.
//  Die Eigenschaften der zu testenden Workloads werden separat vor dem
//  Experiment ueber alle Permutations-Binaries konfiguriert und exakt in
//  Reihenfolge und Umfang fuer jede Binary wiederholt."
//
// Reproduzierbarkeit-Pflicht: Generator mit gleicher Config (insbesondere
// gleichem `seed`) liefert IDENTISCHE Sequenz fuer jede Permutations-Binary.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §53
// @task #715 V41.F.6.1.R6.A

#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::builder::workload_driver {

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadOpKind — Operations-Klassifizierung
// ─────────────────────────────────────────────────────────────────────────────

enum class WorkloadOpKind : std::uint8_t {
    Insert = 0, ///< (key, value) ins Container einfuegen
    Lookup = 1, ///< key suchen, optional value zurueckgeben
    Erase  = 2, ///< key entfernen
    Clear  = 3, ///< Container komplett leeren (selten, fuer Stress-Tests)
    // V5-#49-E/F (YCSB-Treue, Op-Set-Erweiterung):
    Scan            = 4, ///< YCSB-E Range-Scan: ab `key` die nächsten `value` (=scan_length) Records in Key-Reihenfolge
    ReadModifyWrite = 5  ///< YCSB-F: lookup(key) → modifizieren → insert(key, value) als EINE Op (kein ABI-Bedarf)
};

// ─────────────────────────────────────────────────────────────────────────────
// KeyDistribution — Key-Zugriffsverteilung (YCSB-Treue, Task #49 Pfad A)
// ─────────────────────────────────────────────────────────────────────────────
// YCSB = Yahoo! Cloud Serving Benchmark (Cooper, Silberstein, Tam, Ramakrishnan, Sears:
// "Benchmarking Cloud Serving Systems with YCSB", Proc. 1st ACM Symposium on Cloud Computing (SoCC),
// 2010, S. 143–154, doi:10.1145/1807128.1807152; OSS brianfrankcooper/YCSB).
// Das DEFINITIONSMERKMAL von YCSB ist die SKEWED Key-Verteilung (Default ZIPFIAN, theta≈0.99), bzw.
// "latest" für Workload D. Reine Uniform-Verteilung ist NICHT YCSB-konform. Zipfian-Algorithmus:
// Gray, Sundaresan, Englert, Baclawski, Weinberger: "Quickly Generating Billion-Record Synthetic
// Databases", SIGMOD 1994 (das von YCSB verwendete schnelle Inverse-CDF-Verfahren).
enum class KeyDistribution : std::uint8_t {
    Uniform = 0, ///< gleichverteilt [key_min, key_max] (nicht YCSB-konform; Default für Rückwärtskompat)
    Zipfian = 1, ///< Zipf-verteilt (YCSB Default: wenige heiße Keys; theta≈0.99) — YCSB A/B/C
    Latest  = 2  ///< Zipf, aber auf die ZULETZT eingefügten (höchsten) Keys verschoben — YCSB D
};

[[nodiscard]] constexpr std::string_view op_kind_name(WorkloadOpKind k) noexcept {
    switch (k) {
        case WorkloadOpKind::Insert: return "Insert";
        case WorkloadOpKind::Lookup: return "Lookup";
        case WorkloadOpKind::Erase: return "Erase";
        case WorkloadOpKind::Clear: return "Clear";
        case WorkloadOpKind::Scan: return "Scan";
        case WorkloadOpKind::ReadModifyWrite: return "ReadModifyWrite";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadOp — eine einzelne Operation in der Workload-Sequenz
// ─────────────────────────────────────────────────────────────────────────────

/// WorkloadOp — POD mit Operation-Kind + Daten.
///
/// Pro Op-Kind ist nur ein Teil relevant:
/// - Insert:          `key` + `value`
/// - Lookup:          `key` (value ignoriert)
/// - Erase:           `key` (value ignoriert)
/// - Clear:           alles ignoriert
/// - Scan:            `key` = Start-Key, `value` = scan_length (Anzahl zu lesender Records, YCSB-E)
/// - ReadModifyWrite: `key` + `value` (lookup(key) → modifizieren → insert(key, value), YCSB-F)
struct WorkloadOp {
    WorkloadOpKind kind;
    std::uint64_t  key;
    std::uint64_t  value;

    [[nodiscard]] friend constexpr bool operator==(WorkloadOp const&, WorkloadOp const&) noexcept = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadConfig — Konfiguration einer Workload-Sequenz
// ─────────────────────────────────────────────────────────────────────────────

/// WorkloadConfig — Eigenschaften der pro-Permutation wiederholten Workload.
///
/// Pflicht-Reproduzierbarkeit (User-Direktive):
/// - **Gleiche WorkloadConfig + gleiches num_operations → identische Sequenz**
///   ueber alle Permutations-Binaries
/// - `seed` ist die einzige Source-of-Randomness; deterministisch via xorshift64
///
/// Op-Mix-Pflicht: pct_insert + pct_lookup + pct_erase + pct_clear == 1.0.
/// Bei !=1.0 wird beim Generator-Konstruktor normalisiert (siehe
/// WorkloadGenerator).
struct WorkloadConfig {
    /// Seed der xorshift64-PRNG. Pflicht != 0 (xorshift64 ist undefiniert bei 0).
    std::uint64_t seed = 42;

    /// Anzahl der zu generierenden Operationen.
    std::size_t num_operations = 1000;

    /// Key-Range [key_min, key_max] inklusive. key_max muss > key_min sein.
    std::uint64_t key_min = 1;
    std::uint64_t key_max = 1'000'000;

    /// Op-Mix Wahrscheinlichkeiten. Generator normalisiert wenn Summe != 1.0.
    double pct_insert = 0.50;
    double pct_lookup = 0.40;
    double pct_erase  = 0.09;
    double pct_clear  = 0.01;
    /// V5-#49-E/F: YCSB-E (Range-Scan) + YCSB-F (Read-Modify-Write). Default 0.0 → rückwärtskompatibel
    /// (bestehende Profile A–D unverändert). scan_length_max = YCSB-E max Records je Scan (YCSB-Default 100).
    double        pct_scan        = 0.0;
    double        pct_rmw         = 0.0;
    std::uint64_t scan_length_max = 100;

    /// Key-Zugriffsverteilung (YCSB-Treue, #49). Default Uniform (rückwärtskompatibel); die YCSB-Profile
    /// setzen Zipfian (A/B/C) bzw. Latest (D). zipfian_theta = Skew-Parameter (YCSB-Default 0.99).
    KeyDistribution key_distribution = KeyDistribution::Uniform;
    double          zipfian_theta    = 0.99;

    /// CoCo-Trie-Direktive (P04, get_queries/QUERY_NOT_IN_SET_PERCENTAGE): Anteil [0,100] der Lookup-/Scan-Queries,
    /// die auf einen NICHT-geladenen Key ([key_max+1, 2*key_max]) zielen → garantierter Miss. Erlaubt den
    /// vergleichbaren Negativ-Query-Sweep {0,25,50,75,100}. Default 0 = alle Queries treffen geladene Keys.
    double negative_query_pct = 0.0;

    /// Identifikation fuer Mess-Protokoll-Ausgaben.
    std::string_view name = "DefaultMixedWorkload";

    /// Validation: Pre-Condition fuer WorkloadGenerator Konstruktion.
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        if (seed == 0) return false; // xorshift64 undefiniert
        if (num_operations == 0) return false;
        if (key_max <= key_min) return false;
        if (pct_insert < 0.0 || pct_lookup < 0.0 || pct_erase < 0.0 || pct_clear < 0.0 || pct_scan < 0.0 ||
            pct_rmw < 0.0)
            return false;
        if ((pct_insert + pct_lookup + pct_erase + pct_clear + pct_scan + pct_rmw) <= 0.0) return false;
        if (negative_query_pct < 0.0 || negative_query_pct > 100.0) return false;
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Vorgefertigte Workload-Profile (typische Mess-Reihen)
// ─────────────────────────────────────────────────────────────────────────────

/// Insert-Heavy: 80% Insert, 20% Lookup (Bulk-Load-Phase).
[[nodiscard]] constexpr WorkloadConfig make_insert_heavy(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed           = seed,
                          .num_operations = ops,
                          .key_min        = 1,
                          .key_max        = 1'000'000,
                          .pct_insert     = 0.80,
                          .pct_lookup     = 0.20,
                          .pct_erase      = 0.0,
                          .pct_clear      = 0.0,
                          .name           = "InsertHeavy_80_20"};
}

/// Lookup-Heavy: 95% Lookup, 5% Insert (Read-dominant — typisch fuer
/// Index-Benchmarks nach Bulk-Load).
[[nodiscard]] constexpr WorkloadConfig make_lookup_heavy(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed           = seed,
                          .num_operations = ops,
                          .key_min        = 1,
                          .key_max        = 1'000'000,
                          .pct_insert     = 0.05,
                          .pct_lookup     = 0.95,
                          .pct_erase      = 0.0,
                          .pct_clear      = 0.0,
                          .name           = "LookupHeavy_95_5"};
}

/// Mixed-A: 50/50 Insert/Lookup (YCSB-A-aehnlich).
[[nodiscard]] constexpr WorkloadConfig make_mixed_a(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed             = seed,
                          .num_operations   = ops,
                          .key_min          = 1,
                          .key_max          = 1'000'000,
                          .pct_insert       = 0.50,
                          .pct_lookup       = 0.50,
                          .pct_erase        = 0.0,
                          .pct_clear        = 0.0,
                          .key_distribution = KeyDistribution::Zipfian, // YCSB-A: Zipf-Skew (Definitionsmerkmal)
                          .name             = "MixedA_50_50"};
}

/// Mixed-B: 5/95 Insert/Lookup (YCSB-B-aehnlich, read-heavy mit kleinem write-share).
[[nodiscard]] constexpr WorkloadConfig make_mixed_b(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed             = seed,
                          .num_operations   = ops,
                          .key_min          = 1,
                          .key_max          = 1'000'000,
                          .pct_insert       = 0.05,
                          .pct_lookup       = 0.95,
                          .pct_erase        = 0.0,
                          .pct_clear        = 0.0,
                          .key_distribution = KeyDistribution::Zipfian, // YCSB-B: Zipf-Skew, read-heavy
                          .name             = "MixedB_5_95"};
}

// ─────────────────────────────────────────────────────────────────────────────
// YCSB-TREUE — Stand nach #49 Pfad A (User-Direktive „müssen treu einbinden")
// ─────────────────────────────────────────────────────────────────────────────
// Die make_ycsb_*-Profile sind jetzt YCSB-TREU bzgl. Op-Mix UND Key-VERTEILUNG (das Definitionsmerkmal):
// A/B/C = Zipfian (theta=0.99, YCSB-Default), D = Latest. Vollzitat: Cooper et al., ACM SoCC 2010 (s.
// KeyDistribution-Doku oben). VERBLEIBENDE, EHRLICH offene Treue-Lücken (brauchen Op-Set-/Tier-Interface-
// Erweiterung, separater Punkt — NICHT als Fake-Proxy erfunden, [[feedback_no_quick_fixes]]):
//   - "update" (A/B) wird auf Insert gemappt; tier_insert=emplace ≠ Overwrite (kein Update-Op-Kind).
//   - YCSB-E (95% Range-SCAN) + YCSB-F (Read-Modify-Write) brauchen Scan- bzw. RMW-Op + entsprechende
//     Tier-Interface-Methoden (existieren noch nicht) → bewusst NICHT als A–D-Profile vorgetäuscht.

/// YCSB-C: 100% Lookup (Read-Only), Zipfian-Verteilung. Op-Mix + Verteilung YCSB-C-treu.
[[nodiscard]] constexpr WorkloadConfig make_ycsb_c(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed             = seed,
                          .num_operations   = ops,
                          .key_min          = 1,
                          .key_max          = 1'000'000,
                          .pct_insert       = 0.0,
                          .pct_lookup       = 1.0,
                          .pct_erase        = 0.0,
                          .pct_clear        = 0.0,
                          .key_distribution = KeyDistribution::Zipfian, // YCSB-C: 100% read, Zipf-Skew
                          .name             = "YCSB_C_read_only"};
}

/// YCSB-D: 95% Lookup, 5% Insert, Read-Latest-Verteilung (zuletzt eingefügte/höchste Keys sind am heißesten).
[[nodiscard]] constexpr WorkloadConfig make_ycsb_d(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed           = seed,
                          .num_operations = ops,
                          .key_min        = 1,
                          .key_max        = 1'000'000,
                          .pct_insert     = 0.05,
                          .pct_lookup     = 0.95,
                          .pct_erase      = 0.0,
                          .pct_clear      = 0.0,
                          .key_distribution =
                              KeyDistribution::Latest, // YCSB-D: „read latest"-Skew (Definitionsmerkmal)
                          .name = "YCSB_D_read_latest"};
}

/// YCSB-E: 95% Range-Scan, 5% Insert, Zipfian-Verteilung. Scan-Länge je Op uniform [1, scan_length_max=100]
/// (YCSB-Default). Setzt eine scanbare Tier-Binary voraus (IScannableTier, ABI-Minor ≥ 2); alt-gebaute DLLs
/// ohne Scan-Fähigkeit → der Host überspringt die Scan-Ops ehrlich (dynamic_cast → null).
[[nodiscard]] constexpr WorkloadConfig make_ycsb_e(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed             = seed,
                          .num_operations   = ops,
                          .key_min          = 1,
                          .key_max          = 1'000'000,
                          .pct_insert       = 0.05,
                          .pct_lookup       = 0.0,
                          .pct_erase        = 0.0,
                          .pct_clear        = 0.0,
                          .pct_scan         = 0.95,
                          .pct_rmw          = 0.0,
                          .scan_length_max  = 100,
                          .key_distribution = KeyDistribution::Zipfian, // YCSB-E: Zipf-Skew auf den Scan-Start-Key
                          .name             = "YCSB_E_scan_insert"};
}

/// YCSB-F: 50% Read, 50% Read-Modify-Write, Zipfian-Verteilung. RMW = lookup(key) → modifizieren →
/// insert(key, neuer Wert) als EINE Op (host-seitig, kein zusätzliches Tier-Interface nötig).
[[nodiscard]] constexpr WorkloadConfig make_ycsb_f(std::uint64_t seed = 42, std::size_t ops = 10'000) noexcept {
    return WorkloadConfig{.seed             = seed,
                          .num_operations   = ops,
                          .key_min          = 1,
                          .key_max          = 1'000'000,
                          .pct_insert       = 0.0,
                          .pct_lookup       = 0.50,
                          .pct_erase        = 0.0,
                          .pct_clear        = 0.0,
                          .pct_scan         = 0.0,
                          .pct_rmw          = 0.50,
                          .key_distribution = KeyDistribution::Zipfian, // YCSB-F: Zipf-Skew, read + read-modify-write
                          .name             = "YCSB_F_read_modify_write"};
}

// STAND YCSB-E/F (V5-#49, Op-Set-Erweiterung): E (95% Range-Scan / 5% Insert) und F (50% Read / 50% RMW) sind
// jetzt TREU abgebildet — E über den neuen WorkloadOpKind::Scan + das ABI-Sub-Interface IScannableTier
// (anatomy/scannable_tier.hpp, additiv, ABI-Minor 2), F über WorkloadOpKind::ReadModifyWrite (lookup+insert
// host-seitig, kein ABI-Bedarf). KEIN Fake-Proxy ([[feedback_no_quick_fixes]]). Verbleibende ehrliche Lücke:
// "update" (A/B) bleibt auf Insert gemappt (tier_insert=emplace mit Update-bei-Kollision in ComposedSearch,
// s. composable_search.hpp:69/97 — faktisch ein Upsert, daher YCSB-treu genug; kein separater Update-Op-Kind).

} // namespace comdare::cache_engine::builder::workload_driver
