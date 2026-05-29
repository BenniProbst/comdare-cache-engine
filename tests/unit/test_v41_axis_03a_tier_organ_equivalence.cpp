// V41 #40 (Doku 24 §6.2, Doku 14 §3.3) — Tier→Organ-REKONSTRUKTIONS-Beleg.
// Belegt, dass jeder bereits sezierte monolithische Tier-Wrapper exakt aus seiner äquivalenten
// Organ-Komposition wiederherstellbar ist (Tier ≡ Organ-Komposition ≡ std::map). Das ist die Brücke
// für die Umstufung (Doku 24 §6.2): erst wenn die Äquivalenz belegt ist, darf der monolithische Wrapper
// aus der Achse entfernt + als Gattungs-Konfigurator (Composition über Organe) rekonstruiert werden.
// key-type-sicher: Vergleich INNERHALB der schmalen Tier-Breite (uint8 -> 200/255, uint16 -> 1000/1000),
// sonst kollidieren Keys beim Cast (organ=uint64 vs tier=uint8/uint16).

#include <gtest/gtest.h>

#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_registry.hpp>          // Tier-Wrapper-Typen
#include <topics/traversal/axis_03a_search_algo/composable/tier_to_organ_mapping.hpp>        // Organ-Pendants
#include "support/std_map_equivalence_harness.hpp"

namespace ce_03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ce_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
namespace ts     = ::comdare::cache_engine::test_support;

// --- Jeder bereits sezierte Tier-Wrapper == std::map (vertikal, in seiner eigenen Key-Breite) -----------
TEST(Axis03aTierOrgan, EachDissectedTierMatchesStdMap) {
    ts::verify_matches_std_map<ce_03a::Array256SearchAlgo>(200u, 255u);          // uint8
    ts::verify_matches_std_map<ce_03a::VectorU8U8SearchAlgo>(200u, 255u);        // uint8
    ts::verify_matches_std_map<ce_03a::VectorU16U16SearchAlgo>(1000u, 1000u);    // uint16
    ts::verify_matches_std_map<ce_03a::Array65535SearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::KArySearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::EytzingerSearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::LinearScanSearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::InterpolationSearchAlgo>(1000u, 1000u);
    ts::verify_matches_std_map<ce_03a::BinarySearchTreeSearchAlgo>(1000u, 1000u);
    SUCCEED();
}

// --- uint8-Tiere ≡ Organ-Komposition (REKONSTRUKTION belegt, key_mod=200) --------------------------------
TEST(Axis03aTierOrgan, Uint8TiersReconstructibleFromOrgan) {
    ts::verify_variants_equivalent<ce_cmp::LinearScanOrgan, ce_03a::Array256SearchAlgo>(200u, 255u);
    ts::verify_variants_equivalent<ce_cmp::LinearScanOrgan, ce_03a::VectorU8U8SearchAlgo>(200u, 255u);
    SUCCEED();  // dense/sparse uint8-Tier exakt aus LinearScan-Organ-Komposition wiederherstellbar
}

// --- uint16-Tiere ≡ Organ-Komposition (REKONSTRUKTION belegt, key_mod=1000) ------------------------------
TEST(Axis03aTierOrgan, Uint16TiersReconstructibleFromOrgan) {
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::VectorU16U16SearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::Array65535SearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::KArySearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::SortedBinaryOrgan, ce_03a::EytzingerSearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::LinearScanOrgan,   ce_03a::LinearScanSearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::InterpolationOrgan, ce_03a::InterpolationSearchAlgo>(1000u, 1000u);
    ts::verify_variants_equivalent<ce_cmp::BstTreeOrgan,      ce_03a::BinarySearchTreeSearchAlgo>(1000u, 1000u);
    SUCCEED();  // jeder uint16-Tier exakt aus seiner Organ-Komposition wiederherstellbar
}
