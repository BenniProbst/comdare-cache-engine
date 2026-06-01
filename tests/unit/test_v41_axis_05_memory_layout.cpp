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
#include <axes/layout/axis_05_memory_layout_flags.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>

#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aosoa.hpp>

#include <boost/mp11.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ax05 = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace ml   = ::comdare::cache_engine::memory_layout;
namespace mp   = ::boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Concept-Conformance (MemoryLayoutStrategy)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, AllWrappersConformToMemoryLayoutStrategy) {
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::CacheLineAlignedMemoryLayout>);
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::AoSStrictMemoryLayout>);
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::SoAMemoryLayout>);
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::PackedBitmapMemoryLayout>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Pflicht-Properties pro Wrapper (cache_line_size, name)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, CacheLineAlignedHasStandard64ByteAlignment) {
    static_assert(ax05::CacheLineAlignedMemoryLayout::cache_line_size() == 64);
    static_assert(ax05::CacheLineAlignedMemoryLayout::name() == std::string_view{"memory_layout_cache_line_aligned"});
    SUCCEED();
}

TEST(R7_1_b_Axis05, AoSStrictHasPackedAlignment) {
    static_assert(ax05::AoSStrictMemoryLayout::cache_line_size() == 1);
    static_assert(ax05::AoSStrictMemoryLayout::name() == std::string_view{"memory_layout_aos_strict"});
    SUCCEED();
}

TEST(R7_1_b_Axis05, SoAHas64ByteAlignment) {
    static_assert(ax05::SoAMemoryLayout::cache_line_size() == 64);
    static_assert(ax05::SoAMemoryLayout::name() == std::string_view{"memory_layout_soa"});
    SUCCEED();
}

TEST(R7_1_b_Axis05, PackedBitmapHas8ByteWordAlignment) {
    static_assert(ax05::PackedBitmapMemoryLayout::cache_line_size() == 8);
    static_assert(ax05::PackedBitmapMemoryLayout::name() == std::string_view{"memory_layout_packed_bitmap"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Goldstandard: CacheEnginePermutationStrategy Concept
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, AllWrappersSatisfyCacheEnginePermutationConcept) {
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::CacheLineAlignedMemoryLayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::AoSStrictMemoryLayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::SoAMemoryLayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::PackedBitmapMemoryLayout>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Subaxes-Tags (HM1-HM4)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, SubaxesTagsAreOrthogonal) {
    static_assert(std::is_same_v<ax05::CacheLineAlignedMemoryLayout::axis_tag, ax05::subaxes::alignment_strategy_tag>);
    static_assert(std::is_same_v<ax05::AoSStrictMemoryLayout::axis_tag,        ax05::subaxes::data_organization_tag>);
    static_assert(std::is_same_v<ax05::SoAMemoryLayout::axis_tag,              ax05::subaxes::data_organization_tag>);
    static_assert(std::is_same_v<ax05::PackedBitmapMemoryLayout::axis_tag,     ax05::subaxes::packing_density_tag>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — flag_suffix (UPPERCASE Naming-Konvention)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, FlagSuffixUppercase) {
    static_assert(ax05::CacheLineAlignedMemoryLayout::flag_suffix() == std::string_view{"CACHE_LINE_ALIGNED"});
    static_assert(ax05::AoSStrictMemoryLayout::flag_suffix()        == std::string_view{"AOS_STRICT"});
    static_assert(ax05::SoAMemoryLayout::flag_suffix()              == std::string_view{"SOA"});
    static_assert(ax05::PackedBitmapMemoryLayout::flag_suffix()     == std::string_view{"PACKED_BITMAP"});
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Registry: AllLayouts + EnabledLayouts (mp_filter)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R7_1_b_Axis05, AllLayoutsContainsFiveWrappers) {
    static_assert(mp::mp_size<ax05::AllLayouts>::value == 5);  // + AoSoA (A4, 2026-05-29)
    SUCCEED();
}

// V41.F.6.1 A4 — AoSoAMemoryLayout (Array-of-Structures-of-Arrays, Block-SoA+AoS Hybrid, SIMD-tiled)
TEST(R7_1_b_Axis05, AoSoAHybridLayoutProperties) {
    static_assert(ax05::concepts::MemoryLayoutStrategy<ax05::AoSoAMemoryLayout>);
    static_assert(ax05::concepts::CacheEnginePermutationStrategy<ax05::AoSoAMemoryLayout>);
    static_assert(ax05::AoSoAMemoryLayout::name()        == std::string_view{"memory_layout_aosoa"});
    static_assert(ax05::AoSoAMemoryLayout::flag_suffix() == std::string_view{"AOSOA"});
    static_assert(ax05::AoSoAMemoryLayout::cache_line_size() == 64);
    static_assert(ax05::AoSoAMemoryLayout::block_width()     == 8);   // AVX2-u64-Lane-Zahl
    static_assert(ax05::AoSoAMemoryLayout::family_id::value  == 5);
    static_assert(std::is_same_v<ax05::AoSoAMemoryLayout::axis_tag, ax05::subaxes::data_organization_tag>);
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
    static_assert(ax05::is_enabled<ax05::CacheLineAlignedMemoryLayout>::value == ax05::CacheLineAlignedMemoryLayout::enabled);
    static_assert(ax05::is_enabled<ax05::AoSStrictMemoryLayout>::value        == ax05::AoSStrictMemoryLayout::enabled);
    static_assert(ax05::is_enabled<ax05::SoAMemoryLayout>::value              == ax05::SoAMemoryLayout::enabled);
    static_assert(ax05::is_enabled<ax05::PackedBitmapMemoryLayout>::value     == ax05::PackedBitmapMemoryLayout::enabled);
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
    [[maybe_unused]] ax05::CacheLineAlignedMemoryLayout cla;
    [[maybe_unused]] ax05::AoSStrictMemoryLayout        aos;
    [[maybe_unused]] ax05::SoAMemoryLayout              soa;
    [[maybe_unused]] ax05::PackedBitmapMemoryLayout     pbm;
    SUCCEED();
}

// V41.F.6.1 R5.B — scan_field_sum KORREKTHEIT (beweist die layout-charakteristischen Zugriffsmuster,
// nicht Geschwindigkeit): pro Layout wird der Puffer IN dessen Muster mit Werten (i+1) befuellt und
// geprueft, dass scan_field_sum exakt die Summe sum(i+1)=n(n+1)/2 liefert. Damit ist die neue
// Laufzeit-API (die die Achse F15-operativ macht) als korrekt verifiziert.
TEST(R5B_Axis05_ScanFieldSum, EachLayoutReadsItsCharacteristicPattern) {
    constexpr std::size_t n = 64;                 // Vielfaches von 8 (AoSoA-Block)
    constexpr std::size_t record_size = 64;
    constexpr std::uint64_t expected = (std::uint64_t{n} * (n + 1)) / 2;  // 2080
    std::vector<unsigned char> buf(n * record_size, 0u);

    auto put32 = [&](std::size_t off, std::uint32_t v) { std::memcpy(buf.data() + off, &v, sizeof(v)); };
    auto reset = [&]() { std::fill(buf.begin(), buf.end(), static_cast<unsigned char>(0)); };

    // AoS (CacheLineAligned + AoSStrict): Feld i bei i*record_size
    reset();
    for (std::size_t i = 0; i < n; ++i) put32(i * record_size, static_cast<std::uint32_t>(i + 1));
    EXPECT_EQ(ax05::CacheLineAlignedMemoryLayout::scan_field_sum(buf.data(), n, record_size), expected);
    EXPECT_EQ(ax05::AoSStrictMemoryLayout::scan_field_sum(buf.data(), n, record_size), expected);

    // SoA: Feld i kontiguierlich bei i*4
    reset();
    for (std::size_t i = 0; i < n; ++i) put32(i * sizeof(std::uint32_t), static_cast<std::uint32_t>(i + 1));
    EXPECT_EQ(ax05::SoAMemoryLayout::scan_field_sum(buf.data(), n, record_size), expected);

    // AoSoA (Block-Breite 8): Feld i bei block*(8*record_size) + within*4
    reset();
    for (std::size_t i = 0; i < n; ++i) {
        std::size_t const off = (i / 8u) * (8u * record_size) + (i % 8u) * sizeof(std::uint32_t);
        put32(off, static_cast<std::uint32_t>(i + 1));
    }
    EXPECT_EQ(ax05::AoSoAMemoryLayout::scan_field_sum(buf.data(), n, record_size), expected);

    // PackedBitmap: kontiguierliche 2-Byte-Felder bei i*2
    reset();
    for (std::size_t i = 0; i < n; ++i) {
        std::uint16_t const v = static_cast<std::uint16_t>(i + 1);
        std::memcpy(buf.data() + i * sizeof(std::uint16_t), &v, sizeof(v));
    }
    EXPECT_EQ(ax05::PackedBitmapMemoryLayout::scan_field_sum(buf.data(), n, record_size), expected);
}
