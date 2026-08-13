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
//   F  unlesbarer NEUZUGANG (neue Traeger-*.hpp, NICHT im Lock, chmod 000) -> Exit 2 + Datei-Name.
//      ROT-ZUERST 13.08.2026 (Pflicht-Fixup, Luecke 1): die organ-Discovery uebersprang unlesbare
//      Dateien still ('continue' ohne Signal) -- literal belegt: --check Exit 0 'GRUEN bestand
//      konsistent -- 158 Dateien' UND --write regenerierte das Register OHNE die Datei (0 Treffer).
//      F2 pinnt den heuristik-Kontrast: dort war unlesbar schon IMMER Exit 2 (die heuristik-Discovery
//      liest keine Bytes, die Datei faellt im Digest-Schritt laut um -- Objekt-Verifikation 13.08.).
//   G  Register-Strenge des Lock-Parsers (ROT-ZUERST 13.08.2026, Pflicht-Fixup, Luecke 2): G1
//      Kopfzeile mit viertem Token (dieselbe Grenze faengt Pfade MIT Leerzeichen) und G2 Rest-Muell
//      hinter dem Digest wurden VORHER still verschluckt (--check Exit 0 GRUEN, literal belegt);
//      G3 64 Nicht-Hex-Zeichen gingen als 'Digest' durch den Parser und der Fehler erschien als
//      DATEI-Drift ('erwartet zzzz...'), nie als Register-Fehler; G4 dupliziertes Record-Paar
//      wurde still 'last wins' aufgeloest (Exit 0). NACHHER: alle vier ROT (Exit 1) als
//      Formatfehler mit der literalen Zeile bzw. dem Pfad.
//
// PFLICHT-FIXUP 2 (13.08.2026, Owner-Dauerregel 'Luecke = Behebung ist IMMER Pflicht') -- die
// Koeder H..S pinnen die sieben Fund-Gruppen; jeder Kern-Fund wurde VOR dem Fix am gebauten Tool
// literal gefahren (Rot-zuerst-Protokoll im Fixup-Commit):
//   H  (G2, schwerster Fund) FOLGEZUSTAND des Bump-Zweigs: Bump ohne Regen war Exit 0, und JEDER
//      weitere Drift unter der gebumpten Version blieb dauerhaft gruen. Jetzt: Bump => Exit 3
//      'REGEN ERFORDERLICH'; erst der Regen-Commit (--write) segnet den KONKRETEN Inhalt; Drift
//      nach dem Regen ohne neuen Bump => Exit 1.
//   I  (G1a) RUHELAGE: verfaelschte Register-Version bei Digest-Gleichheit war GRUEN/Exit 0
//      (literal belegt) -- jetzt Exit 1 mit beiden Werten.
//   J  (G1b) Sentinel-Literal MIT passendem Register-Digest war GRUEN/Exit 0 (literal belegt;
//      genau der Zustand, den das alte '--write trotz rc=1' committen liess) -- jetzt IMMER rot;
//      --write verweigert bei rc != 0 BYTE-IDENTISCH (kein Truncate vor der Pruefung).
//   K  (G3) LITERAL-LISTE: k_ary traegt zwei Variantenfamilien; Bump am falschen Literal war
//      'OK MIT Version-Bump'/GRUEN, korrekter Bump am zweiten Literal falsches ROT (beides
//      literal belegt). Jetzt: Liste im Register (gleiche Werte komprimiert, ungleiche
//      komma-gefuegt), kein Element darf sinken, mindestens eines muss steigen.
//   L  (G4 + Luecke 7a) HEURISTIK auf Organ-Niveau UND testseitig bewiesen: Drift ohne
//      Marker-Bump => 1, mit Bump => 3, Regen => 0; Marker fehlt/mehrdeutig/Ueberlauf => ROT
//      (vorher '0 + Warnung', --write akzeptierte); Prosa-Zitat mid-line stellt die Version
//      nicht mehr (vorher Version 99 aus dem Zitat, literal belegt).
//   M  (G5a) MINDEST-NENNER: leere Homes ergaben '--check Exit 0, GRUEN -- 0 Dateien' (literal
//      belegt; V-1 woertlich gebrochen) -- jetzt Exit 2 mit beiden Zahlen, --write legt nichts an.
//   N  (G5b) unlesbares UNTERverzeichnis: vorher std::terminate/SIGABRT 134 (literal belegt) --
//      jetzt Exit 2 mit benannter Ursache.
//   O  (G5c) Traeger mit fremder Endung (.h) war vollstaendig unsichtbar (literal belegt) --
//      jetzt ROT mit Namen.
//   P  (G5d) Verzeichnis-Symlink unter einem Home wurde still uebersprungen (Traeger dahinter
//      unsichtbar, literal belegt) -- jetzt Exit 2 mit Namen des Links.
//   Q  (G6a) C++14-Digit-Separator schaltete den char_lit-Modus: ein Literal hinter 1'000.0
//      wurde still als '-' digest-only gelockt (literal belegt) -- jetzt korrekt erfasst.
//   R  (G6b) Literal in einem '#if 0'-Block stellte die Version (literal belegt) -- jetzt ROT
//      'nicht entscheidbar' (Bestand hat 0 solche Faelle, gemessen).
//   S  v1-ABWEISUNG als dauerhafter Koeder: ein Lock mit '# format: v1'-Kopf wird mit klarer
//      Meldung abgewiesen, nie still weitergelesen.
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

// Koeder J braucht den Digest des Sentinel-Inhalts, um das 'committete Sentinel-Register' echt
// nachzustellen -- DIESELBE SHA-256 wie das Werkzeug (keine Zweit-Implementation).
#include <sha256/ctsha.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/stat.h> // ::chmod fuer Koeder F (unlesbarer Neuzugang)
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

/// Faehrt das Tool mit explizitem Lock und Root (Koeder M braucht eine ZWEITE Wurzel).
[[nodiscard]] ToolRun run_tool_mit(std::string const& mode, fs::path const& lock, fs::path const& root) {
    ToolRun           r;
    fs::path const    out = g_tmp_root / "tool_out.txt";
    std::string const cmd = "\"" + g_tool + "\" " + mode + " \"" + lock.string() + "\" --root \"" + root.string() +
                            "\" > \"" + out.string() + "\" 2>&1";
    int const         rc  = std::system(cmd.c_str());
#if defined(_WIN32)
    r.exit_code = rc;
#else
    r.exit_code = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
#endif
    r.output = slurp(out);
    return r;
}

/// Faehrt das Tool mit --root auf den Kopien. mode = "--write" | "--check".
[[nodiscard]] ToolRun run_tool(std::string const& mode) { return run_tool_mit(mode, g_lockfile, g_tmp_root); }

/// SHA-256-Hex eines Strings -- DIESELBE Implementation wie im Werkzeug (Koeder J stellt damit
/// ein 'committetes Sentinel-Register' mit korrektem Digest nach).
[[nodiscard]] std::string sha256_hex_von(std::string const& inhalt) {
    std::vector<std::uint8_t> bytes(inhalt.begin(), inhalt.end());
    auto const d = ::comdare::cache_engine::sha256::sha256(std::span<const std::uint8_t>{bytes.data(), bytes.size()});
    auto const h = ::comdare::cache_engine::sha256::to_hex(d);
    return std::string(h.begin(), h.end());
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

/// Record der Lock-Datei v2 (ZWEI Zeilen): '<category> <version> <relpath>' + '    <sha256>'.
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
        if (line[0] == ' ' || line[0] == '\t') {
            // Digest-Folgezeile gehoert zum zuletzt gelesenen Record-Kopf.
            if (!out.empty() && out.back().digest.empty()) {
                std::istringstream ds(line);
                ds >> out.back().digest;
            }
            continue;
        }
        std::istringstream ls(line);
        LockZeile          z;
        if (ls >> z.category >> z.version >> z.rel) out.push_back(z);
    }
    return out;
}

/// Versions-Token des Lock-Records fuer rel ("" = Record nicht gefunden). Koeder K/L/Q pruefen
/// damit die KANONISCHE Register-Form (komprimiert/gefuegt) am geschriebenen Lock.
[[nodiscard]] std::string lock_version_von(fs::path const& lock, std::string const& rel) {
    for (LockZeile const& z : parse_lock_v2(lock))
        if (z.rel == rel) return z.version;
    return "";
}

int fehler(int schritt, std::string const& text, ToolRun const& r) {
    std::printf("FEHLER Schritt %d: %s\n", schritt, text.c_str());
    protokoll("letzte Tool-Ausgabe", r);
    return schritt;
}

// Der synthetische Traeger: ein echtes algo_version-String-Literal, sonst inert. Er lebt NUR in der
// Temp-Kopie unter axes/ (Discovery-Home), nie im Quellbaum.
constexpr char const* kSynthRel = "libs/cache_engine/axes/s14_synth/axis_s14_synth_koeder.hpp";
// Koeder F: der unlesbare Neuzugang -- gleicher Inhalt wie der synthetische Traeger, eigener Pfad.
constexpr char const* kKoederFRel = "libs/cache_engine/axes/s14_synth/axis_s14_synth_unlesbar.hpp";
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
    // PFLICHT-FIXUP 2 (G2): der akzeptierte Bump ist KEIN Exit 0 mehr -- das Register ist
    // veraltet, und nur der Regen-Commit segnet den konkreten Inhalt. Vorher zementierte genau
    // diese Stelle das Dauerloch (sie forderte Exit 0 und fuhr den Folgezustand nie); den
    // Folgezustand faehrt jetzt Koeder H.
    ToolRun b_regen = run_tool("--check");
    protokoll("KOEDER B mit Bump 1.0.0.c -> 1.1.0.c", b_regen);
    if (b_regen.exit_code != 3) return fehler(8, "Koeder B mit Bump muss Exit 3 liefern (Regen erforderlich)", b_regen);
    if (!contains(b_regen.output, "MIT Version-Bump"))
        return fehler(8, "Koeder B muss die MIT-Bump-Diagnose tragen", b_regen);
    if (!contains(b_regen.output, "REGEN ERFORDERLICH"))
        return fehler(8, "Koeder B muss REGEN ERFORDERLICH melden", b_regen);
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

    // Schritt 13 -- KOEDER F (PFLICHT-FIXUP 13.08.2026, Luecke 1 'fail-open in der Discovery'): ein
    // unlesbarer NEUZUGANG darf nicht still aus der Grundgesamtheit verschwinden. Ein BEREITS
    // gelockter Traeger fiele als verwaist auf (Koeder D) -- nur der Neuzugang fiel durch. NACH dem
    // Fix ist die organ-Discovery fail-closed: Exit 2 mit Datei-NAME, --write scheitert VOR dem
    // Truncate des Locks (das Register regeneriert nie ueber eine selbst geschrumpfte Menge).
    ToolRun leer;
#if !defined(_WIN32)
    fs::path const f_pfad = g_tmp_root / kKoederFRel;
    spew(f_pfad, std::string(kSynthBasis));
    if (::chmod(f_pfad.c_str(), 0) != 0) return fehler(13, "chmod 000 fehlgeschlagen", leer);
    {
        // K13: der Koeder muss beissen KOENNEN. Als root/CAP_DAC_OVERRIDE bleibt die Datei trotz
        // chmod 000 lesbar -- dann LAUT scheitern statt still einen stumpfen Koeder zu fahren.
        std::ifstream probe(f_pfad);
        if (probe.is_open())
            return fehler(13, "Umgebung unterlaeuft chmod 000 (root/CAP_DAC_OVERRIDE?) -- Koeder F beisst nicht", leer);
    }
    ToolRun f_rot = run_tool("--check");
    protokoll("KOEDER F (unlesbarer Neuzugang unter axes/, organ-Discovery)", f_rot);
    if (f_rot.exit_code != 2) return fehler(13, "Koeder F muss Exit 2 liefern (fail-closed)", f_rot);
    if (!contains(f_rot.output, kKoederFRel)) return fehler(13, "Koeder F muss die Datei nennen", f_rot);
    if (!contains(f_rot.output, "nicht lesbar")) return fehler(13, "Koeder F muss 'nicht lesbar' melden", f_rot);
    // PFLICHT-FIXUP 2 (Luecke 7b): BYTEIDENTITAET statt contains() -- ein --write, das trotz
    // Exit 2 regeneriert und nur die unlesbare Datei auslaesst, bestuende den contains-Check
    // (der synth-Record stuende ja wieder drin). Das Lock muss VOR und NACH dem gescheiterten
    // --write byte-gleich sein.
    std::string const lock_vor_f = slurp(g_lockfile);
    ToolRun           f_write    = run_tool("--write");
    protokoll("KOEDER F --write (muss VOR dem Lock-Truncate scheitern)", f_write);
    if (f_write.exit_code != 2) return fehler(13, "Koeder F --write muss Exit 2 liefern", f_write);
    if (slurp(g_lockfile) != lock_vor_f)
        return fehler(13, "Koeder F --write muss die Lock-Datei BYTE-IDENTISCH lassen", f_write);
    (void)::chmod(f_pfad.c_str(), 0644);
    fs::remove(f_pfad);
    ToolRun f_gruen = run_tool("--check");
    if (f_gruen.exit_code != 0) return fehler(13, "Koeder-F-Ruecknahme muss Exit 0 liefern", f_gruen);

    // Schritt 13b -- KOEDER F2, BESTANDS-PIN (bewusst KEIN Rot-zuerst): die heuristik-Discovery liest
    // keine Bytes, ein unlesbarer heuristik-Neuzugang lag schon immer in rels und fiel im
    // Digest-Schritt laut um (Exit 2; Objekt-Verifikation 13.08.2026). Der Pin friert das ein, damit
    // niemand den Zweig spaeter auf die fail-open-Form 'harmonisiert'.
    fs::path const f2_pfad = g_tmp_root / "libs/cache_engine/heuristik/s14_koeder_f2_unlesbar.hpp";
    spew(f2_pfad, "// AXIS_ALGO_VERSION: 1\n#pragma once\n");
    if (::chmod(f2_pfad.c_str(), 0) != 0) return fehler(13, "chmod 000 (F2) fehlgeschlagen", leer);
    ToolRun f2_rot = run_tool("--check");
    protokoll("KOEDER F2 (unlesbarer Neuzugang unter heuristik/ -- fail-closed-Pin)", f2_rot);
    if (f2_rot.exit_code != 2) return fehler(13, "Koeder F2 muss Exit 2 liefern", f2_rot);
    if (!contains(f2_rot.output, "nicht lesbar")) return fehler(13, "Koeder F2 muss 'nicht lesbar' melden", f2_rot);
    (void)::chmod(f2_pfad.c_str(), 0644);
    fs::remove(f2_pfad);
    ToolRun f2_gruen = run_tool("--check");
    if (f2_gruen.exit_code != 0) return fehler(13, "Koeder-F2-Ruecknahme muss Exit 0 liefern", f2_gruen);
#else
    // Koeder F/F2 brauchen den POSIX-chmod-000-Mechanismus; unter Windows LAUT benannt uebersprungen
    // (dieser ctest laeuft auf den Linux-Runnern, s. Kopf).
    std::printf("KOEDER F/F2 UEBERSPRUNGEN: kein chmod-000-Mechanismus unter _WIN32\n");
#endif

    // Schritt 14 -- KOEDER G (PFLICHT-FIXUP 13.08.2026, Luecke 2 'laxer Lock-Parser'): die Wache
    // muss ihr eigenes Register STRENG lesen -- die Lock-Datei ist das Identitaets-Register.
    // Ziel der Manipulationen ist der Record des synthetischen Traegers (nie hartkodierte Pfade).
    std::string const        lock_urbytes = slurp(g_lockfile);
    std::vector<std::string> lock_zeilen;
    {
        std::istringstream lz(lock_urbytes);
        std::string        l;
        while (std::getline(lz, l)) lock_zeilen.push_back(l);
    }
    std::size_t kopf_idx = lock_zeilen.size();
    for (std::size_t i = 0; i + 1 < lock_zeilen.size(); ++i) {
        if (!lock_zeilen[i].empty() && lock_zeilen[i][0] != '#' && lock_zeilen[i][0] != ' ' &&
            contains(lock_zeilen[i], kSynthRel)) {
            kopf_idx = i;
            break;
        }
    }
    if (kopf_idx >= lock_zeilen.size()) return fehler(14, "synth-Record im Lock nicht gefunden", leer);
    auto mit_lock = [&](std::vector<std::string> const& zeilen) {
        std::string neu;
        for (std::string const& l : zeilen) {
            neu += l;
            neu += '\n';
        }
        spew(g_lockfile, neu);
    };
    // G1: viertes Token auf der Kopfzeile. VORHER still verschluckt (Exit 0 GRUEN, literal belegt)
    // -- ein Pfad MIT Leerzeichen wurde damit abgeschnitten und das Urteil traf eine andere Datei.
    std::vector<std::string> man = lock_zeilen;
    man[kopf_idx] += " muell";
    mit_lock(man);
    ToolRun g1 = run_tool("--check");
    protokoll("KOEDER G1 (Kopfzeile mit viertem Token)", g1);
    if (g1.exit_code != 1) return fehler(14, "Koeder G1 muss Exit 1 liefern", g1);
    if (!contains(g1.output, "Zusatz-Token")) return fehler(14, "Koeder G1 muss das Zusatz-Token benennen", g1);
    if (!contains(g1.output, man[kopf_idx])) return fehler(14, "Koeder G1 muss die literale Zeile zeigen", g1);
    // G2: Rest-Muell hinter dem Digest. VORHER still verschluckt (Exit 0 GRUEN, literal belegt).
    man = lock_zeilen;
    man[kopf_idx + 1] += " muell";
    mit_lock(man);
    ToolRun g2 = run_tool("--check");
    protokoll("KOEDER G2 (Digest-Zeile mit Rest-Muell)", g2);
    if (g2.exit_code != 1) return fehler(14, "Koeder G2 muss Exit 1 liefern", g2);
    if (!contains(g2.output, "unparsbare Digest-Zeile"))
        return fehler(14, "Koeder G2 muss als Digest-Formatfehler melden", g2);
    if (!contains(g2.output, man[kopf_idx + 1])) return fehler(14, "Koeder G2 muss die literale Zeile zeigen", g2);
    // G3: 64 Nicht-Hex-Zeichen. VORHER akzeptierte der Parser den Muellstring als Digest
    // (size()==64) und die Meldung beschuldigte die DATEI ('Digest geaendert OHNE ... erwartet
    // zzzz...') statt des Registers. NACHHER: Register-Formatfehler, nie Datei-Drift.
    man               = lock_zeilen;
    man[kopf_idx + 1] = "    " + std::string(64, 'z');
    mit_lock(man);
    ToolRun g3 = run_tool("--check");
    protokoll("KOEDER G3 (Digest = 64 Nicht-Hex-Zeichen)", g3);
    if (g3.exit_code != 1) return fehler(14, "Koeder G3 muss Exit 1 liefern", g3);
    if (!contains(g3.output, "unparsbare Digest-Zeile"))
        return fehler(14, "Koeder G3 muss als Digest-Formatfehler melden (nicht als Datei-Drift)", g3);
    if (contains(g3.output, "Digest geaendert"))
        return fehler(14, "Koeder G3 darf NICHT als Drift der Datei gemeldet werden", g3);
    // G4: dupliziertes Record-Paar (Kopf+Digest adjazent). VORHER still 'last wins' (Exit 0).
    man = lock_zeilen;
    man.insert(man.begin() + static_cast<std::ptrdiff_t>(kopf_idx) + 2, man[kopf_idx]);
    man.insert(man.begin() + static_cast<std::ptrdiff_t>(kopf_idx) + 3, man[kopf_idx + 1]);
    mit_lock(man);
    ToolRun g4 = run_tool("--check");
    protokoll("KOEDER G4 (dupliziertes Record-Paar)", g4);
    if (g4.exit_code != 1) return fehler(14, "Koeder G4 muss Exit 1 liefern", g4);
    if (!contains(g4.output, "doppelter Lock-Record")) return fehler(14, "Koeder G4 muss den Doppel-Record melden", g4);
    if (!contains(g4.output, kSynthRel)) return fehler(14, "Koeder G4 muss den Pfad nennen", g4);
    // Gegenprobe: Original-Lock byteidentisch zurueck -> gruen.
    spew(g_lockfile, lock_urbytes);
    ToolRun g_gruen = run_tool("--check");
    if (g_gruen.exit_code != 0) return fehler(14, "Koeder-G-Ruecknahme muss Exit 0 liefern", g_gruen);

    // Schritt 15 -- KOEDER H (PFLICHT-FIXUP 2, G2 -- der schwerste Fund): FOLGEZUSTAND des
    // Bump-Zweigs. ROT-ZUERST 13.08.2026 literal: Edit1+Bump -> Exit 0; Edit2 mit NEUEM Inhalt,
    // Version unveraendert, Lock ungeregen -> WIEDER Exit 0 -- jeder weitere Drift unter der
    // einmal gebumpten Version blieb dauerhaft gruen. NACHHER ist kein Zustand des Zyklus still:
    // Bump => 3, Drift unterm Bump => 3, erst der Regen (--write) => 0, Drift danach => 1.
    std::string h_edit = kSynthBasis;
    h_edit.replace(h_edit.find("\"1.0.0.c\""), 9, "\"1.1.0.c\"");
    std::string const h_edit1 = h_edit + "// s14 koeder H: edit1 mit bump\n";
    std::string const h_edit2 = h_edit + "// s14 koeder H: edit2 ANDERER inhalt, version unveraendert\n";
    std::string const h_edit3 = h_edit + "// s14 koeder H: edit3 drift NACH dem regen\n";
    spew(synth, h_edit1);
    ToolRun h1 = run_tool("--check");
    protokoll("KOEDER H edit1+bump (Regen offen)", h1);
    if (h1.exit_code != 3) return fehler(15, "Koeder H edit1+bump muss Exit 3 liefern", h1);
    if (!contains(h1.output, "REGEN ERFORDERLICH")) return fehler(15, "Koeder H muss REGEN ERFORDERLICH melden", h1);
    spew(synth, h_edit2);
    ToolRun h2 = run_tool("--check");
    protokoll("KOEDER H edit2 (vorher das Dauerloch: Exit 0)", h2);
    if (h2.exit_code != 3) return fehler(15, "Koeder H edit2 muss Exit 3 liefern (vorher 0 = Dauerloch)", h2);
    ToolRun h3 = run_tool("--write");
    if (h3.exit_code != 0) return fehler(15, "Koeder H Regen (--write) muss Exit 0 liefern", h3);
    ToolRun h4 = run_tool("--check");
    if (h4.exit_code != 0) return fehler(15, "Koeder H nach Regen muss Exit 0 liefern", h4);
    spew(synth, h_edit3);
    ToolRun h5 = run_tool("--check");
    protokoll("KOEDER H edit3 nach Regen ohne neuen Bump", h5);
    if (h5.exit_code != 1) return fehler(15, "Koeder H edit3 muss Exit 1 liefern", h5);
    if (!contains(h5.output, "OHNE gueltigen Version-Bump"))
        return fehler(15, "Koeder H edit3 muss die OHNE-Bump-Diagnose tragen", h5);
    spew(synth, kSynthBasis);
    ToolRun h6 = run_tool("--write");
    if (h6.exit_code != 0) return fehler(15, "Koeder H Aufraeum-Regen muss Exit 0 liefern", h6);
    ToolRun h7 = run_tool("--check");
    if (h7.exit_code != 0) return fehler(15, "Koeder-H-Ruecknahme muss Exit 0 liefern", h7);

    // Schritt 16 -- KOEDER I (G1a): RUHELAGE. ROT-ZUERST 13.08.2026 literal: Register-Version von
    // Hand verfaelscht (Digest-Zeile erhalten) => 'GRUEN bestand konsistent', Exit 0 -- und der
    // naechste echte Vergleich haette gegen die falsche Basis geprueft. NACHHER Exit 1 mit beiden
    // Werten.
    std::string const lock_vor_i = slurp(g_lockfile);
    std::string const i_kopf_alt = "organ 1.0.0.c " + std::string(kSynthRel);
    std::string const i_kopf_neu = "organ 1.9.0.c " + std::string(kSynthRel);
    std::string       lock_i     = lock_vor_i;
    std::size_t const i_kopf_pos = lock_i.find(i_kopf_alt);
    if (i_kopf_pos == std::string::npos) return fehler(16, "synth-Kopfzeile fuer Koeder I nicht gefunden", leer);
    lock_i.replace(i_kopf_pos, i_kopf_alt.size(), i_kopf_neu);
    spew(g_lockfile, lock_i);
    ToolRun i1 = run_tool("--check");
    protokoll("KOEDER I (Register-Version verfaelscht, Digest gleich)", i1);
    if (i1.exit_code != 1) return fehler(16, "Koeder I muss Exit 1 liefern (vorher 0)", i1);
    if (!contains(i1.output, "Version im Register weicht"))
        return fehler(16, "Koeder I muss die Register-Abweichung benennen", i1);
    if (!contains(i1.output, "1.9.0.c")) return fehler(16, "Koeder I muss beide Werte zeigen", i1);
    spew(g_lockfile, lock_vor_i);
    ToolRun i2 = run_tool("--check");
    if (i2.exit_code != 0) return fehler(16, "Koeder-I-Ruecknahme muss Exit 0 liefern", i2);

    // Schritt 17 -- KOEDER J (G1b): SENTINEL TROTZ DIGEST-GLEICHHEIT. ROT-ZUERST 13.08.2026
    // literal: (a) --write ueber algo_version="kaputt" lieferte rc=1, SCHRIEB DAS LOCK ABER
    // TROTZDEM; (b) genau dieses Lock committet => --check dauerhaft GRUEN/Exit 0 (der
    // Digest-Kurzschluss uebersprang die Sentinel-Pruefung). NACHHER: (b) IMMER rot; (a) --write
    // verweigert BYTE-IDENTISCH.
    std::string j_kaputt = kSynthBasis;
    j_kaputt.replace(j_kaputt.find("\"1.0.0.c\""), 9, "\"kaputt\"");
    spew(synth, j_kaputt);
    std::string const j_digest   = sha256_hex_von(j_kaputt);
    std::string const j_alt_kopf = "organ 1.0.0.c " + std::string(kSynthRel);
    std::string const lock_vor_j = slurp(g_lockfile);
    std::string       lock_j     = lock_vor_j;
    std::size_t const j_pos      = lock_j.find(j_alt_kopf);
    if (j_pos == std::string::npos) return fehler(17, "synth-Kopfzeile fuer Koeder J nicht gefunden", leer);
    // Record (Kopf + Digest-Folgezeile) durch den Sentinel-Stand ersetzen -- exakt das Register,
    // das das alte --write bei rc=1 hinterliess.
    std::size_t const j_ende = lock_j.find('\n', lock_j.find('\n', j_pos) + 1);
    lock_j.replace(j_pos, j_ende - j_pos, "organ kaputt " + std::string(kSynthRel) + "\n    " + j_digest);
    spew(g_lockfile, lock_j);
    ToolRun j1 = run_tool("--check");
    protokoll("KOEDER J (Sentinel-Register, Digest gleich -- vorher GRUEN)", j1);
    if (j1.exit_code != 1) return fehler(17, "Koeder J muss Exit 1 liefern (vorher 0)", j1);
    if (!contains(j1.output, "unparsbar")) return fehler(17, "Koeder J muss 'unparsbar' melden", j1);
    std::string const lock_vor_j2 = slurp(g_lockfile);
    ToolRun           j2          = run_tool("--write");
    protokoll("KOEDER J --write ueber Sentinel-Bestand (muss byte-identisch verweigern)", j2);
    if (j2.exit_code != 1) return fehler(17, "Koeder J --write muss Exit 1 liefern", j2);
    if (slurp(g_lockfile) != lock_vor_j2)
        return fehler(17, "Koeder J --write muss die Lock-Datei BYTE-IDENTISCH lassen", j2);
    spew(g_lockfile, lock_vor_j);
    spew(synth, kSynthBasis);
    ToolRun j3 = run_tool("--check");
    if (j3.exit_code != 0) return fehler(17, "Koeder-J-Ruecknahme muss Exit 0 liefern", j3);

    // Schritt 18 -- KOEDER K (G3): LITERAL-LISTE. Der Bestandsfall ist k_ary (zwei Varianten-
    // familien, je eigenes Literal): ROT-ZUERST 13.08.2026 literal am echten k_ary -- Bump am
    // FALSCHEN Literal => 'OK MIT Version-Bump'/GRUEN (Exit 0), korrekter Bump am ZWEITEN
    // Literal => falsches ROT (1.0.0.c -> 1.0.0.c). Der synthetische Zweifach-Traeger faehrt
    // die Klasse; der multi-Anker unten haelt den Bestandsfall im Log fest.
    auto const k_inhalt = [](char const* v1, char const* v2) {
        std::string s = "// s14 zweifach-traeger (koeder K)\n#pragma once\n#include <string_view>\n";
        s += "namespace comdare::s14_synth {\nstruct FamA {\n    static constexpr std::string_view algo_version = \"";
        s += v1;
        s += "\";\n};\nstruct FamB {\n    static constexpr std::string_view algo_version = \"";
        s += v2;
        s += "\";\n};\n} // namespace comdare::s14_synth\n";
        return s;
    };
    fs::path const synth2 = g_tmp_root / "libs/cache_engine/axes/s14_synth/axis_s14_synth_zweifach.hpp";
    spew(synth2, k_inhalt("2.0.0.c", "2.0.0.c"));
    ToolRun k1 = run_tool("--write");
    protokoll("KOEDER K --write mit Zweifach-Traeger (gleiche Literale)", k1);
    if (k1.exit_code != 0) return fehler(18, "Koeder K --write muss Exit 0 liefern", k1);
    std::string const k_rel = "libs/cache_engine/axes/s14_synth/axis_s14_synth_zweifach.hpp";
    if (lock_version_von(g_lockfile, k_rel) != "2.0.0.c")
        return fehler(18, "Koeder K: gleiche Literale muessen KOMPRIMIERT im Register stehen (2.0.0.c)", k1);
    long const k_multi = parse_counter(k1.output, "BESTAND organ", "multi");
    std::printf("KOEDER K multi-Anker: organ multi=%ld (soll >=2: k_ary im Bestand + Zweifach-Traeger)\n", k_multi);
    if (k_multi < 2) return fehler(18, "Koeder K: multi-Zaehler muss k_ary UND den Zweifach-Traeger sehen", k1);
    spew(synth2, k_inhalt("2.0.0.c", "2.1.0.c") + "// s14 koeder K: bump NUR an FamB\n");
    ToolRun k2 = run_tool("--check");
    protokoll("KOEDER K Bump nur am ZWEITEN Literal (vorher falsches ROT)", k2);
    if (k2.exit_code != 3) return fehler(18, "Koeder K Zweit-Literal-Bump muss Exit 3 liefern (vorher 1)", k2);
    if (!contains(k2.output, "MIT Version-Bump")) return fehler(18, "Koeder K muss die MIT-Bump-Diagnose tragen", k2);
    ToolRun k3 = run_tool("--write");
    if (k3.exit_code != 0) return fehler(18, "Koeder K Regen muss Exit 0 liefern", k3);
    if (lock_version_von(g_lockfile, k_rel) != "2.0.0.c,2.1.0.c")
        return fehler(18, "Koeder K: ungleiche Literale muessen KOMMA-GEFUEGT im Register stehen", k3);
    ToolRun k4 = run_tool("--check");
    if (k4.exit_code != 0) return fehler(18, "Koeder K nach Regen muss Exit 0 liefern", k4);
    spew(synth2, k_inhalt("2.0.0.c", "2.1.0.c") + "// s14 koeder K: drift OHNE bump gegen Listen-Register\n");
    ToolRun k5 = run_tool("--check");
    protokoll("KOEDER K Drift ohne Bump gegen Listen-Register", k5);
    if (k5.exit_code != 1) return fehler(18, "Koeder K Drift ohne Bump muss Exit 1 liefern", k5);
    spew(synth2, k_inhalt("1.9.0.c", "2.1.0.c"));
    ToolRun k6 = run_tool("--check");
    protokoll("KOEDER K DOWNGRADE eines Elements (2.0.0.c -> 1.9.0.c)", k6);
    if (k6.exit_code != 1) return fehler(18, "Koeder K Element-Downgrade muss Exit 1 liefern", k6);
    fs::remove(synth2);
    ToolRun k7 = run_tool("--write");
    if (k7.exit_code != 0) return fehler(18, "Koeder K Aufraeum-Regen muss Exit 0 liefern", k7);
    ToolRun k8 = run_tool("--check");
    if (k8.exit_code != 0) return fehler(18, "Koeder-K-Ruecknahme muss Exit 0 liefern", k8);

    // Schritt 19 -- KOEDER L (G4 + Luecke 7a): der heuristik-ROT-Pfad, testseitig bisher
    // UNBEWIESEN -- ausgerechnet die Kategorie, deren stiller Drei-Wochen-Ausfall der Anlass
    // fuer S-14 war. Dazu die drei Marker-Schaerfungen (ROT-ZUERST 13.08.2026 literal: Prosa-
    // Zitat 'AXIS_ALGO_VERSION: 99' VOR dem Marker stellte Version 99; Marker 2^64 wurde
    // modulo zu Version 0; fehlender Marker war '0 + Warnung' und --write akzeptierte).
    fs::path const    heur     = g_tmp_root / "libs/cache_engine/heuristik/s14_synth_heuristik_koeder.hpp";
    std::string const heur_rel = "libs/cache_engine/heuristik/s14_synth_heuristik_koeder.hpp";
    spew(heur, "// AXIS_ALGO_VERSION: 3\n#pragma once\n// s14 koeder L basis\n");
    ToolRun l1 = run_tool("--write");
    if (l1.exit_code != 0) return fehler(19, "Koeder L --write muss Exit 0 liefern", l1);
    if (lock_version_von(g_lockfile, heur_rel) != "3")
        return fehler(19, "Koeder L: Marker-Version 3 muss im Register stehen", l1);
    spew(heur, "// AXIS_ALGO_VERSION: 3\n#pragma once\n// s14 koeder L basis\n// drift ohne bump\n");
    ToolRun l2 = run_tool("--check");
    protokoll("KOEDER L heuristik-Drift OHNE Marker-Bump", l2);
    if (l2.exit_code != 1) return fehler(19, "Koeder L Drift ohne Bump muss Exit 1 liefern", l2);
    if (!contains(l2.output, heur_rel)) return fehler(19, "Koeder L muss die Datei nennen", l2);
    if (!contains(l2.output, "OHNE")) return fehler(19, "Koeder L muss die OHNE-Bump-Diagnose tragen", l2);
    spew(heur, "// AXIS_ALGO_VERSION: 4\n#pragma once\n// s14 koeder L basis\n// drift MIT bump\n");
    ToolRun l3 = run_tool("--check");
    protokoll("KOEDER L heuristik-Drift MIT Marker-Bump 3 -> 4", l3);
    if (l3.exit_code != 3) return fehler(19, "Koeder L Drift mit Bump muss Exit 3 liefern", l3);
    if (!contains(l3.output, "MIT Version-Bump")) return fehler(19, "Koeder L muss die MIT-Bump-Diagnose tragen", l3);
    ToolRun l4 = run_tool("--write");
    if (l4.exit_code != 0) return fehler(19, "Koeder L Regen muss Exit 0 liefern", l4);
    ToolRun l5 = run_tool("--check");
    if (l5.exit_code != 0) return fehler(19, "Koeder L nach Regen muss Exit 0 liefern", l5);
    spew(heur, "#pragma once\n// s14 koeder L: marker ENTFERNT\n");
    ToolRun l6 = run_tool("--check");
    protokoll("KOEDER L Marker fehlt (vorher: '0' + Warnung, gruen-faehig)", l6);
    if (l6.exit_code != 1) return fehler(19, "Koeder L fehlender Marker muss Exit 1 liefern", l6);
    if (!contains(l6.output, "Marker")) return fehler(19, "Koeder L muss den fehlenden Marker benennen", l6);
    std::string const lock_vor_l7 = slurp(g_lockfile);
    ToolRun           l7          = run_tool("--write");
    if (l7.exit_code != 1) return fehler(19, "Koeder L --write ohne Marker muss Exit 1 liefern", l7);
    if (slurp(g_lockfile) != lock_vor_l7)
        return fehler(19, "Koeder L --write muss die Lock-Datei BYTE-IDENTISCH lassen", l7);
    spew(heur, "// Prosa zitiert mid-line: siehe AXIS_ALGO_VERSION: 99 im Kopf der Wache\n"
               "// AXIS_ALGO_VERSION: 5\n#pragma once\n");
    ToolRun l8 = run_tool("--write");
    protokoll("KOEDER L Prosa-Zitat vor echtem Marker (vorher: Version 99)", l8);
    if (l8.exit_code != 0) return fehler(19, "Koeder L Prosa-Zitat: --write muss Exit 0 liefern", l8);
    if (lock_version_von(g_lockfile, heur_rel) != "5")
        return fehler(19, "Koeder L: das mid-line-Zitat darf die Version nicht stellen (soll 5)", l8);
    spew(heur, "// AXIS_ALGO_VERSION: 18446744073709551616\n#pragma once\n");
    ToolRun l9 = run_tool("--check");
    protokoll("KOEDER L Ueberlauf-Marker 2^64 (vorher: modulo => Version 0)", l9);
    if (l9.exit_code != 1) return fehler(19, "Koeder L Ueberlauf muss Exit 1 liefern", l9);
    if (!contains(l9.output, "unparsbar")) return fehler(19, "Koeder L Ueberlauf muss 'unparsbar' melden", l9);
    spew(heur, "// AXIS_ALGO_VERSION: 6\n// AXIS_ALGO_VERSION: 7\n#pragma once\n");
    ToolRun l10 = run_tool("--check");
    protokoll("KOEDER L zwei Marker-Zeilen (mehrdeutig)", l10);
    if (l10.exit_code != 1) return fehler(19, "Koeder L mehrdeutig muss Exit 1 liefern", l10);
    if (!contains(l10.output, "mehrdeutig")) return fehler(19, "Koeder L muss 'mehrdeutig' melden", l10);
    fs::remove(heur);
    ToolRun l11 = run_tool("--write");
    if (l11.exit_code != 0) return fehler(19, "Koeder L Aufraeum-Regen muss Exit 0 liefern", l11);
    ToolRun l12 = run_tool("--check");
    if (l12.exit_code != 0) return fehler(19, "Koeder-L-Ruecknahme muss Exit 0 liefern", l12);

    // Schritt 20 -- KOEDER M (G5a): MINDEST-NENNER. ROT-ZUERST 13.08.2026 literal: leere Homes
    // (Verzeichnisse da, null Traeger) => --write Exit 0 mit leerem Register und --check Exit 0
    // 'GRUEN bestand konsistent -- 0 Dateien' -- V-1 woertlich gebrochen. NACHHER Exit 2 mit
    // beiden Zahlen; --write legt kein Lock an.
    fs::path const leer_root = g_tmp_root / "s14_leer_root";
    fs::create_directories(leer_root / "libs/cache_engine/heuristik");
    fs::create_directories(leer_root / "libs/cache_engine/axes");
    fs::create_directories(leer_root / "libs/cache_engine/topics/queuing");
    fs::path const leer_lock = leer_root / "leer.lock";
    ToolRun        m1        = run_tool_mit("--write", leer_lock, leer_root);
    protokoll("KOEDER M --write ueber leere Homes", m1);
    if (m1.exit_code != 2) return fehler(20, "Koeder M --write muss Exit 2 liefern (vorher 0)", m1);
    if (!contains(m1.output, "Mindest-Nenner")) return fehler(20, "Koeder M muss den Nenner benennen", m1);
    if (fs::exists(leer_lock)) return fehler(20, "Koeder M --write darf kein Lock anlegen", m1);
    ToolRun m2 = run_tool_mit("--check", g_lockfile, leer_root);
    protokoll("KOEDER M --check ueber leere Homes", m2);
    if (m2.exit_code != 2) return fehler(20, "Koeder M --check muss Exit 2 liefern (vorher 0/1)", m2);
    fs::remove_all(leer_root, ec);

#if !defined(_WIN32)
    // Schritt 21 -- KOEDER N (G5b): unlesbares UNTERverzeichnis. ROT-ZUERST 13.08.2026 literal:
    // recursive_directory_iterator warf filesystem_error => std::terminate/SIGABRT (Shell: 134),
    // nicht der deklarierte Exit 2. NACHHER: gefangen, benannt, Exit 2.
    fs::path const n_dir = g_tmp_root / "libs/cache_engine/axes/s14_koeder_dir_unlesbar";
    fs::create_directories(n_dir);
    if (::chmod(n_dir.c_str(), 0) != 0) return fehler(21, "chmod 000 (N) fehlgeschlagen", leer);
    {
        // K13: der Koeder muss beissen KOENNEN -- als root/CAP_DAC_OVERRIDE bleibt das
        // Verzeichnis trotz chmod 000 iterierbar, dann LAUT scheitern.
        bool wirft = false;
        try {
            for (auto const& probe_eintrag : fs::directory_iterator(n_dir)) (void)probe_eintrag;
        } catch (fs::filesystem_error const&) { wirft = true; }
        if (!wirft) {
            (void)::chmod(n_dir.c_str(), 0755);
            return fehler(21, "Umgebung unterlaeuft chmod 000 (root?) -- Koeder N beisst nicht", leer);
        }
    }
    ToolRun n1 = run_tool("--check");
    protokoll("KOEDER N (unlesbares Unterverzeichnis -- vorher SIGABRT 134)", n1);
    if (n1.exit_code != 2) return fehler(21, "Koeder N muss Exit 2 liefern (vorher Abort)", n1);
    if (!contains(n1.output, "abgebrochen")) return fehler(21, "Koeder N muss den Abbruch benennen", n1);
    (void)::chmod(n_dir.c_str(), 0755);
    fs::remove_all(n_dir, ec);
    ToolRun n2 = run_tool("--check");
    if (n2.exit_code != 0) return fehler(21, "Koeder-N-Ruecknahme muss Exit 0 liefern", n2);
#else
    std::printf("KOEDER N UEBERSPRUNGEN: kein chmod-000-Mechanismus unter _WIN32\n");
#endif

    // Schritt 22 -- KOEDER O (G5c): ENDUNGS-GRENZE. ROT-ZUERST 13.08.2026 literal: ein Traeger
    // mit .h-Endung (echtes Literal) war vollstaendig unsichtbar -- GRUEN, Exit 0, nie im
    // Bestand, nie 'unlocked'. NACHHER ROT mit Namen.
    fs::path const o_pfad = g_tmp_root / "libs/cache_engine/axes/s14_synth/axis_s14_falsche_endung.h";
    spew(o_pfad, std::string(kSynthBasis));
    ToolRun o1 = run_tool("--check");
    protokoll("KOEDER O (Traeger mit .h-Endung -- vorher unsichtbar)", o1);
    if (o1.exit_code != 1) return fehler(22, "Koeder O muss Exit 1 liefern (vorher 0)", o1);
    if (!contains(o1.output, "Endung")) return fehler(22, "Koeder O muss die Endungs-Grenze benennen", o1);
    if (!contains(o1.output, "axis_s14_falsche_endung.h")) return fehler(22, "Koeder O muss die Datei nennen", o1);
    fs::remove(o_pfad);
    ToolRun o2 = run_tool("--check");
    if (o2.exit_code != 0) return fehler(22, "Koeder-O-Ruecknahme muss Exit 0 liefern", o2);

#if !defined(_WIN32)
    // Schritt 23 -- KOEDER P (G5d): VERZEICHNIS-SYMLINK. ROT-ZUERST 13.08.2026 literal: der
    // Iterator deszendiert Verzeichnis-Symlinks nicht -- ein Traeger dahinter blieb unsichtbar,
    // GRUEN/Exit 0. NACHHER Exit 2 mit dem Ort des LINKS (relevant ab der #16-Umgliederung).
    fs::path const p_extern = g_tmp_root / "s14_extern_home";
    fs::create_directories(p_extern);
    spew(p_extern / "axis_s14_link_traeger.hpp", std::string(kSynthBasis));
    fs::path const p_link = g_tmp_root / "libs/cache_engine/axes/s14_link";
    fs::create_directory_symlink(p_extern, p_link, ec);
    if (ec) return fehler(23, "Symlink-Anlage (P) fehlgeschlagen", leer);
    ToolRun p1 = run_tool("--check");
    protokoll("KOEDER P (Verzeichnis-Symlink unter axes/ -- vorher still uebersprungen)", p1);
    if (p1.exit_code != 2) return fehler(23, "Koeder P muss Exit 2 liefern (vorher 0)", p1);
    if (!contains(p1.output, "Symlink")) return fehler(23, "Koeder P muss den Symlink benennen", p1);
    if (!contains(p1.output, "s14_link")) return fehler(23, "Koeder P muss den Ort des Links nennen", p1);
    fs::remove(p_link);
    fs::remove_all(p_extern, ec);
    ToolRun p2 = run_tool("--check");
    if (p2.exit_code != 0) return fehler(23, "Koeder-P-Ruecknahme muss Exit 0 liefern", p2);
#else
    std::printf("KOEDER P UEBERSPRUNGEN: keine POSIX-Symlinks unter _WIN32\n");
#endif

    // Schritt 24 -- KOEDER Q (G6a): DIGIT-SEPARATOR. ROT-ZUERST 13.08.2026 literal: EIN
    // Apostroph-Separator (1'000.0) vor dem Literal schaltete den char_lit-Modus -- der Traeger
    // wurde still als '-' (digest-only) gelockt, sein Literal war weg. Der Bestand uebt den
    // Stil bereits (axis_q2_queuing_adaptive_lsm.hpp:105, heute NACH dem Literal). NACHHER wird
    // das Literal korrekt erfasst.
    fs::path const    q_pfad = g_tmp_root / "libs/cache_engine/axes/s14_synth/axis_s14_separator.hpp";
    std::string const q_rel  = "libs/cache_engine/axes/s14_synth/axis_s14_separator.hpp";
    spew(q_pfad, "// s14 separator-koeder\n#pragma once\n#include <string_view>\nstruct Sep {\n"
                 "    static constexpr double kBudget = 1'000.0;\n"
                 "    static constexpr std::string_view algo_version = \"1.0.0.c\";\n"
                 "    static constexpr double kSpaeter = 2'000.0;\n};\n");
    ToolRun q1 = run_tool("--write");
    protokoll("KOEDER Q --write mit Separator-Traeger (vorher: version '-')", q1);
    if (q1.exit_code != 0) return fehler(24, "Koeder Q --write muss Exit 0 liefern", q1);
    if (lock_version_von(g_lockfile, q_rel) != "1.0.0.c")
        return fehler(24, "Koeder Q: das Literal hinter dem Separator muss als 1.0.0.c im Register stehen", q1);
    fs::remove(q_pfad);
    ToolRun q2 = run_tool("--write");
    if (q2.exit_code != 0) return fehler(24, "Koeder Q Aufraeum-Regen muss Exit 0 liefern", q2);

    // Schritt 25 -- KOEDER R (G6b): PRAEPROZESSOR. ROT-ZUERST 13.08.2026 literal: ein Literal in
    // einem '#if 0'-Block stellte die Version (9.9.9.c gewann gegen das aktive 1.0.0.c). Der
    // Bestand hat 0 Literale unter #if/#ifdef/#ifndef (gemessen 13.08.2026) -- deshalb ist die
    // strengste Form bestandsneutral: nicht entscheidbar => ROT.
    fs::path const r_pfad = g_tmp_root / "libs/cache_engine/axes/s14_synth/axis_s14_praep.hpp";
    spew(r_pfad, "#pragma once\n#include <string_view>\n#if 0\n"
                 "struct Alt { static constexpr std::string_view algo_version = \"9.9.9.c\"; };\n"
                 "#endif\n"
                 "struct Akt { static constexpr std::string_view algo_version = \"1.0.0.c\"; };\n");
    ToolRun r1 = run_tool("--check");
    protokoll("KOEDER R (Literal in '#if 0' -- vorher stellte es die Version)", r1);
    if (r1.exit_code != 1) return fehler(25, "Koeder R muss Exit 1 liefern", r1);
    if (!contains(r1.output, "nicht entscheidbar")) return fehler(25, "Koeder R muss 'nicht entscheidbar' melden", r1);
    fs::remove(r_pfad);
    ToolRun r2 = run_tool("--check");
    if (r2.exit_code != 0) return fehler(25, "Koeder-R-Ruecknahme muss Exit 0 liefern", r2);

    // Schritt 26 -- KOEDER S: v1-ABWEISUNG dauerhaft gepinnt (bisher nur Tool-Code, kein Koeder).
    std::string const lock_vor_s = slurp(g_lockfile);
    spew(g_lockfile, "# axis_version.lock v1\nlibs/cache_engine/heuristik/axis_spline.hpp 1 deadbeef\n");
    ToolRun s1 = run_tool("--check");
    protokoll("KOEDER S (v1-/kopfloses Lock)", s1);
    if (s1.exit_code != 1) return fehler(26, "Koeder S muss Exit 1 liefern", s1);
    if (!contains(s1.output, "format: v2")) return fehler(26, "Koeder S muss den v2-Kopf einfordern", s1);
    spew(g_lockfile, "# format: v1\nlibs/cache_engine/heuristik/axis_spline.hpp 1 deadbeef\n");
    ToolRun s2 = run_tool("--check");
    protokoll("KOEDER S2 (explizite '# format: v1'-Zeile)", s2);
    if (s2.exit_code != 1) return fehler(26, "Koeder S2 muss Exit 1 liefern", s2);
    if (!contains(s2.output, "unbekannt/veraltet")) return fehler(26, "Koeder S2 muss die Abweisung benennen", s2);
    spew(g_lockfile, lock_vor_s);
    ToolRun s3 = run_tool("--check");
    if (s3.exit_code != 0) return fehler(26, "Koeder-S-Ruecknahme muss Exit 0 liefern", s3);

    fs::remove_all(g_tmp_root, ec);
    std::printf("test_s14_axis_version_lock_tripwire: GRUEN (Koeder A-S gefahren, Anker 6/%ld gehalten)\n", o_disc);
    return 0;
}
