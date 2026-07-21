// test_cache_mc_timeout -- Cache-Resthygiene (2026-07-21): der harte Wall-Clock-Cap fuer mc-Shellouts (Push UND Pull)
// gegen Netz-Blackhole/TLS-Stall. Belegt:
//   (1) build_mc_argv (rein/statisch): timeout verfuegbar + s>0 => `timeout -k 5 <s> mc ...` (Kommando-Aufbau LITERAL);
//       timeout nicht verfuegbar ODER s==0 => argv UNVERAENDERT.
//   (2) End-to-End: ein HAENGENDES mc (fake, sleep) terminiert BOUNDED ueber den timeout-Wrapper -> push_tier_binary
//       kehrt zurueck (kein Hang), Fehlerdoktrin ArtefaktIo + lokale Kopie bleibt. (Gegatet auf verfuegbares `timeout`.)
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS.

#include "builder/artifact_transport/artifact_cache.hpp"
#include "comdare_test_tmp.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace at = comdare::cache_engine::builder::artifact_transport;

static int  g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    // ── (1) build_mc_argv: der Kommando-Aufbau LITERAL. ──
    {
        std::vector<std::string> const base    = {"mc", "cp", "--quiet", "src", "dst"};
        auto const                     wrapped = at::ArtifactCache::build_mc_argv(base, "timeout", true, 120);
        std::vector<std::string> const expect  = {"timeout", "-k", "5", "120", "mc", "cp", "--quiet", "src", "dst"};
        check_true("(1) verfuegbar+s=120 => `timeout -k 5 120 mc ...` (literal)", wrapped == expect);

        check_true("(1) NICHT verfuegbar => argv unveraendert",
                   at::ArtifactCache::build_mc_argv(base, "timeout", false, 120) == base);
        check_true("(1) s==0 => argv unveraendert", at::ArtifactCache::build_mc_argv(base, "timeout", true, 0) == base);
        check_true("(1) leeres argv => leer (kein Wrapper)",
                   at::ArtifactCache::build_mc_argv({}, "timeout", true, 120).empty());
    }

    // ── (2) End-to-End: haengendes mc => Cap greift. Nur wenn `timeout` verfuegbar ist (sonst Wrapper inaktiv). ──
    bool const timeout_present = (std::system("timeout --version >/dev/null 2>&1") == 0);
    if (!timeout_present) {
        std::cout
            << "  [INFO] `timeout`-Binary nicht verfuegbar -> Wrapper inaktiv, End-to-End-Hang-Test uebersprungen "
               "(build_mc_argv-Kontrakt oben deckt den Aufbau)\n";
    } else {
        std::error_code             ec;
        std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_mc_timeout";
        std::filesystem::remove_all(base, ec);
        std::filesystem::create_directories(base, ec);
        std::filesystem::path const bin_dir = base / "perm_hang";
        std::filesystem::create_directories(bin_dir, ec);
        { std::ofstream{bin_dir / "perm.dll", std::ios::binary} << "DLLBYTES"; }
        { std::ofstream{bin_dir / "perm.dll.version", std::ios::binary} << "m3v2"; }

        // Fake-mc, das HAENGT (sleep 20) -> ohne Cap wuerde der Push 20s blockieren; mit `timeout 2` ~2-7s.
        std::filesystem::path const fakemc = base / "fake_mc_hang.sh";
        { std::ofstream{fakemc} << "#!/bin/sh\nsleep 20\nexit 0\n"; }
        std::filesystem::permissions(fakemc, std::filesystem::perms::owner_all, ec);

        ::setenv("COMDARE_MINIO_ENDPOINT", "fakealias", 1);
        ::setenv("COMDARE_MINIO_BUCKET", "fakebucket", 1);
        ::setenv("COMDARE_MC_BIN", fakemc.string().c_str(), 1);
        ::setenv("COMDARE_MC_TIMEOUT_S", "2", 1);   // Push-Cap 2s
        ::setenv("COMDARE_ARTEFAKT_TRIES", "1", 1); // 1 Versuch -> obere Schranke = 1 x (Cap + Grace)
        ::setenv("COMDARE_ARTEFAKT_RETRY_SLEEP_S", "0", 1);
        ::unsetenv("COMDARE_MEASUREMENT_DROP_URL");

        at::ArtifactCache const cache = at::ArtifactCache::from_env();
        check_true("(2) minio_enabled (fake)", cache.minio_enabled());

        double const bound_s = /* tries */ 1.0 * (/* cap */ 2.0 + /* grace */ 5.0) + /* spawn/OS */ 20.0;
        auto const   t0      = std::chrono::steady_clock::now();
        cache.push_tier_binary(bin_dir, "m3v2"); // MUSS bounded zurueckkehren (kein 20s-Hang je Versuch), kein throw
        auto const   t1    = std::chrono::steady_clock::now();
        double const dur_s = std::chrono::duration<double>(t1 - t0).count();
        std::cout << "  [DAUER] push_tier_binary gegen haengendes mc terminierte nach " << dur_s
                  << " s (obere Schranke " << bound_s << " s; cap=2s tries=1)\n";
        check_true("(2) push TERMINIERT bounded (Cap greift, kein 20s-Hang)", dur_s < bound_s);
        check_true("(2) lokale perm.dll bleibt (ArtefaktIo-Doktrin: MESSEN WEITER)",
                   std::filesystem::exists(bin_dir / "perm.dll", ec));

        ::unsetenv("COMDARE_MINIO_ENDPOINT");
        ::unsetenv("COMDARE_MINIO_BUCKET");
        ::unsetenv("COMDARE_MC_BIN");
        ::unsetenv("COMDARE_MC_TIMEOUT_S");
        ::unsetenv("COMDARE_ARTEFAKT_TRIES");
        ::unsetenv("COMDARE_ARTEFAKT_RETRY_SLEEP_S");
        std::filesystem::remove_all(base, ec);
    }

    std::cout << "\n==== Cache-Resthygiene mc-Timeout-Wrapper: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
