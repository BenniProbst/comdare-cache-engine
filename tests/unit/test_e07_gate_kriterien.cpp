// test_e07_gate_kriterien.cpp -- W2 R-12 (E-7): das E-07-Gate hat jetzt AUSFUEHRBARE Kriterien.
//
// DER GEGENSTAND: profile_facade/e07_gate_kriterien.hpp -- der Pruefer der Dossier-Kriterien
// K1-K5 + N1-N3 (super docs/sessions/20260803-DOSSIER-e07-gate-definition-b5-zweistufig.md
// Abschn. 1). Befund r3 C4: die E07_-Gate-Literale hatten 0 Treffer im Code; das Gate ist
// Position B10 der Vor-Trigger-Checkliste und trigger-blockierend.
//
// KOEDER-DOKTRIN BEIDSEITIG (T-1):
//   Seite GRUEN: EIN arithmetisch konsistentes Protokoll MUSS GO ergeben (ein unerfuellbares
//   Gate waere ein Schein-Gate: es "sichert", indem es alles blockiert, und wird dann umgangen).
//   Seite ROT: jede einzelne Fail-Closed-Klasse MUSS NO-GO ergeben -- fehlendes Literal,
//   unbekannter Schluessel, Konflikt, unlesbarer Wert, Wertebereich, Kriteriums-Verstoss,
//   Delta-0-Probe (Anti-Schein-Gruen), leeres Protokoll.
//
// NENNER FREMD (T-4): das GRUEN-Protokoll wird im Test aus einem Mini-Szenario ARITHMETISCH
// hergeleitet (n = 2*3 = 6 Permutationen; eine Organ-Achse verliert 1 von m=3 Werten =>
// n' = n*(m-1)/m = 4), nicht aus dem Pruefling zurueckgelesen. Die Nenner 10 (Kriterien) und
// 23 (Pflicht-Literale) werden gegen die Dossier-Tabelle gezaehlt erwartet, nicht aus den
// Header-Konstanten uebernommen.

#include <profile_facade/e07_gate_kriterien.hpp>

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace e07 = ::comdare::cache_engine::e07;

namespace {

// Das arithmetisch konsistente Mini-Szenario (T-4: alle Zahlen HIER hergeleitet):
//   Organ-Achsen a=2 Werte, b=3 Werte  => n = 2*3 = 6 (K1: tree == plan == built == 6).
//   K3a: b verliert 1 Wert (m=3 -> 2)  => n' = 6*(3-1)/3 = 4; 0 Treffer des entfernten Werts.
//   K3b: opt*simd-Zellen 4 -> 3 (eine Zelle entfernt), binary_id-Menge unveraendert.
//   K3c: Mess-Paesse 2 -> 1, binary_id-Menge unveraendert.
//   K4:  Fenster = volles Mini-Fenster count=6 => Cursor-Maximum 5, genau EIN done.
std::string gruen_protokoll() {
    return "E07_TREE_COUNT=6\n"
           "E07_PLAN_PERMS=6\n"
           "E07_BUILT_COUNT=6\n"
           "E07_ID_ROUNDTRIP_MISMATCH=0\n"
           "E07_ORGAN_DELTA_EXPECTED=4\n"
           "E07_ORGAN_DELTA_ACTUAL=4\n"
           "E07_REMOVED_VALUE_HITS=0\n"
           "E07_SYS_CELLS_BEFORE=4\n"
           "E07_SYS_CELLS_AFTER=3\n"
           "E07_BINARY_ID_SET_CHANGED=0\n"
           "E07_MEASURE_PASSES_BEFORE=2\n"
           "E07_MEASURE_PASSES_AFTER=1\n"
           "E07_WINDOW_COUNT=6\n"
           "E07_WINDOW_FIRST_ID_MATCH=1\n"
           "E07_WINDOW_LAST_ID_MATCH=1\n"
           "E07_PROGRESS_CURSOR_MAX=5\n"
           "E07_PROGRESS_DONE_COUNT=1\n"
           "E07_PLAN_BYTE_EQUAL=1\n"
           "E07_RERUN_ID_SET_EQUAL=1\n"
           "E07_BYPASS_FINDINGS=0\n"
           "E07_STALE_ID_MEASURED=0\n"
           "E07_WINDOW_SUBSET=1\n"
           "E07_WINDOW_NEW_IDS=0\n";
}

/// Ersetzt in `proto` genau die Zeile "key=..." durch `ersatz` ("" = Zeile entfernen).
std::string ersetze(std::string const& proto, std::string_view key, std::string_view ersatz) {
    std::string out;
    std::size_t pos = 0;
    while (pos < proto.size()) {
        std::size_t const nl    = proto.find('\n', pos);
        std::string const zeile = proto.substr(pos, nl - pos);
        pos                     = nl + 1;
        if (zeile.rfind(std::string(key) + "=", 0) == 0) {
            if (!ersatz.empty()) out += std::string(ersatz) + "\n";
        } else {
            out += zeile + "\n";
        }
    }
    return out;
}

/// Kurzform auf dem GRUEN-Protokoll (der haeufige Fall: genau EIN Koeder je Probe).
std::string mit_zeile(std::string_view key, std::string_view ersatz) { return ersetze(gruen_protokoll(), key, ersatz); }

} // namespace

// ------------------------------------------------------------------------------------------------
// Seite GRUEN: das konsistente Protokoll ergibt GO -- das Gate ist erfuellbar, kein Schein-Gate.
// ------------------------------------------------------------------------------------------------
TEST(E07GateKriterien, GruenesProtokollErgibtGoMitVollenNennern) {
    auto const erg = e07::e07_gate_auswerten(gruen_protokoll());
    for (auto const& b : erg.befunde) ADD_FAILURE() << "unerwarteter Befund: " << b;
    EXPECT_EQ(erg.kriterien_erfuellt, 10) << "Dossier-Zaehlung: K1,K2,K3a,K3b,K3c,K4,K5,N1,N2,N3";
    EXPECT_EQ(erg.literale_vorhanden, 23) << "Dossier-Literale + E07_WINDOW_COUNT";
    EXPECT_TRUE(erg.go);
    // NENNER-DOKTRIN: beide Nenner stehen LITERAL in der Ausgabe.
    auto const p = erg.protokoll();
    EXPECT_NE(p.find("E07_GATE_KRITERIEN_ERFUELLT=10/10"), std::string::npos) << p;
    EXPECT_NE(p.find("E07_GATE_PFLICHT_LITERALE=23/23"), std::string::npos) << p;
    EXPECT_NE(p.find("E07_GATE=GO"), std::string::npos) << p;
}

TEST(E07GateKriterien, TraegerLogUmDasProtokollAendertNichts) {
    std::string const eingebettet =
        "[ 12:00:01 ] harness start\nirgendein Lauf-Log\n" + gruen_protokoll() + "harness ende, exit 0\n";
    EXPECT_TRUE(e07::e07_gate_auswerten(eingebettet).go);
}

// ------------------------------------------------------------------------------------------------
// Seite ROT -- fail-closed: LEER und FEHLEND.
// ------------------------------------------------------------------------------------------------
TEST(E07GateKriterien, LeeresProtokollIstNoGoMitSichtbarenNennern) {
    auto const erg = e07::e07_gate_auswerten("");
    EXPECT_FALSE(erg.go);
    EXPECT_EQ(erg.literale_vorhanden, 0);
    EXPECT_EQ(erg.kriterien_erfuellt, 0);
    auto const p = erg.protokoll();
    EXPECT_NE(p.find("E07_GATE_KRITERIEN_ERFUELLT=0/10"), std::string::npos) << p;
    EXPECT_NE(p.find("E07_GATE_PFLICHT_LITERALE=0/23"), std::string::npos) << p;
    EXPECT_NE(p.find("E07_GATE=NO-GO"), std::string::npos) << p;
}

TEST(E07GateKriterien, JedesEinzelneFehlendeLiteralErzwingtNoGo) {
    // Fail-closed VOLLSTAENDIG: jede der 23 Zeilen einzeln entfernt => NO-GO + FEHLT-Befund.
    for (auto const key : e07::kPflichtLiterale) {
        auto const erg = e07::e07_gate_auswerten(mit_zeile(key, ""));
        EXPECT_FALSE(erg.go) << "fehlendes " << key << " ergab GO";
        EXPECT_EQ(erg.literale_vorhanden, 22) << key;
        bool fehlt_befund = false;
        for (auto const& b : erg.befunde) fehlt_befund = fehlt_befund || b.find(std::string(key)) != std::string::npos;
        EXPECT_TRUE(fehlt_befund) << "kein FEHLT-Befund fuer " << key;
    }
}

// ------------------------------------------------------------------------------------------------
// Seite ROT -- Kriteriums-Koeder (je Kriterium mindestens ein beissender Verstoss).
// ------------------------------------------------------------------------------------------------
TEST(E07GateKriterien, K1KardinalitaetsBruchBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_BUILT_COUNT", "E07_BUILT_COUNT=5"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[0]) << "K1 muss fallen (6==6==5 ist falsch)";
}

TEST(E07GateKriterien, K1NullFensterIstScheinGruenUndBeisst) {
    // Alle drei Zaehler 0: "nichts gebaut == nichts geplant" darf NIE als Steuerungs-Beweis gelten.
    // (Die K3a-Delta-Probe faellt dann arithmetisch mit -- hier zaehlt allein, dass K1 fallen MUSS.)
    auto proto     = ersetze(gruen_protokoll(), "E07_TREE_COUNT", "E07_TREE_COUNT=0");
    proto          = ersetze(proto, "E07_PLAN_PERMS", "E07_PLAN_PERMS=0");
    proto          = ersetze(proto, "E07_BUILT_COUNT", "E07_BUILT_COUNT=0");
    auto const erg = e07::e07_gate_auswerten(proto);
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[0]) << "K1 mit 0==0==0 waere Schein-Gruen";
}

TEST(E07GateKriterien, K2RoundtripMismatchBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_ID_ROUNDTRIP_MISMATCH", "E07_ID_ROUNDTRIP_MISMATCH=3"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[1]);
}

TEST(E07GateKriterien, K3aStaleTrefferBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_REMOVED_VALUE_HITS", "E07_REMOVED_VALUE_HITS=2"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[2]);
}

TEST(E07GateKriterien, AntiScheinGruenDeltaNullProbeIstUnguelig) {
    // Der Anti-Schein-Gruen-Koeder aus dem Dossier: erwartetes Delta 0 (n' == n == 6). ALLE
    // Einzelwerte sehen "gruen" aus (expected==actual, 0 Treffer) -- die Probe ist trotzdem
    // UNGUELTIG, weil sie nichts unterscheidet.
    auto proto     = ersetze(gruen_protokoll(), "E07_ORGAN_DELTA_EXPECTED", "E07_ORGAN_DELTA_EXPECTED=6");
    proto          = ersetze(proto, "E07_ORGAN_DELTA_ACTUAL", "E07_ORGAN_DELTA_ACTUAL=6");
    auto const erg = e07::e07_gate_auswerten(proto);
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[2]) << "K3a muss als UNGUELTIG fallen";
    bool unguelig = false;
    for (auto const& b : erg.befunde) unguelig = unguelig || b.find("UNGUELTIG") != std::string::npos;
    EXPECT_TRUE(unguelig) << "der Delta-0-Befund muss WOERTLICH ausgewiesen sein";
}

TEST(E07GateKriterien, K3bOhneSchrumpfDeltaIstUngueltig) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_SYS_CELLS_AFTER", "E07_SYS_CELLS_AFTER=4"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[3]) << "K3b: 4 -> 4 ist kein Delta";
}

TEST(E07GateKriterien, K3cBinaryIdSetBruchBeisstBeideProben) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_BINARY_ID_SET_CHANGED", "E07_BINARY_ID_SET_CHANGED=1"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[3]) << "K3b haengt am gemeinsamen Literal";
    EXPECT_FALSE(erg.kriterium_erfuellt[4]) << "K3c haengt am gemeinsamen Literal";
}

TEST(E07GateKriterien, K4CursorOffByOneBeisst) {
    // Cursor-Maximum muss count-1 sein (Fenster count=6 => 5). 6 waere ein Element ZUVIEL.
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_PROGRESS_CURSOR_MAX", "E07_PROGRESS_CURSOR_MAX=6"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[5]);
}

TEST(E07GateKriterien, K4ZweitesDoneBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_PROGRESS_DONE_COUNT", "E07_PROGRESS_DONE_COUNT=2"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[5]);
}

TEST(E07GateKriterien, K5PlanByteUngleichheitBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_PLAN_BYTE_EQUAL", "E07_PLAN_BYTE_EQUAL=0"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[6]);
}

TEST(E07GateKriterien, N1BypassFundBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_BYPASS_FINDINGS", "E07_BYPASS_FINDINGS=1"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[7]);
}

TEST(E07GateKriterien, N2StaleMessungBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_STALE_ID_MEASURED", "E07_STALE_ID_MEASURED=1"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[8]);
}

TEST(E07GateKriterien, N3NeueIdImFensterBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_WINDOW_NEW_IDS", "E07_WINDOW_NEW_IDS=1"));
    EXPECT_FALSE(erg.go);
    EXPECT_FALSE(erg.kriterium_erfuellt[9]);
}

// ------------------------------------------------------------------------------------------------
// Seite ROT -- Gegeneingaenge (Form-Verstoesse, fail-closed).
// ------------------------------------------------------------------------------------------------
TEST(E07GateKriterien, UnbekannterE07SchluesselBeisst) {
    auto const erg = e07::e07_gate_auswerten(gruen_protokoll() + "E07_ERFUNDENES_KRITERIUM=1\n");
    EXPECT_FALSE(erg.go) << "ein Protokoll mit erfundenem Literal ist kein Beleg";
    bool unbekannt = false;
    for (auto const& b : erg.befunde) unbekannt = unbekannt || b.find("UNBEKANNT") != std::string::npos;
    EXPECT_TRUE(unbekannt);
}

TEST(E07GateKriterien, KonfliktDoppelLiteralBeisstIdentischesDoppelNicht) {
    // Widerspruch (=1 dann =0) -> NO-GO; identische Wiederholung -> weiterhin GO.
    auto const konflikt = e07::e07_gate_auswerten(gruen_protokoll() + "E07_PLAN_BYTE_EQUAL=0\n");
    EXPECT_FALSE(konflikt.go);
    bool k = false;
    for (auto const& b : konflikt.befunde) k = k || b.find("KONFLIKT") != std::string::npos;
    EXPECT_TRUE(k);
    auto const doppel = e07::e07_gate_auswerten(gruen_protokoll() + "E07_PLAN_BYTE_EQUAL=1\n");
    EXPECT_TRUE(doppel.go) << "E07_BINARY_ID_SET_CHANGED deckt K3b+K3c -- identische Doppel sind legal";
}

TEST(E07GateKriterien, UnlesbareWerteBeissen) {
    for (std::string_view kaputt : {"E07_TREE_COUNT=abc", "E07_TREE_COUNT=", "E07_TREE_COUNT=-1", "E07_TREE_COUNT=0x10",
                                    "E07_TREE_COUNT= 1 2", "E07_TREE_COUNT"}) {
        auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_TREE_COUNT", kaputt));
        EXPECT_FALSE(erg.go) << "unlesbare Zeile ergab GO: " << kaputt;
    }
}

TEST(E07GateKriterien, WertebereichsVerstossBeiFlagBeisst) {
    auto const erg = e07::e07_gate_auswerten(mit_zeile("E07_WINDOW_SUBSET", "E07_WINDOW_SUBSET=2"));
    EXPECT_FALSE(erg.go) << "ein 0/1-Flag mit Wert 2 ist keine gueltige Aussage";
    bool wb = false;
    for (auto const& b : erg.befunde) wb = wb || b.find("WERTEBEREICH") != std::string::npos;
    EXPECT_TRUE(wb);
}

TEST(E07GateKriterien, VerdiktRueckeinspeisungIstKeinBeleg) {
    // Wird die AUSGABE des Gates als Mess-Protokoll wiedereingespeist, muss sie durchfallen
    // (E07_GATE=GO traegt '='; "GO" ist kein dezimal-uint64 -> UNLESBAR/UNBEKANNT, fail-closed).
    auto const erg = e07::e07_gate_auswerten(e07::e07_gate_auswerten(gruen_protokoll()).protokoll());
    EXPECT_FALSE(erg.go) << "ein Verdikt ist kein Mess-Protokoll (Echo-Gruen-Sperre)";
}
