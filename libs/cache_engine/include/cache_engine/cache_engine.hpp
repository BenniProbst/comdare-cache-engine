#pragma once
// cache_engine.hpp - Public Aggregation-Header
// Reicht aus, um cache_engine als Konsument einzubinden.

// U09-Pipeline-Konzepte (comdare::cache_engine::*).
#include "cache_engine/concepts/cache_recommendation.hpp"
#include "cache_engine/concepts/i_cache_engine.hpp"
#include "cache_engine/concepts/i_sub_engine.hpp"
#include "cache_engine/concepts/platform_snapshot.hpp"
#include "cache_engine/concepts/pressure_state.hpp"
#include "cache_engine/concepts/request_context.hpp"
#include "cache_engine/platform_probe/cpuid_probe.hpp"

// F5.R1 (#35): Master-Framework-Fassade + Abstract-Factory-Slot (comdare::cache_engine::api::*).
// Über diese EINE Tür linkt ein Konsument NUR gegen cache-engine und vermittelt die Subsysteme.
#include "cache_engine/api/i_cache_engine.hpp"
#include "cache_engine/api/i_cache_engine_tools.hpp"
#include "cache_engine/api/i_pruefling_factory.hpp"
#include "cache_engine/api/pruefling_registry.hpp"
