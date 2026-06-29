#pragma once
// V41.F.6.1.R7.5.g axis_migration Subaxes-Tags

namespace comdare::cache_engine::migration_policy::subaxes {

// MG1: Trigger (none / threshold / observation)
struct trigger_tag {};

// MG2: Direction (none / up / down / bidirectional)
struct direction_tag {};

// MG3: Granularity (none / page / block / object)
struct granularity_tag {};

} // namespace comdare::cache_engine::migration_policy::subaxes
