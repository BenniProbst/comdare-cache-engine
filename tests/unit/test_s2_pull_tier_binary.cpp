// test_s2_pull_tier_binary -- S2 (#46a Pull-Faehigkeit): ArtifactCache::pull_tier_binary (per-Binary-Spiegel zu push)
// + pull_tier_prefix (BATCH-Warm-Cache-Hydrierung). Belegt (mock-basiert, KEIN echter mc/minio, via COMDARE_MC_BIN):
//   (1) HIT: vollstaendiger Remote-Satz (dll+algos+fingerprint+variant+version) -> pull_tier_binary=true, alle lokal.
//   (2) MISS: kein Remote-Objekt -> pull_tier_binary=false, lokal NICHTS hydriert.
//   (3) HALB-PUSH (remote dll[+algos] aber KEINE .version) -> invertierte ZULETZT-Pruefung => MISS => false, kein Pull.
//   (4) MISMATCH-.algos => Neubau: nach dem Pull entscheidet AUSSCHLIESSLICH lokal dll_is_current -- passende
//       version+algos => HIT (skip), gebumpte algos => false (Neubau). (Korrektheits-Arbiter, Dossier-Risiko 1.)
//   (5) pull_tier_prefix (rekursiv): hydriert den ganzen Praefix -> dest/<stem>/perm.dll + JEDES Sidecar daneben; der
//       _gn_chunk_markers-Namensraum wird ausgespart.
//   (6) NEGATIV: unkonfiguriertes Env => inert() => pull_tier_binary/pull_tier_prefix=false, KEIN mc-Prozess-Spawn.
//   (8) #13 (B25/L-d): die OPTIONALEN Sidecars .fingerprint/.variant reisen SPIEGELBILDLICH zum Push mit --
//       (8a) voller Satz remote => lokal hydriert, und die Varianten-Provenienz traegt sich (dll_is_current mit
//            gesetzter variant_sig sagt HIT statt Neubau);
//       (8b) remote nur teilweise vorhanden => fehlende Sidecars werden uebersprungen, Pull bleibt ERFOLGREICH;
//       (8c) ein STEHENGEBLIEBENES lokales Sidecar, das der Remote-Satz nicht deckt, ist nach dem Pull WEG (sonst
//            taeuschte es dll_is_current eine Provenienz vor, die der Objekt-Store nie geliefert hat).
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS (wie test_w11/test_s1).

#include "builder/artifact_transport/artifact_cache.hpp"
#include "builder/build_orchestrator/build_orchestrator.hpp" // ex::dll_is_current (Korrektheits-Arbiter nach dem Pull)
#include "comdare_test_tmp.hpp"                              // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator> // #13: std::istreambuf_iterator (read_file, Byte-Gleichheits-Beleg der Sidecars)
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

// #13: fuer den Byte-Gleichheits-Beleg der hydrierten Sidecars (leer, wenn die Datei fehlt).
static std::string read_file(std::filesystem::path const& p) {
    std::ifstream f{p, std::ios::binary};
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)), {});
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
    std::string const fpr_v1  = std::string(128, 'a'); // #13: 128-hex-Lager-Anker (key_from_hex-Laenge)
    std::string const var_v1  = "bv=1;pt=four_kb;se=avx2;hw=generic";

    // ---- (1) HIT: vollstaendiger Remote-Satz fuer stemA (inkl. der #13-Sidecars .fingerprint/.variant). ----
    {
        write_file(store / kp / "perm_cellA" / "perm.dll", "DLLBYTES-A");
        write_file(store / kp / "perm_cellA" / "perm.dll.algos", algo_v1);
        write_file(store / kp / "perm_cellA" / "perm.dll.fingerprint", fpr_v1);
        write_file(store / kp / "perm_cellA" / "perm.dll.variant", var_v1);
        write_file(store / kp / "perm_cellA" / "perm.dll.version", bv);
        std::filesystem::path const bin_dir = base / "out" / "perm_cellA";
        bool const                  hit     = cache.pull_tier_binary(bin_dir, bv);
        check_true("(1) HIT: pull_tier_binary == true", hit);
        check_true("(1) HIT: lokale perm.dll da", std::filesystem::exists(bin_dir / "perm.dll", ec));
        check_true("(1) HIT: lokale perm.dll.algos da", std::filesystem::exists(bin_dir / "perm.dll.algos", ec));
        check_true("(1) HIT: lokale perm.dll.version da (Marke ZULETZT)",
                   std::filesystem::exists(bin_dir / "perm.dll.version", ec));
        // (8a) #13: die beiden neuen Sidecars sind mitgereist -- ohne .fingerprint waere die hydrierte Binary fuer
        //      das Lager unsichtbar (bestand_key_of -> nullopt -> DedupOutcome::no_key).
        check_true("(8a) HIT: lokale perm.dll.fingerprint da (Lager-Anker hydriert)",
                   std::filesystem::exists(bin_dir / "perm.dll.fingerprint", ec));
        check_true("(8a) HIT: lokale perm.dll.variant da", std::filesystem::exists(bin_dir / "perm.dll.variant", ec));
        check_true("(8a) HIT: perm.dll.fingerprint byte-gleich zum Remote-Inhalt",
                   read_file(bin_dir / "perm.dll.fingerprint") == fpr_v1);
        // Die Varianten-Provenienz traegt sich: mit gesetzter variant_sig sagt der Arbiter HIT statt Neubau.
        check_true("(8a) dll_is_current HIT bei passender version+algos+variant (skip)",
                   ex::dll_is_current(bin_dir / "perm.dll", bv, algo_v1, var_v1));
        check_true("(8a) dll_is_current FALSE bei gewechselter variant => Neubau",
                   !ex::dll_is_current(bin_dir / "perm.dll", bv, algo_v1, "bv=1;pt=huge_2mb;se=avx2;hw=generic"));
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

    // ---- (8b) #13: remote nur TEILWEISE bestueckt (dll + .fingerprint + .version, KEIN .algos/.variant). Die fehlenden
    //      Sidecars werden uebersprungen -- der Pull bleibt ERFOLGREICH (Gate-aus ist der legitime Default). ----
    {
        write_file(store / kp / "perm_cellPart" / "perm.dll", "DLLBYTES-PART");
        write_file(store / kp / "perm_cellPart" / "perm.dll.fingerprint", fpr_v1);
        write_file(store / kp / "perm_cellPart" / "perm.dll.version", bv);
        std::filesystem::path const bin_dir = base / "out" / "perm_cellPart";
        bool const                  hit     = cache.pull_tier_binary(bin_dir, bv);
        check_true("(8b) TEIL-SATZ: pull_tier_binary == true (fehlende Sidecars sind kein Fehler)", hit);
        check_true("(8b) TEIL-SATZ: perm.dll.fingerprint hydriert",
                   std::filesystem::exists(bin_dir / "perm.dll.fingerprint", ec));
        check_true("(8b) TEIL-SATZ: KEIN lokales perm.dll.algos (remote nicht vorhanden)",
                   !std::filesystem::exists(bin_dir / "perm.dll.algos", ec));
        check_true("(8b) TEIL-SATZ: KEIN lokales perm.dll.variant (remote nicht vorhanden)",
                   !std::filesystem::exists(bin_dir / "perm.dll.variant", ec));
        check_true("(8b) TEIL-SATZ: perm.dll.version da (Marke ZULETZT gesetzt)",
                   std::filesystem::exists(bin_dir / "perm.dll.version", ec));
    }

    // ---- (8c) #13: ein STEHENGEBLIEBENES lokales Sidecar aus einem fruheren Bau, das der Remote-Satz NICHT deckt, muss
    //      nach dem Pull weg sein. Sonst faende dll_is_current eine Varianten-/Lager-Provenienz, die der Objekt-Store
    //      nie geliefert hat -- der lokale Satz MUSS nach dem Pull exakt der remote Satz sein. ----
    {
        std::filesystem::path const bin_dir = base / "out" / "perm_cellStale";
        write_file(store / kp / "perm_cellStale" / "perm.dll", "DLLBYTES-STALE");
        write_file(store / kp / "perm_cellStale" / "perm.dll.version", bv); // remote: NUR dll + Marke
        write_file(bin_dir / "perm.dll.variant", "bv=1;pt=STALE");          // lokaler Rest eines fruheren Baus
        write_file(bin_dir / "perm.dll.fingerprint", std::string(128, 'f'));
        write_file(bin_dir / "perm.dll.algos", "algo=STALE");
        bool const hit = cache.pull_tier_binary(bin_dir, bv);
        check_true("(8c) STALE: pull_tier_binary == true", hit);
        check_true("(8c) STALE: altes lokales perm.dll.variant ENTFERNT",
                   !std::filesystem::exists(bin_dir / "perm.dll.variant", ec));
        check_true("(8c) STALE: altes lokales perm.dll.fingerprint ENTFERNT",
                   !std::filesystem::exists(bin_dir / "perm.dll.fingerprint", ec));
        check_true("(8c) STALE: altes lokales perm.dll.algos ENTFERNT",
                   !std::filesystem::exists(bin_dir / "perm.dll.algos", ec));
        check_true("(8c) STALE: dll_is_current FALSE bei gesetzter variant_sig (kein Provenienz-Vortaeuschen)",
                   !ex::dll_is_current(bin_dir / "perm.dll", bv, std::string{}, "bv=1;pt=STALE"));
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
        // #13: pull_tier_prefix ist SUFFIX-BLIND (mc mirror ueber den Praefix) -> die neuen Sidecars reisen ohne
        // Code-Aenderung mit. Das ist der Weg, den der Voll-Bau real nimmt (BATCH-Hydrierung, EIN mc-Prozess).
        check_true("(5) stemA .fingerprint hydriert (mirror ist suffix-blind)",
                   std::filesystem::exists(dest / "perm_cellA" / "perm.dll.fingerprint", ec));
        check_true("(5) stemA .variant hydriert",
                   std::filesystem::exists(dest / "perm_cellA" / "perm.dll.variant", ec));
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
