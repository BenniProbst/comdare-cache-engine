#pragma once
// D4b / L-75 (2026-06-02) — IContainerTier: ABI-stabiles Antriebs-/Observer-Sub-Interface der CONTAINER-Gattung
// (Adapter), analog IObservableTier für SearchAlgorithm, aber Queue-Semantik (put/get statt insert/lookup/erase).
//
// Doc 24 §8.8 (User-Direktive): JEDE neue Gattung bekommt ein EIGENES Antriebs-Sub-Interface + einen EIGENEN
// flachen V1-POD — `IAnatomyBase`/`ComdareTierObserverSnapshotV1` werden NIE mutiert (das bräche alte DLLs).
// Der host-seitige Container-Dock fragt eine geladene DLL via `dynamic_cast<IContainerTier*>(ianatomy_ptr)` ab;
// SearchAlgorithm-DLLs (kein IContainerTier) → nullptr → sauberes Degradieren (kein ABI-Bruch).
//
// IContainerTier ist EIGENSTÄNDIG (erbt NICHT IDriveableTier — dessen K/V-insert/lookup/erase passt nicht zur
// Queue). Der genus-typisierte ContainerAbiAdapter erbt IAnatomyBase (Gattungs-Meta + Lifecycle) + IContainerTier.
// Der POD quert die .dll-Grenze als reine uint64-Felder (standard_layout + trivially_copyable → memcpy-fähig).

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// Komposition-UNABHÄNGIGER, ABI-stabiler Container-Observer-Snapshot (cross-boundary). FIXE Layout (V1):
/// NUR uint64 → standard_layout + trivially_copyable, identisches Layout in Host + Modul-Binary. Spiegelt
/// die Q1-Buffer- + Q2-Flush-Zähler von ContainerObserverSnapshot (container_anatomy.hpp).
struct ContainerObserverSnapshotV1 {
    // ── Q1 buffer_strategy ──
    std::uint64_t put_count                 = 0;
    std::uint64_t get_count                 = 0;
    std::uint64_t current_occupancy         = 0;
    std::uint64_t peak_occupancy            = 0;
    // ── Q2 flush_policy ──
    std::uint64_t flush_decisions_evaluated = 0;
    std::uint64_t full_flush_count          = 0;
    std::uint64_t flush_complete_count      = 0;
    // ── Meta ──
    std::uint64_t organ_count               = 0;   // == ContainerAnatomy::organ_count() (2: buffer + flush)

    [[nodiscard]] constexpr bool operator==(ContainerObserverSnapshotV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ContainerObserverSnapshotV1>,
              "ABI-Pflicht: Container-Cross-Boundary-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ContainerObserverSnapshotV1>,
              "ABI-Pflicht: Container-Cross-Boundary-Snapshot muss memcpy-fähig (trivially_copyable) sein");

/// ABI-Version des Container-Snapshot-Formats (eigene Versionierung; unabhängig vom SearchAlgorithm-Snapshot).
inline constexpr std::uint32_t kContainerObserverSnapshotVersion = 1;

/// IContainerTier — Antriebs-/Observer-Sub-Interface der Container-Gattung (Queue-Semantik). Der Host treibt
/// eine geladene Container-DLL über put/get/size/clear (uint64-Element-Raum) und zieht den eingebauten Observer
/// als flachen POD (tier_observe_container). Eigenständig (kein IDriveableTier).
class IContainerTier {
public:
    virtual ~IContainerTier() = default;

    /// Legt ein Element ab (treibt das Q1-Buffer-Organ; löst danach die Q2-Flush-Policy-Entscheidung aus).
    virtual void tier_put(std::uint64_t value) noexcept = 0;

    /// Entnimmt das nächste Element. Bei Erfolg wird *out_value gesetzt (falls out_value != nullptr) und true geliefert.
    [[nodiscard]] virtual bool tier_get(std::uint64_t* out_value) noexcept = 0;

    /// Aktuelle Belegung (Füllstand) des Buffers.
    [[nodiscard]] virtual std::uint64_t tier_size() const noexcept = 0;

    /// Leert den Buffer-Zustand (Statistik-Reset ist separat über reset()).
    virtual void tier_clear() noexcept = 0;

    /// Schreibt den aktuellen Container-Observer-Snapshot (observe_all → flacher POD) nach *out. out != nullptr.
    virtual void tier_observe_container(ContainerObserverSnapshotV1* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
