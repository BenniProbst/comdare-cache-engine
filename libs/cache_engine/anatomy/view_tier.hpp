#pragma once
// D11 / L-76c (2026-06-02) — IViewTier: ABI-stabiles Antriebs-/Observer-Sub-Interface der VIEW-Gattung (Pflanze,
// non-owning, Doku 14 §27.2/§28). Analog ISequenceTier, ABER non-owning: bind(externer Puffer)/read(index) —
// KEIN insert/erase/clear/alloc (View besitzt keinen Speicher, referenziert fremden). Doc 24 §8.8: eigenes Sub-IF + V1-POD.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// ABI-stabiler View-Observer-Snapshot (cross-boundary, nur uint64 → standard_layout + trivially_copyable).
struct ViewObserverSnapshotV1 {
    std::uint64_t read_count            = 0; // read(index)-Zugriffe
    std::uint64_t read_oob_count        = 0; // davon out-of-bounds
    std::uint64_t bound_size            = 0; // Länge des aktuell gebundenen externen Puffers
    std::uint64_t bind_count            = 0; // Anzahl bind()-Aufrufe (Re-Binding)
    std::uint64_t observable_axis_count = 0; // real beobachtete Achsen (R5.B, ehrlich)
    std::uint64_t organ_count           = 0; // == ViewAnatomy::organ_count() (7: 4 geteilt + extent/layout/accessor)

    [[nodiscard]] constexpr bool operator==(ViewObserverSnapshotV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ViewObserverSnapshotV1>,
              "ABI-Pflicht: View-Cross-Boundary-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ViewObserverSnapshotV1>,
              "ABI-Pflicht: View-Cross-Boundary-Snapshot muss memcpy-fähig sein");

inline constexpr std::uint32_t kViewObserverSnapshotVersion = 1;

/// IViewTier — Antriebs-/Observer-Sub-Interface der View-Gattung (non-owning, V-indexed read-only). Bewusst KEIN
/// tier_insert/erase/clear (API-Asymmetrie zur Sequence: View ist nicht-besitzend + immutabel über den fremden Puffer).
class IViewTier {
public:
    virtual ~IViewTier() = default;

    /// Bindet einen EXTERNEN Puffer (non-owning — der Aufrufer garantiert Lebensdauer > View). data darf null sein (size 0).
    virtual void tier_bind(std::uint64_t const* data, std::uint64_t size) noexcept = 0;

    /// Liest Element an Position index aus dem gebundenen Puffer. Bei gültigem Index → *out_value gesetzt, true.
    [[nodiscard]] virtual bool tier_read(std::uint64_t index, std::uint64_t* out_value) const noexcept = 0;

    /// Länge des gebundenen Puffers.
    [[nodiscard]] virtual std::uint64_t tier_size() const noexcept = 0;

    /// Schreibt den aktuellen View-Observer-Snapshot nach *out. out != nullptr.
    virtual void tier_observe_view(ViewObserverSnapshotV1* out) const noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
