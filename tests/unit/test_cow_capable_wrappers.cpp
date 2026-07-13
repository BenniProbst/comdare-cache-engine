// #142 Welle 2 / Audit-K3 — VERIFIKATION der Ziel-Population (NICHT nur Referenz-Kompositionen).
//
// Audit-Befund K3: die 320 Basis-Tiere (Profil-Basis-Selektion) tragen im search_algo-Slot die ROHEN Registry-Wrapper
// (KArySearchAlgo etc.). Diese hatten KEIN restore_statistics → abi_adapter::organ_cow_capable_v war fuer
// sie false → stiller if-constexpr-Rueckfall auf das eager Copy-Memento; der 42/42-Test pruefte nur die
// Referenz-Kompositionen Art/Hot/Masstree (deren Slot die Observable-Huelle ist) — Test-Pop != Ziel-Pop.
//
// Audit-Meta-Lehre #1 (Capability-Detection darf nie still degradieren) + #2 (Test-Pop == Ziel-Pop):
// dieser Test erzwingt GENAU die requires-Klausel aus abi_adapter::organ_cow_capable_v ueber ALLE 21
// produktiven search_algo-Wrapper compile-time (17 Legacy + 4 per-K KArySearchAlgoK2/4/8/16, #188 Inc2). Ohne
// den Fix (restore_statistics je Wrapper) schluege
// jeder static_assert fehl — das ist der Pfad-Differenz-Nachweis (Meta-Lehre #3) auf Compile-Ebene.
//
// Build (standalone, COMDARE_CE_ENABLE_STATISTICS aktiviert die Mess-Schnittstelle der Wrapper):
//   cl /std:c++latest /EHsc /Od /DCOMDARE_CE_ENABLE_STATISTICS=1 <harness-include-set> diese.cpp
//   -> Skript tests/unit/build_test_cow_capable_wrappers.ps1
// SUPERSEDED 2026-07-11: obiges .ps1-Skript entfernt (Behelfsweg-Bereinigung); Test jetzt registriertes
//   ctest-Target (tests/unit/CMakeLists.txt, M-CE-24-Block COMDARE_MCE24_PLAIN_TESTS).

#include <axes/lookup/axis_03a_search_algo_array65535.hpp>
#include <axes/lookup/axis_03a_search_algo_array256.hpp>
#include <axes/lookup/axis_03a_search_algo_bst.hpp>
#include <axes/lookup/axis_03a_search_algo_btree.hpp>
#include <axes/lookup/axis_03a_search_algo_eytzinger.hpp>
#include <axes/lookup/axis_03a_search_algo_hash_search.hpp>
#include <axes/lookup/axis_03a_search_algo_interpolation.hpp>
#include <axes/lookup/axis_03a_search_algo_k_ary.hpp>
#include <axes/lookup/axis_03a_search_algo_linear_scan.hpp>
#include <axes/lookup/axis_03a_search_algo_original_art.hpp>
#include <axes/lookup/axis_03a_search_algo_original_hot.hpp>
#include <axes/lookup/axis_03a_search_algo_original_start.hpp>
#include <axes/lookup/axis_03a_search_algo_original_surf.hpp>
#include <axes/lookup/axis_03a_search_algo_original_wormhole.hpp>
#include <axes/lookup/axis_03a_search_algo_skip_list.hpp>
#include <axes/lookup/axis_03a_search_algo_vector_u16u16.hpp>
#include <axes/lookup/axis_03a_search_algo_vector_u8u8.hpp>

#include <iostream>
#include <type_traits>

namespace lk = ::comdare::cache_engine::lookup;

// EXAKT die Detektor-Klausel aus abi_adapter::organ_cow_capable_v (#133 Rev. 2): das Organ traegt den
// O(1)-Stat-Snapshot/-Restore. Faellt sie false, kippt cow_capable_ und der Adapter messe ueber den eager
// Copy-Fallback (Audit-K3: genau das galt fuer die 320).
template <class S>
concept OrganCowCapable = requires(S& s, S const& cs) { s.restore_statistics(cs.statistics()); };

// Volle CoW-Faehigkeit am search_algo-Slot = organ_cow_capable_v PLUS Kopierbarkeit (cow_capable_ verlangt
// beides). Nur kopierbare Wrapper aktivieren den lazy CoW-Pfad real; nicht-kopierbare bleiben korrekt auf
// dem Fallback (self-gating). KArySearchAlgo = das aktuelle Profil-Basis-search_algo (Basis-Selektion).
template <class S>
concept SearchAlgoFullyCowCapable =
    OrganCowCapable<S> && std::is_copy_constructible_v<S> && std::is_copy_assignable_v<S>;

#define K3_CHECK(T)                                                                                                    \
    static_assert(OrganCowCapable<lk::T>, #T ": restore_statistics fehlt -> organ_cow_capable_v false (Audit-K3)");
K3_CHECK(Array65535SearchAlgo)
K3_CHECK(Array256SearchAlgo)
K3_CHECK(BinarySearchTreeSearchAlgo)
K3_CHECK(BTreeSearchAlgo)
K3_CHECK(EytzingerSearchAlgo)
K3_CHECK(HashSearchAlgo)
K3_CHECK(InterpolationSearchAlgo)
K3_CHECK(KArySearchAlgo)
// #188 per-K Increment 2: die 4 compile-time-K Wrapper (aus KArySearchAlgoT<K>) tragen restore_statistics ebenso.
K3_CHECK(KArySearchAlgoK2)
K3_CHECK(KArySearchAlgoK4)
K3_CHECK(KArySearchAlgoK8)
K3_CHECK(KArySearchAlgoK16)
K3_CHECK(LinearScanSearchAlgo)
K3_CHECK(OriginalArtSearchAlgo)
K3_CHECK(OriginalHotSearchAlgo)
K3_CHECK(OriginalStartSearchAlgo)
K3_CHECK(OriginalSurfSearchAlgo)
K3_CHECK(OriginalWormholeSearchAlgo)
K3_CHECK(SkipListSearchAlgo)
K3_CHECK(VectorU16U16SearchAlgo)
K3_CHECK(VectorU8U8SearchAlgo)
#undef K3_CHECK

// Ziel-Population end-to-end: das produktive Profil-Basis-search_algo aktiviert CoW vollstaendig.
static_assert(SearchAlgoFullyCowCapable<lk::KArySearchAlgo>,
              "KArySearchAlgo (Profil-Basis-search_algo) muss den CoW-Pfad real aktivieren (K3-Ziel-Population)");

int main() {
    std::cout << "Audit-K3 VERIFIKATION (compile-time bestanden): alle 21 produktiven search_algo-Wrapper\n"
                 "(17 Legacy + 4 per-K KArySearchAlgoK2/4/8/16, #188 Inc2) tragen restore_statistics ->\n"
                 "organ_cow_capable_v aktiv; KArySearchAlgo voll CoW-faehig.\n"
                 "==== #142 Welle 2 / K3: ALLE 21 OK ====\n";
    return 0;
}
