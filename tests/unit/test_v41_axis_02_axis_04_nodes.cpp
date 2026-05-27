// V41.F.6.1.R7.1.c+d axis_02_path_compression + axis_04_node_type Tests
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md
// @memory [[axis-gold-standard-checklist]]
// @task #716 V41.F.6.1.R7.1.c + R7.1.d

#include <gtest/gtest.h>

// axis_02
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_none.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_patricia.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_registry.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_subaxes_pc1_to_pc3.hpp>
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_flags.hpp>

// axis_04
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node4.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node16.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node48.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_node256.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_registry.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_subaxes_nt1_to_nt3.hpp>
#include <topics/nodes/axis_04_node_type/axis_04_node_type_flags.hpp>

// TopicConfigSet
#include <topics/nodes/topic_nodes_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax02 = ::comdare::cache_engine::nodes::axis_02_path_compression;
namespace ax04 = ::comdare::cache_engine::nodes::axis_04_node_type;
namespace nodes = ::comdare::cache_engine::nodes;
namespace mp   = ::boost::mp11;

// ─── axis_02 path_compression ───
TEST(R7_1_c_Axis02, AllCompressionsSatisfyCacheEnginePermutationConcept) {
    static_assert(ax02::concepts::CacheEnginePermutationStrategy<ax02::PathCompressionNone>);
    static_assert(ax02::concepts::CacheEnginePermutationStrategy<ax02::PatriciaPathCompression>);
    static_assert(ax02::concepts::CacheEnginePermutationStrategy<ax02::ByteWisePathCompression>);
    SUCCEED();
}

TEST(R7_1_c_Axis02, FlagSuffixUppercase) {
    static_assert(ax02::PathCompressionNone::flag_suffix()      == std::string_view{"NONE"});
    static_assert(ax02::PatriciaPathCompression::flag_suffix()  == std::string_view{"PATRICIA"});
    static_assert(ax02::ByteWisePathCompression::flag_suffix()  == std::string_view{"BYTE_WISE"});
    SUCCEED();
}

TEST(R7_1_c_Axis02, RegistryAllCompressions3) {
    static_assert(mp::mp_size<ax02::AllCompressions>::value == 3);
    static_assert(mp::mp_size<ax02::EnabledCompressions>::value > 0);
    SUCCEED();
}

TEST(R7_1_c_Axis02, IsEnabledMatchesWrapperFlag) {
    static_assert(ax02::is_enabled<ax02::PathCompressionNone>::value      == ax02::PathCompressionNone::enabled);
    static_assert(ax02::is_enabled<ax02::PatriciaPathCompression>::value  == ax02::PatriciaPathCompression::enabled);
    static_assert(ax02::is_enabled<ax02::ByteWisePathCompression>::value  == ax02::ByteWisePathCompression::enabled);
    SUCCEED();
}

TEST(R7_1_c_Axis02, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(ax02::flags::none_enabled),      const bool>);
    static_assert(std::is_same_v<decltype(ax02::flags::patricia_enabled),  const bool>);
    static_assert(std::is_same_v<decltype(ax02::flags::byte_wise_enabled), const bool>);
    SUCCEED();
}

TEST(R7_1_c_Axis02, CompressionRatiosDistinguished) {
    ax02::PathCompressionNone n;
    ax02::PatriciaPathCompression p;
    ax02::ByteWisePathCompression b;
    EXPECT_DOUBLE_EQ(n.compression_ratio(), 1.0);
    EXPECT_LT(p.compression_ratio(), 1.0);
    EXPECT_LT(b.compression_ratio(), 1.0);
}

// ─── axis_04 node_type ───
TEST(R7_1_d_Axis04, AllNodeTypesSatisfyCacheEnginePermutationConcept) {
    static_assert(ax04::concepts::CacheEnginePermutationStrategy<ax04::Node4Layout>);
    static_assert(ax04::concepts::CacheEnginePermutationStrategy<ax04::Node16Layout>);
    static_assert(ax04::concepts::CacheEnginePermutationStrategy<ax04::Node48Layout>);
    static_assert(ax04::concepts::CacheEnginePermutationStrategy<ax04::Node256Layout>);
    SUCCEED();
}

TEST(R7_1_d_Axis04, FlagSuffixUppercase) {
    static_assert(ax04::Node4Layout::flag_suffix()   == std::string_view{"NODE4"});
    static_assert(ax04::Node16Layout::flag_suffix()  == std::string_view{"NODE16"});
    static_assert(ax04::Node48Layout::flag_suffix()  == std::string_view{"NODE48"});
    static_assert(ax04::Node256Layout::flag_suffix() == std::string_view{"NODE256"});
    SUCCEED();
}

TEST(R7_1_d_Axis04, CapacitiesArtClassic) {
    static_assert(ax04::Node4Layout::max_capacity()   == 4);
    static_assert(ax04::Node16Layout::max_capacity()  == 16);
    static_assert(ax04::Node48Layout::max_capacity()  == 48);
    static_assert(ax04::Node256Layout::max_capacity() == 256);
    SUCCEED();
}

TEST(R7_1_d_Axis04, RegistryAllNodeTypes4) {
    static_assert(mp::mp_size<ax04::AllNodeTypes>::value == 4);
    static_assert(mp::mp_size<ax04::EnabledNodeTypes>::value > 0);
    SUCCEED();
}

TEST(R7_1_d_Axis04, FamilyIdsMatchCapacity) {
    static_assert(ax04::Node4Layout::family_id::value   == 4);
    static_assert(ax04::Node16Layout::family_id::value  == 16);
    static_assert(ax04::Node48Layout::family_id::value  == 48);
    static_assert(ax04::Node256Layout::family_id::value == 256);
    SUCCEED();
}

// ─── topic_nodes_config_set ───
TEST(R7_1_cd_Nodes, TopicConfigSetExposesBothAxes) {
    static_assert(mp::mp_size<nodes::TopicConfigSet::StaticAxisVariants_02>::value > 0);
    static_assert(mp::mp_size<nodes::TopicConfigSet::StaticAxisVariants_04>::value > 0);
    static_assert(std::is_same_v<nodes::TopicConfigSet::StaticAxisVariants,
                                  nodes::TopicConfigSet::StaticAxisVariants_04>);
    SUCCEED();
}

TEST(R7_1_cd_Nodes, CartesianCompressionTimesNodeTypeIsProduct) {
    constexpr auto comp_count = mp::mp_size<nodes::TopicConfigSet::StaticAxisVariants_02>::value;
    constexpr auto node_count = mp::mp_size<nodes::TopicConfigSet::StaticAxisVariants_04>::value;
    constexpr auto prod_count = mp::mp_size<nodes::TopicConfigSet::CartesianCompression02xNodeType04>::value;
    static_assert(prod_count == comp_count * node_count);
    SUCCEED();
}
