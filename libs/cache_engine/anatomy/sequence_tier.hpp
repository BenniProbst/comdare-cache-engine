#pragma once
// D10 / L-76b (2026-06-02) — ISequenceTier: ABI-stabiles Antriebs-/Observer-Sub-Interface der SEQUENCE-Gattung
// (Reptil, V-indexed, Doku 14 §27.2/§28). Analog ISetTier/IContainerTier, aber Sequenz-Semantik: push_back(value)/
// at(index)→value — V-only-indexiert (KEIN Key-Suchorgan). Doc 24 §8.8: eigenes Sub-Interface + eigener V1-POD.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// ABI-stabiler Sequence-Observer-Snapshot (cross-boundary, nur uint64 → standard_layout + trivially_copyable).
struct SequenceObserverSnapshotV1 {
    std::uint64_t push_count            = 0;   // push_back-Aufrufe
    std::uint64_t at_count              = 0;   // at(index)-Zugriffe
    std::uint64_t at_oob_count          = 0;   // davon out-of-bounds
    std::uint64_t current_size          = 0;   // aktuelle Länge
    std::uint64_t peak_size             = 0;   // maximale Länge
    std::uint64_t growth_events         = 0;   // Kapazitäts-Wachstums-Ereignisse (axis_growth getrieben)
    std::uint64_t observable_axis_count = 0;   // real beobachtete Achsen (R5.B, ehrlich)
    std::uint64_t organ_count           = 0;   // == SequenceAnatomy::organ_count() (10 + axis_growth = 11)

    [[nodiscard]] constexpr bool operator==(SequenceObserverSnapshotV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<SequenceObserverSnapshotV1>,
              "ABI-Pflicht: Sequence-Cross-Boundary-Snapshot muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<SequenceObserverSnapshotV1>,
              "ABI-Pflicht: Sequence-Cross-Boundary-Snapshot muss memcpy-fähig sein");

inline constexpr std::uint32_t kSequenceObserverSnapshotVersion = 1;

/// ISequenceTier — Antriebs-/Observer-Sub-Interface der Sequence-Gattung (V-indexed). Eigenständig (kein
/// IDriveableTier: dessen K/V-insert passt nicht zur index-adressierten Sequenz).
class ISequenceTier {
public:
    virtual ~ISequenceTier() = default;

    /// Hängt value am Ende an (treibt das V-Speicher-Organ + ggf. die Growth-Policy).
    virtual void tier_push_back(std::uint64_t value) noexcept = 0;

    /// Liest Element an Position index. Bei gültigem Index wird *out_value gesetzt und true geliefert.
    [[nodiscard]] virtual bool tier_at(std::uint64_t index, std::uint64_t* out_value) const noexcept = 0;

    /// Länge der Sequenz.
    [[nodiscard]] virtual std::uint64_t tier_size() const noexcept = 0;

    /// Leert die Sequenz.
    virtual void tier_clear() noexcept = 0;

    /// Schreibt den aktuellen Sequence-Observer-Snapshot nach *out. out != nullptr.
    virtual void tier_observe_sequence(SequenceObserverSnapshotV1* out) const noexcept = 0;
};

}  // namespace comdare::cache_engine::anatomy
