// test_s1_cache_key_prefix -- S1 (#46a Key-Haertung): die EINE Objekt-Store-Key-Naht cache_key_prefix
// (artifact_cache.hpp, W12-B-Single-Source) montiert aus der per-Perm build_version PLUS drei Segmenten:
//   +ceb=<COMDARE_ANATOMY_ABI_MAJOR>.<kCebContractCodegenMinor>   (CEB-Contract-Version, ABI-Bump invalidiert alles)
//   +mtool=<sanitisierte COMDARE_MEASUREMENT_COMBO>               (je-Combo-DLL-Stempel; leer/[all] = Default)
//   +mrg=none                                                     (Reserve-Segment fuer den K6a-Merge-Stempel #37)
// Der Test asserted die NEUE Montage-Form LITERAL (exakte String-Gleichheit). Bewusst: der Key AENDERT sich (einmalige
// Bucket-Invalidierung, Dossier-Risiko 2) -- kein Alt-Identitaets-Anker. Der +ceb-Wert wird aus DENSELBEN Konstanten
// gebaut wie die Naht (Anti-Drift: ein ABI-Bump bricht den Test NICHT spurios, nur die Segment-FORM ist gepinnt).
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_w11/test_s5).

#include "builder/artifact_transport/artifact_cache.hpp"

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // COMDARE_ANATOMY_ABI_MAJOR + kCebContractCodegenMinor

#include <cstdlib>
#include <iostream>
#include <string>

namespace at  = comdare::cache_engine::builder::artifact_transport;
namespace abi = comdare::cache_engine::abi;

static int  g_fail = 0;
static void check_eq(char const* what, std::string const& got, std::string const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n        got  = '" << got << "'\n";
    if (!ok) std::cout << "        want = '" << want << "'\n";
    if (!ok) ++g_fail;
}

int main() {
    // Das +ceb-Segment aus DENSELBEN Konstanten wie die Naht (Single-Source, Anti-Drift).
    std::string const ceb =
        "+ceb=" + std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." + std::to_string(abi::kCebContractCodegenMinor);
    std::cout << "  [INFO] CEB-Contract-Segment (live-Konstanten) = '" << ceb << "'\n";

    std::string const bv = "m3v2+cxx=g++-16+opt=O2+ext=avx2";

    // ── Fall 1: COMDARE_MEASUREMENT_COMBO UNGESETZT (Default/[all] -> Director exportiert nichts) => +mtool= leer. ──
    {
        ::unsetenv("COMDARE_MEASUREMENT_COMBO");
        ::unsetenv("COMDARE_MINIO_ENDPOINT"); // inert-Instanz: cache_key_prefix arbeitet dennoch (nur Push ist No-Op)
        ::unsetenv("COMDARE_MINIO_BUCKET");
        at::ArtifactCache const cache    = at::ArtifactCache::from_env();
        std::string const       expected = bv + ceb + "+mtool=" + "+mrg=none";
        check_eq("Fall1: Default (kein Combo) -> +mtool= leer, +mrg=none Reserve", cache.cache_key_prefix(bv),
                 expected);
    }

    // ── Fall 2: COMDARE_MEASUREMENT_COMBO="[wallclock,macro]" -> sanitisiert '_wallclock_macro_' (orch_make_stem-Regel). ──
    {
        ::setenv("COMDARE_MEASUREMENT_COMBO", "[wallclock,macro]", 1);
        at::ArtifactCache const cache    = at::ArtifactCache::from_env();
        std::string const       expected = bv + ceb + "+mtool=_wallclock_macro_" + "+mrg=none";
        check_eq("Fall2: Combo [wallclock,macro] -> +mtool=_wallclock_macro_", cache.cache_key_prefix(bv), expected);
    }

    // ── Fall 3: Sonderzeichen-Sanitisierung ('[',']',',' -> '_'); nur [A-Za-z0-9] bleiben. ──
    {
        ::setenv("COMDARE_MEASUREMENT_COMBO", "[a,b,c]", 1);
        at::ArtifactCache const cache    = at::ArtifactCache::from_env();
        std::string const       expected = bv + ceb + "+mtool=_a_b_c_" + "+mrg=none";
        check_eq("Fall3: Combo [a,b,c] -> +mtool=_a_b_c_ (alle Nicht-alnum '_')", cache.cache_key_prefix(bv), expected);
    }

    // ── Fall 4: eine ANDERE build_version fliesst 1:1 als Praefix ein (die drei Segmente haengen additiv hinten an). ──
    {
        ::unsetenv("COMDARE_MEASUREMENT_COMBO");
        at::ArtifactCache const cache    = at::ArtifactCache::from_env();
        std::string const       bv2      = "m3v2+cxx=clang++-19+opt=O3+bt=Debug";
        std::string const       expected = bv2 + ceb + "+mtool=" + "+mrg=none";
        check_eq("Fall4: fremde build_version 1:1 als Praefix", cache.cache_key_prefix(bv2), expected);
    }

    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    std::cout << "\n==== S1 cache_key_prefix Segment-Montage (#46a Key-Haertung): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
