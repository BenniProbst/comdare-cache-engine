// W2 (Owner-GO mittag-6 R1 "-D-Define wie empfohlen", Fork-A-Minimalhaerte, 2026-08-05) -- Flaechen-Gate des
// CT-STEMPEL-VORRANGS an der EINEN Stempel-Naht measurement_stamp_from_env (lazy_adhoc_source_gen.hpp).
//
// DIESE TU wird MIT -DCOMDARE_MEASUREMENT_COMBO_CT="[wallclock]" kompiliert (tests/unit/CMakeLists.txt) -- sie ist
// damit der einzige Ort im Baum, an dem der CT-Zweig ueberhaupt materialisiert. Alle anderen TUs (inklusive
// test_lazy_adhoc_source_gen) kompilieren den #else-Zweig weiter und belegen dessen unveraenderte Semantik.
//
// -- F-B2 (Codex-Nachreview W1/W2, Ledger-Nachtrag 05.08.2026 nachmittag-7): DIE ZUSAGE (b) IST INVERTIERT ----
// Der Erst-Stand dieser TU pruefte "ENV UNGESETZT => die einkompilierte Combo gilt (stumm)". Genau dieser stumme
// Pfad WAR der Befund: bei gesetztem CT-Define und leerer Env fuehrt der STEMPEL die CT-Welt, waehrend der
// Cache-Key (+mtool aus der Env) und die Bestandslog-Zelle z.combo die LEERE/[all]-Identitaet fuehren --
// SCHLUESSEL-WELT-SPLIT gegen die Anker-Doktrin "EINE Schluessel-Welt". Die Heilung ist fail-loud (Fehlerklassen-
// Doktrin): eine leere/fehlende Env bei SPEZIFISCHEM CT-Define ist ab jetzt derselbe Konfigurationswiderspruch
// wie eine abweichende Env. Die alte (b)-Zeile ist damit NICHT "geloescht", sondern in ihr Gegenteil ueberfuehrt
// und hier als Inversion deklariert (Doku-Doktrin: Aenderungen benennen, nicht stillschweigend wegnehmen).
// Der emittierte CI-/Batch-Pfad traegt ohnehin BEIDE Formen synchron (Env-Export + -D-Define je Combo), die
// [all]-Emission traegt seit F-B1 KEIN Define mehr (sie loescht die CMake-Cache-Var explizit) -- der neue Wurf
// kann also keinen legalen Lauf treffen.
//
// GEPRUEFT:
//   (a) BYTE-GLEICHHEIT CT-Weg vs. Env-Weg: der CT-Zweig ruft DENSELBEN Renderer
//       (abi::measurement_stamp_line_from_combo_legend) mit DEMSELBEN Legenden-String wie der Env-Zweig ->
//       die Mess-Zeile ist byte-identisch. Das ist der Kern der DAUER-AUFLAGE Fingerprint-Neutralitaet: die
//       Mess-Zeile der Tier-Fingerprints bewegt sich durch W2 NICHT (auch F-B2 bewegt KEIN Stempel-BYTE --
//       es wirft nur dort, wo der Alt-Stand still ein Split-Ergebnis lieferte).
//   (b) ENV LEER/FEHLEND bei SPEZIFISCHEM CT-Define => Wurf fehlerklasse=konfiguration_widerspruch (F-B2, NEU;
//       beide Formen geprueft: unsetenv UND gesetzt-aber-leer). BISS: am Alt-Stand blieb dieser Pfad STUMM und
//       lieferte die CT-Zeile -- diese Zeile ist am Alt-Stand ROT.
//   (c) ENV GLEICH => stumm zulaessig (der emittierte Job traegt weiterhin BEIDE Formen mit demselben Wert;
//       der Env-Kanal speist +mtool/Bestandslog -- die W-11-Flaeche, in dieser Welle unangetastet).
//   (d) ENV ABWEICHEND => Wurf fehlerklasse=konfiguration_widerspruch (Fehlerklassen-Doktrin: fail-loud, nie
//       still). Ohne diese Wache truege die DLL eine andere Mess-Zeile als der umgebende Job behauptet.
//   (e) BISS-BELEG (semantisch): die CT-Zeile ist NICHT die [all]-Vollmengen-Zeile. Waere der CT-Zweig nicht da
//       (Alt-Alt-Stand), liefe (a) mit synchroner Env zwar noch gruen, aber (b) waere ohne Wurf ROT --
//       der Test kann also nicht trivial gruen sein. Zusaetzlich wird die Vollmengen-Zeile selbst gerendert und
//       als VERSCHIEDEN belegt.
//
// Plain-main-Test (Muster test_lazy_adhoc_source_gen.cpp): check_eq/check_true, exit 0 = alle OK.

#include "lazy_adhoc_source_gen.hpp" // measurement_stamp_from_env (die EINE Stempel-Naht)

#include <cache_engine/abi/anatomy_version_stamp.hpp> // measurement_stamp_line_from_combo_legend / _full_set

#include <cstdlib> // setenv/unsetenv (Env-Kanal der Widerspruchs-Wache)
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace abi = ::comdare::cache_engine::abi;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

#ifndef COMDARE_MEASUREMENT_COMBO_CT
#error "test_w2_combo_ct_stamp MUSS mit -DCOMDARE_MEASUREMENT_COMBO_CT kompiliert werden (CT-Zweig-Flaeche)"
#endif

constexpr std::string_view kCtLegend = COMDARE_MEASUREMENT_COMBO_CT;

/// Ruft die Stempel-Naht und meldet (warf?, Meldung, Ergebnis) -- EIN Helfer fuer alle vier Faelle, damit kein
/// Fall einen eigenen try/catch-Dialekt entwickelt (der stumme Alt-Pfad entstand genau aus so einer Doppelform).
struct NahtLauf {
    bool        warf = false;
    std::string was;
    std::string zeile;
};

[[nodiscard]] NahtLauf ruf_naht() {
    NahtLauf r;
    try {
        r.zeile = tlz::measurement_stamp_from_env();
    } catch (std::exception const& e) {
        r.warf = true;
        r.was  = e.what();
    }
    return r;
}

void check_ct_equals_env_render() {
    std::cout << "\n---- (a)+(e) CT-Weg == Env-Weg (derselbe Renderer, derselbe Legenden-String) ----\n";
    // F-B2: die SYNCHRONE Env ist ab jetzt die einzige legale Aufruf-Form des CT-Zweiges -- der Vergleich laeuft
    // deshalb mit gesetzter, gleicher Env (er prueft den RENDERER, nicht die Env-Wache).
    ::setenv("COMDARE_MEASUREMENT_COMBO", std::string{kCtLegend}.c_str(), 1);
    NahtLauf const    lauf      = ruf_naht();
    std::string const via_ren   = abi::measurement_stamp_line_from_combo_legend(kCtLegend);
    std::string const vollmenge = abi::measurement_stamp_line_full_set();
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    std::cout << "  CT-Legende  = " << kCtLegend << "\n";
    std::cout << "  Mess-Zeile  = " << lauf.zeile << "\n";
    check_true("(a) synchrone Env wirft NICHT", !lauf.warf);
    check_true("(a) CT-Mess-Zeile nicht leer", !lauf.zeile.empty());
    check_eq("(a) CT-Zeile == Renderer-Zeile derselben Legende (BYTE-GLEICH)", lauf.zeile, via_ren);
    check_true("(e) CT-Zeile != [all]-Vollmengen-Zeile (der Test kann nicht trivial gruen sein)",
               lauf.zeile != vollmenge);
    check_true("(e) die Vollmengen-Zeile ist weiterhin renderbar (Alt-Weg unveraendert)", !vollmenge.empty());
}

void check_env_missing_throws() {
    std::cout << "\n---- (b) F-B2: ENV LEER/FEHLEND bei spezifischem CT-Define => fail-loud (Alt-Stand: STUMM) ----\n";
    // Form 1: gar nicht gesetzt.
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    NahtLauf const ohne = ruf_naht();
    check_true("(b) fehlende Env wirft (F-B2; am Alt-Stand lieferte sie stumm die CT-Zeile)", ohne.warf);
    check_true("(b) die Meldung traegt die Fehlerklasse",
               ohne.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
    check_true("(b) die Meldung benennt die FEHLENDE Env", ohne.was.find("fehlt/leer") != std::string::npos);
    check_true("(b) die Meldung nennt die einkompilierte Combo",
               ohne.was.find(std::string{kCtLegend}) != std::string::npos);

    // Form 2: gesetzt, aber leer -- semantisch dasselbe (der Alt-Stand behandelte beide gleich stumm).
    ::setenv("COMDARE_MEASUREMENT_COMBO", "", 1);
    NahtLauf const leer = ruf_naht();
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    check_true("(b) LEER gesetzte Env wirft ebenso", leer.warf);
    check_true("(b) dieselbe Fehlerklasse wie bei fehlender Env",
               leer.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
}

void check_env_equal_is_silent() {
    std::cout << "\n---- (c) ENV GLEICH => stumm zulaessig (beide Traeger, ein Wert) ----\n";
    ::setenv("COMDARE_MEASUREMENT_COMBO", std::string{kCtLegend}.c_str(), 1);
    NahtLauf const lauf = ruf_naht();
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    check_true("(c) gleiche Env wirft NICHT", !lauf.warf);
    check_eq("(c) gleiche Env liefert dieselbe Zeile", lauf.zeile,
             abi::measurement_stamp_line_from_combo_legend(kCtLegend));
}

void check_env_conflict_throws() {
    std::cout << "\n---- (d) ENV ABWEICHEND => fehlerklasse=konfiguration_widerspruch (fail-loud) ----\n";
    ::setenv("COMDARE_MEASUREMENT_COMBO", "[macro]", 1);
    NahtLauf const lauf = ruf_naht();
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    check_true("(d) abweichende Env wirft", lauf.warf);
    check_true("(d) die Meldung traegt die Fehlerklasse",
               lauf.was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
    check_true("(d) die Meldung nennt BEIDE Werte", lauf.was.find("[macro]") != std::string::npos &&
                                                        lauf.was.find(std::string{kCtLegend}) != std::string::npos);
}

} // namespace

int main() {
    std::cout << "==== W2 CT-Stempel-Vorrang (Fork-A-Minimalhaerte) + F-B2 Env-Synchronitaets-Wache ====\n";
    check_ct_equals_env_render();
    check_env_missing_throws();
    check_env_equal_is_silent();
    check_env_conflict_throws();
    std::cout << "\n==== W2 CT-Gate: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
