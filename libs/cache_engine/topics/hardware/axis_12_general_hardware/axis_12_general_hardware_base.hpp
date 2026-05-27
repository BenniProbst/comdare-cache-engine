#pragma once
// V41.F.6.1.R7.1.a.2 axis_12 General-Hardware CRTP-Base (Goldstandard-konform)
//
// Analog axis_06_allocator_strategy_base: 3-fach Concept-Check im Konstruktor.
//   - GeneralHardwareStrategy           — Plattform-Properties
//   - CacheEnginePermutationStrategy    — axis_tag + family_id + name + flag_suffix + enabled
//   - AxisBaseConcept                   — get_compiler() Default-API
#include "concepts/axis_12_general_hardware_concept.hpp"
#include "concepts/axis_12_general_hardware_cache_engine_permutation_concept.hpp"
#include "axis_12_general_hardware_subaxes_hw1_to_hw4.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::hardware::axis_12_general_hardware {

template <typename Derived>
class GeneralHardwareBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    GeneralHardwareBase() noexcept {
        static_assert(concepts::GeneralHardwareStrategy<Derived>,
            "Derived must satisfy GeneralHardwareStrategy concept "
            "(see concepts/axis_12_general_hardware_concept.hpp)");
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>,
            "Derived must satisfy CacheEnginePermutationStrategy concept "
            "(see concepts/axis_12_general_hardware_cache_engine_permutation_concept.hpp)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
            "Derived must satisfy AxisBaseConcept (get_compiler() Default-API). "
            "GeneralHardwareBase erbt von AxisBase — Derived bekommt Default 'original' automatisch.");
    }
};

}  // namespace comdare::cache_engine::hardware::axis_12_general_hardware
