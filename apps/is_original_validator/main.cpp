// V41.F.6.1.P2.A0.5 is_original_validator — Auto-Discovery + Lock-File + Mixin
//
// @stand V41.F.6.1.P2.A0.5
// @reference [[compile-time-only-no-runtime]] Memory
// @reference [[paper-original-code-pattern]] Memory
// @reference [[legacy-code-sha256-validation]] Memory (Auto-Discovery + Mixin Korrektur)
//
// **User-Direktive 2026-05-26:** "Die in der compile time abzugleichenden
// Function bodies ueber den Funktions Namen im Original Paper file gefunden
// und von allein per regex ausgewertet und dann gehasht werden. Die
// Registrierung muss voll dynamisch sein..."
//
// **3 Erweiterungs-Schichten:**
//   (1) AUTO-DISCOVERY: extrahiert Function-Body via Function-Name +
//       Brace-Balancer aus Paper-Source-File
//   (2) LOCK-FILE: bei erstem Build auto-generiert, bei spaeteren Builds
//       gegen aktuelle Source verglichen → Auto-Erkennung von Modifikationen
//   (3) MIXIN: generiert komplette Mixin-Struct (KEINE Macros mehr!) mit
//       allen is_original_<fn>() + Properties — Wrapper erbt + 0 manuelle
//       Registrierung
//
// Verwendung (von CMake comdare_generate_is_original_mixin):
//
//   is_original_validator \
//       --manifest legacy_code/paper_a04_mimalloc/manifest.txt \
//       --base-dir legacy_code/paper_a04_mimalloc \
//       --lock-file legacy_code/paper_a04_mimalloc/sha256_locked.txt \
//       --output ${CMAKE_BINARY_DIR}/generated/.../mimalloc_is_original.hpp \
//       --namespace comdare::cache_engine::allocator::axis_06_allocator::generated
//
// Manifest-Format (@-Annotations + Function-Mappings):
//
//   @compiler gcc-9.5
//   @has_original_paper_code true
//   @mixin_name MimallocAllocator_OriginalCodeMixin
//
//   # wrapper_function paper_function source_relative_path
//   allocate    mi_malloc    src/alloc.c
//   deallocate  mi_free      src/alloc.c
//   reallocate  mi_realloc   src/alloc.c
//
// Output-Header (Mixin-Pattern):
//
//   #pragma once
//   namespace <namespace> {
//       inline constexpr bool kIsOriginal_allocate   = true;
//       inline constexpr bool kIsOriginal_deallocate = true;
//       inline constexpr bool kIsOriginal_reallocate = false;
//       inline constexpr bool kIsOriginal_module     = false;
//
//       struct MimallocAllocator_OriginalCodeMixin {
//           static constexpr std::string_view compiler() noexcept { return "gcc-9.5"; }
//           static constexpr bool has_original_paper_code() noexcept { return true; }
//           static constexpr bool is_original_allocate() noexcept { return kIsOriginal_allocate; }
//           static constexpr bool is_original_deallocate() noexcept { return kIsOriginal_deallocate; }
//           static constexpr bool is_original_reallocate() noexcept { return kIsOriginal_reallocate; }
//           static constexpr bool is_original_module() noexcept { return kIsOriginal_module; }
//       };
//   }

#include "sha256/ctsha.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fs    = std::filesystem;
namespace ctsha = ::comdare::cache_engine::sha256;

namespace {

struct Manifest {
    std::string compiler;
    std::string has_original_paper_code; // "true"/"false"
    std::string axis_mixin_type;         // fully-qualified Achsen-Mixin-Template
    struct Mapping {
        std::string wrapper_fn;
        std::string paper_fn;
        std::string source_relative_path;
    };
    std::vector<Mapping> mappings;
};

void print_usage(char const* argv0) {
    std::cerr << "Usage: " << argv0 << " --manifest <path> --base-dir <path> --lock-file <path>"
              << " --output <header.hpp> --namespace <ns> [--update-lock]\n";
    std::cerr << "  --update-lock: (re)write sha256_locked.txt from the (LF-normalized) computed hashes instead of"
              << " validating (one-shot lock regeneration; used only by the manual is_original:relock CI job).\n";
    std::cerr << "\n";
    std::cerr << "Manifest-Format:\n";
    std::cerr << "  @compiler <id>     (z.B. 'gcc-9.5' oder 'self')\n";
    std::cerr << "  @has_original_paper_code <bool>\n";
    std::cerr << "  @axis_mixin_type <fully::qualified::AxisOriginalCodeMixin>\n";
    std::cerr << "  <wrapper_fn> <paper_fn> <source_relative_path>  (eine Zeile pro Mapping)\n";
}

Manifest read_manifest(fs::path const& p) {
    std::ifstream ifs(p);
    if (!ifs) {
        std::cerr << "ERROR: cannot open manifest " << p.string() << "\n";
        std::exit(2);
    }
    Manifest    m;
    std::string line;
    int         line_no = 0;
    while (std::getline(ifs, line)) {
        ++line_no;
        std::size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;
        // @-Annotation
        if (line[start] == '@') {
            std::istringstream iss(line.substr(start + 1));
            std::string        key;
            iss >> key;
            std::string value;
            std::getline(iss, value);
            // trim leading whitespace from value
            std::size_t v_start = value.find_first_not_of(" \t");
            if (v_start != std::string::npos) value = value.substr(v_start);
            if (key == "compiler")
                m.compiler = value;
            else if (key == "has_original_paper_code")
                m.has_original_paper_code = value;
            else if (key == "axis_mixin_type")
                m.axis_mixin_type = value;
            else {
                std::cerr << "WARN: manifest " << p.string() << " line " << line_no << " unknown annotation @" << key
                          << "\n";
            }
            continue;
        }
        // Mapping
        std::istringstream iss(line.substr(start));
        Manifest::Mapping  mapping;
        if (!(iss >> mapping.wrapper_fn >> mapping.paper_fn >> mapping.source_relative_path)) {
            std::cerr << "ERROR: manifest " << p.string() << " line " << line_no
                      << " malformed (expected: wrapper_fn paper_fn source_path)\n";
            std::exit(2);
        }
        m.mappings.push_back(std::move(mapping));
    }
    if (m.compiler.empty() || m.has_original_paper_code.empty() || m.axis_mixin_type.empty()) {
        std::cerr << "ERROR: manifest " << p.string() << " missing required annotations "
                  << "(@compiler / @has_original_paper_code / @axis_mixin_type)\n";
        std::exit(2);
    }
    return m;
}

std::string read_file_string(fs::path const& p) {
    std::ifstream ifs(p, std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::cerr << "ERROR: cannot open source " << p.string() << "\n";
        std::exit(2);
    }
    auto size = static_cast<std::size_t>(ifs.tellg());
    ifs.seekg(0);
    std::string raw(size, '\0');
    if (size > 0) ifs.read(raw.data(), static_cast<std::streamsize>(size));
    // Line-Ending-Normalisierung (CRLF/lone-CR -> LF) VOR dem Hashen, damit die Habich-Compliance-SHA256
    // PLATTFORM-NEUTRAL ist: die committeten Locks (sha256_locked.txt) wurden auf einem CRLF-Windows-Worktree
    // erzeugt, das Repo ist aber LF (.gitattributes eol=lf) -> auf Linux-CI hasht der Validator LF-Bytes und
    // mismatched die CRLF-Locks (module=MODIFIED, is_original=false), obwohl der Paper-Code UNVERAENDERT ist.
    // Die Normalisierung aendert NUR die Zeilenende-Repraesentation, NICHT den Code-Inhalt -> echte
    // Modifikationen (Whitespace/Tabs/Edits/Body-Drift) werden weiterhin als MISMATCH erkannt.
    // [[legacy-code-sha256-validation]]
    std::string s;
    s.reserve(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        char const c = raw[i];
        if (c == '\r') {
            s.push_back('\n');
            if (i + 1 < raw.size() && raw[i + 1] == '\n') ++i; // CRLF -> ein einzelnes LF
        } else {
            s.push_back(c);
        }
    }
    return s;
}

/// **AUTO-DISCOVERY**: Findet Function-Body via Function-Name + Brace-Balancer
///
/// Suchstrategie:
///   1. Regex: \b<paper_fn>\s*\(...\)\s*\{
///   2. Brace-Balancer: ab dem ersten { zaehle alle braces, bis count==0
///   3. Return: Substring von Function-Signatur-Start bis matching '}'
///
/// Liefert std::nullopt wenn nicht gefunden.
std::optional<std::string> extract_function_body(std::string const& source, std::string const& paper_fn) {
    // Robuster Regex:
    //   word-boundary + name + optional whitespace + ( args... ) + ANYTHING_NOT_BRACE_OR_SEMI + {
    //   - ANYTHING_NOT_BRACE_OR_SEMI deckt typische C-Code-Variationen ab:
    //     Function-Body in def:  "void f(int x) { ... }"
    //     Mit Cobweb-Attribut:   "void f(int x) __attribute__((cold)) { ... }"
    //     Mit Block-Comment:     "void f(int x) /* doc */ { ... }"
    //   - Filtert Forward-Declarations aus (die enden mit ';' nicht '{')
    std::string pattern = R"((?:^|\n)([^\n;]*?\b)" + paper_fn + R"(\s*\([^)]*\)[^{;]*\{))";
    std::regex  re(pattern);
    std::smatch match;
    if (!std::regex_search(source, match, re)) return std::nullopt;

    // Position des '{' im Source
    std::size_t brace_pos = match.position(0) + match.length(0) - 1;
    // Function-Signatur-Start (skip leading newline if present)
    std::size_t sig_start = match.position(0);
    if (sig_start < source.size() && source[sig_start] == '\n') ++sig_start;

    // Brace-Balancer ab brace_pos
    int         depth            = 0;
    std::size_t end_pos          = brace_pos;
    bool        in_string        = false;
    bool        in_char          = false;
    bool        in_line_comment  = false;
    bool        in_block_comment = false;
    for (std::size_t i = brace_pos; i < source.size(); ++i) {
        char c    = source[i];
        char prev = (i > 0) ? source[i - 1] : '\0';
        // Comment handling
        if (in_line_comment) {
            if (c == '\n') in_line_comment = false;
            continue;
        }
        if (in_block_comment) {
            if (c == '/' && prev == '*') in_block_comment = false;
            continue;
        }
        if (in_string) {
            if (c == '"' && prev != '\\') in_string = false;
            continue;
        }
        if (in_char) {
            if (c == '\'' && prev != '\\') in_char = false;
            continue;
        }
        // Comment-Start
        if (c == '/' && i + 1 < source.size()) {
            if (source[i + 1] == '/') {
                in_line_comment = true;
                ++i;
                continue;
            }
            if (source[i + 1] == '*') {
                in_block_comment = true;
                ++i;
                continue;
            }
        }
        if (c == '"') {
            in_string = true;
            continue;
        }
        if (c == '\'') {
            in_char = true;
            continue;
        }
        if (c == '{') ++depth;
        if (c == '}') {
            --depth;
            if (depth == 0) {
                end_pos = i + 1; // include the closing '}'
                break;
            }
        }
    }
    if (depth != 0) return std::nullopt; // Unbalanced — kein vollstaendiger Body
    return source.substr(sig_start, end_pos - sig_start);
}

std::string runtime_to_hex(ctsha::Digest const& d) {
    static char const* const kHex = "0123456789abcdef";
    std::string              s(64, '0');
    for (std::size_t i = 0; i < 32; ++i) {
        s[2 * i + 0] = kHex[(d[i] >> 4) & 0x0f];
        s[2 * i + 1] = kHex[d[i] & 0x0f];
    }
    return s;
}

ctsha::Digest sha256_runtime(std::string_view s) {
    auto const* data = reinterpret_cast<const std::uint8_t*>(s.data());
    return ctsha::detail::sha256_bytes(data, s.size());
}

std::unordered_map<std::string, std::string> read_lock_file(fs::path const& p) {
    std::unordered_map<std::string, std::string> locked;
    std::ifstream                                ifs(p);
    if (!ifs) return locked; // not found is OK (first-time init)
    std::string line;
    while (std::getline(ifs, line)) {
        std::size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos || line[start] == '#') continue;
        std::istringstream iss(line.substr(start));
        std::string        fn, sha;
        if (iss >> fn >> sha && sha.size() == 64) locked[fn] = sha;
    }
    return locked;
}

void write_lock_file(fs::path const& p, std::vector<std::pair<std::string, std::string>> const& entries) {
    std::ofstream ofs(p);
    ofs << "# AUTO-GENERATED by is_original_validator (Lock-File)\n";
    ofs << "# Format: wrapper_function <whitespace> sha256_hex\n";
    ofs << "# Commit this file into git to enable validation on subsequent builds.\n";
    ofs << "# Modifications to legacy_code source files will be detected as MISMATCH.\n";
    ofs << "\n";
    for (auto const& [fn, sha] : entries) { ofs << fn << "  " << sha << "\n"; }
}

} // namespace

int main(int argc, char** argv) {
    std::string manifest_path;
    std::string base_dir;
    std::string lock_file_path;
    std::string output_path;
    std::string namespace_name;
    bool        update_lock = false; // --update-lock: schreibt die (normalisierten) SHAs neu ins Lock (one-shot-Regenerierung)

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "--manifest" && i + 1 < argc)
            manifest_path = argv[++i];
        else if (arg == "--base-dir" && i + 1 < argc)
            base_dir = argv[++i];
        else if (arg == "--lock-file" && i + 1 < argc)
            lock_file_path = argv[++i];
        else if (arg == "--output" && i + 1 < argc)
            output_path = argv[++i];
        else if (arg == "--namespace" && i + 1 < argc)
            namespace_name = argv[++i];
        else if (arg == "--update-lock")
            update_lock = true;
        else {
            print_usage(argv[0]);
            return 1;
        }
    }
    if (manifest_path.empty() || base_dir.empty() || lock_file_path.empty() || output_path.empty() ||
        namespace_name.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    Manifest manifest = read_manifest(manifest_path);
    if (manifest.mappings.empty()) {
        std::cerr << "ERROR: manifest " << manifest_path << " has no function mappings\n";
        return 2;
    }

    auto locked          = read_lock_file(lock_file_path);
    bool first_time_init = locked.empty();

    // Pro Mapping: Auto-Discovery + Hash + Compare-against-Lock
    struct ValidatedEntry {
        Manifest::Mapping mapping;
        std::string       computed_hex;
        bool              is_match;
        bool              body_found;
    };
    std::vector<ValidatedEntry>                      validated;
    std::vector<std::pair<std::string, std::string>> new_lock_entries;
    bool                                             module_all_match = true;

    for (auto const& mapping : manifest.mappings) {
        fs::path source_abs     = fs::path(base_dir) / mapping.source_relative_path;
        auto     source_content = read_file_string(source_abs);
        auto     body_opt       = extract_function_body(source_content, mapping.paper_fn);
        if (!body_opt.has_value()) {
            std::cerr << "ERROR: function '" << mapping.paper_fn << "' not found in " << source_abs.string() << "\n";
            return 3;
        }
        auto digest   = sha256_runtime(*body_opt);
        auto computed = runtime_to_hex(digest);

        bool is_match = true;
        // --update-lock: die Referenz wird bewusst NEU gesetzt -> als original behandeln (kein Vergleich).
        // Normal-Modus (Codex #188-4b-b-V BLOCKER 2): fehlt ein Lock-Eintrag, obwohl das Lock EXISTIERT, gilt der
        // wrapper als NICHT validiert -> MISMATCH (kein stilles is_original=true fuer neue/unbekannte Mappings).
        // Nur first_time_init (Lock komplett leer) behandelt alle als INIT/original und schreibt das Lock.
        if (!update_lock) {
            auto it = locked.find(mapping.wrapper_fn);
            if (it != locked.end()) {
                is_match = (it->second == computed);
            } else if (!first_time_init) {
                is_match = false;
            }
        }
        if (!is_match) module_all_match = false;

        validated.push_back({mapping, computed, is_match, true});
        new_lock_entries.push_back({mapping.wrapper_fn, computed});

        std::cout << "is_original_validator: " << mapping.wrapper_fn << " (paper=" << mapping.paper_fn << ") "
                  << (update_lock ? "UPDATED" : (first_time_init ? "INIT" : (is_match ? "PASS" : "MISMATCH")))
                  << " (sha=" << computed.substr(0, 12) << "...)\n";
    }

    if (first_time_init || update_lock) {
        std::cout << "is_original_validator: " << (update_lock ? "--update-lock (regenerating)" : "First-time-init")
                  << " — writing lock-file " << lock_file_path << " (commit this into git)\n";
        write_lock_file(lock_file_path, new_lock_entries);
    }

    // Mixin-Header generieren
    fs::create_directories(fs::path(output_path).parent_path());
    std::ofstream ofs(output_path);
    if (!ofs) {
        std::cerr << "ERROR: cannot open output " << output_path << "\n";
        return 2;
    }
    ofs << "#pragma once\n";
    ofs << "// AUTO-GENERATED by is_original_validator from " << manifest_path << "\n";
    ofs << "// DO NOT EDIT — re-runs on every CMake build (Cache-Engine Pre-Build-Step)\n";
    ofs << "// Module-Status: " << (module_all_match ? "ALL ORIGINAL" : "MODIFIED") << "\n";
    ofs << "// Axis-Mixin-Type: " << manifest.axis_mixin_type << "\n";
    ofs << "\n";
    ofs << "#include <string_view>\n";
    ofs << "\n";
    ofs << "namespace " << namespace_name << " {\n";
    ofs << "\n";
    ofs << "// ─── Per-function kIsOriginal_<wrapper_fn> Booleans ──────────────────\n";
    for (auto const& v : validated) {
        ofs << "// " << v.mapping.wrapper_fn << " (paper=" << v.mapping.paper_fn << " from "
            << v.mapping.source_relative_path << ")\n";
        ofs << "//   computed_sha=" << v.computed_hex << "\n";
        ofs << "inline constexpr bool kIsOriginal_" << v.mapping.wrapper_fn << " = " << (v.is_match ? "true" : "false")
            << ";\n";
    }
    ofs << "\n";
    ofs << "// ─── Achsen-Manifest-Struct (Pflicht-Constants fuer Mixin-Template) ──\n";
    ofs << "struct PaperManifest {\n";
    ofs << "    static constexpr ::std::string_view kCompiler = \"" << manifest.compiler << "\";\n";
    ofs << "    static constexpr bool kHasOriginalPaperCode = " << manifest.has_original_paper_code << ";\n";
    for (auto const& v : validated) {
        ofs << "    static constexpr bool kIsOriginal_" << v.mapping.wrapper_fn << " = ::" << namespace_name
            << "::kIsOriginal_" << v.mapping.wrapper_fn << ";\n";
    }
    ofs << "};\n";
    ofs << "\n";
    ofs << "// ─── Convenience-Alias: Wrapper erbt davon (Achsen-generischer Mixin) ─\n";
    ofs << "using OriginalCodeMixin = ::" << manifest.axis_mixin_type << "<PaperManifest>;\n";
    ofs << "\n";
    ofs << "}  // namespace " << namespace_name << "\n";

    std::cout << "is_original_validator: WROTE " << output_path << " (axis_mixin=" << manifest.axis_mixin_type << ", "
              << validated.size() << " functions, module=" << (module_all_match ? "ALL ORIGINAL" : "MODIFIED") << ")\n";

    return 0;
}
