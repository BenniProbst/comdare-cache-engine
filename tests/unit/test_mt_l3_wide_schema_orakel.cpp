// test_mt_l3_wide_schema_orakel -- MT-L3: das UNABHAENGIGE Schema-Orakel der WIDE-Mess-CSV
// (2026-08-10), Testklasse T-4. Google Test, laeuft in Debug UND Release.
//
// ===================================================================================================
// DER BEFUND, GEGEN DEN DIESER TEST GEBAUT IST
// ===================================================================================================
// Am 10.08.2026 mit zwei Werkzeugen am Objekt gemessen (git grep gegen den Baum-SHA, und unabhaengig
// find+grep gegen die Arbeitskopie, beide gegen denselben Nenner von 479 Test-.cpp):
//
//   33 Stellen in 19 Uebersetzungseinheiten unter tests/ rufen lazy_csv_header().
//   JEDE dieser Stellen zieht ihr SOLL aus dem Erzeuger SELBST.
//   Genau 1 Testdatei fuehrt den Voll-Header als Literal -- test_lazy_resume_binary.cpp -- und dort
//   ist es ein ABSICHTLICH VERALTETES Schema als Koeder fuer den Resume-Mismatch, also gerade KEIN
//   Orakel.
//
// Das ist die Klasse "test-zementiert-defekt": ein Schema-Vergleich, dessen SOLL aus dem PRUEFLING
// stammt, ist per Konstruktion immer gruen. Er sieht von aussen exakt aus wie eine scharfe Wache und
// deckt nichts. Aendert jemand den Kopf -- absichtlich oder aus Versehen -- klappert nichts, und die
// Bestands-Leser (Auswerte-Stufen, Resume-Wache, Lager, xlsx-Mappe, Thesis-PDF) brechen erst
// stromabwaerts, wo die Ursache nicht mehr zu sehen ist.
//
// DER GEGENSTAND, PRAEZISE: die grosse Produktions-Mess-CSV, Erzeuger lazy_csv_header() in
// builder/experiment_tree/cache_engine_builder_iterator.hpp, Trenner ';'. AUSDRUECKLICH NICHT der
// f15-Export result_csv_header() (builder/commands/result_aggregator.hpp) -- anderer Datentyp,
// anderer Konsument, anderes Schema, eigenes Orakel.
//
// ===================================================================================================
// WORAUS DAS SOLL KOMMT -- DREI QUELLEN, ZWEI DAVON DEM PRUEFLING FREMD (T-3 / V-7)
// ===================================================================================================
//   IST     lazy_csv_header()                     -- der Pruefling.
//   SOLL A  comdare::mt_l3::gefrorener_wide_kopf() -- eingefrorenes BYTE-Literal, mt_l3_wide_schema_-
//           gefroren.cpp. Diese Uebersetzungseinheit inkludiert <cstddef> und <string_view> und
//           SONST NICHTS: der Erzeuger ist dort weder direkt noch mittelbar sichtbar. Das ist der
//           Nachweis, dass das Soll nicht aus dem Pruefling stammen KANN.
//   SOLL B  kWideSchemaFreezeStufe1                -- 189 Spaltennamen, eingefroren am 09.08.2026 von
//           einem anderen Strang, aus einem anderen Bau, in cache_engine/measurement/schema_freeze.hpp.
//
// SOLL A und SOLL B sind unabhaengig voneinander entstanden. Der Test haelt sie deshalb auch
// GEGENEINANDER (Fall "ZweiFreezesStimmenUeberein"): stimmen zwei getrennt abgeschriebene Kopien
// ueberein, ist ein Abschreibfehler in einer von beiden ausgeschlossen -- und wer beim naechsten
// Schema-Bump nur eine der beiden nachzieht, wird rot statt still zu driften. Das ist die
// Schwesterpflicht T-6, als Werkzeug verdrahtet und nicht als gute Absicht notiert.
//
// ===================================================================================================
// BEIDE RICHTUNGEN (T-4) UND DER GEGENKOEDER (K13)
// ===================================================================================================
// Eine Wache, die nie rot wird, ist wertlos; eine, die immer rot ist, genauso. Dieser Test fuehrt
// deshalb VIER Gegeneingaenge vor, bevor er ueber das echte Schema irgendetwas Gruenes sagt:
//   (1) eine Spalte FEHLT           -- Spaltenzahl sinkt
//   (2) eine Spalte kommt DAZU      -- Spaltenzahl steigt
//   (3) zwei Spalten VERTAUSCHT     -- Spaltenzahl BLEIBT GLEICH; genau der Fall, den eine blosse
//                                      Zaehl-Wache durchwinkt
//   (4) der TRENNER wechselt        -- Namen und Reihenfolge bleiben, das Format nicht
// Alle vier laufen durch DIESELBE Vergleichsstrecke wie die scharfe Pruefung. Ein Gegeneingang, der
// einen eigenen, milderen Vergleich benutzt, beweist ueber die scharfe Pruefung nichts.
//
// Position und Name der Koeder sind je Lauf frisch GEWUERFELT (std::random_device), nicht fest
// verdrahtet: eine Wache, die nur an Position 0 oder nur am Namen "foo" biss, waere gegen genau eine
// Aenderung scharf und gegen alle anderen blind. Der Startwert steht woertlich in der Ausgabe, damit
// ein roter Lauf von Hand nachstellbar ist.
//
// WORAUF DIE KOEDER AUFSETZEN -- und warum ausdruecklich NICHT auf dem Pruefling:
// Sie mutieren SOLL A und halten das Ergebnis gegen SOLL B. Beide Quellen sind unabhaengig
// voneinander entstanden und beide sind dem Pruefling fremd; jede Mutation der einen MUSS sich
// gegen die andere zeigen. Der erste Entwurf setzte die Koeder auf lazy_csv_header() auf -- das
// wurde in einer Wegwerf-Mutation am 10.08. verworfen: driftet der Erzeuger, stimmt die ERWARTETE
// KLASSE des Koeders nicht mehr (aus "eine Spalte entfernt" wird "eine entfernt UND eine zugefuegt"),
// und der Koeder faellt aus einem zweiten Grund. Rot war er dann zwar -- aber nicht mehr fuer das,
// was er zusichert. Ein Koeder muss praezise bleiben, auch waehrend der Pruefling falsch ist.
//
// Der GEGENKOEDER ist ein eigener Fall ("GegenkoederUnmutierteBasisBleibtGruen"): dieselbe Strecke,
// dieselbe Vergleichsfunktion, unmutierte Basis -- und sie muss GRUEN melden. Ohne ihn waere ein Test,
// der blind alles rot faerbt, von einem scharfen nicht zu unterscheiden.
//
// ===================================================================================================
// NENNER IN DER AUSGABE (V-1)
// ===================================================================================================
// Jeder Fall druckt seine Nenner, auch im gruenen Fall: eine Wache, die nur "ok" sagt, ist von einer
// Wache, die gar nichts geprueft hat, nicht zu unterscheiden.
//
// WIE DAS LITERAL NACHZUZIEHEN IST, wenn eine Spalte absichtlich dazukommt: siehe Kopf von
// mt_l3_wide_schema_gefroren.cpp. Kurzform -- der Test DRUCKT bei Abweichung den neuen Kopf als
// einfuegefertigen C++-Block; abschreiben von Hand ist nicht noetig.
//
// Build: Google Test (gtest_main via comdare_add_test), Include-/Link-Rezeptur wie
// test_b3_schema_freeze_stufe1 (die TU zieht den ECHTEN Erzeuger statt eines Nachbaus).
// ASCII-only.
//
// SELBSTCHECK: Erschiene in dieser Datei jemals `soll = lazy_csv_header()` -- in welcher Schreibweise
// auch immer -- waere der Test ab dieser Zeile wertlos, egal wie gruen er meldet.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header (DER PRUEFLING)
#include <cache_engine/measurement/schema_freeze.hpp>                // Vergleichsstrecke + SOLL B

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

// SOLL A. Bewusst als Vorwaerts-Deklaration und nicht ueber einen gemeinsamen Header: der Uebersetzer
// bindet hier gegen eine Uebersetzungseinheit, die den Erzeuger nicht kennt. Ein Header, den beide
// Seiten teilen, waere ein Weg, ueber den der Erzeuger doch noch in das Orakel gelangen koennte.
namespace comdare::mt_l3 {
std::string_view gefrorener_wide_kopf() noexcept;
std::size_t      gefrorener_wide_nenner() noexcept;
char             gefrorener_wide_trenner() noexcept;
} // namespace comdare::mt_l3

namespace {

namespace mess = ::comdare::cache_engine::measurement;
namespace ex   = ::comdare::cache_engine::builder::experiment;

/// Der Startwert, aus dem alle Koeder dieses Laufs gewuerfelt werden. Einmal je Prozess gezogen und
/// woertlich ausgegeben -- ein roter Lauf muss von Hand nachstellbar sein.
std::uint32_t koeder_startwert() {
    static std::uint32_t const s = std::random_device{}();
    return s;
}

std::mt19937 wuerfel(std::uint32_t versatz) { return std::mt19937{koeder_startwert() + versatz}; }

/// Setzt eine Spaltenliste wieder zu einer Kopfzeile zusammen. Das ist die Umkehrung von
/// spalten_aus_kopfzeile und die einzige Stelle, an der die Koeder ihre Kopfzeile bauen -- damit
/// jeder Koeder exakt dieselbe Vergleichsstrecke durchlaeuft wie der scharfe Fall.
std::string kopfzeile_aus_spalten(std::vector<std::string> const& spalten, char trenner) {
    std::string h;
    for (std::size_t i = 0; i < spalten.size(); ++i) {
        if (i != 0) h += trenner;
        h += spalten[i];
    }
    return h;
}

/// Ein Spaltenname, der im echten Schema garantiert nicht vorkommt: gewuerfelt und mit einem Praefix
/// versehen, das kein Erzeuger je erzeugt. Ein fest verdrahtetes "foo" waere ein Koeder, der eines
/// Tages zufaellig ein echter Spaltenname wird.
std::string fremder_spaltenname(std::mt19937& rng) {
    std::uniform_int_distribution<int> buchstabe(0, 25);
    std::string                        s = "koeder_";
    for (int i = 0; i < 8; ++i) s += static_cast<char>('a' + buchstabe(rng));
    return s;
}

/// DIE EINE VERGLEICHSSTRECKE. Scharfe Pruefung und Koeder gehen beide hier durch -- sonst beweist
/// ein beissender Koeder ueber die scharfe Pruefung nichts.
mess::SchemaFreezeBefund gegen_das_orakel(std::string_view kopfzeile) {
    std::vector<std::string> const soll = mess::als_spaltenliste(mess::kWideSchemaFreezeStufe1);
    return mess::pruefe_schema_freeze(kopfzeile, comdare::mt_l3::gefrorener_wide_trenner(), soll);
}

/// Macht aus einem Rohstueck ein gueltiges C++-Quelltext-Literal. NICHT kosmetisch: lazy_csv_header()
/// haengt dem Kopf ein '\n' an, und ein roh ausgegebenes Zeilenende erzeugt einen Block, der sich gar
/// nicht uebersetzen laesst. Ein "einfuegefertiger" Block, der nicht uebersetzt, ist eine Ankuendigung
/// und kein Werkzeug (V-8).
std::string als_cpp_literal_inhalt(std::string_view roh) {
    std::string s;
    for (char const c : roh) {
        switch (c) {
            case '\n': s += "\\n"; break;
            case '\r': s += "\\r"; break;
            case '\t': s += "\\t"; break;
            case '"': s += "\\\""; break;
            case '\\': s += "\\\\"; break;
            default: s += c; break;
        }
    }
    return s;
}

/// Druckt den vorgelegten Kopf als einfuegefertigen C++-Block. Das ist Anforderung 4 als WERKZEUG:
/// ein Kommentar, der sagt "bitte nachziehen", wird umgangen; ein fertiger Block wird eingefuegt.
void drucke_einfuegefertiges_literal(std::string const& kopf, char trenner) {
    std::cout << "\n--- NEUES LITERAL, einfuegefertig fuer mt_l3_wide_schema_gefroren.cpp ---\n";
    std::cout << "inline constexpr std::string_view kGefrorenerWideKopf =\n";
    std::size_t i = 0;
    while (i < kopf.size()) {
        std::size_t ende = i;
        // Zeilen an Trenner-Grenzen brechen, hoechstens 100 Zeichen Nutzlast -- so bleibt die Zeile
        // mit Anfuehrungszeichen und Einrueckung unter der 120-Byte-Grenze des Hauses.
        while (ende < kopf.size()) {
            std::size_t const next = kopf.find(trenner, ende);
            std::size_t const kand = (next == std::string::npos) ? kopf.size() : next + 1;
            if (kand - i > 100 && ende > i) break;
            ende = kand;
            if (next == std::string::npos) break;
        }
        std::cout << "    \"" << als_cpp_literal_inhalt(std::string_view{kopf}.substr(i, ende - i)) << "\""
                  << (ende >= kopf.size() ? ";" : "") << "\n";
        i = ende;
    }
    std::cout << "--- ENDE NEUES LITERAL ---\n\n";
}

// =================================================================================================
// TEIL 1 -- DIE VIER GEGENEINGAENGE. Sie laufen VOR jeder gruenen Aussage ueber das echte Schema.
//
// Jeder mutiert SOLL A (das eingefrorene Byte-Literal) und haelt das Ergebnis gegen SOLL B (die 189
// Namen aus schema_freeze.hpp). Zwei dem Pruefling fremde Quellen, siehe Kopf -- so bleibt die
// ERWARTETE KLASSE des Koeders auch dann richtig, wenn der Erzeuger gerade driftet.
// =================================================================================================

/// (1) EINE SPALTE FEHLT. Muss rot machen, und zwar mit der Klasse ENTFERNT.
TEST(MtL3WideSchemaOrakel, GegeneingangFehlendeSpalteWirdRot) {
    std::string const        basis   = std::string{comdare::mt_l3::gefrorener_wide_kopf()};
    std::vector<std::string> spalten = mess::spalten_aus_kopfzeile(basis, comdare::mt_l3::gefrorener_wide_trenner());
    ASSERT_GT(spalten.size(), 1U) << "Ohne mindestens zwei Spalten ist dieser Koeder gegenstandslos.";

    std::mt19937                               rng = wuerfel(1);
    std::uniform_int_distribution<std::size_t> pos(0, spalten.size() - 1);
    std::size_t const                          weg  = pos(rng);
    std::string const                          name = spalten[weg];
    spalten.erase(spalten.begin() + static_cast<std::ptrdiff_t>(weg));

    std::string const              mutiert = kopfzeile_aus_spalten(spalten, comdare::mt_l3::gefrorener_wide_trenner());
    mess::SchemaFreezeBefund const b       = gegen_das_orakel(mutiert);

    std::cout << "[ MT-L3 ] KOEDER 1/4 fehlende Spalte: Startwert=" << koeder_startwert() << " Position=" << weg
              << " Name=\"" << name << "\" NENNER soll=" << b.soll_spalten << " ist=" << b.ist_spalten << "\n";

    EXPECT_FALSE(b.identisch) << "Eine entfernte Spalte muss rot machen.\n" << mess::schema_freeze_bericht(b, "WIDE");
    EXPECT_EQ(b.entfernt.size(), 1U) << "Genau eine Spalte wurde entfernt.";
    if (b.entfernt.size() == 1U) { EXPECT_EQ(b.entfernt[0], name); }
    EXPECT_TRUE(b.zugefuegt.empty());
    // Und die Byte-Strecke muss ebenfalls anschlagen -- sie ist die schaerfere der beiden.
    EXPECT_NE(mutiert, std::string{comdare::mt_l3::gefrorener_wide_kopf()});
}

/// (2) EINE SPALTE KOMMT DAZU. Muss rot machen, und zwar mit der Klasse ZUGEFUEGT.
TEST(MtL3WideSchemaOrakel, GegeneingangUnerwarteteSpalteWirdRot) {
    std::string const        basis   = std::string{comdare::mt_l3::gefrorener_wide_kopf()};
    std::vector<std::string> spalten = mess::spalten_aus_kopfzeile(basis, comdare::mt_l3::gefrorener_wide_trenner());
    ASSERT_FALSE(spalten.empty());

    std::mt19937                               rng = wuerfel(2);
    std::uniform_int_distribution<std::size_t> pos(0, spalten.size());
    std::size_t const                          wohin = pos(rng);
    std::string const                          name  = fremder_spaltenname(rng);
    spalten.insert(spalten.begin() + static_cast<std::ptrdiff_t>(wohin), name);

    std::string const              mutiert = kopfzeile_aus_spalten(spalten, comdare::mt_l3::gefrorener_wide_trenner());
    mess::SchemaFreezeBefund const b       = gegen_das_orakel(mutiert);

    std::cout << "[ MT-L3 ] KOEDER 2/4 unerwartete Spalte: Startwert=" << koeder_startwert() << " Position=" << wohin
              << " Name=\"" << name << "\" NENNER soll=" << b.soll_spalten << " ist=" << b.ist_spalten << "\n";

    EXPECT_FALSE(b.identisch) << "Eine unerwartete Spalte muss rot machen.\n" << mess::schema_freeze_bericht(b, "WIDE");
    EXPECT_EQ(b.zugefuegt.size(), 1U) << "Genau eine Spalte kam dazu.";
    if (b.zugefuegt.size() == 1U) { EXPECT_EQ(b.zugefuegt[0], name); }
    EXPECT_TRUE(b.entfernt.empty());
    EXPECT_NE(mutiert, std::string{comdare::mt_l3::gefrorener_wide_kopf()});
}

/// (3) ZWEI SPALTEN VERTAUSCHT. Die Spaltenzahl bleibt gleich -- genau der Fall, den eine blosse
///     Zaehl-Wache durchwinkt. Muss trotzdem rot machen, und zwar mit der Klasse UMGEORDNET.
TEST(MtL3WideSchemaOrakel, GegeneingangVertauschteSpaltenWerdenRot) {
    std::string const        basis   = std::string{comdare::mt_l3::gefrorener_wide_kopf()};
    std::vector<std::string> spalten = mess::spalten_aus_kopfzeile(basis, comdare::mt_l3::gefrorener_wide_trenner());
    ASSERT_GT(spalten.size(), 1U);

    std::mt19937                               rng = wuerfel(3);
    std::uniform_int_distribution<std::size_t> pos(0, spalten.size() - 1);
    std::size_t                                a     = pos(rng);
    std::size_t                                b_idx = pos(rng);
    // Zwei GLEICHE Namen zu tauschen waere keine Aenderung -- solange suchen, bis der Tausch einer ist.
    for (int versuch = 0; versuch < 64 && (a == b_idx || spalten[a] == spalten[b_idx]); ++versuch) {
        a     = pos(rng);
        b_idx = pos(rng);
    }
    ASSERT_NE(a, b_idx) << "Kein taugliches Paar gefunden -- der Koeder waere gegenstandslos.";
    ASSERT_NE(spalten[a], spalten[b_idx]);
    std::size_t const vorher = spalten.size();
    std::swap(spalten[a], spalten[b_idx]);

    std::string const              mutiert = kopfzeile_aus_spalten(spalten, comdare::mt_l3::gefrorener_wide_trenner());
    mess::SchemaFreezeBefund const b       = gegen_das_orakel(mutiert);

    std::cout << "[ MT-L3 ] KOEDER 3/4 vertauscht: Startwert=" << koeder_startwert() << " Positionen=" << a << "/"
              << b_idx << " NENNER soll=" << b.soll_spalten << " ist=" << b.ist_spalten
              << " (Spaltenzahl unveraendert: " << vorher << ")\n";

    EXPECT_EQ(b.ist_spalten, vorher) << "Der Tausch darf die Spaltenzahl nicht aendern -- sonst pruefte dieser "
                                        "Koeder dasselbe wie Koeder 1/2 und nicht das Umordnen.";
    EXPECT_FALSE(b.identisch) << "Vertauschte Spalten muessen rot machen.\n" << mess::schema_freeze_bericht(b, "WIDE");
    EXPECT_TRUE(b.zugefuegt.empty());
    EXPECT_TRUE(b.entfernt.empty());
    EXPECT_EQ(b.umgeordnet.size(), 2U) << "Ein Tausch aendert genau zwei Positionen.";
    EXPECT_NE(mutiert, std::string{comdare::mt_l3::gefrorener_wide_kopf()});
}

/// (4) DER TRENNER WECHSELT. Namen und Reihenfolge bleiben, das Format nicht. Eine reine
///     Namenslisten-Wache saehe hier nichts Auffaelliges; die Byte-Strecke sieht es.
TEST(MtL3WideSchemaOrakel, GegeneingangFalscherTrennerWirdRot) {
    std::string const basis   = std::string{comdare::mt_l3::gefrorener_wide_kopf()};
    std::string       mutiert = basis;
    std::size_t       ersetzt = 0;
    for (char& c : mutiert) {
        if (c == comdare::mt_l3::gefrorener_wide_trenner()) {
            c = ',';
            ++ersetzt;
        }
    }
    ASSERT_GT(ersetzt, 0U) << "Ohne einen einzigen Trenner im Kopf ist dieser Koeder gegenstandslos.";

    mess::SchemaFreezeBefund const b = gegen_das_orakel(mutiert);
    std::cout << "[ MT-L3 ] KOEDER 4/4 falscher Trenner: " << ersetzt
              << " Trenner ';' -> ',' ersetzt, NENNER soll=" << b.soll_spalten << " ist=" << b.ist_spalten << "\n";

    EXPECT_FALSE(b.identisch) << "Ein Trenner-Wechsel muss rot machen.\n" << mess::schema_freeze_bericht(b, "WIDE");
    EXPECT_EQ(b.ist_spalten, 1U) << "Mit dem falschen Trenner zerfaellt der Kopf nicht mehr -- eine Riesen-Spalte.";
    EXPECT_NE(mutiert, std::string{comdare::mt_l3::gefrorener_wide_kopf()});
}

// =================================================================================================
// TEIL 2 -- DER GEGENKOEDER. Dieselbe Strecke, unmanipuliert, muss GRUEN melden.
// =================================================================================================

/// Ohne diesen Fall waere ein Test, der blind alles rot faerbt, von einem scharfen nicht zu
/// unterscheiden. Er ist die zweite Haelfte von K13 und nicht optional.
///
/// Er nimmt AUSDRUECKLICH dieselbe Basis wie die vier Koeder -- SOLL A, nur eben unmutiert -- und
/// nicht den Erzeuger. Nur so sagt er wirklich "die Strecke ist nicht blind rot": liefe er ueber den
/// Erzeuger, wiederholte er bloss die scharfe Pruefung aus Teil 3 und faerbte sich bei jeder
/// Erzeuger-Drift mit rot, ohne dass ueber die Strecke selbst etwas ausgesagt waere.
TEST(MtL3WideSchemaOrakel, GegenkoederUnmutierteBasisBleibtGruen) {
    std::string const              basis = std::string{comdare::mt_l3::gefrorener_wide_kopf()};
    mess::SchemaFreezeBefund const b     = gegen_das_orakel(basis);
    std::cout << "[ MT-L3 ] GEGENKOEDER unmutierte Basis: NENNER soll=" << b.soll_spalten << " ist=" << b.ist_spalten
              << " -> " << (b.identisch ? "UNVERAENDERT" : "GEAENDERT") << "\n";
    EXPECT_TRUE(b.identisch) << "Die Vergleichsstrecke schlaegt an der UNMUTIERTEN Basis an -- eine Wache, die immer "
                                "rot ist, ist so wertlos wie eine, die nie rot wird.\n"
                             << mess::schema_freeze_bericht(b, "WIDE");
}

// =================================================================================================
// TEIL 3 -- DIE SCHARFE PRUEFUNG. Erst jetzt, nachdem die Koeder gebissen haben.
// =================================================================================================

/// Byte-genau gegen SOLL A. Das ist die schaerfste der drei Aussagen: sie deckt Spaltenzahl,
/// Reihenfolge, Namen, Trenner, Leerzeichen und einen etwaigen Abschluss-Trenner in einem Zug.
TEST(MtL3WideSchemaOrakel, WideKopfIstByteGleichDemEingefrorenenLiteral) {
    std::string const echt     = ex::lazy_csv_header();
    std::string const gefroren = std::string{comdare::mt_l3::gefrorener_wide_kopf()};
    std::size_t const ist_n    = mess::spalten_aus_kopfzeile(echt, comdare::mt_l3::gefrorener_wide_trenner()).size();
    std::size_t const soll_n = mess::spalten_aus_kopfzeile(gefroren, comdare::mt_l3::gefrorener_wide_trenner()).size();

    std::cout << "[ MT-L3 ] NENNER erzeugter Kopf (lazy_csv_header)      : " << ist_n << " Spalten, " << echt.size()
              << " Byte\n"
              << "[ MT-L3 ] NENNER eingefrorenes Literal (SOLL A)        : " << soll_n << " Spalten, "
              << gefroren.size() << " Byte\n"
              << "[ MT-L3 ] NENNER eingefrorener Nenner (SOLL A, Konstante): "
              << comdare::mt_l3::gefrorener_wide_nenner() << " Spalten\n";

    if (echt != gefroren) drucke_einfuegefertiges_literal(echt, comdare::mt_l3::gefrorener_wide_trenner());

    EXPECT_EQ(soll_n, comdare::mt_l3::gefrorener_wide_nenner())
        << "Das eingefrorene Literal und der eingefrorene Nenner widersprechen sich -- beim letzten Nachziehen "
           "wurde eine der beiden Stellen vergessen.";
    EXPECT_EQ(ist_n, comdare::mt_l3::gefrorener_wide_nenner())
        << "Die Spaltenzahl des erzeugten Kopfes weicht vom eingefrorenen Nenner ab.";
    EXPECT_EQ(echt, gefroren) << "Der erzeugte Kopf weicht BYTE-GENAU vom eingefrorenen ab. Ist die Aenderung "
                                 "gewollt, siehe Kopf von mt_l3_wide_schema_gefroren.cpp -- der einfuegefertige "
                                 "Block steht oben in dieser Ausgabe.";
}

/// Gegen SOLL B: Spaltenzahl, Reihenfolge und Namen gegen die 189 Namen aus schema_freeze.hpp --
/// eine Quelle, die weder aus dem Pruefling noch aus SOLL A abgeleitet ist.
TEST(MtL3WideSchemaOrakel, WideKopfHaeltSpaltenzahlReihenfolgeUndNamenGegenFremdeQuelle) {
    std::string const              echt = ex::lazy_csv_header();
    mess::SchemaFreezeBefund const b    = gegen_das_orakel(echt);

    std::cout << "[ MT-L3 ] NENNER Fremdquelle (kWideSchemaFreezeStufe1) : " << b.soll_spalten << " Spalten\n"
              << "[ MT-L3 ] NENNER erzeugter Kopf                        : " << b.ist_spalten << " Spalten\n";

    EXPECT_EQ(b.soll_spalten, mess::kWideSchemaFreezeSpalten);
    EXPECT_TRUE(b.identisch) << mess::schema_freeze_bericht(b, "WIDE");
    EXPECT_TRUE(b.zugefuegt.empty());
    EXPECT_TRUE(b.entfernt.empty());
    EXPECT_TRUE(b.umgeordnet.empty());
}

/// T-6, DIE SCHWESTERSTELLE ALS WERKZEUG: die beiden unabhaengig entstandenen Freezes gegeneinander.
/// Wer beim naechsten Schema-Bump nur einen von beiden nachzieht, wird hier rot -- und zwar auch
/// dann, wenn der Erzeuger selbst gar nicht mehr gebaut wuerde.
TEST(MtL3WideSchemaOrakel, ZweiUnabhaengigeFreezesStimmenUeberein) {
    std::vector<std::string> const soll_a =
        mess::spalten_aus_kopfzeile(comdare::mt_l3::gefrorener_wide_kopf(), comdare::mt_l3::gefrorener_wide_trenner());
    std::vector<std::string> const soll_b = mess::als_spaltenliste(mess::kWideSchemaFreezeStufe1);
    mess::SchemaFreezeBefund const b      = mess::pruefe_schema_freeze(soll_a, soll_b);

    std::cout << "[ MT-L3 ] NENNER SOLL A (mt_l3_wide_schema_gefroren.cpp, 2026-08-10) : " << soll_a.size() << "\n"
              << "[ MT-L3 ] NENNER SOLL B (schema_freeze.hpp, 2026-08-09)              : " << soll_b.size() << "\n";

    EXPECT_TRUE(b.identisch) << "Die beiden eingefrorenen Listen widersprechen sich. Genau eine von beiden wurde "
                                "beim letzten Schema-Bump nachgezogen -- die andere ist stale.\n"
                             << mess::schema_freeze_bericht(b, "WIDE SOLL A gegen SOLL B");
}

/// Die Voraussetzung der Aussage "umgeordnet": ohne Duplikatfreiheit waere ein Tausch zweier
/// gleichnamiger Spalten keine Aenderung, und Koeder 3 pruefte nichts. Geprueft statt angenommen.
TEST(MtL3WideSchemaOrakel, DasEingefroreneSchemaIstDuplikatfrei) {
    std::vector<std::string> const soll_a =
        mess::spalten_aus_kopfzeile(comdare::mt_l3::gefrorener_wide_kopf(), comdare::mt_l3::gefrorener_wide_trenner());
    std::cout << "[ MT-L3 ] NENNER Duplikatpruefung: " << soll_a.size() << " Spalten\n";
    EXPECT_FALSE(mess::hat_doppelte_spalten(soll_a))
        << "Doppelte Spaltennamen machen die Aussage 'umgeordnet' mehrdeutig.";
}

} // namespace
