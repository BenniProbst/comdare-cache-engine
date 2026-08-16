#pragma once
// 234-V-a (2026-07-07, User-GO Option A) -- 2-armige Shaped-Erweiterung der Tier->Organ-Naht.
//
// @topic traversal @achse 03a @schicht composable
//
// **Zweck:** `organ_for_search_algo_shaped<S, Shape>` waehlt fuer eine organ-backed Pool-Familie das
// SHAPED-Organ (z.B. `BTreeSearchOrganShaped<BtreeOrderKt8>`), wenn ein echter Shape-Traeger anliegt.
// Die bestehende EINARMIGE Naht `organ_for_search_algo<S>` bleibt UNANGETASTET (golden-neutral, Z.198):
// dieser Header ist ein reiner Sibling; Konsument ist der ABI-Adapter ueber seinen defaulted
// ShapeCarrier-Parameter (void = exakt das einarmige Verhalten, typ-identisch).
//
// **Disziplin (Z.198 / Doc 30):** Shape ist KEIN 18. Slot -- die 17-Slot-ABI-Invariante bleibt; die
// Scheinmultiplikation familienfremder Kombinationen wird ZWEIFACH verhindert: (1) auf Typ-Ebene faellt
// jede nicht spezialisierte Familie auf die einarmige Zuordnung zurueck (Shape wirkt nicht), (2) der
// Emitter emittiert Shaped-Quellen NUR hinter dem organ_for!=void-Filter (adhoc_emitter.hpp).
//
// **234-V-b (2026-07-08):** BST/Hash/SkipList ergaenzt; SwissTable + ART/HOT/START/Wormhole/SuRF/
// Eytzinger/Masstree bleiben mangels Shape-Param out of scope.
//
// **A8-S5 PHASE B (2026-08-05) -- DER DRITTE PARAMETER: DIE STRATEGIE DER KOMPOSITION.**
// Bis hierher war der Trait zweistellig, und genau deshalb kam die T6-Wahl der Komposition im
// KONSTITUTIVEN Pool-Pfad nicht an: der Adapter waehlte ueber diesen Trait ein Organ, dessen Store
// den Achsen-Default fest instanziierte (tier_to_organ_mapping: alle Aliase mit `<>`), waehrend der
// flache Pfad Composition::allocator seit 02a real durchreicht (abi_adapter LayoutAwareChunkedStore
// <node,layout,allocator>). Eine mimalloc-Komposition mass also ihren Store an mimalloc und ihr
// Pool-Organ am Achsen-Default -- die stille zweite Strategie, die Owner-KERN abend-11 abstellt.
// `CompAlloc` ist mit dem Achsen-Default DEFAULTIERT: alle bestehenden zweistelligen Aufrufe
// (Bestands-TUs, Adapter vor dieser Scheibe) bleiben gueltig UND typ-identisch. Die
// Alloc-Neutralitaet ist unten je Familie compile-hart gepinnt, nicht behauptet.
//
// **WAS BEWUSST NICHT PASSIERT:** die 7 shape-losen Familien binden CompAlloc fuer JEDEN Shape
// (ihr Store hat keinen Shape-Parameter -- Shape wirkt bei ihnen wie bisher nicht, die Strategie
// aber schon). Die 4 Shape-Familien behalten ihre Zwei-Armigkeit und binden CompAlloc in BEIDEN
// Armen -- sonst driftete der void-Arm gegen den Shape-Arm auseinander. Familienfremde (S, Shape)-
// Kombinationen fallen weiter auf den Primaerfall (kein Organ-Wechsel, keine Scheinmultiplikation);
// dort ist CompAlloc INERT, weil `organ_for_search_algo_t<S>` fuer sie ohnehin `void` liefert.

#include "organ_for_search_algo.hpp" // einarmige Naht + Familien-Fwd-Decls + Shaped-Aliase (via tier_to_organ_mapping.hpp)

namespace comdare::cache_engine::lookup::composable {

// Primaer: Shape wirkt nicht -> exakt die einarmige Zuordnung. Deckt (a) Shape=void fuer ALLE Familien
// (Level-0-Neutralitaet) und (b) familienfremde (S, Shape)-Kombinationen (kein Organ-Wechsel, keine
// Scheinmultiplikation auf Typ-Ebene). CompAlloc ist hier INERT: die Familien, die diesen Zweig
// erreichen, tragen kein organ-backed Substrat, das eine Strategie binden koennte (organ_for == void).
template <class S, class Shape, class CompAlloc = ::comdare::cache_engine::alloc::ExgenAllocator>
struct organ_for_search_algo_shaped {
    using type = organ_for_search_algo_t<S>;
};

// BTree (S17) -- 234-V-a Beweis-Familie: ein echter Shape-Traeger (BtreeOrderKtN) waehlt das Shaped-Organ.
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BTreeSearchAlgo, Shape, CompAlloc> {
    using type = BTreeSearchOrganShaped<Shape, CompAlloc>;
};

// Level-0 explizit: ohne Shape-Traeger bleibt der einarmige Default (== Kt4-Anker, test_234_f1) --
// seit PHASE B mit der Strategie der Komposition im Store statt mit dem Achsen-Default.
template <class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BTreeSearchAlgo, void, CompAlloc> {
    using type = BTreeSearchOrganBound<CompAlloc>;
};

// BST (S16) -- echter Shape-Traeger waehlt das BST-Shaped-Organ.
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo, Shape, CompAlloc> {
    using type = BstTreeOrganShaped<Shape, CompAlloc>;
};

// Level-0 explizit: void bleibt typ-identisch zur einarmigen BST-Zuordnung (am Achsen-Default).
template <class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo, void, CompAlloc> {
    using type = BstTreeOrganBound<CompAlloc>;
};

// Hash (S14) -- echter Shape-Traeger waehlt das Hash-Shaped-Organ.
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::HashSearchAlgo, Shape, CompAlloc> {
    using type = HashSearchOrganShaped<Shape, CompAlloc>;
};

// Level-0 explizit: void bleibt typ-identisch zur einarmigen Hash-Zuordnung (am Achsen-Default).
template <class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::HashSearchAlgo, void, CompAlloc> {
    using type = HashSearchOrganBound<CompAlloc>;
};

// SkipList (S13) -- echter Shape-Traeger waehlt das SkipList-Shaped-Organ.
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::SkipListSearchAlgo, Shape, CompAlloc> {
    using type = SkipListOrganShaped<Shape, CompAlloc>;
};

// Level-0 explizit: void bleibt typ-identisch zur einarmigen SkipList-Zuordnung (am Achsen-Default).
template <class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::SkipListSearchAlgo, void, CompAlloc> {
    using type = SkipListOrganBound<CompAlloc>;
};

// --- A8-S5 PHASE B: die SIEBEN shape-losen organ-backed Familien. -----------------------------------
// Ihr Store traegt keinen Shape-Parameter; Shape wirkt bei ihnen deshalb wie bisher NICHT (die
// Spezialisierung ist ueber JEDEN Shape partiell, nicht nur ueber void). Die STRATEGIE aber wirkt --
// sie ist der ganze Zweck dieser Scheibe. Ohne diese sieben Zeilen kaeme die T6-Wahl der Komposition
// ausgerechnet bei den Trie-/Hybrid-Organen nicht an, deren Knoten-Pools den groessten Teil des
// Speichers halten.
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::SwissTableSearchAlgo, Shape, CompAlloc> {
    using type = SwissTableOrganBound<CompAlloc>;
};
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::EytzingerSearchAlgo, Shape, CompAlloc> {
    using type = EytzingerOrganBound<CompAlloc>;
};
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::OriginalArtSearchAlgo, Shape, CompAlloc> {
    using type = ArtTrieOrganBound<CompAlloc>;
};
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::OriginalHotSearchAlgo, Shape, CompAlloc> {
    using type = HotPatriciaOrganBound<CompAlloc>;
};
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::OriginalStartSearchAlgo, Shape, CompAlloc> {
    using type = StartTrieOrganBound<CompAlloc>;
};
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::OriginalWormholeSearchAlgo, Shape, CompAlloc> {
    using type = WormholeOrganBound<CompAlloc>;
};
template <class Shape, class CompAlloc>
struct organ_for_search_algo_shaped<::comdare::cache_engine::lookup::OriginalSurfSearchAlgo, Shape, CompAlloc> {
    using type = SurfMapOrganBound<CompAlloc>;
};

template <class S, class Shape, class CompAlloc = ::comdare::cache_engine::alloc::ExgenAllocator>
using organ_for_search_algo_shaped_t = typename organ_for_search_algo_shaped<S, Shape, CompAlloc>::type;

// Self-proving (kein Raten): void-Neutralitaet -- der Shaped-Trait ist mit Shape=void fuer die Beweis-
// Familie UND eine fremde Familie typ-identisch zur einarmigen Naht (=> Adapter-Default byte-identisch).
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::BTreeSearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::BTreeSearchAlgo>>,
              "234-V-a: Level-0 (void) muss exakt die einarmige BTree-Zuordnung liefern");
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::Array256SearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::Array256SearchAlgo>>,
              "234-V-a: fremde Familie (flach, organ_for=void) bleibt unter dem Shaped-Trait void");
static_assert(
    std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo, void>,
                   organ_for_search_algo_t<::comdare::cache_engine::lookup::BinarySearchTreeSearchAlgo>>,
    "234-V-b: Level-0 (void) muss exakt die einarmige BST-Zuordnung liefern");
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::HashSearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::HashSearchAlgo>>,
              "234-V-b: Level-0 (void) muss exakt die einarmige Hash-Zuordnung liefern");
static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::SkipListSearchAlgo, void>,
                             organ_for_search_algo_t<::comdare::cache_engine::lookup::SkipListSearchAlgo>>,
              "234-V-b: Level-0 (void) muss exakt die einarmige SkipList-Zuordnung liefern");

// --- A8-S5 PHASE B: LEVEL-0 UEBER ALLE ELF organ-backed Familien. ------------------------------------
// DIE Aussage, auf der die golden-Neutralitaet dieses Commits ruht: der dritte Parameter am
// ACHSEN-DEFAULT bewegt KEINEN Typ. Sie steht je Familie da, weil eine Sammel-Formulierung genau die
// eine Familie durchrutschen liesse, die man beim naechsten Mal vergisst -- und weil der Adapter ab
// dieser Scheibe fuer JEDE von ihnen den dritten Parameter uebergibt.
#define COMDARE_PHASE_B_LEVEL0_PIN(ALGO)                                                                               \
    static_assert(std::is_same_v<organ_for_search_algo_shaped_t<::comdare::cache_engine::lookup::ALGO, void,           \
                                                                ::comdare::cache_engine::alloc::ExgenAllocator>,       \
                                 organ_for_search_algo_t<::comdare::cache_engine::lookup::ALGO>>,                      \
                  "PHASE B Level-0 verletzt (" #ALGO "): der dritte Trait-Parameter bewegt am Achsen-Default "         \
                  "einen Typ -- damit laege ein anderes Organ auf dem golden-Pfad.")
COMDARE_PHASE_B_LEVEL0_PIN(BTreeSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(BinarySearchTreeSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(HashSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(SkipListSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(SwissTableSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(EytzingerSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(OriginalArtSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(OriginalHotSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(OriginalStartSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(OriginalWormholeSearchAlgo);
COMDARE_PHASE_B_LEVEL0_PIN(OriginalSurfSearchAlgo);
#undef COMDARE_PHASE_B_LEVEL0_PIN

// DIE GEGENPROBE (Level 1) -- damit die elf Zeilen oben nicht bloss "beide gleich, weil der Parameter
// gar nicht ankommt" behaupten -- steht BEWUSST NICHT HIER, sondern in der Familien-Wache
// tests/unit/test_s5_01c_fassaden_conformance.cpp: sie braucht eine ECHTE fremde Strategie, und der
// einzige Allokator-Header, der ueber die Pool-Stores hierher reist, ist der Achsen-DEFAULT
// (axis_06_allocator_exgen.hpp). Eine zweite Strategie hier zu inkludieren hiesse, einem sehr breit
// gezogenen Header eine neue Kante samt moeglichem vendored-Link zu geben -- genau die Klasse des
// 05.08.-Hotfix cda964e0 ("Include-Satz waechst -> Link-Satz pruefen"). Die Familien-Wache fuehrt
// axis_06 und die Referenz-Komposition ohnehin; dort steht der Level-1-Beweis inklusive der Aussage,
// dass der KNOTEN-POOL die fremde Strategie real traegt.

} // namespace comdare::cache_engine::lookup::composable
