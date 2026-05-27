#pragma once
// V41.F.6.1.R7.5.e axis_filter Subaxes-Tags

namespace comdare::cache_engine::filter::axis_filter::subaxes {

// FT1: Query-Type (point / range)
struct query_type_tag {};

// FT2: Mutability (immutable / mutable)
struct mutability_tag {};

// FT3: Error-Profile (false-positive / approximative / lossless)
struct error_profile_tag {};

}  // namespace
