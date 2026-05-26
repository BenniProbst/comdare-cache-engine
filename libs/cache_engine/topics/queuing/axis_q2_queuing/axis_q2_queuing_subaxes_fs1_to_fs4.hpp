#pragma once
// V41.F.6.1 axis_q2_queuing Subaxis-Tags FS1-FS3 (2026-05-26)
// @topic queuing @achse Q2

namespace comdare::cache_engine::queuing::axis_q2_queuing::subaxes {

struct event_triggered_tag {};   // FS1 — eager_per_op / lazy_on_evict
struct threshold_triggered_tag {}; // FS2 — watermark
struct time_triggered_tag {};    // FS3 — time_window
struct adaptive_triggered_tag {}; // FS4 — adaptive_lsm

}  // namespace
