#pragma once
// V41.F.2 Forwarding-Header (Stufe 2): Achse physisch nach axes/telemetry_axis/ migriert.
#include <axes/telemetry_axis/axis_11_telemetry_leaf_only.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_observable.hpp>  // V42 L-74c: ObservableTelemetry-Huelle mitgeliefert
namespace comdare::cache_engine::telemetry::axis_11_telemetry { using namespace comdare::cache_engine::telemetry_axis; }
