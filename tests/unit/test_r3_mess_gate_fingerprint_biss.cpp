// tests/unit/test_r3_mess_gate_fingerprint_biss.cpp -- R-3 BISS, die RUNNER-SEITE (07.08.2026).
//
// DIE ZU BEWEISENDE AUSSAGE, in einem Satz: ZWEI TIER-BINARIES MIT VERSCHIEDENEM MESS-GATE-ZUSTAND
// TRAGEN VERSCHIEDENE FINGERPRINTS.
//
// WARUM DIESER TEST EXISTIERT (Befund R-3, am Objekt gemessen, Stand development aa223961):
// anatomy_fingerprint_hex rechnete kFP ausschliesslich aus den DREI String-LITERALEN plus den
// injizierten Gliedern [5] Toolchain / [6] bvset / [7] Overlay. Ein Zensus ueber
// abi/anatomy_fingerprint.hpp und abi/toolchain_stamp_glied.hpp nach COMDARE_MEASUREMENT_ON|
// mess_tooling lieferte 0 Treffer -- KEIN Gate-Makro stand im Preimage. Glied [3] traegt zwar die
// Mess-Tooling-COMBO, aber als HOST-Literal (Renderer measurement_stamp_line_from_combo_legend), also
// als BEHAUPTUNG ueber die Ausstattung, nicht als Wahrheit der Uebersetzungseinheit.
// FOLGE, mechanisch: dasselbe emittierte .cpp mit und ohne -DCOMDARE_MEASUREMENT_ON=1 ergibt zwei
// VERSCHIEDENE Binaries mit DEMSELBEN kFP. dll_is_current (build_orchestrator.hpp) vergleicht genau
// diese Zahl gegen ein Sidecar -- der Skip ist still und falsch.
//
// DER BISS BEISST AN DER ECHTEN MAKRO-NAHT UND UEBER DIE ECHTE .so-GRENZE: zwei real gebaute Module
// aus EINEM Quelltext (r3_mess_gate_stamp_module.cpp), byte-identische Stempel-Literale, einziger
// Unterschied der Praeprozessor-Zustand der Mess-Gates. Am Vor-R-3-Stand sind die beiden sha512_line
// IDENTISCH -> ROT. Mit dem neunten Preimage-Glied "mess-gates" sind sie verschieden -> GRUEN.
//
// NENNER + GEGENPROBE (eine nackte Ungleichung ist kein Befund): der Test prueft ZUERST, dass beide
// Module ueberhaupt laden, dass ihr POD-Layout stimmt, dass beide einen 128-hex-Fingerprint tragen
// UND dass ihre drei Stempel-ZEILEN byte-gleich sind. Waeren die Zeilen verschieden, waere ein
// Fingerprint-Unterschied trivial und bewiese nichts ueber die Gates.
//
// ASCII-only. Keine Mess-CSV-Beruehrung, kein Schreiben ausserhalb stdout.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <dlfcn.h>

#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace cea = ::comdare::cache_engine::abi;

namespace {

int g_fail = 0;

void check_true(std::string const& was, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << was << "\n";
    if (!ok) ++g_fail;
}

struct GeladenesModul {
    void*                        handle = nullptr;
    cea::AnatomyVersionLines const* pod  = nullptr;
    std::string                  pfad;
    std::string                  fehler;
};

using VersionLinesFn = cea::AnatomyVersionLines const* (*)();

[[nodiscard]] GeladenesModul lade(std::string const& pfad) {
    GeladenesModul m;
    m.pfad = pfad;
    // RTLD_LOCAL: die beiden Module exportieren DASSELBE Symbol. Ohne LOCAL koennte der zweite
    // dlopen auf die Definition des ersten aufloesen -- der Test waere dann trivial gruen (bzw.
    // trivial rot) und bewiese nichts. Das ist der wichtigste Handgriff dieser Datei.
    m.handle = ::dlopen(pfad.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (m.handle == nullptr) {
        char const* e = ::dlerror();
        m.fehler      = e != nullptr ? e : "dlopen fehlgeschlagen (keine Diagnose)";
        return m;
    }
    ::dlerror();
    void* sym = ::dlsym(m.handle, "comdare_anatomy_version_lines");
    if (sym == nullptr) {
        char const* e = ::dlerror();
        m.fehler      = std::string{"comdare_anatomy_version_lines fehlt: "} + (e != nullptr ? e : "?");
        return m;
    }
    VersionLinesFn fn = reinterpret_cast<VersionLinesFn>(sym);
    m.pod             = fn();
    if (m.pod == nullptr) m.fehler = "comdare_anatomy_version_lines() lieferte nullptr";
    return m;
}

[[nodiscard]] std::string zeile(char const* p, std::uint64_t n) {
    return p == nullptr ? std::string{"<nullptr>"} : std::string{p, static_cast<std::size_t>(n)};
}

void zeig(char const* titel, GeladenesModul const& m) {
    std::cout << "  -- " << titel << " (" << m.pfad << ")\n";
    if (m.pod == nullptr) {
        std::cout << "     NICHT GELADEN: " << m.fehler << "\n";
        return;
    }
    std::cout << "     stamp_layout_version = " << m.pod->stamp_layout_version << "\n";
    std::cout << "     organ_line           = " << zeile(m.pod->organ_line, m.pod->organ_len) << "\n";
    std::cout << "     system_line          = " << zeile(m.pod->system_line, m.pod->system_len) << "\n";
    std::cout << "     measurement_line     = " << zeile(m.pod->measurement_line, m.pod->measurement_len) << "\n";
    std::cout << "     sha512_line          = " << zeile(m.pod->sha512_line, m.pod->sha512_len) << "\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string pfad_an;
    std::string pfad_aus;
    for (int i = 1; i < argc; ++i) {
        std::string_view const a{argv[i]};
        if (a.rfind("--an=", 0) == 0) pfad_an = std::string{a.substr(5)};
        else if (a.rfind("--aus=", 0) == 0)
            pfad_aus = std::string{a.substr(6)};
    }
    std::cout << "== R-3 BISS: der Mess-Gate-Zustand der TU diskriminiert den Tier-Fingerprint ==\n";
    if (pfad_an.empty() || pfad_aus.empty()) {
        std::cout << "  [ERR] Aufruf: test_r3_mess_gate_fingerprint_biss --an=<modul.so> --aus=<modul.so>\n";
        return 2;
    }

    GeladenesModul const an  = lade(pfad_an);
    GeladenesModul const aus = lade(pfad_aus);

    std::cout << "\n---- (a) NENNER: beide Module laden und tragen einen vollstaendigen Stempel ----\n";
    zeig("GATES AN ", an);
    zeig("GATES AUS", aus);
    check_true("(a) GATES-AN-Modul geladen", an.pod != nullptr);
    check_true("(a) GATES-AUS-Modul geladen", aus.pod != nullptr);
    if (an.pod == nullptr || aus.pod == nullptr) {
        std::cout << "\n== Fehler: " << (g_fail + 1) << " (ohne beide Module ist keine Aussage moeglich) ==\n";
        return 1;
    }
    check_true("(a) POD-Layout AN  == kAnatomyVersionLinesLayout",
               an.pod->stamp_layout_version == cea::kAnatomyVersionLinesLayout);
    check_true("(a) POD-Layout AUS == kAnatomyVersionLinesLayout",
               aus.pod->stamp_layout_version == cea::kAnatomyVersionLinesLayout);
    check_true("(a) AN  traegt 128 Hex-Zeichen Fingerprint", an.pod->sha512_len == 128);
    check_true("(a) AUS traegt 128 Hex-Zeichen Fingerprint", aus.pod->sha512_len == 128);

    std::cout << "\n---- (b) GEGENPROBE: die drei Stempel-ZEILEN sind byte-gleich ----\n";
    std::string const organ_an   = zeile(an.pod->organ_line, an.pod->organ_len);
    std::string const organ_aus  = zeile(aus.pod->organ_line, aus.pod->organ_len);
    std::string const sys_an     = zeile(an.pod->system_line, an.pod->system_len);
    std::string const sys_aus    = zeile(aus.pod->system_line, aus.pod->system_len);
    std::string const mess_an    = zeile(an.pod->measurement_line, an.pod->measurement_len);
    std::string const mess_aus   = zeile(aus.pod->measurement_line, aus.pod->measurement_len);
    check_true("(b) Organ-Zeile identisch (der Unterschied liegt NICHT in den Literalen)", organ_an == organ_aus);
    check_true("(b) System-Zeile identisch", sys_an == sys_aus);
    check_true("(b) Mess-Tooling-Zeile identisch -- Glied [3] SIEHT den Gate-Unterschied nicht",
               mess_an == mess_aus);

    std::cout << "\n---- (c) BISS: die Fingerprints MUESSEN sich unterscheiden ----\n";
    std::string const fp_an  = zeile(an.pod->sha512_line, an.pod->sha512_len);
    std::string const fp_aus = zeile(aus.pod->sha512_line, aus.pod->sha512_len);
    std::cout << "     fp(GATES AN ) = " << fp_an << "\n";
    std::cout << "     fp(GATES AUS) = " << fp_aus << "\n";
    check_true("(c) fp(GATES AN) != fp(GATES AUS) -- sonst meldet dll_is_current STILL 'current' ueber "
               "eine Mess-Ausstattungs-Grenze hinweg (falscher Skip)",
               fp_an != fp_aus);

    ::dlclose(an.handle);
    ::dlclose(aus.handle);
    std::cout << "\n== Fehler: " << g_fail << " ==\n";
    return g_fail == 0 ? 0 : 1;
}
