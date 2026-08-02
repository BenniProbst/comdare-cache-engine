#pragma once
// experiment_tree/slice_marker.hpp -- E-04-P1 (Marker-Familie v2, Owner-KERN "ich muss live sehen ...
// exakt WELCHE Tier-Binary Rekombinationen und wie viele davon noch offen sind"): die EINE Renderer-
// Quelle der Fortschritts-Marker des Slice-Kanals.
//
// WARUM EINE QUELLE: der Job-Trace ist der einzige waehrend eines 7d-Jobs live lesbare Kanal. Der
// Aggregator (E-04-P4 `plan status`) keyt auf das TUPEL (zelle, fenster) -- NIE auf die Zeilen-
// Reihenfolge (der Scheiben-Arbiter macht Fenster kuenftig nicht-monoton, und mehrere Perms einer
// Lane wiederholen dieselben fenster=-Werte). Damit dieses Keying ueberhaupt moeglich ist, sind
// lane=, zelle= und fenster= PFLICHTFELDER JEDER Zeile der Familie (Review-Auflage A7): eine Zeile
// muss fuer sich allein zuordenbar sein, ohne den umgebenden Shell-Kontext.
//
// LAYER-TRENNUNG (Section 62-B-NACHTRAG, wie in der Shell-Testat-Grammatik): zelle= traegt die
// Bau-Ebene [d,e,f][g,h,i] (System-Perm + Organ-Referenz), die CEB-Ebene [a,b,c] steht in ihrem
// EIGENEN Feld ceb= -- die Layer werden NIE in eine Klammer verschmolzen. Der frueher nur im
// Batch-KOPF stehende [a,b,c]-Wert wird dadurch je Zeile mitgefuehrt, ohne die Grammatik zu brechen.
//
// GOLDEN-NEUTRAL: reine String-Formatierung fuer std::cerr. Kein Mess-Datum, kein binary_id-Byte,
// keine CSV-Beruehrung -- dasselbe Muster wie die bestehenden [bestandslog]-/[pruef-fail]-Zeilen.
// Header-only, nur std.

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

// ---------------------------------------------------------------------------
// Die Marken der Familie (Single-Source: Emitter UND Tee-Filter der CI-Emission lesen dieselben
// Literale -- ein Umbenennen kann den Filter nicht mehr stumm hinter dem Emitter zuruecklassen).
// ---------------------------------------------------------------------------
inline constexpr std::string_view kMarkePlanTestat   = "PLAN-TESTAT";   // VOR dem Fenster-Bau (Soll/offen)
inline constexpr std::string_view kMarkeBilanzTestat = "BILANZ-TESTAT"; // NACH dem Fenster-Bau (Ist)
inline constexpr std::string_view kMarkePruefBilanz  = "PRUEF-BILANZ";  // NACH dem Gate-Durchlauf (Ist)

// Genau die Marken, die LIVE in den Job-Trace gehoeren (Tee-Filter-Liste der Stufe-2-Emission,
// E-04-P1-1c). Trace-Hygiene bleibt: alles Uebrige der Treiber-Ausgabe bleibt im Artefakt-Log.
inline constexpr std::array<std::string_view, 3> kSliceMarkerTraceMarken{kMarkePlanTestat, kMarkeBilanzTestat,
                                                                         kMarkePruefBilanz};

// Sentinel eines NICHT belegten Pflichtfeldes. Ein Pflichtfeld verschwindet nie aus der Zeile (der
// Parser haette sonst zwei Zeilen-Formen); ein ungesetzter Wert wird EHRLICH benannt statt geraten.
inline constexpr std::string_view kMarkerUnbelegt = "unbelegt";

// ---------------------------------------------------------------------------
// Feld-Wert-Saeuberung: ein Marker-Wert darf die Zeilen-Grammatik nicht brechen. Getrennt wird an
// Leerzeichen, Feld und Wert an '='; beide Zeichen (und alles Nicht-Druckbare, inkl. Zeilenumbruch)
// falten daher auf '_'. Die Legenden-Zeichen '[' ']' ',' bleiben ERHALTEN -- sie sind der Inhalt der
// zelle=/ceb=-Werte. Leer => Sentinel.
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string marker_wert(std::string_view roh) {
    std::string out;
    out.reserve(roh.size());
    for (char const c : roh) {
        auto const u        = static_cast<unsigned char>(c);
        bool const druckbar = u > 0x20U && u < 0x7FU && c != '=';
        out += druckbar ? c : '_';
    }
    if (out.empty()) return std::string{kMarkerUnbelegt};
    return out;
}

// ---------------------------------------------------------------------------
// Der Fenster-Token "<begin>:<count>". Die Werte sind GLOBALE StaticBinaryView-Indizes -- exakt die
// Koordinate, die die CI-Emission als COMDARE_GOLDEN_N_RANGE="${START}:${COUNT}" setzt. Nur so
// treffen die Treiber-Zeilen und die Shell-Testate desselben Fensters auf denselben Aggregator-Key.
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string marker_fenster(std::size_t begin, std::size_t count) {
    return std::to_string(begin) + ":" + std::to_string(count);
}

// ---------------------------------------------------------------------------
// Die PFLICHT-Koordinaten des Laufs (lauf-konstant; der Host belegt sie aus derselben Env, aus der
// die CI-Emission ihre Testat-Zelle bildet -- KEINE zweite Ableitung, kein Raten).
// ---------------------------------------------------------------------------
struct MarkerKontext {
    std::string lane;  // Host-Lane des Batch-Jobs (amd/intel/...) -- Pflichtfeld
    std::string zelle; // Bau-Ebene [d,e,f][g,h,i] -- Pflichtfeld, zweiter Teil des Aggregator-Keys
    std::string ceb;   // CEB-Ebene [a,b,c] -- eigenes Feld (Layer nie verschmolzen)
};

// ---------------------------------------------------------------------------
// Der gemeinsame KOPF jeder Zeile der Familie:
//   "[<MARKE>] ts=<iso> lane=<L> zelle=<Z> ceb=<C> phase=<P> fenster=<begin:count>"
// Die Zaehler haengt der Aufrufer an (je Marke verschieden). Die Feld-Ordnung spiegelt die
// bestehende Shell-Testat-Grammatik ([TESTAT] ts= lane= zelle= phase= fenster=) -- EINE Grammatik,
// EIN Parser. `ts` liefert der Aufrufer (Zeit-Formatierung ist nicht Sache dieser Schicht).
// ---------------------------------------------------------------------------
[[nodiscard]] inline std::string marker_kopf(std::string_view marke, MarkerKontext const& kontext, std::string_view ts,
                                             std::string_view phase, std::string_view fenster) {
    std::string s;
    s += '[';
    s += marke;
    s += "] ts=";
    s += marker_wert(ts);
    s += " lane=";
    s += marker_wert(kontext.lane);
    s += " zelle=";
    s += marker_wert(kontext.zelle);
    s += " ceb=";
    s += marker_wert(kontext.ceb);
    s += " phase=";
    s += marker_wert(phase);
    s += " fenster=";
    s += marker_wert(fenster);
    return s;
}

} // namespace comdare::cache_engine::builder::experiment
