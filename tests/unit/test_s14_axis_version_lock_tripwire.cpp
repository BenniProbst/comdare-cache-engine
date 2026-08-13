// S-14a BUMP-WACHE TEIL 1 -- DER RIEGEL (Task #33; Plan 13.3 #90 'Teil 1: der Riegel'; Owner KON17-03
// 'Wache Modular erweitern und in Detail-Klassen splitten'). ctest-Tripwire ueber das Werkzeug
// tools/axis_version_lock (Target comdare_axis_version_lock, EXCLUDE_FROM_ALL -- die CMake-Registrierung
// haengt es als Fixture-Dependency an, 2-Pass-Muster der Tool-Gates wie comdare_f15_compare_cli).
//
// WAS DER TEST BEWEIST (Koeder A-E, jeder wird GEFAHREN, nie nur behauptet):
//   A  1 Byte in einer ECHTEN Traeger-Kopie aendern OHNE Version-Bump -> --check Exit 1 + Digest-Meldung,
//      die genau diese Datei nennt. Ruecknahme -> Exit 0.
//   B  dieselbe Aenderungsklasse am synthetischen Traeger: OHNE Bump -> Exit 1; MIT Bump (X.Y.Z-Tripel
//      echt groesser) -> Exit 0 ('MIT Version-Bump').
//   C  Datei mit algo_version-Literal, die NICHT im Lock steht -> 'unlocked' -> Exit 1.
//   D  Lock-Eintrag OHNE Datei (verwaist) -> Exit 1. DIES IST DIE DESIGNIERTE ROT-ZUERST-STELLE:
//      das v1-Tool iterierte nur ueber argv und sah verwaiste Lock-Eintraege NIE (am 13.08.2026 literal
//      belegt: Phantom-Eintrag im Lock, v1 antwortet 'GRUEN alle 2 Strategie-Header konsistent', Exit 0).
//   E  unparsbares algo_version-Literal (Sentinel) nach Inhalts-Aenderung -> Exit 1.
//
// HERMETIK: der Test kopiert die drei Traeger-Baeume (heuristik/, axes/, topics/queuing/) in ein
// Temp-Verzeichnis (comdare_test_tmp.hpp, worktree-getrennt) und faehrt das Tool per --root NUR auf den
// Kopien. Der Quellbaum wird ausschliesslich GELESEN; golden-/TABU-Fixtures werden nicht beruehrt.
//
// ANKER MIT NENNER (Riegel-Schaerfung ii): heuristik discovered == 6, organ discovered >= 120 -- geparst
// aus den BESTAND-Zeilen der Tool-Ausgabe, nie aus einer eigenen Zweit-Zaehlung (eine Zaehlung, eine
// Wahrheit). Der CT-Zaehler kAllRegisteredOrganVariantCount wird daneben NUR GELOGGT und nie
// gleichgesetzt: Datei != Variante (eine Datei kann mehrere Varianten tragen, Varianten koennen
// deaktiviert sein).
//
// SUBPROZESS-FORM: std::system mit Shell-Redirektion wie tests/unit/test_w0b_durchstich_kette.cpp --
// dieser Test laeuft auf den Linux-Runnern (baremetal); die POSIX-Statusauswertung ist unten gekapselt.

#include "comdare_test_tmp.hpp"

// CT-Zaehler-LOG (bewusst schwere TU, Blockvorbild test_reflect_versions_all17): Datei-Zaehler des
// Werkzeugs und Varianten-Zaehler der Registry nebeneinander sichtbar machen, OHNE sie gleichzusetzen.
#include <builder/experiment_tree/axis_variant_version_table.hpp>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace {

namespace fs = std::filesystem;

struct ToolRun {
    int         exit_code = -1;
    std::string output; // stdout+stderr gemeinsam
};

std::string g_tool;     // Pfad zur comdare_axis_version_lock-Binary (argv[1])
fs::path    g_tmp_root; // Wurzel der Kopien
fs::path    g_lockfile; // Lock der Kopien

[[nodiscard]] std::string slurp(fs::path const& p) {
    std::ifstream      f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void spew(fs::path const& p, std::string const& bytes) {
    fs::create_directories(p.parent_path());
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    f << bytes;
}

/// Faehrt das Tool mit --root auf den Kopien. mode = "--write" | "--check".
[[nodiscard]] ToolRun run_tool(std::string const& mode) {
    ToolRun           r;
    fs::path const    out = g_tmp_root / "tool_out.txt";
    std::string const cmd = "\"" + g_tool + "\" " + mode + " \"" + g_lockfile.string() + "\" --root \"" +
                            g_tmp_root.string() + "\" > \"" + out.string() + "\" 2>&1";
    int const         rc  = std::system(cmd.c_str());
#if defined(_WIN32)
    r.exit_code = rc;
#else
    r.exit_code = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
#endif
    r.output = slurp(out);
    return r;
}

/// Protokolliert eine Tool-Ausgabe woertlich in das ctest-Log (Koeder-Beleg gehoert in die Ausgabe).
void protokoll(std::string const& titel, ToolRun const& r) {
    std::printf("---- %s (exit=%d) ----\n%s----\n", titel.c_str(), r.exit_code, r.output.c_str());
}

[[nodiscard]] bool contains(std::string const& hay, std::string const& needle) {
    return hay.find(needle) != std::string::npos;
}

/// Liest 'key=<zahl>' aus der ersten Zeile der Ausgabe, die 'zeilen_marke' enthaelt. -1 = nicht gefunden.
[[nodiscard]] long parse_counter(std::string const& out, std::string const& zeilen_marke, std::string const& key) {
    std::istringstream ls(out);
    std::string        line;
    while (std::getline(ls, line)) {
        if (!contains(line, zeilen_marke)) continue;
        std::size_t const kp = line.find(key + "=");
        if (kp == std::string::npos) return -1;
        return std::strtol(line.c_str() + kp + key.size() + 1, nullptr, 10);
    }
    return -1;
}

/// Eintrag der Lock-Datei v2: <category> <version> <sha256> <relpath>.
struct LockZeile {
    std::string category;
    std::string version;
    std::string digest;
    std::string rel;
};

[[nodiscard]] std::vector<LockZeile> parse_lock_v2(fs::path const& lock) {
    std::vector<LockZeile> out;
    std::ifstream          f(lock);
    std::string            line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ls(line);
        LockZeile          z;
        if (ls >> z.category >> z.version >> z.digest >> z.rel) out.push_back(z);
    }
    return out;
}

int fehler(int schritt, std::string const& text, ToolRun const& r) {
    std::printf("FEHLER Schritt %d: %s\n", schritt, text.c_str());
    protokoll("letzte Tool-Ausgabe", r);
    return schritt;
}

// Der synthetische Traeger: ein echtes algo_version-String-Literal, sonst inert. Er lebt NUR in der
// Temp-Kopie unter axes/ (Discovery-Home), nie im Quellbaum.
constexpr char const* kSynthRel   = "libs/cache_engine/axes/s14_synth/axis_s14_synth_koeder.hpp";
constexpr char const* kSynthBasis = "// s14 synthetischer traeger -- nur fuer den tripwire-test\n"
                                    "#pragma once\n"
                                    "#include <string_view>\n"
                                    "namespace comdare::s14_synth {\n"
                                    "struct SynthKoeder {\n"
                                    "    static constexpr std::string_view algo_version = \"1.0.0.c\";\n"
                                    "};\n"
                                    "} // namespace comdare::s14_synth\n";

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::printf("usage: test_s14_axis_version_lock_tripwire <tool> <source-root>\n");
        return 90;
    }
    g_tool = argv[1];
    fs::path const source_root{argv[2]};

    // Schritt 1: Temp-Wurzel vorbereiten (deterministisch je Build-Verzeichnis, Reste raeumen).
    g_tmp_root = comdare::test::user_tmp_dir() / "s14_axis_lock_tripwire";
    std::error_code ec;
    fs::remove_all(g_tmp_root, ec);
    fs::create_directories(g_tmp_root);
    g_lockfile = g_tmp_root / "axis_version.lock";

    // Schritt 2: die drei Traeger-Baeume KOPIEREN (Quellbaum nur lesen).
    for (char const* sub :
         {"libs/cache_engine/heuristik", "libs/cache_engine/axes", "libs/cache_engine/topics/queuing"}) {
        fs::path const from = source_root / sub;
        if (!fs::exists(from)) {
            std::printf("FEHLER Schritt 2: Quellbaum fehlt: %s\n", from.string().c_str());
            return 2;
        }
        fs::create_directories((g_tmp_root / sub).parent_path());
        fs::copy(from, g_tmp_root / sub, fs::copy_options::recursive);
    }

    // Schritt 3: synthetischen Traeger anlegen, DANN Lock schreiben -- er ist damit mitverriegelt.
    fs::path const synth = g_tmp_root / kSynthRel;
    spew(synth, kSynthBasis);

    ToolRun w = run_tool("--write");
    protokoll("--write Baseline (Kopien)", w);
    if (w.exit_code != 0) return fehler(3, "--write muss Exit 0 liefern", w);

    // Schritt 4: ANKER MIT NENNER aus den BESTAND-Zeilen des Tools (heuristik==6, organ>=120).
    long const h_disc = parse_counter(w.output, "BESTAND heuristik", "discovered");
    long const o_disc = parse_counter(w.output, "BESTAND organ", "discovered");
    long const o_trae = parse_counter(w.output, "BESTAND organ", "traeger");
    std::printf("ANKER: heuristik discovered=%ld (soll ==6), organ discovered=%ld (soll >=120, "
                "davon traeger=%ld)\n",
                h_disc, o_disc, o_trae);
    if (h_disc != 6) return fehler(4, "Anker heuristik==6 verfehlt", w);
    if (o_disc < 120) return fehler(4, "Anker organ>=120 verfehlt", w);

    // Schritt 5: Roundtrip -- der frisch geschriebene Lock prueft gruen.
    ToolRun c0 = run_tool("--check");
    if (c0.exit_code != 0) return fehler(5, "Roundtrip --check muss Exit 0 liefern", c0);

    // Schritt 6: Ziele aus dem LOCK waehlen (nie hartkodierte Dateinamen -- S-6-Umzuege duerfen den
    // Test nicht brechen): der synthetische Eintrag + der erste ECHTE organ-Traeger (version != '-').
    std::vector<LockZeile> const zeilen = parse_lock_v2(g_lockfile);
    std::string                  ziel_a;
    bool                         synth_im_lock = false;
    for (LockZeile const& z : zeilen) {
        if (z.rel == kSynthRel) synth_im_lock = true;
        if (ziel_a.empty() && z.category == "organ" && z.version != "-" && z.rel != kSynthRel) ziel_a = z.rel;
    }
    if (!synth_im_lock) return fehler(6, "synthetischer Traeger fehlt im Lock", c0);
    if (ziel_a.empty()) return fehler(6, "kein echter organ-Traeger im Lock gefunden", c0);
    std::printf("KOEDER-A-Ziel (erster echter organ-Traeger im Lock): %s\n", ziel_a.c_str());

    // Schritt 7 -- KOEDER A: 1-Byte-Klasse an der ECHTEN Kopie ohne Bump -> ROT, Ruecknahme -> GRUEN.
    fs::path const    a_pfad    = g_tmp_root / ziel_a;
    std::string const a_urbytes = slurp(a_pfad);
    spew(a_pfad, a_urbytes + "// s14 koeder A: drift ohne bump\n");
    ToolRun a_rot = run_tool("--check");
    protokoll("KOEDER A (drift ohne Bump, echte Kopie)", a_rot);
    if (a_rot.exit_code != 1) return fehler(7, "Koeder A muss Exit 1 liefern", a_rot);
    if (!contains(a_rot.output, ziel_a)) return fehler(7, "Koeder A muss die Datei nennen", a_rot);
    if (!contains(a_rot.output, "OHNE")) return fehler(7, "Koeder A muss die OHNE-Bump-Diagnose tragen", a_rot);
    spew(a_pfad, a_urbytes);
    ToolRun a_gruen = run_tool("--check");
    if (a_gruen.exit_code != 0) return fehler(7, "Koeder-A-Ruecknahme muss Exit 0 liefern", a_gruen);

    // Schritt 8 -- KOEDER B: dieselbe Aenderung am synthetischen Traeger, erst OHNE, dann MIT Bump.
    std::string geaendert = kSynthBasis;
    geaendert += "// s14 koeder B: inhaltsaenderung\n";
    spew(synth, geaendert);
    ToolRun b_rot = run_tool("--check");
    protokoll("KOEDER B ohne Bump", b_rot);
    if (b_rot.exit_code != 1) return fehler(8, "Koeder B ohne Bump muss Exit 1 liefern", b_rot);
    std::string gebumpt = geaendert;
    gebumpt.replace(gebumpt.find("\"1.0.0.c\""), 9, "\"1.1.0.c\"");
    spew(synth, gebumpt);
    ToolRun b_gruen = run_tool("--check");
    protokoll("KOEDER B mit Bump 1.0.0.c -> 1.1.0.c", b_gruen);
    if (b_gruen.exit_code != 0) return fehler(8, "Koeder B mit Bump muss Exit 0 liefern", b_gruen);
    if (!contains(b_gruen.output, "MIT Version-Bump"))
        return fehler(8, "Koeder B muss die MIT-Bump-Diagnose tragen", b_gruen);
    spew(synth, kSynthBasis);

    // Schritt 9 -- KOEDER C: Traeger-Datei, die NICHT im Lock steht -> 'unlocked' ROT.
    fs::path const unlocked = g_tmp_root / "libs/cache_engine/axes/s14_synth/axis_s14_synth_unlocked.hpp";
    spew(unlocked, std::string(kSynthBasis));
    ToolRun c_rot = run_tool("--check");
    protokoll("KOEDER C (Datei nicht im Lock)", c_rot);
    if (c_rot.exit_code != 1) return fehler(9, "Koeder C muss Exit 1 liefern", c_rot);
    if (!contains(c_rot.output, "unlocked")) return fehler(9, "Koeder C muss 'unlocked' melden", c_rot);
    fs::remove(unlocked);
    ToolRun c_gruen = run_tool("--check");
    if (c_gruen.exit_code != 0) return fehler(9, "Koeder-C-Ruecknahme muss Exit 0 liefern", c_gruen);

    // Schritt 10 -- KOEDER D (designierte Rot-zuerst-Stelle): Lock-Eintrag OHNE Datei -> ROT.
    fs::remove(synth);
    ToolRun d_rot = run_tool("--check");
    protokoll("KOEDER D (Lock-Eintrag ohne Datei)", d_rot);
    if (d_rot.exit_code != 1) return fehler(10, "Koeder D muss Exit 1 liefern", d_rot);
    if (!contains(d_rot.output, "verwaist")) return fehler(10, "Koeder D muss 'verwaist' melden", d_rot);
    if (!contains(d_rot.output, kSynthRel)) return fehler(10, "Koeder D muss den Eintrag nennen", d_rot);
    spew(synth, kSynthBasis);
    ToolRun d_gruen = run_tool("--check");
    if (d_gruen.exit_code != 0) return fehler(10, "Koeder-D-Ruecknahme muss Exit 0 liefern", d_gruen);

    // Schritt 11 -- KOEDER E: unparsbares Literal (Sentinel) nach Inhalts-Aenderung -> ROT.
    std::string kaputt = kSynthBasis;
    kaputt.replace(kaputt.find("\"1.0.0.c\""), 9, "\"kaputt\"");
    kaputt += "// s14 koeder E: inhaltsaenderung\n";
    spew(synth, kaputt);
    ToolRun e_rot = run_tool("--check");
    protokoll("KOEDER E (unparsbares Literal)", e_rot);
    if (e_rot.exit_code != 1) return fehler(11, "Koeder E muss Exit 1 liefern", e_rot);
    if (!contains(e_rot.output, "unparsbar")) return fehler(11, "Koeder E muss 'unparsbar' melden", e_rot);
    spew(synth, kSynthBasis);
    ToolRun e_gruen = run_tool("--check");
    if (e_gruen.exit_code != 0) return fehler(11, "Koeder-E-Ruecknahme muss Exit 0 liefern", e_gruen);

    // Schritt 12: CT-Zaehler NUR LOGGEN, NIE gleichsetzen (Datei != Variante): eine Datei kann mehrere
    // Varianten tragen, Varianten koennen registriert-aber-deaktiviert sein. Der Wert steht hier, damit
    // beide Zahlen im selben Log sichtbar altern.
    std::printf("CT-ZAEHLER-LOG: kAllRegisteredOrganVariantCount=%zu neben organ discovered=%ld "
                "traeger=%ld -- BEWUSST NICHT GLEICHGESETZT (Datei != Variante)\n",
                ::comdare::cache_engine::builder::experiment::kAllRegisteredOrganVariantCount, o_disc, o_trae);

    fs::remove_all(g_tmp_root, ec);
    std::printf("test_s14_axis_version_lock_tripwire: GRUEN (Koeder A-E gefahren, Anker 6/%ld gehalten)\n", o_disc);
    return 0;
}
