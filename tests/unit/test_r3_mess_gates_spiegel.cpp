// tests/unit/test_r3_mess_gates_spiegel.cpp -- R-3 SPIEGEL-WACHE (07.08.2026).
//
// DER BISS (test_r3_mess_gate_fingerprint_biss) beweist, DASS der Gate-Zustand den Fingerprint
// diskriminiert. Diese TU beweist die zweite Haelfte, ohne die der Biss nur die halbe Zusage traegt:
// DASS DIE HOST-SEITE DENSELBEN WERT VORHERSAGT, DEN DIE TIER-TU EINBAUT.
//
// WARUM DAS EINE EIGENE WACHE BRAUCHT (Fehlerklasse D-1, eine Ebene hoeher): der Laufzeit-Zwilling des
// Fingerprints (lazy_adhoc_fingerprint_for) rechnet auf dem HOST. Er kann den Praeprozessor-Zustand der
// Tier-Uebersetzungseinheit nicht LESEN -- er kann ihn nur VORHERSAGEN. Laufen Vorhersage und Einbau
// auseinander, ist die drift-freie Zusage des Zwillings gebrochen: das .fingerprint-Sidecar traegt eine
// andere Zahl als die gebaute .so, und dll_is_current baut ewig neu (oder, schlimmer, skippt falsch,
// sobald die Drift in die andere Richtung zeigt). Genau diese Klasse hat M-1/D-1 fuer die Defines
// geschlossen; hier ist sie fuer das Identitaets-Glied geschlossen.
//
// VIER AUSSAGEN, jede literal statt behauptend:
//   (a) NENNER + TU-WAHRHEIT: das Glied DIESER TU spiegelt den Makro-Zustand DIESER TU.
//   (b) SPIEGEL: fuer JEDE baubare Mess-Combo sagt mess_gates_glied_for_legend genau den Wert vorher,
//       den mess_achsen_defines() als Define-Vektor emittiert.
//   (c) INJEKTIVITAET (Owner-KERN F6): verschiedene baubare Mess-Achsen -> verschiedene Glieder.
//   (d) HOST == TU UEBER DIE ECHTE .so-GRENZE: der Host rechnet den Fingerprint der beiden real
//       gebauten Biss-Module NACH -- aus ihren eigenen Stempel-Zeilen plus dem erwarteten Gate-Glied --
//       und trifft byte-genau das einkompilierte sha512_line. Das ist der Beleg, dass das neunte Glied
//       WIRKLICH der Unterschied ist und nicht bloss irgendein Unterschied.
//
// ASCII-only. Keine Mess-CSV-Beruehrung, kein Schreiben ausserhalb stdout.

#include <profile_facade/mess_achsen_naht.hpp>

#include <cache_engine/abi/anatomy_fingerprint.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>
#include <cache_engine/abi/mess_gates_glied.hpp>
#include <mess_axes/measurement_tooling_registry.hpp>
#include <sha512/ctsha512.hpp>

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace pf  = ::comdare::cache_engine::profile_facade;
namespace cea = ::comdare::cache_engine::abi;
namespace cm  = ::comdare::cache_engine::measurement;
namespace s5  = ::comdare::cache_engine::sha512;

namespace {

int g_fail = 0;

void check_true(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

/// Die vier BAUBAREN Combos (M-1/H-1: eine Combo ohne 'wallclock' ist kein baubarer Zustand).
[[nodiscard]] std::vector<std::string> baubare_legenden() {
    return {"[wallclock]", "[wallclock,macro]", "[wallclock,micro]", "[all]"};
}

[[nodiscard]] bool hat(std::vector<std::string> const& v, std::string const& d) {
    return std::find(v.begin(), v.end(), d) != v.end();
}

/// Der HOST-Nachbau des Tier-Fingerprints: dieselbe EINE Glied-Ordnung, dieselbe SHA-512-Primitive.
///
/// E-E (07.08.2026) -- DAS OVERLAY-GLIED [7] MUSS HIER DEN LIVE-WERT TRAGEN, und dieser Test hat den
/// Grund selbst geliefert: er ist am Bau der Scharfschaltung ROT geworden, weil er weiter mit
/// OverlayHash{""} rechnete, waehrend die beiden REALEN .so-Module den einkompilierten Wert trugen
/// (gemessen: TU 524fa622... gegen Host 0e9e719f...). Das ist genau die Drift, gegen die dieser Test
/// gebaut ist -- nur diesmal auf der Host-Seite. Ab hier zieht er dieselbe Konstante, die auch die
/// Uebersetzungseinheit der Module gezogen hat; damit prueft er zusaetzlich, dass das Glied den Weg in
/// die reale .so wirklich findet und nicht bloss in eine Host-Rechnung.
/// Die uebrigen Glieder bleiben LEER: die Probe-Module werden ohne Toolchain-/bvset-Define uebersetzt,
/// ihr einkompilierter Wert ist dort also tatsaechlich die Identitaet.
[[nodiscard]] std::string host_fingerprint(std::string_view organ, std::string_view system,
                                           std::string_view measurement, std::string_view mess_gates) {
    auto const glieder = cea::anatomy_fingerprint_glieder(
        cea::MessZeile{measurement}, cea::SystemZeile{system}, cea::OrganZeile{organ}, cea::ToolchainGlied{""},
        cea::BvsetGlied{""}, cea::OverlayHash{cea::kOverlaySourceHash}, cea::MessGatesGlied{mess_gates});
    std::string const preimage =
        cea::anatomy_fingerprint_preimage(std::span<std::string_view const>{glieder.data(), glieder.size()});
    auto const digest = s5::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(preimage.data()), preimage.size()});
    auto const hex = s5::to_hex(digest);
    return std::string(hex.data(), hex.size());
}

using VersionLinesFn = cea::AnatomyVersionLines const* (*)();

/// dlopen + Symbol; RTLD_LOCAL, weil beide Module DASSELBE Symbol exportieren (s. Biss-Runner).
[[nodiscard]] cea::AnatomyVersionLines const* lade_pod(std::string const& pfad, std::string& fehler) {
    void* h = ::dlopen(pfad.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (h == nullptr) {
        char const* e = ::dlerror();
        fehler        = e != nullptr ? e : "dlopen fehlgeschlagen";
        return nullptr;
    }
    ::dlerror();
    void* sym = ::dlsym(h, "comdare_anatomy_version_lines");
    if (sym == nullptr) {
        char const* e = ::dlerror();
        fehler        = std::string{"Symbol fehlt: "} + (e != nullptr ? e : "?");
        return nullptr;
    }
    return reinterpret_cast<VersionLinesFn>(sym)();
}

// ---------------------------------------------------------------------------------------------
// (a) NENNER + TU-WAHRHEIT
// ---------------------------------------------------------------------------------------------
void fall_a_tu_wahrheit() {
    std::cout << "\n---- (a) NENNER: das Glied DIESER TU spiegelt ihren Makro-Zustand ----\n";
    std::cout << "  kMessGatesTuGlied = " << cea::kMessGatesTuGlied << "\n";
    check_true("(a) das Glied ist NIEMALS leer (der Aus-Zustand ist mg=m0;s0;...;hm0;hmi0)",
               !cea::kMessGatesTuGlied.empty());
    check_true("(a) das Glied besteht die Injektivitaets-Format-Wache",
               cea::injizierter_glied_wert_ist_wohlgeformt(cea::kMessGatesTuGlied));
    check_true("(a) das Glied haelt sein Preimage-Budget",
               cea::kMessGatesTuGlied.size() <= cea::kAnatomyFingerprintMessGatesMax);
    check_true("(a) es hat genau kMessGatesFeldCount Felder",
               cea::mess_gates_feld(cea::kMessGatesTuGlied, cea::kMessGatesFeldCount).empty() &&
                   !cea::mess_gates_feld(cea::kMessGatesTuGlied, cea::kMessGatesFeldCount - 1).empty());
    // Diese TU uebersetzt MIT MEASUREMENT_ON und CE_ENABLE_STATISTICS (target_compile_definitions) und
    // OHNE die drei Deklarations-Defines -- der Vergleich ist also nicht tautologisch.
    check_true("(a) Feld m spiegelt COMDARE_MEASUREMENT_ON",
               cea::mess_gates_feld(cea::kMessGatesTuGlied, cea::kMessGatesFeldMeasurement) ==
                   (cea::kMessGatesTuMeasurementOn ? "m1" : "m0"));
    check_true("(a) Feld s spiegelt COMDARE_CE_ENABLE_STATISTICS",
               cea::mess_gates_feld(cea::kMessGatesTuGlied, cea::kMessGatesFeldStatistics) ==
                   (cea::kMessGatesTuStatisticsOn ? "s1" : "s0"));
    check_true("(a) die Format-Kennung ist auf 6 gebumpt (B-9/golden-102)",
               cea::kAnatomyFingerprintFormat == std::string_view{"fingerprint_format=6"});
    check_true("(a) die Glied-Folge hat ELF Glieder", cea::kAnatomyFingerprintGliedCount == 11u);
}

// ---------------------------------------------------------------------------------------------
// (b) SPIEGEL: Vorhersage == Define-Vektor
// ---------------------------------------------------------------------------------------------
void fall_b_spiegel() {
    std::cout << "\n---- (b) SPIEGEL: mess_gates_glied_for_legend == der emittierte Define-Vektor ----\n";
    std::size_t geprueft = 0;
    for (auto const& legend : baubare_legenden()) {
        std::vector<std::string> const def = pf::mess_achsen_defines_for_legend(legend);
        std::string const              ist = pf::mess_gates_glied_for_legend(legend);
        // Die ERWARTUNG wird hier UNABHAENGIG gebildet -- aus dem Define-Vektor, Feld fuer Feld, ohne die
        // Spiegel-Funktion zu benutzen. Sonst pruefte der Test eine Funktion gegen sich selbst.
        // B2: das <st>-Feld kommt hier bewusst NUR aus dem expliziten =1 -- in allen vier baubaren
        // Legenden entscheidet die Naht das G3-Gate explizit, sobald G2 emittiert ist; der Erb-Fall
        // des Ableitungs-Headers ist aus dieser Emission nicht erreichbar und wird in (a)/(d) ueber
        // die TU-Wahrheit geprueft.
        std::string erwartet =
            cea::mess_gates_glied_komponieren(
                hat(def, "-DCOMDARE_MEASUREMENT_ON=1"), hat(def, "-DCOMDARE_CE_ENABLE_STATISTICS=1"),
                hat(def, "-DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1"),
                /*experiment_mode_on=*/true, hat(def, "-DCOMDARE_MEASUREMENT_TOOLING_WALLCLOCK=1"),
                hat(def, "-DCOMDARE_MEASUREMENT_TOOLING_MACRO=1"), hat(def, "-DCOMDARE_MEASUREMENT_TOOLING_MICRO=1"),
                // A-12/B-5e: unabhaengig gebildet wie alle Felder daneben -- aus DEMSELBEN Define-Vektor,
                // nicht aus der Spiegel-Funktion. Der Perm-Bau-Pfad setzt die Hybrid-Gates heute nicht;
                // steht das Define eines Tages drin, wandert die Erwartung von selbst mit.
                hat(def, "-DCOMDARE_HYBRID_TOOLING_MACRO=1"), hat(def, "-DCOMDARE_HYBRID_TOOLING_MICRO=1"))
                .str();
        std::cout << "    " << legend << "  ->  " << ist << "\n";
        check_true(std::string{"(b) "} + legend + ": Vorhersage == Define-Vektor", ist == erwartet);
        ++geprueft;
    }
    std::cout << "  geprueft: " << geprueft << " Legenden (erwartet 4)\n";
    check_true("(b) es wurden wirklich 4 Legenden geprueft (der Test kann nicht leer gruen sein)", geprueft == 4);
}

// ---------------------------------------------------------------------------------------------
// (c) INJEKTIVITAET
// ---------------------------------------------------------------------------------------------
void fall_c_injektiv() {
    std::cout << "\n---- (c) INJEKTIVITAET (F6): verschiedene Mess-Achsen -> verschiedene Glieder ----\n";
    std::vector<std::string> const legenden = baubare_legenden();
    std::vector<std::string>       glieder;
    for (auto const& l : legenden) glieder.push_back(pf::mess_gates_glied_for_legend(l));
    std::size_t paare  = 0;
    std::size_t gleich = 0;
    for (std::size_t i = 0; i < glieder.size(); ++i)
        for (std::size_t j = i + 1; j < glieder.size(); ++j) {
            ++paare;
            if (glieder[i] == glieder[j]) {
                std::cout << "  [ERR] KOLLISION: " << legenden[i] << " == " << legenden[j] << "\n";
                ++gleich;
            }
        }
    std::cout << "  Paare verglichen: " << paare << " (erwartet 6), Kollisionen: " << gleich << "\n";
    check_true("(c) 6 Paare wirklich verglichen", paare == 6);
    check_true("(c) KEINE zwei Mess-Achsen-Wahlen kollidieren auf demselben Gate-Glied", gleich == 0);
    // GEGENPROBE: dieselbe Legende zweimal ergibt DASSELBE Glied -- eine Funktion, die immer
    // Verschiedenes lieferte, waere auch injektiv und trotzdem wertlos.
    check_true("(c) GEGENPROBE: dieselbe Legende -> dasselbe Glied",
               pf::mess_gates_glied_for_legend("[all]") == pf::mess_gates_glied_for_legend("[all]"));
}

// ---------------------------------------------------------------------------------------------
// (d) HOST == TU ueber die echte .so-Grenze
// ---------------------------------------------------------------------------------------------
void fall_d_host_gegen_tu(std::string const& pfad_an, std::string const& pfad_aus) {
    std::cout << "\n---- (d) HOST == TU: der Host rechnet die beiden realen .so-Fingerprints nach ----\n";
    // Die erwarteten Gate-Glieder der beiden Biss-Module. Sie stehen hier als Literale, weil sie die
    // BESTELLUNG der beiden CMake-Targets sind (r3_biss_modul_gates_an: MEASUREMENT_ON, STATISTICS,
    // EXPERIMENT_MODE_ON, TOOLING_WALLCLOCK; r3_biss_modul_gates_aus: keines davon). Waere die
    // Bestellung eine andere, faellt diese Wache -- und genau das soll sie.
    // B2: das AN-Modul setzt KEIN explizites G3-Define -- es faehrt den ERB-Fall des
    // Ableitungs-Headers (G3 folgt G2, also AN). Genau deshalb steht hier st=true: diese Wache
    // beweist die Vererbung ueber die echte .so-Grenze, nicht nur im Host-Rechenweg.
    // A-12/B-5e: die beiden Hybrid-Gates stehen in BEIDEN Erwartungen auf false -- die Test-Module sind
    // Tier-Module und bestellen keine Hybrid-Defines. Das AN-Modul heisst "an" wegen der Mess-Gates,
    // nicht wegen aller Felder; genau deshalb steht hier nicht pauschal true.
    std::string const glied_an =
        cea::mess_gates_glied_komponieren(true, true, true, true, true, false, false, false, false).str();
    std::string const glied_aus =
        cea::mess_gates_glied_komponieren(false, false, false, false, false, false, false, false, false).str();
    std::cout << "    erwartetes Glied AN  = " << glied_an << "\n";
    std::cout << "    erwartetes Glied AUS = " << glied_aus << "\n";

    struct Fall {
        // Default-Initialisierer: cppcheck verlangt sie (uninitMemberVarNoCtor), und sie sind hier
        // sachlich richtig -- die Faelle werden ausschliesslich per Aggregat-Initialisierung
        // vollstaendig belegt, ein leeres Feld waere ein Konstruktions-Fehler, kein gueltiger Zustand.
        char const* name  = nullptr;
        std::string pfad  = {};
        std::string glied = {};
    };
    std::array<Fall, 2> const faelle{Fall{"GATES AN ", pfad_an, glied_an}, Fall{"GATES AUS", pfad_aus, glied_aus}};
    for (auto const& f : faelle) {
        std::string                           fehler;
        cea::AnatomyVersionLines const* const v = lade_pod(f.pfad, fehler);
        if (v == nullptr) {
            check_true(std::string{"(d) "} + f.name + " geladen (" + fehler + ")", false);
            continue;
        }
        std::string const organ{v->organ_line, static_cast<std::size_t>(v->organ_len)};
        std::string const system{v->system_line, static_cast<std::size_t>(v->system_len)};
        std::string const mess{v->measurement_line, static_cast<std::size_t>(v->measurement_len)};
        std::string const tu_fp{v->sha512_line, static_cast<std::size_t>(v->sha512_len)};
        std::string const host_fp = host_fingerprint(organ, system, mess, f.glied);
        std::cout << "    " << f.name << " TU  =" << tu_fp << "\n";
        std::cout << "    " << f.name << " HOST=" << host_fp << "\n";
        check_true(std::string{"(d) "} + f.name + ": Host-Nachrechnung == einkompiliertes sha512_line",
                   host_fp == tu_fp);
        // GEGENPROBE: mit dem FALSCHEN Gate-Glied trifft der Host NICHT. Ohne sie waere (d) auch dann
        // gruen, wenn das neunte Glied gar nicht ins Preimage einginge.
        std::string const falsches = f.glied == glied_an ? glied_aus : glied_an;
        check_true(std::string{"(d) GEGENPROBE "} + f.name + ": mit dem falschen Gate-Glied trifft der Host NICHT",
                   host_fingerprint(organ, system, mess, falsches) != tu_fp);
    }
}

} // namespace

int main(int argc, char** argv) {
    std::string pfad_an;
    std::string pfad_aus;
    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        if (a.rfind("--an=", 0) == 0)
            pfad_an = std::string{a.substr(5)};
        else if (a.rfind("--aus=", 0) == 0)
            pfad_aus = std::string{a.substr(6)};
    }
    std::cout << "== R-3 SPIEGEL: die Host-Vorhersage des Mess-Gates-Glieds deckt sich mit der TU ==\n";
    try {
        fall_a_tu_wahrheit();
        fall_b_spiegel();
        fall_c_injektiv();
        if (pfad_an.empty() || pfad_aus.empty()) {
            check_true("(d) beide Modul-Pfade gereicht (--an=/--aus=)", false);
        } else {
            fall_d_host_gegen_tu(pfad_an, pfad_aus);
        }
    } catch (std::exception const& e) {
        std::cout << "  [ERR] unerwarteter Wurf: " << e.what() << "\n";
        ++g_fail;
    }
    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
