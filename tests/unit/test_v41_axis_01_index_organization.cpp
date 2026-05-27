// V41.F.6.1.R7.5.h axis_01_index_organization Tests (Clustering + TYPED_TEST_SUITE)
//
// User-Direktive 2026-05-27: Wrappers sind ORGANE (Clustering-Strategien),
// nicht GATTUNGEN (std-Container-Familien). Container-Familien (std::map,
// std::set, etc.) leben in Doku 14 §26 als Anatomie-Gattungen.
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.h

#include <gtest/gtest.h>

#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_heap.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_clustered.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_non_clustered.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_index_organized_table.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_registry.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_subaxes_io1_to_io3.hpp>
#include <topics/search_engine/axis_01_index_organization/axis_01_index_organization_flags.hpp>
#include <topics/search_engine/topic_search_engine_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax01 = ::comdare::cache_engine::search_engine::axis_01_index_organization;
namespace se   = ::comdare::cache_engine::search_engine;
namespace mp   = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern (axis_06_allocator-Goldstandard) ───
template <class... Os>
using ToGTestTypes = ::testing::Types<Os...>;

using AllOrganizationTypes = mp::mp_apply<ToGTestTypes, ax01::AllOrganizations>;

template <class T>
class IndexOrganizationWrapperTest : public ::testing::Test {};

TYPED_TEST_SUITE(IndexOrganizationWrapperTest, AllOrganizationTypes);

TYPED_TEST(IndexOrganizationWrapperTest, ConceptConformance) {
    static_assert(ax01::concepts::IndexOrganizationStrategy<TypeParam>);
    static_assert(ax01::concepts::CacheEnginePermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(IndexOrganizationWrapperTest, HasTopicTagAndFamilyId) {
    using topic_tag_t = typename TypeParam::topic_tag;
    static_assert(std::is_same_v<topic_tag_t,
                                 ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag>);
    SUCCEED();
}

TYPED_TEST(IndexOrganizationWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(IndexOrganizationWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax01::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 4 Wrappers × 4 TYPED_TESTs = 16 typed Tests

// ─── Spezifische Verhaltens-Tests (5) ───

TEST(R7_5_h_Axis_01_Specific, ClusteringMatrix) {
    // Heap: kein Index → is_clustered=false, has_secondary=false, data_embedded=false
    static_assert(ax01::HeapIndexOrganization::is_clustered()           == false);
    static_assert(ax01::HeapIndexOrganization::has_secondary_indexes()  == false);
    static_assert(ax01::HeapIndexOrganization::data_embedded_in_leaf()  == false);
    // Clustered: Index-Order=Storage-Order, 1 PK, Daten separat
    static_assert(ax01::ClusteredIndexOrganization::is_clustered()           == true);
    static_assert(ax01::ClusteredIndexOrganization::has_secondary_indexes()  == false);
    static_assert(ax01::ClusteredIndexOrganization::data_embedded_in_leaf()  == false);
    // NonClustered: Secondary Index, N pro Tabelle
    static_assert(ax01::NonClusteredIndexOrganization::is_clustered()           == false);
    static_assert(ax01::NonClusteredIndexOrganization::has_secondary_indexes()  == true);
    static_assert(ax01::NonClusteredIndexOrganization::data_embedded_in_leaf()  == false);
    // IOT: clustered + data embedded (Oracle IOT, SQL Server CLUSTERED)
    static_assert(ax01::IotIndexOrganization::is_clustered()           == true);
    static_assert(ax01::IotIndexOrganization::has_secondary_indexes()  == true);
    static_assert(ax01::IotIndexOrganization::data_embedded_in_leaf()  == true);
    SUCCEED();
}

TEST(R7_5_h_Axis_01_Specific, FlagSuffixUppercase) {
    static_assert(ax01::HeapIndexOrganization::flag_suffix()         == std::string_view{"HEAP"});
    static_assert(ax01::ClusteredIndexOrganization::flag_suffix()    == std::string_view{"CLUSTERED"});
    static_assert(ax01::NonClusteredIndexOrganization::flag_suffix() == std::string_view{"NON_CLUSTERED"});
    static_assert(ax01::IotIndexOrganization::flag_suffix()      == std::string_view{"INDEX_ORGANIZED_TABLE"});
    SUCCEED();
}

TEST(R7_5_h_Axis_01_Specific, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax01::HeapIndexOrganization::axis_tag,         ax01::subaxes::storage_order_tag>);
    static_assert(std::is_same_v<ax01::ClusteredIndexOrganization::axis_tag,    ax01::subaxes::storage_order_tag>);
    static_assert(std::is_same_v<ax01::NonClusteredIndexOrganization::axis_tag, ax01::subaxes::index_count_tag>);
    static_assert(std::is_same_v<ax01::IotIndexOrganization::axis_tag,      ax01::subaxes::data_embedding_tag>);
    SUCCEED();
}

TEST(R7_5_h_Axis_01_Specific, RegistryHas4Organizations) {
    static_assert(mp::mp_size<ax01::AllOrganizations>::value == 4);
    static_assert(mp::mp_size<ax01::EnabledOrganizations>::value > 0);
    SUCCEED();
}

TEST(R7_5_h_SearchEngine, TopicConfigSetExposesAxis01) {
    static_assert(mp::mp_size<se::TopicConfigSet::StaticAxisVariants_01>::value > 0);
    static_assert(std::is_same_v<se::TopicConfigSet::StaticAxisVariants,
                                  se::TopicConfigSet::StaticAxisVariants_01>);
    SUCCEED();
}

// Total: 4 Wrappers × 4 TYPED = 16 typed + 5 spezifisch = 21 Tests
