#pragma once
// #31-Schritt-1 (F7 Option A, User-Hybrid-Direktive 2026-07-08): comdare::…::workload_driver-Kopf-Interface
// für die Workload-Achse der 2D-Mess-Matrix (Mess-Frameworks × Workloads).
//
// **HYBRID compile-time + runtime (User 08.07.: „die runtime Profile müssen ein Hybrid aus compile-time und
// runtime werden, um dem Experiment-Baum zu entsprechen"):** die bestehende RUNTIME-Profil-Auswahl
// `profile_by_name` (String-Token → WorkloadConfig, seed/ops runtime) bleibt UNVERÄNDERT; darüber kommt eine
// additive COMPILE-TIME-STRUKTUR — `WorkloadProfile`-Enum + `profile_list` (mp11), die die Workload-Achse
// wie den Experiment-Baum (experiment_tree.hpp) compile-time-iterierbar macht (2D-Matrix = Mess-Kategorie ×
// Workload als Kreuzprodukt). `config_for<P>` ist die Brücke: compile-time-Tag → runtime-WorkloadConfig
// (delegiert bit-identisch an profile_by_name).
//
// **ADDITIV & golden/ABI-NEUTRAL:** profile_by_name/WorkloadConfig/make_*-Profile UNBERÜHRT; die Mess-Matrix
// ist orthogonal zur Anatomie-Permutation (golden_fullpilot_320 / permutation_axes / ABI-4 unberührt, F7-Plan
// docs/architektur/15_F7_…:59). Analog #29-Schritt-1 (container_framework.hpp): Kopf-Interface über Bestand,
// KEIN Umbau, nimmt die Option-A-Voll-Realisierung (Dataset-Kreuz, Kategorie-Achse) NICHT vorweg.

#include "workload_profiles.hpp" // profile_by_name (runtime) + WorkloadConfig

#include <boost/mp11.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::builder::workload_driver {

namespace mp = boost::mp11;

/// WorkloadProfile — die dynamische Mess-Achse „Workload" als compile-time-Tag. Die 8 heutigen
/// profile_by_name-Profile (YCSB-treu A/B/C/D/E/F + Insert/Lookup-Heavy) als Enum, compile-time-iterierbar.
enum class WorkloadProfile : std::uint8_t {
    MixedA = 0,  ///< "A" (YCSB-A, 50/50 Zipf)
    MixedB,      ///< "B" (YCSB-B, read-heavy)
    YcsbC,       ///< "C" (read-only)
    YcsbD,       ///< "D" (read-latest)
    YcsbE,       ///< "E" (range-scan)
    YcsbF,       ///< "F" (read-modify-write)
    InsertHeavy, ///< "IH"
    LookupHeavy  ///< "LH"
};

/// profile_token — HYBRID-Brücke: compile-time-Tag → das exakte runtime-Token für profile_by_name.
[[nodiscard]] constexpr std::string_view profile_token(WorkloadProfile p) noexcept {
    switch (p) {
        case WorkloadProfile::MixedA: return "A";
        case WorkloadProfile::MixedB: return "B";
        case WorkloadProfile::YcsbC: return "C";
        case WorkloadProfile::YcsbD: return "D";
        case WorkloadProfile::YcsbE: return "E";
        case WorkloadProfile::YcsbF: return "F";
        case WorkloadProfile::InsertHeavy: return "IH";
        case WorkloadProfile::LookupHeavy: return "LH";
    }
    return "";
}

/// profile_list — compile-time-Liste aller Workload-Profile (mp11), Iterations-Basis der 2D-Matrix-Achse W.
using profile_list = mp::mp_list<std::integral_constant<WorkloadProfile, WorkloadProfile::MixedA>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::MixedB>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::YcsbC>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::YcsbD>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::YcsbE>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::YcsbF>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::InsertHeavy>,
                                 std::integral_constant<WorkloadProfile, WorkloadProfile::LookupHeavy>>;

inline constexpr std::size_t profile_count = mp::mp_size<profile_list>::value;

/// config_for — HYBRID: compile-time-Tag → runtime-WorkloadConfig, delegiert BIT-IDENTISCH an profile_by_name
/// (golden-neutral, gleiche (Token,seed,ops) ⇒ gleiche Op-Sequenz). seed/ops bleiben runtime (dynamische Achse).
[[nodiscard]] inline WorkloadConfig config_for(WorkloadProfile p, std::uint64_t seed, std::size_t ops) {
    return profile_by_name(profile_token(p), seed, ops);
}

// ── Self-proving (compile-time; kein Raten) ─────────────────────────────────────────────────────────
static_assert(profile_count == 8, "#31: 8 Workload-Profile (A/B/C/D/E/F/IH/LH).");
static_assert(profile_token(WorkloadProfile::MixedA) == std::string_view{"A"});
static_assert(profile_token(WorkloadProfile::YcsbE) == std::string_view{"E"});
static_assert(profile_token(WorkloadProfile::InsertHeavy) == std::string_view{"IH"});
// Hybrid-Brücke ist total: jeder compile-time-Tag mappt auf ein nicht-leeres runtime-Token.
static_assert(
    [] {
        for (auto p : {WorkloadProfile::MixedA, WorkloadProfile::MixedB, WorkloadProfile::YcsbC, WorkloadProfile::YcsbD,
                       WorkloadProfile::YcsbE, WorkloadProfile::YcsbF, WorkloadProfile::InsertHeavy,
                       WorkloadProfile::LookupHeavy}) {
            if (profile_token(p).empty()) { return false; }
        }
        return true;
    }(),
    "#31: jeder Profil-Tag hat ein runtime-Token (Hybrid-Brücke total).");

} // namespace comdare::cache_engine::builder::workload_driver
