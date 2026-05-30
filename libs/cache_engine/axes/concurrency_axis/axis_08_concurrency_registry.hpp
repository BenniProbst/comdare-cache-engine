#pragma once
// V41.F.6.1.R7.3 axis_08_concurrency Zentrale Registry

#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>

#include "axis_08_concurrency_none.hpp"
#include "axis_08_concurrency_blocking.hpp"
#include "axis_08_concurrency_reader_writer.hpp"
#include "axis_08_concurrency_olc.hpp"
#include "axis_08_concurrency_lock_free.hpp"
#include "axis_08_concurrency_wait_free.hpp"
#include "axis_08_concurrency_rcu.hpp"
#include "axis_08_concurrency_hazard_pointer.hpp"
#include "axis_08_concurrency_olc_reserved_blocks.hpp"

#include <boost/mp11.hpp>
#include <type_traits>

namespace comdare::cache_engine::concurrency_axis {

namespace mp = boost::mp11;

using AllStrategies = mp::mp_list<
    NoneConcurrency,
    BlockingConcurrency,
    ReaderWriterConcurrency,
    OlcOptimisticConcurrency,
    LockFreeConcurrency,
    WaitFreeConcurrency,
    RcuConcurrency,
    HazardPointerConcurrency,
    OlcReservedBlocksConcurrency
>;

template <typename T>
using is_enabled = mp::mp_bool<T::enabled>;

using EnabledStrategies = mp::mp_filter<is_enabled, AllStrategies>;

static_assert(mp::mp_size<EnabledStrategies>::value > 0,
    "Axis 08 Concurrency: at least one strategy must be enabled");

}  // namespace
