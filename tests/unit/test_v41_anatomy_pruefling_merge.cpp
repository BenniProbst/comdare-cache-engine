// V41.F.6.1.R5.C — Pruefling-Merge + 3 Kompositionale Joins Tests
//
// Beweist:
// 1. EmptyPrueflingSlot ist Default-Fallback
// 2. HasPruefling_v Detection funktioniert
// 3. Stufe 1 (CE-only): KEINE Pruefling-Beteiligung
// 4. Stufe 2 (ERSETZT-mit-Fallback): mit Slot belegt → PrueflingVariants;
//    ohne Slot → DefaultList
// 5. Stufe 3 (full-join): Union dedupliziert via mp_unique
// 6. MergeAxis Dispatch zwischen 3 Stufen
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §18-§19
// @task #699 V41.F.6.1.R5.C

#include <gtest/gtest.h>

#include <anatomy/pruefling_merge.hpp>

#include <boost/mp11.hpp>

#include <string_view> // A13-M3/C5: Q2-Wachen-Proben (name()/algo_version der Stempel-Identitaet)
#include <type_traits>

namespace prf = ::comdare::cache_engine::anatomy::pruefling;
namespace mp  = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// Sample-Wrappers fuer Tests (Dummy-Typen, keine echten Achsen-Wrappers)
// ─────────────────────────────────────────────────────────────────────────────

struct CeArray256 {
    static constexpr int id = 1;
};
struct CeVectorU8 {
    static constexpr int id = 2;
};
struct CeVectorU16 {
    static constexpr int id = 3;
};
struct PrtArtRadix512 {
    static constexpr int id = 100;
};
struct PrtArtCompact {
    static constexpr int id = 101;
};
struct OtherPrueflingX {
    static constexpr int id = 200;
};

using CeDefaults = mp::mp_list<CeArray256, CeVectorU8, CeVectorU16>;

// Pruefling-Slots (Pflicht-API laut PrueflingSlotConcept)
struct PrtArtSlot {
    using PrueflingVariants             = mp::mp_list<PrtArtRadix512, PrtArtCompact>;
    static constexpr bool has_pruefling = true;
};

struct EmptySlot : prf::EmptyPrueflingSlot {}; // erbt has_pruefling=false

struct OtherPrueflingSlot {
    using PrueflingVariants             = mp::mp_list<OtherPrueflingX>;
    static constexpr bool has_pruefling = true;
};

// Slots fuer DedupeTest (MSVC: static members in lokalen Klassen verboten)
struct DupSlotA {
    using PrueflingVariants             = mp::mp_list<CeArray256, PrtArtRadix512>;
    static constexpr bool has_pruefling = true;
};
struct DupSlotB {
    using PrueflingVariants             = mp::mp_list<PrtArtRadix512, OtherPrueflingX>;
    static constexpr bool has_pruefling = true;
};

// ─────────────────────────────────────────────────────────────────────────────
// §1 — PrueflingSlotConcept Conformance + HasPruefling_v Detection
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_PrueflingSlot, EmptySlotIsValidSlotButHasNoPruefling) {
    static_assert(prf::PrueflingSlotConcept<prf::EmptyPrueflingSlot>);
    static_assert(!prf::HasPruefling_v<prf::EmptyPrueflingSlot>);
    static_assert(prf::PrueflingSlotConcept<EmptySlot>);
    static_assert(!prf::HasPruefling_v<EmptySlot>);
    SUCCEED();
}

TEST(R5C_PrueflingSlot, PrtArtSlotIsValidWithPrueflingFlag) {
    static_assert(prf::PrueflingSlotConcept<PrtArtSlot>);
    static_assert(prf::HasPruefling_v<PrtArtSlot>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Stufe 1: CE-only (KEINE Pruefling-Beteiligung)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_Verbund1, CeOnlyIdenticalToDefaultList) {
    using S1 = prf::Verbund1Axis<CeDefaults>;
    static_assert(std::is_same_v<S1, CeDefaults>);
    static_assert(mp::mp_size<S1>::value == 3);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Stufe 2: ERSETZT-mit-Fallback
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_Verbund2, WithPrueflingSlotErsetztDefaultsKomplett) {
    using S2 = prf::Verbund2Axis<CeDefaults, PrtArtSlot>;
    // PrtArtSlot hat 2 Varianten: PrtArtRadix512, PrtArtCompact
    // Defaults werden komplett ueberschrieben (NICHT vereinigt!)
    static_assert(mp::mp_size<S2>::value == 2);
    static_assert(std::is_same_v<mp::mp_at_c<S2, 0>, PrtArtRadix512>);
    static_assert(std::is_same_v<mp::mp_at_c<S2, 1>, PrtArtCompact>);
    SUCCEED();
}

TEST(R5C_Verbund2, WithoutPrueflingSlotFallbackZurDefaults) {
    using S2 = prf::Verbund2Axis<CeDefaults, EmptySlot>;
    // EmptySlot hat keine Pruefling-Beteiligung → Defaults bleiben
    static_assert(std::is_same_v<S2, CeDefaults>);
    static_assert(mp::mp_size<S2>::value == 3);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Stufe 3: Full-Join non-redundant (mp_unique<mp_append>)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_Verbund3, UnionVereinigtAlle) {
    using S3 = prf::Verbund3Axis<CeDefaults, PrtArtSlot, OtherPrueflingSlot>;
    // CeDefaults (3) + PrtArtSlot::PrueflingVariants (2) + OtherPrueflingSlot::PrueflingVariants (1) = 6
    static_assert(mp::mp_size<S3>::value == 6);
    SUCCEED();
}

TEST(R5C_Verbund3, UnionIstNonRedundant) {
    // Duplicate-Wrapper in 2 Slots (DupSlotA + DupSlotB definiert oben)
    using S3 = prf::Verbund3Axis<CeDefaults, DupSlotA, DupSlotB>;
    // CeDefaults (3 unique) + DupSlotA (CeArray256 dup, PrtArtRadix512 neu)
    // + DupSlotB (PrtArtRadix512 dup, OtherPrueflingX neu)
    // → mp_unique dedupliziert: CeArray256, CeVectorU8, CeVectorU16, PrtArtRadix512, OtherPrueflingX = 5
    static_assert(mp::mp_size<S3>::value == 5);
    SUCCEED();
}

TEST(R5C_Verbund3, UnionMitNurEmptySlots) {
    using S3 = prf::Verbund3Axis<CeDefaults, EmptySlot, EmptySlot>;
    // EmptySlots tragen leere PrueflingVariants bei → nur Defaults bleiben
    static_assert(mp::mp_size<S3>::value == 3);
    static_assert(std::is_same_v<S3, CeDefaults>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — MergeAxis Dispatch zwischen 3 Stufen
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_MergeDispatch, Stufe1CeOnly) {
    using S = prf::MergeAxis<prf::PrueflingVerbundStrategy::Verbund1_CeOnly, CeDefaults>;
    static_assert(std::is_same_v<S, CeDefaults>);
    SUCCEED();
}

TEST(R5C_MergeDispatch, Stufe2PrueflingReplace) {
    using S = prf::MergeAxis<prf::PrueflingVerbundStrategy::Verbund2_Replace, CeDefaults, PrtArtSlot>;
    static_assert(mp::mp_size<S>::value == 2); // PrtArtSlot ersetzt komplett
    SUCCEED();
}

TEST(R5C_MergeDispatch, Verbund3Union) {
    using S = prf::MergeAxis<prf::PrueflingVerbundStrategy::Verbund3_Union, CeDefaults, PrtArtSlot, OtherPrueflingSlot>;
    static_assert(mp::mp_size<S>::value == 6);
    SUCCEED();
}

// -----------------------------------------------------------------------------
// Abschnitt 5a -- A13-M3/C5: die Q2-CT-Wache an der Merge-Namens-Naht BEISST
// -----------------------------------------------------------------------------
//
// Seit Owner-E2 (02.08.2026) gibt es keine merge-ZEILE mehr; seit Owner-Q2 lebt die Merge-Art im Stempel nur
// noch ueber erweiterte hierarchische Namen und das 'e'-Flag. Damit ist der NAME die einzige Trennung zweier
// byte-verschiedener Merge-Binaries -- und das muss eine Wache sein. Diese Proben belegen, dass sie
// TRENNSCHARF ist (nicht bloss vorhanden): sie schlaegt bei Namensgleichheit an und schweigt bei
// Unterscheidbarkeit. Die static_asserts an MergeImpl selbst sind damit keine leere Zusage.

namespace {
struct StampCeVariant {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "gleicher_name"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};
// Ein ANDERER Typ mit demselben Namen UND derselben Version -> byte-gleiches Organ-Segment.
struct StampPrueflingKollision {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "gleicher_name"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};
// Owner-Q2-konform: erweiterter hierarchischer Name (seit A13-M1 parser-gedeckt).
struct StampPrueflingHierarchisch {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "prt-art.gleicher_name"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};
// Owner-Q2-konform: gleicher Name, aber ANDERE Version -- die Segmente sind damit byte-verschieden.
// FLAG-GRAMMATIK v2: hier stand bis zum Owner-KERN vom 07.08.2026 "v1.0.0ce" und der Kommentar sagte
// "als PRUEFLING-Stand mit 'e' markiert". Diese Markierung gibt es nicht mehr -- 'e' bedeutet EFFICIENCY
// CORE. Die Fixture braucht fuer ihre Aussage aber gar keine Pruefling-Semantik, sondern nur eine
// UNTERSCHEIDBARE Version; sie traegt deshalb jetzt ein anderes Komposit-Flag. Ein bloss umgeschriebenes
// 'e'-Literal haette den Test gruen gelassen und dabei ein totes Konzept bezeugt.
struct StampPrueflingAndereVersion {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "gleicher_name"; }
    static constexpr std::string_view               algo_version = "1.0.0.c{e}";
};
} // namespace

TEST(A13M3C5MergeNamensNaht, Q2WacheTrenntByteVerschiedeneMergeBinaries) {
    using CeList = mp::mp_list<StampCeVariant>;
    // (1) NEGATIV: gleicher Name + gleiche Version -> die Wache schlaegt an. Genau diese Instanziierung
    //     braeche als MergeAxis<Verbund2_Replace, CeList, Slot> den Bau mit benanntem Text.
    static_assert(!prf::merge_lists_render_distinct_segments<CeList, mp::mp_list<StampPrueflingKollision>>(),
                  "Q2-Wache: eine namens- UND versionsgleiche Pruefling-Variante MUSS als Kollision gelten.");
    static_assert(prf::merge_segments_collide<StampCeVariant, StampPrueflingKollision>());
    // (2) POSITIV, Weg A (erweiterter hierarchischer Name, Owner-Q2-Muster "prt-art.memory.abc").
    static_assert(prf::merge_lists_render_distinct_segments<CeList, mp::mp_list<StampPrueflingHierarchisch>>());
    // (3) POSITIV, Weg B (das 'e'-Experimentalflag -- ce-eigene Versionen tragen es nie, also trennt es sicher).
    static_assert(prf::merge_lists_render_distinct_segments<CeList, mp::mp_list<StampPrueflingAndereVersion>>());
    // (4) ABGRENZUNG: derselbe TYP ist keine Kollision (er ist dieselbe Variante, nicht zwei).
    static_assert(!prf::merge_segments_collide<StampCeVariant, StampCeVariant>());
    // (5) ABGRENZUNG: nicht-stempelbare Dummy-Typen rendern kein Segment und koennen keines duplizieren --
    //     deshalb bleiben die Mechanik-Tests oben baubar, ohne dass die Wache je aussetzt, wo sie zaehlt.
    static_assert(!prf::merge_segments_collide<CeArray256, CeVectorU8>());
    static_assert(prf::merge_lists_render_distinct_segments<CeDefaults, CeDefaults>());
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Integration mit PermutationEngine (Stufe 2 Beispiel)
// ─────────────────────────────────────────────────────────────────────────────

#include <src/permutations/permutation_engine.hpp>

namespace pe = ::comdare::cache_engine::permutations;

// MSVC: TopicConfig muss ausserhalb TEST stehen damit StaticAxisVariants als using OK ist
struct TopicConfigStufe2 {
    using StaticAxisVariants = prf::Verbund2Axis<CeDefaults, PrtArtSlot>; // 2
};
struct TopicConfigStufe3 {
    using StaticAxisVariants = prf::Verbund3Axis<CeDefaults, PrtArtSlot, OtherPrueflingSlot>; // 6
};

TEST(R5C_Integration, Stufe2AxisFedToPermutationEngine) {
    using Engine = pe::PermutationEngine<TopicConfigStufe2>;
    static_assert(Engine::count() == 2);
    SUCCEED();
}

TEST(R5C_Integration, Verbund3UnionFedToPermutationEngine) {
    using Engine = pe::PermutationEngine<TopicConfigStufe3>;
    static_assert(Engine::count() == 6);
    SUCCEED();
}
