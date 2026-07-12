// h2_score_akte_tool — GO-5 Fork 7 (2026-07-12, Dossier 20260712-go5 A.7 Option (b)):
// der EINE Generator der maschinenlesbaren, TOOL-BERECHNETEN H2-Code-Qualitaets-Akte
// (sota_h2_scores.xml neben den sota/*.profile.xml; bestehende Profile byte-unberuehrt).
//
// WAS: je sota/*.profile.xml ein <code_quality>-Eintrag. Fuer Papers MIT vendorter Original-
// Quelle (ext/traversal/P*-Snapshots, READ-ONLY analysiert, NIE veraendert) faehrt das Tool
// cppcheck (CI-Bestand 2.21.0, Ledger `:480`) ueber den Quell-Snapshot und berechnet die
// gewichtete Befunddichte pro kLOC (Formel-Single-Source: profile_facade/h2_score_akte.hpp).
// Papers OHNE vendorte Original-Quelle (Rekonstruktionen) erhalten score="n/a" + reason —
// EHRLICH, kein Pseudo-Wert (Dossier-A.7-Ehrlichkeits-Regel; Anti-Fake: kein Hand-Wert-Pfad).
//
// DETERMINISMUS (FF4, #25-Akten-Muster compute_dataset_akte „kein Hand-Hash"):
//   • Datei-Menge = sortierte Liste aller C/C++-Quellen (.c .cc .cpp .cxx .h .hh .hpp) des
//     Snapshots, ausgenommen die Verzeichnisnamen ".git"/"build" (exclude_dirs in der Akte).
//     Header werden EXPLIZIT mitanalysiert (--file-list) — sonst waeren header-only-Papers
//     (z.B. P02 HOT) systematisch unteranalysiert (Numerator≠Denominator-Verzerrung).
//   • LOC-Metrik = physical_lines der analysierten Datei-Menge (in der Akte benannt).
//   • Tool-Argumente gepinnt + WOERTLICH in der Akte; Toolname+Version in der Akte.
//   • Ausgabe OHNE Zeitstempel; Eintraege in sortierter profile_id-Reihenfolge
//     ⇒ identische Quellen + identische Tool-Version ⇒ byte-identische Akte.
//
// ANTI-PHANTOM: ist eine als vendored kartierte Quelle lokal LEER/FEHLEND (Submodule nicht
// initialisiert), bricht das Tool HART ab (exit 3) — es schreibt NIE ein stilles n/a fuer
// eine Quelle, die per Kartierung existieren muss.
//
// Verwendung:
//   comdare-h2-score-akte --ce-root <ce-wurzel> [--out <akte.xml>] [--cppcheck <exe>] [--jobs N]
//   (Default-Out: <ce-root>/libs/cache_engine/algorithm_profiles/sota/sota_h2_scores.xml;
//    Default-Exe: env COMDARE_CPPCHECK, sonst "cppcheck" im PATH.)

#include "h2_score_akte.hpp"                // Score-Formel + Akten-(De-)Serialisierung (Single-Source)
#include "xml_config_parser/xml_reader.hpp" // paper_ref-Attribut der sota-Profile

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace fs  = std::filesystem;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace cx  = ::comdare::common::xml;

namespace {

// ── Die Kartierung paper_ref → vendorte Original-Quelle (relativ zur ce-Wurzel). Single-Source ──
//    dieser Zuordnung (Provenienz: ext/traversal/REPOS_OVERVIEW.md). Alle NICHT gelisteten
//    paper_refs sind Rekonstruktionen ohne vendorte Quelle ⇒ score="n/a".
struct VendoredSource {
    std::string_view paper_ref;
    std::string_view rel_dir;
};
inline constexpr VendoredSource kVendoredSources[] = {
    {"P01", "ext/traversal/P01-ART/unodb"},
    {"P02", "ext/traversal/P02-HOT/hot"},
    {"P03", "ext/traversal/P03-Masstree/masstree-beta"},
    {"P04", "ext/traversal/P04-CoCo-trie/CoCo-trie"},
    {"P05", "ext/traversal/P05-START/START"},
    {"P06", "ext/traversal/P06-B2tree"},
    {"P07", "ext/traversal/P07-Wormhole/wormhole"},
    {"P10", "ext/traversal/P10-SuRF/SuRF"},
    {"P20", "ext/traversal/P20-BTreesAreBack/leanstore"},
    {"P25", "ext/traversal/P25-Mahling/prefetching"},
    {"P29", "ext/traversal/P29-RCU/userspace-rcu"},
    {"P30", "ext/traversal/P30-HazardPointers/haz_ptr"},
};

// Die gepinnten cppcheck-Analyse-Argumente (WOERTLICH in die Akte geschrieben; -j/--file-list sind
// Lauf-Mechanik und veraendern die Befund-Menge nicht → nicht Teil der Signatur).
inline constexpr std::string_view kCppcheckArgs =
    "--enable=warning,style,performance,portability --quiet --platform=unix64 --template={severity}|{id}|{file}";
// Dieselben Argumente in Shell-Form: das Template ist double-quoted, damit sh/cmd die '|' NICHT als
// Pipe interpretieren (double quotes wirken in BEIDEN Shells; semantisch identisch zu kCppcheckArgs).
inline constexpr std::string_view kCppcheckArgsShell =
    "--enable=warning,style,performance,portability --quiet --platform=unix64 \"--template={severity}|{id}|{file}\"";

inline constexpr std::string_view kExcludeDirs = ".git;build";
inline constexpr std::string_view kLocMetric   = "physical_lines";
inline constexpr std::string_view kMethod      = "cppcheck_weighted_finding_density_per_kloc";

void print_usage(char const* argv0) {
    std::cerr << "Usage: " << argv0 << " --ce-root <path> [--out <akte.xml>] [--cppcheck <exe>] [--jobs N]\n";
}

[[nodiscard]] bool is_cxx_source(fs::path const& p) {
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".c" || ext == ".cc" || ext == ".cpp" || ext == ".cxx" || ext == ".h" || ext == ".hh" ||
           ext == ".hpp";
}

[[nodiscard]] bool is_excluded_dir_name(std::string const& name) { return name == ".git" || name == "build"; }

/// Sammelt die analysierte Datei-Menge deterministisch (rekursiv, exclude_dirs, sortiert).
[[nodiscard]] std::vector<fs::path> collect_sources(fs::path const& dir) {
    std::vector<fs::path>            out;
    fs::recursive_directory_iterator it{dir, fs::directory_options::skip_permission_denied};
    for (auto const end = fs::recursive_directory_iterator{}; it != end; ++it) {
        if (it->is_directory()) {
            if (is_excluded_dir_name(it->path().filename().string())) it.disable_recursion_pending();
            continue;
        }
        if (it->is_regular_file() && is_cxx_source(it->path())) out.push_back(it->path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

/// physical_lines einer Datei: '\n'-Zaehlung + 1 fuer eine nicht-newline-terminierte letzte Zeile.
[[nodiscard]] std::uint64_t count_physical_lines(fs::path const& p) {
    std::ifstream in{p, std::ios::binary};
    if (!in) return 0;
    std::uint64_t lines = 0;
    char          last  = '\n';
    char          c{};
    while (in.get(c)) {
        if (c == '\n') ++lines;
        last = c;
    }
    if (last != '\n') ++lines;
    return lines;
}

[[nodiscard]] std::string read_file(fs::path const& p) {
    std::ifstream      in{p, std::ios::binary};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

/// Fuehrt `cmd` via std::system aus (in-pattern: build_orchestrator/experiment_driver spawnen identisch).
[[nodiscard]] int run_command(std::string cmd) {
#ifdef _WIN32
    cmd = "\"" + cmd + "\""; // cmd.exe-Quirk: aeusseres Quote-Paar um quoted exe + quoted Args
#endif
    return std::system(cmd.c_str());
}

/// Tool-Version aus `<exe> --version` (z.B. "Cppcheck 2.21.0" → "2.21.0"). Leer bei Fehlschlag.
[[nodiscard]] std::string cppcheck_version(std::string const& exe, fs::path const& tmp_dir) {
    fs::path const    ver_file = tmp_dir / "h2_cppcheck_version.txt";
    std::string const cmd      = "\"" + exe + "\" --version > \"" + ver_file.string() + "\" 2>&1";
    if (run_command(cmd) != 0) return {};
    std::istringstream is{read_file(ver_file)};
    std::string        word, version;
    while (is >> word) version = word; // letztes Token der ersten Zeile ("Cppcheck 2.21.0")
    return version;
}

struct AnalyzeResult {
    tlz::H2FindingCounts counts{};
    std::uint64_t        loc   = 0;
    std::uint64_t        files = 0;
};

/// Analysiert EINE vendorte Quelle: Datei-Menge sammeln, LOC zaehlen, cppcheck fahren, Severities zaehlen.
[[nodiscard]] std::optional<AnalyzeResult> analyze_source(std::string const& exe, fs::path const& src_dir,
                                                          unsigned jobs, fs::path const& tmp_dir) {
    std::vector<fs::path> const sources = collect_sources(src_dir);
    if (sources.empty()) return std::nullopt; // Anti-Phantom: Aufrufer bricht hart ab

    AnalyzeResult r;
    r.files                  = sources.size();
    fs::path const list_file = tmp_dir / "h2_cppcheck_files.txt";
    {
        std::ofstream lst{list_file, std::ios::trunc};
        for (auto const& s : sources) {
            r.loc += count_physical_lines(s);
            lst << s.string() << "\n";
        }
    }

    fs::path const    err_file = tmp_dir / "h2_cppcheck_findings.txt";
    fs::path const    out_file = tmp_dir / "h2_cppcheck_stdout.txt";
    std::string const cmd      = "\"" + exe + "\" " + std::string{kCppcheckArgsShell} + " -j " + std::to_string(jobs) +
                                 " --file-list=\"" + list_file.string() + "\" > \"" + out_file.string() + "\" 2> \"" +
                                 err_file.string() + "\"";
    if (int const rc = run_command(cmd); rc != 0) {
        std::cerr << "ERROR: cppcheck-Aufruf scheiterte (rc=" << rc << "): " << cmd << "\n";
        return std::nullopt;
    }

    // Befund-Zeilen: "<severity>|<id>|<file>" — NUR die 5 Klassen des Enable-Sets zaehlen
    // (andere stderr-Zeilen, z.B. information/debug, gehen bewusst nicht in den Score ein).
    std::istringstream findings{read_file(err_file)};
    std::string        line;
    while (std::getline(findings, line)) {
        std::size_t const sep = line.find('|');
        if (sep == std::string::npos) continue;
        std::string_view const sev{line.data(), sep};
        if (sev == "error")
            ++r.counts.error;
        else if (sev == "warning")
            ++r.counts.warning;
        else if (sev == "style")
            ++r.counts.style;
        else if (sev == "performance")
            ++r.counts.performance;
        else if (sev == "portability")
            ++r.counts.portability;
    }
    return r;
}

} // namespace

int main(int argc, char** argv) {
    std::string ce_root_arg;
    std::string out_arg;
    std::string cppcheck_arg;
    unsigned    jobs = std::max(1u, std::thread::hardware_concurrency());

    for (int i = 1; i < argc; ++i) {
        std::string_view const arg = argv[i];
        if (arg == "--ce-root" && i + 1 < argc)
            ce_root_arg = argv[++i];
        else if (arg == "--out" && i + 1 < argc)
            out_arg = argv[++i];
        else if (arg == "--cppcheck" && i + 1 < argc)
            cppcheck_arg = argv[++i];
        else if (arg == "--jobs" && i + 1 < argc)
            jobs = std::max(1u, static_cast<unsigned>(std::atoi(argv[++i])));
        else {
            print_usage(argv[0]);
            return 1;
        }
    }
    if (ce_root_arg.empty()) {
        print_usage(argv[0]);
        return 1;
    }
    fs::path const ce_root  = fs::path{ce_root_arg};
    fs::path const sota_dir = ce_root / "libs" / "cache_engine" / "algorithm_profiles" / "sota";
    fs::path const out_path = out_arg.empty() ? (sota_dir / "sota_h2_scores.xml") : fs::path{out_arg};
    if (!fs::is_directory(sota_dir)) {
        std::cerr << "ERROR: sota-Profil-Verzeichnis fehlt: " << sota_dir.string() << "\n";
        return 2;
    }
    std::string exe = cppcheck_arg;
    if (exe.empty()) {
        if (char const* e = std::getenv("COMDARE_CPPCHECK"); e != nullptr && *e != '\0') exe = e;
    }
    if (exe.empty()) exe = "cppcheck";

    fs::path const tmp_dir = fs::temp_directory_path();

    std::string const version = cppcheck_version(exe, tmp_dir);
    if (version.empty()) {
        std::cerr << "ERROR: cppcheck nicht ausfuehrbar (--cppcheck/COMDARE_CPPCHECK/PATH pruefen): " << exe << "\n";
        return 2;
    }
    std::cout << "H2_SCORE_AKTE Generator: tool=cppcheck version=" << version << " jobs=" << jobs << "\n";

    // ── Alle sota/*.profile.xml in sortierter Reihenfolge (Determinismus). ──
    std::vector<fs::path> profiles;
    for (auto const& de : fs::directory_iterator{sota_dir}) {
        std::string const name = de.path().filename().string();
        if (de.is_regular_file() && name.size() > 12 && name.ends_with(".profile.xml")) profiles.push_back(de.path());
    }
    std::sort(profiles.begin(), profiles.end());
    if (profiles.empty()) {
        std::cerr << "ERROR: keine sota/*.profile.xml gefunden in " << sota_dir.string() << "\n";
        return 2;
    }

    tlz::H2ScoreAkte akte;
    akte.tool         = "cppcheck";
    akte.tool_version = version;
    akte.tool_args    = std::string{kCppcheckArgs};
    akte.loc_metric   = std::string{kLocMetric};
    akte.weights      = tlz::h2_weights_signature();
    akte.exclude_dirs = std::string{kExcludeDirs};

    std::size_t computed_n = 0;
    std::size_t na_n       = 0;
    for (auto const& profile_path : profiles) {
        std::string profile_id = profile_path.filename().string();
        profile_id             = profile_id.substr(0, profile_id.size() - std::string{".profile.xml"}.size());

        auto const root = cx::parse_document(read_file(profile_path));
        if (!root.has_value() || root->tag != "comdare_algorithm_profile") {
            std::cerr << "ERROR: sota-Profil nicht parsbar (comdare_algorithm_profile erwartet): "
                      << profile_path.string() << "\n";
            return 2;
        }
        std::string const paper_ref = root->attr("paper_ref");

        tlz::H2CodeQualityEntry entry;
        entry.profile_id = profile_id;
        entry.paper_ref  = paper_ref;
        entry.method     = std::string{kMethod};
        entry.computed   = "tool"; // der EINZIGE Pfad — es gibt keinen Hand-Wert (Anti-Fake)

        std::string_view rel_dir;
        for (auto const& v : kVendoredSources)
            if (v.paper_ref == paper_ref) rel_dir = v.rel_dir;

        if (rel_dir.empty()) {
            entry.score  = "n/a";
            entry.reason = "no_vendored_original_source";
            ++na_n;
        } else {
            fs::path const src_dir = ce_root / fs::path{std::string{rel_dir}};
            if (!fs::is_directory(src_dir)) {
                std::cerr << "ERROR: kartierte vendorte Quelle fehlt lokal (Submodule initialisieren!): "
                          << src_dir.string() << " (profile=" << profile_id << ")\n";
                return 3; // Anti-Phantom: NIE ein stilles n/a fuer eine kartierte Quelle
            }
            auto const res = analyze_source(exe, src_dir, jobs, tmp_dir);
            if (!res.has_value()) {
                std::cerr << "ERROR: Analyse leer/fehlgeschlagen fuer " << src_dir.string()
                          << " (profile=" << profile_id << ")\n";
                return 3;
            }
            entry.source   = std::string{rel_dir};
            entry.findings = res->counts;
            entry.loc      = res->loc;
            entry.files    = res->files;
            entry.score    = tlz::compute_h2_score(res->counts, res->loc);
            ++computed_n;
        }

        std::cout << "H2_SCORE profile=" << entry.profile_id << " paper_ref=" << entry.paper_ref
                  << " score=" << entry.score;
        if (entry.score == "n/a")
            std::cout << " reason=" << entry.reason;
        else
            std::cout << " (error=" << entry.findings.error << " warning=" << entry.findings.warning
                      << " style=" << entry.findings.style << " performance=" << entry.findings.performance
                      << " portability=" << entry.findings.portability << " loc=" << entry.loc
                      << " files=" << entry.files << ")";
        std::cout << "\n";
        akte.entries.push_back(std::move(entry));
    }

    std::ofstream out{out_path, std::ios::binary | std::ios::trunc};
    if (!out) {
        std::cerr << "ERROR: Akte nicht schreibbar: " << out_path.string() << "\n";
        return 2;
    }
    out << tlz::serialize_h2_score_akte(akte);
    out.flush();
    if (!out) {
        std::cerr << "ERROR: Akte-Schreibfehler: " << out_path.string() << "\n";
        return 2;
    }

    std::cout << "H2_SCORE_AKTE WROTE " << out_path.string() << " (" << akte.entries.size()
              << " Eintraege: " << computed_n << " tool-berechnet, " << na_n << " n/a)\n";
    return 0;
}
