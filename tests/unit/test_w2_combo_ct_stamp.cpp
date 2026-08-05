// W2 (Owner-GO mittag-6 R1 "-D-Define wie empfohlen", Fork-A-Minimalhaerte, 2026-08-05) -- Flaechen-Gate des
// CT-STEMPEL-VORRANGS an der EINEN Stempel-Naht measurement_stamp_from_env (lazy_adhoc_source_gen.hpp).
//
// DIESE TU wird MIT -DCOMDARE_MEASUREMENT_COMBO_CT="[wallclock]" kompiliert (tests/unit/CMakeLists.txt) -- sie ist
// damit der einzige Ort im Baum, an dem der CT-Zweig ueberhaupt materialisiert. Alle anderen TUs (inklusive
// test_lazy_adhoc_source_gen) kompilieren den #else-Zweig weiter und belegen dessen unveraenderte Semantik.
//
// GEPRUEFT:
//   (a) BYTE-GLEICHHEIT CT-Weg vs. Env-Weg: der CT-Zweig ruft DENSELBEN Renderer
//       (abi::measurement_stamp_line_from_combo_legend) mit DEMSELBEN Legenden-String wie der Env-Zweig ->
//       die Mess-Zeile ist byte-identisch. Das ist der Kern der DAUER-AUFLAGE Fingerprint-Neutralitaet: die
//       Mess-Zeile der Tier-Fingerprints bewegt sich durch W2 NICHT.
//   (b) ENV UNGESETZT => die einkompilierte Combo gilt (KEIN Rueckfall auf die [all]-Vollmenge) -- genau das ist
//       der Stufe-2-CT-Einbau: die Haertung haengt nicht mehr an der Laufzeit-Umgebung.
//   (c) ENV GLEICH => stumm zulaessig (der emittierte Job traegt weiterhin BEIDE Formen mit demselben Wert;
//       der Env-Kanal speist +mtool/Bestandslog -- die W-11-Flaeche, in dieser Welle unangetastet).
//   (d) ENV ABWEICHEND => Wurf fehlerklasse=konfiguration_widerspruch (Fehlerklassen-Doktrin: fail-loud, nie
//       still). Ohne diese Wache truege die DLL eine andere Mess-Zeile als der umgebende Job behauptet.
//   (e) BISS-BELEG (semantisch, am Alt-Stand): die CT-Zeile ist NICHT die [all]-Vollmengen-Zeile. Waere der
//       CT-Zweig nicht da (Alt-Stand), liefe (b) mit ungesetzter Env in die Vollmenge und (a)/(b) waeren ROT --
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

void check_ct_equals_env_render() {
    std::cout << "\n---- (a)+(e) CT-Weg == Env-Weg (derselbe Renderer, derselbe Legenden-String) ----\n";
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    std::string const via_ct    = tlz::measurement_stamp_from_env();
    std::string const via_ren   = abi::measurement_stamp_line_from_combo_legend(kCtLegend);
    std::string const vollmenge = abi::measurement_stamp_line_full_set();
    std::cout << "  CT-Legende  = " << kCtLegend << "\n";
    std::cout << "  Mess-Zeile  = " << via_ct << "\n";
    check_true("(a) CT-Mess-Zeile nicht leer", !via_ct.empty());
    check_eq("(a) CT-Zeile == Renderer-Zeile derselben Legende (BYTE-GLEICH)", via_ct, via_ren);
    check_true("(e) CT-Zeile != [all]-Vollmengen-Zeile (der Test kann nicht trivial gruen sein)", via_ct != vollmenge);
    check_true("(e) die Vollmengen-Zeile ist weiterhin renderbar (Alt-Weg unveraendert)", !vollmenge.empty());
}

void check_env_unset_uses_ct() {
    std::cout << "\n---- (b) ENV UNGESETZT => die einkompilierte Combo gilt (kein Rueckfall auf [all]) ----\n";
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    std::string const got = tlz::measurement_stamp_from_env();
    check_eq("(b) ungesetzte Env => CT-Combo", got, abi::measurement_stamp_line_from_combo_legend(kCtLegend));
    check_true("(b) ungesetzte Env faellt NICHT auf die Vollmenge zurueck",
               got != abi::measurement_stamp_line_full_set());
}

void check_env_equal_is_silent() {
    std::cout << "\n---- (c) ENV GLEICH => stumm zulaessig (beide Traeger, ein Wert) ----\n";
    ::setenv("COMDARE_MEASUREMENT_COMBO", std::string{kCtLegend}.c_str(), 1);
    bool        warf = false;
    std::string got;
    try {
        got = tlz::measurement_stamp_from_env();
    } catch (std::exception const&) { warf = true; }
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    check_true("(c) gleiche Env wirft NICHT", !warf);
    check_eq("(c) gleiche Env liefert dieselbe Zeile", got, abi::measurement_stamp_line_from_combo_legend(kCtLegend));
}

void check_env_conflict_throws() {
    std::cout << "\n---- (d) ENV ABWEICHEND => fehlerklasse=konfiguration_widerspruch (fail-loud) ----\n";
    ::setenv("COMDARE_MEASUREMENT_COMBO", "[macro]", 1);
    bool        warf = false;
    std::string was;
    try {
        (void)tlz::measurement_stamp_from_env();
    } catch (std::exception const& e) {
        warf = true;
        was  = e.what();
    }
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    check_true("(d) abweichende Env wirft", warf);
    check_true("(d) die Meldung traegt die Fehlerklasse",
               was.find("fehlerklasse=konfiguration_widerspruch") != std::string::npos);
    check_true("(d) die Meldung nennt BEIDE Werte",
               was.find("[macro]") != std::string::npos && was.find(std::string{kCtLegend}) != std::string::npos);
}

} // namespace

int main() {
    std::cout << "==== W2 CT-Stempel-Vorrang (Fork-A-Minimalhaerte) ====\n";
    check_ct_equals_env_render();
    check_env_unset_uses_ct();
    check_env_equal_is_silent();
    check_env_conflict_throws();
    std::cout << "\n==== W2 CT-Gate: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
