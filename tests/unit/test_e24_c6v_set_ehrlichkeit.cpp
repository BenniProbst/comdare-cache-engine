// test_e24_c6v_set_ehrlichkeit -- E-24 C6-V (ABI-neutraler Vor-Baustein des b-Teils, E5).
//
// GEGENSTAND: die SET-EHRLICHKEITS-SCHLIESSUNG insert_attempt_count / erase_attempt_count
// (Katalog Sektion 2 Punkt 4 + Zeile G-1, docs/sessions/20260804-DOSSIER-achsen-qualitaets-parameter-
// katalog.md). insert_count/erase_count zaehlen am Ist NUR die ERFOLGREICHEN Ops -- ohne die Versuchs-Zahl
// sind Duplikat-Quote und Erase-Miss-Quote strukturell unbestimmbar und wuerden in C6 so eingefroren.
//
//   (A) CT-PINS          -- der in-process-POD waechst um GENAU zwei uint64-Felder, additiv am Ende;
//                           die bestehenden sieben Felder behalten Typ und Reihenfolge.
//   (B) ABI-NEUTRALITAET -- der WIRE-POD SetObserverSnapshotV1 (set_tier.hpp) bleibt UNVERAENDERT;
//                           seine Erweiterung ist der ABI-Schritt C6, nicht dieser Baustein.
//   (C) LAUFZEIT-TREIBEN -- Duplikat-Insert und Erase-Miss ueber die ECHTE Set-Gattungs-API; beide
//                           Quoten werden aus den Zaehlern real ableitbar (ABL).
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/set_anatomy.hpp"

#include "anatomy/set_default_organ.hpp" // SortedArrayKeySet: das echte search_algo-Kern-Organ
#include "anatomy/set_tier.hpp"          // SetObserverSnapshotV1 (die WIRE-Form -- Neutralitaets-Wache)

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <type_traits>

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

/// Set-Gattung mit dem echten Kern-Organ auf Slot 0; die uebrigen zwoelf Slots sind stumm.
using SetComp  = cea::SetComposition<cea::SortedArrayKeySet, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                     PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using SetOrgan = cea::SetAnatomy<SetComp>;

// =============================================================================================
// (A) CT-PINS -- der in-process-POD waechst um GENAU zwei Felder, additiv am Ende.
// =============================================================================================
static_assert(sizeof(cea::SetObserverSnapshot) == 9 * sizeof(std::uint64_t),
              "C6-V: SetObserverSnapshot traegt die 7 Bestands-Felder + insert_attempt_count + "
              "erase_attempt_count -- alle uint64.");
static_assert(std::is_standard_layout_v<cea::SetObserverSnapshot> &&
              std::is_trivially_copyable_v<cea::SetObserverSnapshot>);
static_assert(std::is_same_v<decltype(cea::SetObserverSnapshot::insert_attempt_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::SetObserverSnapshot::erase_attempt_count), std::uint64_t>);
static_assert(std::is_same_v<decltype(cea::SetObserverSnapshot::insert_count), std::uint64_t> &&
                  std::is_same_v<decltype(cea::SetObserverSnapshot::peak_size), std::uint64_t>,
              "die Bestands-Felder behalten ihren Typ (additiv, nicht umgebaut)");

// =============================================================================================
// (B) ABI-NEUTRALITAET -- der WIRE-POD bleibt unveraendert; C6 promotet, nicht C6-V.
// =============================================================================================
static_assert(sizeof(cea::SetObserverSnapshotV1) == 9 * sizeof(std::uint64_t),
              "ABI-Wache: der Wire-POD traegt weiterhin GENAU seine 9 V1-Felder (7 Zaehler + "
              "observable_axis_count + organ_count) -- C6-V fasst ihn NICHT an.");
static_assert(cea::kSetObserverSnapshotVersion == 1, "ABI-Wache: keine Wire-Versions-Bewegung in C6-V.");
static_assert(!std::is_same_v<cea::SetObserverSnapshot, cea::SetObserverSnapshotV1>,
              "in-process-Form und Wire-Form bleiben getrennte Typen (die Spiegelung macht set_abi_adapter).");

} // namespace

int main() {
    std::cout << "E-24 C6-V: Set-Ehrlichkeits-Schliessung insert_attempt_count / erase_attempt_count:\n";

    SetOrgan set{};
    // 3 Insert-VERSUCHE, davon 1 Duplikat -> 2 neue Keys.
    check_true("insert(1) ist neu", set.insert(1));
    check_true("insert(2) ist neu", set.insert(2));
    check_true("insert(1) ist ein DUPLIKAT (kein neuer Key)", !set.insert(1));
    // 3 Erase-VERSUCHE, davon 1 Miss -> 2 Entfernungen.
    check_true("erase(1) entfernt real", set.erase(1));
    check_true("erase(2) entfernt real", set.erase(2));
    check_true("erase(99) ist ein MISS", !set.erase(99));

    cea::SetObserverSnapshot const s = set.observe_all();
    std::cout << "\nZaehler nach 3 Insert-Versuchen (1 Duplikat) und 3 Erase-Versuchen (1 Miss):\n";
    check_eq("insert_attempt_count (jeder Aufruf zaehlt)", s.insert_attempt_count, std::uint64_t{3});
    check_eq("insert_count (nur die erfolgreichen)", s.insert_count, std::uint64_t{2});
    check_eq("erase_attempt_count", s.erase_attempt_count, std::uint64_t{3});
    check_eq("erase_count (nur die erfolgreichen)", s.erase_count, std::uint64_t{2});
    check_eq("Duplikat-Zahl == attempts - erfolgreiche (ABL, vorher UNBESTIMMBAR)",
             s.insert_attempt_count - s.insert_count, std::uint64_t{1});
    check_eq("Erase-Miss-Zahl == attempts - erfolgreiche (ABL, vorher UNBESTIMMBAR)",
             s.erase_attempt_count - s.erase_count, std::uint64_t{1});
    check_eq("current_size (alles wieder entfernt)", s.current_size, std::uint64_t{0});
    check_eq("peak_size unveraendert erhoben", s.peak_size, std::uint64_t{2});

    // Die bestehenden Zaehler bleiben unberuehrt (kein Umdeuten vorhandener Felder).
    check_true("contains() zaehlt weiter getrennt", !set.contains(1));
    check_eq("contains_count", set.observe_all().contains_count, std::uint64_t{1});
    check_eq("contains_miss_count", set.observe_all().contains_miss_count, std::uint64_t{1});
    check_eq("die Versuchs-Zaehler bleiben davon unberuehrt", set.observe_all().insert_attempt_count, std::uint64_t{3});

    std::cout << "\n==== E-24 C6-V Set-Ehrlichkeit: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
