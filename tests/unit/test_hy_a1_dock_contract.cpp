// test_hy_a1_dock_contract -- HY-A1 (Wellenplan-Definition): Dock-Contract + Pruef-Dock + Factory
// + Dock-Array der Hybrid-Stufe, beide Richtungen.
//
// ============================================================================================
// WAS DIESE TU BEWEIST -- UND WAS SIE AUSDRUECKLICH NICHT KANN
// ============================================================================================
// (A) DIE VERTRAGS-REGISTRY IST DIE EINZELQUELLE. Vier Vertraege (standard/rollback/scan/
//     resource_control), Index == Enum-Wert, Token-Lookup aus DERSELBEN Tabelle. Muster:
//     run_methodology_registry.hpp:93-121 (constexpr-Tabelle + consteval-Vollstaendigkeit +
//     Namens-Anker), soll_design Abschnitt 3.1 nennt genau dieses Vorbild.
// (B) DAS DOCK-ARRAY TRAEGT BEIDE POLICIES. Owner-E1 woertlich: "wahlweise statisches oder
//     runtime array". Die Belegung ist in BEIDEN dynamisch; statisch setzt nur die
//     Compile-Time-KAPAZITAET (soll_design 3.1 (c)).
// (C) DIE FACTORY IST DER EINZIGE KONSTRUKTIONS-ORT der variant-Alternativen (Paragraf-49-KORREKTUR
//     woertlich: "per Abstract-Factory-Methode gelesen und verarbeitet").
// (D) DER HOT-PATH IST VARIANT-FREI. Nach dem attach-Zeit-visit haelt der Slot einen gecachten
//     statischen Antrieb (IObservableTier*); die Op-Strecke fasst den variant nie an
//     (soll_design 3.2 Punkt 2, Risiko-Zeile 1 der Tabelle in Abschnitt 9).
// (E) DER VARIANT LEBT NUR AN EINER STELLE IM hybrid/-BAUM. Das ist eine ZONEN-Aussage ueber
//     Dateien, kein Typ-Praedikat -- deshalb liest der letzte Test die Quelldateien selbst.
//     Der Bauplan verlangt sie ausdruecklich mit "Scope ausdruecklich hybrid/-lokal", weil die
//     System-weite Fassung des Satzes heute nicht zutrifft (Bauplan Widerspruch 8).
//
// WAS SIE NICHT KANN, ausdruecklich benannt: sie beweist KEINEN echten Reroute auf eine plain
// Tier-Binary. Dafuer braucht es den Proxy mit AnatomyModuleLoader -- und dessen Schichten-Zuordnung
// ist die offene Auflage K2 (soll_design Abschnitt 7). Der Roundtrip ist HY-A2 und steht dort mit
// eigenem Koeder. Wer diese TU als Reroute-Beweis liest, liest sie falsch.
//
// @autoritaet Owner-Entscheid E1 02.08.2026 (super docs/sessions/20260802-OWNER-entscheide-*.md:7)
// @design docs/architecture/20260802-hybrid_tier_stufe_soll_design.md Abschnitte 3.1/3.2, K6
// @bauplan super docs/plaene/20260809-HYBRID-bauplan-und-entscheidungsvorlage.md Abschnitt IV HY-A1

#include "hybrid/hybrid_dock_array.hpp"
#include "hybrid/hybrid_dock_contract.hpp"
#include "hybrid/hybrid_dock_factory.hpp"
#include "hybrid/hybrid_pruef_dock.hpp"

#include <anatomy/anatomy_base.hpp>
#include <anatomy/observable_tier.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace cea = ::comdare::cache_engine::anatomy;
namespace hy  = ::comdare::cache_engine::hybrid;

namespace {

// ============================================================================================
// EINE ECHTE ZIEL-ATTRAPPE -- kein fabrizierter Zeiger
// ============================================================================================
// Die Bindung wird ueber ZEIGER-IDENTITAET geprueft. Ein aus einer Zahl gecasteter Zeiger taete es
// technisch auch, waere aber implementierungsabhaengig und -- wichtiger -- er koennte nicht zeigen,
// dass ZWEI verschiedene Ziele wirklich verschieden ankommen. Deshalb eine echte, minimale
// IObservableTier-Ableitung nach dem Bestands-Muster (test_e3_contract_conformance_gate_wirksam.cpp:37).
// Sie speichert nichts und misst nichts: gebraucht wird allein ihre ADRESSE.
class ZielAttrappe final : public cea::IObservableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t, std::uint64_t) noexcept override { return false; }
    [[nodiscard]] bool tier_lookup(std::uint64_t, std::uint64_t*) const noexcept override { return false; }
    [[nodiscard]] bool tier_erase(std::uint64_t) noexcept override { return false; }
    void               tier_clear() noexcept override {}
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return 0; }
    void                        tier_observe(cea::ComdareTierObserverSnapshot* out) const noexcept override {
        if (out != nullptr) *out = cea::ComdareTierObserverSnapshot{};
    }
    void tier_measure_accept(cea::IMessVisitor&) const noexcept override {}
    void tier_reset_statistics() noexcept override {}
};

// ============================================================================================
// COMPILE-TIME-BEWEISE (laufen bei JEDEM Bau dieser TU mit)
// ============================================================================================

// (A) Die Registry ist vollstaendig und ihre Tokens sind angenagelt.
static_assert(hy::kHybridDockContractRegistry.size() == hy::kHybridDockContractCount);
static_assert(hy::hybrid_dock_contract_of_token("standard") == hy::HybridDockContract::Standard);
static_assert(hy::hybrid_dock_contract_of_token("resource_control") == hy::HybridDockContract::ResourceControl);

// Der DETEKTOR trennt "unbekannt" von "bekannt" -- ohne ihn koennte die Token-Pruefung auf
// immer-wahr degenerieren und der Negativ-Test faenge nichts mehr.
static_assert(hy::hybrid_dock_contract_token_bekannt("standard"));
static_assert(!hy::hybrid_dock_contract_token_bekannt("contract_f0a01c9e"),
              "HY-A1-DOCK: der Koeder-Token DARF NICHT in der Vertrags-Registry stehen. Steht er "
              "drin, ist die Negativ-Fixture wertlos -- sie wuerde gruen durchlaufen.");

// (B) Der Deskriptor ist wirklich ein POD (soll_design 3.1 (a): "POD, trivially_copyable").
static_assert(std::is_trivially_copyable_v<hy::DockContractDescriptor>);
static_assert(std::is_standard_layout_v<hy::DockContractDescriptor>);

// (C) Beide Policies existieren, und "statisch" heisst KAPAZITAET, nicht feste Belegung.
static_assert(hy::StatischeDockArrayPolicy<4>::statisch);
static_assert(hy::StatischeDockArrayPolicy<4>::kapazitaet == 4);
static_assert(!hy::RuntimeDockArrayPolicy::statisch);

// (D) HOT-PATH-KONFORMITAET, die Kern-Zusage der Stufe: der gecachte Antrieb ist der STATISCHE
//     IObservableTier-Zeiger, kein variant und kein Ergebnis eines Casts pro Operation.
static_assert(std::is_same_v<decltype(std::declval<hy::StandardHybridDock const&>().antrieb()),
                             cea::IObservableTier*>,
              "HY-A1-HOTPATH: der gecachte Antrieb MUSS IObservableTier* sein. Wird hier ein "
              "variant oder ein Wrapper daraus, laeuft der visit im Op-Pfad statt am Umschaltpunkt "
              "-- genau die Risiko-Zeile 1 der soll_design-Tabelle in Abschnitt 9.");

// (E) Der variant traegt heute GENAU EINE Alternative -- und das ist kein Versehen, sondern der
//     F8-Minimal-Schnitt (genau 1 Standard-Dock). Die Zahl steigt mit jedem neuen VERTRAGS-DOCK-TYP,
//     NICHT mit einem neuen Vertrag in der Registry: Vertraege sind Daten, Dock-Typen sind Code.
static_assert(std::variant_size_v<hy::HybridDockVariant> == 1,
              "HY-A1-VARIANT: der F8-Minimal-DoD traegt genau EINEN Dock-Typ (StandardHybridDock). "
              "Wer einen Alternativ-Vertrags-Dock baut, zieht diese Zahl bewusst mit -- die "
              "Registry-Zahl 4 bleibt davon unberuehrt (Daten gegen Code).");
static_assert(std::is_same_v<std::variant_alternative_t<0, hy::HybridDockVariant>, hy::StandardHybridDock>);

} // namespace

// ============================================================================================
// (A) DIE VERTRAGS-REGISTRY
// ============================================================================================

TEST(HyA1DockContract, RegistryIstEinzelquelleUndIndexTreu) {
    // Die Schleife laeuft ueber die REGISTRY, nicht ueber eine im Test gefuehrte Liste -- genau
    // daran ist die alte kAllGenera-Wache gescheitert (heuristik_adapter_klassifikation.hpp:20-30).
    for (std::size_t i = 0; i < hy::kHybridDockContractCount; ++i) {
        auto const& info = hy::kHybridDockContractRegistry[i];
        EXPECT_EQ(static_cast<std::size_t>(info.contract), i) << "Index-Treue verletzt bei " << i;
        EXPECT_FALSE(info.id.empty());
        EXPECT_FALSE(info.name.empty());
        // Rundreise Token -> Vertrag -> Token: der Lookup nimmt wirklich DIESE Tabelle.
        // Hier die LAUFZEIT-Form (info.id ist ein Schleifenwert); die consteval-Form steht oben
        // als static_assert. Beide muessen dasselbe liefern -- sonst haette der Parser eine andere
        // Wahrheit als die compile-time-Verwendungsstellen.
        EXPECT_TRUE(hy::hybrid_dock_contract_token_bekannt(info.id)) << info.id;
        auto const gefunden = hy::hybrid_dock_contract_lookup(info.id);
        ASSERT_TRUE(gefunden.has_value()) << info.id;
        EXPECT_EQ(*gefunden, info.contract) << info.id;
    }
}

TEST(HyA1DockContract, TokensSindPaarweiseVerschieden) {
    // Ohne diese Probe koennten zwei Zeilen dasselbe Token tragen; der Lookup traefe dann immer
    // die erste und die zweite waere unerreichbar -- still, ohne dass eine andere Wache anschlaegt.
    for (std::size_t i = 0; i < hy::kHybridDockContractCount; ++i) {
        for (std::size_t j = i + 1; j < hy::kHybridDockContractCount; ++j) {
            EXPECT_NE(hy::kHybridDockContractRegistry[i].id, hy::kHybridDockContractRegistry[j].id)
                << "Vertrags-Token " << i << " und " << j << " sind gleich";
        }
    }
}

TEST(HyA1DockContract, UnbekanntesTokenIstKeinDefault) {
    // FAIL-CLOSED, nach dem Haus-Vorbild run_methodology_for_ids (run_methodology_registry.hpp:188):
    // ein Tippfehler darf NICHT still auf den ersten Eintrag fallen. Hier ist der Koeder-Token die
    // Probe -- er ist gewuerfelt (K13) und steht in keiner Doku.
    EXPECT_FALSE(hy::hybrid_dock_contract_token_bekannt("contract_f0a01c9e"));
    EXPECT_FALSE(hy::hybrid_dock_contract_token_bekannt(""));
    EXPECT_FALSE(hy::hybrid_dock_contract_token_bekannt("Standard")); // Gross/klein ist NICHT egal
    // Gegenprobe im selben Test, damit der Rot-Beweis wirklich vom Token kommt und nicht davon,
    // dass die Funktion generell false liefert (Bauplan HY-A1, Koeder-Beobachtbarkeit).
    EXPECT_TRUE(hy::hybrid_dock_contract_token_bekannt("standard"));
}

// ============================================================================================
// (C) DIE FACTORY -- EINZIGER KONSTRUKTIONS-ORT
// ============================================================================================

TEST(HyA1DockContract, FactoryBautDenStandardVertragUndLehntUnbekannteAb) {
    hy::HybridDockVariant slot_dock{};

    hy::DockContractDescriptor const gut{static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                                         cea::AnatomyGenus::SearchAlgorithm};
    EXPECT_EQ(hy::HybridDockFactory::make_dock(gut, slot_dock), hy::hybrid_status_ok);
    // Nicht index() pruefen -- der ist bei einer einzigen Alternative tautologisch 0. Geprueft wird,
    // dass wirklich der Standard-Dock drinsteht und seinen eigenen Vertrag nennt.
    EXPECT_EQ(std::visit([](auto const& d) { return d.dock_name(); }, slot_dock),
              std::string_view{"StandardHybridDock"});
    EXPECT_EQ(std::visit([](auto const& d) { return d.contract(); }, slot_dock), hy::HybridDockContract::Standard);

    // Ein Vertrag, der in der Registry STEHT, aber noch keinen Dock-TYP hat: das ist der reale
    // Zwischenzustand des F8-Minimals (4 Vertraege, 1 Dock-Typ). Er muss benannt scheitern, nicht
    // still den Standard-Dock liefern -- sonst traegt ein rollback-Slot heimlich Standard-Semantik.
    hy::DockContractDescriptor const ungebaut{static_cast<std::uint8_t>(hy::HybridDockContract::Rollback),
                                              cea::AnatomyGenus::SearchAlgorithm};
    EXPECT_EQ(hy::HybridDockFactory::make_dock(ungebaut, slot_dock), hy::hybrid_status_contract_ohne_dock_typ);

    // Ein Vertrags-Index JENSEITS der Registry (die Wire-Form ist ein uint8_t und kann alles tragen).
    hy::DockContractDescriptor const unbekannt{200, cea::AnatomyGenus::SearchAlgorithm};
    EXPECT_EQ(hy::HybridDockFactory::make_dock(unbekannt, slot_dock), hy::hybrid_status_unbekannter_vertrag);

    // Und die Ziel-Gattung wird geprueft: ein Klassifikations-Genus ist nie ein Reroute-Ziel
    // (das Gate aus HY-A1-Klassifikation, hier an der Dock-Kante durchgesetzt).
    hy::DockContractDescriptor const kein_ziel{static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                                               cea::AnatomyGenus::FunctionInterfaceReroute};
    EXPECT_EQ(hy::HybridDockFactory::make_dock(kein_ziel, slot_dock), hy::hybrid_status_kein_zielfaehiges_genus);
}

TEST(HyA1DockContract, JederStatusHatEinenNamen) {
    // NAHT-1-Lehre (pruef_dock.hpp:59-63): ein Status ohne Namen ist im Bericht STUMM und sieht aus
    // wie ein bekannter Fehler. Deshalb wird die Namens-Totalitaet hier mitgeprueft.
    int const alle[] = {hy::hybrid_status_ok, hy::hybrid_status_unbekannter_vertrag,
                        hy::hybrid_status_contract_ohne_dock_typ, hy::hybrid_status_kein_zielfaehiges_genus,
                        hy::hybrid_status_array_voll, hy::hybrid_status_slot_leer};
    for (int const s : alle) {
        EXPECT_NE(hy::hybrid_status_name(s), std::string_view{"unknown"}) << "Status " << s << " ist stumm";
    }
    // Gegenprobe: ein Status, den es nicht gibt, MUSS "unknown" heissen -- sonst waere die
    // Namensfunktion auf immer-benannt degeneriert und die Probe oben wertlos.
    EXPECT_EQ(hy::hybrid_status_name(99), std::string_view{"unknown"});
}

// ============================================================================================
// (B) DAS DOCK-ARRAY -- beide Policies, Belegung immer dynamisch
// ============================================================================================

TEST(HyA1DockContract, StatischePolicyDeckeltDieKapazitaetNichtDieBelegung) {
    hy::DockArray<hy::StatischeDockArrayPolicy<2>> arr;
    EXPECT_EQ(arr.kapazitaet(), std::size_t{2});
    EXPECT_EQ(arr.size(), std::size_t{0}); // DYNAMISCH: leer, obwohl die Kapazitaet 2 ist

    hy::DockContractDescriptor const d{static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                                       cea::AnatomyGenus::SearchAlgorithm};
    EXPECT_EQ(arr.attach(d), 0);  // Slot-Index 0
    EXPECT_EQ(arr.size(), std::size_t{1});
    EXPECT_EQ(arr.attach(d), 1);
    EXPECT_EQ(arr.size(), std::size_t{2});

    // Die Kapazitaets-Grenze wird benannt gemeldet, nicht still ueberschrieben.
    EXPECT_EQ(arr.attach(d), -hy::hybrid_status_array_voll);
    EXPECT_EQ(arr.size(), std::size_t{2});

    // detach gibt den Slot wirklich frei -- und der naechste attach nimmt ihn wieder.
    EXPECT_EQ(arr.detach(0), hy::hybrid_status_ok);
    EXPECT_EQ(arr.size(), std::size_t{1});
    EXPECT_EQ(arr.attach(d), 0) << "der freigegebene Slot 0 muss wiederverwendet werden";

    // Ein zweites detach auf denselben Slot ist ein benannter Fehler, kein stiller No-Op.
    EXPECT_EQ(arr.detach(1), hy::hybrid_status_ok);
    EXPECT_EQ(arr.detach(1), hy::hybrid_status_slot_leer);
    EXPECT_EQ(arr.detach(99), hy::hybrid_status_slot_leer);
}

TEST(HyA1DockContract, RuntimePolicyWaechstUndTraegtDenselbenVertrag) {
    hy::DockArray<hy::RuntimeDockArrayPolicy> arr;
    EXPECT_EQ(arr.size(), std::size_t{0});

    hy::DockContractDescriptor const d{static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                                       cea::AnatomyGenus::Set};
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(arr.attach(d), i);
    }
    EXPECT_EQ(arr.size(), std::size_t{5});

    // Die Runtime-Policy deckelt auf denselben PROGRAMM-DECKEL wie die Synthese-Matrix (KON28-03:
    // "maximal 32"). Der Deckel ist eine willkuerlich gesetzte Maximal-Variable, kein Fach-Nenner
    // -- Docks sind KEINE Mess-Permutationen (KON41-03).
    EXPECT_EQ(arr.kapazitaet(), hy::kHybridNodeObergrenzeDefault);

    // Und der Slot traegt wirklich seinen Deskriptor zurueck (nicht eine Kopie des ersten).
    auto const* slot = arr.slot(3);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->desc.genus, cea::AnatomyGenus::Set);
    EXPECT_EQ(slot->desc.contract_id, static_cast<std::uint8_t>(hy::HybridDockContract::Standard));
    EXPECT_EQ(arr.slot(99), nullptr);
}

TEST(HyA1DockContract, DerDeckelIstDIESELBEZahlWieInDerSyntheseMatrix) {
    // EINE Wahrheit, nicht zwei. Der Zahlen-Widerspruch 8-gegen-32 ist per KON28-03 aufgeloest
    // (32 loest die 8 ab); wer hier zwei getrennte Konstanten pflegt, holt ihn zurueck.
    EXPECT_EQ(hy::kHybridNodeObergrenzeDefault, std::size_t{32});
    EXPECT_EQ(hy::DockArray<hy::RuntimeDockArrayPolicy>{}.kapazitaet(), hy::kHybridNodeObergrenzeDefault);
}

// ============================================================================================
// (D) HOT-PATH: der visit laeuft am UMSCHALTPUNKT, nicht je Operation
// ============================================================================================

TEST(HyA1DockContract, AntriebWirdBeimAttachGecachedUndDanachOhneVisitGelesen) {
    hy::DockArray<hy::StatischeDockArrayPolicy<2>> arr;
    hy::DockContractDescriptor const               d{static_cast<std::uint8_t>(hy::HybridDockContract::Standard),
                                       cea::AnatomyGenus::SearchAlgorithm};
    ASSERT_EQ(arr.attach(d), 0);

    // Vor der Bindung ist der Antrieb ehrlich leer -- KEIN Platzhalter-Objekt, das spaeter wie ein
    // echtes Ziel aussieht. Das ist die Stelle, an der HY-A2 den Proxy einhaengt.
    auto const* slot = arr.slot(0);
    ASSERT_NE(slot, nullptr);
    EXPECT_EQ(slot->antrieb, nullptr);
    EXPECT_FALSE(arr.slot_ist_bereit(0));

    // Die Bindung laeuft ueber den Umschaltpunkt -- EINMAL, mit visit. Danach liest der Op-Pfad
    // nur noch den gecachten Zeiger.
    ZielAttrappe                ziel_a;
    cea::IObservableTier* const attrappe = &ziel_a;
    EXPECT_EQ(arr.antrieb_binden(0, attrappe), hy::hybrid_status_ok);
    EXPECT_TRUE(arr.slot_ist_bereit(0));
    EXPECT_EQ(arr.slot(0)->antrieb, attrappe);

    // Der Op-Pfad-Zugriff ist variant-frei: er liefert denselben Zeiger, ohne den variant anzufassen.
    EXPECT_EQ(arr.antrieb_von(0), attrappe);
    EXPECT_EQ(arr.antrieb_von(1), nullptr); // unbelegter Slot -> kein Antrieb, kein UB

    // ZWEI Slots, ZWEI Ziele: die Bindung ist wirklich slot-lokal. Ohne diese Probe koennte
    // antrieb_binden global schreiben und der Test oben bliebe gruen -- bei einem einzigen Ziel
    // sieht ein globaler Zeiger genauso aus wie ein slot-lokaler.
    ZielAttrappe ziel_b;
    ASSERT_EQ(arr.attach(d), 1);
    EXPECT_EQ(arr.antrieb_binden(1, &ziel_b), hy::hybrid_status_ok);
    EXPECT_EQ(arr.antrieb_von(0), attrappe);
    EXPECT_EQ(arr.antrieb_von(1), &ziel_b);
    EXPECT_NE(arr.antrieb_von(0), arr.antrieb_von(1));
    EXPECT_EQ(arr.detach(1), hy::hybrid_status_ok);

    // detach loescht die Bindung mit -- ein Slot darf NIE einen Antrieb ueberleben lassen, dessen
    // Modul entladen wurde (soll_design 3.3: destroy-vor-dlclose, Quiesce am Op-Rand).
    EXPECT_EQ(arr.detach(0), hy::hybrid_status_ok);
    EXPECT_EQ(arr.antrieb_von(0), nullptr);
}

// ============================================================================================
// (E) DIE ZONEN-WACHE: der variant lebt an GENAU EINER Stelle im hybrid/-Baum
// ============================================================================================

TEST(HyA1DockContract, StdVariantStehtNurInDerEinenErlaubtenDatei) {
    // WARUM DAS EIN DATEI-TEST IST UND KEIN static_assert: "es gibt im Verzeichnis nur eine
    // variant-Stelle" ist eine Aussage ueber QUELLTEXT, nicht ueber Typen. Kein Typ-Praedikat kann
    // sie treffen -- ein zweiter variant in einer anderen Datei waere fuer jeden static_assert
    // unsichtbar. Der Bauplan verlangt genau deshalb einen eigenen Guard mit "Scope ausdruecklich
    // hybrid/-lokal": die system-weite Fassung des Satzes ist heute NICHT wahr (CEB-Duldung +
    // zwei achsenfremde Bestandsnutzungen, Bauplan Widerspruch 8), die zonen-lokale ist es.
    std::filesystem::path const hybrid_dir{COMDARE_HY_QUELLVERZEICHNIS};
    ASSERT_TRUE(std::filesystem::is_directory(hybrid_dir)) << hybrid_dir;

    std::vector<std::string> traeger;
    std::size_t              geprueft = 0;
    for (auto const& eintrag : std::filesystem::directory_iterator{hybrid_dir}) {
        if (!eintrag.is_regular_file()) continue;
        auto const ext = eintrag.path().extension().string();
        if (ext != ".hpp" && ext != ".cpp") continue;
        ++geprueft;

        std::ifstream     ein{eintrag.path()};
        ASSERT_TRUE(ein.is_open()) << eintrag.path();
        std::ostringstream puffer;
        puffer << ein.rdbuf();
        std::string const inhalt = puffer.str();

        // Gesucht wird die VERWENDUNG als Typ ("std::variant<"), nicht die Erwaehnung im Fliesstext.
        // Die Doku-Zeilen dieser Stufe sprechen oft ueber variant, ohne einen zu bilden -- wer auf
        // das nackte Wort prueft, baut eine Wache, die an ihren eigenen Kommentaren erstickt.
        if (inhalt.find("std::variant<") != std::string::npos) {
            traeger.push_back(eintrag.path().filename().string());
        }
    }

    // Nenner mitdrucken: eine Wache ueber 0 Dateien waere gruen und wertlos.
    EXPECT_GE(geprueft, std::size_t{5}) << "zu wenige hybrid/-Quelldateien gescannt -- Wache ohne Gegenstand";
    ASSERT_EQ(traeger.size(), std::size_t{1}) << "geprueft: " << geprueft << " Dateien";
    EXPECT_EQ(traeger.front(), std::string{"hybrid_dock_array.hpp"})
        << "std::variant< darf im hybrid/-Baum NUR im DockSlot des Dock-Arrays stehen "
           "(Owner-E1 + Paragraf-49-KORREKTUR). Gefunden in: " << traeger.front();
}

// ============================================================================================
// (F) KON45-01(6): DIE KEY-GRAMMATIK -- alle Literale dieser Stufe sind stempel-faehig
// ============================================================================================

TEST(HyA1DockContract, AlleLiteraleDieserStufeHaltenDenFingerprintZeichenvorrat) {
    // DIE AUFLAGE: der Fingerprint-Zeichenvorrat (anatomy_fingerprint.hpp,
    // anatomy_glied_zeichen_erlaubt) laesst alnum und '= @ ; . , + - _ : / [ ] { }' zu --
    // KEIN '<', KEIN '>', KEIN Whitespace. Ein Literal, das dagegen verstoesst, reisst die
    // Format-Wache in dem Moment, in dem es je in ein Preimage-Glied geraet.
    //
    // WARUM DAS HIER GEPRUEFT WIRD, OBWOHL HEUTE KEIN LITERAL DIESER STUFE IN EINEN STEMPEL GEHT:
    // weil es billig ist und weil der Weg dorthin kurz ist. Die Dock-Bestueckung gehoert per
    // Design ins Sidecar-Manifest -- und das Sidecar-Format wird NACH der A13-Stempel-Regression
    // fixiert. Wer dort spaeter Vertrags-Tokens oder Dock-Namen einsetzt, soll nicht erst am
    // fertigen Manifest merken, dass sie nicht hineinpassen.
    //
    // BESTANDS-BEFUND, ausdruecklich NICHT hier behoben: die Strategie-Namen der
    // Klassifikations-Schicht ("Reroute<SearchAlgorithm>", "Reroute<View>", ...) tragen '<' und
    // '>' und sind damit NICHT stempel-faehig. Das ist heute folgenlos -- sie sind reine
    // Anzeige-Namen und gehen in kein Glied -- und es bleibt es, solange die Adressierung nicht
    // literal-basiert wird. Die Umstellung auf eine adressbasierte Form gehoert zu HY-A2 und wird
    // hier bewusst NICHT vorgezogen: sie beruehrt die Stempel-Flaeche und damit ein anderes
    // Buendel. Diese Wache deckt deshalb genau die Literale DIESER Stufe -- Scope benannt.
    auto zeichen_ok = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '=' || c == '@' ||
               c == ';' || c == '.' || c == ',' || c == '+' || c == '-' || c == '_' || c == ':' || c == '/' ||
               c == '[' || c == ']' || c == '{' || c == '}';
    };
    auto pruefe = [&](std::string_view was, std::string_view wert) {
        EXPECT_FALSE(wert.empty()) << was;
        for (char const c : wert) {
            EXPECT_TRUE(zeichen_ok(c)) << was << ": Zeichen '" << c << "' ist nicht stempel-faehig (Wert: " << wert
                                       << ")";
        }
    };

    std::size_t geprueft = 0;
    for (auto const& info : hy::kHybridDockContractRegistry) {
        pruefe("Vertrags-Token", info.id);
        pruefe("Vertrags-Name", info.name);
        geprueft += 2;
    }
    pruefe("Dock-Name", hy::StandardHybridDock::dock_name());
    ++geprueft;
    for (int const s : hy::kAlleHybridStatus) {
        pruefe("Status-Name", hy::hybrid_status_name(s));
        ++geprueft;
    }
    // Nenner mitdrucken -- eine Wache ueber 0 Literale waere gruen und wertlos.
    EXPECT_EQ(geprueft, 2 * hy::kHybridDockContractCount + 1 + hy::kAlleHybridStatus.size());
    EXPECT_GE(geprueft, std::size_t{15});

    // GEGENPROBE: das Praedikat ist nicht auf immer-wahr degeneriert -- es faengt genau die
    // Zeichen, an denen die Bestands-Strategie-Namen scheitern wuerden.
    EXPECT_FALSE(zeichen_ok('<'));
    EXPECT_FALSE(zeichen_ok('>'));
    EXPECT_FALSE(zeichen_ok(' '));
    EXPECT_TRUE(zeichen_ok('_'));
}
