#pragma once
// abi/mess_gates_glied.hpp -- R-3 (07.08.2026): DAS NEUNTE PREIMAGE-GLIED "mess-gates".
//
// ------------------------------------------------------------------------------------------------
// WAS HIER GEHEILT WIRD (Befund R-3, am Objekt gemessen, Stand development aa223961)
// ------------------------------------------------------------------------------------------------
// Das Stempel-Makro COMDARE_ANATOMY_VERSION_STAMP_M rechnete den Tier-Fingerprint AUSSCHLIESSLICH
// aus den drei String-LITERALEN plus den injizierten Gliedern [5] Toolchain / [6] bvset / [7]
// Overlay. Ein Zensus nach COMDARE_MEASUREMENT_ON|mess_tooling ueber abi/anatomy_fingerprint.hpp und
// abi/toolchain_stamp_glied.hpp lieferte 0 Treffer: KEIN Mess-Gate-Makro stand im Preimage.
// FOLGE, mechanisch und am Objekt gemessen (zwei .so aus EINEM Quelltext,
// tests/unit/r3_mess_gate_stamp_module.cpp, byte-identische Stempel-Literale):
//     Objekt GATES AN   sha256 820533fb...     sha512_line 6739cae7...
//     Objekt GATES AUS  sha256 e92658ce...     sha512_line 6739cae7...   <- IDENTISCH
// Zwei verschiedene Binaries, ein Fingerprint. dll_is_current (build_orchestrator.hpp) ist EIN
// Sidecar-Vergleich ueber genau diese Zahl -- der Skip ist still und falsch.
//
// ------------------------------------------------------------------------------------------------
// WARUM GLIED [3] DAS NICHT TRAGEN KANN -- UND WARUM ES EIN NEUNTES GLIED BRAUCHT
// ------------------------------------------------------------------------------------------------
// Glied [3] traegt die Mess-Tooling-COMBO, aber als HOST-LITERAL: der Renderer
// abi::measurement_stamp_line_from_combo_legend erzeugt die Zeile aus der Legende und der Emitter
// schreibt sie in den Makro-Call. Sie ist damit eine BEHAUPTUNG des Bauwerkzeugs ueber die
// Ausstattung, nicht die WAHRHEIT der Uebersetzungseinheit. Genau diese Trennung ist der Befund:
// M-1/D-1 hat die Behauptung und den Bau an EINE Aufloesung gebunden (Verhaltens-Kopplung), aber
// die Abbildung Legende -> Defines steht selbst NICHT im Preimage. Aendert sie sich, oder faehrt
// jemand den Release-Baumodus (COMDARE_RELEASE_MODE=ON, real erreichbar --
// test_a8s4_release_pfad_neutralitaet.cpp kompiliert exakt diesen Zustand), kollidieren
// verschiedene Kompilate auf einem Fingerprint.
//
// DIESES GLIED IST DESHALB TU-WAHRHEIT UND KEINE INJEKTION. Sein Wert entsteht per PRAEPROZESSOR
// aus den REAL wirksamen Makros der uebersetzten TU. Kein neues Compile-Define ist noetig: die
// Gate-Defines selbst (perm_mess_defines -> live_mess_achsen_defines -> mess_achsen_defines) SIND
// bereits die Injektion; diese Datei liest sie direkt. Damit kann das Glied konstruktiv nicht
// luegen -- der bewusste Unterschied zu den Gliedern [5]/[6]/[7], die Host-Werte TRAGEN.
//
// ------------------------------------------------------------------------------------------------
// ODR/IFNDR -- DIE FALLE, DIE DIESE DATEI BEWUSST NICHT AUFMACHT (R-3/R4)
// ------------------------------------------------------------------------------------------------
// kMessGatesTuGlied ist TU-ABHAENGIG. Sie hat deshalb INTERNE BINDUNG (namespace-scope constexpr,
// also implizit const, also internal linkage) und darf NIEMALS
//   -- Default-Argument einer inline-Funktion,
//   -- Initialisierer einer inline-constexpr-Variablen,
//   -- oder sonst Teil einer Entitaet mit EXTERNER Bindung
// werden: verschiedene TUs desselben Programms tragen verschiedene Werte, und Misch-Programme
// EXISTIEREN real (die a8s4-Release-TU beweist es). Der Verstoss waere still (IFNDR). Praezedenz
// derselben Fehlerklasse und derselben Wache: ceb_version_stamp.hpp (M-1/D-4 Verdrahtungs-Wache).
// Der Default des neuen Glieds in anatomy_fingerprint_glieder ist deshalb die LEERE Identitaet und
// ausdruecklich NICHT diese Konstante; gereicht wird sie NUR am Makro-Expansionsort.
//
// ------------------------------------------------------------------------------------------------
// DIE GRAMMATIK
// ------------------------------------------------------------------------------------------------
//   "mg=" <m> ";" <s> ";" <x> ";" <tw> ";" <tm> ";" <tmi>
// mit m in {m0,m1} (COMDARE_MEASUREMENT_ON, wertbasiert wie anatomy/abi_adapter.hpp:9),
//     s in {s0,s1} (COMDARE_CE_ENABLE_STATISTICS, #ifdef -- so steht es im anatomy/-Baum),
//     x in {x0,x1} (COMDARE_EXPERIMENT_MODE_ON, wertbasiert wie abi_adapter.hpp:9),
//     tw/tm/tmi    (die drei Deklarations-Defines COMDARE_MEASUREMENT_TOOLING_<ID>, #ifdef).
// Es kommen ausschliesslich Zeichen aus anatomy_glied_zeichen_erlaubt vor, kein '\n', und der
// erste Schluessel ist nicht leer -- die Injektivitaets-Format-Wache
// injizierter_glied_wert_ist_wohlgeformt haelt das compile-hart fest (in anatomy_fingerprint.hpp,
// wo die Wache wohnt).
//
// KUENFTIGE GATE-VERFEINERUNG (mess_achsen_naht.hpp:93-98: G3 aus dem STATISTICS-Gate herausloesen):
// dieses Glied MUSS dann um das neue Makro erweitert werden. Die Selbstbeweis-static_asserts unten
// und der Spiegel-Test machen das Vergessen compile- bzw. test-hart; die Erweiterung selbst gehoert
// in die Beschreibung jenes Folgepakets.
//
// header-only, ASCII-only, keine Abhaengigkeit ausser <string>/<string_view>.

#include <cstddef>
#include <string>
#include <string_view>

namespace comdare::cache_engine::abi {

// -- DIE PRAEPROZESSOR-LESUNG DER TU ------------------------------------------------------------
// Je Gate ein Segment-Literal. Die Formen (#if wertbasiert vs. #ifdef) sind NICHT frei gewaehlt,
// sondern spiegeln, wie der anatomy/-Baum sein Gate tatsaechlich prueft -- ein hier abweichend
// gelesenes Makro wuerde ein Glied erzeugen, das den Kompilat-Zustand verfehlt.
#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
#define COMDARE_MESS_GATES_SEG_M "m1"
#else
#define COMDARE_MESS_GATES_SEG_M "m0"
#endif

#ifdef COMDARE_CE_ENABLE_STATISTICS
#define COMDARE_MESS_GATES_SEG_S "s1"
#else
#define COMDARE_MESS_GATES_SEG_S "s0"
#endif

#if defined(COMDARE_EXPERIMENT_MODE_ON) && COMDARE_EXPERIMENT_MODE_ON
#define COMDARE_MESS_GATES_SEG_X "x1"
#else
#define COMDARE_MESS_GATES_SEG_X "x0"
#endif

#ifdef COMDARE_MEASUREMENT_TOOLING_WALLCLOCK
#define COMDARE_MESS_GATES_SEG_TW "tw1"
#else
#define COMDARE_MESS_GATES_SEG_TW "tw0"
#endif

#ifdef COMDARE_MEASUREMENT_TOOLING_MACRO
#define COMDARE_MESS_GATES_SEG_TM "tm1"
#else
#define COMDARE_MESS_GATES_SEG_TM "tm0"
#endif

#ifdef COMDARE_MEASUREMENT_TOOLING_MICRO
#define COMDARE_MESS_GATES_SEG_TMI "tmi1"
#else
#define COMDARE_MESS_GATES_SEG_TMI "tmi0"
#endif

/// Das Praefix der Grammatik. Es traegt den nicht-leeren Schluessel, den die Format-Wache verlangt.
inline constexpr std::string_view kMessGatesGliedPraefix = "mg=";

/// Die FELD-POSITIONEN, benannt statt nackt (dieselbe Begruendung wie bei den Glied-Positionen in
/// anatomy_fingerprint.hpp: eine Umsortierung ohne Nachzug muss brechen, nicht still verschieben).
inline constexpr std::size_t kMessGatesFeldMeasurement = 0;
inline constexpr std::size_t kMessGatesFeldStatistics  = 1;
inline constexpr std::size_t kMessGatesFeldExperiment  = 2;
inline constexpr std::size_t kMessGatesFeldWallclock   = 3;
inline constexpr std::size_t kMessGatesFeldMacro       = 4;
inline constexpr std::size_t kMessGatesFeldMicro       = 5;
inline constexpr std::size_t kMessGatesFeldCount       = 6;

/// mess_gates_feld(glied, index) -- das index-te ';'-getrennte Feld NACH dem "mg="-Praefix.
/// Ausserhalb des Bereichs (oder bei fehlendem Praefix) LEER -- das ist kein stiller Ersatzwert,
/// sondern ein Ergebnis, auf das die Wachen unten ausdruecklich pruefen.
[[nodiscard]] constexpr std::string_view mess_gates_feld(std::string_view glied, std::size_t index) noexcept {
    if (glied.size() < kMessGatesGliedPraefix.size()) return {};
    if (glied.substr(0, kMessGatesGliedPraefix.size()) != kMessGatesGliedPraefix) return {};
    std::string_view const rest  = glied.substr(kMessGatesGliedPraefix.size());
    std::size_t            start = 0;
    for (std::size_t i = 0;; ++i) {
        std::size_t const semi = rest.find(';', start);
        std::size_t const end  = semi == std::string_view::npos ? rest.size() : semi;
        if (i == index) return rest.substr(start, end - start);
        if (semi == std::string_view::npos) return {};
        start = semi + 1;
    }
}

/// mess_gates_glied_komponieren(...) -- DIE EINE GRAMMATIK-BILDUNG.
///
/// Sie steht hier und nicht an der Mess-Naht, weil BEIDE Seiten sie brauchen: die TU-Seite
/// (die Konstante unten, per Praeprozessor) und die HOST-Seite (mess_gates_glied_for_legend in
/// profile_facade/mess_achsen_naht.hpp, die den Wert vorhersagt, den eine so gebaute Tier-Binary
/// annehmen WIRD). Zwei Bildungen waeren exakt die Drift-Klasse aus D-1: der Host sagt eine
/// Grammatik voraus, die TU schreibt eine andere -- und nichts braeche.
/// constexpr: der static_assert unten bindet die Praeprozessor-Fassung compile-hart an sie.
[[nodiscard]] constexpr std::string mess_gates_glied_komponieren(bool measurement_on, bool statistics_on,
                                                                bool experiment_mode_on, bool tooling_wallclock,
                                                                bool tooling_macro, bool tooling_micro) {
    std::string g{kMessGatesGliedPraefix};
    g += measurement_on ? "m1" : "m0";
    g += ';';
    g += statistics_on ? "s1" : "s0";
    g += ';';
    g += experiment_mode_on ? "x1" : "x0";
    g += ';';
    g += tooling_wallclock ? "tw1" : "tw0";
    g += ';';
    g += tooling_macro ? "tm1" : "tm0";
    g += ';';
    g += tooling_micro ? "tmi1" : "tmi0";
    return g;
}

// -- DER TU-ZUSTAND ALS BENANNTE KONSTANTEN ------------------------------------------------------
// INTERNE BINDUNG (constexpr am Namensraum-Umfang) -- s. der ODR-Absatz im Kopf. Sie sind bewusst
// KEINE `inline constexpr`: `inline` gaebe ihnen externe Bindung, und der Wert ist TU-abhaengig.
// NOLINTBEGIN(misc-definitions-in-headers)
constexpr bool kMessGatesTuMeasurementOn =
#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
    true;
#else
    false;
#endif
constexpr bool kMessGatesTuStatisticsOn =
#ifdef COMDARE_CE_ENABLE_STATISTICS
    true;
#else
    false;
#endif
constexpr bool kMessGatesTuExperimentModeOn =
#if defined(COMDARE_EXPERIMENT_MODE_ON) && COMDARE_EXPERIMENT_MODE_ON
    true;
#else
    false;
#endif
constexpr bool kMessGatesTuToolingWallclock =
#ifdef COMDARE_MEASUREMENT_TOOLING_WALLCLOCK
    true;
#else
    false;
#endif
constexpr bool kMessGatesTuToolingMacro =
#ifdef COMDARE_MEASUREMENT_TOOLING_MACRO
    true;
#else
    false;
#endif
constexpr bool kMessGatesTuToolingMicro =
#ifdef COMDARE_MEASUREMENT_TOOLING_MICRO
    true;
#else
    false;
#endif

/// kMessGatesTuGlied -- DER WERT DES NEUNTEN GLIEDS FUER **DIESE** UEBERSETZUNGSEINHEIT.
/// Er entsteht rein aus den Segment-Literalen oben, also aus dem Praeprozessor-Zustand, den der
/// Compiler wirklich sieht. Interne Bindung -- s. der ODR-Absatz im Kopf.
constexpr std::string_view kMessGatesTuGlied = "mg=" COMDARE_MESS_GATES_SEG_M ";" COMDARE_MESS_GATES_SEG_S
                                               ";" COMDARE_MESS_GATES_SEG_X ";" COMDARE_MESS_GATES_SEG_TW
                                               ";" COMDARE_MESS_GATES_SEG_TM ";" COMDARE_MESS_GATES_SEG_TMI;
// NOLINTEND(misc-definitions-in-headers)

// -- W3: DER SELBSTBEWEIS. DIE KONSTANTE KANN DEN TU-ZUSTAND NICHT VERFEHLEN ----------------------
// Je Praeprozessor-Zweig ein static_assert auf das POSITIONSGENAUE Feld (nicht auf ein Vorkommen
// des Teilstrings: "tm1" enthaelt "m1", eine find()-Wache waere hier stillschweigend blind).
#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldMeasurement) == "m1",
              "R-3/W3: COMDARE_MEASUREMENT_ON ist in dieser TU AN, das mess-gates-Glied sagt aber 'm0'.");
#else
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldMeasurement) == "m0",
              "R-3/W3: COMDARE_MEASUREMENT_ON ist in dieser TU AUS, das mess-gates-Glied sagt aber 'm1'.");
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldStatistics) == "s1",
              "R-3/W3: COMDARE_CE_ENABLE_STATISTICS ist in dieser TU AN, das Glied sagt aber 's0'.");
#else
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldStatistics) == "s0",
              "R-3/W3: COMDARE_CE_ENABLE_STATISTICS ist in dieser TU AUS, das Glied sagt aber 's1'.");
#endif
#if defined(COMDARE_EXPERIMENT_MODE_ON) && COMDARE_EXPERIMENT_MODE_ON
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldExperiment) == "x1",
              "R-3/W3: COMDARE_EXPERIMENT_MODE_ON ist in dieser TU AN, das Glied sagt aber 'x0'.");
#else
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldExperiment) == "x0",
              "R-3/W3: COMDARE_EXPERIMENT_MODE_ON ist in dieser TU AUS, das Glied sagt aber 'x1'.");
#endif
#ifdef COMDARE_MEASUREMENT_TOOLING_WALLCLOCK
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldWallclock) == "tw1",
              "R-3/W3: COMDARE_MEASUREMENT_TOOLING_WALLCLOCK ist gesetzt, das Glied sagt aber 'tw0'.");
#else
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldWallclock) == "tw0",
              "R-3/W3: COMDARE_MEASUREMENT_TOOLING_WALLCLOCK ist NICHT gesetzt, das Glied sagt aber 'tw1'.");
#endif
#ifdef COMDARE_MEASUREMENT_TOOLING_MACRO
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldMacro) == "tm1",
              "R-3/W3: COMDARE_MEASUREMENT_TOOLING_MACRO ist gesetzt, das Glied sagt aber 'tm0'.");
#else
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldMacro) == "tm0",
              "R-3/W3: COMDARE_MEASUREMENT_TOOLING_MACRO ist NICHT gesetzt, das Glied sagt aber 'tm1'.");
#endif
#ifdef COMDARE_MEASUREMENT_TOOLING_MICRO
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldMicro) == "tmi1",
              "R-3/W3: COMDARE_MEASUREMENT_TOOLING_MICRO ist gesetzt, das Glied sagt aber 'tmi0'.");
#else
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldMicro) == "tmi0",
              "R-3/W3: COMDARE_MEASUREMENT_TOOLING_MICRO ist NICHT gesetzt, das Glied sagt aber 'tmi1'.");
#endif

/// EIN Feld mehr gibt es nicht -- sonst waere die Feld-Buchhaltung oben unvollstaendig und ein
/// kuenftiges siebtes Gate koennte still hinten anhaengen, ohne dass eine Wache es forderte.
static_assert(mess_gates_feld(kMessGatesTuGlied, kMessGatesFeldCount).empty(),
              "R-3/W3: das mess-gates-Glied traegt mehr Felder, als kMessGatesFeldCount kennt.");

/// DIE BINDUNG DER BEIDEN BILDUNGEN: die Praeprozessor-Fassung und der Komponist liefern dasselbe.
/// Ohne sie waere "eine Grammatik" eine Absichtserklaerung -- der Host koennte eine andere Form
/// vorhersagen, als die TU einbaut, und genau diese Drift ist die Fehlerklasse aus D-1.
static_assert(mess_gates_glied_komponieren(kMessGatesTuMeasurementOn, kMessGatesTuStatisticsOn,
                                           kMessGatesTuExperimentModeOn, kMessGatesTuToolingWallclock,
                                           kMessGatesTuToolingMacro, kMessGatesTuToolingMicro) == kMessGatesTuGlied,
              "R-3: die Praeprozessor-Bildung des mess-gates-Glieds und mess_gates_glied_komponieren() "
              "sind auseinandergelaufen -- es gibt dann ZWEI Grammatiken und die Host-Vorhersage ist wertlos.");

#undef COMDARE_MESS_GATES_SEG_M
#undef COMDARE_MESS_GATES_SEG_S
#undef COMDARE_MESS_GATES_SEG_X
#undef COMDARE_MESS_GATES_SEG_TW
#undef COMDARE_MESS_GATES_SEG_TM
#undef COMDARE_MESS_GATES_SEG_TMI

} // namespace comdare::cache_engine::abi
