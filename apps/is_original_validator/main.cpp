// V41.F.6.1.P1 Phase B — is_original_validator Pre-Build-Tool (Multi-Function Manifest)
//
// @stand V41.F.6.1.P1 Phase B.2.A0 Refactor
// @reference [[compile-time-only-no-runtime]] Memory
// @reference [[paper-original-code-pattern]] Memory
// @reference [[legacy-code-sha256-validation]] Memory (Multi-Function-Manifest Korrektur)
//
// **PRE-BUILD-TOOL** zur Erzeugung von constexpr Header-Files mit hardcoded
// is_original-Booleans. Wird zur BUILD-TIME (nicht Runtime!) ausgefuehrt;
// Output-Header wird vom Cache-Engine-Wrapper includiert.
//
// **User-Direktive [[compile-time-only-no-runtime]]:** Runtime-SHA-Berechnung
// ist VERBOTEN fuer Latenz-kritische Suchalgorithmen. Workaround: dieses Tool
// rechnet SHA zur Build-Zeit + emittiert constexpr Header. Cache-Engine selbst
// hat ZERO Runtime-Overhead — `is_original_<fn>()` ist hardcoded literal.
//
// **User-Korrektur 2026-05-26 (Multi-Function-Manifest):** Tool ist generisch
// wiederverwendbar — 1 Aufruf = N Functions = 1 Header. Vorher single-function
// CLI war zu eng.
//
// Verwendung (von CMake, comdare_generate_is_original_header()):
//
//   is_original_validator \
//       --manifest   path/to/legacy_code/paper_<id>/manifest.txt \
//       --base-dir   path/to/legacy_code/paper_<id> \
//       --output     ${CMAKE_BINARY_DIR}/generated/.../wrapper_is_original.hpp \
//       --namespace  comdare::cache_engine::allocator::axis_06_allocator::generated
//
// Manifest-Format (one entry per line; # = comment; leerzeilen ignoriert):
//
//   # function_name <whitespace> source_relative_path <whitespace> expected_sha256_hex
//   allocate    src/alloc.c    5f8b3a8e0c1a8e7c...
//   deallocate  src/alloc.c    a7c2d96b4e1093b2...
//   reallocate  src/alloc.c    b6e1f48c2a37d9e8...
//
// Output-Header:
//
//   #pragma once
//   namespace <namespace> {
//   inline constexpr bool kIsOriginal_allocate   = true;
//   inline constexpr bool kIsOriginal_deallocate = true;
//   inline constexpr bool kIsOriginal_reallocate = false;
//   inline constexpr bool kIsOriginal_module     = false;  // mp_all_of-Aggregat
//   }

#include "sha256/ctsha.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace ctsha = ::comdare::cache_engine::sha256;

namespace {

struct ManifestEntry {
    std::string function_name;
    std::string source_relative_path;
    std::string expected_sha_hex;
};

void print_usage(char const* argv0) {
    std::cerr << "Usage: " << argv0 << " --manifest <path> --base-dir <path> "
              << "--output <header.hpp> --namespace <ns>\n";
    std::cerr << "\n";
    std::cerr << "Manifest-Format (one entry per line, # for comments):\n";
    std::cerr << "  function_name  source_relative_path  expected_sha256_hex\n";
}

std::vector<ManifestEntry> read_manifest(fs::path const& p) {
    std::ifstream ifs(p);
    if (!ifs) {
        std::cerr << "ERROR: cannot open manifest " << p.string() << "\n";
        std::exit(2);
    }
    std::vector<ManifestEntry> entries;
    std::string line;
    int line_no = 0;
    while (std::getline(ifs, line)) {
        ++line_no;
        // Trim leading whitespace
        std::size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;          // empty/whitespace line
        if (line[start] == '#') continue;                  // comment
        std::istringstream iss(line.substr(start));
        ManifestEntry e;
        if (!(iss >> e.function_name >> e.source_relative_path >> e.expected_sha_hex)) {
            std::cerr << "ERROR: manifest " << p.string() << " line " << line_no
                      << " malformed (expected: function source sha)\n";
            std::exit(2);
        }
        if (e.expected_sha_hex.size() != 64) {
            std::cerr << "ERROR: manifest " << p.string() << " line " << line_no
                      << " sha must be 64 hex chars (got " << e.expected_sha_hex.size() << ")\n";
            std::exit(2);
        }
        entries.push_back(std::move(e));
    }
    return entries;
}

std::vector<std::uint8_t> read_file_bytes(fs::path const& p) {
    std::ifstream ifs(p, std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::cerr << "ERROR: cannot open source " << p.string() << "\n";
        std::exit(2);
    }
    auto size = static_cast<std::size_t>(ifs.tellg());
    ifs.seekg(0);
    std::vector<std::uint8_t> bytes(size);
    if (size > 0) {
        ifs.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    }
    return bytes;
}

std::string runtime_to_hex(ctsha::Digest const& d) {
    static char const* const kHex = "0123456789abcdef";
    std::string s(64, '0');
    for (std::size_t i = 0; i < 32; ++i) {
        s[2 * i + 0] = kHex[(d[i] >> 4) & 0x0f];
        s[2 * i + 1] = kHex[d[i] & 0x0f];
    }
    return s;
}

ctsha::Digest sha256_runtime(std::span<const std::uint8_t> data) {
    return ctsha::detail::sha256_bytes(data.data(), data.size());
}

}  // namespace

int main(int argc, char** argv) {
    std::string manifest_path;
    std::string base_dir;
    std::string output_path;
    std::string namespace_name;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if      (arg == "--manifest"  && i + 1 < argc) manifest_path  = argv[++i];
        else if (arg == "--base-dir"  && i + 1 < argc) base_dir       = argv[++i];
        else if (arg == "--output"    && i + 1 < argc) output_path    = argv[++i];
        else if (arg == "--namespace" && i + 1 < argc) namespace_name = argv[++i];
        else {
            print_usage(argv[0]);
            return 1;
        }
    }
    if (manifest_path.empty() || base_dir.empty()
        || output_path.empty() || namespace_name.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    auto entries = read_manifest(manifest_path);
    if (entries.empty()) {
        std::cerr << "ERROR: manifest " << manifest_path << " is empty\n";
        return 2;
    }

    // Pro Entry: SHA berechnen, Match-Status sammeln
    struct ValidatedEntry {
        ManifestEntry source;
        std::string   computed_hex;
        bool          is_match;
    };
    std::vector<ValidatedEntry> validated;
    validated.reserve(entries.size());

    bool module_all_match = true;
    for (auto const& e : entries) {
        fs::path source_abs = fs::path(base_dir) / e.source_relative_path;
        auto bytes  = read_file_bytes(source_abs);
        auto digest = sha256_runtime(std::span<const std::uint8_t>{bytes.data(), bytes.size()});
        auto computed = runtime_to_hex(digest);
        bool match = (computed == e.expected_sha_hex);
        validated.push_back({e, computed, match});
        if (!match) module_all_match = false;
        std::cout << "is_original_validator: " << e.function_name << " "
                  << (match ? "PASS" : "MISMATCH")
                  << " (computed=" << computed.substr(0, 12) << "...)\n";
    }

    // Header generieren
    fs::create_directories(fs::path(output_path).parent_path());
    std::ofstream ofs(output_path);
    if (!ofs) {
        std::cerr << "ERROR: cannot open output " << output_path << "\n";
        return 2;
    }
    ofs << "#pragma once\n";
    ofs << "// AUTO-GENERATED by is_original_validator from " << manifest_path
        << " — DO NOT EDIT\n";
    ofs << "// Base-Dir: " << base_dir << "\n";
    ofs << "// Module-Match: " << (module_all_match ? "ALL ORIGINAL" : "MODIFIED") << "\n";
    ofs << "\n";
    ofs << "namespace " << namespace_name << " {\n";
    for (auto const& v : validated) {
        ofs << "// " << v.source.function_name << ": expected=" << v.source.expected_sha_hex
            << " computed=" << v.computed_hex << "\n";
        ofs << "inline constexpr bool kIsOriginal_" << v.source.function_name
            << " = " << (v.is_match ? "true" : "false") << ";\n";
    }
    ofs << "\n";
    ofs << "// Modul-Aggregat (mp_all_of):\n";
    ofs << "inline constexpr bool kIsOriginal_module = "
        << (module_all_match ? "true" : "false") << ";\n";
    ofs << "}  // namespace " << namespace_name << "\n";

    std::cout << "is_original_validator: WROTE " << output_path << " ("
              << entries.size() << " entries, module="
              << (module_all_match ? "ALL ORIGINAL" : "MODIFIED") << ")\n";

    return 0;  // exit 0 auch bei Mismatch — Wrapper-Tests fangen das ab
}
