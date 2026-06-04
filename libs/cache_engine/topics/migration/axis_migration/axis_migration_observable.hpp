#pragma once
// Phase B (2026-06-04) Topic-Forwarding-Header für die migration_policy-Observer-Hülle (T15). Reicht die in axes/
// definierte ObservableMigration<Strategy> in den Topic-Namespace durch (Parität zu axis_02_path_compression_
// observable.hpp / axis_01_index_organization_observable.hpp).
#include <axes/migration_policy/axis_migration_observable.hpp>
namespace comdare::cache_engine::migration::axis_migration { using namespace comdare::cache_engine::migration_policy; }
