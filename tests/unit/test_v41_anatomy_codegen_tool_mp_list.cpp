// V41.F.6.1.R5.J — mp_list-driven Tool-Tabelle Tests
//
// Beweist:
// 1. KnownReferenceCompositions enthaelt 11 Entry-Wrappers
// 2. Jeder Entry-Wrapper hat composition-Type + short_name
// 3. mp_for_each-Iteration ueber KnownReferenceCompositions liefert genau 11
// 4. known_compositions() Tool-API liefert die selben 11 (drift-frei)
// 5. Pro Entry: descriptor_from_composition<C>() match die Tool-Tabelle
// 6. Compile-Time Assert: kKnownReferenceCompositionsCount == 11
//
// Pre-Read: dieser Test ist die explizite Verifikation der R5.J-Bridge —
// existing Tool-Tests (R5.F 14 + R5.G 8 + R5.H 0 + R5.I 4) bleiben weiter
// gueltig, dieser Test beweist die mp_list-Verbindung.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §52
// @task #714 V41.F.6.1.R5.J

#include <gtest/gtest.h>

#include <builder/anatomy_codegen_tool/anatomy_codegen_tool.hpp>
#include <compositions/known_compositions_list.hpp>

#include <boost/mp11.hpp>

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace tool = ::comdare::cache_engine::builder::codegen_tool;
namespace comp = ::comdare::cache_engine::compositions;
namespace mp   = ::boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — Compile-Time-Konsistenz der mp_list
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5J_MpList, KnownReferenceCompositionsHasElevenEntries) {
    static_assert(mp::mp_size<comp::KnownReferenceCompositions>::value == 11);
    static_assert(comp::kKnownReferenceCompositionsCount == 11);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Entry-Wrapper-Properties (composition + short_name)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5J_MpList, EntryWrappersHaveCompositionAndShortName) {
    static_assert(comp::ArtEntry::short_name == std::string_view{"art"});
    static_assert(std::is_same_v<comp::ArtEntry::composition, comp::ArtComposition>);
    static_assert(comp::HotEntry::short_name == std::string_view{"hot"});
    static_assert(comp::ArtPaperBindingEntry::short_name == std::string_view{"art_pb"});
    static_assert(std::is_same_v<comp::ArtPaperBindingEntry::composition, comp::ArtPaperBindingComposition>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — mp_for_each iteriert alle 11
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5J_MpList, ForEachIteratesAllElevenEntries) {
    std::set<std::string> short_names;
    mp::mp_for_each<comp::KnownReferenceCompositions>(
        [&]<class Entry>(Entry) { short_names.emplace(Entry::short_name); });
    EXPECT_EQ(short_names.size(), 11u);
    EXPECT_TRUE(short_names.contains("art"));
    EXPECT_TRUE(short_names.contains("hot"));
    EXPECT_TRUE(short_names.contains("wormhole"));
    EXPECT_TRUE(short_names.contains("surf"));
    EXPECT_TRUE(short_names.contains("masstree"));
    EXPECT_TRUE(short_names.contains("start"));
    EXPECT_TRUE(short_names.contains("art_pb"));
    EXPECT_TRUE(short_names.contains("hot_pb"));
    EXPECT_TRUE(short_names.contains("start_pb"));
    EXPECT_TRUE(short_names.contains("wormhole_pb"));
    EXPECT_TRUE(short_names.contains("surf_pb"));
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Tool-API known_compositions() liefert die selben 11 (drift-frei)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5J_MpList, ToolApiYieldsSameElevenAsMpList) {
    auto const tool_all = tool::known_compositions();
    EXPECT_EQ(tool_all.size(), comp::kKnownReferenceCompositionsCount);

    std::set<std::string> tool_short_names;
    for (auto const& d : tool_all) { tool_short_names.emplace(d.short_name); }

    std::set<std::string> mp_short_names;
    mp::mp_for_each<comp::KnownReferenceCompositions>(
        [&]<class Entry>(Entry) { mp_short_names.emplace(Entry::short_name); });

    EXPECT_EQ(tool_short_names, mp_short_names);
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — Pro Entry: descriptor_from_composition<C> match Tool-Tabelle
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5J_MpList, EachEntryDescriptorMatchesToolTable) {
    mp::mp_for_each<comp::KnownReferenceCompositions>([]<class Entry>(Entry) {
        using C                      = typename Entry::composition;
        constexpr auto expected_desc = tool::descriptor_from_composition<C>();
        auto const*    tool_entry    = tool::find_composition(Entry::short_name);
        ASSERT_NE(tool_entry, nullptr) << "short_name=" << Entry::short_name;
        EXPECT_EQ(tool_entry->cpp_type_name, expected_desc.cpp_type_name) << "short_name=" << Entry::short_name;
        EXPECT_EQ(tool_entry->header_include, expected_desc.header_include) << "short_name=" << Entry::short_name;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Keine Duplikate (cpp_type_name) in mp_list
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5J_MpList, NoDuplicateCompositionTypesInMpList) {
    std::set<std::string_view> seen;
    mp::mp_for_each<comp::KnownReferenceCompositions>([&]<class Entry>(Entry) {
        using C = typename Entry::composition;
        seen.emplace(C::cpp_type_name);
    });
    EXPECT_EQ(seen.size(), comp::kKnownReferenceCompositionsCount)
        << "Duplicate composition cpp_type_name in KnownReferenceCompositions mp_list";
}
