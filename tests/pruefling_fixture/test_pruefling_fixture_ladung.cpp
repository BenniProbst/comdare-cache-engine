// =============================================================================
//  DER LADUNGS-BEWEIS -- dieser Test existiert nur, wenn der Plugin-Weg traegt.
//  W0a/S6, 2026-08-10.
// =============================================================================
//
// T-7: Ein Test existiert erst, wenn er in `ctest -N` erscheint. Genau das ist
// hier die Aussage. Diese Datei wird ausschliesslich ueber die Zusicherung des
// Fixture-Prueflings registriert (COMDARE_PRUEFLING_TEST_SOURCES). Erscheint sie
// in `ctest -N`, dann hat die ganze Kette getragen:
//   Loader -> comdare_pruefling_deklarieren() -> Faehigkeits-Eintrag ->
//   comdare_pruefling_faehigkeit() -> Registrierung -> ctest.
// Fehlt ein Glied, fehlt der Test -- und die Registrierungs-/Abdeckungs-Wache
// sieht das, weil der Block sich in BEIDEN Zweigen ins Protokoll meldet.
//
// DIE static_assert SIND DER EIGENTLICHE BEWEIS. Sie laufen zur Uebersetzungs-
// zeit; der gtest-Rumpf haelt das Ergebnis nur fuer `ctest -N` sichtbar.

#include <fixture/slot_min.hpp>

#include <anatomy/pruefling_merge.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <string_view>

namespace {

namespace fx  = ::comdare::tests::pruefling_fixture;
namespace cep = ::comdare::cache_engine::anatomy::pruefling;
namespace mp  = boost::mp11;

// ---------------------------------------------------------------------------
// (1) Die Naht nimmt den Slot ueberhaupt an.
// ---------------------------------------------------------------------------
static_assert(cep::PrueflingSlotConcept<fx::SlotMin>,
              "SlotMin erfuellt PrueflingSlotConcept nicht -- die Naht nimmt ihn nicht an.");
static_assert(cep::HasPruefling_v<fx::SlotMin>, "HasPruefling_v<SlotMin> ist false, obwohl has_pruefling = true.");

// GEGENEINGANG (T-4): derselbe Bau, has_pruefling = false. Das Concept gilt
// weiter (die Form stimmt), das Praedikat nicht (der Slot ist unbelegt).
static_assert(cep::PrueflingSlotConcept<fx::SlotLeer>,
              "SlotLeer erfuellt die FORM des Concepts -- sonst misst der Gegeneingang etwas anderes.");
static_assert(!cep::HasPruefling_v<fx::SlotLeer>,
              "HasPruefling_v<SlotLeer> muss false sein -- sonst ist has_pruefling wirkungslos.");

// ---------------------------------------------------------------------------
// (2) Stufe 2 ERSETZT -- mit Fallback.
// ---------------------------------------------------------------------------
using Stufe2Belegt = cep::Verbund2Axis<fx::DefaultVariants, fx::SlotMin>;
using Stufe2Leer   = cep::Verbund2Axis<fx::DefaultVariants, fx::SlotLeer>;

static_assert(std::is_same_v<Stufe2Belegt, mp::mp_list<fx::FixtureNeu>>,
              "Stufe 2 hat die CE-Default-Liste nicht durch die Pruefling-Liste ersetzt.");
static_assert(std::is_same_v<Stufe2Leer, fx::DefaultVariants>,
              "Stufe 2 ist bei unbelegtem Slot nicht auf die CE-Default-Liste zurueckgefallen.");

// ---------------------------------------------------------------------------
// (3) Stufe 3 VEREINIGT -- und dedupliziert. Beide Richtungen, in Zahlen.
// ---------------------------------------------------------------------------
using Stufe3Fremd   = cep::Verbund3Axis<fx::DefaultVariants, fx::SlotMin>;
using Stufe3Doppelt = cep::Verbund3Axis<fx::DefaultVariants, fx::SlotDoppelt>;

static_assert(mp::mp_size<fx::DefaultVariants>::value == 1,
              "Der Nenner stimmt nicht: die Default-Liste hat nicht 1 Element.");
static_assert(mp::mp_size<Stufe3Fremd>::value == 2, "Stufe 3 waechst bei fremdem Typ nicht um genau 1.");
static_assert(mp::mp_size<Stufe3Doppelt>::value == 1,
              "Stufe 3 waechst bei bereits enthaltenem Typ nicht um 0 -- mp_unique greift nicht.");

// ---------------------------------------------------------------------------
// (4) V-8: ist der Header, gegen den hier uebersetzt wurde, WIRKLICH dieser?
// ---------------------------------------------------------------------------
static_assert(fx::kNonce == std::string_view{"9714b781ca792698"},
              "Die Fixture-Nonce weicht ab -- der eingebundene slot_min.hpp stammt aus "
              "einer anderen Quelle als dem Fixture-Pruefling dieses Baums.");

// ---------------------------------------------------------------------------
// Der Laufzeit-Rumpf. Er traegt keine eigene Aussage -- er macht die oben
// bereits bewiesenen Saetze in `ctest -N` und im Testbericht sichtbar.
// ---------------------------------------------------------------------------
TEST(PrueflingFixtureLadung, DieKetteHatGetragen) {
    EXPECT_EQ(fx::kNonce, std::string_view{"9714b781ca792698"});
    EXPECT_TRUE(cep::HasPruefling_v<fx::SlotMin>);
    EXPECT_FALSE(cep::HasPruefling_v<fx::SlotLeer>);
}

TEST(PrueflingFixtureLadung, MergeStufenInZahlen) {
    EXPECT_EQ(mp::mp_size<fx::DefaultVariants>::value, 1u);
    EXPECT_EQ(mp::mp_size<Stufe3Fremd>::value, 2u);
    EXPECT_EQ(mp::mp_size<Stufe3Doppelt>::value, 1u);
}

} // namespace
