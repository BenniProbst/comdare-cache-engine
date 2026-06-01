#pragma once
// V41.F.6.1.R5.C.A2 — ExecutionEngine als Wurzel ueber AnatomyBase (Mess-Schicht)
//
// User-Direktive 2026-05-27 frueh (Doku 14 Teil 5 §33-§40):
// "ExecutionEngine ist die Wurzel ueber AnatomyBase. Anatomie verhaelt sich zu
// ExecutionEngine wie Lebewesen zu Viren — Viren sind nicht lebendig (keine
// Topics/Achsen), aber ausmessbar. Mess-Interface auf ExecutionEngine-Ebene."
//
// Zwei-Schichten-Architektur (analog AnatomyBase Teil 4):
//   1. ExecutionEngineConcept — Compile-Time C++23 Concept (Static Dispatch)
//   2. IExecutionEngine        — Virtual Interface (Runtime ABI fuer Module-Loader R5.E)
//
// Spezialisierungen:
//   - IAnatomyBase : IExecutionEngine — Lebewesen mit Topics+Achsen (5 Gattungen)
//   - IVirusExecutionEngine : IExecutionEngine — Nicht-Lebewesen (Graphen-Algos etc.)
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §33-§40
// @task #701 V41.F.6.1.R5.C.A2
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]
// @historical_reference prt_art IExecutingEngine REV5 (comdare-prt-art-Repo; libs/deprecated/prt_art_legacy-Skelett 2026-06-01 entfernt)

#include <concepts>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::execution_engine {

// ─────────────────────────────────────────────────────────────────────────────
// ExecutionEngineKind — 3 Wurzel-Kategorien (Compile-Time Enum)
// ─────────────────────────────────────────────────────────────────────────────

/// ExecutionEngineKind — biologische Wurzel-Kategorisierung jeder ausmessbaren Engine.
///
/// | Kind     | Charakteristik                      | Beispiele                       |
/// |----------|-------------------------------------|----------------------------------|
/// | Anatomy  | Lebewesen, Topics+Achsen, AnatomyBase | SearchAlgorithm/Sequence/Set/Adapter/View |
/// | Virus    | Nicht-Lebewesen, kein Achsen-System | Graphen-Algos / Pipelines / Pure-Math |
/// | Hybrid   | experimentell Mixed                  | V42+ (z.B. Algorithmus-mit-internem-Cache) |
enum class ExecutionEngineKind : std::uint8_t {
    Anatomy = 0,  ///< Lebewesen — erbt AnatomyBase, hat Topics/Achsen + Composition
    Virus   = 1,  ///< Nicht-Lebewesen — kein Anatomie-Stoffwechsel, eigenes Mess-System
    Hybrid  = 2   ///< experimentell: kombiniert beide (V42+)
};

/// engine_kind_name() — Compile-Time-String pro Kind.
[[nodiscard]] constexpr std::string_view engine_kind_name(ExecutionEngineKind k) noexcept {
    switch (k) {
        case ExecutionEngineKind::Anatomy: return "Anatomy";
        case ExecutionEngineKind::Virus:   return "Virus";
        case ExecutionEngineKind::Hybrid:  return "Hybrid";
    }
    return "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// EngineLifecycleState — Mess-Lebenszyklus (analog prt_art_legacy IExecutingEngine)
// ─────────────────────────────────────────────────────────────────────────────

/// EngineLifecycleState — Phasen einer ExecutionEngine-Mess-Session.
enum class EngineLifecycleState : std::uint8_t {
    Uninitialized = 0,
    Warming       = 1,  ///< warm_up() laeuft (Cache-Preheat, Index-Bulk-Load)
    Running       = 2,  ///< aktive Mess-Phase
    Idle          = 3,  ///< zwischen Mess-Reihen, statistics-stable
    Shutdown      = 4   ///< shutdown() abgeschlossen
};

// ─────────────────────────────────────────────────────────────────────────────
// ExecutionEngineConcept — Compile-Time-Wurzel-Concept aller ausmessbaren Algos
// ─────────────────────────────────────────────────────────────────────────────

/// ExecutionEngineConcept — Wurzel-Concept aller ausmessbaren Algorithmen.
///
/// **Pflicht-Members:**
/// - `measurement_snapshot_t` Type-Alias (Engine-spezifischer POD-Snapshot)
/// - `engine_name()` — Compile-Time-String
/// - `engine_kind()` — Compile-Time ExecutionEngineKind
template <class E>
concept ExecutionEngineConcept = requires {
    typename E::measurement_snapshot_t;
    { E::engine_name() } -> std::convertible_to<std::string_view>;
    { E::engine_kind() } -> std::convertible_to<ExecutionEngineKind>;
};

// ─────────────────────────────────────────────────────────────────────────────
// IExecutionEngine — Virtual Interface (Runtime ABI fuer Module-Loader R5.E)
// ─────────────────────────────────────────────────────────────────────────────

/// IExecutionEngine — abstract base fuer alle ausmessbaren Engines (Runtime-Polymorph).
///
/// **Verwendung:** Module-Loader (R5.E) loadet .so/.dll und ruft Pflicht-API
/// ueber dieses Interface. Konkrete Engines nutzen die Compile-Time-Concept-Schicht
/// (ExecutionEngineConcept) fuer Hot-Path-Performance.
///
/// **Spezialisierungen:**
/// - IAnatomyBase erbt + setzt engine_kind() = Anatomy (Lebewesen)
/// - IVirusExecutionEngine erbt + setzt engine_kind() = Virus (Nicht-Lebewesen)
class IExecutionEngine {
public:
    virtual ~IExecutionEngine() = default;

    /// Engine-Identifier (z.B. "ArtComposition" fuer Anatomy, "GraphBFS" fuer Virus)
    [[nodiscard]] virtual std::string_view    engine_name() const noexcept = 0;

    /// Engine-Kind (Anatomy / Virus / Hybrid)
    [[nodiscard]] virtual ExecutionEngineKind engine_kind() const noexcept = 0;

    /// Mess-Lebenszyklus aktueller Stand
    [[nodiscard]] virtual EngineLifecycleState lifecycle_state() const noexcept = 0;

    // Mess-Schnittstelle (Pflicht fuer alle ExecutionEngines — analog prt_art_legacy)
    virtual void warm_up()  = 0;  ///< Engine vor Mess-Reihe vorwaermen (Cache-Preheat, Bulk-Load)
    /// R5.C.A4: aktive Mess-Phase starten (Uebergang Warming/Idle → Running).
    /// CacheEngineBuilder ruft run() bevor Workload-Driver Insert/Lookup-Commands
    /// dispatched. Mess-Hooks (Latenz/Throughput) sind nur waehrend Running aktiv.
    virtual void run()      = 0;  ///< Aktive Mess-Phase starten (lifecycle_state → Running)
    virtual void reset()    = 0;  ///< Statistik-Reset (NICHT Container-Clear! — siehe [[reset-is-statistics-reset]])
    virtual void shutdown() = 0;  ///< Engine sauber herunterfahren (Resources freigeben)
};

}  // namespace comdare::cache_engine::execution_engine
