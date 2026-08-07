#pragma once
// STRUKT-R ORG-18 axis_persistence_target MemoryOnlyTarget (Durchreich-/Baseline-Baustein)
// @topic io @achse persistence_target @subaxis PT1 residency
//
// Session-Doc §5 (Owner-KERN): "memory_only (Default, golden_wired, Durchreich-Semantik)". Damit ist
// dieser Baustein der Wert, den JEDE bestehende Komposition traegt -- er MUSS byte-neutral im Verhalten
// sein (er fuegt keinen Pfad hinzu, er stellt fest, dass keiner existiert).
//
// Durchreich-Koerper 1:1 nach axes/migration_policy/axis_migration_none.hpp (NoMigration): family_id 0,
// is-Praedikat false, Mess-Op mit ehrlichem `return 0` als echte Baseline (bewusst leerer Pfad, NICHT n/a).

#include "axis_persistence_target_strategy_base.hpp"
#include "axis_persistence_target_subaxes_pt1_to_pt2.hpp"
#include "concepts/axis_persistence_target_cache_engine_permutation_concept.hpp"
#include <axes/persistence_target/axis_persistence_target_flags.hpp>
#include <topics/io/concepts/topic_io_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::persistence_target {

/// MemoryOnlyTarget -- Default: der Index lebt ausschliesslich im RAM, es gibt KEINEN Rueckschreib-Pfad.
/// Vergleichs-Nullpunkt der persistence_target-Achse (golden_wired-Wert).
class MemoryOnlyTarget : public PersistenceTargetStrategyBase<MemoryOnlyTarget> {
public:
    using topic_tag = ::comdare::cache_engine::io::concepts::IoTopicTag;
    using axis_tag  = subaxes::residency_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::memory_only_enabled;

    [[nodiscard]] static constexpr bool             writes_back_to_disk() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "persistence_memory_only"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::persistence_target::MemoryOnlyTarget",
                                  "axes/persistence_target/axis_persistence_target_memory_only.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "MemoryOnlyTarget (RAM-only, kein Rueckschreib-Pfad, Baseline)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "MEMORY_ONLY"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    // Verhaltens-tragende Mess-Op der persistence_target-Achse (F15-operativ, Seg-Timer-Traeger).
    // EHRLICHKEIT (Praezedenz axis_migration_none.hpp): gemessen wird die Rueckschreib-Last. MemoryOnly hat
    // per Definition KEINE -> `return 0` ist die echte Baseline, ein bewusst leerer Pfad, NICHT "n/a" und
    // keine erfundene Konstante. buf/n/record_size bleiben absichtlich ungenutzt. Der zugehoerige Seg-Timer
    // misst damit den Vergleichs-Nullpunkt der Achse (Instrumentierungs-Rest), was genau die Aussage ist.
    [[nodiscard]] static std::uint64_t persistence_writeback_scan(unsigned char const* /*buf*/, std::size_t /*n*/,
                                                                  std::size_t /*record_size*/) noexcept {
        return 0;
    }
};

} // namespace comdare::cache_engine::persistence_target

namespace comdare::cache_engine::persistence_target {
static_assert(concepts::PersistenceTargetStrategy<MemoryOnlyTarget>);
static_assert(concepts::CacheEnginePermutationStrategy<MemoryOnlyTarget>);
} // namespace comdare::cache_engine::persistence_target
