// tests/unit/test_org19_meta_meta_requirement.cpp -- E-10/ORG-19 SCHRITT-2-GATE (k5-KOEDER, T-1 rot-zuerst;
// Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19.md Schritt 2 + VERIFY-Fix FIX-7(a), 26.08.2026).
//
// GEGENSTAND: der DEKLARATIONS-ORT der required-/meaningful-Flags einer Organ-Meta-Meta, per Binary
// erreichbar -- das 18+1-Register: die 18 Kompositions-Zeilen leben in simd_organ_requirement.hpp,
// die +1 Meta-Meta-Zeile in organ_meta_meta_requirement.hpp (H-23-C.1-Schnitt; kCompositionAxisNames
// bleibt 18). Die erste Zeile (disk_io) ist EHRLICH LEER (KON16-02: required = hartes Funktions-
// MINIMUM, kein Beschleunigungs-Wunsch; kein SIMD-Flag ist Funktionsminimum fuer Festplatten-IO;
// KON59-01: Bau-Ermessen, keine Owner-Runde). Der Beweis, dass eine NICHT-leere Zeile PER BINARY
// wirkt, faehrt als consteval-Probe ueber das 2c-Probe-Register (D-4; K13: der Koeder beisst).
//
// KOPPLUNGS-PIN (Praezedenz anatomy_version_stamp.hpp "Wer ORG-18 aendert, aendert BEIDE Orte"):
// measurement/ darf organ_axes/ nicht einbinden -- die Register-Strings DOPPELN die 1a-Konstanten;
// DIESER Test haelt beide Orte deckungsgleich (Label, Carrier-Achse, Carrier-Wert).
//
// T-1/K13: VOR Schritt 2a/2b uebersetzt diese TU NICHT (organ_meta_meta_requirement.hpp fehlt) = der
// Koeder-Biss. k6-Zwilling der Abnahme: test_simd_organ_achsen_deckung bleibt UNVERAENDERT gruen
// (18-Nenner der Kompositions-Tabellen; dieser Schritt aendert dort keine Zeile).
//
// FORM-HINWEIS (CI 16069/cppcheck): CT-Proben als BENANNTE consteval-Funktionen, keine IIFE-Lambdas.

#include <gtest/gtest.h>

#include <cache_engine/measurement/organ_meta_meta_requirement.hpp> // 2a (k5: vor dem Bau FEHLT sie = rot)

#include <cache_engine/measurement/simd_feature_flag.hpp>
#include <cache_engine/measurement/simd_organ_requirement.hpp> // 2b: Kern + aggregate_meaningful_for_axes
#include <cache_engine/measurement/simd_organ_sensibility.hpp>
#include <organ_axes/organ_meta_meta/axis_disk_io_organ_meta_meta.hpp> // 1a (Kopplungs-PIN der Strings)

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace meas = ::comdare::cache_engine::measurement;
namespace omm  = ::comdare::cache_engine::organ_meta_meta;

// -- (1) 18+1-REGISTER: die Nenner BEIDER Orte -----------------------------------------------------
static_assert(meas::kSimdOrganRequirement.size() == 18,
              "die Kompositions-Tabelle bleibt 18 (kCompositionAxisNames-Nenner; Positions-Pin in "
              "test_simd_organ_achsen_deckung) -- ORG-19 ist KEINE 19. Zeile dieser Tabelle");
static_assert(meas::kOrganMetaMetaRequirement.size() == 1,
              "18 Komposition (simd_organ_requirement.hpp) + 1 Meta-Meta (organ_meta_meta_requirement.hpp) "
              "-- der 18+1-Schnitt nach H-23 C.1; heute GENAU ORG-19-IO (disk_io)");

// -- (2) KOPPLUNGS-PINS: Register-Strings == 1a-Konstanten (BEIDE Orte deckungsgleich) -------------
static_assert(meas::kOrganMetaMetaRequirement[0].meta_meta_label == omm::DiskIoOrganMetaMeta::axis_label(),
              "Register-Label doppelt do_axis_label() aus 1a -- wer einen Ort aendert, aendert BEIDE");
static_assert(meas::kOrganMetaMetaRequirement[0].carrier_axis == omm::DiskIoOrganMetaMeta::kCarrierAxis,
              "Carrier-Achse doppelt kCarrierAxis aus 1a (D-1 Single-Source der Comp-Bindung)");
static_assert(meas::kOrganMetaMetaRequirement[0].carrier_value == omm::DiskIoOrganMetaMeta::kCarrierValues[0],
              "Carrier-Wert doppelt kCarrierValues[0] aus 1a (== DiskWritebackTarget::name(), k1-Pin)");

// -- (3) EHRLICH LEER (KON16-02 Funktions-MINIMUM; KON59-01 Bau-Ermessen; Regel 'nie raten') -------
static_assert(meas::required_of_meta_meta("disk_io").empty(),
              "required(disk_io) ist LEER -- ein erfundener Wert waere ein Stempel-/Gate-Beitrag ohne "
              "Sachgrund (KN-1 Lesart A); ein echter Wert traegt spaeter seinen eigenen Minor");
static_assert(meas::meaningful_of_meta_meta("disk_io").empty(),
              "sensibility/meaningful(disk_io) ist LEER (kein spekulativer Zuordnungs-Erfund)");
static_assert(!meas::any_meta_meta_declares_required(),
              "heutiger Stand: KEINE Meta-Meta deklariert required -> Gate-Beitrag bleibt byte-neutral");
static_assert(meas::required_of_meta_meta("unbekannte_meta_meta").empty(),
              "unbekanntes Label bleibt leer (kein Wurf, Muster required_of)");

// -- (4) PAAR->LABEL-AUFLOESUNG (per Binary erreichbar: (achse,wert) -> Meta-Meta-Label|leer) ------
static_assert(meas::meta_meta_for_pair("persistence_target", "persistence_disk_writeback") ==
                  std::string_view{"disk_io"},
              "der IO-Traeger (persistence_target=persistence_disk_writeback) loest auf ORG-19-IO auf");
static_assert(meas::meta_meta_for_pair("persistence_target", "persistence_memory_only").empty(),
              "MemoryOnly ist KEIN Meta-Meta-Traeger (Flotte all_axes_golden: leer, N-1)");
static_assert(meas::meta_meta_for_pair("search_algo", "k_ary").empty(), "Nicht-Traeger-Achsen loesen auf leer auf");

// -- (5) PROBE-AGGREGATION (2c, detail): eine NICHT-leere Zeile wirkt PER BINARY (D-4) -------------
consteval bool probe_required_mit_traeger_ist_genau_avx2() {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 2> disk_paare{
        {{"search_algo", "k_ary"}, {"persistence_target", "persistence_disk_writeback"}}};
    auto const menge = meas::aggregate_required_for_axes_kern(disk_paare, meas::detail::kProbeMetaMetaRequirement);
    return menge.count == 1 && menge.flags[0].cpuinfo == meas::kAvx2.cpuinfo;
}
consteval bool probe_required_ohne_traeger_ist_leer() {
    constexpr std::array<std::pair<std::string_view, std::string_view>, 2> memory_paare{
        {{"search_algo", "k_ary"}, {"persistence_target", "persistence_memory_only"}}};
    return meas::aggregate_required_for_axes_kern(memory_paare, meas::detail::kProbeMetaMetaRequirement).count == 0;
}
static_assert(probe_required_mit_traeger_ist_genau_avx2(),
              "D-4: Achsen-Paare MIT IO-Traeger aggregieren die Probe-Menge {avx2} -- die required-"
              "Vereinigung ist PER BINARY, nicht global");
static_assert(probe_required_ohne_traeger_ist_leer(),
              "D-4: Achsen-Paare OHNE Traeger aggregieren LEER -- keine Binary erbt fremde required-Flags");
static_assert(meas::detail::probe_per_binary_aggregation_ist_exakt(),
              "D-4-Gesamtprobe (Grundlage des C-3a-UMSCHLAGS in Schritt 3): mit Traeger -> Probe-Menge, "
              "ohne Traeger -> leer, meaningful analog");

// -- (6) LAUFZEIT: bestehende Signatur (Laufzeit-Huelle, FIX-7(a)) + ehrliches Produktions-Leer ----
TEST(Org19MetaMetaRequirement, ProduktionsAggregatIstEhrlichLeer) {
    std::vector<std::pair<std::string, std::string>> const axes{{"search_algo", "k_ary"},
                                                                {"persistence_target", "persistence_disk_writeback"}};
    EXPECT_TRUE(meas::aggregate_required_for_axes(axes).empty())
        << "required(disk_io) ist LEER deklariert -- das Produktions-Aggregat bleibt byte-neutral";
    auto const meaningful = meas::aggregate_meaningful_for_axes(axes);
    EXPECT_EQ(meaningful.size(), 4u) << "Sinnhaftigkeits-Obergrenze PER BINARY: search_algo traegt 4 "
                                        "Katalog-Flags (bw/dq/f/vl), persistence_target und die "
                                        "Meta-Meta-Zeile tragen 0";
    for (auto const& f : meaningful)
        EXPECT_NE(f.cpuinfo, "avx2") << "avx2 kaeme nur aus dem 2c-Probe-Register (produktionsfrei)";
}

TEST(Org19MetaMetaRequirement, ProbeRegisterBleibtProduktionsfrei) {
    // Das 2c-Register ist NUR consteval-Beweis (kein Produktions-Wert): die Produktions-Lookups
    // kennen sein Label nicht, und der Traeger-Paar-Lookup liefert weiterhin die disk_io-Zeile.
    EXPECT_TRUE(meas::required_of_meta_meta("probe_meta_meta").empty());
    EXPECT_TRUE(meas::meaningful_of_meta_meta("probe_meta_meta").empty());
    EXPECT_EQ(meas::meta_meta_for_pair("persistence_target", "persistence_disk_writeback"), "disk_io");
}

} // namespace
