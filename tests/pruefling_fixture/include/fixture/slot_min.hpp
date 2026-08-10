// =============================================================================
//  MINIMAL-PRUEFLING (Fixture) -- der CE-eigene Nenner fuer den Plugin-Weg.
//  W0a/S6, 2026-08-10.
// =============================================================================
//
// WOZU: der Plugin-Weg COMDARE_CE_PRUEFLINGE existierte seit dem 29.05.2026 und
// wurde von KEINEM Baum je gefahren -- gemessen: 0 Treffer fuer
// COMDARE_CE_PRUEFLINGE in jeder .yml/.yaml von ce, super und comdare-prt-art.
// Der Fall "Gatter = TRUE" war damit 73 Tage lang erschlossen, nie beobachtet.
// Dieses Fixture macht ihn beobachtbar, ohne einen fremden Klon zu brauchen:
// es liegt in ce, ist immer da, braucht kein Netz und keinen Token.
//
// WAS ES BEWEIST:  Loader -> Zusicherung -> Gatter -> Registrierung -> ctest -N.
// WAS ES NICHT BEWEIST:  die vier echten prt-art-Slots, die Composition-Demo,
//   den DLL-Codegen. Das Fixture ist der Nachweis des MECHANISMUS, nicht des
//   Pruefling-Inhalts. Wer den Inhalt will, laedt den echten Pruefling (S9).
//
// WARUM TAG-TYPEN STATT ECHTER CE-STRATEGIEN: die Naht pruefen heisst
// PrueflingSlotConcept, StufeTwoAxis und StufeThreeAxis instanziieren. Dafuer
// zaehlt ausschliesslich die Listen-Algebra ueber mp11. Echte Strategie-Basen
// zoegen ihre eigenen CRTP-Ctor-Wachen (algo_version, Fehlerklassen) herein und
// wuerden das Fixture an Zusicherungen binden, die mit dem Plugin-Weg nichts zu
// tun haben -- ein Fixture, das aus fremdem Grund bricht, ist kein Messgeraet.

#ifndef COMDARE_TESTS_PRUEFLING_FIXTURE_SLOT_MIN_HPP
#define COMDARE_TESTS_PRUEFLING_FIXTURE_SLOT_MIN_HPP

#include <anatomy/pruefling_merge.hpp>

#include <boost/mp11.hpp>

#include <string_view>

namespace comdare::tests::pruefling_fixture {

namespace mp = boost::mp11;

// Der Wuerfelwert dieses Fixtures. Er beantwortet V-8: "Was waere der Zustand,
// in dem die Ausgabe erscheint und die Sache trotzdem nicht existiert?" --
// naemlich der, in dem ein gleichnamiger slot_min.hpp aus einer ANDEREN Quelle
// auf dem Include-Pfad liegt und das Gatter mit fremdem Inhalt gruen faerbt.
// Der Test sichert diesen Wert zu; weicht er ab, bricht die Uebersetzung.
inline constexpr std::string_view kNonce = "9714b781ca792698";

// Zwei Tag-Typen. CeBekannt steht bereits in der CE-Default-Liste, FixtureNeu
// nicht -- so werden die beiden Richtungen von mp_unique unterscheidbar.
struct CeBekannt {};
struct FixtureNeu {};

using DefaultVariants = mp::mp_list<CeBekannt>;

// DER SLOT. Genau die zwei Glieder, die PrueflingSlotConcept verlangt.
// Kein CRTP-Basiszwang: der Pruefling erbt nie von CE, er erfuellt nur.
struct SlotMin {
    using PrueflingVariants             = mp::mp_list<FixtureNeu>;
    static constexpr bool has_pruefling = true;
};

// Der Gegeneingang (T-4): derselbe Slot-Bau, aber has_pruefling = false.
// Er belegt, dass StufeTwoAxis dann auf die CE-Default-Liste zurueckfaellt --
// eine Zusicherung ohne Eingang, bei dem sie NICHT gilt, ist keine.
struct SlotLeer {
    using PrueflingVariants             = mp::mp_list<FixtureNeu>;
    static constexpr bool has_pruefling = false;
};

// Und der Fall, der mp_unique beweisen muss: ein Slot, dessen Variante bereits
// in der Default-Liste steht. Stufe 3 darf hier um 0 wachsen.
struct SlotDoppelt {
    using PrueflingVariants             = mp::mp_list<CeBekannt>;
    static constexpr bool has_pruefling = true;
};

} // namespace comdare::tests::pruefling_fixture

#endif // COMDARE_TESTS_PRUEFLING_FIXTURE_SLOT_MIN_HPP
