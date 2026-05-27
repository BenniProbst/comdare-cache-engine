#pragma once
// V41.F.6.1.R7.1.c+d TopicConfigSet fuer nodes-Topic (2 Achsen)
//
// @topic nodes (axis_02 path_compression + axis_04 node_type)
//
// 2-Achsen Topic analog queuing. TopicConfigSet bietet beide Achsen separat
// + Cartesian-Product fuer kombinierte Permutationen.

#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_registry.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_registry.hpp>

#include <boost/mp11.hpp>
#include <array>
#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::nodes {

namespace mp = boost::mp11;

/// TopicConfigSet — zentrale Konfiguration fuer Topic `nodes`.
/// 2 Achsen: axis_02 (PathCompression) + axis_04 (NodeType).
struct TopicConfigSet {
    using StaticAxisVariants_02 = axis_02_path_compression::EnabledCompressions;
    using StaticAxisVariants_04 = axis_04_node_type::EnabledNodeTypes;

    // Default = axis_04 (NodeType ist Hauptachse, PathCompression Sub-Variante)
    using StaticAxisVariants = StaticAxisVariants_04;

    // Cartesian-Product axis_02 x axis_04
    using CartesianCompression02xNodeType04 = mp::mp_product<
        mp::mp_list,
        StaticAxisVariants_02,
        StaticAxisVariants_04
    >;

    template <class Wrapper>
    using AspectIterations = std::conditional_t<
        requires { typename Wrapper::iterable_aspect_t; },
        void,
        void
    >;

    template <class /*Wrapper*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

}  // namespace
