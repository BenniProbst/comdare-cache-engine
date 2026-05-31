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

#include "idriveable_tier.hpp"   // V5-I2: IObservableTier erbt den funktionalen Antrieb (immer einkompiliert)

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
/// V5-I2: IObservableTier erbt den funktionalen Antrieb aus IDriveableTier (idriveable_tier.hpp, IMMER einkompiliert)
/// und ergänzt NUR die Beobachtung (tier_observe). Der ABI-Adapter vererbt IObservableTier NUR bei Messung-AN
/// (COMDARE_MEASUREMENT_ON) — die Antriebs-Ops bleiben über IDriveableTier auch in der Release-/funktional-only-DLL.
class IObservableTier : public IDriveableTier {
public:
    ~IObservableTier() override = default;

    // ── Observer-Zugriff: die IM Tier eingebauten Observer als flachen POD durch die ABI-Grenze ziehen ──

    /// Schreibt den aktuellen Observer-Snapshot (observe_all → flacher POD) nach *out. out != nullptr.
    /// noexcept (reines Auslesen von Zählern). Der Host stempelt das Resultat mit Wall-Clock + persistiert.
    virtual void tier_observe(ComdareTierObserverSnapshotV1* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
