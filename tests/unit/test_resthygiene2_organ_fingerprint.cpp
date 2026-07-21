// test_resthygiene2_organ_fingerprint -- Cache-Resthygiene-2 (2026-07-21): der Treiber-Fingerprint-Modus muss BYTE-GENAU
// dasselbe Pre-Image erzeugen wie der S1-F1-Marker-Write (`find <dll_dir> -name perm.dll.algos | LC_ALL=C sort |
// xargs cat`), sonst wuerde die Marker-Wache spurios MISSen. Diese Kreuz-Validierung legt synthetische perm.dll.algos-
// Dateien an und prueft: organ_fingerprint_preimage_from_pairs (die ce-sort+concat-Naht) == `find|LC_ALL=C sort|
// xargs cat` (via std::system, im Test erlaubt) -- inkl. Praefix-stem-Faellen (foo/foobar, fon/foo), plus sha256-
// Gleichheit. Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS.

#include "builder/experiment_tree/organ_fingerprint.hpp" // organ_fingerprint_preimage_from_pairs (leichte Naht)
#include "comdare_test_tmp.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;

static int  g_fail = 0;
static void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

static std::string read_file(std::filesystem::path const& p) {
    std::ifstream f{p, std::ios::binary};
    return std::string((std::istreambuf_iterator<char>(f)), {});
}

int main() {
    std::error_code             ec;
    std::filesystem::path const base = ::comdare::test::user_tmp_dir() / "comdare_resthyg2_fp";
    std::filesystem::remove_all(base, ec);
    std::filesystem::path const dll_dir = base / "dll";
    std::filesystem::create_directories(dll_dir, ec);

    // (stem, algo_sig): stems sind [A-Za-z0-9_] (wie orch_make_stem); Praefix-/Ordnungs-Faelle bewusst gemischt.
    // algo_sig-Inhalte mit ';', '@', '.', '=' (wie compose_algo_signature), OHNE trailing newline (== write_algos_sidecar).
    std::vector<std::pair<std::string, std::string>> const cases = {
        {"foo", "search=avl@1.0.0;alloc=jemalloc@2.3.1"},
        {"foobar", "search=btree@0.9.0;alloc=mimalloc@1.0.0"}, // Praefix-Erweiterung von 'foo'
        {"fon", "search=hopscotch@3.0.0"},                     // < 'foo' (n<o)
        {"bar", "x=1@0.0.1"},
        {"bar_1", "x=2@0.0.2"}, // '_' (0x5F) nach 'bar'
        {"Bar", "x=3@0.0.3"},   // Grossbuchstabe (0x42 < 'b' 0x62) -> vor 'bar'
        {"9zed", "x=4@0.0.4"},  // Ziffer (0x39 < Buchstabe)
        {"_lead", "x=5@0.0.5"}, // fuehrender '_' (0x5F)
        {"a", "x=6@0.0.6"},
        {"ab", "x=7@0.0.7"}, // Praefix-Erweiterung von 'a'
    };
    for (auto const& [stem, algo] : cases) {
        std::filesystem::create_directories(dll_dir / stem, ec);
        std::ofstream{dll_dir / stem / "perm.dll.algos", std::ios::binary} << algo; // KEIN newline
    }

    // ── ce-Seite: die sort+concat-Naht ──
    std::string const ce_preimage = ex::organ_fingerprint_preimage_from_pairs(cases);

    // ── Shell-Seite: EXAKT der S1-F1-Marker-Aufbau (find | LC_ALL=C sort | xargs cat) ──
    std::filesystem::path const shell_out = base / "shell_preimage.bin";
    std::string const           cmd       = "cd '" + dll_dir.string() +
                                            "' && find . -name 'perm.dll.algos' | LC_ALL=C sort | "
                                            "xargs cat > '" +
                                            shell_out.string() + "'";
    int const                   rc        = std::system(cmd.c_str());
    check_true("shell find|sort|cat exit 0", rc == 0);
    std::string const shell_preimage = read_file(shell_out);

    // ── Der Kern-Beweis: byte-identisch ──
    check_true("ce-Pre-Image == find|LC_ALL=C sort|xargs cat (BYTE-identisch)", ce_preimage == shell_preimage);
    if (ce_preimage != shell_preimage) {
        std::cout << "        ce   =' " << ce_preimage << "'\n        shell=' " << shell_preimage << "'\n";
    }

    // ── sha256-Gleichheit (== der Marker-algo_sig) ──
    std::filesystem::path const ce_file = base / "ce_preimage.bin";
    std::ofstream{ce_file, std::ios::binary} << ce_preimage;
    auto sha = [&](std::filesystem::path const& p) -> std::string {
        std::filesystem::path const o = p.string() + ".sha";
        std::system(("sha256sum < '" + p.string() + "' | awk '{print $1}' > '" + o.string() + "'").c_str());
        std::string s = read_file(o);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
        return s;
    };
    std::string const ce_sha    = sha(ce_file);
    std::string const shell_sha = sha(shell_out);
    check_true("sha256(ce) == sha256(shell) (== Marker-algo_sig)", !ce_sha.empty() && ce_sha == shell_sha);
    std::cout << "  [INFO] fingerprint sha256 = " << ce_sha << "\n";

    // ── Leerfall: keine .algos => leeres Pre-Image (find liefert nichts) ──
    check_true("leere Paar-Liste => leeres Pre-Image", ex::organ_fingerprint_preimage_from_pairs({}).empty());

    std::filesystem::remove_all(base, ec);
    std::cout << "\n==== Resthygiene-2 Organ-Fingerprint (find|sort|cat-Deckung): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
