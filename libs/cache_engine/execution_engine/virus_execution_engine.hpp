#pragma once
// D12 / L-76d (2026-06-02) — IVirusExecutionEngine: die NICHT-LEBEWESEN-Schwester von IAnatomyBase an der Wurzel
// IExecutionEngine (Doku 14 §33-§40). Viren = Graph-/Pure-Math-Algos OHNE Achsen-System (keine Komposition, keine
// PermutationEngine, kein observe_all über Achsen) — sie sind dennoch AUSMESSBAR (Latenz/Durchsatz + algo-eigene Zähler).
//
// engine_kind() == Virus (final). Eigener flacher Mess-POD VirusMeasurementSnapshotV1 (kein Achsen-Observer).
// Die Gattungs-∏-Logik (binary_count == ∏ mp_size) gilt für Viren NICHT (0 Achsen) — sie stehen außerhalb des
// Permutations-Baums (eigene Kapsel). Doc 28 §2 „(Viren) | keine Achsen | 0 (Kapsel)".

#include "execution_engine_base.hpp"

#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::execution_engine {

/// Flacher, ABI-stabiler Virus-Mess-Snapshot (kein Achsen-Observer; algo-eigene Zähler + Latenz). Nur uint64.
struct VirusMeasurementSnapshotV1 {
    std::uint64_t run_count       = 0; // Anzahl run_*-Aufrufe
    std::uint64_t visited_nodes   = 0; // letzter Lauf: besuchte Knoten (Graph) / verarbeitete Elemente
    std::uint64_t edges_traversed = 0; // letzter Lauf: traversierte Kanten / Operationen
    std::uint64_t result_checksum = 0; // Ergebnis-Prüfsumme (gegen Wegoptimierung + Korrektheits-Anker)

    [[nodiscard]] constexpr bool operator==(VirusMeasurementSnapshotV1 const&) const noexcept = default;
};
static_assert(std::is_standard_layout_v<VirusMeasurementSnapshotV1>);
static_assert(std::is_trivially_copyable_v<VirusMeasurementSnapshotV1>);

inline constexpr std::uint32_t kVirusMeasurementSnapshotVersion = 1;

/// IVirusExecutionEngine — Wurzel aller Nicht-Lebewesen-Engines (Graph/FFT/Crypto). engine_kind() final = Virus.
class IVirusExecutionEngine : public IExecutionEngine {
public:
    ~IVirusExecutionEngine() override = default;

    [[nodiscard]] ExecutionEngineKind engine_kind() const noexcept final { return ExecutionEngineKind::Virus; }

    /// Algorithmus-Familie (z.B. "GraphBFS", "FFT", "SHA256") — die Virus-Identität (statt Composition/genus).
    [[nodiscard]] virtual std::string_view algorithm_family() const noexcept = 0;

    /// Zieht den Virus-Mess-Snapshot (algo-eigene Zähler) als flachen POD. out != nullptr.
    virtual void virus_observe(VirusMeasurementSnapshotV1* out) const noexcept = 0;
};

} // namespace comdare::cache_engine::execution_engine
