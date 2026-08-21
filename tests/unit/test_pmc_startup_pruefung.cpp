// test_pmc_startup_pruefung -- #83 (C-1(c), Owner-GO KON103 17.08.2026): der CEB-GEGENEINGANG am Lauf-Host.
//
// GEGENSTAND: builder/pmc_startup_pruefung.hpp -- die Drei-Wege-Unterscheidung aus KON106-04 ("Fehler bei
// fehlender Quelle / WARNUNG bei vorhandener-aber-ungenutzter Quelle / stiller Normalfall"). Der
// Fehler-Weg wohnt in der Emission (test_experiment_plan_director.cpp, PmcFailLoud); HIER stehen die
// beiden Warn-Richtungen und die beiden stillen Lagen -- je Richtung mit KOEDER (eine Fake-Strategie, die
// die auf dieser Maschine nicht herstellbaren Lagen erzeugt; Muster pmc_host_probe.hpp-Tests).
//
// FREMDER NENNER (T-3): die Event-Zahl kommt aus measurement::kPmcEventCount (der EINEN Liste), nie als
// hier erfundene Zahl -- waechst die Liste, waechst dieser Test mit, statt still zu luegen.

#include <gtest/gtest.h>

#include <string>

#include "pmc_startup_pruefung.hpp"

#include <cache_engine/measurement/pmc_event_set.hpp>

namespace bld = comdare::cache_engine::builder;
namespace cme = comdare::cache_engine::measurement;

namespace {

/// KOEDER-Strategie 1: JEDES Event beisst (der Host "kann PMC" -- auch wenn diese Maschine es nicht kann).
struct BeisstImmer {
    [[nodiscard]] static bool event_beisst(cme::PmcEventSpec const&) noexcept { return true; }
};

/// KOEDER-Strategie 2: KEIN Event beisst (der Host "kann kein PMC").
struct BeisstNie {
    [[nodiscard]] static bool event_beisst(cme::PmcEventSpec const&) noexcept { return false; }
};

} // namespace

// Richtung 1 (Owner-Warnfall, verbatim "Warnung bei (c) wenn PMC vorhanden, aber nicht verwendet"):
// Quelle NICHT einkompiliert, aber der Host-Koeder beisst => WARNUNG mit Nenner.
TEST(PmcStartupPruefung, VorhandenAberNichtVerwendetWarnt) {
    auto const b = bld::pmc_startup_pruefung<BeisstImmer>(/*quelle_einkompiliert=*/false,
                                                          /*quelle_available=*/false);
    EXPECT_EQ(b.lage, bld::PmcStartupLage::VorhandenNichtVerwendet);
    EXPECT_TRUE(b.warnung());
    // Der NENNER ist die EINE Event-Liste -- beide Mengen muessen genannt sein (V-1).
    EXPECT_EQ(b.events_geprueft, cme::kPmcEventCount);
    EXPECT_EQ(b.events_gebissen, cme::kPmcEventCount) << "BeisstImmer: jedes Event der Liste beisst";
    std::string const zeile = bld::pmc_startup_warnzeile(b);
    EXPECT_NE(zeile.find("[PMC-WARN]"), std::string::npos) << "die Warnung traegt den grep-stabilen Marker";
    EXPECT_NE(zeile.find("pmc_startup_vorhanden_nicht_verwendet"), std::string::npos);
    EXPECT_NE(zeile.find("PMC VORHANDEN, ABER NICHT VERWENDET"), std::string::npos)
        << "die Owner-Aussage steht woertlich in der Zeile";
    std::string const nenner = std::to_string(cme::kPmcEventCount) + "/" + std::to_string(cme::kPmcEventCount);
    EXPECT_NE(zeile.find("host_biss=" + nenner), std::string::npos)
        << "beide Mengen (gebissen/geprueft) stehen in der Warnzeile -- eine Warnung ohne Nenner waere "
           "eine Behauptung";
}

// Richtung 2: Quelle einkompiliert, aber am Lauf-Host ohne Zugriff => Teil-Lauf-WARNUNG (die Zeilen der
// CSV tragen Tokens; die Warnung nennt den LAUF).
TEST(PmcStartupPruefung, EinkompiliertOhneZugriffWarntAlsTeilLauf) {
    // Der Host wird in diesem Zweig NICHT gefragt (available() der realen Quelle IST die Host-Aussage) --
    // die BeisstNie-Strategie beweist genau das: waere sie gefragt, kippte die Lage auf OhnePmc.
    auto const b = bld::pmc_startup_pruefung<BeisstNie>(/*quelle_einkompiliert=*/true,
                                                        /*quelle_available=*/false);
    EXPECT_EQ(b.lage, bld::PmcStartupLage::TeilLaufQuelleOhneZugriff);
    EXPECT_TRUE(b.warnung());
    EXPECT_EQ(b.events_geprueft, 0u) << "einkompilierter Fall: kein zweiter Koeder (s. Header-Begruendung)";
    std::string const zeile = bld::pmc_startup_warnzeile(b);
    EXPECT_NE(zeile.find("[PMC-WARN]"), std::string::npos);
    EXPECT_NE(zeile.find("pmc_startup_teil_lauf_quelle_ohne_zugriff"), std::string::npos);
    EXPECT_NE(zeile.find("TEIL-LAUF"), std::string::npos);
}

// Die beiden STILLEN Lagen: einkompiliert+live und nicht-einkompiliert auf einem Host ohne PMC.
// Beide liefern KEINE Warnung und eine LEERE Warnzeile (es gibt nichts zu sagen; KON106-04 "stiller
// Normalfall" -- ein Dauer-Rauschen wuerde die echten Warnungen entwerten).
TEST(PmcStartupPruefung, DieBeidenNormalfaelleBleibenStill) {
    auto const mit = bld::pmc_startup_pruefung<BeisstNie>(/*quelle_einkompiliert=*/true,
                                                          /*quelle_available=*/true);
    EXPECT_EQ(mit.lage, bld::PmcStartupLage::NormalfallMitQuelle);
    EXPECT_FALSE(mit.warnung());
    EXPECT_TRUE(bld::pmc_startup_warnzeile(mit).empty());

    auto const ohne = bld::pmc_startup_pruefung<BeisstNie>(/*quelle_einkompiliert=*/false,
                                                           /*quelle_available=*/false);
    EXPECT_EQ(ohne.lage, bld::PmcStartupLage::NormalfallOhnePmc);
    EXPECT_FALSE(ohne.warnung());
    EXPECT_TRUE(bld::pmc_startup_warnzeile(ohne).empty());
    EXPECT_EQ(ohne.events_geprueft, cme::kPmcEventCount) << "gefragt wurde wohl -- nur gebissen hat nichts";
    EXPECT_EQ(ohne.events_gebissen, 0u);
}

// GEGENEINGANG am ECHTEN Host (reale Strategie): die Lage ist host-abhaengig (prod1 beisst, ein
// Container ohne perf nicht) -- der Test pinnt deshalb KEINE Lage, sondern die INVARIANTE: im
// nicht-einkompilierten Fall ist das Ergebnis IMMER eine der zwei dafuer vorgesehenen Lagen, und die
// Warnzeile ist genau dann nicht-leer, wenn die Lage eine Warnung ist.
TEST(PmcStartupPruefung, RealeStrategieLiefertEineDerBeidenNichtEinkompiliertenLagen) {
    auto const b = bld::pmc_startup_pruefung<>(/*quelle_einkompiliert=*/false, /*quelle_available=*/false);
    bool const ist_erwartete_lage =
        b.lage == bld::PmcStartupLage::VorhandenNichtVerwendet || b.lage == bld::PmcStartupLage::NormalfallOhnePmc;
    EXPECT_TRUE(ist_erwartete_lage) << "lage=" << std::string(bld::pmc_startup_label(b.lage));
    EXPECT_EQ(b.events_geprueft, cme::kPmcEventCount);
    EXPECT_EQ(bld::pmc_startup_warnzeile(b).empty(), !b.warnung());
}
