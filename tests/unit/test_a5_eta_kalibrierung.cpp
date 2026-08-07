// test_a5_eta_kalibrierung -- A5 / F5-ETA-Paket (eta_kalibrierung.hpp).
//
// WAS HIER BEWIESEN WIRD -- und warum jeder Fall doppelt gefahren wird:
// Ein Schaetzer, der nie danebenlag, ist unbewiesen. Jeder Abschnitt zeigt deshalb ZWEI Seiten:
//   (1) den TREFFER  -- eine bekannte Folge, deren Ergebnis exakt vorhersagbar ist;
//   (2) den GEGENFALL -- zu wenige Punkte, ein Ausreisser, fehlende Kampagnen-Zellen -- und dass der
//       Schaetzer dort EHRLICH ist statt eine Zahl zu erfinden.
//
// Literal, zeit- und IO-frei: alle Dauern sind Argumente, keine Wall-Clock.

#include "bestandslog/bestandslog_lock.hpp" // (E2): merge_documents -- der Befund, auf den sich A5 stuetzt
#include "bestandslog/eta_kalibrierung.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace bl = comdare::cache_engine::builder::bestandslog;

namespace {

// Ein Block aus n Compiles zu je t Sekunden, jede Binary gleich gross.
void fuelle(bl::EtaKalibrierung& k, std::size_t n, double t_s, std::uint64_t bytes = 428032) {
    for (std::size_t i = 0; i < n; ++i) k.beobachte(t_s, bytes);
}

} // namespace

// ===========================================================================
// (A) BELASTBARKEIT: ab wann -- und was steht da, solange nicht?
// ===========================================================================

TEST(A5EtaKalibrierung, UnterhalbDerSchwelleGibtEsKEINEZahlSondernNA) {
    bl::EtaKalibrierung k{32}; // Doktrin-Schwelle = n_threads = 32 Punkte
    fuelle(k, 5, 2.0);         // nur 5 -> die Aussage traegt nicht

    auto const a = k.rest_projektion(4096);
    EXPECT_FALSE(a.belastbar);
    EXPECT_EQ(a.punkte, 5u);
    EXPECT_EQ(a.noetige_punkte, 32u);

    // DIE KERNAUSSAGE DIESES TESTS: keine erfundene 0, sondern die beiden ehrlichen Formen.
    EXPECT_EQ(bl::eta_draht(a), "") << "Draht-Form: leer == 'noch nicht kalibriert' (bestehende Konvention)";
    EXPECT_EQ(bl::eta_text(a), "n/a") << "Text-Form: n/a, NIE eine Zahl ohne Grundlage";

    // Und sie wird auch nicht heimlich veroeffentlicht.
    EXPECT_FALSE(bl::eta_neu_schreiben(0.0, a));
}

TEST(A5EtaKalibrierung, DieFruehePublikationWaereUmFaktorVierDanebenGewesen) {
    // GEGENFALL MIT ZAHLEN: die ersten 5 Compiles sind ccache-warm (0,5 s), die wahre Bau-Last ist 2,0 s.
    // Wer nach 5 Punkten veroeffentlicht, sagt 64 s statt 256 s -- Faktor 4. Genau das verhindert (A).
    bl::EtaKalibrierung k{32};
    fuelle(k, 5, 0.5);
    EXPECT_FALSE(k.belastbar()) << "5 < 32 -- es gibt keine Zahl, also auch keine falsche";

    // Was sie GESAGT haette, wenn sie gedurft haette (hier nur zur Groessenordnung nachgerechnet):
    double const waere = (0.5 * 4096.0) / 32.0;
    EXPECT_DOUBLE_EQ(waere, 64.0);

    // Nach 32 echten Punkten zu 2,0 s steht die belastbare Zahl -- und sie ist die richtige.
    bl::EtaKalibrierung k2{32};
    fuelle(k2, 32, 2.0);
    ASSERT_TRUE(k2.belastbar());
    EXPECT_DOUBLE_EQ(k2.rest_projektion(4096).eta_s, 256.0);
    EXPECT_NEAR(256.0 / waere, 4.0, 1e-12) << "der verhinderte Fehler betrug Faktor 4";
}

TEST(A5EtaKalibrierung, GenauAnDerSchwelleTraegtSie) {
    bl::EtaKalibrierung k{4};
    fuelle(k, 3, 2.0);
    EXPECT_FALSE(k.belastbar()) << "3 < 4";
    k.beobachte(2.0, 428032);
    EXPECT_TRUE(k.belastbar()) << "4 >= 4 -- die Schwelle ist erreicht, nicht ueberschritten noetig";
}

// ===========================================================================
// (B) AUSREISSER: Median fuer die Projektion, Summe fuer die Beobachtung
// ===========================================================================

TEST(A5EtaKalibrierung, EinZehnfachBauVerschiebtDenMedianNICHT) {
    // DER BISSBEWEIS: 31 Compiles zu 2,0 s + EINER zu 20,0 s (zehnfach). 32 Threads, 4096 Rest.
    bl::EtaKalibrierung k{32};
    fuelle(k, 31, 2.0);
    k.beobachte(20.0, 428032); // der Ausreisser
    ASSERT_TRUE(k.belastbar());
    ASSERT_EQ(k.punkte(), 32u);

    // Der Median ruehrt sich nicht.
    EXPECT_DOUBLE_EQ(k.median_t_s(), 2.0);

    // Das arithmetische Mittel dagegen schon: (31*2 + 20)/32 = 82/32 = 2,5625.
    double const mittel = (31.0 * 2.0 + 20.0) / 32.0;
    EXPECT_DOUBLE_EQ(mittel, 2.5625);

    // Und das ist der Preis, den die Projektion NICHT zahlt: 4096 Binaries x der Differenz.
    double const mit_median = k.rest_projektion(4096).eta_s;
    double const mit_mittel = (mittel * 4096.0) / 32.0;
    EXPECT_DOUBLE_EQ(mit_median, 256.0);
    EXPECT_DOUBLE_EQ(mit_mittel, 328.0);
    EXPECT_NEAR(mit_mittel / mit_median, 1.28125, 1e-12) << "der Mittelwert haette um 28,1 Prozent zu hoch gelegen";
}

TEST(A5EtaKalibrierung, DieBEOBACHTUNGDesBlocksBleibtDieDoktrinFormel) {
    // Der vermessene Block ist KEINE Schaetzung -- die 20 s wurden wirklich bezahlt und stehen in der
    // Summe. Hier gilt Sum(t_i)/N mit Untergrenze max(t_i), unveraendert (estimate_eta_s).
    bl::EtaKalibrierung k{32};
    fuelle(k, 31, 2.0);
    k.beobachte(20.0, 428032);
    auto const a = k.block_auskunft();
    ASSERT_TRUE(a.belastbar);
    // Sum(t_i)/N = 82/32 = 2,5625 -- aber die UNTERGRENZE max(t_i) = 20 greift: 32 Jobs auf 32 Threads
    // laufen alle gleichzeitig, der Block ist fertig, wenn der laengste fertig ist. Der Ausreisser steht
    // hier also mit seiner VOLLEN Zeit im Ergebnis. Genau darum ist die Beobachtung kein Median-Fall: er
    // hat den Block wirklich 20 s lang aufgehalten.
    EXPECT_DOUBLE_EQ(a.eta_s, 20.0) << "Untergrenze max(t_i), nicht der geglaettete Mittelwert";
    EXPECT_GT(a.eta_s, 82.0 / 32.0) << "die Untergrenze schlaegt die Division -- kein Wegmitteln";
}

TEST(A5EtaKalibrierung, UntergrenzeIstDerLaengsteBeobachteteCompile) {
    // Kleine Restmenge: die Projektion darf nie unter EINEN Compile fallen.
    bl::EtaKalibrierung k{32};
    fuelle(k, 31, 2.0);
    k.beobachte(50.0, 428032); // longest_seen = 50
    ASSERT_TRUE(k.belastbar());
    EXPECT_DOUBLE_EQ(k.longest_seen_s(), 50.0);
    // 10 Rest x Median 2,0 / 32 Threads = 0,625 s -- das waere physikalisch unmoeglich.
    EXPECT_DOUBLE_EQ(k.rest_projektion(10).eta_s, 50.0);
}

TEST(A5EtaKalibrierung, MedianBeiGeraderUndUngeraderAnzahl) {
    bl::EtaKalibrierung ungerade{3};
    ungerade.beobachte(1.0, 1);
    ungerade.beobachte(3.0, 1);
    ungerade.beobachte(2.0, 1);
    EXPECT_DOUBLE_EQ(ungerade.median_t_s(), 2.0);

    bl::EtaKalibrierung gerade{4};
    gerade.beobachte(4.0, 1);
    gerade.beobachte(1.0, 1);
    gerade.beobachte(3.0, 1);
    gerade.beobachte(2.0, 1);
    EXPECT_DOUBLE_EQ(gerade.median_t_s(), 2.5) << "Mittel der beiden mittleren Werte";
}

// ===========================================================================
// (C) RE-KALIBRIERUNG: Block-Schnitt und Abweichungs-Trigger
// ===========================================================================

TEST(A5EtaKalibrierung, BlockSchnittVerwirftZeitenAberNICHTDieUntergrenze) {
    bl::EtaKalibrierung k{4};
    fuelle(k, 4, 2.0);
    k.beobachte(90.0, 428032);
    ASSERT_DOUBLE_EQ(k.longest_seen_s(), 90.0);

    k.neuer_block();
    EXPECT_EQ(k.punkte(), 0u) << "die Zeiten des alten Blocks sind weg (LEDGER:3299)";
    EXPECT_FALSE(k.belastbar()) << "ein frischer Block ist erst wieder unbelastbar -- ehrlich";
    EXPECT_DOUBLE_EQ(k.longest_seen_s(), 90.0)
        << "die Untergrenze ist eine Aussage ueber die Bau-Last, kein Block-Datum";

    // Sie wirkt auch im neuen Block: kleine Restmenge -> Untergrenze greift weiterhin.
    fuelle(k, 4, 1.0);
    ASSERT_TRUE(k.belastbar());
    EXPECT_DOUBLE_EQ(k.rest_projektion(4).eta_s, 90.0);
}

TEST(A5EtaKalibrierung, NeuSchreibenNurBeiRelevanterAbweichung) {
    bl::EtaKalibrierung k{4};
    fuelle(k, 4, 2.0);
    auto const a = k.rest_projektion(64); // 64*2/4 = 32 s
    ASSERT_TRUE(a.belastbar);
    ASSERT_DOUBLE_EQ(a.eta_s, 32.0);

    EXPECT_TRUE(bl::eta_neu_schreiben(0.0, a)) << "die erste Kalibrierung ist immer eine Neuigkeit";
    EXPECT_FALSE(bl::eta_neu_schreiben(32.0, a)) << "unveraendert -> kein Lock, kein Schrieb";
    EXPECT_FALSE(bl::eta_neu_schreiben(40.0, a)) << "|32-40|/40 = 0,20 -- unter der 25-Prozent-Schwelle";
    EXPECT_TRUE(bl::eta_neu_schreiben(45.0, a)) << "|32-45|/45 = 0,289 -- darueber";
    // Exakt auf der Schwelle wird NICHT geschrieben (strikt groesser).
    EXPECT_FALSE(bl::eta_neu_schreiben(32.0 / 0.75, a)) << "|32-42,67|/42,67 = 0,25 exakt -> kein Schrieb";
}

TEST(A5EtaKalibrierung, EineNichtAussageWirdNIEMALSVeroeffentlicht) {
    bl::EtaKalibrierung k{32};
    fuelle(k, 3, 2.0);
    auto const a = k.rest_projektion(4096);
    ASSERT_FALSE(a.belastbar);
    EXPECT_FALSE(bl::eta_neu_schreiben(0.0, a));
    EXPECT_FALSE(bl::eta_neu_schreiben(1000.0, a)) << "auch eine grosse Abweichung rechtfertigt keine Nicht-Zahl";
}

TEST(A5EtaKalibrierung, RestNullIstEineECHTENullKeineFehlendeZahl) {
    bl::EtaKalibrierung k{4};
    fuelle(k, 4, 2.0);
    auto const a = k.rest_projektion(0);
    EXPECT_TRUE(a.belastbar) << "nichts offen ist eine gemessene Aussage";
    EXPECT_DOUBLE_EQ(a.eta_s, 0.0);
    // Sie wird trotzdem nicht als laufende ETA geschrieben (eine 0 im Feld waere im Takeover toedlich).
    EXPECT_FALSE(bl::eta_neu_schreiben(10.0, a));
}

// ===========================================================================
// (D) ZEIT UND GROESSE SIND ZWEI MENGEN
// ===========================================================================

TEST(A5EtaKalibrierung, FehlgeschlagenerCompileZaehltZeitAberNichtGroesse) {
    bl::EtaKalibrierung k{4};
    k.beobachte(2.0, std::uint64_t{1000});
    k.beobachte(2.0, std::uint64_t{1000});
    k.beobachte(2.0, std::uint64_t{1000});
    k.beobachte(6.0, std::nullopt); // Fehlschlag: 6 s verbraucht, KEINE Binary

    EXPECT_EQ(k.punkte(), 4u) << "die Zeit ist verbraucht und gehoert in die ETA";
    EXPECT_EQ(k.groessen_punkte(), 3u) << "die Binary existiert nicht und gehoert NICHT in avg_size";

    auto const a = k.block_auskunft();
    ASSERT_TRUE(a.belastbar);
    EXPECT_EQ(a.avg_size_bytes, 1000u) << "haette der Fehlschlag als 0 gezaehlt, waeren es 750 gewesen";
    // Sum = 12, Sum/N = 3 -- aber der Fehlschlag selbst dauerte 6 s und ist damit die Untergrenze.
    EXPECT_DOUBLE_EQ(a.eta_s, 6.0) << "die Zeit des Fehlschlags steht voll im Block-Ergebnis";
}

TEST(A5EtaKalibrierung, OhneEinzigeFertigeBinaryIstAvgSizeLEERNichtNull) {
    bl::EtaKalibrierung k{2};
    k.beobachte(3.0, std::nullopt);
    k.beobachte(3.0, std::nullopt);
    auto const a = k.block_auskunft();
    ASSERT_TRUE(a.belastbar) << "die ETA traegt -- zwei Zeit-Punkte bei Schwelle 2";
    EXPECT_EQ(a.groessen_punkte, 0u);
    EXPECT_EQ(bl::avg_size_draht(a), "") << "keine Binary vermessen -> leeres Feld, keine 0";
    EXPECT_NE(bl::eta_draht(a), "") << "die ZEIT-Aussage bleibt davon unberuehrt (Befund je Feld)";
}

TEST(A5EtaKalibrierung, NichtEndlicheOderNegativeDauernWerdenVerworfen) {
    bl::EtaKalibrierung k{4};
    k.beobachte(0.0, std::uint64_t{1});                                      // keine Dauer
    k.beobachte(-1.0, std::uint64_t{1});                                     // Uhren-Befund
    k.beobachte(std::numeric_limits<double>::infinity(), std::uint64_t{1});  // Uhren-Befund
    k.beobachte(std::numeric_limits<double>::quiet_NaN(), std::uint64_t{1}); // Uhren-Befund
    EXPECT_EQ(k.punkte(), 0u) << "keiner davon ist ein Messpunkt -- und keiner wird als 0 gebucht";
    EXPECT_EQ(k.groessen_punkte(), 0u) << "eine verworfene Dauer zieht ihre Groesse mit";
}

// ===========================================================================
// (E) UEBERTRAG IN DIE RESERVIERUNG -- die Kopplung an den Takeover
// ===========================================================================

TEST(A5EtaKalibrierung, UnbelastbarBleibtDerProFormaZweigZustaendig) {
    bl::BatchReservierung r;
    bl::EtaKalibrierung   k{32};
    fuelle(k, 4, 2.0);
    bl::uebertrage_kalibrierung(r, k.rest_projektion(4096));

    EXPECT_EQ(r.eta_s, "");
    // DIE TRAGENDE FOLGE: ein leeres Feld ist keine Kalibrierung -> is_reservation_takeable urteilt weiter
    // ueber die pro-forma-Frist. Eine erfundene 0 haette hier "SOFORT uebernehmbar" bedeutet.
    EXPECT_FALSE(bl::has_usable_eta(r.eta_s));
}

TEST(A5EtaKalibrierung, BelastbarSchaltetDenETAZweigScharf) {
    bl::BatchReservierung r;
    bl::EtaKalibrierung   k{32};
    fuelle(k, 32, 2.0);
    bl::uebertrage_kalibrierung(r, k.rest_projektion(4096)); // 256 s

    EXPECT_EQ(r.eta_s, "256.000");
    EXPECT_EQ(r.avg_size_bytes, "428032");
    ASSERT_TRUE(bl::has_usable_eta(r.eta_s));

    // Und der Takeover rechnet ab hier gegen 1,5 x 256 = 384 s statt gegen die pauschale 30-min-Frist.
    EXPECT_FALSE(bl::is_takeable_by_eta(256.0, 1000, 1384)); // exakt 384 -> noch nicht
    EXPECT_TRUE(bl::is_takeable_by_eta(256.0, 1000, 1385));
}

// ===========================================================================
// (E2) DER MERGE-BEFUND -- warum je Block nur EINMAL veroeffentlicht wird
// ===========================================================================

// Dieser Test BEHAUPTET nichts ueber richtig oder falsch -- er NAGELT das heutige Verhalten des in B2
// eingefrorenen Record-Union-Merge fest, weil die A5-Verdrahtung sich darauf verlaesst:
//
//   Bei gleichem Fortschritts-Rang gewinnt die Reservierung mit gefuellter eta_s -- und wenn BEIDE eine
//   tragen, stabil das REMOTE-Dokument. Die zweite Fortschreibung DERSELBEN offenen Reservierung wird
//   also verworfen, waehrend store() true meldet.
//
// FOLGE: die Re-Kalibrierung je BLOCK (LEDGER:3299) traegt, weil jeder Slice eine eigene id hat. Die
// periodische Fortschreibung INNERHALB eines Slices (F5-Spez Baupunkt 3) traegt NICHT -- sie braucht
// eine geaenderte Konflikt-Aufloesung oder ein monotones Ordnungs-Feld (Draht-/Semantik-Entscheid).
//
// SCHLAEGT DIESER TEST FEHL, ist das eine gute Nachricht: dann wurde die Aufloesung geaendert, und die
// Fortschreibung innerhalb des Slices kann scharfgeschaltet werden (SliceEtaKanal::veroeffentlicht).
TEST(A5EtaKalibrierung, MergeVERWIRFTDieZweiteFortschreibungDerselbenReservierung) {
    auto mach = [](std::string const& eta, bl::BatchStatus s) {
        bl::BestandslogDocument d;
        d.genus = bl::Genus::binary;
        bl::BatchReservierung r;
        r.id     = "uuid/0";
        r.status = s;
        r.eta_s  = eta;
        d.reservierungen.push_back(r);
        return d;
    };

    // ERSTE Kalibrierung: remote traegt noch nichts -> der neue Wert landet.
    auto const erst = bl::merge_documents(mach("", bl::BatchStatus::offen), mach("250.000", bl::BatchStatus::offen));
    EXPECT_EQ(erst.reservierungen.at(0).eta_s, "250.000") << "die erste Kalibrierung kommt durch";

    // ZWEITE Fortschreibung: beide tragen eine ETA -> das REMOTE gewinnt, der neue Wert faellt weg.
    auto const zweit =
        bl::merge_documents(mach("100.000", bl::BatchStatus::offen), mach("250.000", bl::BatchStatus::offen));
    EXPECT_EQ(zweit.reservierungen.at(0).eta_s, "100.000")
        << "HEUTIGES Verhalten: die Fortschreibung wird verworfen -- deshalb je Block nur EIN Schrieb";

    // Der Done-Schrieb dagegen gewinnt ueber den Rang -- das Abschluss-Testat ist davon unberuehrt.
    auto const ende =
        bl::merge_documents(mach("100.000", bl::BatchStatus::offen), mach("999.000", bl::BatchStatus::done));
    EXPECT_EQ(ende.reservierungen.at(0).eta_s, "999.000");
    EXPECT_EQ(ende.reservierungen.at(0).status, bl::BatchStatus::done);
}

// ===========================================================================
// (F) KAMPAGNEN-PROJEKTION
// ===========================================================================

TEST(A5EtaKalibrierung, VollstaendigeKampagneTrifftExakt) {
    // Zwei Maschinen, zwei Perms. m1 baut 4096 Stueck in 256 s, m2 braucht das Doppelte.
    std::vector<bl::KampagnenPosten> posten{
        {"m1", "p1", 256.0, 4096, 428032},
        {"m1", "p2", 256.0, 4096, 428032},
        {"m2", "p1", 512.0, 4096, 428032},
        {"m2", "p2", 512.0, 4096, 428032},
    };
    auto const p = bl::projiziere_kampagne(posten, 131072, 4);

    ASSERT_TRUE(p.belastbar);
    EXPECT_TRUE(p.vollstaendig);
    EXPECT_EQ(p.zellen_ist, 4u);
    EXPECT_EQ(p.verworfen, 0u);
    // m1: 256/4096 * 131072 = 8192 s je Perm -> 16384 s.  m2: doppelt -> 32768 s.
    EXPECT_DOUBLE_EQ(p.wandzeit_s, 32768.0) << "die Kampagne ist fertig, wenn die LANGSAMSTE Maschine fertig ist";
    EXPECT_DOUBLE_EQ(p.rechenzeit_s, 49152.0) << "Flotten-Gesamtlast = 16384 + 32768";
    EXPECT_EQ(p.bytes, std::uint64_t{428032} * 131072u * 4u);
}

TEST(A5EtaKalibrierung, FehlendeZelleErgibtEineUNTERESCHRANKEKeineSchaetzung) {
    // DER GEGENFALL MIT ZAHLEN: dieselbe Kampagne, aber die zweite Perm der LANGSAMEN Maschine fehlt.
    std::vector<bl::KampagnenPosten> posten{
        {"m1", "p1", 256.0, 4096, 428032},
        {"m1", "p2", 256.0, 4096, 428032},
        {"m2", "p1", 512.0, 4096, 428032},
    };
    auto const p = bl::projiziere_kampagne(posten, 131072, 4);

    ASSERT_TRUE(p.belastbar);
    EXPECT_FALSE(p.vollstaendig) << "3 von 4 Zellen -- die Zahl ist nicht die Kampagne";
    EXPECT_EQ(p.zellen_ist, 3u);
    // m1 = 16384, m2 = 16384 -> max = 16384. Die WAHRE Zahl waere 32768 gewesen: die Projektion liegt um
    // Faktor 2 zu NIEDRIG. Genau deshalb wird sie als ">=" gefuehrt und nie als Schaetzung ausgegeben.
    EXPECT_DOUBLE_EQ(p.wandzeit_s, 16384.0);
    EXPECT_NE(bl::kampagnen_zeile(p).find("kampagne_eta_s=>="), std::string::npos)
        << "die Unvollstaendigkeit steht IN der Zeile, nicht in einer Fussnote";
    EXPECT_NE(bl::kampagnen_zeile(p).find("zellen=3/4"), std::string::npos) << "der Nenner faehrt immer mit";
}

TEST(A5EtaKalibrierung, PostenOhneBezugsgroesseWirdVerworfenNichtAlsNullGerechnet) {
    std::vector<bl::KampagnenPosten> posten{
        {"m1", "p1", 256.0, 4096, 428032},
        {"m1", "p2", 300.0, 0, 428032},  // keine Bezugsgroesse -> nicht skalierbar
        {"m2", "p1", 0.0, 4096, 428032}, // keine brauchbare Zeit
    };
    auto const p = bl::projiziere_kampagne(posten, 131072, 4);

    ASSERT_TRUE(p.belastbar);
    EXPECT_EQ(p.zellen_ist, 1u);
    EXPECT_EQ(p.verworfen, 2u) << "verworfen wird GEZAEHLT -- ein stiller Ausschluss ist ein Defekt";
    EXPECT_DOUBLE_EQ(p.wandzeit_s, 8192.0) << "nur der eine gueltige Posten traegt";
}

TEST(A5EtaKalibrierung, GarKeineDatenErgebenNAUndKeineZahl) {
    std::vector<bl::KampagnenPosten> leer{};
    auto const                       p = bl::projiziere_kampagne(leer, 131072, 4);

    EXPECT_FALSE(p.belastbar);
    EXPECT_FALSE(p.vollstaendig);
    EXPECT_DOUBLE_EQ(p.wandzeit_s, 0.0);
    auto const zeile = bl::kampagnen_zeile(p);
    EXPECT_NE(zeile.find("kampagne_eta_s=n/a"), std::string::npos);
    EXPECT_NE(zeile.find("bytes=n/a"), std::string::npos);
    EXPECT_NE(zeile.find("zellen=0/4"), std::string::npos);
}
