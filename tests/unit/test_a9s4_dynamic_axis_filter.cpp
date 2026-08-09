// tests/unit/test_a9s4_dynamic_axis_filter.cpp -- A9-S4: die CoR-Filterkette fuer dynamische
// Unter-Achsen-Variablen (tools/mess_report/dynamic_axis_filter.hpp), direkt gegen die drei
// Handler gebissen (leer / genau ein Wert / mehr als ein Wert), nicht nur incidental ueber die
// Gesamt-Render-Pipeline.

#include "../../tools/mess_report/dynamic_axis_filter.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace mr = ::comdare::cache_engine::tools::mess_report;

TEST(A9S4DynamikKette, AlleZellenLeer_LiefertKonstantMitDash) {
    mr::DynamikKette const kette;
    auto const             v = kette.bewerte("platform", {"", "", ""});
    EXPECT_EQ(v.entscheidung, mr::DynamikVerdikt::Entscheidung::Konstant);
    EXPECT_EQ(v.wert, "-");
}

TEST(A9S4DynamikKette, GenauEinDistinkterWert_LiefertKonstantMitDemWert) {
    mr::DynamikKette const kette;
    auto const             v = kette.bewerte("platform", {"linux-x86_64", "linux-x86_64", "linux-x86_64"});
    EXPECT_EQ(v.entscheidung, mr::DynamikVerdikt::Entscheidung::Konstant);
    EXPECT_EQ(v.wert, "linux-x86_64");
}

TEST(A9S4DynamikKette, MehrDistinkteWerte_LiefertDynamischMitSweep) {
    mr::DynamikKette const kette;
    auto const             v = kette.bewerte("workload", {"ycsb_a", "ycsb_b", "ycsb_a"});
    EXPECT_EQ(v.entscheidung, mr::DynamikVerdikt::Entscheidung::Dynamisch);
    EXPECT_EQ(v.wert, "sweep");
}

// -- Bissprobe je Handler direkt (nicht nur ueber DynamikKette::bewerte) --

TEST(A9S4DynamikKette, FehlendWertHandlerReagiertNurAufDurchgaengigLeer) {
    mr::FehlendWertHandler h;
    h.set_next(nullptr);
    EXPECT_EQ(h.dispatch("x", {"", ""}).entscheidung, mr::DynamikVerdikt::Entscheidung::Konstant);
    // EIN nicht-leerer Wert -> dieser Handler ist NICHT zustaendig, gibt ohne next() also
    // WeiterInDerKette zurueck (keine Ketten-Verkettung hier -- direkter Handler-Aufruf).
    EXPECT_EQ(h.dispatch("x", {"a", ""}).entscheidung, mr::DynamikVerdikt::Entscheidung::WeiterInDerKette);
}

TEST(A9S4DynamikKette, EinWertHandlerReagiertNurBeiGenauEinemDistinktenWert) {
    mr::EinWertHandler h;
    EXPECT_EQ(h.dispatch("x", {"a", "a", "a"}).entscheidung, mr::DynamikVerdikt::Entscheidung::Konstant);
    EXPECT_EQ(h.dispatch("x", {"a", "b"}).entscheidung, mr::DynamikVerdikt::Entscheidung::WeiterInDerKette);
}

TEST(A9S4DynamikKette, MehrWerteHandlerIstDasKettenendeUndEntscheidetImmer) {
    mr::MehrWerteHandler h;
    // Kettenende: entscheidet IMMER (nie WeiterInDerKette), auch bei einer leeren Eingabe -- die
    // vorigen zwei Handler haben diesen Fall in der realen Kette bereits abgefangen; dieser Test
    // belegt nur, dass der letzte Handler NIE unentschieden bleibt (kein next_ == nullptr-Crash).
    EXPECT_EQ(h.dispatch("x", {}).entscheidung, mr::DynamikVerdikt::Entscheidung::Dynamisch);
}

TEST(A9S4DynamikKette, ReihenfolgeIstBindend_ErsterTreffenderHandlerGewinnt) {
    // Ein Wert, der sowohl "leer waere sie durchgaengig" als auch "genau ein Wert" NICHT ist,
    // muss beim MehrWerteHandler ankommen -- Reihenfolge leer -> ein Wert -> mehrere Werte.
    mr::DynamikKette const kette;
    auto const gemischt = kette.bewerte("setting", {"", "a", "b"}); // eine leere Zelle + zwei distinkte Werte
    EXPECT_EQ(gemischt.entscheidung, mr::DynamikVerdikt::Entscheidung::Dynamisch)
        << "leere Zellen werden bei der Distinktheits-Zaehlung ignoriert (nur echte Werte zaehlen)";
}
