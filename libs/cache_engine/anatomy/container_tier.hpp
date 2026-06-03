#pragma once
// D4b / L-75 (2026-06-02; §28-Modell #87+#90 2026-06-03) — IContainerTier: ABI-stabiles Antriebs-/Observer-Sub-
// Interface der CONTAINER-Gattung (Adapter-Tier-Unterklasse), analog IObservableTier für SearchAlgorithm, aber
// Adapter-Semantik (put/get = push/pop_front auf dem inner_container statt insert/lookup/erase).
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
/// die inner_container-Antriebs-Zähler von ContainerObserverSnapshot (container_anatomy.hpp) 1:1.
/// #87+#90 (2026-06-03, Doku 14 §28): die Adapter-Tier-Unterklasse hat 13 Achsen (12 geteilt/delegiert +
/// inner_container) und KEINE „ordering"-Achse. Die Disziplin (FIFO/LIFO) ist API-Nutzung (front vs back),
/// daher zählt der Observer Enden-Zugriffe (front_reads/back_reads) statt ordering-select/comparison.
struct ContainerObserverSnapshotV1 {
    // ── inner_container (die spezifische §28-Achse, real getrieben) ──
    std::uint64_t push_count        = 0;   // push → inner_container
    std::uint64_t pop_count         = 0;   // erfolgreiche pop_front/pop_back
    std::uint64_t front_reads       = 0;   // front()-Zugriffe (FIFO-Disziplin)
    std::uint64_t back_reads        = 0;   // back()/top()-Zugriffe (LIFO-Disziplin)
    std::uint64_t current_occupancy = 0;   // aktuelle inner_container-Größe
    std::uint64_t peak_occupancy    = 0;   // maximale inner_container-Größe
    // ── Meta ──
    std::uint64_t organ_count       = 0;   // == ContainerAnatomy::organ_count() (13: 12 geteilt/delegiert + inner_container)

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

    /// Legt ein Element ab (treibt das inner_container-Organ: Inner.push_back).
    virtual void tier_put(std::uint64_t value) noexcept = 0;

    /// Entnimmt das nächste Element (get == FIFO-Default pop_front auf dem inner_container; die Disziplin
    /// FIFO/LIFO ist API-Nutzung, §26.4). Bei Erfolg wird *out_value gesetzt (falls out_value != nullptr) und true geliefert.
    [[nodiscard]] virtual bool tier_get(std::uint64_t* out_value) noexcept = 0;

    /// Aktuelle Belegung (Füllstand) des Inner-Substrats.
    [[nodiscard]] virtual std::uint64_t tier_size() const noexcept = 0;

    /// Leert den Inner-Zustand (Statistik-Reset ist separat über reset()).
    virtual void tier_clear() noexcept = 0;

    /// Schreibt den aktuellen Container-Observer-Snapshot (observe_all → flacher POD) nach *out. out != nullptr.
    virtual void tier_observe_container(ContainerObserverSnapshotV1* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
