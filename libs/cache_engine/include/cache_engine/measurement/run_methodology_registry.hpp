#pragma once
// Run-Methodik-Mess-UNTER-Achse (Ledger Section 47 / Section 54-T2 / Section 55, 2026-07-20): die 4 Ablauf-
// Methoden {Debug, Measure, Release, Compare} des Mess-Vollzugs. Sie TYPISIERT den offenen TODO in
// measurement_axis_registry.xml:55-56 ("die 3 Mess-Modi Debug/Mess/Release existieren NICHT als Typen ->
// nicht emittiert; erst nach ihrer Typisierung als Mess-Unter-Achse reflektierbar").
//
// STRUKT-R Lane D (Q-4, 2026-07-26): COMPARE ist der 4. Registry-Modus; zuvor lebte er NUR als
// XSD-Kommentar-Reserve (experiment_schema.xsd:60-61), nicht als Typ.
//
// ORDNUNG KORRIGIERT (Owner-Entscheid O-A, 2026-08-07 -- §62-C ist insoweit SUPERSEDED): compare steht
// FORMAL VOR release, nicht danach. Die geltende Reihenfolge ist measure -> compare -> release, kumulativ:
// Owner verbatim "formal kommt compare als Stufe mit eigenen Optionen (lesend Messwertlager) vor dem
// release, der auch die Messwerte nachlesen muss, aber dann eine optimale binary produziert."
// Die alte Fassung (§62-C, 21.07.: "ERST ZUM SCHLUSS, NACH DEM RELEASE, folgt je Maschine der erweiterte
// COMPARE-Modus") stand hier bis heute und ist nicht mehr die Anweisung. Zwei Konsequenzen fuer den Bau:
// compare braucht EIGENE Optionen mit LESENDEM Lager-Zugriff, und release liest die Messwerte ebenfalls
// nach -- es erzeugt daraus die OPTIMALE Binary. Beides ist noch nicht gebaut (Paket D2, s. unten).
//
// NICHT VERWECHSELN: die REGISTRY-Reihenfolge unten ist {debug, measure, release, compare} -- eine
// AUFZAEHLUNG in der historischen Reihenfolge ihrer Entstehung, KEINE Ablauf-Ordnung. Die Ablauf-Ordnung
// ist measure -> compare -> release (s.o.). Die Enum-Reihenfolge wurde bewusst NICHT umgestellt: sie ist
// stempel-/ABI-relevant, und der Owner-Entscheid O-A betrifft die formale STUFEN-Ordnung, nicht die
// Aufzaehlung. Wer eine Ablauf-Ordnung braucht, leitet sie aus der Enthaltungs-Ordnung ab
// (measure ist in compare enthalten, compare in release), nicht aus dem Enum-Index.
//
// WAEHLBARKEIT, NICHT VOLLZUG: seine Build-Semantik ist heute bewusst release-GLEICH
// {Release, misst NICHT, parallel}, damit die Wahl von compare an KEINER Emissions-Naht ein anderes Verhalten
// erzeugt (die Emitter verzweigen ausschliesslich auf cmake_build_type == "Debug"; measurement_on gatet allein
// die Mess-Parallelitaet, measure_parallelism.hpp:25). Der modus-SPEZIFISCHE Ablauf (Replay-Vergleich statt
// Messen) ist das Nach-Trigger-Paket D2 -- bis dahin ist compare ein waehlbares, validierbares ETIKETT.
// CUSTOM_COMPILE ist KEIN Modus (Q-4): es ueberschreibt spaeter als CLI-Feature (§60-R3) die 4 Modi und liefert
// dem Anwender die Wunsch-Binary -- es kommt NIE in diese Registry.
//
// ABGRENZUNG (Section 54-T2): dies ist eine Mess-Tooling-UNTER-Achse (Planer-gesteuert, delegiert,
// binary_id-NEUTRAL) -- NICHT die HAUPT-Auffaecherung (das ist MeasurementTooling {WallClock/Macro/Micro},
// measurement_tooling_registry.hpp, die ALLEIN den kMeasurementAxisVersionLine-Stempel traegt, Section 43).
// A9.1 traegt diese Achse PASSIV (Feld + Parse + XSD + validate-id-Check); der Fan-out/Vollzug gehoert S5.
//
// KEIN Runtime-Switch: reine constexpr-Tabelle + Metaprogrammierungs-Iteration (analog measurement_tooling_registry).
// header-only, C++23. GOLDEN/HOST-NEUTRAL: reine constexpr-Identitaet, keine Host-/Bau-/Mess-Semantik.

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::measurement {

/// Die Run-Methodik-UNTER-Achse: WELCHE Ablauf-Methode der Mess-Vollzug faehrt (Section 47/55; Section 32-F1/F7).
enum class RunMethodology : std::uint8_t {
    Debug,   ///< Debug-Lauf -- parallel/schnell, KEINE Mess-golden-Zahlen (Verifikation der Verdrahtung)
    Measure, ///< Mess-Lauf -- 1-Thread/deterministisch, die golden-Messung (Run-to-Run-stabil)
    Release, ///< Release-Lauf -- Voll-Optimierung ohne Mess-Instrumentierung (Referenz-Durchsatz)
    Compare, ///< COMPARE-Lauf -- Stufe VOR release (O-A): liest das Messwertlager, eigene Optionen; Etikett bis D2
};

// Single-Source: Drift einer 5. Methode bricht hier compile-time (statt still 4 zu bleiben).
inline constexpr std::size_t kRunMethodologyCount = 4;

struct RunMethodologyInfo {
    RunMethodology   methodology;
    std::string_view id;   ///< kanonischer XML-/Legenden-Token ("debug"/"measure"/"release")
    std::string_view name; ///< exakt der Enum-Name (Doku/Reporting)
    // S5-P1 (P-VOLLZUG, Section 47/55, 2026-07-20): die Build-Semantik jeder Ablauf-Methode. Der Planer speist
    // daraus den CMAKE_BUILD_TYPE + die Mess-/Thread-Politik der emittierten Bau-/Mess-Jobs (Single-Source statt
    // Magic-String im Emitter). GOLDEN/binary_id-NEUTRAL: reine Bau-/Mess-Matrix, KEIN Stempel-Feld -- opt/simd/
    // build_type sind system_config und fliessen NIE in binary_id (Q2 Option C).
    std::string_view cmake_build_type; ///< CMAKE_BUILD_TYPE dieses Laufs ("Debug"/"Release")
    bool             measurement_on;   ///< misst der Lauf (golden-Zahlen) oder baut/referenziert er nur
    // §61-MODI (User-Volldefinition 2026-07-21): single_thread bezeichnet die MESS-SEQUENTIALITAET (der Mess-Loop
    // misst 1-Thread-deterministisch, Section 38.b) -- NICHT die Bau-Parallelitaet. Der BAU (DLL-Provision-Pool,
    // COMDARE_BUILD_PARALLEL) bleibt in ALLEN Modi parallel; NUR das Messen ist bei measure/release sequentiell.
    // Modi-Volldefinition (§61-MODI): debug = je Maschine parallel bauen + parallel MESSEN (paralleler Mess-Loop
    // §16.2-M1 GEBAUT, #45/99a608c2: debug misst nproc-parallel via resolve_measure_parallelism + collect_ordered,
    // measure/release strikt 0); measure = parallel bauen + sequentiell messen; release = parallel bauen + sequentiell
    // messen -> Auslieferung (Observer-frei + Wallclock-Beweis, S7/Post-Scope). Werte hier unveraendert.
    bool single_thread; ///< MESS-Sequentialitaet (Mess-Loop 1-Thread, Section 38.b) -- NICHT Bau-Politik
};

/// Die EINE Registry der Run-Methodik-UNTER-Achse -- Index == RunMethodology-Wert (static_assert-gesichert).
/// S5-P1-Build-Semantik: measure = deterministischer 1-Thread-Messlauf (Release, misst); debug = paralleler
/// Verdrahtungs-Check (Debug, misst, KEINE 1-Thread-Determinismus-Garantie); release = Referenz-Durchsatz (Release,
/// misst NICHT). Der Emitter waehlt fuer die S5-Mess-Strecke die measure-Zeile (der Methodik-Fanout ist S6).
/// Lane D (Q-4/Q-5), Ordnung nach O-A (2026-08-07): compare ist die Stufe VOR release -- es LIEST das
/// Messwertlager (eigene Optionen) und vergleicht die bereits gemessenen Sichten, erhebt aber selbst KEINE
/// golden-Zahlen. Es baut deshalb wie release (Release, misst NICHT, parallel); die Zeile ist bewusst
/// byte-gleich zu release: Waehlbarkeit ohne Verhaltens-Aenderung (Vollzug D2). Danach erst release, das die
/// Messwerte ebenfalls nachliest und daraus die OPTIMALE Binary erzeugt -- auch das ist Paket D2.
inline constexpr std::array<RunMethodologyInfo, kRunMethodologyCount> kRunMethodologyRegistry{{
    {RunMethodology::Debug, "debug", "Debug", "Debug", true, false},
    {RunMethodology::Measure, "measure", "Measure", "Release", true, true},
    {RunMethodology::Release, "release", "Release", "Release", false, false},
    {RunMethodology::Compare, "compare", "Compare", "Release", false, false},
}};

namespace detail {
[[nodiscard]] consteval bool run_methodology_registry_is_complete() {
    for (std::size_t i = 0; i < kRunMethodologyCount; ++i) {
        if (static_cast<std::size_t>(kRunMethodologyRegistry[i].methodology) != i) return false;
        if (kRunMethodologyRegistry[i].id.empty()) return false;
        if (kRunMethodologyRegistry[i].name.empty()) return false;
        if (kRunMethodologyRegistry[i].cmake_build_type.empty()) return false; // S5-P1: Build-Typ nie leer
    }
    return true;
}
} // namespace detail
static_assert(kRunMethodologyRegistry.size() == kRunMethodologyCount,
              "kRunMethodologyRegistry: Array-Groesse == kRunMethodologyCount (Anzahl-Anker).");
static_assert(detail::run_methodology_registry_is_complete(),
              "kRunMethodologyRegistry: 4 Eintraege, Index==RunMethodology, id/name nie leer.");
// Namen-Anker: Drift eines id-Tokens (Umbenennung/Vertauschung) bricht hier compile-time.
static_assert(
    kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].id == std::string_view{"debug"} &&
        kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].id == std::string_view{"measure"} &&
        kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].id == std::string_view{"release"} &&
        kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Compare)].id == std::string_view{"compare"},
    "kRunMethodologyRegistry: id-Tokens sind {debug,measure,release,compare} (Namen-Anker).");
// S5-P1 Build-Semantik-Anker: Drift der Build-/Mess-/Thread-Politik einer Methode bricht hier compile-time. Der
// Emitter verlaesst sich auf measure == {Release, misst, 1-Thread} (die S5-Mess-Strecke); direkter Index-Zugriff,
// weil run_methodology_info() erst weiter unten deklariert ist.
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].cmake_build_type ==
                      std::string_view{"Release"} &&
                  kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].measurement_on &&
                  kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Measure)].single_thread,
              "kRunMethodologyRegistry: measure = {Release, misst, 1-Thread-deterministisch} (S5-Mess-Strecke).");
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].cmake_build_type ==
                      std::string_view{"Debug"} &&
                  kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].measurement_on &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Debug)].single_thread,
              "kRunMethodologyRegistry: debug = {Debug, misst, parallel/kein 1-Thread-Garantie}.");
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].cmake_build_type ==
                      std::string_view{"Release"} &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].measurement_on &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Release)].single_thread,
              "kRunMethodologyRegistry: release = {Release, misst NICHT, parallel} (Referenz-Durchsatz).");
// Lane D (Q-5): compare = {Release, misst NICHT, parallel} -- der ETIKETT-Stand. Der Anker ist die Tripwire fuer
// D2: wer compare seinen eigenen Ablauf gibt (Replay-Vergleich statt Bau/Mess-Semantik von release), muss ihn
// BEWUSST loesen, statt still eine Emissions-Naht mitzunehmen. cmake_build_type nicht-leer ist Pflicht (consteval
// run_methodology_registry_is_complete) -- "Release" ist die korrekte Wahl, weil compare Release-Sichten vergleicht.
static_assert(kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Compare)].cmake_build_type ==
                      std::string_view{"Release"} &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Compare)].measurement_on &&
                  !kRunMethodologyRegistry[static_cast<std::size_t>(RunMethodology::Compare)].single_thread,
              "kRunMethodologyRegistry: compare = {Release, misst NICHT, parallel} (Etikett-Stand, Vollzug D2).");

/// constexpr-Lookup (Index == RunMethodology-Wert, durch static_assert garantiert).
[[nodiscard]] constexpr RunMethodologyInfo const& run_methodology_info(RunMethodology m) noexcept {
    return kRunMethodologyRegistry[static_cast<std::size_t>(m)];
}

namespace detail {
/// Die gueltigen Tokens als Text -- fuer die Fehlermeldung aus DERSELBEN Registry (keine zweite Wissensquelle,
/// keine handgepflegte Aufzaehlung, die bei einer 5. Methode still veraltet).
[[nodiscard]] inline std::string run_methodology_known_ids() {
    std::string out;
    for (std::size_t i = 0; i < kRunMethodologyCount; ++i) {
        if (i > 0) out += ", ";
        out.append(kRunMethodologyRegistry[i].id);
    }
    return out;
}
} // namespace detail

/// #45/§61-STUFEN: die AKTIVE Methodik aus den XML-Tokens (tp/ep.run_methodology; validate erzwingt exactly-one).
/// LEER => measure-Default (Release, 1-Thread): die ABWESENHEIT einer Deklaration ist eine Aussage und bleibt der
/// byte-neutrale Vor-§61-Stand. UNBEKANNTES Token => HARTER Fehler (Welle B/1, 2026-08-07, FAIL-CLOSED): bis hierher
/// fiel ein Tippfehler im XML ("mesure", "profiling") STILL auf measure zurueck und loeste damit eine MESSUNG aus,
/// die niemand angefordert hat. Leer und unbekannt sind NICHT dasselbe -- das eine ist Abwesenheit, das andere ein
/// falsch geschriebener Wunsch. Der Validator gated denselben Token bereits VOR dem Bau (check_measurement_sub_axis,
/// validate_profile.hpp); diese Wache traegt den UNVALIDIERTEN Direkt-Pfad (Struct-Injektion, Entry ohne Validat).
/// SINGLE-SOURCE fuer den Runtime-Konsum UND fuer den Planer (experiment_plan_director::build_semantic_of_run_
/// methodology delegiert hierher) -- der Mess-Loop leitet daraus single_thread ab (debug => paralleler Mess-Loop).
[[nodiscard]] inline RunMethodologyInfo const& run_methodology_for_ids(std::vector<std::string> const& ids) {
    // R5/§61-STUFEN (LED:3190): GENAU EINE Methodik je Call. >1 ist ein Kontraktbruch -- der Validator gated ihn
    // VOR dem Bau (validate_profile: run_methodology.size()>1 => nicht ok). Hier defensiv HART (statt still ids.front()),
    // damit eine mehrdeutige 2-Modi-Liste NIE zufaellig eine Modus-Semantik (z.B. debug-Mess-Loop) waehlt.
    if (ids.size() > 1)
        throw std::invalid_argument(
            "run_methodology_for_ids: " + std::to_string(ids.size()) +
            " Methoden deklariert -- GENAU EINE erlaubt (exactly-one je Call, Ledger 61-STUFEN).");
    if (ids.empty()) return run_methodology_info(RunMethodology::Measure); // Abwesenheit => measure-Default
    for (std::size_t i = 0; i < kRunMethodologyCount; ++i)
        if (kRunMethodologyRegistry[i].id == ids.front()) return kRunMethodologyRegistry[i];
    throw std::invalid_argument("run_methodology_for_ids: unbekannter Modus-Token \"" + ids.front() +
                                "\" -- gueltig sind {" + detail::run_methodology_known_ids() +
                                "}. FAIL-CLOSED (Welle B/1): ein stiller measure-Ersatz wuerde eine Messung "
                                "ausloesen, die niemand angefordert hat.");
}

/// Compile-time-Iteration ueber die Run-Methodik-UNTER-Achse (Metaprogrammierungs-Interface).
template <class Visitor>
constexpr void for_each_run_methodology(Visitor&& visitor) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (visitor(kRunMethodologyRegistry[I]), ...);
    }(std::make_index_sequence<kRunMethodologyCount>{});
}

} // namespace comdare::cache_engine::measurement
