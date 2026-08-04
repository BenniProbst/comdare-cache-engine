// test_e24_c6_set_algebra_abi -- E-24 C6 / F2 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: ISetAlgebraTier (anatomy/set_tier_algebra.hpp) -- die native Set-ABI der Mengen-Algebra
// (extract/merge/set-ops), append-only, mit std::set als Semantik-Anker (dem C6-Konformitaets-Orakel
// der Set-Gattung, Bauplan 1.1).
//
//   (A) APPEND-ONLY   -- ISetTier unveraendert; die Algebra ist ein EIGENES Sub-Interface.
//   (B) ORAKEL        -- jede Op wird gegen die GLEICHE Op auf einer echten std::set<uint64_t>
//                        gefahren; verglichen wird Ergebnis UND Endzustand der Menge.
//   (C) EINE WAHRHEIT -- die Algebra fuehrt KEINE zweite Buchhaltung: ihre Wirkung erscheint in den
//                        BESTEHENDEN Observer-Feldern (insert/erase) und in den C6-Versuchs-Zaehlern.
//   (D) ROBUSTHEIT    -- nullptr/count==0 liefern 0 statt UB.
//   (E) DEGRADIEREN   -- eine Fremd-Gattung liefert beim dynamic_cast nullptr.
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/set_abi_adapter.hpp"

#include "anatomy/sequence_abi_adapter.hpp"
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/set_default_organ.hpp"
#include "anatomy/set_tier.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <set>
#include <type_traits>
#include <vector>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

struct PlainAxis {};

using SetComp    = cea::SetComposition<cea::SortedArrayKeySet, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                       PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using SetAdapter = cea::SetAbiAdapter<cea::SetAnatomy<SetComp>>;

using SeqComp    = cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                            PlainAxis, cea::DoublingGrowth>;
using SeqAdapter = cea::SequenceAbiAdapter<cea::SequenceAnatomy<SeqComp>>;

// (A) APPEND-ONLY: die Algebra haengt NICHT an ISetTier (sonst waere sie ein vtable-Anhang).
static_assert(!std::is_base_of_v<cea::ISetTier, cea::ISetAlgebraTier>);
static_assert(!std::is_base_of_v<cea::ISetAlgebraTier, cea::ISetTier>);
static_assert(!std::is_base_of_v<cea::ISetTierV2, cea::ISetAlgebraTier>);
static_assert(std::is_base_of_v<cea::ISetAlgebraTier, SetAdapter>);
// Der V1-POD bleibt unangetastet -- die Algebra hat keine eigenen Felder bekommen.
static_assert(sizeof(cea::SetObserverSnapshotV1) == 9 * 8);

} // namespace

int main() {
    std::cout << "=== E-24 C6 / F2 -- ISetAlgebraTier: native Mengen-Algebra gegen das std::set-Orakel ===\n";

    SetAdapter              adapter{};
    std::set<std::uint64_t> orakel;

    // Ausgangsmenge A = {1, 2, 3, 4}
    for (std::uint64_t k : {1U, 2U, 3U, 4U}) {
        (void)adapter.tier_set_insert(k);
        orakel.insert(k);
    }

    std::cout << "\n[E] Die Algebra-Flaeche wird 1x kalt geholt; eine Fremd-Gattung degradiert\n";
    cea::IAnatomyBase* base = &adapter;
    auto*              alg  = dynamic_cast<cea::ISetAlgebraTier*>(base);
    check_true("dynamic_cast<ISetAlgebraTier*> auf der Set-Gattung trifft", alg != nullptr);
    SeqAdapter         fremd{};
    cea::IAnatomyBase* fremd_base = &fremd;
    check_true("dieselbe Abfrage an der Sequence-Gattung liefert nullptr",
               dynamic_cast<cea::ISetAlgebraTier*>(fremd_base) == nullptr);
    if (alg == nullptr) {
        std::cout << "\n=== FEHLER: ohne ISetAlgebraTier sind die Folge-Pruefungen gegenstandslos ===\n";
        return 1;
    }

    std::cout << "\n[B1] extract gegen std::set::extract\n";
    {
        bool const ist_treffer    = alg->tier_set_extract(3);
        bool const orakel_treffer = !orakel.extract(3).empty();
        bool const ist_miss       = alg->tier_set_extract(99);
        bool const orakel_miss    = !orakel.extract(99).empty();
        check_eq("extract(3) trifft (Ist)", ist_treffer, orakel_treffer);
        check_eq("extract(99) trifft nicht (Ist)", ist_miss, orakel_miss);
        check_eq("Kardinalitaet nach extract", adapter.tier_set_size(), std::uint64_t{orakel.size()});
    }

    std::cout << "\n[B2] merge gegen std::set::merge (nur REAL neue Schluessel zaehlen)\n";
    {
        // B = {3, 4, 5, 6}: 3 war entfernt -> neu; 4 ist vorhanden -> KEIN Zuwachs; 5/6 neu.
        std::vector<std::uint64_t> const b       = {3, 4, 5, 6};
        std::uint64_t const              neu_ist = alg->tier_set_merge(b.data(), b.size());

        std::set<std::uint64_t> quelle(b.begin(), b.end());
        std::size_t const       vorher = orakel.size();
        orakel.merge(quelle);
        std::uint64_t const neu_orakel = static_cast<std::uint64_t>(orakel.size() - vorher);

        check_eq("merge meldet die REAL neu entstandenen Schluessel", neu_ist, neu_orakel);
        check_eq("Kardinalitaet nach merge", adapter.tier_set_size(), std::uint64_t{orakel.size()});
        check_true("das bereits vorhandene Element hat NICHT gezaehlt", neu_ist == 3);
    }

    std::cout << "\n[B3] set-ops gegen die std-Mengenlehre\n";
    {
        // Ist-Menge A ist jetzt {1,2,3,4,5,6}; Probe-Puffer B = {2, 4, 42, 43}.
        std::vector<std::uint64_t> const b         = {2, 4, 42, 43};
        std::uint64_t const              schnitt   = alg->tier_set_intersection_count(b.data(), b.size());
        std::uint64_t const              differenz = alg->tier_set_difference_count(b.data(), b.size());

        std::uint64_t schnitt_orakel = 0;
        for (auto k : b) {
            if (orakel.count(k) != 0) ++schnitt_orakel;
        }
        check_eq("|A geschnitten B|", schnitt, schnitt_orakel);
        check_eq("|B ohne A|", differenz, static_cast<std::uint64_t>(b.size()) - schnitt_orakel);
        check_eq("Schnitt + Differenz == |B| (die Zerlegung ist vollstaendig)", schnitt + differenz,
                 static_cast<std::uint64_t>(b.size()));
        check_eq("die Menge selbst ist unveraendert (nicht-mutierende Ops)", adapter.tier_set_size(),
                 std::uint64_t{orakel.size()});
    }

    std::cout << "\n[C] EINE Wahrheit: die Algebra erscheint im BESTEHENDEN Observer\n";
    {
        cea::SetObserverSnapshotV1 v1{};
        adapter.tier_observe_set(&v1);
        auto* v2 = dynamic_cast<cea::ISetTierV2*>(base);
        check_true("die Wire-Flaeche ist ebenfalls da", v2 != nullptr);
        cea::SetObserverAggregateWire agg{};
        if (v2 != nullptr) v2->tier_observe_set_axes(&agg);

        // 4 Einzel-Inserts + 3 reale merge-Inserts == 7 erfolgreiche Inserts.
        check_eq("v1.insert_count enthaelt die merge-Inserts", v1.insert_count, std::uint64_t{7});
        // 4 Einzel-Versuche + 4 merge-Versuche == 8 Versuche (einer davon ein Duplikat).
        check_eq("genus_stats[0] enthaelt die merge-VERSUCHE", agg.genus_stats[0], std::uint64_t{8});
        check_eq("Duplikat-Versuche == Versuche - Erfolge", agg.genus_stats[0] - v1.insert_count, std::uint64_t{1});
        // 2 extract-Versuche (einer Treffer, einer Miss).
        check_eq("v1.erase_count enthaelt den extract-Treffer", v1.erase_count, std::uint64_t{1});
        check_eq("genus_stats[1] enthaelt die extract-VERSUCHE", agg.genus_stats[1], std::uint64_t{2});
        check_true("die contains-Zaehler tragen die nicht-mutierenden set-ops", v1.contains_count >= 8);
    }

    std::cout << "\n[D] Robustheit: nullptr/leer liefern 0 statt UB\n";
    {
        check_eq("merge(nullptr, 0)", alg->tier_set_merge(nullptr, 0), std::uint64_t{0});
        check_eq("merge(nullptr, 5) (Aufrufer-Fehler, kein UB)", alg->tier_set_merge(nullptr, 5), std::uint64_t{0});
        check_eq("intersection(nullptr, 3)", alg->tier_set_intersection_count(nullptr, 3), std::uint64_t{0});
        check_eq("difference(nullptr, 3)", alg->tier_set_difference_count(nullptr, 3), std::uint64_t{0});
        std::uint64_t const leer[1] = {0};
        check_eq("intersection(buf, 0)", alg->tier_set_intersection_count(leer, 0), std::uint64_t{0});
        check_eq("die Menge blieb dabei unveraendert", adapter.tier_set_size(), std::uint64_t{orakel.size()});
    }

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
