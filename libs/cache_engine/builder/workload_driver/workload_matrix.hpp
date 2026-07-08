#pragma once
// #31-Schritt-1 (F7 Hybrid; WorkloadKind-REUSE nach Ledger-Agent-Bestands-Befund 2026-07-08):
// compile-time-Workload-Achse der 2D-Mess-Matrix — OHNE neuen Enum.
//
// **REUSE (feedback_reuse_existing_framework_modules_first):** nutzt den BESTEHENDEN
// `enum class WorkloadKind` (builder/commands/workload.hpp) statt einen dritten Workload-Typ zu schaffen
// (der erste Versuch mit eigenem WorkloadProfile-Enum wurde deshalb revertiert).
//
// **HYBRID (User 08.07.: „runtime Profile → Hybrid aus compile-time + runtime, passend zum Experiment-Baum"):**
// compile-time = `WorkloadKind`/`ycsb_profile_list` (Achse W der Matrix, wie eine DynamicVariableNode-Wertmenge
// des experiment_tree, KEINE Binary-Identitäts-Achse); runtime = `profile_by_name` (WorkloadConfig/seed/ops,
// UNVERÄNDERT). `config_for` = Brücke.
//
// **Scope Schritt-1 = die eindeutigen YCSB A–F** (in allen 3 Workload-Mengen — WorkloadKind, profile_by_name,
// F7-Plan A/C/E — IDENTISCH → konfliktfrei, kein Raten). Die Extras (WorkloadKind Custom_HotKey/RangeDelete/
// BulkInsert vs. profile_by_name IH/LH vs. F7-Datasets) sind die noch offene 3-Mengen-Vereinheitlichung
// (Ledger, Design-Folge) → hier bewusst NICHT gebrückt.
//
// **golden/ABI-NEUTRAL:** Workload ist Mess-INPUT, orthogonal zur Anatomie-Permutation
// (golden_fullpilot_320 / permutation_axes.xml / ABI-4 unberührt). Analog #29-Schritt-1 (container_framework).

#include "workload_profiles.hpp" // profile_by_name (runtime) + WorkloadConfig

#include "../commands/workload.hpp" // WorkloadKind (Bestands-Enum, REUSE)

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::builder::workload_driver {

namespace mp  = boost::mp11;
namespace cmd = ::comdare::cache_engine::builder::commands;

/// ycsb_token — Hybrid-Brücke: bestehende compile-time-WorkloadKind (YCSB A–F) → runtime-profile_by_name-Token.
/// Custom_*-Ops (WorkloadKind) haben in profile_by_name kein Mess-Lastprofil-Pendant → "" (3-Mengen-
/// Vereinheitlichung = offene Design-Folge, Ledger). "" ist zugleich profile_by_name's „überspringen"-Marker.
[[nodiscard]] constexpr std::string_view ycsb_token(cmd::WorkloadKind k) noexcept {
    switch (k) {
        case cmd::WorkloadKind::YCSB_A_Read50Write50: return "A";
        case cmd::WorkloadKind::YCSB_B_Read95Write5: return "B";
        case cmd::WorkloadKind::YCSB_C_ReadOnly: return "C";
        case cmd::WorkloadKind::YCSB_D_ReadLatest: return "D";
        case cmd::WorkloadKind::YCSB_E_Scan95: return "E";
        case cmd::WorkloadKind::YCSB_F_ReadModifyWrite: return "F";
        case cmd::WorkloadKind::Custom_HotKey:
        case cmd::WorkloadKind::Custom_RangeDelete:
        case cmd::WorkloadKind::Custom_BulkInsert: return ""; // offene 3-Mengen-Vereinheitlichung
    }
    return "";
}

/// ycsb_profile_list — die 6 YCSB-Profile (A–F) als compile-time-Liste = Achse W der 2D-Mess-Matrix.
using ycsb_profile_list =
    mp::mp_list<std::integral_constant<cmd::WorkloadKind, cmd::WorkloadKind::YCSB_A_Read50Write50>,
                std::integral_constant<cmd::WorkloadKind, cmd::WorkloadKind::YCSB_B_Read95Write5>,
                std::integral_constant<cmd::WorkloadKind, cmd::WorkloadKind::YCSB_C_ReadOnly>,
                std::integral_constant<cmd::WorkloadKind, cmd::WorkloadKind::YCSB_D_ReadLatest>,
                std::integral_constant<cmd::WorkloadKind, cmd::WorkloadKind::YCSB_E_Scan95>,
                std::integral_constant<cmd::WorkloadKind, cmd::WorkloadKind::YCSB_F_ReadModifyWrite>>;

inline constexpr std::size_t ycsb_profile_count = mp::mp_size<ycsb_profile_list>::value;

/// config_for — Hybrid: compile-time-WorkloadKind (YCSB A–F) → runtime-WorkloadConfig via profile_by_name
/// (bit-identisch delegiert, golden-neutral). seed/ops bleiben runtime (dynamische Achse).
[[nodiscard]] inline WorkloadConfig config_for(cmd::WorkloadKind k, std::uint64_t seed, std::size_t ops) {
    return profile_by_name(ycsb_token(k), seed, ops);
}

// ── Self-proving (compile-time) ──────────────────────────────────────────────────────────────────────
static_assert(ycsb_profile_count == 6, "#31: 6 YCSB-Profile (A–F) als compile-time-Achse via WorkloadKind-Reuse.");
static_assert(ycsb_token(cmd::WorkloadKind::YCSB_A_Read50Write50) == std::string_view{"A"});
static_assert(ycsb_token(cmd::WorkloadKind::YCSB_E_Scan95) == std::string_view{"E"});
static_assert(ycsb_token(cmd::WorkloadKind::YCSB_F_ReadModifyWrite) == std::string_view{"F"});

// ── Achse D: die 6 Kanon-Datensätze (#25 / G-6v48, Thesis-tab:datasets — eindeutig fixiert, kein Fork) ──
enum class CanonicalDataset : std::uint8_t { Url = 0, Dna, Protein, Xml, TpcdsId, TrecTerms };

[[nodiscard]] constexpr std::string_view dataset_name(CanonicalDataset d) noexcept {
    switch (d) {
        case CanonicalDataset::Url: return "url";
        case CanonicalDataset::Dna: return "dna";
        case CanonicalDataset::Protein: return "protein";
        case CanonicalDataset::Xml: return "xml";
        case CanonicalDataset::TpcdsId: return "tpcds-id";
        case CanonicalDataset::TrecTerms: return "trec-terms";
    }
    return "";
}

using dataset_list = mp::mp_list<std::integral_constant<CanonicalDataset, CanonicalDataset::Url>,
                                 std::integral_constant<CanonicalDataset, CanonicalDataset::Dna>,
                                 std::integral_constant<CanonicalDataset, CanonicalDataset::Protein>,
                                 std::integral_constant<CanonicalDataset, CanonicalDataset::Xml>,
                                 std::integral_constant<CanonicalDataset, CanonicalDataset::TpcdsId>,
                                 std::integral_constant<CanonicalDataset, CanonicalDataset::TrecTerms>>;

inline constexpr std::size_t dataset_count = mp::mp_size<dataset_list>::value;

// ── 2D-Mess-Matrix = Achse W (YCSB A–F) × Achse D (6 Datasets) als compile-time-Kreuzprodukt (F7-Plan §4).
// Jede Zelle = mp_list<WorkloadKind-Tag, CanonicalDataset-Tag>; runtime instanziiert je Mess-Lauf (Hybrid).
using matrix_cells                             = mp::mp_product<mp::mp_list, ycsb_profile_list, dataset_list>;
inline constexpr std::size_t matrix_cell_count = mp::mp_size<matrix_cells>::value;

static_assert(dataset_count == 6, "#31: 6er-Kanon-Datensätze (#25 / G-6v48).");
static_assert(dataset_name(CanonicalDataset::TrecTerms) == std::string_view{"trec-terms"});
static_assert(matrix_cell_count == 36, "#31: 2D-Mess-Matrix = 6 Workloads (A–F) × 6 Datasets = 36 Zellen.");

} // namespace comdare::cache_engine::builder::workload_driver
