#pragma once
// cache_engine.hpp - Public Aggregation-Header
// Reicht aus, um cache_engine als Konsument einzubinden.

#include "cache_engine/concepts/cache_recommendation.hpp"
#include "cache_engine/concepts/i_cache_engine.hpp"
#include "cache_engine/concepts/i_sub_engine.hpp"
#include "cache_engine/concepts/platform_snapshot.hpp"
#include "cache_engine/concepts/pressure_state.hpp"
#include "cache_engine/concepts/request_context.hpp"
#include "cache_engine/platform_probe/cpuid_probe.hpp"
