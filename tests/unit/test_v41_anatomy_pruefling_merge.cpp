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
#include <type_traits>

namespace prf = ::comdare::cache_engine::anatomy::pruefling;
namespace mp  = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// Sample-Wrappers fuer Tests (Dummy-Typen, keine echten Achsen-Wrappers)
// ─────────────────────────────────────────────────────────────────────────────

struct CeArray256       { static constexpr int id = 1; };
struct CeVectorU8       { static constexpr int id = 2; };
struct CeVectorU16      { static constexpr int id = 3; };
struct PrtArtRadix512   { static constexpr int id = 100; };
struct PrtArtCompact    { static constexpr int id = 101; };
struct OtherPrueflingX  { static constexpr int id = 200; };

using CeDefaults = mp::mp_list<CeArray256, CeVectorU8, CeVectorU16>;

// Pruefling-Slots (Pflicht-API laut PrueflingSlotConcept)
struct PrtArtSlot {
    using PrueflingVariants = mp::mp_list<PrtArtRadix512, PrtArtCompact>;
    static constexpr bool has_pruefling = true;
};

struct EmptySlot : prf::EmptyPrueflingSlot {};  // erbt has_pruefling=false

struct OtherPrueflingSlot {
    using PrueflingVariants = mp::mp_list<OtherPrueflingX>;
    static constexpr bool has_pruefling = true;
};

// Slots fuer DedupeTest (MSVC: static members in lokalen Klassen verboten)
struct DupSlotA {
    using PrueflingVariants = mp::mp_list<CeArray256, PrtArtRadix512>;
    static constexpr bool has_pruefling = true;
};
struct DupSlotB {
    using PrueflingVariants = mp::mp_list<PrtArtRadix512, OtherPrueflingX>;
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

TEST(R5C_StufeOne, CeOnlyIdenticalToDefaultList) {
    using S1 = prf::StufeOneAxis<CeDefaults>;
    static_assert(std::is_same_v<S1, CeDefaults>);
    static_assert(mp::mp_size<S1>::value == 3);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Stufe 2: ERSETZT-mit-Fallback
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_StufeTwo, WithPrueflingSlotErsetztDefaultsKomplett) {
    using S2 = prf::StufeTwoAxis<CeDefaults, PrtArtSlot>;
    // PrtArtSlot hat 2 Varianten: PrtArtRadix512, PrtArtCompact
    // Defaults werden komplett ueberschrieben (NICHT vereinigt!)
    static_assert(mp::mp_size<S2>::value == 2);
    static_assert(std::is_same_v<mp::mp_at_c<S2, 0>, PrtArtRadix512>);
    static_assert(std::is_same_v<mp::mp_at_c<S2, 1>, PrtArtCompact>);
    SUCCEED();
}

TEST(R5C_StufeTwo, WithoutPrueflingSlotFallbackZurDefaults) {
    using S2 = prf::StufeTwoAxis<CeDefaults, EmptySlot>;
    // EmptySlot hat keine Pruefling-Beteiligung → Defaults bleiben
    static_assert(std::is_same_v<S2, CeDefaults>);
    static_assert(mp::mp_size<S2>::value == 3);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Stufe 3: Full-Join non-redundant (mp_unique<mp_append>)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_StufeThree, FullJoinVereinigtAlle) {
    using S3 = prf::StufeThreeAxis<CeDefaults, PrtArtSlot, OtherPrueflingSlot>;
    // CeDefaults (3) + PrtArtSlot::PrueflingVariants (2) + OtherPrueflingSlot::PrueflingVariants (1) = 6
    static_assert(mp::mp_size<S3>::value == 6);
    SUCCEED();
}

TEST(R5C_StufeThree, FullJoinIstNonRedundant) {
    // Duplicate-Wrapper in 2 Slots (DupSlotA + DupSlotB definiert oben)
    using S3 = prf::StufeThreeAxis<CeDefaults, DupSlotA, DupSlotB>;
    // CeDefaults (3 unique) + DupSlotA (CeArray256 dup, PrtArtRadix512 neu)
    // + DupSlotB (PrtArtRadix512 dup, OtherPrueflingX neu)
    // → mp_unique dedupliziert: CeArray256, CeVectorU8, CeVectorU16, PrtArtRadix512, OtherPrueflingX = 5
    static_assert(mp::mp_size<S3>::value == 5);
    SUCCEED();
}

TEST(R5C_StufeThree, FullJoinMitNurEmptySlots) {
    using S3 = prf::StufeThreeAxis<CeDefaults, EmptySlot, EmptySlot>;
    // EmptySlots tragen leere PrueflingVariants bei → nur Defaults bleiben
    static_assert(mp::mp_size<S3>::value == 3);
    static_assert(std::is_same_v<S3, CeDefaults>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §5 — MergeAxis Dispatch zwischen 3 Stufen
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5C_MergeDispatch, Stufe1CeOnly) {
    using S = prf::MergeAxis<prf::MergeStrategy::Stufe1_CeOnly, CeDefaults>;
    static_assert(std::is_same_v<S, CeDefaults>);
    SUCCEED();
}

TEST(R5C_MergeDispatch, Stufe2PrueflingReplace) {
    using S = prf::MergeAxis<prf::MergeStrategy::Stufe2_PrueflingReplace, CeDefaults, PrtArtSlot>;
    static_assert(mp::mp_size<S>::value == 2);  // PrtArtSlot ersetzt komplett
    SUCCEED();
}

TEST(R5C_MergeDispatch, Stufe3FullJoin) {
    using S = prf::MergeAxis<prf::MergeStrategy::Stufe3_FullJoin, CeDefaults, PrtArtSlot, OtherPrueflingSlot>;
    static_assert(mp::mp_size<S>::value == 6);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §6 — Integration mit PermutationEngine (Stufe 2 Beispiel)
// ─────────────────────────────────────────────────────────────────────────────

#include <src/permutations/permutation_engine.hpp>

namespace pe = ::comdare::cache_engine::permutations;

// MSVC: TopicConfig muss ausserhalb TEST stehen damit StaticAxisVariants als using OK ist
struct TopicConfigStufe2 {
    using StaticAxisVariants = prf::StufeTwoAxis<CeDefaults, PrtArtSlot>;  // 2
};
struct TopicConfigStufe3 {
    using StaticAxisVariants = prf::StufeThreeAxis<CeDefaults, PrtArtSlot, OtherPrueflingSlot>;  // 6
};

TEST(R5C_Integration, Stufe2AxisFedToPermutationEngine) {
    using Engine = pe::PermutationEngine<TopicConfigStufe2>;
    static_assert(Engine::count() == 2);
    SUCCEED();
}

TEST(R5C_Integration, Stufe3FullJoinFedToPermutationEngine) {
    using Engine = pe::PermutationEngine<TopicConfigStufe3>;
    static_assert(Engine::count() == 6);
    SUCCEED();
}
