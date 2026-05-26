#pragma once
#include "concepts/axis_migration_concept.hpp"
#include "../../axis_base.hpp"
namespace comdare::cache_engine::migration::axis_migration {
template <typename Derived> class MigrationBase : public ::comdare::cache_engine::topics::AxisBase {
protected:
    MigrationBase() noexcept {
        static_assert(concepts::MigrationStrategy<Derived>);
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>);
    }
};
}  // namespace
