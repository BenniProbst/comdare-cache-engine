// tests/unit/test_p2_ram_identity_verdict.cpp -- Task #7, Paket P2: die RAM-Drift-Gegenprobe.
//
// WAS HIER BEWIESEN WIRD: dass verify_declared_ram in JEDER Lage ein benanntes Urteil faellt statt
// eines stillen "passt schon" -- und insbesondere, dass es NICHT urteilt, wo es nicht urteilen darf
// (Auflage A3: ungleiche Groessen). Der teuerste denkbare Fehler dieses Headers ist nicht ein falsches
// Nein, sondern ein Ja, das ungeprueft zustande kam: es wuerde eine Datenblatt-Zahl als bestaetigten
// Ist-Takt in die Messreihe schreiben.
//
// FIXTURE-HERKUNFT (A9 -- ableiten, nicht behaupten): saemtliche Maschinen-Angaben werden aus
// kDeclaredMachines GEZOGEN, kein Schluessel und keine Zahl ist in dieser Datei getippt. Die
// Referenz-Zeile wird per Eigenschaft GESUCHT (die mit einer erhobenen Frequenz), nicht indiziert --
// ein fester Index waere schon die halbe Handkopie und wuerde bei einer Umsortierung stumm auf die
// falsche Maschine zeigen.

#include <cache_engine/measurement/ram_identity_verdict.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

namespace meas = comdare::cache_engine::measurement;

namespace {

[[nodiscard]] meas::DeclaredMachine const& referenz_maschine() {
    for (auto const& m : meas::kDeclaredMachines)
        if (m.ram_frequency_mhz != 0U) return m;
    ADD_FAILURE() << "Keine Maschine in kDeclaredMachines traegt eine RAM-Frequenz -- die Fixture-Basis "
                     "dieses Tests ist weggefallen.";
    return meas::kDeclaredMachines[0];
}

[[nodiscard]] meas::DeclaredMachine const& maschine_mit_nullmarke() {
    for (auto const& m : meas::kDeclaredMachines)
        if (m.ram_frequency_mhz == 0U) return m;
    ADD_FAILURE() << "Alle Maschinen tragen inzwischen eine RAM-Frequenz -- der 0-Marken-Fall gehoert "
                     "angepasst, nicht entfernt.";
    return meas::kDeclaredMachines[0];
}

/// Ein erhobenes Ergebnis der gegebenen Stufe mit der gegebenen Rate.
[[nodiscard]] meas::RamFrequencyReading erhoben(std::uint32_t mts, meas::RamFrequencyProvenance p) {
    meas::RamFrequencyReading r{};
    r.mts        = mts;
    r.provenance = p;
    r.state      = meas::RamReadingState::Erhoben;
    return r;
}

} // namespace

// =================================================================================================
// 1. DIE FAELLE, IN DENEN NICHT GEURTEILT WIRD
// =================================================================================================
TEST(P2RamIdentityVerdict, OhneErhebungWirdNichtGeurteilt) {
    auto const& m = referenz_maschine();
    EXPECT_EQ(meas::verify_declared_ram(&m, meas::RamFrequencyReading{}, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::NichtErhoben);
    // Und auch dann nicht, wenn eine Zahl im Ergebnis steht, der Zustand aber NichtErhoben ist --
    // eine Zahl ohne Erhoben-Zustand ist kein Wert (P1-Regel, hier im Urteil nachgezogen).
    meas::RamFrequencyReading zahl_ohne_zustand{};
    zahl_ohne_zustand.mts = m.ram_frequency_mhz;
    EXPECT_EQ(meas::verify_declared_ram(&m, zahl_ohne_zustand, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::NichtErhoben);
}

TEST(P2RamIdentityVerdict, OhneDeklarationWirdNichtGeurteilt) {
    auto const& m = referenz_maschine();
    auto const  r = erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_EQ(meas::verify_declared_ram(nullptr, r, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::KeineDeklaration);
}

TEST(P2RamIdentityVerdict, DieNullMarkeGeraetNIEInEinenVergleich) {
    // Ohne diese Regel bekaeme jede echte Erhebung auf einer Maschine mit 0-Marke eine "Abweichung"
    // gegen 0 MT/s gemeldet -- ein Fehler, der wie ein Hardware-Problem aussaehe.
    auto const& ohne = maschine_mit_nullmarke();
    auto const  r    = erhoben(4800, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_EQ(meas::verify_declared_ram(&ohne, r, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::KeineDeklaration);
    EXPECT_NE(meas::verify_declared_ram(&ohne, r, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::Abweichung);
}

TEST(P2RamIdentityVerdict, DieDeklarationBestaetigtSichNichtSelbst) {
    // Faellt die Kette auf Stufe 3 durch, KAM der Wert aus der Deklaration -- er ist ihr
    // trivialerweise gleich. Ein Match waere hier eine Selbstbestaetigung und der gefaehrlichste
    // Ausgang dieses Headers: er saehe im Log aus wie eine bestandene Gegenprobe.
    auto const& m = referenz_maschine();
    auto const  r = erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::DeclaredNotMeasured);
    EXPECT_EQ(meas::verify_declared_ram(&m, r, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::NurDeklariert);
    EXPECT_NE(meas::verify_declared_ram(&m, r, meas::DeclarationOrigin::Unbekannt), meas::RamIdentityVerdict::Match);
}

// =================================================================================================
// 2. AUFLAGE A3 -- die Provenienz-Gleichheitsregel
// =================================================================================================
TEST(P2RamIdentityVerdict, A3GleicheZahlenAberUngleicheGroessenErgebenKEINMatch) {
    // Das Kernpaar des Tests: EXAKT dieselben Zahlen, nur die Ursprungs-Notiz unterscheidet sich --
    // und das Urteil kippt. Ohne dieses Paar waere A3 eine Behauptung im Kommentar.
    auto const& m = referenz_maschine();
    auto const  r = erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase);

    EXPECT_EQ(meas::verify_declared_ram(&m, r, meas::DeclarationOrigin::Unbekannt), meas::RamIdentityVerdict::Match)
        << "Ohne Ursprungs-Notiz macht die Deklaration keine Groessen-Aussage -- dann darf verglichen werden.";
    EXPECT_EQ(meas::verify_declared_ram(&m, r, meas::DeclarationOrigin::EingefrorenesDmi),
              meas::RamIdentityVerdict::UnvergleichbareStufen)
        << "Eine JEDEC-Nennrate gegen einen eingefrorenen Ist-Takt vergleicht Datenblatt mit "
           "Betriebszustand -- gleiche Zahlen beweisen dort nichts.";
}

TEST(P2RamIdentityVerdict, A3GleicheGroessenDuerfenVerglichenWerden) {
    auto const& m = referenz_maschine();
    // Stufe 1 gegen einen eingefrorenen dmidecode-Ursprung: dieselbe Groesse, also ein echtes Urteil.
    EXPECT_EQ(meas::verify_declared_ram(&m,
                                        erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::ConfiguredMeasured),
                                        meas::DeclarationOrigin::EingefrorenesDmi),
              meas::RamIdentityVerdict::Match);
    EXPECT_EQ(meas::verify_declared_ram(
                  &m, erhoben(m.ram_frequency_mhz + 800U, meas::RamFrequencyProvenance::ConfiguredMeasured),
                  meas::DeclarationOrigin::EingefrorenesDmi),
              meas::RamIdentityVerdict::Abweichung);
    // Und die SPD-Seite gegen einen SPD-Ursprung ebenso.
    EXPECT_EQ(meas::verify_declared_ram(&m, erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase),
                                        meas::DeclarationOrigin::EingefrorenesSpd),
              meas::RamIdentityVerdict::Match);
}

TEST(P2RamIdentityVerdict, A3GiltInBEIDEnRichtungen) {
    // Die Umkehrung des Kernfalls: ein Ist-Takt gegen einen eingefrorenen SPD-Ursprung ist genauso
    // unvergleichbar. Ohne diese Zeile waere die Regel nur halb belegt.
    auto const& m = referenz_maschine();
    EXPECT_EQ(meas::verify_declared_ram(&m,
                                        erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::ConfiguredMeasured),
                                        meas::DeclarationOrigin::EingefrorenesSpd),
              meas::RamIdentityVerdict::UnvergleichbareStufen);
}

// =================================================================================================
// 3. ECHTE URTEILE
// =================================================================================================
TEST(P2RamIdentityVerdict, AbweichungWirdErkanntUndIstDerEINZIGEFehler) {
    auto const& m      = referenz_maschine();
    auto const  fremd  = erhoben(m.ram_frequency_mhz + 800U, meas::RamFrequencyProvenance::SpdJedecBase);
    auto const  urteil = meas::verify_declared_ram(&m, fremd, meas::DeclarationOrigin::Unbekannt);

    EXPECT_EQ(urteil, meas::RamIdentityVerdict::Abweichung);
    ASSERT_TRUE(meas::error_class_of_ram_verdict(urteil).has_value());
    EXPECT_EQ(*meas::error_class_of_ram_verdict(urteil), meas::CompilerCompilerErrorClass::HardwareErweiterungFehlt)
        << "Dokumentierte Dehnung (E-4): dieselbe Klasse, die verify_declared_cpu fuer die CPU-Seite "
           "nutzt. Eine eigene Klasse waere eine sechste in einem fremden Enum.";

    // Und die Gegenrichtung: KEIN anderes Verdikt ist ein Fehler.
    for (std::size_t i = 0; i < meas::kRamIdentityVerdictCount; ++i) {
        auto const v = static_cast<meas::RamIdentityVerdict>(i);
        if (v == meas::RamIdentityVerdict::Abweichung) continue;
        EXPECT_FALSE(meas::error_class_of_ram_verdict(v).has_value())
            << meas::ram_identity_verdict_label(v) << " ist ein definierter ZUSTAND, kein Fehler.";
    }
}

TEST(P2RamIdentityVerdict, EtikettWiderspruchIstEinBenannterNICHTFehler) {
    // Owner-Entscheid E-2: der Widerspruch wird ausgewiesen, hat aber keine Dispatch-Wirkung. Der
    // Fall ist auf prod2 seit dem Schluessel-Fix nicht mehr auszuloesen -- der ZUSTAND bleibt noetig,
    // denn er ist der einzige Ort, an dem ein Etikett der gemessenen Hardware widersprechen kann.
    auto const spd = erhoben(4800, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_TRUE(meas::ram_pair_etikett_widerspricht("ddr4_2x32", spd));
    EXPECT_FALSE(meas::error_class_of_ram_verdict(meas::RamIdentityVerdict::EtikettWiderspruch).has_value());

    // Der heutige Schluessel der Referenz-Maschine widerspricht NICHT -- gezogen, nicht getippt.
    auto const& m = referenz_maschine();
    EXPECT_FALSE(meas::ram_pair_etikett_widerspricht(m.ram_pair, spd))
        << "Nach dem E-2-Schluesselfix darf keine deklarierte Maschine mehr widersprechen.";
    EXPECT_NE(meas::verify_declared_ram(&m, spd, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::EtikettWiderspruch);
}

TEST(P2RamIdentityVerdict, NurEinSpdBelegBegruendetEineGenerationsAussage) {
    // Ein Deklarations-Wert belegt nichts ueber die Hardware -- er darf dem Etikett deshalb nicht
    // widersprechen. Sonst widerspraeche sich die Deklaration selbst.
    auto const deklariert = erhoben(4800, meas::RamFrequencyProvenance::DeclaredNotMeasured);
    EXPECT_FALSE(meas::ram_pair_etikett_widerspricht("ddr4_2x32", deklariert));
    // Und ein Etikett ohne Generations-Angabe schweigt: Schweigen ist kein Widerspruch.
    auto const spd = erhoben(4800, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_FALSE(meas::ram_pair_etikett_widerspricht("unbenannt_2x32", spd));
}

TEST(P2RamIdentityVerdict, DasEtikettWirdVORDerZahlGeprueft) {
    // Zwei Zahlen aus verschiedenen Speicher-Generationen sagen nichts ueber einander, auch wenn sie
    // gleich sind. Deshalb darf ein Generations-Widerspruch nicht von einem zufaelligen Zahlen-Match
    // ueberdeckt werden. Der Fall wird ueber die Regel-Funktion belegt, weil keine DEKLARIERTE
    // Maschine ihn heute noch traegt (E-2) -- ihn ueber eine erfundene Zeile zu zeigen hiesse, eine
    // Maschine zu erfinden.
    auto const spd_gleiche_zahl =
        erhoben(referenz_maschine().ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_TRUE(meas::ram_pair_etikett_widerspricht("ddr4_2x32", spd_gleiche_zahl))
        << "Die Gleichheit der Zahlen darf den Generations-Widerspruch nicht verdecken.";
}

// =================================================================================================
// 4. DIE TAXONOMIE SELBST
// =================================================================================================
TEST(P2RamIdentityVerdict, JedesVerdiktTraegtEinEigenesLesbaresEtikett) {
    for (std::size_t i = 0; i < meas::kRamIdentityVerdictCount; ++i) {
        auto const v = static_cast<meas::RamIdentityVerdict>(i);
        EXPECT_FALSE(meas::ram_identity_verdict_label(v).empty());
        EXPECT_NE(meas::ram_identity_verdict_label(v), std::string_view{"ram_verdikt_unbekannt"})
            << "Verdikt " << i << " faellt in den Fallback -- es fehlt ein case.";
        // Paarweise Verschiedenheit: zwei Verdikte mit demselben Etikett waeren in der Auswertung
        // nicht auseinanderzuhalten.
        for (std::size_t j = i + 1; j < meas::kRamIdentityVerdictCount; ++j)
            EXPECT_NE(meas::ram_identity_verdict_label(v),
                      meas::ram_identity_verdict_label(static_cast<meas::RamIdentityVerdict>(j)));
    }
}

// =================================================================================================
// 3b. P3 -- DIE URSPRUNGS-NOTIZ IST PRODUKTIV (Kreuzproben zur 2-Argument-Form)
// =================================================================================================
TEST(P3DeclarationOrigin, DieProduktiveNotizKipptDasFruehereMatch) {
    // DER Umschlag-Fall des Pakets: VOR P3 war die Notiz ueberall Unbekannt, und eine SPD-Nennrate
    // mit der deklarierten Zahl ergab ein Match (der Test oben mit explizitem Unbekannt belegt das
    // weiterhin). SEIT P3 traegt die Referenz-Zeile ihren dmidecode-Ursprung selbst -- dieselben
    // Zahlen ergeben ueber die Produktions-Form UnvergleichbareStufen (A3).
    auto const& m = referenz_maschine();
    ASSERT_EQ(m.ram_frequency_origin, meas::DeclarationOrigin::EingefrorenesDmi)
        << "Die Referenz-Zeile (einzige mit deklarierter Frequenz) MUSS seit P3 den eingefrorenen "
           "dmidecode-Ursprung tragen -- faellt er weg, ist A3 wieder entschaerft.";
    EXPECT_FALSE(m.ram_frequency_source.empty()) << "Die source_id-Notiz (A2) gehoert zur Deklaration.";

    auto const spd = erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase);
    EXPECT_EQ(meas::verify_declared_ram(&m, spd), meas::RamIdentityVerdict::UnvergleichbareStufen)
        << "Die 2-Argument-Form zieht die Notiz aus der Maschine -- gleiche Zahlen, ungleiche "
           "Groessen, kein Match.";
    // Gleiche Groesse bleibt ein echtes Urteil -- in beide Richtungen.
    EXPECT_EQ(
        meas::verify_declared_ram(&m, erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::ConfiguredMeasured)),
        meas::RamIdentityVerdict::Match);
    EXPECT_EQ(meas::verify_declared_ram(
                  &m, erhoben(m.ram_frequency_mhz + 800U, meas::RamFrequencyProvenance::ConfiguredMeasured)),
              meas::RamIdentityVerdict::Abweichung);
}

TEST(P3DeclarationOrigin, DieZweiArgumentFormLiestGenauDieNotizDerMaschine) {
    // Selbstkonsistenz statt zweiter Regel-Tabelle: fuer JEDE deklarierte Maschine und JEDE
    // Provenienz muss die Produktions-Form exakt das Urteil der Kern-Form mit der Maschinen-Notiz
    // liefern. Damit kann die Overload nie eine eigene, leicht andere Regel entwickeln.
    for (auto const& m : meas::kDeclaredMachines) {
        for (std::size_t p = 0; p < meas::kRamFrequencyProvenanceCount; ++p) {
            auto const r = erhoben(4800, static_cast<meas::RamFrequencyProvenance>(p));
            EXPECT_EQ(meas::verify_declared_ram(&m, r), meas::verify_declared_ram(&m, r, m.ram_frequency_origin))
                << "Maschine " << m.machine_id << ", Provenienz " << p;
        }
    }
    // Der nullptr-Fall der neuen Form bleibt der benannte Ausgang.
    EXPECT_EQ(meas::verify_declared_ram(nullptr, erhoben(4800, meas::RamFrequencyProvenance::SpdJedecBase)),
              meas::RamIdentityVerdict::KeineDeklaration);
}

TEST(P3DeclarationOrigin, F11Prod1BleibtOhneNachdeklaration) {
    // Owner-Entscheid F11 (01.08.2026, dauerhaft): die 0-Marken-Maschine bleibt undeklariert und
    // traegt folgerichtig KEINE Notiz -- die Kette liefert ihren Wert je Lauf (Stufe 1 live).
    auto const& ohne = maschine_mit_nullmarke();
    EXPECT_EQ(ohne.ram_frequency_origin, meas::DeclarationOrigin::Unbekannt);
    EXPECT_TRUE(ohne.ram_frequency_source.empty());
    EXPECT_EQ(meas::verify_declared_ram(&ohne, erhoben(5600, meas::RamFrequencyProvenance::ConfiguredMeasured)),
              meas::RamIdentityVerdict::KeineDeklaration)
        << "Eine echte Stufe-1-Erhebung gegen die 0-Marke ist KEIN Vergleich und KEINE Abweichung.";
}

TEST(P2RamIdentityVerdict, RamUndCpuVerdikteSindNebeneinanderLesbar) {
    // Beide Urteile stehen als eigene Spalten nebeneinander. Ein geteiltes Wort ("match") waere dort
    // nicht auseinanderzuhalten -- deshalb tragen die RAM-Etiketten durchgaengig ihr Praefix.
    for (std::size_t i = 0; i < meas::kRamIdentityVerdictCount; ++i) {
        auto const rl = meas::ram_identity_verdict_label(static_cast<meas::RamIdentityVerdict>(i));
        EXPECT_TRUE(rl.starts_with("ram_")) << rl << " traegt kein ram_-Praefix.";
        for (std::size_t j = 0; j < meas::kMachineIdentityVerdictCount; ++j)
            EXPECT_NE(rl, meas::machine_identity_verdict_label(static_cast<meas::MachineIdentityVerdict>(j)));
    }
}

TEST(P2RamIdentityVerdict, JederAusgangIstErreichbar) {
    // Ein unerreichbarer Ausgang waere toter Code, der im Log nie erscheint und dessen Fehlen
    // niemandem auffiele. Sechs der sieben werden hier aus echten Eingaben erzeugt; der siebte
    // (EtikettWiderspruch) haengt an einer Maschinen-Deklaration, die es nach E-2 nicht mehr gibt --
    // seine Regel-Funktion ist oben einzeln belegt.
    auto const& m    = referenz_maschine();
    auto const& ohne = maschine_mit_nullmarke();

    EXPECT_EQ(meas::verify_declared_ram(&m, meas::RamFrequencyReading{}, meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::NichtErhoben);
    EXPECT_EQ(meas::verify_declared_ram(nullptr, erhoben(4800, meas::RamFrequencyProvenance::SpdJedecBase),
                                        meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::KeineDeklaration);
    EXPECT_EQ(meas::verify_declared_ram(&ohne, erhoben(4800, meas::RamFrequencyProvenance::SpdJedecBase),
                                        meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::KeineDeklaration);
    EXPECT_EQ(meas::verify_declared_ram(&m,
                                        erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::DeclaredNotMeasured),
                                        meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::NurDeklariert);
    EXPECT_EQ(meas::verify_declared_ram(&m, erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase),
                                        meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::Match);
    EXPECT_EQ(meas::verify_declared_ram(&m,
                                        erhoben(m.ram_frequency_mhz + 800U, meas::RamFrequencyProvenance::SpdJedecBase),
                                        meas::DeclarationOrigin::Unbekannt),
              meas::RamIdentityVerdict::Abweichung);
    EXPECT_EQ(meas::verify_declared_ram(&m, erhoben(m.ram_frequency_mhz, meas::RamFrequencyProvenance::SpdJedecBase),
                                        meas::DeclarationOrigin::EingefrorenesDmi),
              meas::RamIdentityVerdict::UnvergleichbareStufen);
}
