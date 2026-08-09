// D5-4 TRACE-SCHEMA (2026-08-09) -- die EINE Feldliste der Tier-Trace-Ausgabe, BEIDSEITIG geprueft.
//
// WAS DIESER TEST GIBT, DAS ES VORHER NICHT GAB
// Bis heute stand der Perzentil-Feldblock ZWEIMAL im Baum (tier_observe_trace_abi.hpp inline,
// genus_tier_observe_trace_abi.hpp als write_shared_json_fields). Beide riefen den richtigen KANON --
// und trotzdem fehlte delete_p99_ns in BEIDEN, weil keine Seite von der anderen wusste. Kein Test hat
// das gesehen: 12 Testdateien mit 478 EXPECT_-Zeilen, davon 132 mit Perzentilen, pruefen NAMENTLICH
// ausgewaehlte Einzelfelder ("traegt read_p99_ns"). Ein Feld, das NIEMAND nennt, ist fuer eine solche
// Suite unsichtbar. Dieser Test nennt keine Felder -- er laeuft ueber das Schema.
//
// SELBSTCHECK DIESER DATEI
//   ZUSICHERT: (1) ERZEUGER-SEITE -- jeder der FUENF JSON-Serialisierer (SearchAlgorithm + die vier
//              Container-Gattungen) schreibt JEDES Feld aus trace_schema::kLatenzFelder, in
//              Schema-REIHENFOLGE, mit dem KANON-Wert;
//              (2) LESER-SEITE -- ein Leser, der die JSON nur nach der FORM <praefix>_p<ziffern>_ns
//              absucht, findet in jedem der fuenf Ausgaben GENAU die Schema-Menge: keine Waise
//              (Feld im Serialisierer, nicht im Schema) und keine Fehlstelle (Feld im Schema, nicht in
//              der Ausgabe). Das ist die Richtung, die vorher gar nicht geprueft wurde;
//              (3) die WERTE sind von Hand nach k = ceil(q*n)-1 gerechnet und als Literale eingefroren
//              (T-3), auf drei GERADEN Stichprobenlaengen (20/100/200) -- dem Bereich, in dem sich die
//              Konventionen unterscheiden (T-4);
//              (4) die drei Kurven write/read/delete tragen DISJUNKTE Wertebereiche (1e3..2e4 /
//              ~1e5 / ~7e6) -- eine vertauschte Feld-zu-Kurve-Zuordnung faellt dadurch als Zahl auf,
//              nicht erst als Nachdenken;
//              (5) die CSV-Ausgaben tragen KEINE Perzentil-Spalte -- sonst gaebe es ein zweites,
//              ungeprueftes Schema neben dem JSON.
//   ZUSICHERT NICHT: nichts ueber die LESER ausserhalb dieses Repos. Die super-Werkzeuge
//              (Code/04_csv_to_latex, Code/05_diagram_generator) tragen eigene Feldlisten an einem
//              eigenen ce-Vendor-Stand -- das ist D5-2/D5-3. Nichts ueber die Roh-Kurven selbst
//              (dass write_ns wirklich Schreib-Zeiten enthaelt, belegen die Treiber-Tests). Nichts
//              ueber die uebrigen Median-Bauarten (eta_kalibrierung::median_t_s mittelt bei geradem n;
//              HDR-Histogramm ist ein eigenes Verfahren) -- das ist D5-2/D5-5.
//
// TDD-VERTRAG
//   T-1  Der Test lief ROT, bevor gebaut wurde -- belegt durch drei Koeder (s. KOEDER-PROTOKOLL unten).
//   T-2  Jede Zusicherung prueft einen WERT oder eine MENGE, nie blosse Existenz.
//   T-3  Die Sollwerte kommen NICHT aus dem Pruefling. Sie sind unten je Kurve mit der Rechnung
//        ausgeschrieben und als Literal eingefroren. Das Zufalls-Orakel rechnet den Rang in REINER
//        GANZZAHL-Arithmetik ((n*z + n_nenner - 1) / n_nenner) -- der Pruefling rechnet mit std::ceil
//        auf double plus Rundungsschutz. Zwei verschiedene Mechaniken, kein gemeinsamer Fehlerpfad.
//   T-4  ALLE drei Stichprobenlaengen sind GERADE (20/100/200), und alle drei Quantile treffen ein
//        ganzzahliges q*n. Genau dort weichen die Konventionen voneinander ab; auf ungerader Laenge
//        waere der Test blind (daran ist die bestehende Suite vollstaendig vorbeigelaufen).
//   K13  Der Koeder wurde FRISCH gewuerfelt und musste ERST BEISSEN.
//
// KOEDER-PROTOKOLL (2026-08-09; jeder Koeder wurde eingesetzt, gebaut, gefahren und zurueckgenommen.
// Zitiert ist die WOERTLICHE Ausgabe des Laufs, nicht ihre Zusammenfassung.)
//
//   K-A  Der Eintrag {"delete_p99_ns", ...} aus trace_schema::kLatenzFelder entfernt -- exakt der
//        Zustand VOR diesem Paket. ROT, 2 von 5 Tests:
//          "gefunden.size() Which is: 8 / kHandPins.size() Which is: 9"
//          "SearchAlgorithm: 8 Perzentilfelder statt 9"
//        BEFUND, DER FESTGEHALTEN GEHOERT: die LESER-Seite blieb hier GRUEN -- und das ist richtig.
//        Sie vergleicht Ausgabe gegen SCHEMA; faellt das Feld aus dem Schema, faellt es aus beiden
//        zugleich und die Mengen stimmen wieder ueberein. Was ein zu KLEINES Schema faengt, ist die
//        UNABHAENGIGE Hand-Liste kHandPins. Deshalb wird sie getrennt gefuehrt und nicht aus
//        kLatenzFelder abgeleitet -- eine abgeleitete Erwartung haette diesen Koeder durchgelassen.
//
//   K-B  stats::nearest_rank_index auf die VERWORFENE Formel round(q*(n-1)) gesetzt. ROT, 2 von 5
//        Tests, 15 Wert-Abweichungen ueber ALLE FUENF Serialisierer -- und zwar genau die Zahlen
//        aus der Divergenz-Tabelle unten:
//          "Which is: 11000 / Which is: 10000     SearchAlgorithm: Wert von write_p50_ns"
//          "Which is: 100350 / Which is: 100343   SearchAlgorithm: Wert von read_p50_ns"
//          "Which is: 7000300 / Which is: 7000297 SearchAlgorithm: Wert von delete_p50_ns"
//        Sie faerben NUR, weil alle drei Laengen GERADE sind (T-4). Auf ungerader Laenge liefert die
//        verworfene Formel dieselben Zahlen -- der Koeder haette nicht gebissen.
//
//   K-C  In genau EINEN Serialisierer (Set) ein handgeschriebenes ",\"delete_p999_ns\":0" gehaengt.
//        ROT, 2 von 5 Tests, und die LESER-Seite benennt die Waise namentlich:
//          "[D5-4]   WAISE Set: delete_p999_ns"
//          "Set: Feld in der Ausgabe, das das Schema nicht kennt"
//        Das ist der Fall, den eine Suite aus namentlichen Einzelfeld-Pruefungen NIE sehen kann:
//        niemand nennt ein Feld, von dem niemand weiss.
//
// SEED: kSeed ist am 2026-08-09 EINMAL aus /dev/urandom gewuerfelt (`od -An -N8 -tu8 /dev/urandom`)
// und wird bei jedem Lauf GEDRUCKT -- ein Fehlschlag ist damit exakt nachstellbar.

#include <builder/anatomy_commands/genus_tier_observe_trace_abi.hpp>
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp>
#include <builder/anatomy_commands/tier_trace_schema.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace ac = ::comdare::cache_engine::builder::anatomy_commands;
namespace ts = ::comdare::cache_engine::builder::anatomy_commands::trace_schema;
namespace an = ::comdare::cache_engine::anatomy;

namespace {

// EINMAL gewuerfelt: `od -An -N8 -tu8 /dev/urandom` am 2026-08-09.
constexpr std::uint64_t kSeed = 10548389114479757949ULL;

// =============================================================================================
// DIE HAND-PINS (T-3/T-4). Drei GERADE Laengen, drei DISJUNKTE Wertebereiche.
//
// Kanon: aufsteigend sortiertes Feld, Index k = ceil(q*n) - 1 (0-basiert).
//
//   write_ns: n = 20,  v[k] = 1000 * (k+1)          -> Bereich   1_000 ..    20_000
//     p50  k = ceil(0.50*20)-1 = ceil(10.0)-1 = 10-1 =  9  -> 1000*10 =    10_000
//     p95  k = ceil(0.95*20)-1 = ceil(19.0)-1 = 19-1 = 18  -> 1000*19 =    19_000
//     p99  k = ceil(0.99*20)-1 = ceil(19.8)-1 = 20-1 = 19  -> 1000*20 =    20_000
//
//   read_ns:  n = 100, v[k] = 100000 + 7*k          -> Bereich 100_000 ..   100_693
//     p50  k = ceil(0.50*100)-1 = ceil(50.0)-1 = 49 -> 100000 + 343 =    100_343
//     p95  k = ceil(0.95*100)-1 = ceil(95.0)-1 = 94 -> 100000 + 658 =    100_658
//     p99  k = ceil(0.99*100)-1 = ceil(99.0)-1 = 98 -> 100000 + 686 =    100_686
//
//   delete_ns: n = 200, v[k] = 7000000 + 3*k        -> Bereich 7_000_000 .. 7_000_597
//     p50  k = ceil(0.50*200)-1 = ceil(100.0)-1 =  99 -> 7000000 + 297 = 7_000_297
//     p95  k = ceil(0.95*200)-1 = ceil(190.0)-1 = 189 -> 7000000 + 567 = 7_000_567
//     p99  k = ceil(0.99*200)-1 = ceil(198.0)-1 = 197 -> 7000000 + 591 = 7_000_591
//
// WARUM DIESE ZAHLEN IM DIVERGIERENDEN BEREICH LIEGEN (der Punkt von T-4). Gegenprobe gegen die
// beiden Formeln, die dieses Repo VERWORFEN hat:
//   (A) k = min(n-1, floor(q*n))      -- bis 2026-08-09 die Rechnung von percentile_ns
//   (B) k = round(q*(n-1))            -- bis 2026-08-09 detail::nearest_rank_p
//                     Kanon        (A) alt          (B) verworfen
//   write  p50       10_000        11_000  ABW      11_000  ABW
//   write  p95       19_000        20_000  ABW      19_000  gleich
//   read   p50      100_343       100_350  ABW     100_350  ABW
//   read   p95      100_658       100_665  ABW     100_658  gleich
//   read   p99      100_686       100_693  ABW     100_686  gleich
//   delete p50    7_000_297     7_000_300  ABW   7_000_300  ABW
//   delete p95    7_000_567     7_000_570  ABW   7_000_567  gleich
//   delete p99    7_000_591     7_000_594  ABW   7_000_591  gleich
// Fuenf der neun Pins trennen den Kanon von (A), drei von (B). Bei UNGERADER Laenge waere die
// Spalte (A) durchweg gleich -- dann pinnte dieser Test nur, dass es ueberhaupt eine Formel gibt.
// =============================================================================================

constexpr std::int64_t kWriteP50 = 10000;
constexpr std::int64_t kWriteP95 = 19000;
constexpr std::int64_t kWriteP99 = 20000;
constexpr std::int64_t kReadP50  = 100343;
constexpr std::int64_t kReadP95  = 100658;
constexpr std::int64_t kReadP99  = 100686;
constexpr std::int64_t kDelP50   = 7000297;
constexpr std::int64_t kDelP95   = 7000567;
constexpr std::int64_t kDelP99   = 7000591;

/// Die Kurven werden ABSTEIGEND befuellt. Wer den Wert am ROHEN Index k zoege statt am Index des
/// sortierten Feldes, bekaeme durchweg etwas anderes -- die Sortierung ist damit mitgeprueft.
[[nodiscard]] std::vector<std::int64_t> kurve(std::size_t n, std::int64_t basis, std::int64_t schritt) {
    std::vector<std::int64_t> v;
    v.reserve(n);
    for (std::size_t i = n; i > 0; --i) v.push_back(basis + schritt * static_cast<std::int64_t>(i - 1));
    return v;
}

[[nodiscard]] std::vector<std::int64_t> write_kurve() { return kurve(20, 1000, 1000); }
[[nodiscard]] std::vector<std::int64_t> read_kurve() { return kurve(100, 100000, 7); }
[[nodiscard]] std::vector<std::int64_t> delete_kurve() { return kurve(200, 7000000, 3); }

/// Die neun Hand-Pins in SCHEMA-Reihenfolge -- getrennt von kLatenzFelder gefuehrt, damit eine
/// Aenderung an der Feldliste diesen Test zwingt, die Zahlen NEU VON HAND zu rechnen (und nicht
/// still mitwandert). Die Kopplung wird unten ueber die Namen geprueft, nicht ueber die Position.
struct HandPin {
    std::string_view name;
    // 09.08.2026: Initialisierer ergaenzt. Jede der neun Instanzen unten wird per
    // Aggregat-Initialisierung mit beiden Feldern belegt, der Wert ist also nie unbestimmt --
    // aber cppcheck sieht das nicht (uninitMemberVarNoCtor) und lint:static faellt hart rot.
    // Ein Default-Initialisierer kostet hier nichts und nimmt der Klasse die Moeglichkeit,
    // spaeter doch unbestimmt zu sein, wenn jemand eine zehnte Zeile unvollstaendig anlegt.
    std::int64_t wert{};
};
constexpr std::array<HandPin, 9> kHandPins{{
    {"write_p50_ns", kWriteP50},
    {"write_p95_ns", kWriteP95},
    {"write_p99_ns", kWriteP99},
    {"read_p50_ns", kReadP50},
    {"read_p95_ns", kReadP95},
    {"read_p99_ns", kReadP99},
    {"delete_p50_ns", kDelP50},
    {"delete_p95_ns", kDelP95},
    {"delete_p99_ns", kDelP99},
}};

// =============================================================================================
// DIE LESER-SEITE. Sie kennt KEINEN Feldnamen -- nur die FORM <praefix>_p<ziffern>_ns.
// Genau dadurch sieht sie eine Waise (Feld nur im Serialisierer) ebenso wie eine Fehlstelle
// (Feld nur im Schema). Ein Leser, der nach bekannten Namen sucht, kann eine Waise nie finden.
// =============================================================================================
[[nodiscard]] bool ist_perzentil_name(std::string const& n) {
    constexpr std::string_view kEnde = "_ns";
    if (n.size() < kEnde.size() + 3) return false;
    if (n.compare(n.size() - kEnde.size(), kEnde.size(), kEnde) != 0) return false;
    std::size_t const nach_ziffern = n.size() - kEnde.size();
    std::size_t       d            = nach_ziffern;
    while (d > 0 && n[d - 1] >= '0' && n[d - 1] <= '9') --d;
    if (d == nach_ziffern) return false; // keine Ziffer unmittelbar vor _ns
    return d >= 2 && n[d - 1] == 'p' && n[d - 2] == '_';
}

/// Alle Perzentil-FORM-Schluessel der JSON, in Reihenfolge ihres Auftretens. Ein Schluessel ist ein
/// Token in Anfuehrungszeichen, dem unmittelbar ':' folgt.
[[nodiscard]] std::vector<std::string> perzentil_schluessel(std::string const& js) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i + 1 < js.size(); ++i) {
        if (js[i] != '"') continue;
        std::size_t const ende = js.find('"', i + 1);
        if (ende == std::string::npos) break;
        std::string const name = js.substr(i + 1, ende - i - 1);
        i                      = ende;
        if (ende + 1 >= js.size() || js[ende + 1] != ':') continue;
        if (ist_perzentil_name(name)) out.push_back(name);
    }
    return out;
}

[[nodiscard]] std::int64_t json_feld(std::string const& js, std::string_view name) {
    std::string const schluessel = "\"" + std::string(name) + "\":";
    std::size_t const pos        = js.find(schluessel);
    if (pos == std::string::npos) return -999999;
    return std::stoll(js.substr(pos + schluessel.size()));
}

// =============================================================================================
// DAS ORAKEL (T-3/T-5): definitionsbasiert und in REINER GANZZAHL-Arithmetik.
// P(q) ist der KLEINSTE Stichprobenwert v, fuer den mindestens R = ceil(q*n) Werte <= v sind.
// Der Rang wird als ceil(n*z/nn) = (n*z + nn - 1) / nn gerechnet -- OHNE double, ohne std::ceil,
// ohne Rundungsschutz. Der Pruefling rechnet mit std::ceil auf double plus Epsilon-Schutz; genau
// diese Differenz der Mechanik ist der Sinn des Orakels.
// =============================================================================================
struct QuantilBruch {
    std::int64_t zaehler;
    std::int64_t nenner;
    double       als_double;
};
constexpr std::array<QuantilBruch, 3> kQuantile{{{1, 2, 0.50}, {19, 20, 0.95}, {99, 100, 0.99}}};

[[nodiscard]] std::int64_t orakel(std::vector<std::int64_t> const& s, QuantilBruch const& q) {
    if (s.empty()) return 0;
    auto const   n    = static_cast<std::int64_t>(s.size());
    std::int64_t rang = (n * q.zaehler + q.nenner - 1) / q.nenner; // ceil(n*z/nn), 1-basiert
    if (rang < 1) rang = 1;
    if (rang > n) rang = n;
    std::int64_t bester = 0;
    bool         hat    = false;
    for (std::int64_t kandidat : s) {
        std::int64_t anzahl = 0;
        for (std::int64_t x : s) {
            if (x <= kandidat) ++anzahl;
        }
        if (anzahl >= rang && (!hat || kandidat < bester)) {
            bester = kandidat;
            hat    = true;
        }
    }
    return hat ? bester : *std::max_element(s.begin(), s.end());
}

// =============================================================================================
// Trace-Bau: dieselben drei Kurven in JEDE der fuenf Gattungen. Die Ausgaben muessen sich in den
// Perzentil-Feldern deshalb NICHT unterscheiden -- Abweichung == eine der Gattungen rechnet eigen.
// =============================================================================================
template <class Snap>
void fuelle(Snap& snap) {
    snap.fill_level      = 200;
    snap.observe_wall_ns = 4242;
    snap.write_ns        = write_kurve();
    snap.read_ns         = read_kurve();
    snap.delete_ns       = delete_kurve();
}

template <class Trace>
[[nodiscard]] Trace genus_trace() {
    Trace                                        t;
    typename decltype(t.checkpoints)::value_type snap;
    fuelle(snap);
    t.checkpoints.push_back(std::move(snap));
    return t;
}

[[nodiscard]] ac::AbiTierObserveTrace abi_trace() {
    ac::AbiTierObserveTrace  t;
    ac::AbiFillLevelSnapshot snap;
    fuelle(snap);
    t.checkpoints.push_back(std::move(snap));
    return t;
}

/// Die fuenf REALEN JSON-Serialisierer, ueber denselben Kurven. Kein Nachbau.
[[nodiscard]] std::vector<std::pair<std::string, std::string>> alle_json_ausgaben() {
    return {
        {"SearchAlgorithm", ac::serialize_abi_tier_trace_json(abi_trace())},
        {"Set", ac::serialize_set_tier_trace_json(genus_trace<ac::SetTierObserveTrace>())},
        {"Sequence", ac::serialize_sequence_tier_trace_json(genus_trace<ac::SequenceTierObserveTrace>())},
        {"Adapter", ac::serialize_adapter_tier_trace_json(genus_trace<ac::AdapterTierObserveTrace>())},
        {"View", ac::serialize_view_tier_trace_json(genus_trace<ac::ViewTierObserveTrace>())},
    };
}

/// Die fuenf REALEN CSV-Serialisierer -- sie duerfen KEINE Perzentil-Spalte tragen.
[[nodiscard]] std::vector<std::pair<std::string, std::string>> alle_csv_ausgaben() {
    return {
        {"SearchAlgorithm", ac::serialize_abi_tier_trace_csv(abi_trace())},
        {"Set", ac::serialize_set_tier_trace_csv(genus_trace<ac::SetTierObserveTrace>())},
        {"Sequence", ac::serialize_sequence_tier_trace_csv(genus_trace<ac::SequenceTierObserveTrace>())},
        {"Adapter", ac::serialize_adapter_tier_trace_csv(genus_trace<ac::AdapterTierObserveTrace>())},
        {"View", ac::serialize_view_tier_trace_csv(genus_trace<ac::ViewTierObserveTrace>())},
    };
}

} // namespace

// =================================================================================================
// (1) Das Schema selbst: neun Felder, drei Kurven, drei Quantile -- und delete_ IST symmetrisch.
// =================================================================================================
TEST(D54TraceSchema, SchemaIstSymmetrischUeberDieDreiKurven) {
    ASSERT_EQ(ts::kLatenzFelder.size(), 9u) << "drei Kurven x drei Quantile";
    ASSERT_EQ(kHandPins.size(), ts::kLatenzFelder.size()) << "Hand-Pins und Schema muessen deckungsgleich sein";
    // Namen und Reihenfolge sind VERTRAG -- die Hand-Pins fuehren dieselbe Liste unabhaengig.
    for (std::size_t i = 0; i < ts::kLatenzFelder.size(); ++i) {
        EXPECT_EQ(ts::kLatenzFelder[i].name, kHandPins[i].name) << "Schema-Position " << i;
    }
    // Je Kurve GENAU die drei Quantile 0.50/0.95/0.99 -- keine Kurve darf eins weniger tragen.
    // Genau diese Asymmetrie war der Defekt: delete_ hatte p50/p95, aber kein p99.
    for (auto kur : {ts::LatenzKurve::Write, ts::LatenzKurve::Read, ts::LatenzKurve::Delete}) {
        std::vector<double> qs;
        for (auto const& f : ts::kLatenzFelder) {
            if (f.kurve == kur) qs.push_back(f.q);
        }
        ASSERT_EQ(qs.size(), 3u) << "jede Kurve traegt drei Quantile";
        EXPECT_EQ(qs[0], 0.50);
        EXPECT_EQ(qs[1], 0.95);
        EXPECT_EQ(qs[2], 0.99);
    }
}

// =================================================================================================
// (2) ERZEUGER-SEITE: jeder der fuenf Serialisierer schreibt jedes Schema-Feld, in Schema-Reihenfolge,
//     mit dem VON HAND gerechneten Wert. GERADE Laengen (T-4).
// =================================================================================================
TEST(D54TraceSchema, ErzeugerSeiteAlleFuenfSerialisiererGegenHandPins) {
    std::cout << "[D5-4] Seed: " << kSeed << "\n";
    std::cout << "[D5-4] Stichprobenlaengen write/read/delete = " << write_kurve().size() << '/' << read_kurve().size()
              << '/' << delete_kurve().size() << " (alle GERADE -- T-4)\n";
    ASSERT_EQ(write_kurve().size() % 2, 0u);
    ASSERT_EQ(read_kurve().size() % 2, 0u);
    ASSERT_EQ(delete_kurve().size() % 2, 0u);

    auto const ausgaben = alle_json_ausgaben();
    ASSERT_EQ(ausgaben.size(), 5u) << "fuenf Gattungen, fuenf JSON-Serialisierer";
    for (auto const& [gattung, js] : ausgaben) {
        auto const gefunden = perzentil_schluessel(js);
        ASSERT_EQ(gefunden.size(), kHandPins.size())
            << gattung << ": " << gefunden.size() << " Perzentilfelder statt " << kHandPins.size();
        for (std::size_t i = 0; i < kHandPins.size(); ++i) {
            EXPECT_EQ(gefunden[i], kHandPins[i].name) << gattung << ": Reihenfolge an Position " << i;
            EXPECT_EQ(json_feld(js, kHandPins[i].name), kHandPins[i].wert)
                << gattung << ": Wert von " << kHandPins[i].name;
        }
    }
    std::cout << "[D5-4] Erzeuger-Seite: 5 Serialisierer x " << kHandPins.size()
              << " Felder = " << (5u * kHandPins.size()) << " Wert-Pins geprueft\n";
}

// =================================================================================================
// (3) LESER-SEITE: die FORM-Suche findet GENAU die Schema-Menge -- keine Waise, keine Fehlstelle.
//     Das ist die Richtung, die vorher fehlte: ein Serialisierer, der ein Feld handschriftlich
//     dazuschreibt, faellt hier auf, ohne dass irgendwer seinen Namen kennt.
// =================================================================================================
TEST(D54TraceSchema, LeserSeiteFindetGenauDieSchemaMenge) {
    std::vector<std::string> schema;
    for (auto const& f : ts::kLatenzFelder) schema.emplace_back(f.name);
    std::sort(schema.begin(), schema.end());

    std::size_t geprueft = 0;
    for (auto const& [gattung, js] : alle_json_ausgaben()) {
        auto gefunden = perzentil_schluessel(js);
        std::sort(gefunden.begin(), gefunden.end());

        std::vector<std::string> waisen;      // in der Ausgabe, nicht im Schema
        std::vector<std::string> fehlstellen; // im Schema, nicht in der Ausgabe
        std::set_difference(gefunden.begin(), gefunden.end(), schema.begin(), schema.end(), std::back_inserter(waisen));
        std::set_difference(schema.begin(), schema.end(), gefunden.begin(), gefunden.end(),
                            std::back_inserter(fehlstellen));
        for (auto const& w : waisen) std::cout << "[D5-4]   WAISE " << gattung << ": " << w << "\n";
        for (auto const& f : fehlstellen) std::cout << "[D5-4]   FEHLSTELLE " << gattung << ": " << f << "\n";
        EXPECT_TRUE(waisen.empty()) << gattung << ": Feld in der Ausgabe, das das Schema nicht kennt";
        EXPECT_TRUE(fehlstellen.empty()) << gattung << ": Schema-Feld, das die Ausgabe nicht schreibt";
        ++geprueft;
    }
    std::cout << "[D5-4] Leser-Seite: " << geprueft << " Ausgaben gegen " << schema.size()
              << " Schema-Felder abgeglichen (beide Richtungen)\n";
}

// =================================================================================================
// (4) Die CSV traegt KEINE Perzentil-Spalte. Sonst gaebe es ein zweites Schema neben dem JSON --
//     und genau ein zweites Schema ist der Defekt, den dieses Paket schliesst.
// =================================================================================================
TEST(D54TraceSchema, CsvTraegtKeineZweitePerzentilListe) {
    std::size_t spalten_gesamt = 0;
    for (auto const& [gattung, csv] : alle_csv_ausgaben()) {
        std::size_t const nl = csv.find('\n');
        ASSERT_NE(nl, std::string::npos) << gattung << ": CSV ohne Kopfzeile";
        std::string const kopf = csv.substr(0, nl);
        EXPECT_EQ(kopf.rfind(std::string(ts::kGeteilterCsvKopf), 0), 0u)
            << gattung << ": CSV beginnt nicht mit dem geteilten Zeit-Kopf";
        std::string spalte;
        for (char c : kopf + ",") {
            if (c == ',') {
                ++spalten_gesamt;
                EXPECT_FALSE(ist_perzentil_name(spalte))
                    << gattung << ": CSV-Spalte '" << spalte << "' ist ein Perzentilfeld";
                spalte.clear();
            } else {
                spalte.push_back(c);
            }
        }
    }
    std::cout << "[D5-4] CSV-Kopfspalten geprueft: " << spalten_gesamt << " (Nenner)\n";
}

// =================================================================================================
// (5) Der KANON gegen das ganzzahlige Orakel, auf GERADEN Zufallslaengen. Seed wird gedruckt.
//     Nicht der Serialisierer wird hier geprueft, sondern die Rechnung, die er ruft -- und zwar mit
//     einer Mechanik, die keinen Codepfad mit ihm teilt.
// =================================================================================================
TEST(D54TraceSchema, KanonGegenGanzzahlOrakelAufGeradenLaengen) {
    std::cout << "[D5-4] Seed: " << kSeed << "\n";
    std::mt19937_64                             rng(kSeed);
    std::uniform_int_distribution<std::int64_t> dist(0, 100000);
    std::vector<std::size_t> const              laengen{2, 4, 6, 10, 20, 50, 100, 200, 500, 1000};
    std::size_t                                 faelle = 0;
    for (std::size_t runde = 0; runde < 12; ++runde) {
        for (std::size_t n : laengen) {
            ASSERT_EQ(n % 2, 0u) << "nur GERADE Laengen -- der divergierende Bereich (T-4)";
            std::vector<std::int64_t> probe(n);
            for (auto& x : probe) x = dist(rng);
            for (auto const& q : kQuantile) {
                ++faelle;
                ASSERT_EQ(::comdare::cache_engine::builder::commands::stats::percentile_ns(probe, q.als_double).count(),
                          orakel(probe, q))
                    << "Seed " << kSeed << " Runde " << runde << " n=" << n << " q=" << q.zaehler << '/' << q.nenner;
            }
        }
    }
    std::cout << "[D5-4] Orakel-Faelle geprueft: " << faelle << " (nur gerade n)\n";
    EXPECT_EQ(faelle, 12u * 10u * 3u);
}
