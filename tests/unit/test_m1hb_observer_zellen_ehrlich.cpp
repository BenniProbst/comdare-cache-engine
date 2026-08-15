// test_m1hb_observer_zellen_ehrlich.cpp -- M-1/H-B BISS (06.08.2026).
//
// DIE ZU BEWEISENDE AUSSAGE, in einem Satz: EINE MESS-ZEILE OHNE OBSERVER-AUSSTATTUNG SCHREIBT IN DIE
// OBSERVER-SPALTEN "n/a" UND NIEMALS 0 -- UND LAESST IHRE GUELTIGEN WALL-CLOCK-SPALTEN UNANGETASTET.
//
// WARUM DIESER TEST EXISTIERT (Befund, am Objekt gemessen -- beide echten .so ueber dlopen, gleicher
// Treiber, gleicher Lauf):
//     [all]        IObservableTier=JA  insert_ok=256 lookup_ok=256  observable_axes=9  fill_level=256
//     [wallclock]  IObservableTier=JA  insert_ok=256 lookup_ok=256  observable_axes=0  fill_level=0
// tier_observe() hat seinen KOMPLETTEN Rumpf unter #ifdef COMDARE_CE_ENABLE_STATISTICS
// (anatomy/abi_adapter.hpp) und liefert ohne dieses Gate einen LEEREN Snapshot. Das Tier haelt in
// BEIDEN Faellen real 256 Eintraege -- und meldete unter [wallclock] fill_level=0.
//
// DER HEBEL, DER DARAUS EINE LUEGE IN DEN MESSDATEN MACHTE: perm_runner setzt unified_real
// BEDINGUNGSLOS auf true, und eine [wallclock]-Binary TRAEGT das Mess-Interface. Die vorhandene,
// ehrliche n/a-Alternative war fuer genau diesen neuen Fall unerreichbar. Zusaetzlich standen 13
// Observer-Zellen als einzige des Blocks voellig UNGESCHUETZT da -- sie schrieben Zahlen auch dann,
// wenn unified_real bereits false war. Zwei davon, tier_fill_level und observable_axis_count, sind
// nicht einmal Mess-Zustand (axis_operability_classification.hpp: "passive Build-/Compile-Konstante").
//
// EINORDNUNG, ehrlich: der Zustand war vor M-1/D-1 NICHT erreichbar -- es war keine Tier-Binary ohne
// STATISTICS baubar. Die D-1-Scheibe hat die Gefahr ERZEUGT; diese Wache schliesst sie.
//
// WAS DIESER TEST BEWUSST NICHT TUT: er behauptet nichts ueber die Wall-Clock-Spalten einer
// [wallclock]-Zeile ausser dass sie UNBERUEHRT bleiben. Eine [wallclock]-Messung ist gueltig; sie hat
// nur keinen Observer. Ein zeilenweiter Ersatz (zell_ersatz) waere deshalb falsch -- die Ehrlichkeit
// muss zellgenau sein, und genau das wird hier nachgewiesen.
//
// B2-ERWEITERUNG (15.08.2026), Abschnitt (h): DIESELBE EHRLICHKEIT EINE SCHICHT HOEHER. Seit der
// Gate-Trennung G2(macro)/G3(micro) ist eine [wallclock,macro]-Binary baubar: Observer AN,
// Segment-Timer NICHT einkompiliert -- ihre seg_ns waeren durchgehend 0 bei batches_measured == 0,
// von einer echten Messung mit Ergebnis 0 nicht unterscheidbar. Die seg_*-Zellen haengen deshalb ab
// B2 zusaetzlich an row.seg_real (cfg.mess_feinkorn_ausstattung, Quelle
// live_mess_feinkorn_ausstattung); die 13 Observer-Zellen bleiben real. Der (h)-Block ist die
// Koeder-Haelfte der B2-Scheibe auf der CSV-Seite: am Vor-B2-Stand existiert row.seg_real nicht --
// diese Datei kompiliert dort NICHT (lauter Rot-Nachweis), die Haelften (a)-(g) sind unveraendert.
//
// A2.5-ERWEITERUNG (15.08.2026), Abschnitt (i): T17-SCHLIESSUNG DERSELBEN KLASSE. Die 5
// stat_persistence_target-Zellen (T17) und der T17-Beitrag zu v3_filled_axes entstehen ausschliesslich
// im Segment-Lauf (fill_observer_pathb_driven_v3, seit B2 unter G3) -- sie haengen im Renderer deshalb
// am selben seg_echt wie die seg_*-Zellen; alle uebrigen stat_*-Zellen bleiben zellgenau am Observer.
//
// Build: Standalone int main() (kein gtest), reiner Host-Pfad -- kein Tier, keine Achsen-Instanz.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>
#include <cache_engine/measurement/axis_error.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace cem = ::comdare::cache_engine::measurement;

namespace {

int g_fail = 0;

void tr(std::string const& what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

[[nodiscard]] std::vector<std::string> split_semicolon(std::string const& s) {
    std::vector<std::string> out;
    std::string              cur;
    for (char const c : s) {
        if (c == ';') {
            out.push_back(cur);
            cur.clear();
        } else if (c != '\n' && c != '\r') {
            cur.push_back(c);
        }
    }
    out.push_back(cur);
    return out;
}

[[nodiscard]] std::size_t column_index(std::vector<std::string> const& header, std::string_view name) {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return i;
    return header.size();
}

[[nodiscard]] std::string cell(std::vector<std::string> const& header, std::vector<std::string> const& cells,
                               std::string_view name) {
    std::size_t const i = column_index(header, name);
    if (i >= header.size() || i >= cells.size()) return {};
    return cells[i];
}

/// DIE 13 ZELLEN, die bis M-1/H-B ungeschuetzt Zahlen schrieben. Namentlich aufgezaehlt und nicht
/// positional gesucht -- eine Spaltenverschiebung darf diesen Test nicht still an anderen Zellen
/// vorbeifuehren.
constexpr char const* kObserverSpalten[] = {"search_lookup", "hit",         "miss",         "insert",    "erase",
                                            "peak",          "bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt",
                                            "fail",          "obs_axes",    "fill"};

/// Eine Mess-Zeile mit DURCHGEHEND NICHT-NULL Observer-Werten. Nicht-Null ist tragend: mit einem
/// genullten Snapshot koennte dieser Test die Luege gar nicht sehen, die er sucht (0 vs "n/a").
/// snapshot_gefuellt=false bildet die PRODUKTIONS-Form einer [wallclock]-Zeile nach: tier_observe hat
/// seinen Rumpf unter dem STATISTICS-Gate und schreibt dann NUR `*out = ComdareTierObserverSnapshot{}`
/// -- der Snapshot ist durchgehend GENULLT. Genau in dieser Form ist die Luege sichtbar (0 statt n/a);
/// mit einem nicht-genullten Probe-Snapshot koennte man sie nicht von einer echten Messung trennen.
[[nodiscard]] ex::LazyMeasuredRow probe_row(bool observer_ausstattung, bool snapshot_gefuellt,
                                            bool feinkorn_ausstattung = true) {
    ex::LazyMeasuredRow row;
    row.binary_id    = "m1hb_probe";
    row.setting_id   = row.binary_id;
    row.unified_real = observer_ausstattung;
    // B2: das FEINKORN (G3) dieser Zeile. Default true == Identitaet (jede Bestands-Zeile wie vor B2);
    // false bildet die [wallclock,macro]-Form nach: Observer real, seg_* ehrlich n/a.
    row.seg_real  = feinkorn_ausstattung;
    row.total_ns  = 1'000'000; // WALL-CLOCK: in BEIDEN Faellen gueltig, muss unberuehrt bleiben
    row.n_ops     = 1000;
    row.timed_ops = 2000;
    for (std::size_t k = 0; k < row.op_lat.size(); ++k) {
        row.op_lat[k].n       = 1000 + k;
        row.op_lat[k].p50_ns  = 100 + static_cast<std::int64_t>(k);
        row.op_lat[k].p99_ns  = 900 + static_cast<std::int64_t>(k);
        row.op_lat[k].p999_ns = 9000 + static_cast<std::int64_t>(k);
    }
    // Der Observer-Snapshot, wie ihn eine STATISTICS-Binary liefert -- alle 13 Groessen nicht-null.
    if (!snapshot_gefuellt) return row; // genullt == die reale [wallclock]-Form
    // B2: die seg-Quellen des EINEN konsolidierten PODs, nicht-null (mit Nullen koennte (h) die
    // 0-vs-n/a-Luege nicht sehen -- dieselbe Begruendung wie bei den 13 Observer-Groessen).
    for (std::size_t t = 0; t < std::size(row.unified.seg_ns); ++t)
        row.unified.seg_ns[t] = 1000 + static_cast<std::int64_t>(t);
    row.unified.seg_framework_ns = 555;
    row.unified.seg_run_total_ns = 44444;
    // B2-A2.5/F1: die per-Achsen-Observer-Quellen des EINEN PODs, nicht-null -- inklusive der NUR-
    // Pfad-B-getriebenen T17-Zeile UND einer filled_axes-Zahl, die T17 mitzaehlt (18). Mit Nullen
    // koennte (i) die 0-vs-n/a-Luege der stat_*-Zellen nicht sehen (dieselbe Begruendung wie oben);
    // die 18 im MACRO-Fall ist der adversariale Koeder fuer den deklarationsgetriebenen Deckel.
    for (std::size_t t = 0; t < std::size(row.unified.axis_stats); ++t)
        for (std::size_t f = 0; f < std::size(row.unified.axis_stats[t]); ++f)
            row.unified.axis_stats[t][f] = 100 * (t + 1) + f + 1;
    row.unified.filled_axis_count         = std::size(row.unified.axis_stats); // 18: behauptet ALLE inkl. T17
    row.observer.search_lookup_count      = 4711;
    row.observer.search_hit_count         = 4001;
    row.observer.search_miss_count        = 710;
    row.observer.search_insert_count      = 256;
    row.observer.search_erase_count       = 17;
    row.observer.search_peak_occupancy    = 256;
    row.observer.alloc_bytes_allocated    = 65536;
    row.observer.alloc_bytes_in_use       = 32768;
    row.observer.alloc_allocation_count   = 128;
    row.observer.alloc_deallocation_count = 64;
    row.observer.alloc_failure_count      = 3;
    row.observer.observable_axis_count    = 9;   // gemessen: [all] liefert 9, [wallclock] liefert 0
    row.observer.tier_fill_level          = 256; // gemessen: das Tier haelt 256 Eintraege, IMMER
    return row;
}

} // namespace

int main() {
    std::cout << "== M-1/H-B BISS: Observer-Zellen sind n/a statt 0, wenn keine Observer-Ausstattung da ist ==\n";

    std::vector<std::string> const header = split_semicolon(ex::lazy_csv_header());
    // MIT  = eine echte Observer-Messung (Snapshot gefuellt, Ausstattung da).
    // OHNE = die PRODUKTIONS-Form der [wallclock]-Zeile: Ausstattung fehlt UND der Snapshot ist genullt.
    //        Genau hier schrieb der Renderer bis M-1/H-B literal 0.
    // OHNE_GEFUELLT = die Kontrolle, dass die Wache an der AUSSTATTUNG haengt und nicht an den Werten.
    std::vector<std::string> const mit           = split_semicolon(ex::format_csv_row(probe_row(true, true)));
    std::vector<std::string> const ohne          = split_semicolon(ex::format_csv_row(probe_row(false, false)));
    std::vector<std::string> const ohne_gefuellt = split_semicolon(ex::format_csv_row(probe_row(false, true)));

    std::string const na{cem::sample_status_token(cem::SampleStatus::SourceUnavailable)};
    std::cout << "  n/a-Token der EINEN D2-Taxonomie: '" << na << "'\n";
    std::cout << "  Spalten im Header = " << header.size() << "  Zellen MIT = " << mit.size()
              << "  Zellen OHNE = " << ohne.size() << "\n";

    // ---- (a) STRUKTUR-NENNER. Ohne ihn koennte jede folgende Zell-Aussage an der falschen Spalte haengen.
    tr("(a) Header und BEIDE Zeilen haben dieselbe Zellenzahl (keine Spaltenverschiebung)",
       header.size() == mit.size() && header.size() == ohne.size());

    // ---- (b) NENNER + GEGENPROBE: findet dieses Verfahren die 13 Spalten ueberhaupt? Eine nackte Null
    //          waere kein Befund -- ein Tippfehler im Spaltennamen liefe sonst als "alles n/a" gruen durch.
    std::size_t gefunden = 0;
    for (char const* name : kObserverSpalten)
        if (column_index(header, name) < header.size()) ++gefunden;
    std::cout << "  Observer-Spalten im Header gefunden: " << gefunden << " von 13\n";
    tr("(b) NENNER: alle 13 benannten Observer-Spalten existieren im Header", gefunden == 13);

    // ---- (c) DIE GEGENPROBE ZUERST: mit Ausstattung tragen alle 13 ihre ZAHLEN. Ohne diesen Fall waere
    //          ein Renderer, der IMMER "n/a" schreibt, ebenfalls gruen -- und das waere die schwerere
    //          Regression (jede Bestands-Messung entwertet).
    std::size_t zahlen = 0;
    for (char const* name : kObserverSpalten) {
        std::string const v = cell(header, mit, name);
        if (!v.empty() && v != na && v != "0") ++zahlen;
    }
    std::cout << "  MIT Ausstattung: Spalten mit einer echten Zahl (!= n/a, != 0): " << zahlen << " von 13\n";
    tr("(c) GEGENPROBE: MIT Observer-Ausstattung tragen alle 13 Spalten ihre Zahlen", zahlen == 13);
    tr("(c) namentlich: fill (tier_fill_level) == 256 (das real gefuellte Tier)", cell(header, mit, "fill") == "256");
    tr("(c) namentlich: obs_axes (observable_axis_count) == 9", cell(header, mit, "obs_axes") == "9");

    // ---- (d) DIE KERN-AUSSAGE: ohne Ausstattung ist JEDE der 13 Spalten "n/a" -- und keine ist "0".
    std::size_t na_zellen  = 0;
    std::size_t null_luege = 0;
    for (char const* name : kObserverSpalten) {
        std::string const v = cell(header, ohne, name);
        if (v == na) ++na_zellen;
        if (v == "0") {
            std::cout << "  [ERR] LUEGE: Spalte '" << name << "' schreibt 0 statt " << na << "\n";
            ++null_luege;
        }
    }
    std::cout << "  OHNE Ausstattung: n/a-Zellen = " << na_zellen << " von 13, Null-Luegen = " << null_luege << "\n";
    tr("(d) OHNE Observer-Ausstattung sind alle 13 Spalten n/a", na_zellen == 13);
    tr("(d) KEINE der 13 Spalten schreibt eine stille 0", null_luege == 0);
    tr("(d) namentlich: fill (tier_fill_level) ist n/a -- NICHT 0, obwohl das Tier real gefuellt ist "
       "(das ist der gemessene Kern des Befundes)",
       cell(header, ohne, "fill") == na);
    tr("(d) namentlich: obs_axes (observable_axis_count) ist n/a -- die Spalte ist gar kein Mess-Zustand",
       cell(header, ohne, "obs_axes") == na);

    // ---- (e) ZELLGENAUIGKEIT: die gueltigen Wall-Clock-Spalten bleiben UNBERUEHRT. Ein zeilenweiter
    //          Ersatz waere die naheliegende, aber falsche Heilung -- eine [wallclock]-Messung IST gueltig.
    for (char const* name : {"total_ns", "n_ops", "ns_per_op"}) {
        std::string const a = cell(header, mit, name);
        std::string const b = cell(header, ohne, name);
        std::cout << "    " << name << ": MIT='" << a << "'  OHNE='" << b << "'\n";
        tr(std::string{"(e) "} + name + " ist in beiden Zeilen identisch (kein zeilenweiter Ersatz)",
           !a.empty() && a == b);
        tr(std::string{"(e) "} + name + " ist KEIN n/a (die Wall-Clock-Messung bleibt gueltig)", b != na);
    }

    // ---- (f) BYTE-BILANZ: die beiden Zeilen unterscheiden sich UEBERHAUPT. Wuerden sie identisch
    //          gerendert, waere (d) trivial gruen und der Test wertlos.
    tr("(f) die beiden Zeilen sind verschieden (der Test vergleicht wirklich zwei Zustaende)", mit != ohne);

    // ---- (g) DIE WACHE HAENGT AN DER AUSSTATTUNG, NICHT AN DEN WERTEN. Eine Zeile ohne Ausstattung,
    //          deren Snapshot zufaellig Zahlen traegt, muss GENAUSO n/a rendern -- sonst waere die
    //          Ehrlichkeit eine Funktion des Zufalls statt der Deklaration.
    std::size_t na_gefuellt = 0;
    for (char const* name : kObserverSpalten)
        if (cell(header, ohne_gefuellt, name) == na) ++na_gefuellt;
    std::cout << "  OHNE Ausstattung, aber Snapshot GEFUELLT: n/a-Zellen = " << na_gefuellt << " von 13\n";
    tr("(g) auch mit gefuelltem Snapshot rendert eine Zeile ohne Ausstattung n/a (die Wache haengt an "
       "der Deklaration, nicht an den Zahlen)",
       na_gefuellt == 13);

    // ---- (h) B2 (15.08.2026): DIESELBE EHRLICHKEIT EINE SCHICHT HOEHER -- die seg_*-Zellen haengen am
    //          FEINKORN (G3), nicht nur am Observer (G2). Die MACRO-Zeile bildet die seit B2 baubare
    //          [wallclock,macro]-Binary nach: Observer real, Segment-Timer nicht einkompiliert.
    {
        std::vector<std::string> seg_spalten;
        for (auto const& name : header)
            if (name.rfind("seg_", 0) == 0) seg_spalten.push_back(name);
        std::size_t const seg_soll = ex::kCompositionAxisNames.size() + 3; // 18 seg_<achse>_ns + fw/total/coverage
        std::cout << "  (h) seg-Spalten im Header gefunden: " << seg_spalten.size() << " von " << seg_soll << "\n";
        tr("(h) NENNER: alle seg-Spalten existieren im Header (18 Achsen + framework/run_total/coverage)",
           seg_spalten.size() == seg_soll);

        std::vector<std::string> const macro = split_semicolon(ex::format_csv_row(probe_row(true, true, false)));
        tr("(h) STRUKTUR: die MACRO-Zeile hat dieselbe Zellenzahl wie der Header", header.size() == macro.size());

        // GEGENPROBE ZUERST: die MIT-Zeile (Feinkorn da, default true) traegt in JEDER seg-Zelle eine echte
        // Zahl -- ein Renderer, der seg immer n/a schriebe, waere sonst gruen.
        std::size_t seg_zahlen = 0;
        for (auto const& name : seg_spalten) {
            std::string const v = cell(header, mit, name);
            if (!v.empty() && v != na && v != "0") ++seg_zahlen;
        }
        std::cout << "  (h) MIT Feinkorn: seg-Zellen mit echter Zahl (!= n/a, != 0): " << seg_zahlen << " von "
                  << seg_spalten.size() << "\n";
        tr("(h) GEGENPROBE: MIT Feinkorn tragen alle seg-Zellen ihre Zahlen", seg_zahlen == seg_spalten.size());

        // KERN: OHNE Feinkorn ist JEDE seg-Zelle n/a und KEINE "0" -- obwohl der Snapshot gefuellt ist
        // (die Wache haengt an der Deklaration, nicht an den Zahlen; wie (g) eine Schicht tiefer).
        std::size_t seg_na = 0, seg_null_luege = 0;
        for (auto const& name : seg_spalten) {
            std::string const v = cell(header, macro, name);
            if (v == na) ++seg_na;
            if (v == "0") {
                std::cout << "  [ERR] LUEGE: seg-Spalte '" << name << "' schreibt 0 statt " << na << "\n";
                ++seg_null_luege;
            }
        }
        std::cout << "  (h) OHNE Feinkorn (MACRO-Zeile): n/a-Zellen = " << seg_na << " von " << seg_spalten.size()
                  << ", Null-Luegen = " << seg_null_luege << "\n";
        tr("(h) KERN: OHNE Feinkorn sind ALLE seg-Zellen n/a", seg_na == seg_spalten.size());
        tr("(h) KEINE seg-Zelle schreibt eine stille 0", seg_null_luege == 0);

        // ZELLGENAUIGKEIT eine Schicht hoeher: die 13 Observer-Zellen der MACRO-Zeile bleiben ECHT und
        // byte-gleich zur MIT-Zeile -- der Feinkorn-Entzug darf den Observer-Block nicht abwerten.
        std::size_t obs_gleich = 0;
        for (char const* name : kObserverSpalten)
            if (!cell(header, macro, name).empty() && cell(header, macro, name) == cell(header, mit, name))
                ++obs_gleich;
        std::cout << "  (h) Observer-Zellen MACRO == MIT: " << obs_gleich << " von 13\n";
        tr("(h) ZELLGENAU: alle 13 Observer-Zellen der MACRO-Zeile bleiben echte Zahlen (== MIT-Zeile)",
           obs_gleich == 13);
        // NIE AUFWERTEND: Feinkorn true ohne Observer-Ausstattung rendert seg weiterhin n/a (Konjunktion).
        std::vector<std::string> const ohne_obs = split_semicolon(ex::format_csv_row(probe_row(false, true, true)));
        std::size_t                    seg_na_ohne_obs = 0;
        for (auto const& name : seg_spalten)
            if (cell(header, ohne_obs, name) == na) ++seg_na_ohne_obs;
        std::cout << "  (h) OHNE Observer, Feinkorn true: seg-n/a = " << seg_na_ohne_obs << " von "
                  << seg_spalten.size() << "\n";
        tr("(h) seg_real kann NIE aufwerten: ohne Observer bleiben alle seg-Zellen n/a",
           seg_na_ohne_obs == seg_spalten.size());
    }

    // ---- (i) B2-A2.5/F1 (15.08.2026): T17-EHRLICHKEIT AUCH IN DEN stat_*-ZELLEN. Die T17-Zaehler
    //          (persistence_target) ENTSTEHEN ausschliesslich im Segment-Lauf (fill_observer_pathb_driven_v3,
    //          seit B2 unter G3) -- die MACRO-Zeile ([wallclock,macro]) muss ihre 5 stat_persistence_target-
    //          Zellen deshalb als n/a rendern, NIE als 0, und v3_filled_axes darf T17 nicht ausweisen.
    //          Alle uebrigen stat_*-Zellen bleiben zellgenau echt (Feinkorn-Entzug wertet nur T17 ab).
    {
        constexpr char const* kT17Spalten[] = {"stat_persistence_target_rounds", "stat_persistence_target_bytes_staged",
                                               "stat_persistence_target_records_staged",
                                               "stat_persistence_target_device_flushes",
                                               "stat_persistence_target_checksum"};
        std::vector<std::string> const macro = split_semicolon(ex::format_csv_row(probe_row(true, true, false)));

        // NENNER: die 5 T17-Spalten existieren im Header (sonst liefe jede Aussage an der falschen Zelle).
        std::size_t t17_gefunden = 0;
        for (char const* name : kT17Spalten)
            if (column_index(header, name) < header.size()) ++t17_gefunden;
        std::cout << "  (i) T17-stat-Spalten im Header gefunden: " << t17_gefunden << " von 5\n";
        tr("(i) NENNER: alle 5 stat_persistence_target-Spalten existieren im Header", t17_gefunden == 5);

        // GEGENPROBE ZUERST: MIT Feinkorn tragen die 5 T17-Zellen echte Zahlen -- ein Renderer, der T17
        // immer n/a schriebe, waere sonst gruen (und jede [all]-Bestandsmessung entwertet).
        std::size_t t17_zahlen = 0;
        for (char const* name : kT17Spalten) {
            std::string const v = cell(header, mit, name);
            if (!v.empty() && v != na && v != "0") ++t17_zahlen;
        }
        std::cout << "  (i) MIT Feinkorn: T17-Zellen mit echter Zahl (!= n/a, != 0): " << t17_zahlen << " von 5\n";
        tr("(i) GEGENPROBE: MIT Feinkorn tragen alle 5 T17-Zellen ihre Zahlen", t17_zahlen == 5);

        // KERN: OHNE Feinkorn (MACRO) ist JEDE der 5 T17-Zellen n/a und KEINE "0" -- obwohl der Snapshot
        // Zahlen traegt (die Wache haengt an der Deklaration, nicht an den Zahlen; wie (g)/(h)).
        std::size_t t17_na = 0, t17_null_luege = 0;
        for (char const* name : kT17Spalten) {
            std::string const v = cell(header, macro, name);
            if (v == na) ++t17_na;
            if (v == "0") {
                std::cout << "  [ERR] LUEGE: T17-Spalte '" << name << "' schreibt 0 statt " << na << "\n";
                ++t17_null_luege;
            }
        }
        std::cout << "  (i) OHNE Feinkorn (MACRO): T17-n/a = " << t17_na << " von 5, Null-Luegen = " << t17_null_luege
                  << "\n";
        tr("(i) KERN: OHNE Feinkorn sind ALLE 5 T17-stat-Zellen n/a", t17_na == 5);
        tr("(i) KEINE T17-stat-Zelle schreibt eine stille 0", t17_null_luege == 0);

        // ZELLGENAU: die stat_*-Zellen ALLER Nicht-T17-Achsen der MACRO-Zeile bleiben echte Zahlen
        // (byte-gleich zur MIT-Zeile) -- der Feinkorn-Entzug darf nur T17 abwerten.
        std::size_t stat_spalten = 0, stat_gleich = 0;
        for (auto const& name : header) {
            if (name.rfind("stat_", 0) != 0) continue;
            bool ist_t17 = false;
            for (char const* t17name : kT17Spalten)
                if (name == t17name) ist_t17 = true;
            if (ist_t17) continue;
            ++stat_spalten;
            if (!cell(header, macro, name).empty() && cell(header, macro, name) == cell(header, mit, name))
                ++stat_gleich;
        }
        std::cout << "  (i) Nicht-T17-stat-Zellen MACRO == MIT: " << stat_gleich << " von " << stat_spalten << "\n";
        tr("(i) ZELLGENAU: alle Nicht-T17-stat-Zellen der MACRO-Zeile bleiben echte Zahlen (== MIT-Zeile)",
           stat_spalten > 0 && stat_gleich == stat_spalten);

        // v3_filled_axes: die MIT-Zeile weist alle 18 aus (inkl. T17); die MACRO-Zeile darf T17 nicht
        // ausweisen -- deklarationsgetrieben gedeckelt auf 17, obwohl der Snapshot 18 behauptet.
        std::cout << "  (i) v3_filled_axes: MIT='" << cell(header, mit, "v3_filled_axes") << "' MACRO='"
                  << cell(header, macro, "v3_filled_axes") << "'\n";
        tr("(i) v3_filled_axes MIT Feinkorn == 18 (T17 zaehlt mit)", cell(header, mit, "v3_filled_axes") == "18");
        tr("(i) v3_filled_axes OHNE Feinkorn == 17 (T17 nicht ausgewiesen; Deckel an der Deklaration)",
           cell(header, macro, "v3_filled_axes") == "17");

        // NIE AUFWERTEND: ohne Observer bleiben auch T17-Zellen und v3_filled_axes n/a (Konjunktion).
        std::vector<std::string> const ohne_obs2 = split_semicolon(ex::format_csv_row(probe_row(false, true, true)));
        std::size_t                    t17_na_ohne_obs = 0;
        for (char const* name : kT17Spalten)
            if (cell(header, ohne_obs2, name) == na) ++t17_na_ohne_obs;
        std::cout << "  (i) OHNE Observer, Feinkorn true: T17-n/a = " << t17_na_ohne_obs << " von 5\n";
        tr("(i) seg_real kann NIE aufwerten: ohne Observer sind T17-Zellen und v3_filled_axes n/a",
           t17_na_ohne_obs == 5 && cell(header, ohne_obs2, "v3_filled_axes") == na);
    }

    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
