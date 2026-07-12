// #25 Teil B — permutation_codegen_tool Implementation (C++23-Port von codegen.cmake)
//
// TREUE, VOLLE ERSETZUNG: Jede Emit-/Kreuzprodukt-Zeile ist auf die korrespondierende Zeile in
// tools/permutation_codegen/codegen.cmake annotiert. Das cpp-Backend erzeugt ALLE VIER Artefakte,
// die codegen.cmake schreibt — byte-identisch (Beweis via tests/unit/perm_codegen_byte_identity.cmake):
//   (1) permutations.cmake              (add_library/comdare_permutations_all)  — render_permutations_cmake
//   (2) permutations_manifest.txt       (Runtime-Check des Experiment-Drivers)  — render_manifest
//   (3) perm_src/perm_<id>.cpp          (per-Permutation Wrapper-Quelle)        — render_wrapper_source
//   (4) perm_versions/perm_<id>.version (V36.E selective-rebuild-Buchhaltung)   — render_version_file
//
// EINZIGE, EHRLICHE ABWEICHUNG: Feld `last_codegen` in (4). codegen.cmake Z.508 schreibt dort
// ${CMAKE_CURRENT_LIST_FILE} (= Pfad zu codegen.cmake). Das cpp-Tool behauptet NICHT, codegen.cmake
// habe die Datei erzeugt, sondern schreibt seinen EIGENEN Marker (cpp_last_codegen_marker()). Der
// Byte-Identitaets-Test schliesst GENAU diese eine `last_codegen=`-Zeile aus; alle anderen Felder
// sind byte-identisch. GO-2 (2026-07-12): nach diesem Beweis ist `cpp` das DEFAULT-Backend
// (COMDARE_PERMUTATION_CODEGEN_BACKEND=cpp); cmake/sh/bat bleiben waehlbar, codegen.cmake bleibt
// als Referenz-Backend byte-unberuehrt. Historie: eingefuehrt als opt-in (Default war `cmake`).
//
// KEIN Python (Talos-OS-Direktive). Reines C++23 + CMake.

#include <builder/permutation_codegen_tool/permutation_codegen_tool.hpp>

#include <array>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::builder::permutation_codegen {

namespace {

// string(TOUPPER ...) fuer die (rein ASCII-kleingeschriebenen) Achsen-Namen (codegen.cmake Zeile 254-256).
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

// Alle Vorkommen von `from` in `s` durch `to` ersetzen (Template-Platzhalter-Substitution).
void replace_all(std::string& s, std::string_view from, std::string_view to) {
    if (from.empty()) return;
    std::size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
}

// Datei binaer -> string (leer bei Fehler/nicht vorhanden).
[[nodiscard]] std::string read_file_to_string(std::filesystem::path const& p) {
    std::ifstream in{p, std::ios::binary};
    if (!in) return {};
    return std::string{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Byte-genaues binaeres Schreiben (keine Newline-Translation) -> exakt wie CMake file(WRITE).
[[nodiscard]] bool write_file_binary(std::filesystem::path const& p, std::string_view content, std::string* err) {
    std::ofstream out{p, std::ios::binary | std::ios::trunc};
    if (!out) {
        if (err) { *err = "cannot open output file: " + p.string(); }
        return false;
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) {
        if (err) { *err = "write failed: " + p.string(); }
        return false;
    }
    return true;
}

// REGEX MATCH "<key>([^\n\r]+)" (codegen.cmake Z.150/214/218): erster Treffer, Wert bis Zeilenende;
// leer/nicht gefunden -> `def` (das `+` im Regex verlangt >=1 Zeichen).
[[nodiscard]] std::string match_field(std::string_view content, std::string_view key, std::string_view def) {
    auto const pos = content.find(key);
    if (pos == std::string_view::npos) return std::string{def};
    std::size_t const start = pos + key.size();
    std::size_t       end   = start;
    while (end < content.size() && content[end] != '\n' && content[end] != '\r') { ++end; }
    if (end == start) return std::string{def};
    return std::string{content.substr(start, end - start)};
}

// Wrapper-Quell-Template (codegen.cmake Zeile 259-501, file(WRITE ${_wrapper} ...)). Reines ASCII.
// Platzhalter @@...@@ werden in render_wrapper_source substituiert. Als raw-string -> von clang-format
// NICHT umformatiert; die Bytes bleiben exakt.
constexpr std::string_view kWrapperTemplate =
    R"PERMTPL(// Auto-generiert von tools/permutation_codegen/codegen.cmake (V36-V39 2026-05-24)
// Permutation: @@PERM_ID@@
// Profile=@@PROFILE@@ ISA=@@ISA@@
// V36.E Version: @@STORED_VERSION@@  (Achsen-Sig: @@AXES_SIG@@)

#define @@AXIS_MACRO_SIMD@@ 1
#define @@AXIS_MACRO_LAYOUT@@ 1
#define @@AXIS_MACRO_ALLOC@@ 1

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include <utility>
#include <string>

// V41.A5 (2026-05-24): echte Algorithmus-Implementierungen statt STL-wrapper
#include <cache_engine/indexes/linear_probe_hashset.hpp>
#include <cache_engine/indexes/radix_index.hpp>

// V39.B + V41.A3 (2026-05-25) - SIMD-Achse echt via Intrinsics (5 Variants)
#if defined(COMDARE_PERM_SIMD_IS_SSE4) || defined(COMDARE_PERM_SIMD_IS_AVX2) || defined(COMDARE_PERM_SIMD_IS_AVX512)
  #if defined(__GNUC__) || defined(__clang__)
    #include <x86intrin.h>
  #elif defined(_MSC_VER)
    #include <intrin.h>
  #endif
#elif defined(COMDARE_PERM_SIMD_IS_NEON)
  #include <arm_neon.h>
#endif

// V40.A/B + V41.A2 (2026-05-25) - Allokator-Achse echt (7 Variants)
#if defined(COMDARE_PERM_ALLOC_IS_MIMALLOC) && defined(COMDARE_PERM_HAVE_MIMALLOC)
  #include <mimalloc.h>
  #define COMDARE_ALLOC(sz)  ::mi_malloc(sz)
  #define COMDARE_FREE(p)    ::mi_free(p)
  #define COMDARE_ALLOC_NAME "mimalloc"
#elif defined(COMDARE_PERM_ALLOC_IS_JEMALLOC) && defined(COMDARE_PERM_HAVE_JEMALLOC)
  #include <jemalloc/jemalloc.h>
  #define COMDARE_ALLOC(sz)  ::je_malloc(sz)
  #define COMDARE_FREE(p)    ::je_free(p)
  #define COMDARE_ALLOC_NAME "jemalloc"
#elif defined(COMDARE_PERM_ALLOC_IS_SNMALLOC) && defined(COMDARE_PERM_HAVE_SNMALLOC)
  // snmalloc header-only: nutze libc-shim
  #include <snmalloc/snmalloc.h>
  #define COMDARE_ALLOC(sz)  ::snmalloc::libc::malloc(sz)
  #define COMDARE_FREE(p)    ::snmalloc::libc::free(p)
  #define COMDARE_ALLOC_NAME "snmalloc"
#elif defined(COMDARE_PERM_ALLOC_IS_TCMALLOC) && defined(COMDARE_PERM_HAVE_TCMALLOC)
  #include <gperftools/tcmalloc.h>
  #define COMDARE_ALLOC(sz)  ::tc_malloc(sz)
  #define COMDARE_FREE(p)    ::tc_free(p)
  #define COMDARE_ALLOC_NAME "tcmalloc"
#elif defined(COMDARE_PERM_ALLOC_IS_HOARD) && defined(COMDARE_PERM_HAVE_HOARD)
  // hoard ueberschreibt globalen malloc bei dynamic-link; raw extern "C"
  extern "C" void* malloc(std::size_t);
  extern "C" void  free(void*);
  #define COMDARE_ALLOC(sz)  ::malloc(sz)
  #define COMDARE_FREE(p)    ::free(p)
  #define COMDARE_ALLOC_NAME "hoard"
#elif defined(COMDARE_PERM_ALLOC_IS_SCALLOC) && defined(COMDARE_PERM_HAVE_SCALLOC)
  extern "C" void* malloc(std::size_t);
  extern "C" void  free(void*);
  #define COMDARE_ALLOC(sz)  ::malloc(sz)
  #define COMDARE_FREE(p)    ::free(p)
  #define COMDARE_ALLOC_NAME "scalloc"
#else
  #define COMDARE_ALLOC(sz)  std::malloc(sz)
  #define COMDARE_FREE(p)    std::free(p)
  #define COMDARE_ALLOC_NAME "std"
#endif

// V38.B (2026-05-24): Cross-platform Symbol-Export fuer SHARED-Library.
#if defined(_WIN32) || defined(__CYGWIN__)
  #define COMDARE_PERM_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
  #define COMDARE_PERM_EXPORT __attribute__((visibility("default")))
#else
  #define COMDARE_PERM_EXPORT
#endif

#define COMDARE_PERM_ID "@@PERM_ID@@"
#define COMDARE_PERM_VERSION "@@STORED_VERSION@@"
#define COMDARE_PERM_SIMD "@@SIMD@@"
#define COMDARE_PERM_LAYOUT "@@LAYOUT@@"
#define COMDARE_PERM_ALLOC "@@ALLOC@@"@@EXTRA_DEFINES@@

// V38.C - PermDescriptor: ausserhalb namespace damit extern "C" sauberen Symbol-Namen exportiert
struct PermDescriptor {
    const char* id;
    const char* version;
    const char* axes;
    int (*run)(unsigned long, double*);
};

extern "C" COMDARE_PERM_EXPORT const char* perm_@@PERM_ID@@_id() {
    return COMDARE_PERM_ID;
}

extern "C" COMDARE_PERM_EXPORT const char* perm_@@PERM_ID@@_version() {
    return COMDARE_PERM_VERSION;
}

extern "C" COMDARE_PERM_EXPORT const char* perm_@@PERM_ID@@_axes() {
    return "simd=@@SIMD@@,layout=@@LAYOUT@@,alloc=@@ALLOC@@ (real=" COMDARE_ALLOC_NAME ")";
}

// V39 (2026-05-24) - Echt achsen-spezifischer Algorithmus.
// SIMD-Achse: Hash-Berechnung variiert (scalar / sse4 mit crc32 / avx2 mit gather-loop).
// Layout-Achse: AoS=unordered_set, SoA=2 parallele Vektoren, hybrid=pair-Vektor.

// Hash-Helper pro SIMD-Variant
namespace v39_hash {
    constexpr std::uint64_t kPrime = 11400714819323198485ULL;

#if defined(COMDARE_PERM_SIMD_IS_AVX512)
    // AVX-512: 8 keys parallel, extract lane 0
    static inline std::uint64_t mix(std::uint64_t k) {
        __m512i v = _mm512_set1_epi64(static_cast<long long>(k));
        __m512i p = _mm512_set1_epi64(static_cast<long long>(kPrime));
        __m512i x = _mm512_xor_si512(v, p);
        __m256i lo = _mm512_castsi512_si256(x);
        return static_cast<std::uint64_t>(_mm256_extract_epi64(lo, 0));
    }
#elif defined(COMDARE_PERM_SIMD_IS_AVX2)
    // AVX2: 4 keys parallel
    static inline std::uint64_t mix(std::uint64_t k) {
        __m256i v = _mm256_set1_epi64x(static_cast<long long>(k));
        __m256i p = _mm256_set1_epi64x(static_cast<long long>(kPrime));
        __m256i x = _mm256_xor_si256(v, p);
        return static_cast<std::uint64_t>(_mm256_extract_epi64(x, 0));
    }
#elif defined(COMDARE_PERM_SIMD_IS_SSE4)
    // SSE4.2: hardware CRC32
    static inline std::uint64_t mix(std::uint64_t k) {
        return _mm_crc32_u64(kPrime, k);
    }
#elif defined(COMDARE_PERM_SIMD_IS_NEON)
    // ARM NEON: 2 keys parallel, vector-xor
    static inline std::uint64_t mix(std::uint64_t k) {
        uint64x2_t v = vdupq_n_u64(k);
        uint64x2_t p = vdupq_n_u64(kPrime);
        return vgetq_lane_u64(veorq_u64(v, p), 0);
    }
#else
    // scalar fallback: Fibonacci-Hash
    static inline std::uint64_t mix(std::uint64_t k) {
        return k * kPrime;
    }
#endif
}

extern "C" COMDARE_PERM_EXPORT int perm_@@PERM_ID@@_run(unsigned long n_ops, double* out_micros_per_op) {
    if (!out_micros_per_op || n_ops == 0) {
        return -1;
    }

    // V40.C Allokator-Stresstest: pro op eine kleine raw-Allokation
    // ueber den achsenspezifischen Allocator (mi_malloc/je_malloc/std::malloc).
    // Mit reservierter pre-allocated array werden die ops sichtbar verteilt.
    {
        constexpr std::size_t kBufSize = 32;  // klein, viele Operationen
        for (unsigned long i = 0; i < (n_ops / 8); ++i) {
            void* p = COMDARE_ALLOC(kBufSize);
            // Antimuessen-Optimization: write 1 byte
            if (p) {
                reinterpret_cast<volatile char*>(p)[0] = static_cast<char>(i);
                COMDARE_FREE(p);
            }
        }
    }

#if defined(COMDARE_PERM_LAYOUT_IS_AOS)
    // V41.A5: AoS -> echte LinearProbeHashSet (cache-friendly, open addressing)
    // statt std::unordered_set (separate chaining, pointer-chasing).
    comdare::cache_engine::indexes::LinearProbeHashSet<std::uint64_t> store(static_cast<std::size_t>(n_ops));
    auto t0 = std::chrono::steady_clock::now();
    for (unsigned long i = 0; i < n_ops; ++i) {
        store.insert(v39_hash::mix(i));
    }
    std::uint64_t hits = 0;
    for (unsigned long i = 0; i < n_ops; ++i) {
        if (store.contains(v39_hash::mix(i))) ++hits;
    }
    auto t1 = std::chrono::steady_clock::now();
#elif defined(COMDARE_PERM_LAYOUT_IS_SOA)
    // SoA: keys + hashes in zwei separaten Vektoren (besseres Cache-Verhalten beim Scan)
    std::vector<std::uint64_t> keys;
    std::vector<std::uint64_t> hashes;
    keys.reserve(static_cast<std::size_t>(n_ops));
    hashes.reserve(static_cast<std::size_t>(n_ops));
    auto t0 = std::chrono::steady_clock::now();
    for (unsigned long i = 0; i < n_ops; ++i) {
        keys.push_back(i);
        hashes.push_back(v39_hash::mix(i));
    }
    std::uint64_t hits = 0;
    for (unsigned long i = 0; i < n_ops; ++i) {
        std::uint64_t target = v39_hash::mix(i);
        for (std::size_t j = 0; j < hashes.size(); ++j) {
            if (hashes[j] == target) { ++hits; break; }
        }
    }
    auto t1 = std::chrono::steady_clock::now();
#else  // hybrid
    // V41.A5: hybrid -> echter Radix-Index (4-bit nibbles, ART-vereinfacht)
    // statt brute-force pair-vector. Zeigt trie-Charakteristik (O(log_16 N) depth).
    comdare::cache_engine::indexes::RadixIndex store;
    auto t0 = std::chrono::steady_clock::now();
    for (unsigned long i = 0; i < n_ops; ++i) {
        store.insert(static_cast<std::uint32_t>(v39_hash::mix(i)));
    }
    std::uint64_t hits = 0;
    for (unsigned long i = 0; i < n_ops; ++i) {
        if (store.contains(static_cast<std::uint32_t>(v39_hash::mix(i)))) ++hits;
    }
    auto t1 = std::chrono::steady_clock::now();
#endif

    if (hits != static_cast<std::uint64_t>(n_ops)) {
        return -2;
    }
    double total_us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    *out_micros_per_op = total_us / static_cast<double>(n_ops * 2);
    return 0;
}

// V38.C - Einheitliches Entry-Symbol fuer Plugin-Loader (ausserhalb namespace).
// V40.D: axes string enthaelt real= zur Markierung wenn vendored allocator aktiv ist
static constexpr char kAxes_@@PERM_ID@@[] =
    "simd=@@SIMD@@,layout=@@LAYOUT@@,alloc=@@ALLOC@@ (real=" COMDARE_ALLOC_NAME ")";
static constexpr PermDescriptor kDescriptor_@@PERM_ID@@ {
    COMDARE_PERM_ID,
    COMDARE_PERM_VERSION,
    kAxes_@@PERM_ID@@,
    &perm_@@PERM_ID@@_run
};

extern "C" COMDARE_PERM_EXPORT const PermDescriptor* comdare_perm_descriptor() {
    return &kDescriptor_@@PERM_ID@@;
}
)PERMTPL";

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

std::string axis_version_lookup(std::string_view axes_versions_content, std::string_view axis_key) {
    // codegen.cmake Z.149-156: REGEX MATCH "axis_<key>=([^\n\r]+)", default "v0".
    std::string const needle = "axis_" + std::string{axis_key} + "=";
    return match_field(axes_versions_content, needle, "v0");
}

std::string current_axes_signature(std::string_view axes_versions_content, Permutation const& perm) {
    // codegen.cmake Z.197-207.
    std::string const av_simd   = axis_version_lookup(axes_versions_content, "12_simd_" + perm.simd);
    std::string const av_layout = axis_version_lookup(axes_versions_content, "4_layout_" + perm.layout);
    std::string const av_alloc  = axis_version_lookup(axes_versions_content, "6_alloc_" + perm.alloc);
    std::string       av_node   = "v0";
    std::string       av_concur = "v0";
    if (perm.extended) { // codegen.cmake Z.203: if(_node) — node non-empty nur bei medium/full
        av_node   = axis_version_lookup(axes_versions_content, "1_node_" + perm.node);
        av_concur = axis_version_lookup(axes_versions_content, "8_concurrency_" + perm.concur);
    }
    return "simd=" + av_simd + ";layout=" + av_layout + ";alloc=" + av_alloc + ";node=" + av_node +
           ";concur=" + av_concur;
}

std::string bump_minor(std::string_view in_version) {
    // codegen.cmake Z.159-165: MATCHES "([0-9]+)\.([0-9]+)\.([0-9]+)" -> maj.(min+1).0, sonst 0.1.0.
    auto const is_digit = [](char c) { return c >= '0' && c <= '9'; };
    for (std::size_t i = 0; i < in_version.size(); ++i) {
        if (!is_digit(in_version[i])) { continue; }
        std::size_t a = i;
        while (a < in_version.size() && is_digit(in_version[a])) { ++a; }
        if (a >= in_version.size() || in_version[a] != '.') { continue; }
        std::size_t const b_start = a + 1;
        std::size_t       b       = b_start;
        while (b < in_version.size() && is_digit(in_version[b])) { ++b; }
        if (b == b_start || b >= in_version.size() || in_version[b] != '.') { continue; }
        std::size_t const c_start = b + 1;
        std::size_t       c       = c_start;
        while (c < in_version.size() && is_digit(in_version[c])) { ++c; }
        if (c == c_start) { continue; }
        std::string const maj{in_version.substr(i, a - i)};
        long const        minor = std::stol(std::string{in_version.substr(b_start, b - b_start)});
        return maj + "." + std::to_string(minor + 1) + ".0";
    }
    return "0.1.0";
}

std::string_view cpp_last_codegen_marker() noexcept {
    // Ehrlich: kennzeichnet das cpp-Backend, behauptet NICHT codegen.cmake. Der Byte-Identitaets-Test
    // schliesst genau die `last_codegen=`-Zeile aus.
    return "comdare-permutation-codegen (cpp backend; C++23-Port von tools/permutation_codegen/codegen.cmake)";
}

std::string render_wrapper_source(Inputs const& inputs, Permutation const& perm, std::string_view stored_version,
                                  std::string_view current_axes_sig) {
    // _extra_defines (codegen.cmake Z.242-247): leer bei smoke; sonst Node/Concurrency-Defines,
    // beginnend mit \n, ohne abschliessendes \n.
    std::string extra_defines;
    if (perm.extended) {
        extra_defines = "\n#define COMDARE_PERM_NODE \"" + perm.node + "\"\n#define COMDARE_PERM_CONCURRENCY \"" +
                        perm.concur + "\"";
    }

    std::string out{kWrapperTemplate};
    // Reihenfolge: die AXIS_MACRO_*-Platzhalter VOR den kurzen @@SIMD@@/@@LAYOUT@@/@@ALLOC@@ ersetzen
    // (Substitutionswerte enthalten selbst keine @@...@@-Tokens -> ansonsten ordnungs-unabhaengig).
    replace_all(out, "@@EXTRA_DEFINES@@", extra_defines);
    replace_all(out, "@@AXIS_MACRO_SIMD@@", "COMDARE_PERM_SIMD_IS_" + ascii_upper(perm.simd));
    replace_all(out, "@@AXIS_MACRO_LAYOUT@@", "COMDARE_PERM_LAYOUT_IS_" + ascii_upper(perm.layout));
    replace_all(out, "@@AXIS_MACRO_ALLOC@@", "COMDARE_PERM_ALLOC_IS_" + ascii_upper(perm.alloc));
    replace_all(out, "@@PERM_ID@@", perm.id());
    replace_all(out, "@@PROFILE@@", std::string{profile_name(inputs.profile)});
    replace_all(out, "@@ISA@@", inputs.target_isa);
    replace_all(out, "@@STORED_VERSION@@", stored_version);
    replace_all(out, "@@AXES_SIG@@", current_axes_sig);
    replace_all(out, "@@SIMD@@", perm.simd);
    replace_all(out, "@@LAYOUT@@", perm.layout);
    replace_all(out, "@@ALLOC@@", perm.alloc);
    return out;
}

std::string render_version_file(Permutation const& perm, std::string_view stored_version,
                                std::string_view current_axes_sig) {
    // codegen.cmake Z.504-509 (last_codegen = eigener cpp-Marker, ehrlich).
    std::string out = "perm_id=" + perm.id() + "\n";
    out += "version=" + std::string{stored_version} + "\n";
    out += "axes=" + std::string{current_axes_sig} + "\n";
    out += "last_codegen=" + std::string{cpp_last_codegen_marker()} + "\n";
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

    // (1) permutations.cmake (codegen.cmake Z.670).
    if (!write_file_binary(inputs.output, render_permutations_cmake(inputs, perms), error_out)) { return false; }

    // (2) permutations_manifest.txt (codegen.cmake Z.33/525).
    if (!write_file_binary(out_dir / "permutations_manifest.txt", render_manifest(inputs, perms), error_out)) {
        return false;
    }

    // (3)+(4) per-Permutation Wrapper + Versionierung (V36.E selective rebuild, codegen.cmake Z.140-514).
    std::string axes_versions_content; // leer => alle "v0" (wie fehlende Datei, codegen.cmake Z.143-146)
    if (!inputs.axes_versions.empty()) { axes_versions_content = read_file_to_string(inputs.axes_versions); }

    std::filesystem::path const perm_src_dir      = out_dir / "perm_src";      // codegen.cmake Z.32
    std::filesystem::path const perm_versions_dir = out_dir / "perm_versions"; // codegen.cmake Z.168
    std::filesystem::create_directories(perm_src_dir, ec);
    std::filesystem::create_directories(perm_versions_dir, ec);

    for (Permutation const& perm : perms) {
        std::string const           id           = perm.id();
        std::filesystem::path const wrapper_path = perm_src_dir / ("perm_" + id + ".cpp");
        std::filesystem::path const version_path = perm_versions_dir / ("perm_" + id + ".version");

        std::string const current_sig = current_axes_signature(axes_versions_content, perm);

        // gespeicherte Version + Achsen-Signatur (codegen.cmake Z.209-222).
        std::string stored_version = "0.1.0";
        std::string stored_sig;
        if (std::filesystem::exists(version_path)) {
            std::string const vf = read_file_to_string(version_path);
            stored_version       = match_field(vf, "version=", "0.1.0");
            stored_sig           = match_field(vf, "axes=", "");
        }

        // selective Rebuild (codegen.cmake Z.224-237).
        bool needs_rebuild = false;
        if (!std::filesystem::exists(wrapper_path)) {
            needs_rebuild = true;
        } else if (inputs.mode == "on_rebuild") {
            needs_rebuild = true;
        } else if (stored_sig != current_sig) {
            needs_rebuild  = true;
            stored_version = bump_minor(stored_version);
        }

        if (!needs_rebuild) { continue; } // codegen.cmake Z.239-240: skip

        if (!write_file_binary(wrapper_path, render_wrapper_source(inputs, perm, stored_version, current_sig),
                               error_out)) {
            return false;
        }
        if (!write_file_binary(version_path, render_version_file(perm, stored_version, current_sig), error_out)) {
            return false;
        }
    }

    return true;
}

} // namespace comdare::cache_engine::builder::permutation_codegen
