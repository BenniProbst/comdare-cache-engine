// test_s5_artifact_cache_bounded -- S5-Rest (Blackhole-Wache): der measure-drop-PUT (Ebene C, curl-Shellout in
// ArtifactCache::sink_measurement -> curl_put) MUSS auch gegen ein Netz-Blackhole BOUNDED TERMINIEREN, nicht
// unbegrenzt haengen. Regression: measure:smoke (Job 285358) hing 8h an einem haengenden curl-PUT und hielt dabei
// die resource_group ceb-measurement-exclusive endlos. Der Fix haengt --connect-timeout/--max-time JE VERSUCH an
// den curl-Aufruf (aus env uebersteuerbar: COMDARE_ARTEFAKT_CONNECT_TIMEOUT_S/_MAX_TIME_S/_TRIES/_RETRY_SLEEP_S).
//
// Dieser Test schaltet Ebene C scharf (COMDARE_MEASUREMENT_DROP_URL zeigt auf ein unerreichbares Blackhole) mit
// klein konfigurierten env-Werten und belegt: sink_measurement kehrt in beschraenkter Zeit zurueck (kein Hang),
// die lokale Kopie bleibt (honest: Fehler -> Log + Datei bleibt, MESSEN WEITER). Die literale Dauer wird ausgegeben.
//
// ZWEITER TEIL (N7-D2, Lock-Sektions-Budget): dieselbe Frage fuer die mc-Seite (Ebene B). ArtifactCache::
// with_object_budget liefert eine KOPIE der Instanz mit knapp budgetierten mc-Nahten, damit der Bestandslog-Transport
// unter einem Dokument-Lock nicht 50 min Netz-Retries in der Lock-Sektion haelt (Worst-Sektion der Push-Defaults:
// 12 Versuche x 120 s). Belegt wird BEHAVIORAL (keine privaten Felder): welche Caps und wie viele Versuche real an
// die mc-Spawns gehen -- gemessen ueber einen Fake-`timeout`-Wrapper, der Cap und Verb je Spawn protokolliert.
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_w11).

#include "builder/artifact_transport/artifact_cache.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // S1: +ceb-Segment-Konstanten fuer die Key-Montage-Probe

#include <chrono>
#include <cstddef> // N7-D2: std::size_t (Zaehl-Lambda der mc-Spawn-Protokollzeilen)
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

    // ---- N7-D2: with_object_budget -- die knapp budgetierte ZWEIT-INSTANZ fuer den Bestandslog-Transport. Belegt
    //      BEHAVIORAL (nicht ueber private Felder): die Kopie ersetzt GENAU die vier Budget-Zahlen (tries_,
    //      pull_tries_, mc_push_timeout_s_, mc_pull_timeout_s_) und laesst alles andere identisch.
    //      Messpunkt ist der Wall-Clock-Cap-Wrapper: build_mc_argv baut `timeout -k 5 <cap> mc <verb> ...`. Ein
    //      Fake-`timeout` (COMDARE_TIMEOUT_BIN) protokolliert also je mc-Spawn Cap UND Verb und startet danach den
    //      Fake-mc. Der Fake-mc laesst `stat` gelingen und `cp` FEHLSCHLAGEN -> die Retry-Schleifen laufen voll aus
    //      und die Zahl der cp-Zeilen IST das Try-Budget.
    //      WARUM das zaehlt: die Push-Defaults (12 x 120s) ergeben eine Lock-Sektion von ~50 min und sprengen jede
    //      Lock-ttl -> zwei Schreiber am Bestandslog-Dokument. Die Kopie ist der Schnitt dagegen.
    {
        std::filesystem::path const bbase = base / "budget";
        std::filesystem::create_directories(bbase, ec);
        ::setenv("TMPDIR", bbase.string().c_str(), 1); // haelt die Objekt-TEMP-/Log-Reste im aufgeraeumten Baum

        std::filesystem::path const calls  = bbase / "mc_calls.log"; // je mc-Spawn EINE Zeile "CAP=<s> VERB=<verb>"
        std::filesystem::path const fakemc = bbase / "fake_mc.sh";
        std::filesystem::path const faketo = bbase / "fake_timeout.sh";
        {
            // stat gelingt (Objekt "existiert"), cp schlaegt fehl -> mc_cp/mc_pull laufen ihr Try-Budget voll aus.
            std::ofstream f{fakemc};
            f << "#!/bin/sh\n"
                 "if [ \"$1\" = \"stat\" ]; then echo '{\"size\": 1}'; exit 0; fi\n"
                 "exit 1\n";
        }
        {
            // argv des Wrappers: -k 5 <cap> <mc_bin> <verb> ... -> $3=cap, $5=verb. `--version` ist die
            // Verfuegbarkeits-Probe aus from_env (probe_timeout) und MUSS rc 0 liefern, sonst bleibt der Wrapper aus.
            std::ofstream f{faketo};
            f << "#!/bin/sh\n"
                 "if [ \"$1\" = \"--version\" ]; then echo 'fake timeout'; exit 0; fi\n"
                 "echo \"CAP=$3 VERB=$5\" >> \""
              << calls.string()
              << "\"\n"
                 "shift 3\n"
                 "exec \"$@\"\n";
        }
        std::filesystem::permissions(fakemc, std::filesystem::perms::owner_all, ec);
        std::filesystem::permissions(faketo, std::filesystem::perms::owner_all, ec);

        ::setenv("COMDARE_MINIO_ENDPOINT", "fakealias", 1);
        ::setenv("COMDARE_MINIO_BUCKET", "fakebucket", 1);
        ::setenv("COMDARE_MC_BIN", fakemc.string().c_str(), 1);
        ::setenv("COMDARE_TIMEOUT_BIN", faketo.string().c_str(), 1);
        ::setenv("COMDARE_MC_TIMEOUT_S", "111", 1);         // Push-Cap der BASIS-Instanz
        ::setenv("COMDARE_MC_PULL_TIMEOUT_S", "222", 1);    // Pull-Cap der BASIS-Instanz
        ::setenv("COMDARE_ARTEFAKT_TRIES", "7", 1);         // Push-Tries der BASIS-Instanz
        ::setenv("COMDARE_ARTEFAKT_PULL_TRIES", "5", 1);    // Pull-Tries der BASIS-Instanz
        ::setenv("COMDARE_ARTEFAKT_RETRY_SLEEP_S", "0", 1); // keine Pausen -> Test bleibt kurz
        ::setenv("COMDARE_ARTEFAKT_PULL_RETRY_SLEEP_S", "0", 1);
        ::setenv("COMDARE_MEASUREMENT_DROP_URL", "https://10.255.255.1/",
                 1); // Ebene C bleibt SCHARF (Identitaets-Probe)

        at::ArtifactCache const basis  = at::ArtifactCache::from_env();
        at::ArtifactCache const budget = basis.with_object_budget(1, 3);
        check_true("N7-D2: Basis-Instanz minio_enabled (fake)", basis.minio_enabled());

        auto const count = [&](std::string const& want) {
            std::ifstream lf{calls};
            std::size_t   n = 0;
            for (std::string l; std::getline(lf, l);)
                if (l == want) ++n;
            return n;
        };
        auto const reset = [&]() { std::filesystem::remove(calls, ec); };

        // (a) BASIS: Push-Weg -> 7 cp-Versuche mit Cap 111; Pull-Weg -> 1 stat + 5 cp-Versuche mit Cap 222.
        reset();
        check_true("N7-D2 (a) Basis object_store schlaegt fehl (Fake-mc cp exit 1)",
                   !basis.object_store("bestand/doc.xml", "<x/>"));
        std::cout << "  [CALLS] Basis-Push: cp@111 = " << count("CAP=111 VERB=cp") << "\n";
        check_true("N7-D2 (a) Basis: 7 cp-Versuche mit Push-Cap 111 (COMDARE_ARTEFAKT_TRIES/_MC_TIMEOUT_S)",
                   count("CAP=111 VERB=cp") == 7);
        reset();
        check_true("N7-D2 (a) Basis object_fetch liefert nullopt", !basis.object_fetch("bestand/doc.xml").has_value());
        std::cout << "  [CALLS] Basis-Pull: stat@222 = " << count("CAP=222 VERB=stat")
                  << ", cp@222 = " << count("CAP=222 VERB=cp") << "\n";
        check_true("N7-D2 (a) Basis: stat mit Pull-Cap 222", count("CAP=222 VERB=stat") == 1);
        check_true("N7-D2 (a) Basis: 5 cp-Versuche mit Pull-Cap 222 (COMDARE_ARTEFAKT_PULL_TRIES)",
                   count("CAP=222 VERB=cp") == 5);

        // (b) KOPIE: ALLE VIER Budgets ersetzt -> je EIN Versuch, Cap 3 auf BEIDEN Wegen.
        reset();
        check_true("N7-D2 (b) Kopie object_store schlaegt fehl", !budget.object_store("bestand/doc.xml", "<x/>"));
        std::cout << "  [CALLS] Kopie-Push: cp@3 = " << count("CAP=3 VERB=cp") << "\n";
        check_true("N7-D2 (b) Kopie: GENAU 1 cp-Versuch mit Cap 3 (tries_ + mc_push_timeout_s_ ersetzt)",
                   count("CAP=3 VERB=cp") == 1);
        check_true("N7-D2 (b) Kopie: KEIN Push-Versuch mit dem Basis-Cap 111", count("CAP=111 VERB=cp") == 0);
        reset();
        check_true("N7-D2 (b) Kopie object_fetch liefert nullopt", !budget.object_fetch("bestand/doc.xml").has_value());
        std::cout << "  [CALLS] Kopie-Pull: stat@3 = " << count("CAP=3 VERB=stat")
                  << ", cp@3 = " << count("CAP=3 VERB=cp") << "\n";
        check_true("N7-D2 (b) Kopie: stat mit Cap 3 (mc_pull_timeout_s_ ersetzt)", count("CAP=3 VERB=stat") == 1);
        check_true("N7-D2 (b) Kopie: GENAU 1 Pull-cp-Versuch mit Cap 3 (pull_tries_ ersetzt)",
                   count("CAP=3 VERB=cp") == 1);
        check_true("N7-D2 (b) Kopie: KEIN Pull-Versuch mit dem Basis-Cap 222", count("CAP=222 VERB=cp") == 0);

        // (c) ALLES ANDERE IDENTISCH: die Key-Montage ist die kritische Groesse -- eine abweichende cache_key_prefix
        //     wuerde den Bestandslog-Transport auf einen anderen Objekt-Namensraum zeigen lassen.
        std::string const bvk = "m3v2+cxx=g++-16+opt=O2+ext=avx2";
        check_true("N7-D2 (c) cache_key_prefix UNVERAENDERT (Stringgleichheit)",
                   budget.cache_key_prefix(bvk) == basis.cache_key_prefix(bvk));
        check_true("N7-D2 (c) run_stamp UNVERAENDERT (kein zweiter Lauf-Baum)",
                   budget.run_stamp() == basis.run_stamp());
        check_true("N7-D2 (c) minio_enabled UNVERAENDERT (Ebene B)", budget.minio_enabled() == basis.minio_enabled());
        check_true("N7-D2 (c) drop_enabled UNVERAENDERT (Ebene C bleibt scharf)",
                   budget.drop_enabled() && budget.drop_enabled() == basis.drop_enabled());
        check_true("N7-D2 (c) inert UNVERAENDERT", budget.inert() == basis.inert());

        // (d) tries==0 wird auf 1 geklemmt (Doktrin from_env:167) -- ein 0-Budget wuerde JEDEN Push still verwerfen.
        reset();
        at::ArtifactCache const clamped = basis.with_object_budget(0, 9);
        check_true("N7-D2 (d) clamped object_store schlaegt fehl", !clamped.object_store("bestand/doc.xml", "<x/>"));
        std::cout << "  [CALLS] clamped(0,9)-Push: cp@9 = " << count("CAP=9 VERB=cp") << "\n";
        check_true("N7-D2 (d) tries==0 -> GENAU 1 Versuch (kein stiller Drop)", count("CAP=9 VERB=cp") == 1);

        ::unsetenv("COMDARE_MINIO_ENDPOINT");
        ::unsetenv("COMDARE_MINIO_BUCKET");
        ::unsetenv("COMDARE_MC_BIN");
        ::unsetenv("COMDARE_TIMEOUT_BIN");
        ::unsetenv("COMDARE_MC_TIMEOUT_S");
        ::unsetenv("COMDARE_MC_PULL_TIMEOUT_S");
        ::unsetenv("COMDARE_ARTEFAKT_PULL_TRIES");
        ::unsetenv("COMDARE_ARTEFAKT_PULL_RETRY_SLEEP_S");
        ::unsetenv("TMPDIR");
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
