// test_hy_a3_hybrid_config_parser -- HY-A3: der Parser der <hybrid_tier>-Sektion, beide Richtungen.
//
// ============================================================================================
// WAS DIESE TU BEWEIST
// ============================================================================================
// (A) DER ZWEIWEIG-SCHALTER TRAEGT. Owner-Doktrin: je Kette gilt die XOR-Regel -- entweder der
//     direkte Tier-Aufbau ODER der Hybrid-Mehrfach-Aufbau, nie beides (NT 05.08.-mittag-7).
//     Im XML ist das `enabled`. ABWESENHEIT der ganzen Sektion heisst "direkter Zweig" und ist
//     KEIN Fehler -- alle Bestands-XMLs bleiben damit gueltig (byte-neutral).
// (B) DIE XML UEBERSCHREIBT DEN DEFAULT. KON42-01 woertlich: constexpr-Default im Planer, jede
//     XML-Eingabe ueberschreibt ihn. Fuer max_docks ist genau das hier belegt -- und zugleich, dass
//     der Programm-Deckel (32, KON28-03) dabei NICHT umgangen werden kann.
// (C) EIN UNBEKANNTES CONTRACT-TOKEN IST EIN BENANNTER FEHLER, KEIN DEFAULT. Koeder: `04aa13b8`
//     (K13-Wuerfelwurf des Bauplans, HY-A3). Bauplan woertlich: "muss die benannte Fehlerklasse
//     liefern, NICHT still default; Beobachtbarkeit: dieselbe Datei mit gueltigem Token direkt
//     daneben gruen (beweist, dass der Rot vom Token kommt)."
//
// WAS SIE NICHT KANN, ausdruecklich benannt: sie sagt nichts darueber, ob die Sektion im
// PRODUKTIVEN Ladeweg ankommt. Der Anschluss an validate_profile und an die Registry ist NICHT
// gebaut -- die Registry-Seite haengt an der offenen Owner-Frage E-6 (welche der Registries die
// "22->23" meint; heute steht KEINE Achsen-Registry auf 22, der Eintrag ginge in die falsche
// Datei). Geprueft ist hier die Parse-Flaeche, nicht ihre Verdrahtung.
//
// @bauplan super docs/plaene/20260809-HYBRID-bauplan-und-entscheidungsvorlage.md Abschnitt IV HY-A3
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitt 4

#include "hybrid/hybrid_config_xml.hpp"

#include <anatomy/anatomy_base.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace cea = ::comdare::cache_engine::anatomy;
namespace hy  = ::comdare::cache_engine::hybrid;

namespace {

/// Eine vollstaendige, gueltige Sektion -- die Referenz, gegen die alle Negativ-Faelle gehalten
/// werden. Sie steht als Funktion und nicht als Konstante, damit jeder Fall seine eigene Kopie
/// veraendern kann, ohne die uebrigen zu beruehren.
[[nodiscard]] std::string gueltige_sektion() {
    return R"(<hybrid_tier enabled="true" genus="SearchAlgorithm">
  <dock_array storage="static" max_docks="8"/>
  <docks>
    <dock id="d0" contract="standard"/>
  </docks>
</hybrid_tier>)";
}

} // namespace

// ============================================================================================
// (A) DER ZWEIWEIG-SCHALTER
// ============================================================================================

TEST(HyA3HybridConfigParser, VollstaendigeSektionWirdGelesen) {
    auto const cfg = hy::parse_hybrid_tier_xml(gueltige_sektion());
    ASSERT_TRUE(cfg.has_value()) << "Status: " << hy::hybrid_status_name(hy::letzter_parse_status(cfg));
    EXPECT_TRUE(cfg->enabled);
    EXPECT_EQ(cfg->genus, cea::AnatomyGenus::SearchAlgorithm);
    EXPECT_TRUE(cfg->storage_statisch);
    EXPECT_EQ(cfg->max_docks, std::size_t{8});
    ASSERT_EQ(cfg->docks.size(), std::size_t{1});
    EXPECT_EQ(cfg->docks[0].contract_id, static_cast<std::uint8_t>(hy::HybridDockContract::Standard));
    EXPECT_EQ(cfg->docks[0].genus, cea::AnatomyGenus::SearchAlgorithm);
}

TEST(HyA3HybridConfigParser, EnabledIstDerXorSchalterZwischenDenZweiZweigen) {
    // enabled="false" ist eine AUSSAGE ("diese Kette faehrt den direkten Tier-Zweig"), kein Fehler.
    // Der Unterschied zur Abwesenheit ist, dass hier jemand bewusst gewaehlt hat -- deshalb wird
    // die Sektion trotzdem vollstaendig geparst und behaelt ihre Werte.
    std::string aus = gueltige_sektion();
    aus.replace(aus.find("enabled=\"true\""), 14, "enabled=\"false\"");

    auto const cfg = hy::parse_hybrid_tier_xml(aus);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->enabled);
    EXPECT_EQ(cfg->docks.size(), std::size_t{1}) << "auch der abgeschaltete Zweig behaelt seine Konfiguration";

    // Und die Kern-Aussage der XOR-Regel, als eigene Funktion abfragbar: genau EIN Zweig ist aktiv.
    EXPECT_FALSE(hy::faehrt_hybrid_zweig(*cfg));
    auto const an = hy::parse_hybrid_tier_xml(gueltige_sektion());
    ASSERT_TRUE(an.has_value());
    EXPECT_TRUE(hy::faehrt_hybrid_zweig(*an));
    EXPECT_NE(hy::faehrt_hybrid_zweig(*an), hy::faehrt_hybrid_zweig(*cfg));
}

TEST(HyA3HybridConfigParser, FehlendesEnabledIstDefaultAusNichtFehler) {
    // Die Abwesenheit einer Deklaration ist eine Aussage und bleibt der byte-neutrale Vor-Zustand
    // (dieselbe Doktrin wie run_methodology_registry.hpp:170: "LEER => Default, UNBEKANNT => hart").
    // Der Default ist AUS: wer die Sektion schreibt, ohne enabled zu setzen, hat den Hybrid-Zweig
    // NICHT angefordert -- und ein ungewollter Hybrid-Aufbau multipliziert die Binary-Menge.
    auto const cfg = hy::parse_hybrid_tier_xml(R"(<hybrid_tier genus="SearchAlgorithm"/>)");
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->enabled);
    EXPECT_TRUE(cfg->docks.empty());
}

TEST(HyA3HybridConfigParser, UnbekanntesEnabledTokenIstHartUndFaelltNichtStillZurueck) {
    // FAIL-CLOSED. Ein "yes"/"1"/"True" ist KEIN true -- die Wire-Form ist "true"/"false", und ein
    // stiller Rueckfall auf false waere die schlimmere Haelfte: der Anwender fordert den Hybrid-Zweig
    // an und bekommt schweigend den direkten.
    for (auto const* token : {"yes", "1", "True", "TRUE", "on", ""}) {
        std::string const xml = std::string{R"(<hybrid_tier enabled=")"} + token +
                                R"(" genus="SearchAlgorithm"><dock_array max_docks="8"/></hybrid_tier>)";
        EXPECT_FALSE(hy::parse_hybrid_tier_xml(xml).has_value()) << "Token \"" << token << "\" darf nicht durchgehen";
    }
    // Gegenprobe im selben Test: die beiden gueltigen Tokens gehen durch.
    EXPECT_TRUE(hy::parse_hybrid_tier_xml(
                    R"(<hybrid_tier enabled="true" genus="Set"><dock_array max_docks="8"/></hybrid_tier>)")
                    .has_value());
    EXPECT_TRUE(hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="false" genus="Set"/>)").has_value());
}

// ============================================================================================
// (B) DER XML-OVERRIDE DES DECKELS (KON42-01) -- und seine Grenze (KON28-03)
// ============================================================================================

TEST(HyA3HybridConfigParser, MaxDocksDefaultGiltNurFuerDenABGESCHALTETENZweig) {
    // W12: bei enabled="true" ist max_docks PFLICHT -- der Default traegt nur den Fall, in dem
    // gar kein Hybrid gebaut wird. Genau das wird hier geprueft: abgeschaltet, kein <dock_array>,
    // und trotzdem eine definierte Zahl -- DIESELBE wie in der Synthese-Matrix und im Dock-Array.
    // Drei Stellen, eine Zahl.
    auto const cfg = hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="false" genus="Set"/>)");
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->max_docks, hy::kHybridNodeObergrenzeDefault);
    EXPECT_EQ(cfg->max_docks, std::size_t{32});
}

TEST(HyA3HybridConfigParser, W12MaxDocksIstBeiHybridAnforderungPFLICHT) {
    // Die Owner-Linie: Dock-Deckel 32 MIT XML-Pflichtangabe. Wer den Zweig anfordert, deklariert
    // seine Dock-Zahl -- weil sie der Faktor auf die Binary-Menge ist. Ein stilles 32 waere die
    // teuerste Voreinstellung des Hauses, getroffen von niemandem.
    auto const ohne = hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="true" genus="Set"/>)");
    EXPECT_FALSE(ohne.has_value());
    EXPECT_EQ(hy::letzter_parse_status(ohne), hy::hybrid_status_max_docks_fehlt);

    // Auch ein <dock_array> OHNE das Attribut genuegt nicht -- die Sektion allein ist keine Angabe.
    auto const leer = hy::parse_hybrid_tier_xml(
        R"(<hybrid_tier enabled="true" genus="Set"><dock_array storage="runtime"/></hybrid_tier>)");
    EXPECT_FALSE(leer.has_value());
    EXPECT_EQ(hy::letzter_parse_status(leer), hy::hybrid_status_max_docks_fehlt);

    // GEGENPROBEN, damit der Rot wirklich von der fehlenden Angabe kommt:
    // (a) mit Angabe ist derselbe Zweig gruen,
    auto const mit = hy::parse_hybrid_tier_xml(
        R"(<hybrid_tier enabled="true" genus="Set"><dock_array max_docks="4"/></hybrid_tier>)");
    ASSERT_TRUE(mit.has_value());
    EXPECT_EQ(mit->max_docks, std::size_t{4});
    // (b) und der ABGESCHALTETE Zweig braucht sie weiterhin nicht.
    EXPECT_TRUE(hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="false" genus="Set"/>)").has_value());

    // Der Status ist von "falscher Wert" UNTERSCHIEDEN -- zwei verschiedene Anwenderfehler.
    EXPECT_NE(hy::hybrid_status_max_docks_fehlt, hy::hybrid_status_max_docks_ungueltig);
    EXPECT_EQ(hy::hybrid_status_name(hy::hybrid_status_max_docks_fehlt), std::string_view{"max_docks_fehlt"});
}

TEST(HyA3HybridConfigParser, XmlUeberschreibtDenDefaultWirklich) {
    // KON42-01, am Objekt: der Wert aus der XML gewinnt gegen den constexpr-Default.
    std::string xml = gueltige_sektion();
    xml.replace(xml.find("max_docks=\"8\""), 13, "max_docks=\"3\"");
    auto const cfg = hy::parse_hybrid_tier_xml(xml);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_EQ(cfg->max_docks, std::size_t{3});
    EXPECT_NE(cfg->max_docks, hy::kHybridNodeObergrenzeDefault) << "sonst ueberschreibt die XML gar nichts";
}

TEST(HyA3HybridConfigParser, DerProgrammDeckelLaesstSichNichtPerXmlUmgehen) {
    // Die Obergrenze ist willkuerlich gesetzt und W7-anpassbar -- aber sie wird an EINER Stelle
    // angehoben, nicht per XML unterlaufen. 32 selbst ist zulaessig (die Grenze ist inklusiv), 33 nicht.
    std::string zu_gross = gueltige_sektion();
    zu_gross.replace(zu_gross.find("max_docks=\"8\""), 13, "max_docks=\"33\"");
    auto const zu_gross_erg = hy::parse_hybrid_tier_xml(zu_gross);
    EXPECT_FALSE(zu_gross_erg.has_value());
    // F-7: der GRUND gehoert mitgeprueft -- ein blosses nullopt saehe aus wie ein Syntaxfehler.
    EXPECT_EQ(hy::letzter_parse_status(zu_gross_erg), hy::hybrid_status_max_docks_ungueltig);

    std::string genau = gueltige_sektion();
    genau.replace(genau.find("max_docks=\"8\""), 13, "max_docks=\"32\"");
    auto const cfg = hy::parse_hybrid_tier_xml(genau);
    ASSERT_TRUE(cfg.has_value()) << "32 ist der Deckel SELBST und muss zulaessig bleiben";
    EXPECT_EQ(cfg->max_docks, std::size_t{32});

    // 0 Docks ist ebenfalls kein sinnvoller Wert -- ein Array ohne Platz traegt kein Dock.
    std::string null = gueltige_sektion();
    null.replace(null.find("max_docks=\"8\""), 13, "max_docks=\"0\"");
    auto const null_erg = hy::parse_hybrid_tier_xml(null);
    EXPECT_FALSE(null_erg.has_value());
    EXPECT_EQ(hy::letzter_parse_status(null_erg), hy::hybrid_status_max_docks_ungueltig);
}

TEST(HyA3HybridConfigParser, StorageWaehltDiePolicyUndKenntNurZweiWerte) {
    std::string runtime = gueltige_sektion();
    runtime.replace(runtime.find("storage=\"static\""), 16, "storage=\"runtime\"");
    auto const cfg = hy::parse_hybrid_tier_xml(runtime);
    ASSERT_TRUE(cfg.has_value());
    EXPECT_FALSE(cfg->storage_statisch);

    std::string falsch = gueltige_sektion();
    falsch.replace(falsch.find("storage=\"static\""), 16, "storage=\"dynamic\"");
    EXPECT_FALSE(hy::parse_hybrid_tier_xml(falsch).has_value()) << "\"dynamic\" ist kein Policy-Token";
}

// ============================================================================================
// (C) DER KOEDER: ein unbekanntes contract-Token
// ============================================================================================

TEST(HyA3HybridConfigParser, UnbekanntesContractTokenLiefertDieBenannteFehlerklasse) {
    // Bauplan-Koeder (K13): 04aa13b8. Er steht in keiner Registry und in keiner Doku.
    std::string koeder = gueltige_sektion();
    koeder.replace(koeder.find("contract=\"standard\""), 19, "contract=\"04aa13b8\"");

    auto const cfg = hy::parse_hybrid_tier_xml(koeder);
    EXPECT_FALSE(cfg.has_value()) << "ein unbekanntes contract-Token darf NICHT still auf standard fallen";

    // BEOBACHTBARKEIT: der Status muss BENANNT sein und den GRUND treffen. Ein blosses nullopt
    // saehe genauso aus wie ein Syntax-Fehler -- und der Anwender suchte an der falschen Stelle.
    EXPECT_EQ(hy::letzter_parse_status(cfg), hy::hybrid_status_unbekannter_vertrag);
    EXPECT_EQ(hy::hybrid_status_name(hy::letzter_parse_status(cfg)), std::string_view{"unbekannter_vertrag"});

    // GEGENPROBE IM SELBEN TEST, wie vom Bauplan verlangt: dieselbe Sektion mit gueltigem Token ist
    // gruen. Ohne sie koennte der Rot auch von der Sektion selbst kommen statt vom Token.
    auto const gut = hy::parse_hybrid_tier_xml(gueltige_sektion());
    ASSERT_TRUE(gut.has_value());
    EXPECT_EQ(hy::letzter_parse_status(gut), hy::hybrid_status_ok);
}

TEST(HyA3HybridConfigParser, VertragOhneDockTypWirdVomVertragsFehlerUnterschieden) {
    // rollback STEHT in der Registry, hat aber (F8-Minimal) noch keinen Dock-Typ. Das ist ein
    // ANDERER Zustand als "unbekanntes Token" -- der eine ist ein Eingabefehler, der andere der
    // Stand des Baus. Sie zusammenzuwerfen liesse den Anwender an der falschen Stelle suchen.
    std::string rollback = gueltige_sektion();
    rollback.replace(rollback.find("contract=\"standard\""), 19, "contract=\"rollback\"");

    auto const cfg = hy::parse_hybrid_tier_xml(rollback);
    EXPECT_FALSE(cfg.has_value());
    EXPECT_EQ(hy::letzter_parse_status(cfg), hy::hybrid_status_contract_ohne_dock_typ);
    EXPECT_NE(hy::letzter_parse_status(cfg), hy::hybrid_status_unbekannter_vertrag);
}

TEST(HyA3HybridConfigParser, KlassifikationsGenusIstKeinZielUndDasGateGreiftAuchHier) {
    // Dasselbe Gate wie an der Dock-Kante -- die Sperre darf nicht davon abhaengen, auf welchem Weg
    // ein Deskriptor entsteht (XML oder Code).
    auto const cfg = hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="true" genus="FunctionInterfaceReroute">)"
                                               R"(<docks><dock id="d0" contract="standard"/></docks></hybrid_tier>)");
    EXPECT_FALSE(cfg.has_value());
    EXPECT_EQ(hy::letzter_parse_status(cfg), hy::hybrid_status_kein_zielfaehiges_genus);
}

TEST(HyA3HybridConfigParser, UnbekanntesGenusTokenIstHart) {
    EXPECT_FALSE(hy::parse_hybrid_tier_xml(
                     R"(<hybrid_tier enabled="true" genus="Baum"><dock_array max_docks="8"/></hybrid_tier>)")
                     .has_value());
    // F-5: FEHLT und FALSCH sind zwei verschiedene Anwenderfehler -- und tragen zwei Codes.
    auto const ohne_genus =
        hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="true"><dock_array max_docks="8"/></hybrid_tier>)");
    EXPECT_FALSE(ohne_genus.has_value()) << "genus ist Pflicht -- ein Hybrid ohne Gattung waere ein Gehaeuse ohne Ziel";
    EXPECT_EQ(hy::letzter_parse_status(ohne_genus), hy::hybrid_status_genus_fehlt);
    EXPECT_NE(hy::letzter_parse_status(ohne_genus), hy::hybrid_status_unbekanntes_token);
    // Gegenprobe: ein FALSCH GESCHRIEBENES genus bleibt unbekanntes_token.
    auto const falsches_genus = hy::parse_hybrid_tier_xml(
        R"(<hybrid_tier enabled="true" genus="Baum"><dock_array max_docks="8"/></hybrid_tier>)");
    EXPECT_FALSE(falsches_genus.has_value());
    EXPECT_EQ(hy::letzter_parse_status(falsches_genus), hy::hybrid_status_unbekanntes_token);
    // Gegenprobe: alle fuenf ABI-sichtbaren Genera gehen durch (ein Gate, das nur ablehnt, waere
    // genauso falsch wie eines, das alles zulaesst).
    for (auto const* g : {"SearchAlgorithm", "Set", "Sequence", "Adapter", "View"}) {
        std::string const xml = std::string{R"(<hybrid_tier enabled="true" genus=")"} + g +
                                R"("><dock_array max_docks="8"/></hybrid_tier>)";
        EXPECT_TRUE(hy::parse_hybrid_tier_xml(xml).has_value()) << g;
    }
}

// ============================================================================================
// BYTE-NEUTRALITAET: die Sektion ist OPTIONAL
// ============================================================================================

TEST(HyA3HybridConfigParser, EinDokumentOhneHybridSektionIstKeinFehler) {
    // Der Kern der Byte-Neutralitaets-Zusage (soll_design Abschnitt 4: additives OPTIONALES Element,
    // minOccurs=0, "damit alle Bestands-XMLs valide bleiben"). Die Suchfunktion liefert "nicht da",
    // NICHT "kaputt" -- der Unterschied entscheidet, ob eine Bestands-XML weiterlaeuft.
    std::string_view const ohne =
        R"(<comdare_experiment><run_methodology>measure</run_methodology></comdare_experiment>)";
    auto const gefunden = hy::finde_hybrid_tier_sektion(ohne);
    EXPECT_FALSE(gefunden.has_value());
    EXPECT_EQ(hy::letzter_parse_status(gefunden), hy::hybrid_status_ok) << "Abwesenheit ist kein Fehlerzustand";

    // Und mit Sektion wird sie gefunden -- sonst waere der Test oben trivial gruen.
    std::string const mit = std::string{"<comdare_experiment>"} + gueltige_sektion() + "</comdare_experiment>";
    auto const        da  = hy::finde_hybrid_tier_sektion(mit);
    ASSERT_TRUE(da.has_value());
    EXPECT_TRUE(da->enabled);
    EXPECT_EQ(da->docks.size(), std::size_t{1});
}

TEST(HyA3HybridConfigParser, NichtWohlgeformtesXmlIstEinBenannterFehler) {
    // F-7: der Testname verspricht "BenannterFehler" -- also wird die Benennung auch geprueft.
    auto const abgeschnitten = hy::parse_hybrid_tier_xml("<hybrid_tier enabled=");
    EXPECT_FALSE(abgeschnitten.has_value());
    EXPECT_EQ(hy::letzter_parse_status(abgeschnitten), hy::hybrid_status_xml_nicht_wohlgeformt);
    auto const leer = hy::parse_hybrid_tier_xml("");
    EXPECT_FALSE(leer.has_value());
    EXPECT_EQ(hy::letzter_parse_status(leer), hy::hybrid_status_xml_nicht_wohlgeformt);
    // Falscher Wurzel-Tag: das ist KEINE Hybrid-Sektion, auch wenn es wohlgeformt ist.
    EXPECT_FALSE(hy::parse_hybrid_tier_xml(R"(<hybrid enabled="true" genus="Set"><dock_array max_docks="8"/></hybrid>)")
                     .has_value());
}

TEST(HyA3HybridConfigParser, MehrDocksAlsMaxDocksIstEinBenannterFehler) {
    // Die beiden Zahlen der Sektion muessen zueinander passen: eine Bestueckung, die den selbst
    // gesetzten Deckel sprengt, ist in sich widerspruechlich. Sie erst beim attach auffallen zu
    // lassen hiesse, den Fehler eine Stufe zu spaet zu melden.
    auto const zu_viele = hy::parse_hybrid_tier_xml(R"(<hybrid_tier enabled="true" genus="Set">
  <dock_array storage="runtime" max_docks="1"/>
  <docks><dock id="d0" contract="standard"/><dock id="d1" contract="standard"/></docks>
</hybrid_tier>)");
    EXPECT_FALSE(zu_viele.has_value());
    // F-7: auch hier der GRUND -- der Testname verspricht ihn.
    EXPECT_EQ(hy::letzter_parse_status(zu_viele), hy::hybrid_status_mehr_docks_als_deckel);
}
