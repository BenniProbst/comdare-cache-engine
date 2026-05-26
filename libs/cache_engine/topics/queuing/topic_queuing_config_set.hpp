#pragma once
// V41.F.6.1 Topic queuing Config-Set fuer PermutationEngine (2026-05-26)
//
// @topic queuing
//
// Anders als allocator-Topic (1 Achse) hat queuing 2 Achsen — TopicConfigSet
// bietet beide separat fuer PermutationEngine + ein kombiniertes Cartesian-Product
// fuer Buffer-Strategy x Flush-Policy.

#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_registry.hpp>

#include <boost/mp11.hpp>

namespace comdare::cache_engine::queuing {

namespace mp = boost::mp11;

/// TopicConfigSet — beide Achsen Q1 + Q2 zentral, PermutationEngine-tauglich
struct TopicConfigSet {
    // axis_q1: Buffer-Strategy (NoBuffer/FIFO/LIFO/BoundedRing pilot)
    using StaticAxisVariants_Q1 = axis_q1_queuing::EnabledStrategies;

    // axis_q2: Flush-Policy (Eager/Watermark/Lazy pilot)
    using StaticAxisVariants_Q2 = axis_q2_queuing::EnabledPolicies;

    /// Default-StaticAxisVariants — PermutationEngine 1-Topic-Variante nimmt Q1
    /// (Buffer-Strategy ist die Haupt-Achse, Q2 als sub-Permutation)
    using StaticAxisVariants = StaticAxisVariants_Q1;

    /// Cartesian-Product Q1 x Q2 (Pilot: 4 x 3 = 12 Buffer-Flush-Kombinationen)
    using CartesianQ1xQ2 = mp::mp_product<
        mp::mp_list,
        StaticAxisVariants_Q1,
        StaticAxisVariants_Q2
    >;
};

}  // namespace
