#pragma once
// V41.F.6.1.R7.1.d axis_04_node_type Subaxes-Tags

namespace comdare::cache_engine::node::subaxes {

// NT1: Capacity-Class (klein/mittel/gross/maximal)
struct capacity_class_tag {};

// NT2: Access-Pattern (linear/binary/direct)
struct access_pattern_tag {};

// NT3: Compactness (sparse/dense/adaptive)
struct compactness_tag {};

}  // namespace
