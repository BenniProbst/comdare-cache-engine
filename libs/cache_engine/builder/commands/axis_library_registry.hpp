#pragma once
// V32.HH.1 (2026-05-18 spaet) - Lookup-Tabelle pro Achse fuer AutoPermutator
//
// @subsystem CEB
// @phase_owner CEB

#include "auto_permutator.hpp"
#include <vector>

namespace comdare::cache_engine::builder::commands {

/**
 * @brief AxisLibraryRegistry - statische Lookup-Tabelle pro Achse 1-14
 * @subsystem CEB
 *
 * Wird vom AutoPermutator.discover_axis_implementations() aufgerufen.
 * Liefert pro Achsen-ID die Liste verfuegbarer SOTA-Bausteine.
 *
 * V32.HH.1 Implementierung: hardcoded Tabelle.
 * V33+ Folge: Doxygen-Tag-Extraktion via Custom-Script (BB.1 Konvention).
 */
class AxisLibraryRegistry {
public:
    /// Gibt alle verfuegbaren Variants fuer eine Achse zurueck
    [[nodiscard]] static std::vector<AxisVariant> lookup(std::string_view axis_id) {
        std::vector<AxisVariant> result;

        if (axis_id == "12.1") {  // SIMD-Family
            result.push_back({"12.1", "Scalar", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::Scalar"});
            result.push_back({"12.1", "AVX2", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::AVX2"});
            result.push_back({"12.1", "AVX512", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::AVX512"});
            result.push_back({"12.1", "NEON", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::NEON"});
            result.push_back({"12.1", "SVE2", "cache_engine/concepts/hardware_strategy.hpp::SimdFamily::SVE2"});
        } else if (axis_id == "12.2") {  // Cache-Level-Targeting
            result.push_back({"12.2", "L1Aware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::L1Aware"});
            result.push_back({"12.2", "L2Aware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::L2Aware"});
            result.push_back({"12.2", "L3Aware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::L3Aware"});
            result.push_back({"12.2", "HBMAware", "cache_engine/concepts/hardware_strategy.hpp::CacheLevelTarget::HBMAware"});
        } else if (axis_id == "12.3") {  // NUMA-Strategy
            result.push_back({"12.3", "Local", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Local"});
            result.push_back({"12.3", "Interleave", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Interleave"});
            result.push_back({"12.3", "Preferred", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Preferred"});
            result.push_back({"12.3", "Bind", "cache_engine/concepts/hardware_strategy.hpp::NumaStrategy::Bind"});
        } else if (axis_id == "11") {  // Telemetry (Kuehn 11.X1-X4)
            result.push_back({"11", "LeafOnlyCounter", "cache_engine/concepts/telemetry/leaf_only_counter.hpp"});
            result.push_back({"11", "LeafOnlySampledCounter", "cache_engine/concepts/telemetry/leaf_only_sampled_counter.hpp"});
            result.push_back({"11", "RetroactiveAggregator", "cache_engine/concepts/telemetry/retroactive_aggregation.hpp"});
            result.push_back({"11", "PathReadCounter", "cache_engine/concepts/telemetry/path_read_counter.hpp"});
            // PerNodeCounter (11.X4 ANTI-PATTERN) bewusst nicht im default-permutationsraum
        } else if (axis_id == "6.2") {  // Reclamation-Policy
            result.push_back({"6.2", "EpochBased", "cache_engine/concepts/mechanics/comdare_rcu_mechanic.hpp"});
            result.push_back({"6.2", "RCU", "cache_engine/reclamation/rcu_reclaim/"});
            result.push_back({"6.2", "HazardPointer", "adapters/P30-HazardPointers/"});
            result.push_back({"6.2", "QSBR", "cache_engine/concepts/mechanics/qsbr_mechanic.hpp"});
        } else if (axis_id == "13.1") {  // Worker-Pool-Layout
            result.push_back({"13.1", "ThreadPerCore", "cache_engine/concepts/scheduling_strategy.hpp::WorkerPoolLayout::ThreadPerCore"});
            result.push_back({"13.1", "WorkStealing", "cache_engine/concepts/scheduling_strategy.hpp::WorkerPoolLayout::WorkStealing"});
            result.push_back({"13.1", "CpuPinning", "cache_engine/concepts/scheduling_strategy.hpp::WorkerPoolLayout::CpuPinning"});
        } else if (axis_id == "13.2") {  // SIMD-Worker-Count-Limit
            result.push_back({"13.2", "Limit_1", "1 SIMD-Worker"});
            result.push_back({"13.2", "Limit_2", "2 SIMD-Worker (Default, Hardware-Limit)"});
            result.push_back({"13.2", "Limit_4", "4 SIMD-Worker (auf Hardware mit mehr SIMD-Einheiten)"});
        }
        // V33+ Sprint: weitere Achsen 1-10 + 14

        return result;
    }
};

// AutoPermutator::discover_axis_implementations() Implementation (HH.1)
inline void AutoPermutator::discover_axis_implementations() {
    available_variants_ = AxisLibraryRegistry::lookup(axis_id_);
}

}  // namespace comdare::cache_engine::builder::commands
