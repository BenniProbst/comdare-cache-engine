#pragma once
// Phase B (2026-06-04) Topic-Forwarding-Header für die io_dispatch-Observer-Hülle (T14). Reicht die in axes/
// definierte ObservableIoDispatch<Strategy> in den Topic-Namespace durch (Parität zu axis_02_path_compression_
// observable.hpp / axis_01_index_organization_observable.hpp).
#include <axes/io_dispatch/axis_io_dispatch_observable.hpp>
namespace comdare::cache_engine::io::axis_io { using namespace comdare::cache_engine::io_dispatch; }
