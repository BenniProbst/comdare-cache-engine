#pragma once
// V41.F.6.1.R6 — IObservableTier: ABI-stabiles Observer-Zugriffs-Sub-Interface (Pfad B über die
// Modul-Binary-Grenze).
//
// Doku 24 §8.6/§8.7 (HYBRID-Mess-Modell, User-Direktive 2026-05-30): Der composite Tier wird als
// separates, dynamisch ladbares C++23-Modul (.so/.dll) gebaut; die CacheEngineBuilder kommuniziert mit
// ihm AUSSCHLIESSLICH über das ABI-stabile Interface der GATTUNG (SearchAlgorithm). Der Builder
//   (1) testet die Gattungs-API durch (tier_insert/lookup/erase),
//   (2) misst die IM Tier eingebauten Observer (tier_observe → Snapshot),
//   (3) zieht den Snapshot als flachen POD durch die Schnittstelle, und
//   (4) persistiert die korrelierten (wall_clock ↔ Observer)-Ergebnisse.
//
// ABI-SICHER nach demselben Designprinzip wie IMeasurableWorkload (measurable_workload.hpp):
//   - IObservableTier hängt NICHT an IAnatomyBase (das änderte dessen vtable-Layout, bräche alte DLLs),
//     sondern ist ein eigenständiges Sub-Interface, das der genus-typisierte ABI-Adapter ZUSÄTZLICH erbt.
//   - Der Host fragt es via `dynamic_cast<IObservableTier*>(ianatomy_ptr)` ab; alte DLLs → nullptr →
//     Host degradiert sauber (kein ABI-Bruch).
//   - Der Observer-Snapshot quert die Grenze als FLACHER, komposition-UNABHÄNGIGER POD (nur uint64-Felder,
//     fixe Layout, keine STL/vtable) → memcpy-fähig zwischen Host und Modul-Binary.
//
// @doku docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md §8.6/§8.7
// @related [[feedback_zwei_dimensionen_messmodell]] [[execution-engine-als-wurzel]]

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// ─────────────────────────────────────────────────────────────────────────────
// ComdareTierObserverSnapshotV1 — flacher, ABI-stabiler Observer-Snapshot
// ─────────────────────────────────────────────────────────────────────────────

/// Komposition-UNABHÄNGIGER POD-Snapshot, der die Per-Achsen-Observer eines composite Tiers über die
/// Modul-Binary-ABI-Grenze trägt. Anders als ObserverAggregate<Composition> (dessen Layout je Composition
/// variiert) hat dieser eine FIXE, versionierte Layout (V1) — Voraussetzung für memcpy über die .dll-Grenze.
/// NUR uint64-Felder → garantiert standard_layout + trivially_copyable, identisches Layout in Host + Modul.
///
/// V1 trägt die beiden bereits real getriebenen Achsen (search_algo + allocator, Doku 24 §5.4/§8.5) plus
/// Meta. Erweiterung um weitere Achsen = NEUE Snapshot-Version (V2…), das Interface bleibt ABI-stabil.
struct ComdareTierObserverSnapshotV1 {
    // ── Achse search_algo (axis_03a) — SearchAlgoStatistics, gespiegelt ──
    std::uint64_t search_lookup_count   = 0;
    std::uint64_t search_hit_count      = 0;
    std::uint64_t search_miss_count     = 0;
    std::uint64_t search_insert_count   = 0;
    std::uint64_t search_erase_count    = 0;
    std::uint64_t search_peak_occupancy = 0;
    // ── Achse allocator (axis_06) — AllocationStatistics, gespiegelt ──
    std::uint64_t alloc_bytes_allocated = 0;
    std::uint64_t alloc_bytes_in_use    = 0;
    std::uint64_t alloc_allocation_count   = 0;
    std::uint64_t alloc_deallocation_count = 0;
    std::uint64_t alloc_failure_count   = 0;
    // ── Meta ──
    std::uint64_t observable_axis_count = 0;   // ObserverAggregate::observable_count() — Diagnose
    std::uint64_t tier_fill_level       = 0;   // tier_size() zum Snapshot-Zeitpunkt

    [[nodiscard]] constexpr bool operator==(ComdareTierObserverSnapshotV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareTierObserverSnapshotV1>,
              "ABI-Pflicht: Cross-Boundary-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareTierObserverSnapshotV1>,
              "ABI-Pflicht: Cross-Boundary-Snapshot muss memcpy-fähig (trivially_copyable) sein");

/// ABI-Version des Snapshot-Formats. Major-Bump bei Layout-Änderung (neue Achse = neuer Snapshot-Typ V2).
inline constexpr std::uint32_t kTierObserverSnapshotVersion = 1;

// ─────────────────────────────────────────────────────────────────────────────
// IObservableTier — ABI-stabiles Observer-Zugriffs-Sub-Interface
// ─────────────────────────────────────────────────────────────────────────────

/// IObservableTier — optionales Sub-Interface einer geladenen Tier-Anatomie (Pfad B, Doku 24 §8.6).
///
/// Der host-seitige CacheEngineBuilder treibt damit die Gattungs-API des Tier-Moduls (tier_insert/lookup/
/// erase über uint64 — der gemeinsame Key-Raum nach Umstufung-B) und zieht dessen eingebaute Observer als
/// flachen POD (tier_observe). Beide Trigger-Modi aus §8.7 (Zeitschritt-Sync / Zustands-Manipulation)
/// realisiert der Host, indem er nach Operationen bzw. Intervallen tier_observe() aufruft und den Snapshot
/// mit einem Wall-Clock-Zeitstempel korreliert + persistiert.
class IObservableTier {
public:
    virtual ~IObservableTier() = default;

    // ── Gattungs-Antrieb (SearchAlgorithm-Gattung): gemeinsamer uint64-Key/Value-Raum ──

    /// Fügt (key,value) ein bzw. aktualisiert. Rückgabe: true wenn ein NEUER Schlüssel entstand.
    [[nodiscard]] virtual bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept = 0;

    /// Sucht key. Bei Treffer wird *out_value gesetzt (falls out_value != nullptr) und true geliefert.
    [[nodiscard]] virtual bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept = 0;

    /// Entfernt key. Rückgabe: true wenn key vorhanden war.
    [[nodiscard]] virtual bool tier_erase(std::uint64_t key) noexcept = 0;

    /// Leert den Tier-Zustand (Schlüssel; Statistik-Reset ist separat über reset()).
    virtual void tier_clear() noexcept = 0;

    /// Logische Schlüsselzahl (Füllstand) — Stützpunkt der zustands-getriggerten Observer-Erhebung (§8.7b).
    [[nodiscard]] virtual std::uint64_t tier_size() const noexcept = 0;

    // ── Observer-Zugriff: die IM Tier eingebauten Observer als flachen POD durch die ABI-Grenze ziehen ──

    /// Schreibt den aktuellen Observer-Snapshot (observe_all → flacher POD) nach *out. out != nullptr.
    /// noexcept (reines Auslesen von Zählern). Der Host stempelt das Resultat mit Wall-Clock + persistiert.
    virtual void tier_observe(ComdareTierObserverSnapshotV1* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
