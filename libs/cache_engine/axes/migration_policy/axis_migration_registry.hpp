#pragma once
// V41.F.6.1.R7.5.g axis_migration Zentrale Registry

#include <axes/migration_policy/axis_migration_flags.hpp>

#include "axis_migration_none.hpp"
#include "axis_migration_hot_cold.hpp"
#include "axis_migration_tier_based.hpp"
#include "axis_migration_adaptive.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::migration_policy {

namespace mp = boost::mp11;

using AllMigrations = mp::mp_list<NoMigration, HotColdMigration, TierBasedMigration, AdaptiveMigration>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledMigrations = mp::mp_filter<is_enabled, AllMigrations>;

static_assert(mp::mp_size<EnabledMigrations>::value > 0, "Axis Migration: at least one migration must be enabled");

} // namespace comdare::cache_engine::migration_policy
