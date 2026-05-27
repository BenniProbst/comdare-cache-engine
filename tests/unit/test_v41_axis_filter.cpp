// V41.F.6.1.R7.5.e axis_filter Tests (Goldstandard mit TYPED_TEST_SUITE analog Allocator)
//
// Pattern aus axis_06_allocator: mp_apply<ToGTestTypes, AllFilters> generiert
// ::testing::Types<...> automatisch. TYPED_TEST_SUITE-Tests skalieren mit N
// Wrappers ohne Test-Code-Aenderung pro neuem Wrapper.
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.e (Optional-Topics: filter, TYPED_TEST_SUITE intro)

#include <gtest/gtest.h>

#include <topics/filter/axis_filter/axis_filter_bloom.hpp>
#include <topics/filter/axis_filter/axis_filter_cuckoo.hpp>
#include <topics/filter/axis_filter/axis_filter_range_surf.hpp>
#include <topics/filter/axis_filter/axis_filter_xor.hpp>
#include <topics/filter/axis_filter/axis_filter_registry.hpp>
#include <topics/filter/axis_filter/axis_filter_subaxes_ft1_to_ft3.hpp>
#include <topics/filter/axis_filter/axis_filter_flags.hpp>
#include <topics/filter/topic_filter_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax_filter = ::comdare::cache_engine::filter::axis_filter;
namespace flt      = ::comdare::cache_engine::filter;
namespace mp       = ::boost::mp11;

// ─── TYPED_TEST_SUITE Pattern (axis_06_allocator-Goldstandard) ───
// mp_apply<ToGTestTypes, AllFilters> wandelt mp_list<...> in ::testing::Types<...>
template <class... Fs>
using ToGTestTypes = ::testing::Types<Fs...>;

using AllFilterTypes = mp::mp_apply<ToGTestTypes, ax_filter::AllFilters>;

template <class T>
class FilterWrapperTest : public ::testing::Test {};

TYPED_TEST_SUITE(FilterWrapperTest, AllFilterTypes);

TYPED_TEST(FilterWrapperTest, ConceptConformance) {
    static_assert(ax_filter::concepts::FilterStrategy<TypeParam>);
    static_assert(ax_filter::concepts::CacheEnginePermutationStrategy<TypeParam>);
    SUCCEED();
}

TYPED_TEST(FilterWrapperTest, HasTopicTagAndFamilyId) {
    using topic_tag_t = typename TypeParam::topic_tag;
    using family_id_t = typename TypeParam::family_id;
    static_assert(family_id_t::value > 0);
    static_assert(std::is_same_v<topic_tag_t,
                                 ::comdare::cache_engine::filter::concepts::FilterTopicTag>);
    SUCCEED();
}

TYPED_TEST(FilterWrapperTest, NameNonEmpty) {
    static_assert(!TypeParam::name().empty());
    static_assert(!TypeParam::family_name().empty());
    static_assert(!TypeParam::flag_suffix().empty());
    SUCCEED();
}

TYPED_TEST(FilterWrapperTest, IsEnabledMatchesFlag) {
    static_assert(ax_filter::is_enabled<TypeParam>::value == TypeParam::enabled);
    SUCCEED();
}

// 4 Wrappers × 4 TYPED_TESTs = 16 typed Tests (skaliert mit N)

// ─── Spezifische Verhaltens-Tests (5) ───

TEST(R7_5_e_Axis_Filter_Specific, SupportsRangeQueryDifferentiated) {
    // Nur RangeSurf unterstuetzt Range-Query (Zhang SIGMOD 2018)
    static_assert(ax_filter::BloomFilter::supports_range_query()      == false);
    static_assert(ax_filter::CuckooFilter::supports_range_query()     == false);
    static_assert(ax_filter::RangeSurfFilter::supports_range_query()  == true);
    static_assert(ax_filter::XorFilter::supports_range_query()        == false);
    SUCCEED();
}

TEST(R7_5_e_Axis_Filter_Specific, FlagSuffixUppercase) {
    static_assert(ax_filter::BloomFilter::flag_suffix()      == std::string_view{"BLOOM"});
    static_assert(ax_filter::CuckooFilter::flag_suffix()     == std::string_view{"CUCKOO"});
    static_assert(ax_filter::RangeSurfFilter::flag_suffix()  == std::string_view{"RANGE_SURF"});
    static_assert(ax_filter::XorFilter::flag_suffix()        == std::string_view{"XOR"});
    SUCCEED();
}

TEST(R7_5_e_Axis_Filter_Specific, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax_filter::BloomFilter::axis_tag,      ax_filter::subaxes::query_type_tag>);
    static_assert(std::is_same_v<ax_filter::CuckooFilter::axis_tag,     ax_filter::subaxes::mutability_tag>);
    static_assert(std::is_same_v<ax_filter::RangeSurfFilter::axis_tag,  ax_filter::subaxes::query_type_tag>);
    static_assert(std::is_same_v<ax_filter::XorFilter::axis_tag,        ax_filter::subaxes::error_profile_tag>);
    SUCCEED();
}

TEST(R7_5_e_Axis_Filter_Specific, RegistryHas4Filters) {
    static_assert(mp::mp_size<ax_filter::AllFilters>::value == 4);
    static_assert(mp::mp_size<ax_filter::EnabledFilters>::value > 0);
    SUCCEED();
}

TEST(R7_5_e_Filter, TopicConfigSetExposesAxisFilter) {
    static_assert(mp::mp_size<flt::TopicConfigSet::StaticAxisVariants_F>::value > 0);
    static_assert(std::is_same_v<flt::TopicConfigSet::StaticAxisVariants,
                                  flt::TopicConfigSet::StaticAxisVariants_F>);
    SUCCEED();
}

// Total: 4 Wrappers × 4 TYPED = 16 typed + 5 spezifisch = 21 Tests
