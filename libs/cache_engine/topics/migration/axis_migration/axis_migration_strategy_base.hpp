#pragma once
// V41.F.6.1.R7.5.g axis_migration CRTP-StrategyBase (Goldstandard)

#include "concepts/axis_migration_concept.hpp"
#include "concepts/axis_migration_cache_engine_permutation_concept.hpp"
#include "../../axis_base.hpp"

namespace comdare::cache_engine::migration::axis_migration {

template <typename Derived>
class MigrationStrategyBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    MigrationStrategyBase() noexcept {
        static_assert(concepts::MigrationStrategy<Derived>);
        static_assert(concepts::CacheEnginePermutationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};

}  // namespace
