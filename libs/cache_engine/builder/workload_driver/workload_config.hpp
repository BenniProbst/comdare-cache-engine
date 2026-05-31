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
    Insert = 0,  ///< (key, value) ins Container einfuegen
    Lookup = 1,  ///< key suchen, optional value zurueckgeben
    Erase  = 2,  ///< key entfernen
    Clear  = 3   ///< Container komplett leeren (selten, fuer Stress-Tests)
};

[[nodiscard]] constexpr std::string_view op_kind_name(WorkloadOpKind k) noexcept {
    switch (k) {
        case WorkloadOpKind::Insert: return "Insert";
        case WorkloadOpKind::Lookup: return "Lookup";
        case WorkloadOpKind::Erase:  return "Erase";
        case WorkloadOpKind::Clear:  return "Clear";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// WorkloadOp — eine einzelne Operation in der Workload-Sequenz
// ─────────────────────────────────────────────────────────────────────────────

/// WorkloadOp — POD mit Operation-Kind + Daten.
///
/// Pro Op-Kind ist nur ein Teil relevant:
/// - Insert: `key` + `value`
/// - Lookup: `key` (value ignoriert)
/// - Erase:  `key` (value ignoriert)
/// - Clear:  alles ignoriert
struct WorkloadOp {
    WorkloadOpKind kind;
    std::uint64_t  key;
    std::uint64_t  value;

    [[nodiscard]] friend constexpr bool
    operator==(WorkloadOp const&, WorkloadOp const&) noexcept = default;
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
    std::size_t   num_operations = 1000;

    /// Key-Range [key_min, key_max] inklusive. key_max muss > key_min sein.
    std::uint64_t key_min = 1;
    std::uint64_t key_max = 1'000'000;

    /// Op-Mix Wahrscheinlichkeiten. Generator normalisiert wenn Summe != 1.0.
    double pct_insert = 0.50;
    double pct_lookup = 0.40;
    double pct_erase  = 0.09;
    double pct_clear  = 0.01;

    /// Identifikation fuer Mess-Protokoll-Ausgaben.
    std::string_view name = "DefaultMixedWorkload";

    /// Validation: Pre-Condition fuer WorkloadGenerator Konstruktion.
    [[nodiscard]] constexpr bool is_valid() const noexcept {
        if (seed == 0)               return false;  // xorshift64 undefiniert
        if (num_operations == 0)     return false;
        if (key_max <= key_min)      return false;
        if (pct_insert < 0.0 || pct_lookup < 0.0 ||
            pct_erase  < 0.0 || pct_clear  < 0.0) return false;
        if ((pct_insert + pct_lookup + pct_erase + pct_clear) <= 0.0) return false;
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Vorgefertigte Workload-Profile (typische Mess-Reihen)
// ─────────────────────────────────────────────────────────────────────────────

/// Insert-Heavy: 80% Insert, 20% Lookup (Bulk-Load-Phase).
[[nodiscard]] constexpr WorkloadConfig make_insert_heavy(std::uint64_t seed = 42,
                                                          std::size_t  ops  = 10'000) noexcept {
    return WorkloadConfig{
        .seed = seed, .num_operations = ops,
        .key_min = 1, .key_max = 1'000'000,
        .pct_insert = 0.80, .pct_lookup = 0.20,
        .pct_erase = 0.0, .pct_clear = 0.0,
        .name = "InsertHeavy_80_20"};
}

/// Lookup-Heavy: 95% Lookup, 5% Insert (Read-dominant — typisch fuer
/// Index-Benchmarks nach Bulk-Load).
[[nodiscard]] constexpr WorkloadConfig make_lookup_heavy(std::uint64_t seed = 42,
                                                          std::size_t  ops  = 10'000) noexcept {
    return WorkloadConfig{
        .seed = seed, .num_operations = ops,
        .key_min = 1, .key_max = 1'000'000,
        .pct_insert = 0.05, .pct_lookup = 0.95,
        .pct_erase = 0.0, .pct_clear = 0.0,
        .name = "LookupHeavy_95_5"};
}

/// Mixed-A: 50/50 Insert/Lookup (YCSB-A-aehnlich).
[[nodiscard]] constexpr WorkloadConfig make_mixed_a(std::uint64_t seed = 42,
                                                     std::size_t  ops  = 10'000) noexcept {
    return WorkloadConfig{
        .seed = seed, .num_operations = ops,
        .key_min = 1, .key_max = 1'000'000,
        .pct_insert = 0.50, .pct_lookup = 0.50,
        .pct_erase = 0.0, .pct_clear = 0.0,
        .name = "MixedA_50_50"};
}

/// Mixed-B: 5/95 Insert/Lookup (YCSB-B-aehnlich, read-heavy mit kleinem write-share).
[[nodiscard]] constexpr WorkloadConfig make_mixed_b(std::uint64_t seed = 42,
                                                     std::size_t  ops  = 10'000) noexcept {
    return WorkloadConfig{
        .seed = seed, .num_operations = ops,
        .key_min = 1, .key_max = 1'000'000,
        .pct_insert = 0.05, .pct_lookup = 0.95,
        .pct_erase = 0.0, .pct_clear = 0.0,
        .name = "MixedB_5_95"};
}

// ─────────────────────────────────────────────────────────────────────────────
// YCSB-ANLEHNUNG — EHRLICHE EINSCHRÄNKUNG (User 2026-05-31, Task #49)
// ─────────────────────────────────────────────────────────────────────────────
// Die folgenden make_ycsb_*-Profile sind YCSB-INSPIRIERT (Op-MIX-Anlehnung), NICHT YCSB-KONFORM:
//   - YCSB = Yahoo! Cloud Serving Benchmark (Cooper, Silberstein, Tam, Ramakrishnan, Sears: "Benchmarking
//     Cloud Serving Systems with YCSB", ACM SoCC 2010) — Zitat web-zu-verifizieren ([[feedback_web_research_per_algorithm_pflicht]]).
//   - ABWEICHUNGEN vom echten YCSB: (1) Key-Verteilung hier UNIFORM (WorkloadGenerator xorshift64), YCSB nutzt
//     ZIPFIAN bzw. "latest" (D); (2) "update" (A/B) ist hier auf Insert gemappt, tier_insert=emplace ≠ Overwrite;
//     (3) E (Range-Scan) + F (Read-Modify-Write) sind mit dem Op-Set (Insert/Lookup/Erase/Clear) NICHT abbildbar
//     und bewusst NICHT als Fake-Proxy erfunden ([[feedback_no_quick_fixes]]).
//   - Die EXTERNE Testdatensatz-Quelle (vertragliche Schnittstelle) ist NUR gemockt; weder in die Testdaten-
//     Konfigurabilität erweitert noch web-dokumentiert. TREUE Einbindung (Zipfian/latest + Update/Scan-Op +
//     Zitat) ODER ehrliche Entkopplung ("YCSB-inspiriert") = **Task #49** (Planrunde/User-Entscheidung offen).

/// YCSB-C: 100% Lookup (Read-Only). Op-MIX YCSB-C-treu; Key-Verteilung uniform (YCSB=Zipfian). S. Note oben + Task #49.
[[nodiscard]] constexpr WorkloadConfig make_ycsb_c(std::uint64_t seed = 42,
                                                   std::size_t  ops  = 10'000) noexcept {
    return WorkloadConfig{
        .seed = seed, .num_operations = ops,
        .key_min = 1, .key_max = 1'000'000,
        .pct_insert = 0.0, .pct_lookup = 1.0,
        .pct_erase = 0.0, .pct_clear = 0.0,
        .name = "YCSB_C_read_only"};
}

/// YCSB-D: 95% Lookup, 5% Insert (Read-Latest — neue Keys werden eingefügt + bald gelesen). Der Op-MIX ist
/// YCSB-D-treu; die „read-latest"-Key-VERTEILUNG ist mit der aktuellen uniformen Key-Sampling-Strategie des
/// WorkloadGenerator NICHT abgebildet (uniform statt latest-skewed) — dokumentierte Einschränkung.
[[nodiscard]] constexpr WorkloadConfig make_ycsb_d(std::uint64_t seed = 42,
                                                   std::size_t  ops  = 10'000) noexcept {
    return WorkloadConfig{
        .seed = seed, .num_operations = ops,
        .key_min = 1, .key_max = 1'000'000,
        .pct_insert = 0.05, .pct_lookup = 0.95,
        .pct_erase = 0.0, .pct_clear = 0.0,
        .name = "YCSB_D_read_latest"};
}

// HINWEIS YCSB-E/F: E (95% Range-Scan / 5% Insert) und F (Read-Modify-Write) sind mit dem aktuellen
// WorkloadOpKind-Set (Insert/Lookup/Erase/Clear) NICHT verlustfrei abbildbar — E braucht eine Scan/Range-
// Operation, F eine atomare RMW-Op. Bewusst NICHT als Fake-Profil mit Lookup-Proxy erfunden
// ([[feedback_no_quick_fixes]]); Erweiterung des Op-Sets = separater Punkt.

}  // namespace comdare::cache_engine::builder::workload_driver
