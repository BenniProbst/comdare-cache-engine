// V41.F.6.1.R7.1.b axis_05 Memory-Layout Tests (Goldstandard-konformer Aufbau)
//
// Beweist:
//   §1 MemoryLayoutStrategy Concept-Conformance fuer 4 Wrappers
//   §2 Pflicht-Properties (cache_line_size, name) sind constexpr
//   §3 Goldstandard CacheEnginePermutationStrategy Concept
//   §4 Subaxes-Tags (alignment_strategy / data_organization / packing_density)
//   §5 flag_suffix Werte (UPPERCASE)
//   §6 Registry AllLayouts + EnabledLayouts (mp_filter)
//   §7 is_enabled<T> Predicate-Match
//   §8 Flags-Header constexpr-Typen
//   §9 TopicConfigSet StaticAxisVariants
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md
// @memory [[axis-gold-standard-checklist]]
// @task #716 V41.F.6.1.R7.1.b

#include <gtest/gtest.h>

#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aos_strict.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_soa.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_packed_bitmap.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_registry.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_subaxes_hm1_to_hm4.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_flags.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>

#include <boost/mp11.hpp>

#include <string_view>
#include <type_traits>

namespace ax05 = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace ml   = ::comdare::cache_engine::memory_layout;
namespace mp   = ::boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Concept-Conformance (MemoryLayoutStrategy)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, AllWrappersConformToMemoryLayoutStrategy) {
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::CacheLineAlignedLayout>);
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::AoSStrictLayout>);
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::SoALayout>);
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::PackedBitmapLayout>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Pflicht-Properties pro Wrapper (cache_line_size, name)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, CacheLineAlignedHasStandard64ByteAlignment) {
    static_assert(ax05::CacheLineAlignedLayout::cache_line_size() == 64);
    static_assert(ax05::CacheLineAlignedLayout::name() == std::string_view{"memory_layout_cache_line_aligned"});
    SUCCEED();
}

TEST(R7_1_b_Axis05, AoSStrictHasPackedAlignment) {
    static_assert(ax05::AoSStrictLayout::cache_line_size() == 1);
    static_assert(ax05::AoSStrictLayout::name() == std::string_view{"memory_layout_aos_strict"});
    SUCCEED();
}

TEST(R7_1_b_Axis05, SoAHas64ByteAlignment) {
    static_assert(ax05::SoALayout::cache_line_size() == 64);
    static_assert(ax05::SoALayout::name() == std::string_view{"memory_layout_soa"});
    SUCCEED();
}

TEST(R7_1_b_Axis05, PackedBitmapHas8ByteWordAlignment) {
    static_assert(ax05::PackedBitmapLayout::cache_line_size() == 8);
    static_assert(ax05::PackedBitmapLayout::name() == std::string_view{"memory_layout_packed_bitmap"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Goldstandard: CacheEnginePermutationStrategy Concept
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, AllWrappersSatisfyCacheEnginePermutationConcept) {
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::CacheLineAlignedLayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::AoSStrictLayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::SoALayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::PackedBitmapLayout>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Subaxes-Tags (HM1-HM4)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, SubaxesTagsAreOrthogonal) {
    static_assert(std::is_same_v<ax05::CacheLineAlignedLayout::axis_tag, ax05::subaxes::alignment_strategy_tag>);
    static_assert(std::is_same_v<ax05::AoSStrictLayout::axis_tag,        ax05::subaxes::data_organization_tag>);
    static_assert(std::is_same_v<ax05::SoALayout::axis_tag,              ax05::subaxes::data_organization_tag>);
    static_assert(std::is_same_v<ax05::PackedBitmapLayout::axis_tag,     ax05::subaxes::packing_density_tag>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — flag_suffix (UPPERCASE Naming-Konvention)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, FlagSuffixUppercase) {
    static_assert(ax05::CacheLineAlignedLayout::flag_suffix() == std::string_view{"CACHE_LINE_ALIGNED"});
    static_assert(ax05::AoSStrictLayout::flag_suffix()        == std::string_view{"AOS_STRICT"});
    static_assert(ax05::SoALayout::flag_suffix()              == std::string_view{"SOA"});
    static_assert(ax05::PackedBitmapLayout::flag_suffix()     == std::string_view{"PACKED_BITMAP"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Registry: AllLayouts + EnabledLayouts (mp_filter)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, AllLayoutsContainsFourWrappers) {
    static_assert(mp::mp_size<ax05::AllLayouts>::value == 4);
    SUCCEED();
}

TEST(R7_1_b_Axis05, EnabledLayoutsIsNonEmpty) {
    // Default: alle 4 ENABLE-Optionen ON -> alle in EnabledLayouts
    static_assert(mp::mp_size<ax05::EnabledLayouts>::value > 0);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §7 — is_enabled<T> Predicate-Match
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, IsEnabledPredicateMatchesWrapperFlag) {
    static_assert(ax05::is_enabled<ax05::CacheLineAlignedLayout>::value == ax05::CacheLineAlignedLayout::enabled);
    static_assert(ax05::is_enabled<ax05::AoSStrictLayout>::value        == ax05::AoSStrictLayout::enabled);
    static_assert(ax05::is_enabled<ax05::SoALayout>::value              == ax05::SoALayout::enabled);
    static_assert(ax05::is_enabled<ax05::PackedBitmapLayout>::value     == ax05::PackedBitmapLayout::enabled);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §8 — Flags-Header constexpr-Typen
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, FlagsHeaderProvidesAllFourLayouts) {
    static_assert(std::is_same_v<decltype(ax05::flags::cache_line_aligned_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax05::flags::aos_strict_enabled),         const bool>);
    static_assert(std::is_same_v<decltype(ax05::flags::soa_enabled),                const bool>);
    static_assert(std::is_same_v<decltype(ax05::flags::packed_bitmap_enabled),      const bool>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §9 — TopicConfigSet (StaticAxisVariants_05)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_MemoryLayout, TopicConfigSetExposesAxis05) {
    static_assert(mp::mp_size<ml::TopicConfigSet::StaticAxisVariants_05>::value > 0);
    // Default = axis_05 (einzige Achse im Topic)
    static_assert(std::is_same_v<ml::TopicConfigSet::StaticAxisVariants,
                                  ml::TopicConfigSet::StaticAxisVariants_05>);
    SUCCEED();
}

TEST(R7_1_b_MemoryLayout, AllFourLayoutsInstantiable) {
    [[maybe_unused]] ax05::CacheLineAlignedLayout cla;
    [[maybe_unused]] ax05::AoSStrictLayout        aos;
    [[maybe_unused]] ax05::SoALayout              soa;
    [[maybe_unused]] ax05::PackedBitmapLayout     pbm;
    SUCCEED();
}
