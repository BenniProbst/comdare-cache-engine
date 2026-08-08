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
// ------------------------------------------------------------------------------------------------
// WARUM DIE BILDUNG KEINEN std::string BENUTZT (08.08.2026)
// ------------------------------------------------------------------------------------------------
// Die Grammatik-Bildung unten MUSS zur Uebersetzungszeit auswertbar sein: der static_assert am
// Dateiende ist die EINZIGE Wache dagegen, dass die Praeprozessor-Fassung und die Host-Vorhersage
// auseinanderlaufen (Drift-Klasse D-1). Ein `constexpr std::string` traegt das nicht ueberall --
// am Objekt gemessen mit clang 22.1 gegen libstdc++ 13, 15.3 UND 16:
//     error: static assertion expression is not an integral constant expression
//     note:  undefined function '_M_construct<const char *>' cannot be used in a constant expression
// Die Fehlerklasse ist eng und benannt: NUR das Konstruieren eines std::string AUS EINEM
// string_view bricht (der const char*-Konstruktor, operator+=, append(string_view), das Wachsen
// ueber die SSO-Grenze und der Vergleich gegen ein string_view sind alle auswertbar). g++ 15.3
// wertet auch die string_view-Konstruktion aus -- der Bau war deshalb bis heute nur auf EINEM der
// beiden offiziellen Compiler beweisbar, und das ist keine tragfaehige Wache.
// DIE FOLGE IST KEIN AUSWEICHEN, SONDERN DER PASSENDE BAU: die Grammatik ist kurz und in ihrer
// Laenge compile-time BESCHRAENKT (kMessGatesGliedMaxLen unten wird aus den Segmenten gerechnet).
// Eine Bildung, die zur Uebersetzungszeit laufen MUSS, braucht keinen Heap. Es bleibt bei GENAU
// EINER Bildung -- eine zweite, constexpr-taugliche neben der bestehenden waere exakt die Drift,
// die dieser Header verhindert.
//
// header-only, ASCII-only, keine Abhaengigkeit ausser <array>/<cstddef>/<string>/<string_view>
// (<string> traegt nur noch den LAUFZEIT-Ausgang MessGatesGliedText::str(), nicht die Bildung).

#include <array>
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

/// Der Feld-TRENNER der Grammatik. Er steht hier als Konstante und nicht als Literal in der Bildung,
/// weil die Kapazitaets-Rechnung unten mit ihm rechnet.
inline constexpr char kMessGatesFeldTrenner = ';';

/// DIE SEGMENT-FORMEN je Feld, in Feld-Reihenfolge: [feld][0] die AUS-, [feld][1] die AN-Form.
/// Sie stehen HIER und nicht als Literale in der Bildung, weil die Kapazitaets-Rechnung darunter
/// DIESELBEN Texte liest -- wer ein Segment verlaengert, bewegt damit automatisch die Kapazitaet und
/// kann nicht vergessen, sie nachzuziehen. Die Zuordnung Zeile <-> Feld haelt eine Wache am
/// Dateiende fest, damit eine Umsortierung bricht statt still zu verschieben.
/// Die Praeprozessor-Fassung weiter unten traegt dieselben Texte noch einmal als Makro-Literale --
/// sie MUSS, weil der Praeprozessor keine constexpr-Tabelle lesen kann. Genau diese unvermeidbare
/// Doppelung ist der Grund fuer den Binde-static_assert am Dateiende.
inline constexpr std::string_view kMessGatesSegmente[kMessGatesFeldCount][2] = {
    {"m0", "m1"}, {"s0", "s1"}, {"x0", "x1"}, {"tw0", "tw1"}, {"tm0", "tm1"}, {"tmi0", "tmi1"},
};

/// kMessGatesGliedMaxLen -- die LAENGSTE Zeichenkette, die diese Grammatik bilden kann. GERECHNET
/// aus den Segmenten oben, nicht abgezaehlt: der Puffer darunter kann konstruktiv nicht zu klein
/// werden. Heute sind es 24 Zeichen ("mg=m0;s0;x0;tw0;tm0;tmi0"); die Wache am Dateiende prueft alle
/// 2^6 baubaren Formen dagegen, damit die Zahl nicht bloss behauptet ist.
inline constexpr std::size_t kMessGatesGliedMaxLen = [] {
    std::size_t n = kMessGatesGliedPraefix.size() + (kMessGatesFeldCount - 1); // Praefix + Trenner
    for (auto const& feld : kMessGatesSegmente)
        n += feld[0].size() > feld[1].size() ? feld[0].size() : feld[1].size();
    return n;
}();

/// MessGatesGliedText -- DAS ERGEBNIS DER EINEN BILDUNG: ein Zeichenpuffer fester Kapazitaet mit
/// Laenge, also ein Literaltyp, den eine constant expression tragen kann. sv() ist die Sicht fuer die
/// compile-harten Wachen; str() ist der Uebergang auf die Laufzeit-Seite und die einzige Stelle, die
/// ueberhaupt einen Heap anfasst -- und sie tut es erst NACH der Uebersetzung.
class MessGatesGliedText {
  public:
    /// anhaengen(...) -- der Wachstumsschritt. Ein Ueberlauf ist durch kMessGatesGliedMaxLen
    /// konstruktiv ausgeschlossen; .at() haelt das trotzdem fest, statt still ueber den Rand zu
    /// schreiben: in einer constant expression bricht es compile-hart, zur Laufzeit wirft es.
    /// DIESE Meldung ist als einzige im Header unbenannt -- sie liest sich als "static assertion
    /// expression is not an integral constant expression" (clang) bzw. "non-constant condition for
    /// static assertion" (g++), ohne Text. Wer sie sieht, hat kMessGatesGliedMaxLen zu klein gesetzt
    /// (am Objekt geprueft, Koeder K5): die Kapazitaet gehoert aus kMessGatesSegmente gerechnet, nicht
    /// als Zahl eingetragen.
    constexpr void anhaengen(std::string_view s) {
        for (char const c : s) puffer_.at(laenge_++) = c;
    }
    constexpr void anhaengen(char c) { puffer_.at(laenge_++) = c; }

    [[nodiscard]] constexpr std::string_view sv() const noexcept {
        return std::string_view{puffer_.data(), laenge_};
    }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return laenge_; }

    /// str() -- der Ausgang zur Laufzeit. Bewusst NICHT constexpr: wer den Wert in einer constant
    /// expression braucht, will sv(); wer einen std::string braucht, ist per Definition zur Laufzeit.
    /// Sie steht hier und nicht am Aufrufort, damit `komponieren(...).sv()` nicht versehentlich in
    /// einem `auto` landet und auf das gerade zerstoerte Temporary zeigt.
    [[nodiscard]] std::string str() const { return std::string{sv()}; }

  private:
    std::array<char, kMessGatesGliedMaxLen> puffer_{};
    std::size_t                            laenge_ = 0;
};

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
/// constexpr: der static_assert unten bindet die Praeprozessor-Fassung compile-hart an sie. Der
/// Rueckgabetyp ist MessGatesGliedText und NICHT std::string -- s. den Absatz "WARUM DIE BILDUNG
/// KEINEN std::string BENUTZT" im Kopf. Die Laufzeit-Seiten holen sich ihren std::string mit .str().
[[nodiscard]] constexpr MessGatesGliedText mess_gates_glied_komponieren(bool measurement_on, bool statistics_on,
                                                                        bool experiment_mode_on,
                                                                        bool tooling_wallclock, bool tooling_macro,
                                                                        bool tooling_micro) {
    // Die Zuordnung Argument -> Feld laeuft ueber die BENANNTEN Positionen, nicht ueber die
    // Reihenfolge der Argumente: eine Umsortierung der Felder muss die Bildung mitnehmen, und die
    // Wache am Dateiende faengt es, falls jemand nur die Segment-Tabelle umstellt.
    bool an[kMessGatesFeldCount]{};
    an[kMessGatesFeldMeasurement] = measurement_on;
    an[kMessGatesFeldStatistics]  = statistics_on;
    an[kMessGatesFeldExperiment]  = experiment_mode_on;
    an[kMessGatesFeldWallclock]   = tooling_wallclock;
    an[kMessGatesFeldMacro]       = tooling_macro;
    an[kMessGatesFeldMicro]       = tooling_micro;

    MessGatesGliedText g;
    g.anhaengen(kMessGatesGliedPraefix);
    for (std::size_t i = 0; i < kMessGatesFeldCount; ++i) {
        if (i != 0) g.anhaengen(kMessGatesFeldTrenner);
        g.anhaengen(kMessGatesSegmente[i][an[i] ? 1 : 0]);
    }
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
constexpr std::string_view kMessGatesTuGlied =
    "mg=" COMDARE_MESS_GATES_SEG_M ";" COMDARE_MESS_GATES_SEG_S ";" COMDARE_MESS_GATES_SEG_X
    ";" COMDARE_MESS_GATES_SEG_TW ";" COMDARE_MESS_GATES_SEG_TM ";" COMDARE_MESS_GATES_SEG_TMI;
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
                                           kMessGatesTuToolingMacro, kMessGatesTuToolingMicro)
                      .sv() == kMessGatesTuGlied,
              "R-3: die Praeprozessor-Bildung des mess-gates-Glieds und mess_gates_glied_komponieren() "
              "sind auseinandergelaufen -- es gibt dann ZWEI Grammatiken und die Host-Vorhersage ist wertlos.");

// -- DIE WACHEN UM DEN PUFFER (08.08.2026) --------------------------------------------------------
// Sie belegen, was der Kopf-Absatz behauptet: dass die Kapazitaet gerechnet und nicht geraten ist,
// und dass die Segment-Tabelle zu den benannten Feld-Positionen passt.
namespace detail {

/// Die groesste Laenge ueber ALLE 2^6 baubaren Grammatiken. Sie laeuft die Bildung wirklich durch --
/// laege eine Form ueber der Kapazitaet, braeche schon das .at() im Puffer, und zwar hier.
[[nodiscard]] constexpr std::size_t mess_gates_groesste_bildung() {
    std::size_t groesste = 0;
    for (unsigned maske = 0; maske < (1u << kMessGatesFeldCount); ++maske) {
        auto const bit = [maske](std::size_t feld) { return ((maske >> feld) & 1u) != 0u; };
        auto const g   = mess_gates_glied_komponieren(bit(kMessGatesFeldMeasurement), bit(kMessGatesFeldStatistics),
                                                      bit(kMessGatesFeldExperiment), bit(kMessGatesFeldWallclock),
                                                      bit(kMessGatesFeldMacro), bit(kMessGatesFeldMicro));
        if (g.size() > groesste) groesste = g.size();
    }
    return groesste;
}

/// Jedes Feld einzeln AN: steht das erwartete AN-Segment an der erwarteten Position? Das bindet die
/// ARGUMENT-ZUORDNUNG der Bildung (an[kMessGatesFeld*] = ...) an die Zeilen-Ordnung der Tabelle. Wer
/// eines von beiden umstellt und das andere vergisst, bricht HIER -- statt still das Segment eines
/// anderen Gates an diese Stelle zu schreiben.
/// ABGRENZUNG, am Objekt geprueft (Koeder K3): eine Umsortierung der TABELLE ALLEIN faengt diese
/// Wache NICHT -- Bildung und Wache lesen dieselbe Tabelle, also stimmen sie miteinander ueberein.
/// Diesen Fall faengt der Binde-static_assert oben, weil die Praeprozessor-Fassung nicht mitwandert.
/// Die beiden Wachen sind deshalb nicht redundant, sondern decken verschiedene Halbseiten ab.
[[nodiscard]] constexpr bool mess_gates_segmente_stehen_auf_ihrem_feld() {
    for (std::size_t feld = 0; feld < kMessGatesFeldCount; ++feld) {
        auto const bit = [feld](std::size_t f) { return f == feld; };
        auto const g   = mess_gates_glied_komponieren(bit(kMessGatesFeldMeasurement), bit(kMessGatesFeldStatistics),
                                                      bit(kMessGatesFeldExperiment), bit(kMessGatesFeldWallclock),
                                                      bit(kMessGatesFeldMacro), bit(kMessGatesFeldMicro));
        for (std::size_t i = 0; i < kMessGatesFeldCount; ++i)
            if (mess_gates_feld(g.sv(), i) != kMessGatesSegmente[i][i == feld ? 1 : 0]) return false;
    }
    return true;
}

} // namespace detail

static_assert(detail::mess_gates_groesste_bildung() == kMessGatesGliedMaxLen,
              "R-3/08.08.: kMessGatesGliedMaxLen trifft die groesste baubare Grammatik nicht mehr exakt. Zu "
              "klein waere ein Ueberlauf (das .at() im Puffer faengt ihn), zu gross ein stiller Reserve-Rest -- "
              "in beiden Faellen ist die Rechnung aus kMessGatesSegmente nachzuziehen, nicht die Zahl.");
static_assert(detail::mess_gates_segmente_stehen_auf_ihrem_feld(),
              "R-3/08.08.: die Segment-Tabelle kMessGatesSegmente und die benannten kMessGatesFeld*-Positionen "
              "sind auseinandergelaufen -- die Bildung schreibt ein Gate an die Stelle eines anderen.");
static_assert(mess_gates_glied_komponieren(false, false, false, false, false, false).sv() == "mg=m0;s0;x0;tw0;tm0;tmi0",
              "R-3/08.08.: der AUS-Zustand der Grammatik hat seine Form geaendert. Er ist an mehreren Stellen "
              "als Text belegt (anatomy_fingerprint.hpp: 'NIEMALS leer'); wer ihn bewegt, zieht sie mit.");

#undef COMDARE_MESS_GATES_SEG_M
#undef COMDARE_MESS_GATES_SEG_S
#undef COMDARE_MESS_GATES_SEG_X
#undef COMDARE_MESS_GATES_SEG_TW
#undef COMDARE_MESS_GATES_SEG_TM
#undef COMDARE_MESS_GATES_SEG_TMI

} // namespace comdare::cache_engine::abi
