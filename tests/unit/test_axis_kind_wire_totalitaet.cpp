// test_axis_kind_wire_totalitaet.cpp -- Warnungs-Review Runde 2b (clang ueber den ce-Test-Bau), 09.08.2026.
//
// DER BEFUND, WOERTLICH AUS DEM BAU-LOG:
//   /home/comdare/wt-ce-warn-tests/libs/cache_engine/profile_facade/planner/experiment_dock_payload.hpp:77:13:
//   warning: enumeration values 'system_meta_meta', 'measurement_meta_meta', and 'organ_meta_meta'
//            not handled in switch [-Wswitch]
//
// WARUM DAS EIN DEFEKT WAR UND KEIN RAUSCHEN: der Fall-through hinter dem switch lieferte "organ".
// Eine Meta-Meta-Achse waere damit als ORGAN-Achse auf den Draht gegangen -- eine echte, plausible,
// falsche Antwort. Und weil parse_axis_kind("organ") sauber AxisKind::organ zurueckgibt, haette der
// bestehende BYTE-Roundtrip die Fehlfaerbung BESTAETIGT: emit -> parse -> emit ist byte-identisch,
// nur eben um die Faerbung aermer. Es gab nichts, was klappern konnte.
//
// WARUM ES DER BESTEHENDE TEST NICHT FAND -- zwei Gruende, beide gemessen:
//   (1) test_experiment_dock_payload.cpp deckt laut eigenem Kommentar "alle drei AxisKind-Faerbungen"
//       ab. Es sind seit A13-M2 SECHS. Der Satz war der Nenner, und er war veraltet.
//   (2) Jene TU ist ein plain-main-Harnisch (check_*/exit 0) und wird per nacktem add_executable
//       gebaut -- OHNE COMDARE_set_default_warnings. Sie haette die -Wswitch-Warnung also selbst
//       dann nicht gezeigt, wenn sie den Header beruehrt haette. Diese TU hier laeuft ueber
//       comdare_add_test und traegt die Hauswarnstufe.
//
// GEGENSTAND ist die WIRE-TOTALITAET des Faerbungs-Kanals, nicht "der Header uebersetzt":
// jeder Enumerator hat einen Token, die Token sind paarweise verschieden, und token->parse->token
// schliesst sich. Der Nenner steht in der Ausgabe.

#include <profile_facade/planner/experiment_dock_payload.hpp> // PRUEFLING: axis_kind_token / parse_axis_kind
#include <topics/axis.hpp>                                    // AxisKind -- die Enumeratoren, fremde Quelle (T-3)

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

namespace pl  = ::comdare::cache_engine::planner;
namespace cet = ::comdare::cache_engine::topics;

namespace {

// DIE GRUNDGESAMTHEIT. Sie steht hier EINZELN und WOERTLICH und nicht als Schleife ueber die
// Token-Tabelle des Prueflings -- eine Tabelle, die sich selbst abzaehlt, ist zu 100 Prozent
// vollstaendig und beweist nichts (T-5). Wer AxisKind erweitert, faellt in dieser Zeile auf.
constexpr std::array<cet::AxisKind, 6> kAlleAxisKinds{
    cet::AxisKind::organ,          cet::AxisKind::system_measurement,    cet::AxisKind::system_config,
    cet::AxisKind::system_meta_meta, cet::AxisKind::measurement_meta_meta, cet::AxisKind::organ_meta_meta,
};

} // namespace

TEST(AxisKindWire, NENNER_JederEnumeratorHatEinenNichtLeerenWireToken) {
    std::size_t mit_token = 0;
    for (auto const k : kAlleAxisKinds)
        if (!pl::detail::axis_kind_token(k).empty()) ++mit_token;

    std::cout << "[AXISKIND-NENNER] Enumeratoren mit Wire-Token: " << mit_token << "/" << kAlleAxisKinds.size()
              << " (Grundgesamtheit: die im Test einzeln genannten AxisKind-Enumeratoren aus topics/axis.hpp)\n";
    EXPECT_EQ(mit_token, kAlleAxisKinds.size()) << "ein Enumerator ohne Token faellt beim Emit auf den Fall-through";
}

TEST(AxisKindWire, DieWireTokenSindPAARWEISE_VERSCHIEDEN) {
    // Zwei Faerbungen unter demselben Token waeren im Payload ununterscheidbar -- genau der Zustand
    // VOR dieser Heilung, nur dass dort drei Faerbungen auf "organ" fielen.
    std::size_t kollisionen = 0;
    for (std::size_t a = 0; a < kAlleAxisKinds.size(); ++a)
        for (std::size_t b = a + 1; b < kAlleAxisKinds.size(); ++b)
            if (pl::detail::axis_kind_token(kAlleAxisKinds[a]) == pl::detail::axis_kind_token(kAlleAxisKinds[b])) {
                ++kollisionen;
                ADD_FAILURE() << "AxisKind " << static_cast<unsigned>(kAlleAxisKinds[a]) << " und "
                              << static_cast<unsigned>(kAlleAxisKinds[b]) << " teilen den Token '"
                              << pl::detail::axis_kind_token(kAlleAxisKinds[a]) << "'";
            }
    std::cout << "[AXISKIND-NENNER] geprueft: " << kAlleAxisKinds.size() * (kAlleAxisKinds.size() - 1) / 2
              << " Paare -- " << kollisionen << " Kollisionen\n";
    EXPECT_EQ(kollisionen, 0u);
}

TEST(AxisKindWire, TokenUndParserSindSYMMETRISCH) {
    // Der Schreiber darf nichts emittieren, was der Leser nicht kennt. Ohne diese Richtung waere ein
    // neuer Token eine Einbahnstrasse -- mit gruenem Byte-Roundtrip, weil beide Seiten denselben
    // Fall-through nehmen.
    std::size_t geschlossen = 0;
    for (auto const k : kAlleAxisKinds) {
        cet::AxisKind zurueck{};
        auto const    tok = pl::detail::axis_kind_token(k);
        if (pl::detail::parse_axis_kind(tok, zurueck) && zurueck == k) ++geschlossen;
        else ADD_FAILURE() << "Token '" << tok << "' liest sich nicht auf AxisKind "
                           << static_cast<unsigned>(k) << " zurueck";
    }
    std::cout << "[AXISKIND-NENNER] token->parse->token geschlossen: " << geschlossen << "/" << kAlleAxisKinds.size()
              << "\n";
    EXPECT_EQ(geschlossen, kAlleAxisKinds.size());
}

TEST(AxisKindWireNegativ, EinNichtEnumeratorHatKEINEN_TokenUndKeinErsatzName) {
    // GEGENEINGANG (T-4): der Eingang, bei dem die Zusicherung FAELLT. Ohne ihn belegte die Quote
    // oben nur, dass die Schleife laeuft. 200 ist kein deklarierter Diskriminator.
    auto const fremd = static_cast<cet::AxisKind>(static_cast<std::uint8_t>(200));
    EXPECT_TRUE(pl::detail::axis_kind_token(fremd).empty())
        << "ein Ersatzname waere ein SCHREIBBARER Token -- genau der stille Stellvertreter, der den Befund ausmachte";

    cet::AxisKind out{};
    EXPECT_FALSE(pl::detail::parse_axis_kind("", out)) << "und der Leser muss den leeren Token ablehnen";
    EXPECT_FALSE(pl::detail::parse_axis_kind("organ_meta_metaX", out)) << "unbekannter Token => harter Parse-Fehler";
    EXPECT_FALSE(pl::detail::parse_axis_kind("ORGAN", out)) << "die Token sind zeichengenau, nicht case-insensitiv";
}

TEST(AxisKindWire, EineMetaMetaFaerbungUEBERLEBT_DenByteRoundtrip) {
    // Das ist die Szene aus dem Befund, ausgeschrieben: vor der Heilung ging jede dieser drei
    // Achsen als kind="organ" auf den Draht und kam als AxisKind::organ zurueck -- byte-stabil und
    // falsch. Hier faellt der Test, wenn die Faerbung unterwegs verloren geht.
    pl::ExperimentSubtreePayload p;
    p.axes.push_back({cet::AxisKind::system_meta_meta, "simd", pl::AxisRangeForm::enumerated, {"avx2", "avx512"}, 0, 0});
    p.axes.push_back({cet::AxisKind::measurement_meta_meta, "load_framework", pl::AxisRangeForm::index_range, {}, 0, 2});
    p.axes.push_back({cet::AxisKind::organ_meta_meta, "organ_hub", pl::AxisRangeForm::enumerated, {"a"}, 0, 0});

    std::string const s1 = pl::emit_experiment_subtree_xml(p);
    auto const        p2 = pl::parse_experiment_subtree_xml(s1);
    ASSERT_TRUE(p2.has_value()) << "die Nutzlast muss ueberhaupt lesbar sein:\n" << s1;
    EXPECT_EQ(*p2, p) << "Semantik-Roundtrip -- die Faerbung ist Teil der Gleichheit";
    EXPECT_EQ(pl::emit_experiment_subtree_xml(*p2), s1) << "Byte-Roundtrip";

    // Und der Gegenbeweis zum alten Zustand: keine der drei Zeilen darf kind="organ" tragen.
    EXPECT_EQ(s1.find("kind=\"organ\""), std::string::npos)
        << "eine Meta-Meta, die als \"organ\" auf dem Draht steht, ist die Fehlfaerbung von Runde 2b:\n"
        << s1;
    EXPECT_NE(s1.find("kind=\"system_meta_meta\""), std::string::npos);
    EXPECT_NE(s1.find("kind=\"measurement_meta_meta\""), std::string::npos);
    EXPECT_NE(s1.find("kind=\"organ_meta_meta\""), std::string::npos);
}
