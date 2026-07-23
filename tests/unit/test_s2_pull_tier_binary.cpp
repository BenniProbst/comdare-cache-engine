// test_s2_pull_tier_binary -- S2 (#46a Pull-Faehigkeit): ArtifactCache::pull_tier_binary (per-Binary-Spiegel zu push)
// + pull_tier_prefix (BATCH-Warm-Cache-Hydrierung). Belegt (mock-basiert, KEIN echter mc/minio, via COMDARE_MC_BIN):
//   (1) HIT: vollstaendiger Remote-Satz (dll+algos+version) -> pull_tier_binary=true, lokal alle drei da.
//   (2) MISS: kein Remote-Objekt -> pull_tier_binary=false, lokal NICHTS hydriert.
//   (3) HALB-PUSH (remote dll[+algos] aber KEINE .version) -> invertierte ZULETZT-Pruefung => MISS => false, kein Pull.
//   (4) MISMATCH-.algos => Neubau: nach dem Pull entscheidet AUSSCHLIESSLICH lokal dll_is_current -- passende
//       version+algos => HIT (skip), gebumpte algos => false (Neubau). (Korrektheits-Arbiter, Dossier-Risiko 1.)
//   (5) pull_tier_prefix (rekursiv): hydriert den ganzen Praefix -> dest/<stem>/perm.dll(+.algos,+.version); der
//       _gn_chunk_markers-Namensraum wird ausgespart.
//   (6) NEGATIV: unkonfiguriertes Env => inert() => pull_tier_binary/pull_tier_prefix=false, KEIN mc-Prozess-Spawn.
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_w11/test_s1).

#include "builder/artifact_transport/artifact_cache.hpp"
#include "builder/build_orchestrator/build_orchestrator.hpp" // ex::dll_is_current (Korrektheits-Arbiter nach dem Pull)
#include "comdare_test_tmp.hpp"                              // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

namespace at = comdare::cache_engine::builder::artifact_transport;
namespace ex = comdare::cache_engine::builder::experiment;

static int  g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

static void write_file(std::filesystem::path const& p, std::string const& content) {
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    f << content;
}

int main() {
    std::error_code             ec;
    std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_s2_pull";
    std::filesystem::remove_all(base, ec);
    std::filesystem::create_directories(base, ec);
    std::filesystem::path const store    = base / "store";             // Fake-Objekt-Store
    std::filesystem::path const spawnlog = base / "mc_spawned.marker"; // vom Fake-mc bei JEDEM Aufruf angelegt
    std::filesystem::path const fakemc   = base / "fake_mc_pull.sh";
    {
        // PULL-Fake-mc: stat = Existenz im Store (rc 0/1); cp --quiet REMOTE LOCAL = store->lokal (Pull-Richtung);
        // G5 (P-B): mirror --exclude PATTERN SRC/ DST/ = rekursiv store->lokal (inkrementell), _gn_chunk_markers
        // ausgespart. pull_tier_prefix nutzt jetzt `mc mirror` statt `mc cp --recursive` (retry-resumierbar).
        std::ofstream f{fakemc};
        f << "#!/bin/sh\n"
             "STORE=\""
          << store.string() << "\"\n"
          << "echo x >> \"" << spawnlog.string() << "\"\n" // Spawn-Beweis (Negativ-Test darf ihn NIE erzeugen)
          << "if [ \"$1\" = \"stat\" ]; then\n"
             "  T=\"$3\"; KEY=\"${T#*/}\"; KEY=\"${KEY#*/}\"\n"
             "  if [ -f \"$STORE/$KEY\" ]; then SZ=$(wc -c < \"$STORE/$KEY\"); echo \"{\\\"size\\\": $SZ}\"; exit 0; "
             "fi\n"
             "  exit 1\n"
             "fi\n"
             "if [ \"$1\" = \"mirror\" ]; then\n" // G5: mirror --exclude PATTERN SRC DST
             "  SRC=\"$4\"; DST=\"$5\"\n"
             "  KEY=\"${SRC#*/}\"; KEY=\"${KEY#*/}\"; KEY=\"${KEY%/}\"\n"
             "  SRCD=\"$STORE/$KEY\"; DSTD=\"${DST%/}\"\n"
             "  [ -d \"$SRCD\" ] || exit 1\n"
             "  ( cd \"$SRCD\" && find . -type f | while IFS= read -r rf; do\n"
             "      case \"$rf\" in ./_gn_chunk_markers/*) continue;; esac\n"
             "      mkdir -p \"$DSTD/$(dirname \"$rf\")\"; cp \"$SRCD/$rf\" \"$DSTD/$rf\"\n"
             "    done )\n"
             "  exit 0\n"
             "fi\n"
             "if [ \"$1\" = \"cp\" ]; then\n"
             "  SRC=\"$3\"; DST=\"$4\"\n" // cp --quiet REMOTE LOCAL
             "  KEY=\"${SRC#*/}\"; KEY=\"${KEY#*/}\"\n"
             "  [ -f \"$STORE/$KEY\" ] || exit 1\n"
             "  mkdir -p \"$(dirname \"$DST\")\"; cp \"$STORE/$KEY\" \"$DST\"; exit 0\n"
             "fi\n"
             "exit 1\n";
    }
    std::filesystem::permissions(fakemc, std::filesystem::perms::owner_all, ec);

    ::setenv("COMDARE_MINIO_ENDPOINT", "fakealias", 1);
    ::setenv("COMDARE_MINIO_BUCKET", "fakebucket", 1);
    ::setenv("COMDARE_MC_BIN", fakemc.string().c_str(), 1);
    ::unsetenv("COMDARE_MEASUREMENT_COMBO"); // Default-Key
    ::unsetenv("COMDARE_MINIO_PREFIX");
    ::unsetenv("COMDARE_MEASUREMENT_DROP_URL");

    at::ArtifactCache const cache = at::ArtifactCache::from_env();
    check_true("minio_enabled (fake)", cache.minio_enabled());

    std::string const bv      = "m3v2+cxx=g++-16+opt=O2+ext=avx2";
    std::string const kp      = cache.cache_key_prefix(bv); // Single-Source-Praefix (inkl. +ceb/+mtool/+mrg)
    std::string const algo_v1 = "algo=sortA,hashB";

    // ── (1) HIT: vollstaendiger Remote-Satz fuer stemA. ──
    {
        write_file(store / kp / "perm_cellA" / "perm.dll", "DLLBYTES-A");
        write_file(store / kp / "perm_cellA" / "perm.dll.algos", algo_v1);
        write_file(store / kp / "perm_cellA" / "perm.dll.version", bv);
        std::filesystem::path const bin_dir = base / "out" / "perm_cellA";
        bool const                  hit     = cache.pull_tier_binary(bin_dir, bv);
        check_true("(1) HIT: pull_tier_binary == true", hit);
        check_true("(1) HIT: lokale perm.dll da", std::filesystem::exists(bin_dir / "perm.dll", ec));
        check_true("(1) HIT: lokale perm.dll.algos da", std::filesystem::exists(bin_dir / "perm.dll.algos", ec));
        check_true("(1) HIT: lokale perm.dll.version da (Marke ZULETZT)",
                   std::filesystem::exists(bin_dir / "perm.dll.version", ec));
        // (4) Mismatch-.algos => Neubau: der Korrektheits-Arbiter dll_is_current entscheidet nach dem Pull.
        check_true("(4) dll_is_current HIT bei passender version+algos (skip)",
                   ex::dll_is_current(bin_dir / "perm.dll", bv, algo_v1));
        check_true("(4) dll_is_current FALSE bei gebumpter algos => Neubau",
                   !ex::dll_is_current(bin_dir / "perm.dll", bv, "algo=sortA,hashC"));
        check_true("(4) dll_is_current FALSE bei fremder version => Neubau",
                   !ex::dll_is_current(bin_dir / "perm.dll", "m3v2+cxx=g++-16+opt=O3+ext=avx2", algo_v1));
    }

    // ── (2) MISS: kein Remote-Objekt fuer stemMiss. ──
    {
        std::filesystem::path const bin_dir = base / "out" / "perm_cellMiss";
        bool const                  hit     = cache.pull_tier_binary(bin_dir, bv);
        check_true("(2) MISS: pull_tier_binary == false", !hit);
        check_true("(2) MISS: lokal NICHTS hydriert (kein perm.dll)",
                   !std::filesystem::exists(bin_dir / "perm.dll", ec));
    }

    // ── (3) HALB-PUSH: remote dll(+algos) aber KEINE .version => invertierte ZULETZT-Pruefung => MISS. ──
    {
        write_file(store / kp / "perm_cellHalf" / "perm.dll", "DLLBYTES-HALF");
        write_file(store / kp / "perm_cellHalf" / "perm.dll.algos", algo_v1);
        // BEWUSST keine perm.dll.version im Store.
        std::filesystem::path const bin_dir = base / "out" / "perm_cellHalf";
        bool const                  hit     = cache.pull_tier_binary(bin_dir, bv);
        check_true("(3) HALB-PUSH: pull_tier_binary == false (Marke fehlt remote)", !hit);
        check_true("(3) HALB-PUSH: lokal KEIN perm.dll (kein Pull ohne Marke)",
                   !std::filesystem::exists(bin_dir / "perm.dll", ec));
    }

    // ── (5) pull_tier_prefix (rekursiv): ganzer Praefix -> dest/<stem>/...; _gn_chunk_markers ausgespart. ──
    {
        write_file(store / kp / "perm_cellB" / "perm.dll", "DLLBYTES-B");
        write_file(store / kp / "perm_cellB" / "perm.dll.version", bv);
        write_file(store / kp / "_gn_chunk_markers" / "0-4.done", "part=1"); // MUSS ausgespart bleiben
        std::filesystem::path const dest = base / "hydrated";
        bool const                  ok   = cache.pull_tier_prefix(bv, dest);
        check_true("(5) pull_tier_prefix == true", ok);
        check_true("(5) stemA hydriert (dest/perm_cellA/perm.dll)",
                   std::filesystem::exists(dest / "perm_cellA" / "perm.dll", ec));
        check_true("(5) stemA .version hydriert",
                   std::filesystem::exists(dest / "perm_cellA" / "perm.dll.version", ec));
        check_true("(5) stemB hydriert (dest/perm_cellB/perm.dll)",
                   std::filesystem::exists(dest / "perm_cellB" / "perm.dll", ec));
        check_true("(5) _gn_chunk_markers AUSGESPART (nicht hydriert)",
                   !std::filesystem::exists(dest / "_gn_chunk_markers", ec));
    }

    // ── (6) pull_tier_prefix MISS: leerer Praefix (fremde build_version). ──
    {
        std::filesystem::path const dest = base / "hydrated_miss";
        bool const                  ok   = cache.pull_tier_prefix("m3v2+cxx=g++-16+opt=O3", dest);
        check_true("(6) pull_tier_prefix leerer Praefix == false", !ok);
    }

    // ── (7) NEGATIV: inert (kein minio) => kein Pull, KEIN mc-Prozess-Spawn. ──
    {
        std::filesystem::remove(spawnlog, ec); // Spawn-Zaehler zuruecksetzen
        ::unsetenv("COMDARE_MINIO_ENDPOINT");
        ::unsetenv("COMDARE_MINIO_BUCKET");
        at::ArtifactCache const inert = at::ArtifactCache::from_env();
        check_true("(7) inert() bei unkonfiguriertem Env", inert.inert());
        bool const h1 = inert.pull_tier_binary(base / "out" / "perm_cellA", bv);
        bool const h2 = inert.pull_tier_prefix(bv, base / "hydrated2");
        check_true("(7) inert pull_tier_binary == false", !h1);
        check_true("(7) inert pull_tier_prefix == false", !h2);
        check_true("(7) KEIN mc-Prozess gespawnt (kein Marker)", !std::filesystem::exists(spawnlog, ec));
    }

    ::unsetenv("COMDARE_MC_BIN");
    std::filesystem::remove_all(base, ec);
    std::cout << "\n==== S2 pull_tier_binary + pull_tier_prefix (#46a Pull): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
