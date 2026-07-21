// test_w11_async_push_pump -- W11 (Ledger §43.c): der BAU-MODUS async Push-Pump + der ArtifactCache-Teil-Marker.
// Belegt (mock-basiert, KEIN echter mc/minio):
//   (1) Der Pump schiebt JEDE eingereihte bin_dir GENAU EINMAL durch die (serialisierte) Push-Naht -- auch unter
//       nebenlaeufiger enqueue aus mehreren Threads (Build-Worker-Simulation).
//   (2) close() DRAINT alle eingereihten Pushes (Vollstaendigkeits-Garantie), auch bei langsamer Push-Naht.
//   (3) Teil-Marker feuern nach je part_size Pushes mit korrektem, MONOTON steigendem part_index.
//   (4) ArtifactCache::push_chunk_partial_marker ruft mc mit dem KORREKTEN Objekt-Key (Marker-Skip-Pfad) --
//       via Fake-mc-Skript (COMDARE_MC_BIN), der bestehende mc-cp/stat-Zweig, ohne echtes minio.
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_kf16/test_w6).

#include "builder/artifact_transport/async_push_pump.hpp"
#include "builder/artifact_transport/artifact_cache.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace at = comdare::cache_engine::builder::artifact_transport;

static int  g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}
template <typename A, typename B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

int main() {
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 1: jede eingereihte bin_dir GENAU EINMAL gepusht; build_version korrekt durchgereicht.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::mutex               m;
        std::vector<std::string> pushed;
        std::string              seen_bv;
        auto                     push = [&](std::filesystem::path const& bin_dir, std::string const& bv) {
            std::lock_guard<std::mutex> lk(m);
            pushed.push_back(bin_dir.string());
            seen_bv = bv;
        };
        {
            at::AsyncPushPump pump{push, "m3v2+cxx=g++-16+opt=O2", {}, 0};
            for (int i = 0; i < 20; ++i) pump.enqueue(std::filesystem::path{"dir_" + std::to_string(i)});
            pump.close(); // Drain + join
            check_eq("Teil1: pushed_count nach close", pump.pushed_count(), std::size_t{20});
        }
        check_eq("Teil1: alle 20 gepusht", pushed.size(), std::size_t{20});
        std::set<std::string> uniq(pushed.begin(), pushed.end());
        check_eq("Teil1: alle 20 DISTINKT (kein Doppel/Verlust)", uniq.size(), std::size_t{20});
        check_eq("Teil1: build_version durchgereicht", seen_bv, std::string{"m3v2+cxx=g++-16+opt=O2"});
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 2: close() DRAINT alle Pushes, auch bei langsamer Push-Naht (Vollstaendigkeits-Garantie).
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::atomic<int> done{0};
        auto             slow_push = [&](std::filesystem::path const&, std::string const&) {
            std::this_thread::sleep_for(std::chrono::milliseconds(3)); // simuliert mc-cp-Latenz
            done.fetch_add(1);
        };
        at::AsyncPushPump pump{slow_push, "bv", {}, 0};
        for (int i = 0; i < 30; ++i) pump.enqueue(std::filesystem::path{"d" + std::to_string(i)});
        pump.close(); // MUSS alle 30 abwarten
        check_eq("Teil2: close() drainte ALLE (langsame Naht)", done.load(), 30);
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 3: nebenlaeufige enqueue aus mehreren Threads (Build-Worker-Simulation) -> kein Verlust/Doppel.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::mutex            m;
        std::set<std::string> pushed;
        auto                  push = [&](std::filesystem::path const& d, std::string const&) {
            std::lock_guard<std::mutex> lk(m);
            pushed.insert(d.string());
        };
        at::AsyncPushPump        pump{push, "bv", {}, 0};
        std::vector<std::thread> workers;
        for (int w = 0; w < 4; ++w)
            workers.emplace_back([&pump, w] {
                for (int i = 0; i < 50; ++i)
                    pump.enqueue(std::filesystem::path{"w" + std::to_string(w) + "_i" + std::to_string(i)});
            });
        for (auto& t : workers) t.join();
        pump.close();
        check_eq("Teil3: 4x50 nebenlaeufig -> 200 distinkt gepusht", pushed.size(), std::size_t{200});
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 4: Teil-Marker feuern nach je part_size Pushes, part_index MONOTON 1,2,3,... (Cluster-Resume).
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::mutex               m;
        std::vector<std::size_t> parts;
        std::string              seen_bv;
        auto                     push    = [](std::filesystem::path const&, std::string const&) {};
        auto                     partial = [&](std::string const& bv, std::size_t part) {
            std::lock_guard<std::mutex> lk(m);
            parts.push_back(part);
            seen_bv = bv;
        };
        {
            at::AsyncPushPump pump{push, "bvX", partial, 4}; // Teil-Marker nach je 4 Pushes
            for (int i = 0; i < 10; ++i) pump.enqueue(std::filesystem::path{"d" + std::to_string(i)});
            pump.close();
        }
        // 10 Pushes / 4 => Teil-Marker bei 4 (part1) und 8 (part2); der Rest (9,10) traegt keinen vollen Marker.
        check_eq("Teil4: 2 Teil-Marker (bei 4 und 8)", parts.size(), std::size_t{2});
        if (parts.size() == 2) {
            check_eq("Teil4: erster part_index == 1", parts[0], std::size_t{1});
            check_eq("Teil4: zweiter part_index == 2", parts[1], std::size_t{2});
        }
        check_eq("Teil4: build_version an den Marker durchgereicht", seen_bv, std::string{"bvX"});
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 5: ArtifactCache::push_chunk_partial_marker ruft mc mit KORREKTEM Objekt-Key -- via Fake-mc.
    //   Objekt-Key MUSS = <build_version>/_gn_chunk_markers/<range':' -> '-'>.part<k>.done (Marker-Skip-Pfad).
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_w11_fakemc";
        std::error_code             ec;
        std::filesystem::remove_all(base, ec);
        std::filesystem::create_directories(base, ec);
        std::filesystem::path const store  = base / "store"; // Fake-Objekt-Store
        std::filesystem::path const fakemc = base / "fake_mc.sh";
        // Fake-mc: `cp --quiet SRC DST` kopiert SRC -> store/<key> (key = DST ohne alias/bucket); `stat --json DST` -> exit 0.
        {
            std::ofstream f{fakemc};
            f << "#!/bin/sh\n"
                 "STORE=\""
              << store.string()
              << "\"\n"
                 "if [ \"$1\" = \"cp\" ]; then\n"
                 "  SRC=\"$3\"; DST=\"$4\"\n"               // cp --quiet SRC DST
                 "  KEY=\"${DST#*/}\"; KEY=\"${KEY#*/}\"\n" // alias/ + bucket/ strippen
                 "  mkdir -p \"$STORE/$(dirname \"$KEY\")\"\n"
                 "  cp \"$SRC\" \"$STORE/$KEY\"\n"
                 "  exit 0\n"
                 "fi\n"
                 "if [ \"$1\" = \"stat\" ]; then\n"
                 "  DST=\"$3\"; KEY=\"${DST#*/}\"; KEY=\"${KEY#*/}\"\n"    // stat --json DST
                 "  SZ=$(wc -c < \"$STORE/$KEY\" 2>/dev/null || echo 0)\n" // echte Objekt-Groesse -> Verify passt sauber
                 "  echo \"{\\\"size\\\": $SZ}\"; exit 0\n"
                 "fi\n"
                 "exit 1\n";
        }
        std::filesystem::permissions(fakemc, std::filesystem::perms::owner_all, ec);

        ::setenv("COMDARE_MINIO_ENDPOINT", "fakealias", 1);
        ::setenv("COMDARE_MINIO_BUCKET", "fakebucket", 1);
        ::setenv("COMDARE_MC_BIN", fakemc.string().c_str(), 1);
        // KEINE measure-drop-Config -> Ebene C inert; nur Ebene B (mc) aktiv.
        ::unsetenv("COMDARE_MEASUREMENT_DROP_URL");

        at::ArtifactCache const cache = at::ArtifactCache::from_env();
        check_true("Teil5: ArtifactCache minio_enabled (fake)", cache.minio_enabled());
        std::string const bv5 = "m3v2+cxx=g++-16+opt=O2";
        cache.push_chunk_partial_marker(bv5, "32768:16384", 3);
        // Erwarteter Key: <key_prefix>/_gn_chunk_markers/32768-16384.part3.done (':' -> '-'). Der Praefix zieht NUR
        // aus cache_key_prefix (S1: inkl. +ceb/+mtool/+mrg) -- push_chunk_partial_marker teilt die Single-Source-Naht.
        std::filesystem::path const expect =
            store / cache.cache_key_prefix(bv5) / "_gn_chunk_markers" / "32768-16384.part3.done";
        check_true("Teil5: Teil-Marker unter korrektem Objekt-Key im Fake-Store", std::filesystem::exists(expect, ec));

        ::unsetenv("COMDARE_MINIO_ENDPOINT");
        ::unsetenv("COMDARE_MINIO_BUCKET");
        ::unsetenv("COMDARE_MC_BIN");
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // Teil 6 (S1): push_tier_binary schiebt DREI Objekte in HARTER Reihenfolge perm.dll -> perm.dll.algos ->
    //   perm.dll.version (Marke ZULETZT) unter dem Single-Source-Key (cache_key_prefix, inkl. +ceb/+mtool/+mrg).
    //   Ohne .algos (Organ-Gate aus) sauberer 2-Objekt-Push. Via Fake-mc, der die cp-Reihenfolge protokolliert.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_s1_push_order";
        std::error_code             ec;
        std::filesystem::remove_all(base, ec);
        std::filesystem::create_directories(base, ec);
        std::filesystem::path const store  = base / "store";
        std::filesystem::path const order  = base / "cp_order.log"; // je cp EINE Zeile = der Objekt-Key (Reihenfolge)
        std::filesystem::path const fakemc = base / "fake_mc.sh";
        {
            std::ofstream f{fakemc};
            f << "#!/bin/sh\n"
                 "STORE=\""
              << store.string() << "\"\nORDER=\"" << order.string()
              << "\"\n"
                 "if [ \"$1\" = \"cp\" ]; then\n"
                 "  SRC=\"$3\"; DST=\"$4\"\n"               // cp --quiet SRC DST
                 "  KEY=\"${DST#*/}\"; KEY=\"${KEY#*/}\"\n" // alias/ + bucket/ strippen
                 "  mkdir -p \"$STORE/$(dirname \"$KEY\")\"\n"
                 "  cp \"$SRC\" \"$STORE/$KEY\"\n"
                 "  echo \"$KEY\" >> \"$ORDER\"\n" // Reihenfolge-Protokoll
                 "  exit 0\n"
                 "fi\n"
                 "if [ \"$1\" = \"stat\" ]; then\n"
                 "  DST=\"$3\"; KEY=\"${DST#*/}\"; KEY=\"${KEY#*/}\"\n"
                 "  SZ=$(wc -c < \"$STORE/$KEY\" 2>/dev/null || echo 0)\n"
                 "  echo \"{\\\"size\\\": $SZ}\"; exit 0\n"
                 "fi\n"
                 "exit 1\n";
        }
        std::filesystem::permissions(fakemc, std::filesystem::perms::owner_all, ec);

        ::setenv("COMDARE_MINIO_ENDPOINT", "fakealias", 1);
        ::setenv("COMDARE_MINIO_BUCKET", "fakebucket", 1);
        ::setenv("COMDARE_MC_BIN", fakemc.string().c_str(), 1);
        ::setenv("COMDARE_MEASUREMENT_COMBO", "[wallclock,macro]", 1); // -> +mtool=_wallclock_macro_
        ::unsetenv("COMDARE_MEASUREMENT_DROP_URL");

        at::ArtifactCache const cache = at::ArtifactCache::from_env();
        check_true("Teil6: ArtifactCache minio_enabled (fake)", cache.minio_enabled());

        std::string const bv  = "m3v2+cxx=g++-16+opt=O2+ext=avx2";
        std::string const kp  = cache.cache_key_prefix(bv); // Single-Source-Praefix (inkl. +ceb/+mtool/+mrg)
        auto const        obj = [&](std::string const& stem, char const* leaf) { return kp + "/" + stem + "/" + leaf; };

        // (a) 3-Objekt-Push: perm.dll + perm.dll.algos + perm.dll.version vorhanden.
        {
            std::filesystem::remove(order, ec);
            std::filesystem::path const bin_dir = base / "perm_cellA";
            std::filesystem::create_directories(bin_dir, ec);
            { std::ofstream{bin_dir / "perm.dll", std::ios::binary} << "DLLBYTES-A"; }
            { std::ofstream{bin_dir / "perm.dll.algos", std::ios::binary} << "algo=v1"; }
            { std::ofstream{bin_dir / "perm.dll.version", std::ios::binary} << bv; }
            cache.push_tier_binary(bin_dir, bv);

            std::string const stem = bin_dir.filename().string();
            check_true("Teil6a: perm.dll im Store", std::filesystem::exists(store / kp / stem / "perm.dll", ec));
            check_true("Teil6a: perm.dll.algos im Store",
                       std::filesystem::exists(store / kp / stem / "perm.dll.algos", ec));
            check_true("Teil6a: perm.dll.version im Store",
                       std::filesystem::exists(store / kp / stem / "perm.dll.version", ec));

            std::vector<std::string> lines;
            {
                std::ifstream lf{order};
                for (std::string l; std::getline(lf, l);)
                    if (!l.empty()) lines.push_back(l);
            }
            check_eq("Teil6a: genau 3 cp-Aufrufe", lines.size(), std::size_t{3});
            if (lines.size() == 3) {
                // Reihenfolge-Assertion HART: dll -> algos -> version, plus Key-Montage-Stringgleichheit je Objekt.
                check_eq("Teil6a: 1. Push == perm.dll (Key-Montage)", lines[0], obj(stem, "perm.dll"));
                check_eq("Teil6a: 2. Push == perm.dll.algos (Key-Montage)", lines[1], obj(stem, "perm.dll.algos"));
                check_eq("Teil6a: 3. Push == perm.dll.version ZULETZT (Key-Montage)", lines[2],
                         obj(stem, "perm.dll.version"));
            }
        }

        // (b) 2-Objekt-Fallback: kein .algos lokal (Organ-Gate aus) -> perm.dll -> perm.dll.version, KEIN .algos-Push.
        {
            std::filesystem::remove(order, ec);
            std::filesystem::path const bin_dir = base / "perm_cellB";
            std::filesystem::create_directories(bin_dir, ec);
            { std::ofstream{bin_dir / "perm.dll", std::ios::binary} << "DLLBYTES-B"; }
            { std::ofstream{bin_dir / "perm.dll.version", std::ios::binary} << bv; }
            cache.push_tier_binary(bin_dir, bv);

            std::string const stem = bin_dir.filename().string();
            check_true("Teil6b: perm.dll.algos NICHT im Store (Gate aus)",
                       !std::filesystem::exists(store / kp / stem / "perm.dll.algos", ec));

            std::vector<std::string> lines;
            {
                std::ifstream lf{order};
                for (std::string l; std::getline(lf, l);)
                    if (!l.empty()) lines.push_back(l);
            }
            check_eq("Teil6b: genau 2 cp-Aufrufe (Organ-Gate aus)", lines.size(), std::size_t{2});
            if (lines.size() == 2) {
                check_eq("Teil6b: 1. Push == perm.dll", lines[0], obj(stem, "perm.dll"));
                check_eq("Teil6b: 2. Push == perm.dll.version ZULETZT", lines[1], obj(stem, "perm.dll.version"));
            }
        }

        ::unsetenv("COMDARE_MINIO_ENDPOINT");
        ::unsetenv("COMDARE_MINIO_BUCKET");
        ::unsetenv("COMDARE_MC_BIN");
        ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    }

    std::cout << "\n==== W11 async Push-Pump + Teil-Marker (§43.c): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
