#pragma once
// D14 / L-CLUSTER (gate-frei, 2026-06-02) — result_ingest: Rückführung der Cluster-/perm_runner-Mess-Ergebnisse
// in den Experiment-Baum. Eine Ergebnis-Zeile (binary_id + 13 Observer-Felder) wird geparst und als gemessener
// NodeValue (sparse) via tree.set_node_value eingespielt — der Host-seitige Empfangs-Pfad (Doc 28 §5 „Ergebnis-
// Rückführung in den Baum-NodeValue"). LOKAL verifizierbar (kein Cluster-Gate); die echte sbatch-Submission +
// der Webhook-Receiver sind GATE-MAXIMAL (getrennt). Self-contained (kein JSON-Lib), C++23.
//
// Zeilenformat (semikolon-getrennt, = NodeObserverSnapshot-Reihenfolge): EIN Feld binary_id + 13 uint64:
//   search_lookup;hit;miss;insert;erase;peak | alloc_bytes_alloc;bytes_in_use;alloc_cnt;dealloc_cnt;fail | observable_axes;fill

#include "experiment_tree.hpp"

#include <charconv>
#include <iostream> // Q-9-Auflage: Log-Zeile je verworfener Feldzahl (kein stiller Verwurf)
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// STRUKT-R ORG-18 / Owner-Entscheid 26.07.2026 -- LESE-SEITIGE Alt-id-Normalisierung.
///
/// PROBLEM: der 18. Kompositions-Slot persistence_target haengt an JEDE neue binary_id das Segment
/// "/persistence_target=<name>". Eine Ergebnis-Zeile im ALT-Format (160 Felder, Q-9) traegt per Konstruktion
/// eine 17-Segment-id und wuerde sonst keinen Baum-Knoten mehr treffen. Diese Funktion ist damit das
/// id-seitige Gegenstueck zur feld-seitigen Q-9-Regel: ohne sie waere die Laengen-Akzeptanz wirkungslos,
/// weil die Zeile zwar geparst, aber auf keinen Knoten geschrieben wuerde.
///
/// ABGRENZUNG (live geprueft 2026-07-26, NICHT der Anwendungsfall): tests/unit/thesis_tiere/
/// tier150_measurements.csv liegt in einem ANDEREN Format (Builder-Iterator-CSV
/// "binary_id;setting;repetition;n_ops;total_ns;ns_per_op;seg_*", 134 Spalten) und wird von
/// heuristik/measurement_curve_loader.hpp gelesen, NICHT von result_ingest. Ihre 5760 ids sind
/// 19-segmentig (sie tragen noch telemetry= und isa=), also VOR-INC-2c/2d -- sie waren durch die beiden
/// frueheren ABI-Bruecke bereits nicht mehr deckungsgleich, lange vor ORG-18. Der Curve-Loader nutzt die
/// binary_id ausserdem nur als GRUPPIERUNGS-Dimension (variant_col), nicht als Baum-Schluessel. ORG-18
/// fuegt dort also KEINE neue Regression hinzu.
///
/// ENTSCHEID: die CSV-DATEIEN bleiben BYTE-UNVERAENDERT (Messdaten-Doktrin "nie aendern, nie loeschen");
/// normalisiert wird ausschliesslich beim LESEN. Das ist inhaltlich korrekt und keine Erfindung: zur
/// Erhebungszeit existierte nur ein Rueckschreib-Ziel, jede Alt-Messung WAR memory_only. Dieselbe Logik
/// wie die Q-9-Feldzahl-Regel (160 impliziert memory_only), nur auf id-Ebene.
///
/// POSITION: das Segment wird NICHT angehaengt, sondern direkt HINTER "queuing_q2=<wert>" eingeschoben --
/// genau dort emittiert es serialize_composition_path. Ein blindes Anhaengen waere bei ids mit
/// Shape-Segment (with_shape_segment haengt NACH der Komposition an) falsch sortiert und wuerde einen
/// Pfad erzeugen, den der Baum nie bildet.
///
/// IDEMPOTENT: eine id, die das Segment schon traegt, kommt unveraendert zurueck.
[[nodiscard]] inline std::string normalize_legacy_binary_id(std::string_view id) {
    constexpr std::string_view kAxis    = "persistence_target=";
    constexpr std::string_view kSegment = "/persistence_target=persistence_memory_only";
    constexpr std::string_view kAnchor  = "queuing_q2=";
    if (id.find(kAxis) != std::string_view::npos) return std::string{id}; // schon 18-Slot-Form
    std::size_t const a = id.find(kAnchor);
    if (a == std::string_view::npos) return std::string{id}; // keine SA-Kompositions-id -> unberuehrt
    std::size_t const seg_end = id.find('/', a + kAnchor.size());
    std::string       out{id.substr(0, seg_end == std::string_view::npos ? id.size() : seg_end)};
    out += kSegment;
    if (seg_end != std::string_view::npos) out += id.substr(seg_end); // Shape-Schwanz bleibt HINTEN
    return out;
}

/// #45 (paralleler Mess-Loop): die REINE Parse-Naht -- zerlegt `line` in binary_id + 159 Observer-Felder und baut den
/// NodeValue OHNE einen Baum zu beruehren. Rueckgabe: {binary_id, NodeValue} bei wohlgeformter Zeile (==160 Felder),
/// sonst std::nullopt. So kann der parallele Mess-Loop je Zelle worker-LOKAL parsen (kein geteilter ExperimentTree,
/// kein Lock); ingest_result_line ist nur noch die duenne Baum-schreibende Huelle darum (byte-identisch zum Ist).
[[nodiscard]] inline std::optional<std::pair<std::string, NodeValue>>
parse_result_line_to_node_value(std::string_view line) {
    if (line.empty() || line.front() == '#') return std::nullopt;
    std::vector<std::string_view> f;
    std::size_t                   i = 0;
    while (i <= line.size()) {
        std::size_t const sc  = line.find(';', i);
        std::size_t const end = (sc == std::string_view::npos) ? line.size() : sc;
        f.push_back(line.substr(i, end - i));
        if (sc == std::string_view::npos) break;
        i = sc + 1;
    }
    // KONSOLIDIERUNG (I-B.3, 2026-06-04; Bau-INC-2c 19->18): volle Matrix = binary_id + axis_stats[N][8]
    // + seg_ns[N] + Meta (observable_axis_count, tier_fill_level, filled_axis_count, batches_measured)
    // + P-MD3 (seg_framework_ns, seg_run_total_ns) = 1 + N*8 + N + 4 + 2 Felder.
    // MAJOR-MESS-09 (Audit A1): die Feldzahl wird EXAKT geprueft (nicht >=). Ein ';'/Newline in der binary_id
    // (oder ein angehaengtes/verschmolzenes Feld) erzeugt eine unerwartete Laenge -> die axis_stats waeren gegen
    // ihre Slots verschoben. format_perm_result lehnt verletzende IDs bereits ab; diese Schranke ist die
    // zweite, lese-seitige Verteidigung (kein stiller Slot-Versatz).
    //
    // STRUKT-R ORG-18 / Q-9 OWNER-BESTAETIGT 26.07.2026 (VERSIONIERTER READER, Direktive "Messdaten nie loeschen"): es werden ZWEI Laengen
    // akzeptiert, damit die vor dem 18-Achsen-Bruch erhobenen Mess-CSV LESBAR BLEIBEN statt still zu
    // verschwinden (der frueher harte `!= 160` haette jede Alt-Zeile per return nullopt unsichtbar verworfen):
    //   169 = 18 Achsen (aktuell, mit persistence_target)
    //   160 = 17 Achsen (Alt-Format vor STRUKT-R)
    // Eine 160er-Zeile IMPLIZIERT persistence_target=memory_only. Deren T17-Slots bleiben 0 -- und das ist
    // KEINE erfundene Null, sondern der korrekte Wert: MemoryOnlyTarget hat keinen Rueckschreib-Pfad, seine
    // Mess-Op liefert echte 0 und die Huelle stagt 0 Bytes/0 device_flushes. Alt- und Neu-Zeile sind an dieser
    // Stelle also inhaltsgleich, nicht bloss laengen-kompatibel.
    // A8-S1 (2026-08-04, Nachzug zu Befund B-6): kAxesCurrent aus dem ZIEL-Array abgeleitet, nicht als Literal.
    // result_ingest bleibt bewusst anatomy-unabhaengig (experiment_tree.hpp ist der einzige Include), deshalb
    // ist die tragende Konstante hier die Slot-Zahl von NodeObserverSnapshot::seg_ns -- die test_a8s1_t17_-
    // vollzaehligkeit per static_assert gegen anatomy::kV3AxisCount bindet. Waechst die Achsen-Zahl, waechst
    // die akzeptierte Aktuell-Laenge automatisch mit, statt jede neue Zeile still als "unerwartete Feldzahl"
    // zu verwerfen. kAxesLegacy bleibt Literal: 17 ist ein eingefrorenes HISTORISCHES Format, keine Variable.
    constexpr std::size_t kAxesCurrent   = sizeof(NodeObserverSnapshot{}.seg_ns) / sizeof(std::int64_t);
    constexpr std::size_t kAxesLegacy    = 17; // vor STRUKT-R ORG-18 (eingefrorene Alt-Laenge, nie ableiten)
    constexpr std::size_t kFieldsPerAxis = sizeof(NodeObserverSnapshot{}.axis_stats[0]) / sizeof(std::uint64_t);
    auto const            fields_for     = [](std::size_t n) { return 1u + n * kFieldsPerAxis + n + 4u + 2u; };
    std::size_t           axis_count     = 0;
    if (f.size() == fields_for(kAxesCurrent))
        axis_count = kAxesCurrent;
    else if (f.size() == fields_for(kAxesLegacy))
        axis_count = kAxesLegacy;
    if (axis_count == 0 || f[0].empty()) {
        // Q-9-Auflage (Owner 26.07.2026): KEIN stiller Verwurf mehr. Der frueher blanke `return nullopt`
        // liess jede laengen-abweichende Zeile unsichtbar verschwinden -- genau der geruegte Zustand.
        // Eine Zeile hier ist die Ausnahme (wohlgeformte Laengen sind 169 und 160), kein Hot-Path-Spam.
        std::cerr << "[result_ingest] Zeile verworfen: unerwartete Feldzahl " << f.size() << " (erwartet "
                  << fields_for(kAxesCurrent) << " aktuell oder " << fields_for(kAxesLegacy) << " Alt-Format)"
                  << (f.empty() || f[0].empty() ? "; binary_id leer" : "") << "\n";
        return std::nullopt;
    }

    auto u64 = [](std::string_view s) -> std::uint64_t {
        std::uint64_t v = 0;
        std::from_chars(s.data(), s.data() + s.size(), v);
        return v;
    };
    auto i64 = [](std::string_view s) -> std::int64_t {
        std::int64_t v = 0;
        std::from_chars(s.data(), s.data() + s.size(), v);
        return v;
    };
    NodeObserverSnapshot o{};
    std::size_t          idx = 1;
    // Q-9: axis_count ist 18 (aktuell) oder 17 (Alt-CSV). Die Ziel-Arrays sind IMMER [18] -- bei einer
    // Alt-Zeile bleibt Slot 17 auf seiner Null-Initialisierung, was genau memory_only entspricht (s. oben).
    for (std::size_t t = 0; t < axis_count; ++t)
        for (std::size_t fi = 0; fi < kFieldsPerAxis; ++fi) o.axis_stats[t][fi] = u64(f[idx++]);
    for (std::size_t t = 0; t < axis_count; ++t) o.seg_ns[t] = i64(f[idx++]);
    o.observable_axis_count = u64(f[idx++]);
    o.tier_fill_level       = u64(f[idx++]);
    o.filled_axis_count     = u64(f[idx++]);
    o.batches_measured      = u64(f[idx++]);
    // P-MD3 (2026-06-18): die 2 additiven Coverage-Versöhnungs-Meta-Felder (Reihenfolge = format_perm_result).
    o.seg_framework_ns = i64(f[idx++]);
    o.seg_run_total_ns = i64(f[idx++]);
    // Legacy-V1-Projektion (Übergang, entfällt I-C): 13 benannte Felder = Sicht auf axis_stats[0] (search) /
    // axis_stats[6] (allocator) → die noch nicht migrierten 13-Feld-Konsumenten (CSV-o.*, test_d14b/d14c).
    o.search_lookup_count      = o.axis_stats[0][0];
    o.search_hit_count         = o.axis_stats[0][1];
    o.search_miss_count        = o.axis_stats[0][2];
    o.search_insert_count      = o.axis_stats[0][3];
    o.search_erase_count       = o.axis_stats[0][4];
    o.search_peak_occupancy    = o.axis_stats[0][5];
    o.alloc_bytes_allocated    = o.axis_stats[6][0];
    o.alloc_bytes_in_use       = o.axis_stats[6][1];
    o.alloc_allocation_count   = o.axis_stats[6][2];
    o.alloc_deallocation_count = o.axis_stats[6][3];
    o.alloc_failure_count      = o.axis_stats[6][4];

    NodeValue nv{};
    nv.has_result             = true;
    nv.observer               = o;
    nv.observer_real          = true; // aus echtem Cluster-Lauf (perm_runner tier_observe)
    nv.measured_setting_count = 1;
    // STRUKT-R ORG-18: Alt-ids (17 Segmente) werden hier auf die 18-Slot-Form normalisiert, damit
    // Mess-CSV von VOR dem Bruch weiter ihren Baum-Knoten treffen. Neue ids passieren unveraendert.
    return std::pair<std::string, NodeValue>{normalize_legacy_binary_id(f[0]), std::move(nv)};
}

/// Zerlegt `line` in binary_id + 159 Observer-Felder (';'-getrennt) und schreibt sie als gemessenen NodeValue in den
/// Baum. Rueckgabe: true bei wohlgeformter Zeile (==160 Felder). Leerzeilen/Kommentare ('#') -> false. Duenne Huelle um
/// parse_result_line_to_node_value (#45) -- byte-identisch zum Ist (Parse unveraendert, nur set_node_value angehaengt).
[[nodiscard]] inline bool ingest_result_line(ExperimentTree& tree, std::string_view line) {
    auto parsed = parse_result_line_to_node_value(line);
    if (!parsed) return false;
    tree.set_node_value(parsed->first, parsed->second);
    return true;
}

/// Ingest mehrerer Zeilen (z.B. eine Cluster-Ergebnis-Datei / Webhook-Batch). Liefert die Zahl eingespielter Knoten.
[[nodiscard]] inline std::size_t ingest_results(ExperimentTree& tree, std::string_view text) {
    std::size_t n = 0, i = 0;
    while (i <= text.size()) {
        std::size_t const nl   = text.find('\n', i);
        std::size_t const end  = (nl == std::string_view::npos) ? text.size() : nl;
        std::string_view  line = text.substr(i, end - i);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (ingest_result_line(tree, line)) ++n;
        if (nl == std::string_view::npos) break;
        i = nl + 1;
    }
    return n;
}

} // namespace comdare::cache_engine::builder::experiment
