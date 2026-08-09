#pragma once
// D5-4 (2026-08-09) -- tier_trace_schema: DIE EINE Feldliste der Tier-Trace-Ausgabe.
//
// @subsystem CEB (Mess-Serialisierung, neben latency_stats.hpp = der Perzentil-KANON)
// @phase_owner CEB
//
// WOZU ES DIESE DATEI GIBT (der Befund, der sie erzwingt -- Paket D5-4):
// Die p50/p95/p99-Feldblocke der Trace-JSON standen ZWEIMAL im Baum, Zeichen fuer Zeichen gleich:
//   * tier_observe_trace_abi.hpp, inline in serialize_abi_tier_trace_json (SearchAlgorithm-Gattung),
//   * genus_tier_observe_trace_abi.hpp, in genus_trace_detail::write_shared_json_fields (vier
//     Container-Gattungen, von dort 4x gerufen).
// Beide riefen bereits den richtigen KANON (stats::percentile_ns). Trotzdem war es eine ABSCHRIFT:
// als delete_p99_ns fehlte, fehlte es in BEIDEN, und keine Wache konnte das sehen -- die eine Seite
// wusste nichts von der anderen. Genau diese Fehlerklasse hat D5-1 schon einmal getroffen (der
// ersatzlos geloeschte nearest_rank_p ueberlebte als Kopie in einem anderen Repo): eine Loeschung ist
// eine Wache gegen AUFRUFER, nicht gegen KOPIEN.
//
// WAS SICH DADURCH STRUKTURELL AENDERT: die Feldliste ist nicht mehr Text in einem Serialisierer,
// sondern DATEN (kLatenzFelder). Der Serialisierer LAEUFT ueber diese Daten. Ein Feld kann damit nicht
// mehr auf einer Seite existieren und auf der anderen fehlen -- es gibt nur noch eine Seite.
//
// SELBSTCHECK DIESER DATEI
//   ZUSICHERT: (1) kLatenzFelder ist im PRODUKTIV-Baum (libs/) die einzige Stelle, an der die Namen und
//              Quantile der r/w/d-Perzentilfelder als CODE stehen. Gegenprobe am 2026-08-09 ueber das
//              ganze Repo (ohne .git/build): 13 Treffer hier, je 1 in den beiden Serialisierer-Dateien
//              -- und die beiden sind KOMMENTAR (Doku wird nie geloescht), kein Code. Die uebrigen
//              Treffer liegen in tests/: dort SOLLEN unabhaengige Erwartungen stehen, sonst prueft ein
//              Test nur sich selbst; (2) jeder Wert kommt aus stats::percentile_ns
//              (latency_stats.hpp), also aus dem D5-1-KANON k = ceil(q*n)-1 -- hier steht KEINE
//              Perzentil-Formel und KEINE Quantil->Index-Umrechnung; (3) die Zuordnung Feld -> Roh-Kurve
//              ist COMPILE-TIME aufgeloest (if constexpr ueber ein Nicht-Typ-Template-Argument), es gibt
//              keinen Runtime-Switch ueber die Kurven.
//   ZUSICHERT NICHT: nichts ueber die LESER ausserhalb dieses Repos. Die super-Werkzeuge
//              (Code/04_csv_to_latex, Code/05_diagram_generator) tragen eigene Feldlisten an einem
//              eigenen ce-Vendor-Stand -- das ist D5-2/D5-3. Nichts ueber die CSV-Spalten JENSEITS des
//              geteilten Zeit-Kopfs (die sind gattungs-eigen und gehoeren in die jeweilige Datei).
//              Nichts ueber die uebrigen Median-Bauarten (eta_kalibrierung::median_t_s mittelt bei
//              geradem n, HDR-Histogramm ist ein eigenes Verfahren) -- das ist D5-2/D5-5.
//
// REIHENFOLGE IST VERTRAG: kLatenzFelder ist die Reihenfolge im JSON. delete_p99_ns steht deshalb
// HINTEN (nach delete_p95_ns) und nicht etwa nach Symmetrie einsortiert -- die acht bisherigen Felder
// behalten ihre Position, damit ein positionsabhaengiger Leser nicht still verrutscht.

#include <builder/commands/latency_stats.hpp> // D5-1: der EINE Perzentil-Kanon (stats::percentile_ns)

#include <array>
#include <cstddef>
#include <sstream>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::anatomy_commands::trace_schema {

namespace st = ::comdare::cache_engine::builder::commands::stats;

/// Welche der drei Roh-ns-Kurven eines Fuellstands-Checkpoints ein Feld auswertet (Doku 24 Par. 2.1:
/// die Wall-Clock-Samples liegen GETRENNT nach read/write/delete vor).
enum class LatenzKurve { Write, Read, Delete };

/// Ein Ausgabefeld: JSON-Schluessel + Herkunfts-Kurve + Quantil. Mehr braucht die Ausgabe nicht --
/// die RECHNUNG steht ausschliesslich in latency_stats.hpp.
struct LatenzFeld {
    std::string_view name;  ///< JSON-Schluessel, exakt wie er im Trace steht
    LatenzKurve      kurve; ///< aus welcher Roh-ns-Kurve der Wert gezogen wird
    double           q;     ///< Quantil, das der KANON in einen Feld-Index umrechnet
};

/// DAS SCHEMA. Reihenfolge == Reihenfolge im erzeugten JSON.
/// delete_p99_ns (D5-4, 2026-08-09) schliesst die Asymmetrie: write_ und read_ trugen p50/p95/p99,
/// delete_ nur p50/p95. Die Loesch-Latenz hat denselben Tail wie Schreiben und Lesen; ohne p99 war
/// genau die Kennzahl unsichtbar, wegen der p99 ueberhaupt erhoben wird.
inline constexpr std::array<LatenzFeld, 9> kLatenzFelder{{
    {"write_p50_ns", LatenzKurve::Write, 0.50},
    {"write_p95_ns", LatenzKurve::Write, 0.95},
    {"write_p99_ns", LatenzKurve::Write, 0.99},
    {"read_p50_ns", LatenzKurve::Read, 0.50},
    {"read_p95_ns", LatenzKurve::Read, 0.95},
    {"read_p99_ns", LatenzKurve::Read, 0.99},
    {"delete_p50_ns", LatenzKurve::Delete, 0.50},
    {"delete_p95_ns", LatenzKurve::Delete, 0.95},
    {"delete_p99_ns", LatenzKurve::Delete, 0.99},
}};

/// Der geteilte CSV-Zeit-Kopf. Er stand bis D5-4 ZWEIMAL im Baum (einmal als Literal in
/// serialize_abi_tier_trace_csv, einmal als genus_trace_detail::kSharedCsvHead) -- dieselbe Abschrift
/// wie beim JSON-Feldblock, nur ohne Perzentile. Wer den SA-Kopf kennt, liest jede Gattungs-CSV.
inline constexpr std::string_view kGeteilterCsvKopf =
    "checkpoint,observe_wall_ns,fill_level,write_samples,read_samples,delete_samples";

namespace detail {

/// CT-Auswahl der Roh-Kurve. Bewusst `if constexpr` ueber ein Nicht-Typ-Template-Argument statt eines
/// `switch` zur Laufzeit (Haus-Kanon: statischer Dispatch, kein Runtime-Switch). `Snap` ist jeder
/// Checkpoint-Typ mit write_ns/read_ns/delete_ns -- AbiFillLevelSnapshot und
/// GenusFillLevelSnapshot<ObserverV1> erfuellen das, ohne eine gemeinsame Basis zu brauchen.
template <LatenzKurve K, class Snap>
[[nodiscard]] constexpr auto const& kurve(Snap const& cp) noexcept {
    if constexpr (K == LatenzKurve::Write) {
        return cp.write_ns;
    } else if constexpr (K == LatenzKurve::Read) {
        return cp.read_ns;
    } else {
        return cp.delete_ns;
    }
}

/// EIN Feld: ",\"name\":wert". Der Wert kommt ausschliesslich aus dem KANON.
template <std::size_t I, class Snap>
void schreibe_latenz_feld(std::ostringstream& os, Snap const& cp) {
    constexpr LatenzFeld feld = kLatenzFelder[I];
    os << ",\"" << feld.name << "\":" << st::percentile_ns(kurve<feld.kurve>(cp), feld.q).count();
}

template <class Snap, std::size_t... I>
void schreibe_latenz_felder(std::ostringstream& os, Snap const& cp, std::index_sequence<I...>) {
    (schreibe_latenz_feld<I>(os, cp), ...); // Fold ueber das Schema -- keine Schleife, keine Kopie
}

} // namespace detail

/// Schreibt ALLE Schema-Felder in Schema-Reihenfolge, jedes mit fuehrendem Komma. Der Aufrufer hat
/// also bereits mindestens ein Feld geschrieben (das ist bei beiden Serialisierern der Fall).
template <class Snap>
void schreibe_latenz_felder(std::ostringstream& os, Snap const& cp) {
    detail::schreibe_latenz_felder(os, cp, std::make_index_sequence<kLatenzFelder.size()>{});
}

/// Der GANZE geteilte JSON-Vorspann eines Checkpoint-Objekts: oeffnende Klammer, die drei
/// Korrelations-Felder (checkpoint/observe_wall_ns/fill_level) und danach das volle Latenz-Schema.
/// Der Aufrufer haengt seine gattungs-eigenen Observer-Felder an und schliesst mit '}'.
template <class Snap>
void schreibe_geteilten_json_vorspann(std::ostringstream& os, std::size_t index, Snap const& cp) {
    os << "{\"checkpoint\":" << index << ",\"observe_wall_ns\":" << cp.observe_wall_ns
       << ",\"fill_level\":" << cp.fill_level;
    schreibe_latenz_felder(os, cp);
}

/// Die geteilten CSV-Zellen zum geteilten Kopf (gleiche Reihenfolge, ohne fuehrendes/abschliessendes
/// Komma). Der Aufrufer haengt seine gattungs-eigenen Spalten mit fuehrendem Komma an.
template <class Snap>
void schreibe_geteilte_csv_zellen(std::ostringstream& os, std::size_t index, Snap const& cp) {
    os << index << ',' << cp.observe_wall_ns << ',' << cp.fill_level << ',' << cp.write_ns.size() << ','
       << cp.read_ns.size() << ',' << cp.delete_ns.size();
}

} // namespace comdare::cache_engine::builder::anatomy_commands::trace_schema
