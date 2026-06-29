// V41.F.6.1 F.6 axis_01_page_type Tests (NEUE Achse: 6 Pflicht-Seitentypen)
// @doku 19 (F.6 Plan) + 20 (Plugin-Controller); Planrunde 2026-05-29 (Achse-1 PAGE-TYPE)

#include <gtest/gtest.h>

#include <topics/nodes/axis_01_page_type/axis_01_page_type_dense_byte.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_extended_dense.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_sparse_patricia.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_redirect.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_custom_cache.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_bplus.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_registry.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_subaxes_pg1_to_pg3.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_flags.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace pt = ::comdare::cache_engine::nodes::axis_01_page_type;
namespace mp = ::boost::mp11;
using PK     = pt::concepts::PageKind;

TEST(F6_Axis01PageType, AllSatisfyConcepts) {
    static_assert(pt::concepts::PageTypeStrategy<pt::DenseBytePageType>);
    static_assert(pt::concepts::PageTypeStrategy<pt::ExtendedDensePageType>);
    static_assert(pt::concepts::PageTypeStrategy<pt::SparsePatriciaPageType>);
    static_assert(pt::concepts::PageTypeStrategy<pt::RedirectPageType>);
    static_assert(pt::concepts::PageTypeStrategy<pt::CustomCachePageType>);
    static_assert(pt::concepts::PageTypeStrategy<pt::BPlusPageType>);
    static_assert(pt::concepts::CacheEnginePermutationStrategy<pt::DenseBytePageType>);
    static_assert(pt::concepts::CacheEnginePermutationStrategy<pt::RedirectPageType>);
    static_assert(pt::concepts::CacheEnginePermutationStrategy<pt::BPlusPageType>);
    SUCCEED();
}

TEST(F6_Axis01PageType, PageKindsDistinct) {
    static_assert(pt::DenseBytePageType::page_kind() == PK::DenseByte);
    static_assert(pt::ExtendedDensePageType::page_kind() == PK::ExtendedDense);
    static_assert(pt::SparsePatriciaPageType::page_kind() == PK::SparsePatricia);
    static_assert(pt::RedirectPageType::page_kind() == PK::Redirect);
    static_assert(pt::CustomCachePageType::page_kind() == PK::CustomCache);
    static_assert(pt::BPlusPageType::page_kind() == PK::BPlus);
    SUCCEED();
}

TEST(F6_Axis01PageType, RedirectIsNeitherBranchNorLeaf) {
    static_assert(pt::DenseBytePageType::is_branch() == true);
    static_assert(pt::BPlusPageType::is_branch() == true);
    static_assert(pt::RedirectPageType::is_branch() == false);
    static_assert(pt::RedirectPageType::is_leaf() == false);
    SUCCEED();
}

TEST(F6_Axis01PageType, FlagSuffixUppercase) {
    static_assert(pt::DenseBytePageType::flag_suffix() == std::string_view{"DENSE_BYTE"});
    static_assert(pt::ExtendedDensePageType::flag_suffix() == std::string_view{"EXTENDED_DENSE"});
    static_assert(pt::SparsePatriciaPageType::flag_suffix() == std::string_view{"SPARSE_PATRICIA"});
    static_assert(pt::RedirectPageType::flag_suffix() == std::string_view{"REDIRECT"});
    static_assert(pt::CustomCachePageType::flag_suffix() == std::string_view{"CUSTOM_CACHE"});
    static_assert(pt::BPlusPageType::flag_suffix() == std::string_view{"BPLUS"});
    SUCCEED();
}

TEST(F6_Axis01PageType, FamilyIdsDistinct) {
    static_assert(pt::DenseBytePageType::family_id::value == 1);
    static_assert(pt::ExtendedDensePageType::family_id::value == 2);
    static_assert(pt::SparsePatriciaPageType::family_id::value == 3);
    static_assert(pt::RedirectPageType::family_id::value == 4);
    static_assert(pt::CustomCachePageType::family_id::value == 5);
    static_assert(pt::BPlusPageType::family_id::value == 6);
    SUCCEED();
}

TEST(F6_Axis01PageType, SubaxesAssigned) {
    static_assert(std::is_same_v<pt::DenseBytePageType::axis_tag, pt::subaxes::structure_role_tag>);
    static_assert(std::is_same_v<pt::ExtendedDensePageType::axis_tag, pt::subaxes::density_class_tag>);
    static_assert(std::is_same_v<pt::RedirectPageType::axis_tag, pt::subaxes::path_collapse_tag>);
    SUCCEED();
}

TEST(F6_Axis01PageType, RegistryHas6) {
    static_assert(mp::mp_size<pt::AllPageTypes>::value == 6);
    static_assert(mp::mp_size<pt::EnabledPageTypes>::value > 0);
    SUCCEED();
}

TEST(F6_Axis01PageType, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(pt::flags::dense_byte_enabled), const bool>);
    static_assert(std::is_same_v<decltype(pt::flags::redirect_enabled), const bool>);
    static_assert(std::is_same_v<decltype(pt::flags::bplus_enabled), const bool>);
    SUCCEED();
}
