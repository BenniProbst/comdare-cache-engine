// test_s8kopf_planner_kopf -- CONTRACT-TESTS des S-8-KOPFES (T-NEU-6 Teil 1, 2026-08-20).
// =============================================================================================
// GEGENSTAND: die drei neuen Traeger-Stufe-1-Header unter <traeger/planner/...>
//   naht_nachrichten.hpp  B-01 Naht-Schablone   (KON50-01/-02, KON51-01/-03, KON52-01)
//   steuerdock.hpp        B-14 sechs Steuerdocks (Owner-KERN 09.08.; KON41-03-Abgrenzung)
//   traeger_rakete.hpp    Template-Method-Geruest (KON16-03/-04: KEINE YAML; Tiefe 3)
// plus die #22(ii)-Zusicherung "--debug beschleunigt, leitet nie um" AN DER dump-plan-NAHT
// (construct_plan_into-Walk byte-invariant unter COMDARE_DEBUG_FREIGABE).
//
// TDD-VERTRAG: die Nenner sind FREMD (T-3) -- die 3 der Nachrichtenklassen und die 6 der
// Docks kommen aus dem Ledger-Wortlaut bzw. werden per std::next_permutation UNABHAENGIG
// vom Pruefling gezaehlt; die Instrument-Menge {wallclock, makro, mikro} ist im Test als
// EIGENE Abschrift des Owner-KERNs hinterlegt, nicht aus kToolingHauptKette gelesen.
// Gegeneingaenge (T-4): Mess-Rohdaten-Typ, Struktur-Klon, Zweit-OOB, Rechtsakt-Tausch,
// fremde Einrichtung am Dock, Terminal-Stufe der Rakete. Je Test eine Wegwerf-Mutation mit
// literalem Rot (T-11c) -- Protokoll in der Strang-Ergebnisdatei.
//
// WAS DER DEBUG-TEST DECKT -- UND WAS NICHT (ein gruenes Gate deckt nur seinen Gegenstand):
// Gedeckt ist die PLAN-EBENE: derselbe Director-Walk, den --dump-plan/--dump-ci/--dump-cmake
// und der Lauf nehmen (geteilte Naht construct_plan_into), liefert byte-gleichen Plan-Text,
// ob COMDARE_DEBUG_FREIGABE gesetzt ist oder nicht -- "leitet nie um" am Plan-Traeger.
// NICHT gedeckt bleibt die stdout-Byte-Gleichheit der GESAMT-CLI mit/ohne --debug-Flag
// (main.cpp:721-726, VL-3-Restposten): sie haengt am CLI-Prozess, nicht an dieser Naht,
// und bleibt als offener Posten deklariert.
//
// ASCII-only, Zeilen <= 120.

#include "planner/experiment_plan_director.hpp"    // ExperimentPlanDirector / PlanTextBuilder (Monolith-Bestand)
#include "xml_config_parser/xml_config_parser.hpp" // XmlConfigParser / ThesisProfile / ExperimentProfile

#include <traeger/planner/naht_nachrichten.hpp>
#include <traeger/planner/steuerdock.hpp>
#include <traeger/planner/traeger_rakete.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib> // setenv/unsetenv (Debug-Neutralitaets-Probe)
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#ifndef COMDARE_PLANNER_THESIS_ALL_AXES
#error "COMDARE_PLANNER_THESIS_ALL_AXES must point to thesis_profiles/all_axes_golden.profile.xml"
#endif
#ifndef COMDARE_EXPERIMENT_GOLDEN
#error "COMDARE_EXPERIMENT_GOLDEN must point to tests/unit/thesis_tiere/experiment_golden.xml"
#endif

namespace {

namespace trg     = comdare::traeger::planner;
namespace planner = comdare::cache_engine::planner;
namespace cx      = comdare::builder::xml;
namespace fs      = std::filesystem;

// =====================================================================================
// B-01 -- DIE NAHT-SCHABLONE: drei Klassen hinauf, Negativproben compile-hart.
// =====================================================================================

// Positivseite: GENAU die drei Ledger-Klassen erfuellen den Hinauf-Vertrag.
static_assert(trg::SteuerNahtHinauf<trg::FortschrittsDelta>);
static_assert(trg::SteuerNahtHinauf<trg::StatusFehlerLog>);
static_assert(trg::SteuerNahtHinauf<trg::ErgebnisTrace>);

// NEGATIVPROBE 1 (KON50-01, der Kernsatz "NIE Mess-Rohdaten zum Planer"): ein Typ, der
// Messwerte traegt und es zugibt, faellt durch -- auch mit korrekter Richtung.
struct MessRohdatenPaket {
    using naht_richtung                                       = trg::NahtHinauf;
    static constexpr bool                traegt_mess_rohdaten = true; // der Bruch
    static constexpr bool                ausser_der_reihe     = false;
    static constexpr trg::NahtPrioritaet prioritaet           = trg::NahtPrioritaet::Normal;
    double                               werte[8] = {}; // "sieht aus wie Messwerte" -- genau die unheilbare Klasse
};
static_assert(!trg::SteuerNahtHinauf<MessRohdatenPaket>, "KON50-01: Mess-Rohdaten fallen durch");

// NEGATIVPROBE 2 (Geschlossenheit, KON50-02): ein STRUKTUR-KLON mit allen richtigen Markern
// ist trotzdem KEINE Naht-Klasse -- die Schablone ist Whitelist, kein Muster.
struct StrukturKlon {
    using naht_richtung                                       = trg::NahtHinauf;
    static constexpr bool                traegt_mess_rohdaten = false;
    static constexpr bool                ausser_der_reihe     = false;
    static constexpr trg::NahtPrioritaet prioritaet           = trg::NahtPrioritaet::Normal;
    std::string                          beliebig;
};
static_assert(!trg::SteuerNahtHinauf<StrukturKlon>, "geschlossene Menge: Klon faellt durch");

// NEGATIVPROBE 3 (KON52-01 "GENAU EINE OOB-Nachricht"): ein zweiter OOB-Typ faellt durch,
// selbst wenn er ausser_der_reihe = true traegt.
struct ZweitesOobSignal {
    using naht_richtung                        = trg::NahtHinauf;
    static constexpr bool traegt_mess_rohdaten = false;
    static constexpr bool ausser_der_reihe     = true;
};
static_assert(!trg::SteuerNahtAusserDerReihe<ZweitesOobSignal>, "KON52-01: nur das FertigSignal ist OOB");
static_assert(trg::SteuerNahtAusserDerReihe<trg::FertigSignal>);

// Richtungs- und Rollen-Trennung: das Hinab-Fragment ist keine Hinauf-Klasse; das FertigSignal
// reist NICHT in der Queue (es ist OOB); das Fragment ist der einzige Hinab-Traeger.
static_assert(!trg::SteuerNahtHinauf<trg::GefiltertesXmlFragment>);
static_assert(!trg::SteuerNahtHinauf<trg::FertigSignal>);
static_assert(trg::SteuerNahtHinab<trg::GefiltertesXmlFragment>);
static_assert(!trg::SteuerNahtHinab<trg::FortschrittsDelta>);

TEST(S8KopfNahtSchablone, DreiKlassenHinaufMitNegativproben) {
    // FREMDER NENNER: "drei Nachrichtenklassen hinauf" (KON50-02 PRAEZISIERT, Ledger Z.496).
    // Die 3 steht HIER als Ledger-Abschrift und wird gegen die exportierte Typliste gehalten.
    constexpr std::size_t kLedgerNachrichtenKlassenHinauf = 3;
    static_assert(std::tuple_size_v<trg::SteuerNahtHinaufKlassen> == kLedgerNachrichtenKlassenHinauf,
                  "KON50-02: genau DREI Nachrichtenklassen hinauf");

    // KON51-03: Fehler reisen vor Fortschritt -- die Prioritaets-Zuordnung ist Teil der Schablone.
    static_assert(trg::StatusFehlerLog::prioritaet == trg::NahtPrioritaet::Hoch);
    static_assert(trg::FortschrittsDelta::prioritaet == trg::NahtPrioritaet::Normal);
    static_assert(trg::ErgebnisTrace::prioritaet == trg::NahtPrioritaet::Normal);

    // Stummschaltung ist ein benannter Zustand, Default = nicht stumm (ausserhalb der Mess-Phase).
    constexpr trg::Stummschaltung ruhe{};
    static_assert(!ruhe.aktiv);

    SUCCEED() << "alle B-01-Zusicherungen sind compile-hart (static_assert) geprueft";
}

// =====================================================================================
// B-14 -- SECHS STEUERDOCKS: Nenner fremd, Rechtsakte untauschbar, Release nur gesammelt.
// =====================================================================================

TEST(S8KopfSteuerdock, SechsDocksJeCebVersionNennerFremd) {
    // FREMDE NENNER-QUELLE 1: die Instrument-Menge als EIGENE Abschrift des Owner-KERNs 09.08.
    // ("variadisch an die 3 fakultaet vorhandenen CEB Versionen"; Kette wallclock/makro/mikro).
    // Bewusst NICHT kToolingHauptKette gelesen -- driftet der Header, klafft es hier.
    std::array<trg::ToolingInstrument, 3> kern_abschrift = {
        trg::ToolingInstrument::Wallclock,
        trg::ToolingInstrument::Makro,
        trg::ToolingInstrument::Mikro,
    };

    // FREMDE NENNER-QUELLE 2: 3! unabhaengig gezaehlt (std::next_permutation, nicht die Registry).
    std::sort(kern_abschrift.begin(), kern_abschrift.end());
    std::size_t                                        fakultaet_gezaehlt = 0;
    std::vector<std::array<trg::ToolingInstrument, 3>> fremde_anordnungen;
    do {
        ++fakultaet_gezaehlt;
        fremde_anordnungen.push_back(kern_abschrift);
    } while (std::next_permutation(kern_abschrift.begin(), kern_abschrift.end()));
    ASSERT_EQ(fakultaet_gezaehlt, 6u) << "Owner-KERN 09.08.: 3! = 6 CEB-Versionen (Ledger Z.17168)";

    // DER PRUEFLING: Registry-Aufzaehlung == fremder Nenner, eindeutig, deterministisch.
    auto const versionen = trg::SteuerdockRegistry::alle_ceb_versionen();
    EXPECT_EQ(versionen.size(), fakultaet_gezaehlt) << "SECHS Steuerdocks -- je CEB-Version eines (B-14)";
    EXPECT_EQ(trg::SteuerdockRegistry::dock_zahl(), fakultaet_gezaehlt);

    std::set<std::string> kennzeichen;
    for (auto const& v : versionen) { kennzeichen.insert(v.kennzeichen()); }
    EXPECT_EQ(kennzeichen.size(), versionen.size()) << "keine Doppel-Anordnung: 6 ECHTE Versionen";

    // Jede Registry-Anordnung ist eine Permutation der KERN-Abschrift (kein fremdes Instrument,
    // keine Teil-Belegung -- KON34-03: permutiert wird die ORDNUNG einer vollzaehligen Kette).
    for (auto const& v : versionen) {
        auto sortiert = v.anordnung;
        std::sort(sortiert.begin(), sortiert.end());
        EXPECT_EQ(sortiert, kern_abschrift)
            << "Anordnung " << v.kennzeichen() << " ist keine Permutation der Tooling-Haupt-Kette";
    }

    // Determinismus: zwei Aufzaehlungen sind elementweise gleich (Registry-Schluessel-Stabilitaet).
    auto const zweite = trg::SteuerdockRegistry::alle_ceb_versionen();
    ASSERT_EQ(zweite.size(), versionen.size());
    for (std::size_t i = 0; i < versionen.size(); ++i) {
        EXPECT_EQ(zweite[i], versionen[i]) << "Aufzaehlung nicht deterministisch an Index " << i;
    }

    // GEGENPROBE gegen die fremde Zaehlung: dieselben 6 Anordnungen, gleiche Reihenfolge
    // (beide Aufzaehlungen sind lexikografisch ab sortiertem Start).
    ASSERT_EQ(fremde_anordnungen.size(), versionen.size());
    for (std::size_t i = 0; i < versionen.size(); ++i) {
        EXPECT_EQ(versionen[i].anordnung, fremde_anordnungen[i]) << "Ordnungs-Drift an Index " << i;
    }
}

// Compile-harte Passform des variadischen Kerns: Docks bieten GENAU die einkompilierten
// Einrichtungen an; Befehle an fremde Einrichtungen sind KEIN CODE (requires-Probe).
struct EinrichtungWallclock {};
struct EinrichtungMakro {};
struct EinrichtungFremd {}; // NICHT einkompiliert

using DockZweifach = trg::Steuerdock<EinrichtungWallclock, EinrichtungMakro>;

template <class Dock, class E>
concept KannFreigabe = requires(Dock d, trg::FreigabeSystemAchse const& akt) { d.template freigabe<E>(akt); };
template <class Dock, class E>
concept KannDurchsetzung =
    requires(Dock d, trg::DurchsetzungOrganAchse const& akt) { d.template durchsetzung<E>(akt); };
// Der TAUSCH der Rechtsakte als requires-Probe: freigabe mit Durchsetzungs-Akt (und umgekehrt).
template <class Dock, class E>
concept KannFreigabeMitDurchsetzungsAkt =
    requires(Dock d, trg::DurchsetzungOrganAchse const& akt) { d.template freigabe<E>(akt); };
template <class Dock, class E>
concept KannDurchsetzungMitFreigabeAkt =
    requires(Dock d, trg::FreigabeSystemAchse const& akt) { d.template durchsetzung<E>(akt); };

static_assert(DockZweifach::bietet_an<EinrichtungWallclock>());
static_assert(DockZweifach::bietet_an<EinrichtungMakro>());
static_assert(!DockZweifach::bietet_an<EinrichtungFremd>(), "Dock passt GENAU auf Einkompiliertes");
static_assert(KannFreigabe<DockZweifach, EinrichtungWallclock>);
static_assert(KannDurchsetzung<DockZweifach, EinrichtungMakro>);
static_assert(!KannFreigabe<DockZweifach, EinrichtungFremd>, "fremde Einrichtung: Befehl ist kein Code");
static_assert(!KannDurchsetzung<DockZweifach, EinrichtungFremd>);

// "nicht austauschbar": kein Konversionsweg zwischen den Rechtsakten, kein getauschter Aufruf.
static_assert(!std::is_convertible_v<trg::FreigabeSystemAchse, trg::DurchsetzungOrganAchse>);
static_assert(!std::is_convertible_v<trg::DurchsetzungOrganAchse, trg::FreigabeSystemAchse>);
static_assert(!KannFreigabeMitDurchsetzungsAkt<DockZweifach, EinrichtungWallclock>,
              "FREIGABE nimmt keinen Durchsetzungs-Akt (Owner: Begriffe nicht austauschbar)");
static_assert(!KannDurchsetzungMitFreigabeAkt<DockZweifach, EinrichtungWallclock>,
              "DURCHSETZUNG nimmt keinen Freigabe-Akt");

// Release-Fenster: VOR und NACH existieren; ein WAEHREND ist nicht repraesentierbar.
template <class T>
concept HatWaehrendGesamtMessung = requires { T::WaehrendGesamtMessung; };
template <class T>
concept HatWaehrend = requires { T::Waehrend; };
static_assert(!HatWaehrendGesamtMessung<trg::ReleaseFenster>, "Release WAEHREND der Messung: kein Wert dafuer");
static_assert(!HatWaehrend<trg::ReleaseFenster>);

TEST(S8KopfSteuerdock, RechtsakteUndGesammelterRelease) {
    trg::CebVersion const                                   version{}; // kanonische Grund-Anordnung
    trg::Steuerdock<EinrichtungWallclock, EinrichtungMakro> dock{version};

    // Beide Rechtsakte, je ueber eine einkompilierte Einrichtung -- und der gesammelte Release.
    dock.freigabe<EinrichtungWallclock>(trg::FreigabeSystemAchse{.system_achse = "target_isa"});
    dock.durchsetzung<EinrichtungMakro>(trg::DurchsetzungOrganAchse{.organ_achse = "allocator_strategy"});

    trg::GesammelterRelease sammel;
    sammel.fenster = trg::ReleaseFenster::NachGesamtMessung;
    sammel.log_zeilen.push_back("ceb: perm 1/6 gebaut");
    sammel.log_zeilen.push_back("ceb: messung abgeschlossen");
    dock.release(sammel);

    ASSERT_EQ(dock.protokoll().size(), 4u) << "2 Rechtsakte + 2 gesammelte Release-Zeilen";
    EXPECT_EQ(dock.protokoll()[0], "freigabe:target_isa");
    EXPECT_EQ(dock.protokoll()[1], "durchsetzung:allocator_strategy");
    EXPECT_EQ(dock.protokoll()[2], "release:ceb: perm 1/6 gebaut");
    EXPECT_EQ(dock.protokoll()[3], "release:ceb: messung abgeschlossen");
    EXPECT_EQ(dock.version().kennzeichen(), "wallclock>makro>mikro") << "kanonische Grund-Anordnung";
}

// =====================================================================================
// TRAEGER-RAKETE -- Template-Method-Geruest: Tiefe 3, erst Tier dann Hybrid, KEINE YAML.
// =====================================================================================

// KEINE YAML by construction: das Formen-Enum kennt kein Yaml-Mitglied (requires-Probe).
template <class T>
concept HatYamlForm = requires { T::Yaml; };
static_assert(!HatYamlForm<trg::EmissionsForm>, "KON16-03: KEINE YAML -- die Form existiert nicht");

// Probe-Stufen fuer das Geruest: zeichnen die Hook-Reihenfolge auf.
struct PlanerProbe final : trg::TraegerRaketenGeruest<PlanerProbe, trg::TraegerRaketenStufe::Planer> {
    std::vector<std::string> spur;
    std::string              bau_auftrag(trg::EmissionsGegenstand g) {
        spur.push_back("auftrag:" + std::to_string(static_cast<int>(g)));
        return "starte-bau-" + std::to_string(static_cast<int>(g));
    }
    void verifiziere_emission(trg::EmissionsProdukt const& p) {
        spur.push_back("verifiziere:" + std::to_string(static_cast<int>(p.gegenstand)));
    }
};
struct CebProbe final : trg::TraegerRaketenGeruest<CebProbe, trg::TraegerRaketenStufe::Ceb> {
    std::vector<std::string> spur;
    std::string              bau_auftrag(trg::EmissionsGegenstand g) {
        spur.push_back("auftrag:" + std::to_string(static_cast<int>(g)));
        return "starte-bau-" + std::to_string(static_cast<int>(g));
    }
    void verifiziere_emission(trg::EmissionsProdukt const& p) {
        spur.push_back("verifiziere:" + std::to_string(static_cast<int>(p.gegenstand)));
    }
};
struct TerminalProbe final : trg::TraegerRaketenGeruest<TerminalProbe, trg::TraegerRaketenStufe::TierStufe> {
    std::string bau_auftrag(trg::EmissionsGegenstand) { return ""; }
    void        verifiziere_emission(trg::EmissionsProdukt const&) {}
};

// GEGENEINGANG (T-4): die Terminal-Stufe HAT keine Emission -- der Aufruf ist kein Code.
template <class T>
concept KannEmittieren = requires(T t, trg::IEmissionsBauModul& m) { t.emittiere_naechste(m); };
static_assert(KannEmittieren<PlanerProbe>);
static_assert(KannEmittieren<CebProbe>);
static_assert(!KannEmittieren<TerminalProbe>, "Tiefe 3: die Terminal-Stufe emittiert NICHT");

TEST(S8KopfRakete, StufenketteTiefeDreiUndEmissionsFolge) {
    // FREMDER NENNER: Owner 11.08. "die Tiefe ist nur 3". Der Test WANDERT die Kette selbst.
    constexpr std::size_t                   kOwnerTiefe = 3;
    std::size_t                             tiefe       = 0;
    std::optional<trg::TraegerRaketenStufe> s           = trg::TraegerRaketenStufe::Planer;
    std::vector<trg::TraegerRaketenStufe>   besucht;
    while (s.has_value() && tiefe <= kOwnerTiefe) { // <=: eine Endlos-Kette wuerde HIER auffallen
        besucht.push_back(*s);
        ++tiefe;
        s = trg::naechste_stufe(*s);
    }
    EXPECT_EQ(tiefe, kOwnerTiefe) << "Planer -> Ceb -> Stufe 3, dann Schluss";
    ASSERT_EQ(besucht.size(), 3u);
    EXPECT_EQ(besucht[0], trg::TraegerRaketenStufe::Planer);
    EXPECT_EQ(besucht[1], trg::TraegerRaketenStufe::Ceb);
    EXPECT_EQ(besucht[2], trg::TraegerRaketenStufe::TierStufe);

    // ORT != ZEIT: Stufe 3 ist EIN Ort; die ZEIT-Folge der CEB-Emission ist erst Tier, DANN Hybrid.
    auto const folge_ceb = trg::emissions_folge(trg::TraegerRaketenStufe::Ceb);
    ASSERT_EQ(folge_ceb.size(), 2u) << "die CEB baut BEIDE Stufe-3-Traeger (B-10)";
    EXPECT_EQ(folge_ceb[0], trg::EmissionsGegenstand::Tier) << "erst Tier (Owner-Wort)";
    EXPECT_EQ(folge_ceb[1], trg::EmissionsGegenstand::Hybrid) << "spaeter Hybrid, durch die CEB";
    EXPECT_TRUE(trg::emissions_folge(trg::TraegerRaketenStufe::TierStufe).empty()) << "terminal";
    ASSERT_EQ(trg::emissions_folge(trg::TraegerRaketenStufe::Planer).size(), 1u);
    EXPECT_EQ(trg::emissions_folge(trg::TraegerRaketenStufe::Planer)[0], trg::EmissionsGegenstand::Ceb);
}

TEST(S8KopfRakete, TemplateMethodFixiertBauenDannVerifizieren) {
    trg::ProzessBauModul bau_modul; // das ZENTRALE Bau-Modul -- beide Stufen nutzen DIESELBE Instanz

    PlanerProbe planer_stufe;
    auto const  p1 = planer_stufe.emittiere_naechste(bau_modul);
    ASSERT_EQ(p1.size(), 1u);
    EXPECT_EQ(p1[0].gegenstand, trg::EmissionsGegenstand::Ceb);
    EXPECT_EQ(p1[0].form, trg::EmissionsForm::Prozess) << "Prozess, keine YAML";
    EXPECT_EQ(p1[0].bau_kommando, "starte-bau-0");
    ASSERT_EQ(planer_stufe.spur.size(), 2u) << "je Emission: erst bauen, dann verifizieren";
    EXPECT_EQ(planer_stufe.spur[0], "auftrag:0");
    EXPECT_EQ(planer_stufe.spur[1], "verifiziere:0");

    CebProbe   ceb_stufe;
    auto const p2 = ceb_stufe.emittiere_naechste(bau_modul);
    ASSERT_EQ(p2.size(), 2u);
    EXPECT_EQ(p2[0].gegenstand, trg::EmissionsGegenstand::Tier);
    EXPECT_EQ(p2[1].gegenstand, trg::EmissionsGegenstand::Hybrid);
    // Die Template-Method fixiert die SEQUENZ: Tier vollstaendig (bauen+verifizieren) VOR Hybrid.
    ASSERT_EQ(ceb_stufe.spur.size(), 4u);
    EXPECT_EQ(ceb_stufe.spur[0], "auftrag:1");
    EXPECT_EQ(ceb_stufe.spur[1], "verifiziere:1");
    EXPECT_EQ(ceb_stufe.spur[2], "auftrag:2");
    EXPECT_EQ(ceb_stufe.spur[3], "verifiziere:2");
}

// =====================================================================================
// #22(ii) -- "--debug beschleunigt, leitet nie um": Plan-Text byte-invariant unter der
// Freigabe-Umgebung. Gedeckt ist die dump-plan-NAHT (s. Kopf-Kommentar); der CLI-stdout-
// Bytevergleich (main.cpp:721-726) bleibt deklarierter VL-3-Restposten.
// =====================================================================================

class DebugFreigabeUmgebung {
public:
    DebugFreigabeUmgebung() {
        char const* const alt = std::getenv("COMDARE_DEBUG_FREIGABE");
        if (alt != nullptr) { alt_wert_ = alt; }
    }
    ~DebugFreigabeUmgebung() {
        if (alt_wert_.has_value()) {
            ::setenv("COMDARE_DEBUG_FREIGABE", alt_wert_->c_str(), 1);
        } else {
            ::unsetenv("COMDARE_DEBUG_FREIGABE");
        }
    }

private:
    std::optional<std::string> alt_wert_;
};

TEST(S8KopfDebugNeutralitaet, DumpPlanTextByteInvariantUnterDebugFreigabe) {
    DebugFreigabeUmgebung const restaurator; // stellt den Vor-Zustand wieder her (T-8)

    cx::XmlConfigParser const parser;
    auto const                tp = parser.parse_thesis_profile(fs::path{COMDARE_PLANNER_THESIS_ALL_AXES});
    ASSERT_TRUE(tp.has_value()) << "all_axes_golden.profile.xml nicht parsbar";
    auto const ep = parser.parse_experiment_profile(fs::path{COMDARE_EXPERIMENT_GOLDEN});
    ASSERT_TRUE(ep.has_value()) << "experiment_golden.xml nicht parsbar";

    planner::ExperimentPlanDirector const director;

    // Lauf A: Freigabe-Variable GARANTIERT abwesend.
    ::unsetenv("COMDARE_DEBUG_FREIGABE");
    planner::PlanTextBuilder thesis_ohne;
    planner::PlanTextBuilder experiment_ohne;
    director.construct(*tp, thesis_ohne);
    director.construct(*ep, experiment_ohne);

    // Lauf B: Freigabe gesetzt -- exakt der Wert, den das CLI-Gate als Zulassung liest.
    ASSERT_EQ(::setenv("COMDARE_DEBUG_FREIGABE", "true", 1), 0);
    planner::PlanTextBuilder thesis_mit;
    planner::PlanTextBuilder experiment_mit;
    director.construct(*tp, thesis_mit);
    director.construct(*ep, experiment_mit);

    EXPECT_FALSE(thesis_ohne.text().empty());
    EXPECT_FALSE(experiment_ohne.text().empty());
    EXPECT_EQ(thesis_ohne.text(), thesis_mit.text())
        << "#22(ii) verletzt: der Thesis-Plan-Text haengt an COMDARE_DEBUG_FREIGABE (leitet um)";
    EXPECT_EQ(experiment_ohne.text(), experiment_mit.text())
        << "#22(ii) verletzt: der Experiment-Plan-Text haengt an COMDARE_DEBUG_FREIGABE (leitet um)";
}

} // namespace
