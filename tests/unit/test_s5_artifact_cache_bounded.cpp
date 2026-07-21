// test_s5_artifact_cache_bounded -- S5-Rest (Blackhole-Wache): der measure-drop-PUT (Ebene C, curl-Shellout in
// ArtifactCache::sink_measurement -> curl_put) MUSS auch gegen ein Netz-Blackhole BOUNDED TERMINIEREN, nicht
// unbegrenzt haengen. Regression: measure:smoke (Job 285358) hing 8h an einem haengenden curl-PUT und hielt dabei
// die resource_group ceb-measurement-exclusive endlos. Der Fix haengt --connect-timeout/--max-time JE VERSUCH an
// den curl-Aufruf (aus env uebersteuerbar: COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S/_MAX_TIME_S/_TRIES/_RETRY_SLEEP_S).
//
// Dieser Test schaltet Ebene C scharf (COMDARE_MEASUREMENT_DROP_URL zeigt auf ein unerreichbares Blackhole) mit
// klein konfigurierten env-Werten und belegt: sink_measurement kehrt in beschraenkter Zeit zurueck (kein Hang),
// die lokale Kopie bleibt (honest: Fehler -> Log + Datei bleibt, MESSEN WEITER). Die literale Dauer wird ausgegeben.
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_w11).

#include "builder/artifact_transport/artifact_cache.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // S1: +ceb-Segment-Konstanten fuer die Key-Montage-Probe

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

namespace at = comdare::cache_engine::builder::artifact_transport;

static int  g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::error_code             ec;
    std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_s5_bounded";
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(base, ec);

    // Kleine Mess-Datei, die "abgelegt" werden soll (existiert -> sink_measurement versucht den PUT).
    std::filesystem::path const local = base / "cell_measurement.csv";
    {
        std::ofstream f{local, std::ios::binary | std::ios::trunc};
        f << "axis,value\nbounded_probe,1\n";
    }

    // Ebene C scharf gegen ein BLACKHOLE. 10.255.255.1 ist regulaer unerreichbar -> curl-connect haengt, bis
    // --connect-timeout greift (bzw. terminiert per unreachable). Klein konfigurierte Gates halten den Test kurz:
    //   connect 2s, max-time 2s je Versuch, 3 Versuche, kein Retry-Schlaf. Ohne den Fix wuerde curl_put unbegrenzt
    //   auf dem ersten haengenden Versuch stehen (die 8h-Regression).
    ::setenv("COMDARE_MEASUREMENT_DROP_URL", "https://10.255.255.1/", 1);
    ::setenv("COMDARE_NFS_DROP_TOKEN", "dummy-token-not-logged", 1);
    ::setenv("COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S", "2", 1);
    ::setenv("COMDARE_ARTEFAKT_MAX_TIME_S", "2", 1);
    ::setenv("COMDARE_ARTEFAKT_TRIES", "3", 1);
    ::setenv("COMDARE_ARTEFAKT_RETRY_SLEEP_S", "0", 1);
    ::unsetenv("COMDARE_MINIO_ENDPOINT"); // Ebene B aus (nur der curl-Pfad wird geprueft)
    ::unsetenv("COMDARE_MINIO_BUCKET");

    at::ArtifactCache const cache = at::ArtifactCache::from_env();
    check_true("Ebene C scharf (drop_enabled)", cache.drop_enabled());
    check_true("Ebene B inert (kein minio)", !cache.minio_enabled());

    // Konservative obere Schranke aus der Konfiguration: 3 Versuche x max-time 2s + Prozess-/Spawn-Reserve.
    // Grosszuegig bemessen -- der Regressionsbeweis ist "terminiert ueberhaupt" (statt 8h Hang), nicht die exakte s.
    double const bound_s = /* tries */ 3.0 * /* max_time */ 2.0 + /* Spawn/OS-Reserve */ 30.0;

    auto const t0 = std::chrono::steady_clock::now();
    cache.sink_measurement(local, "run/cell_measurement.csv"); // MUSS zurueckkehren (kein Hang), kein throw
    auto const t1 = std::chrono::steady_clock::now();

    double const dur_s = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "  [DAUER] sink_measurement gegen Blackhole terminierte nach " << dur_s << " s"
              << " (obere Schranke " << bound_s << " s; tries=3 x max-time=2s)\n";
    check_true("sink_measurement TERMINIERT bounded (kein 8h-Hang)", dur_s < bound_s);

    // Honest-Verhalten: Fehler -> lokale Kopie bleibt (nie geloescht), MESSEN WEITER.
    check_true("lokale Mess-Kopie bleibt nach fehlgeschlagenem PUT erhalten", std::filesystem::exists(local, ec));

    // S1 (#46a Key-Haertung): die Objekt-Store-Key-Naht cache_key_prefix montiert +ceb/+mtool/+mrg an die
    // build_version. Ebene B bleibt hier inert -- cache_key_prefix ist davon unabhaengig (nur der Push waere No-Op).
    {
        ::setenv("COMDARE_MEASUREMENT_COMBO", "[wallclock]", 1);
        at::ArtifactCache const kc       = at::ArtifactCache::from_env();
        std::string const       bv       = "m3v2+cxx=g++-16+opt=O2+ext=avx2";
        std::string const       ceb      = "+ceb=" + std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
                                           std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor);
        std::string const       expected = bv + ceb + "+mtool=_wallclock_" + "+mrg=none";
        std::string const       got      = kc.cache_key_prefix(bv);
        std::cout << "  [KEY] cache_key_prefix = '" << got << "'\n";
        check_true("cache_key_prefix montiert +ceb/+mtool/+mrg (Stringgleichheit)", got == expected);
        ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    }

    ::unsetenv("COMDARE_MEASUREMENT_DROP_URL");
    ::unsetenv("COMDARE_NFS_DROP_TOKEN");
    ::unsetenv("COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S");
    ::unsetenv("COMDARE_ARTEFAKT_MAX_TIME_S");
    ::unsetenv("COMDARE_ARTEFAKT_TRIES");
    ::unsetenv("COMDARE_ARTEFAKT_RETRY_SLEEP_S");
    std::filesystem::remove_all(base, ec);

    std::cout << "\n==== S5-Rest ArtifactCache Blackhole-Wache: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
