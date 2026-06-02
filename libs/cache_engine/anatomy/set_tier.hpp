#pragma once
// D9 / L-76a (2026-06-02) — ISetTier: ABI-stabiles Antriebs-/Observer-Sub-Interface der SET-Gattung (Vogel,
// K-only, Doku 14 §27.2/§28). Analog IObservableTier (SearchAlgorithm) / IContainerTier (Adapter), aber
// Mengen-Semantik: insert(key)/contains(key)/erase(key) — KEIN Value (K=V, Doku 14 §28 „— (K=V)").
//
// Doc 24 §8.8: neue Gattung = EIGENES Sub-Interface + EIGENER flacher V1-POD (IAnatomyBase NIE mutieren). Der
// host-seitige Set-Dock fragt eine geladene DLL via dynamic_cast<ISetTier*> ab. Eigenständig (kein IDriveableTier:
// dessen insert(key,value) trägt einen Value, den die Set-Gattung nicht hat). uint64-Key-Raum (Umstufung-B).

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// Komposition-UNABHÄNGIGER, ABI-stabiler Set-Observer-Snapshot (cross-boundary). FIXE Layout (V1): nur uint64
/// → standard_layout + trivially_copyable (memcpy über die .dll-Grenze). K-only-Mengen-Statistik.
struct SetObserverSnapshotV1 {
    std::uint64_t insert_count          = 0;   // tier_set_insert-Aufrufe, die einen NEUEN Key erzeugten
    std::uint64_t contains_count        = 0;   // tier_set_contains-Abfragen
    std::uint64_t contains_hit_count    = 0;   // davon Treffer
    std::uint64_t contains_miss_count   = 0;   // davon Fehlschläge
    std::uint64_t erase_count           = 0;   // erfolgreiche Entfernungen
    std::uint64_t current_size          = 0;   // aktuelle Kardinalität der Menge
    std::uint64_t peak_size             = 0;   // maximale Kardinalität
    std::uint64_t observable_axis_count = 0;   // wie viele der 15 Set-Achsen real beobachtet (R5.B, ehrlich)
    std::uint64_t organ_count           = 0;   // == SetAnatomy::organ_count() (15)

    [[nodiscard]] constexpr bool operator==(SetObserverSnapshotV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<SetObserverSnapshotV1>,
              "ABI-Pflicht: Set-Cross-Boundary-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<SetObserverSnapshotV1>,
              "ABI-Pflicht: Set-Cross-Boundary-Snapshot muss memcpy-fähig (trivially_copyable) sein");

inline constexpr std::uint32_t kSetObserverSnapshotVersion = 1;

/// ISetTier — Antriebs-/Observer-Sub-Interface der Set-Gattung (Mengen-Semantik, K-only). Der Host treibt eine
/// geladene Set-DLL über insert/contains/erase (uint64-Key) und zieht den eingebauten Observer als flachen POD.
class ISetTier {
public:
    virtual ~ISetTier() = default;

    /// Fügt key in die Menge ein. Rückgabe: true wenn ein NEUER Schlüssel entstand (Set-Semantik: keine Duplikate).
    [[nodiscard]] virtual bool tier_set_insert(std::uint64_t key) noexcept = 0;

    /// Mitgliedschaftstest. true gdw. key in der Menge ist (kein Value-Out — K-only).
    [[nodiscard]] virtual bool tier_set_contains(std::uint64_t key) const noexcept = 0;

    /// Entfernt key. Rückgabe: true wenn key vorhanden war.
    [[nodiscard]] virtual bool tier_set_erase(std::uint64_t key) noexcept = 0;

    /// Kardinalität der Menge.
    [[nodiscard]] virtual std::uint64_t tier_set_size() const noexcept = 0;

    /// Leert die Menge (Statistik-Reset ist separat über reset()).
    virtual void tier_set_clear() noexcept = 0;

    /// Schreibt den aktuellen Set-Observer-Snapshot nach *out. out != nullptr.
    virtual void tier_observe_set(SetObserverSnapshotV1* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
