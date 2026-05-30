#pragma once
// V41.F.6.1.R7.5.a axis_07_prefetch Subaxes-Tags

namespace comdare::cache_engine::prefetch_axis::subaxes {

// PF1: Trigger-Mechanismus (none / compile-hint / hw-instruction / observed-pattern)
struct trigger_mechanism_tag {};

// PF2: Distance-Heuristik (none / linear / adaptive)
struct distance_heuristic_tag {};

// PF3: Granularitaet (cache-line / page / bundle)
struct granularity_tag {};

}  // namespace
