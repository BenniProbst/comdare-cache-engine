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
// Build: Standalone int main() (kein gtest), reiner Host-Pfad -- kein Tier, keine Achsen-Instanz.

#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>
#include <cache_engine/measurement/axis_error.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
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
[[nodiscard]] ex::LazyMeasuredRow probe_row(bool observer_ausstattung, bool snapshot_gefuellt) {
    ex::LazyMeasuredRow row;
    row.binary_id    = "m1hb_probe";
    row.setting_id   = row.binary_id;
    row.unified_real = observer_ausstattung;
    row.total_ns     = 1'000'000; // WALL-CLOCK: in BEIDEN Faellen gueltig, muss unberuehrt bleiben
    row.n_ops        = 1000;
    row.timed_ops    = 2000;
    for (std::size_t k = 0; k < row.op_lat.size(); ++k) {
        row.op_lat[k].n       = 1000 + k;
        row.op_lat[k].p50_ns  = 100 + static_cast<std::int64_t>(k);
        row.op_lat[k].p99_ns  = 900 + static_cast<std::int64_t>(k);
        row.op_lat[k].p999_ns = 9000 + static_cast<std::int64_t>(k);
    }
    // Der Observer-Snapshot, wie ihn eine STATISTICS-Binary liefert -- alle 13 Groessen nicht-null.
    if (!snapshot_gefuellt) return row; // genullt == die reale [wallclock]-Form
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

    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
