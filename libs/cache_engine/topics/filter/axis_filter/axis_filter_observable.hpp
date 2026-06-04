#pragma once
// Phase B (2026-06-04) Topic-Forwarding-Header für die filter-Observer-Hülle (T16). Reicht die in axes/ definierte
// ObservableFilter<Strategy> in den Topic-Namespace durch (Parität zu axis_02_path_compression_observable.hpp /
// axis_01_index_organization_observable.hpp).
#include <axes/filter_axis/axis_filter_observable.hpp>
namespace comdare::cache_engine::filter::axis_filter { using namespace comdare::cache_engine::filter_axis; }
