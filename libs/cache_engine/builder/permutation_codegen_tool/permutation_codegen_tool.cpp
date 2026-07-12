// #25 Teil B — permutation_codegen_tool Implementation (C++23-Port von codegen.cmake)
//
// TREUE ERSETZUNG: Jede Emit-/Kreuzprodukt-Zeile ist auf die korrespondierende Zeile in
// tools/permutation_codegen/codegen.cmake annotiert. Die erzeugte `permutations.cmake` ist
// byte-identisch zum Output des cmake-Backends (Beweis via tests/unit/perm_codegen_byte_identity.cmake).
//
// UMFANGS-GRENZE (bewusst, dokumentiert): codegen.cmake schreibt vier Artefakte —
//   (1) permutations.cmake            (add_library/comdare_permutations_all)  <- HIER portiert
//   (2) permutations_manifest.txt     (Runtime-Check des Experiment-Drivers)  <- HIER portiert
//   (3) perm_src/perm_<id>.cpp        (per-Permutation Wrapper-Quelle)        <- NICHT portiert
//   (4) perm_versions/perm_<id>.version (V36.E selective-rebuild-Buchhaltung) <- NICHT portiert
// Die Aufgabenstellung nennt als "Ausgang" ausdruecklich die permutations.cmake; (1)+(2) sind die
// beiden Registrierungs-Artefakte, deren Byte-Identitaet sinnvoll UND erreichbar ist. (3) zoege die
// gesamte V36.E-Versions-Maschinerie (axes_versions.txt-Lookup + Minor-Bump) nach; (4) traegt in
// codegen.cmake Zeile 508 `last_codegen=${CMAKE_CURRENT_LIST_FILE}` — ein backend-identifizierendes
// Feld, das ein ehrliches C++-Tool nicht byte-identisch reproduzieren KANN, ohne faelschlich zu
// behaupten, codegen.cmake habe die Datei erzeugt. Da das Default-Backend `cmake` bleibt und der
// reale Bau (der (3)+(4) braucht) damit unberuehrt ist, ist diese Grenze risikofrei.

#include <builder/permutation_codegen_tool/permutation_codegen_tool.hpp>

#include <array>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::permutation_codegen {

namespace {

// string(TOUPPER ...) fuer die (rein ASCII-kleingeschriebenen) Allokator-Namen (codegen.cmake Zeile 254-256).
[[nodiscard]] std::string ascii_upper(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char const c : s) { out.push_back((c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c); }
    return out;
}

// SIMD-Compile-Flags pro Variant (codegen.cmake Zeile 569-581). {msvc, gcc}.
[[nodiscard]] std::pair<std::string, std::string> simd_flags(std::string_view simd) {
    if (simd == "avx512") return {"/arch:AVX512", "-mavx512f"};
    if (simd == "avx2") return {"/arch:AVX2", "-mavx2"};
    if (simd == "sse4") return {"", "-msse4.2"};
    return {"", ""}; // scalar, neon
}

// Allokator-Achse: nur diese sechs bekommen einen vendor-Link-Block (codegen.cmake Zeile 607-649);
// "std" erzeugt keinen Block (kein else-Zweig).
[[nodiscard]] bool alloc_has_vendor_block(std::string_view alloc) {
    return alloc == "mimalloc" || alloc == "jemalloc" || alloc == "snmalloc" || alloc == "tcmalloc" ||
           alloc == "hoard" || alloc == "scalloc";
}

} // namespace

std::optional<Profile> parse_profile(std::string_view value) noexcept {
    if (value == "smoke") return Profile::Smoke;
    if (value == "medium") return Profile::Medium;
    if (value == "full") return Profile::Full;
    return std::nullopt;
}

std::string_view profile_name(Profile profile) noexcept {
    switch (profile) {
        case Profile::Smoke: return "smoke";
        case Profile::Medium: return "medium";
        case Profile::Full: return "full";
    }
    return "smoke";
}

std::string Permutation::id() const {
    // codegen.cmake Zeile 180/548: string(REPLACE "|" "_" _perm_id "${_perm}")
    std::string out = simd + "_" + layout + "_" + alloc;
    if (extended) { out += "_" + node + "_" + concur; }
    return out;
}

std::string Permutation::axis_path() const {
    // codegen.cmake Zeile 553/558
    std::string out = "simd_" + simd + "/layout_" + layout + "/alloc_" + alloc;
    if (extended) { out += "/node_" + node + "/concur_" + concur; }
    return out;
}

bool host_supports_isa(std::string_view isa, std::string_view target_isa) noexcept {
    // 1:1-Port von _perm_host_supports_isa (codegen.cmake Zeile 78-96).
    if (isa == "scalar") return true;
    if (target_isa == "auto") return isa != "neon";
    return target_isa == isa;
}

std::vector<Permutation> enumerate_permutations(Profile profile, std::string_view target_isa) {
    // Achsen-Basiswertmengen (codegen.cmake Zeile 46-57).
    std::vector<std::string>       axis12 = {"scalar", "sse4", "avx2"};      // Achse 12 SIMD
    std::vector<std::string> const axis4  = {"aos", "soa", "hybrid"};        // Achse 4 Cache-Layout
    std::vector<std::string>       axis6  = {"std", "jemalloc", "mimalloc"}; // Achse 6 Allokator
    std::vector<std::string> const axis1  = {"compact", "wide"};             // Achse 1 Knoten-Format
    std::vector<std::string> const axis8  = {"single", "multi"};             // Achse 8 Concurrency

    // Profil-Auswahl (codegen.cmake Zeile 60-72).
    bool extended = false;
    switch (profile) {
        case Profile::Smoke: extended = false; break;
        case Profile::Medium:
            extended = true;
            axis12.emplace_back("avx512");
            axis6.emplace_back("snmalloc");
            break;
        case Profile::Full:
            extended = true;
            axis12.emplace_back("avx512");
            axis12.emplace_back("neon");
            axis6.emplace_back("snmalloc");
            axis6.emplace_back("tcmalloc");
            axis6.emplace_back("hoard");
            axis6.emplace_back("scalloc");
            break;
    }

    // Verschachtelte foreach + ISA-Filter (codegen.cmake Zeile 106-134).
    std::vector<Permutation> perms;
    for (std::string const& v12 : axis12) {
        if (!host_supports_isa(v12, target_isa)) { continue; }
        for (std::string const& v4 : axis4) {
            for (std::string const& v6 : axis6) {
                if (!extended) {
                    perms.push_back(Permutation{v12, v4, v6, false, "", ""});
                } else {
                    for (std::string const& v1 : axis1) {
                        for (std::string const& v8 : axis8) { perms.push_back(Permutation{v12, v4, v6, true, v1, v8}); }
                    }
                }
            }
        }
    }
    return perms;
}

std::string render_permutations_cmake(Inputs const& inputs, std::vector<Permutation> const& perms) {
    std::string const  profile = std::string{profile_name(inputs.profile)};
    std::string const& isa     = inputs.target_isa;
    std::string const& mode    = inputs.mode;
    std::string const  n       = std::to_string(perms.size());
    // _out_dir + _perm_src_dir (codegen.cmake Zeile 31-32): "${dirname(OUTPUT)}/perm_src".
    std::string const perm_src_dir = (inputs.output.parent_path() / "perm_src").generic_string();

    std::string out;

    // Header (codegen.cmake Zeile 530-537). Das "—" ist ein UTF-8 Em-Dash (E2 80 94).
    out += "# permutations.cmake — generiert von tools/permutation_codegen/codegen.cmake\n";
    out += "# V36.B Vollausbau (2026-05-23): " + n + " Per-Permutation-Targets\n";
    out += "# Profile=" + profile + " ISA=" + isa + " Mode=" + mode + "\n";
    out += "\n";
    out += "message(STATUS \"Permutations: registriere " + n + " Per-Permutation-Targets\")\n";
    out += "\n";

    for (Permutation const& p : perms) {
        std::string const id        = p.id();
        std::string const axis_path = p.axis_path();

        // add_library-Block (codegen.cmake Zeile 583-598). ${PROJECT_SOURCE_DIR}/${CMAKE_BINARY_DIR}
        // sind in codegen.cmake escaped -> LITERAL in der Ausgabe; ${_perm_src_dir} ist expandiert.
        out += "add_library(perm_" + id + " SHARED \"" + perm_src_dir + "/perm_" + id + ".cpp\")\n";
        out += "target_compile_features(perm_" + id + " PRIVATE cxx_std_23)\n";
        out += "# V41.A5: Permutations brauchen cache_engine/indexes/* Headers\n";
        out +=
            "target_include_directories(perm_" + id + " PRIVATE \"${PROJECT_SOURCE_DIR}/libs/cache_engine/include\")\n";
        out += "set_target_properties(perm_" + id + " PROPERTIES\n";
        out += "    PREFIX \"\"\n";
        out += "    OUTPUT_NAME \"perm_" + id + "\"\n";
        out += "    RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/perm/cache_engine/" + axis_path + "\"\n";
        out += "    LIBRARY_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/perm/cache_engine/" + axis_path + "\"\n";
        out += "    ARCHIVE_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/perm/cache_engine/" + axis_path + "\"\n";
        out += "    CXX_VISIBILITY_PRESET hidden\n";
        out += "    VISIBILITY_INLINES_HIDDEN ON\n";
        out += "    POSITION_INDEPENDENT_CODE ON\n";
        out += "    FOLDER \"perm_cache_engine\")\n";

        // SIMD-Compile-Options (codegen.cmake Zeile 599-605). Generator-Expressions escaped -> literal.
        auto const [msvc, gcc] = simd_flags(p.simd);
        if (!msvc.empty() || !gcc.empty()) {
            out += "target_compile_options(perm_" + id + " PRIVATE\n";
            out += "    $<$<CXX_COMPILER_ID:MSVC>:" + msvc + ">\n";
            out += "    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:" + gcc +
                   ">)\n";
        }

        // Allokator-Link-Block (codegen.cmake Zeile 606-649).
        if (alloc_has_vendor_block(p.alloc)) {
            std::string const upper = ascii_upper(p.alloc);
            out += "if(TARGET comdare::vendor_" + p.alloc + ")\n";
            out += "    target_link_libraries(perm_" + id + " PRIVATE comdare::vendor_" + p.alloc + ")\n";
            out += "    target_compile_definitions(perm_" + id + " PRIVATE COMDARE_PERM_HAVE_" + upper + "=1)\n";
            out += "endif()\n";
        }

        // Perm-Separator (codegen.cmake Zeile 650-651): eine Leerzeile.
        out += "\n";
    }

    // Aggregator comdare_permutations_all (codegen.cmake Zeile 657-668).
    out += "add_custom_target(comdare_permutations_all\n";
    out += "    COMMENT \"V36.B Aggregator: " + n + " cache-engine Permutationen\"\n";
    out += "    DEPENDS";
    for (Permutation const& p : perms) { out += "\n        perm_" + p.id(); }
    out += "\n)\n";

    return out;
}

std::string render_manifest(Inputs const& inputs, std::vector<Permutation> const& perms) {
    std::string const  profile = std::string{profile_name(inputs.profile)};
    std::string const& isa     = inputs.target_isa;
    std::string const& mode    = inputs.mode;
    std::string const  n       = std::to_string(perms.size());

    // codegen.cmake Zeile 521-525.
    std::string out = "# permutations_manifest.txt (V36.B)\n";
    out += "# Profile=" + profile + " ISA=" + isa + " Mode=" + mode + "\n";
    out += "# count=" + n + "\n";
    for (Permutation const& p : perms) { out += "perm_" + p.id() + "\n"; }
    return out;
}

bool generate(Inputs const& inputs, std::string* error_out) {
    std::vector<Permutation> const perms = enumerate_permutations(inputs.profile, inputs.target_isa);

    std::error_code             ec;
    std::filesystem::path const out_dir = inputs.output.parent_path();
    if (!out_dir.empty()) {
        std::filesystem::create_directories(out_dir, ec); // codegen.cmake Zeile 34 (file(MAKE_DIRECTORY))
        if (ec) {
            if (error_out) {
                *error_out = "cannot create output directory: " + out_dir.string() + " (" + ec.message() + ")";
            }
            return false;
        }
    }

    // binaerer Write (keine Newline-Translation) -> exakte Bytes wie CMake file(WRITE).
    {
        std::ofstream cmake_out{inputs.output, std::ios::binary | std::ios::trunc};
        if (!cmake_out) {
            if (error_out) { *error_out = "cannot open output file: " + inputs.output.string(); }
            return false;
        }
        std::string const content = render_permutations_cmake(inputs, perms);
        cmake_out.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!cmake_out) {
            if (error_out) { *error_out = "write failed: " + inputs.output.string(); }
            return false;
        }
    }

    {
        std::filesystem::path const manifest_path = out_dir / "permutations_manifest.txt"; // codegen.cmake Zeile 33
        std::ofstream               manifest_out{manifest_path, std::ios::binary | std::ios::trunc};
        if (!manifest_out) {
            if (error_out) { *error_out = "cannot open manifest file: " + manifest_path.string(); }
            return false;
        }
        std::string const content = render_manifest(inputs, perms);
        manifest_out.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!manifest_out) {
            if (error_out) { *error_out = "write failed: " + manifest_path.string(); }
            return false;
        }
    }

    return true;
}

} // namespace comdare::cache_engine::builder::permutation_codegen
