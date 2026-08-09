// test_a2_sha512_skip_gate -- A2-SHA512-ONLY-SKIP-GATE (F7, KONSOLIDIERT:72), GEEICHT am 2026-08-05 auf
// Basis 24e07219 (GATE 5, der EINE Anker-Vollzug). Diese TU ist der Beweis, dass dll_is_current seither
// GENAU EINEN Vergleich fuehrt: erwarteter CT-Fingerprint == Inhalt des `.fingerprint`-Sidecars.
//
//   (a) FAIL-CLOSED ohne Sidecar   -- perm.dll + vollstaendig passendes `.version` (die ALTE Skip-Bedingung),
//                                     aber KEIN `.fingerprint` => NICHT aktuell => Neubau. (Uebergangsregel F7:
//                                     "kein .fingerprint => nicht aktuell => Neubau", KEIN Grandfathering.)
//   (b) SKIP bei Match             -- `.fingerprint` == expected => Skip; inkl. Trim-Toleranz (AUF-A3): eine
//                                     Datei mit abschliessendem '\n' oder CRLF gilt weiter als gleich.
//   (c) NEUBAU bei Mismatch        -- anderer Wert => Neubau; plus die vier Formwachen (leer / 127 / 129 /
//                                     nicht-hex) und die beiden Struktur-Wachen (expected leer, DLL fehlt).
//   (d) F7-"NUR" (DER Biss)        -- `.version` passt, `.algos`/`.variant` sind gefuellt, nur `.fingerprint`
//                                     weicht ab => FALSE. Die alte Dreifach-String-Gleichheit entscheidet NICHT
//                                     mehr mit; sie ist ERSETZT, nicht ergaenzt.
//   (e) HYDRATIONS-SKIP            -- eine per pull_tier_binary hydrierte Binary skippt, weil
//                                     `perm.dll.fingerprint` in kOptionalTierSidecars steht (artifact_cache.hpp:92)
//                                     und damit in BEIDE Richtungen mitreist. Gegenprobe: Remote-Satz ohne
//                                     `.fingerprint` => nach der Hydration KEIN Skip (Alt-Bestand => Neubau).
//   (f) ORCHESTRATOR-VERDRAHTUNG   -- am echten Call-Site (provision_core): der Skip haengt am Fingerprint-
//                                     Provider, NICHT mehr an cfg.build_version. Vier Laeufe, darunter der
//                                     fail-closed-Lauf OHNE Provider bei gesetzter build_version.
//   (g) F3/C5 STALE-RAEUMUNG       -- BEIDSEITIG am echten Call-Site: ein Neubau, dessen neuer Sidecar-Wert
//                                     LEER ist, laesst die Marke des Vorgaenger-Baus NICHT liegen (Raeum-Weg);
//                                     ein Neubau mit gefuelltem Wert schreibt sie unveraendert (Schreib-Weg,
//                                     kein Kollateralschaden). Ohne den Raeum-Weg ueberlebt ein 128-hex aus
//                                     einem fremden Lauf und kann einen spaeteren Lauf falsch skippen lassen.
//
// EICH-DISZIPLIN (D7): diese TU verwendet SYNTHETISCHE 128-hex-Vektoren. Der geeichte Referenz-Vektor
// kFrozenFingerprintV1 bleibt bewusst auf seine drei bestehenden Fundstellen gepinnt (test_g3_sha512_index.cpp,
// test_w10_system_cell_values.cpp, test_m_w12_stamp_bausteine.cpp) -- das Gate selbst ist wert-agnostisch.
// [NACHGEFUEHRT 2026-08-05, O-2/C-2: die drei Fundstellen sind mit dem Preimage-Format-Bump 2 -> 3 NEU
// eingefroren worden, alle drei im SELBEN Commit. Diese TU war davon nicht betroffen und soll es nicht
// sein -- genau dafuer ist der Pin da: das Skip-Gate prueft den VERGLEICH, nie einen Digest-WERT.]
// [NACHGEFUEHRT 2026-08-06, NB/CX-4: die drei Fundstellen sind ein ZWEITES und LETZTES Mal neu eingefroren
// worden -- diesmal in der END-FORM (die Preimage-Glieder [5]/[6] sind im Vektor BELEGT statt leer), weil
// die Live-Naht sie produktiv befuellt. Wieder alle drei im SELBEN Commit, wieder OHNE diese TU: der Pin
// haelt unveraendert, das Gate bleibt wert-agnostisch.]
//
// L14: geeicht wurde MIT LEEREM Overlay-Glied (deklarierte, nicht stille Luecke; Heilung im Overlay-Fenster
// Phase 6; seit O-2/C-2 ist es das ACHTE und letzte Glied). Das beruehrt diese TU nicht: sie prueft den
// VERGLEICH, nicht den Preimage-Inhalt.

#include "builder/artifact_transport/artifact_cache.hpp"      // (e): pull_tier_binary + kOptionalTierSidecars
#include "builder/build_orchestrator/build_orchestrator.hpp"  // dll_is_current / write_*_sidecar / BuildOrchestrator
#include "builder/build_orchestrator/fingerprint_sidecar.hpp" // fingerprint_sidecar_path / read_fingerprint_sidecar
#include "builder/experiment_tree/experiment_tree.hpp"        // (f): StaticBinaryView fuer den echten Call-Site
#include "comdare_test_tmp.hpp"                               // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include <gtest/gtest.h>

namespace at = comdare::cache_engine::builder::artifact_transport;
namespace ex = comdare::cache_engine::builder::experiment;
namespace fs = std::filesystem;

namespace {

void write_file(fs::path const& p, std::string const& content) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f{p, std::ios::binary | std::ios::trunc};
    f << content;
}

// Roh-Inhalt einer Datei (fuer die Byte-Gleichheits-Proben der Sidecar-Writer).
std::string read_file(fs::path const& p) {
    std::ifstream     f{p, std::ios::binary};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Ein frisches, leeres Arbeitsverzeichnis je Abschnitt (kein Uebersprechen zwischen den Faellen).
fs::path fresh_dir(char const* name) {
    std::error_code ec;
    fs::path const  d = ::comdare::test::user_tmp_dir() / "comdare_a2_skip_gate" / name;
    fs::remove_all(d, ec);
    fs::create_directories(d, ec);
    return d;
}

// Synthetische, deterministische 128-hex-Vektoren (D7). `gen` trennt die "Fingerprint-Welten" der
// Orchestrator-Laeufe, `id` macht den Wert je Binary verschieden -- genau wie der echte K7b-Fingerprint.
std::string synth_fp(std::string const& gen, std::string const& id) {
    std::uint64_t h = 0xcbf29ce484222325ULL; // FNV-1a, wie orch_fnv1a_hex -- reine Testarithmetik
    for (char const c : gen) {
        h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        h *= 0x100000001b3ULL;
    }
    for (char const c : id) {
        h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        h *= 0x100000001b3ULL;
    }
    static constexpr char hexd[] = "0123456789abcdef";
    std::string           block(16, '0');
    for (int i = 15; i >= 0; --i) {
        block[static_cast<std::size_t>(i)] = hexd[h & 0xF];
        h >>= 4;
    }
    std::string out;
    out.reserve(128);
    for (int k = 0; k < 8; ++k) out += block; // 8 x 16 = 128 Hex-Zeichen
    return out;
}

std::string const kFpA = std::string(128, 'a'); // "Welt A"
std::string const kFpB = std::string(128, 'b'); // "Welt B"

} // namespace

// -- (a) FAIL-CLOSED ohne Sidecar ---------------------------------------------------------------------
// Der Kern der Uebergangsregel: ein Bestand, der nach der ALTEN Regel vollstaendig aktuell waere
// (perm.dll da, .version byte-gleich der Erwartung), ist ohne `.fingerprint` NICHT aktuell.
TEST(A2SkipGate, FailClosedOhneSidecar) {
    fs::path const d   = fresh_dir("a_failclosed");
    fs::path const dll = d / "perm.dll";
    write_file(dll, "DLL");
    write_file(ex::version_sidecar_path(dll), kFpA); // die ALTE Skip-Bedingung waere voll erfuellt

    ASSERT_FALSE(fs::exists(ex::fingerprint_sidecar_path(dll)));
    EXPECT_FALSE(ex::dll_is_current(dll, kFpA)); // kein Anker => Neubau
    EXPECT_FALSE(ex::read_fingerprint_sidecar(dll).has_value());
}

// -- (b) SKIP bei Match (inkl. Trim-Welt) -------------------------------------------------------------
TEST(A2SkipGate, SkipBeiMatchInklusiveTrim) {
    fs::path const d   = fresh_dir("b_match");
    fs::path const dll = d / "perm.dll";
    write_file(dll, "DLL");

    write_file(ex::fingerprint_sidecar_path(dll), kFpA);
    EXPECT_TRUE(ex::dll_is_current(dll, kFpA));

    // AUF-A3: der Schreiber setzt keinen Newline, ein fremd erzeugtes Sidecar traegt ihn oft doch.
    // Skip-Gate und Lager-Binder trimmen ueber DIESELBE Funktion -- beide sehen denselben Wert.
    write_file(ex::fingerprint_sidecar_path(dll), kFpA + "\n");
    EXPECT_TRUE(ex::dll_is_current(dll, kFpA));
    EXPECT_EQ(ex::read_fingerprint_sidecar(dll).value_or(std::string{}), kFpA);

    write_file(ex::fingerprint_sidecar_path(dll), "  " + kFpA + "\r\n");
    EXPECT_TRUE(ex::dll_is_current(dll, kFpA));
}

// -- (c) NEUBAU bei Mismatch + Form-/Struktur-Wachen --------------------------------------------------
TEST(A2SkipGate, NeubauBeiMismatchUndFormverstoss) {
    fs::path const d   = fresh_dir("c_mismatch");
    fs::path const dll = d / "perm.dll";
    write_file(dll, "DLL");

    write_file(ex::fingerprint_sidecar_path(dll), kFpB);
    EXPECT_FALSE(ex::dll_is_current(dll, kFpA)); // anderer Fingerprint => Neubau

    write_file(ex::fingerprint_sidecar_path(dll), "");
    EXPECT_FALSE(ex::dll_is_current(dll, kFpA)); // sidecar_leer

    write_file(ex::fingerprint_sidecar_path(dll), std::string(127, 'a'));
    EXPECT_FALSE(ex::dll_is_current(dll, std::string(127, 'a'))); // laenge_verstoss (auch bei "Gleichheit")

    write_file(ex::fingerprint_sidecar_path(dll), std::string(129, 'a'));
    EXPECT_FALSE(ex::dll_is_current(dll, std::string(129, 'a'))); // laenge_verstoss

    write_file(ex::fingerprint_sidecar_path(dll), std::string(128, 'z'));
    EXPECT_FALSE(ex::dll_is_current(dll, std::string(128, 'z'))); // zeichen_verstoss

    // expected leer: OHNE CT-Erwartung wird nie uebersprungen -- auch bei tadellosem Sidecar.
    write_file(ex::fingerprint_sidecar_path(dll), kFpA);
    EXPECT_FALSE(ex::dll_is_current(dll, std::string{}));

    // Ein Sidecar ohne Binary skippt NIE (Phantom-Schutz).
    fs::path const einsam = fresh_dir("c_phantom") / "perm.dll";
    write_file(ex::fingerprint_sidecar_path(einsam), kFpA);
    ASSERT_FALSE(fs::exists(einsam));
    EXPECT_FALSE(ex::dll_is_current(einsam, kFpA));
}

// -- (d) F7-"NUR": die alte .version/.algos/.variant-Logik entscheidet NICHT mehr (DER Biss) ----------
TEST(A2SkipGate, F7NurDerFingerprintEntscheidet) {
    fs::path const d   = fresh_dir("d_f7nur");
    fs::path const dll = d / "perm.dll";
    write_file(dll, "DLL");
    // Der komplette Alt-Satz, und zwar in seiner GUENSTIGSTEN Belegung fuer einen Skip:
    write_file(ex::version_sidecar_path(dll), kFpA);
    write_file(ex::algo_sidecar_path(dll), "algo=sortA,hashB");
    write_file(ex::variant_sidecar_path(dll), "bv=1;pt=four_kb;se=avx2;hw=generic");
    // ... nur der Anker weicht ab.
    write_file(ex::fingerprint_sidecar_path(dll), kFpB);

    EXPECT_FALSE(ex::dll_is_current(dll, kFpA)); // .version-Gleichheit rettet nichts mehr

    // Und die Gegenrichtung: ohne JEDES Alt-Sidecar, aber mit passendem Anker => Skip.
    fs::path const d2   = fresh_dir("d_nur_anker");
    fs::path const dll2 = d2 / "perm.dll";
    write_file(dll2, "DLL");
    write_file(ex::fingerprint_sidecar_path(dll2), kFpA);
    ASSERT_FALSE(fs::exists(ex::version_sidecar_path(dll2)));
    ASSERT_FALSE(fs::exists(ex::algo_sidecar_path(dll2)));
    ASSERT_FALSE(fs::exists(ex::variant_sidecar_path(dll2)));
    EXPECT_TRUE(ex::dll_is_current(dll2, kFpA)); // der EINE Vergleich genuegt
}

// -- (e) HYDRATIONS-SKIP: eine gepullte Binary skippt (R6 entschaerft) --------------------------------
// Ohne diesen Beweis waere das Gate zu scharf: hydrierte Binaries wuerden nie skippen und das Lager
// waere wertlos. `perm.dll.fingerprint` steht seit #13 in kOptionalTierSidecars (artifact_cache.hpp:92)
// und reist in Push UND Pull mit -- der Fake-Store hier belegt die Pull-Richtung end-to-end.
TEST(A2SkipGate, HydrationsSkipUeberPullTierBinary) {
    std::error_code ec;
    fs::path const  base     = fresh_dir("e_hydration");
    fs::path const  store    = base / "store";
    fs::path const  fakemc   = base / "fake_mc_pull.sh";
    fs::path const  spawnlog = base / "mc_spawned.marker";
    {
        // Minimaler Pull-Fake-mc (Muster test_s2_pull_tier_binary): stat = Existenz, cp = store -> lokal.
        std::ofstream f{fakemc};
        f << "#!/bin/sh\n"
             "STORE=\""
          << store.string() << "\"\n"
          << "echo x >> \"" << spawnlog.string() << "\"\n"
          << "if [ \"$1\" = \"stat\" ]; then\n"
             "  T=\"$3\"; KEY=\"${T#*/}\"; KEY=\"${KEY#*/}\"\n"
             "  if [ -f \"$STORE/$KEY\" ]; then SZ=$(wc -c < \"$STORE/$KEY\"); echo \"{\\\"size\\\": $SZ}\"; exit 0; "
             "fi\n"
             "  exit 1\n"
             "fi\n"
             "if [ \"$1\" = \"cp\" ]; then\n"
             "  SRC=\"$3\"; DST=\"$4\"\n"
             "  KEY=\"${SRC#*/}\"; KEY=\"${KEY#*/}\"\n"
             "  [ -f \"$STORE/$KEY\" ] || exit 1\n"
             "  mkdir -p \"$(dirname \"$DST\")\"; cp \"$STORE/$KEY\" \"$DST\"; exit 0\n"
             "fi\n"
             "exit 1\n";
    }
    fs::permissions(fakemc, fs::perms::owner_all, ec);

    ::setenv("COMDARE_MINIO_ENDPOINT", "fakealias", 1);
    ::setenv("COMDARE_MINIO_BUCKET", "fakebucket", 1);
    ::setenv("COMDARE_MC_BIN", fakemc.string().c_str(), 1);
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    ::unsetenv("COMDARE_MINIO_PREFIX");
    ::unsetenv("COMDARE_MEASUREMENT_DROP_URL");

    at::ArtifactCache const cache = at::ArtifactCache::from_env();
    ASSERT_TRUE(cache.minio_enabled());

    std::string const bv = "m3v2+cxx=g++-16+opt=O2+ext=avx2";
    std::string const kp = cache.cache_key_prefix(bv);

    // HIT: voller Remote-Satz INKLUSIVE des Ankers.
    {
        write_file(store / kp / "perm_cellFp" / "perm.dll", "DLLBYTES");
        write_file(store / kp / "perm_cellFp" / "perm.dll.fingerprint", kFpA);
        write_file(store / kp / "perm_cellFp" / "perm.dll.version", bv);
        fs::path const bin_dir = base / "out" / "perm_cellFp";
        ASSERT_TRUE(cache.pull_tier_binary(bin_dir, bv));
        ASSERT_TRUE(fs::exists(bin_dir / "perm.dll.fingerprint", ec));
        EXPECT_TRUE(ex::dll_is_current(bin_dir / "perm.dll", kFpA));  // gepullte Binary skippt
        EXPECT_FALSE(ex::dll_is_current(bin_dir / "perm.dll", kFpB)); // fremde Welt => Neubau
    }
    // GEGENPROBE: Alt-Bestand ohne Anker (nur dll + Transport-Marke) => nach der Hydration KEIN Skip.
    {
        write_file(store / kp / "perm_cellAlt" / "perm.dll", "DLLBYTES");
        write_file(store / kp / "perm_cellAlt" / "perm.dll.version", bv);
        fs::path const bin_dir = base / "out" / "perm_cellAlt";
        ASSERT_TRUE(cache.pull_tier_binary(bin_dir, bv));
        ASSERT_FALSE(fs::exists(bin_dir / "perm.dll.fingerprint", ec));
        EXPECT_FALSE(ex::dll_is_current(bin_dir / "perm.dll", kFpA)); // fail-closed, kein Grandfathering
    }
}

// -- (f) ORCHESTRATOR-VERDRAHTUNG am echten Call-Site (provision_core) --------------------------------
// Vier Laeufe ueber DASSELBE Ausgabe-Verzeichnis. cfg.build_version ist in allen vier Laeufen GESETZT und
// KONSTANT -- sie darf den Skip weder herbeifuehren noch verhindern. Was den Skip traegt, ist allein der
// Fingerprint-Provider; Lauf 4 zeigt die fail-closed-Folge seines Fehlens.
TEST(A2SkipGate, OrchestratorSkipHaengtAmFingerprintProvider) {
    auto           factory = std::make_shared<ex::ExperimentNodeFactory>();
    fs::path const base    = fresh_dir("f_orchestrator");

    ex::ExperimentTree tree{factory};
    tree.build({ex::AxisLevel{"traversal", {"ART"}, true, "", ""},
                ex::AxisLevel{"node", {"v0", "v1", "v2", "v3", "v4"}, true, "", ""}});
    auto const        view = tree.static_binary_view();
    std::size_t const n    = view.size();
    ASSERT_EQ(n, std::size_t{5});

    auto compile_touch = [](ex::BuildJob const& j) -> int {
        std::ofstream f{j.output, std::ios::binary | std::ios::trunc};
        f << "DLL";
        return 0;
    };
    auto gen = [](std::string const&) { return std::string{"//\n"}; };

    ex::BuildConfig cfg{4, 8, base / "src", base / "dll"};
    cfg.build_version = "m3v2+cxx=g++-16+opt=O2+ext=avx2"; // KONSTANT ueber alle vier Laeufe

    // Lauf 1 -- Erstbau in Welt A.
    {
        ex::BuildOrchestrator orch{cfg, compile_touch, gen};
        orch.set_fingerprint_provider([](std::string const& id) { return synth_fp("A", id); });
        ex::BuildStats st;
        orch.provision_all(view, &st);
        EXPECT_EQ(st.built, n);
        EXPECT_EQ(st.skipped, std::size_t{0});
    }
    // Lauf 2 -- gleiche Welt, gleiches Out-Dir: der Anker liegt daneben => ALLE skippen.
    {
        ex::BuildOrchestrator orch{cfg, compile_touch, gen};
        orch.set_fingerprint_provider([](std::string const& id) { return synth_fp("A", id); });
        ex::BuildStats st;
        orch.provision_all(view, &st);
        EXPECT_EQ(st.skipped, n);
        EXPECT_EQ(st.built, std::size_t{0});
    }
    // Lauf 3 -- Welt B bei UNVERAENDERTER build_version: der Fingerprint entscheidet => ALLE neu.
    {
        ex::BuildOrchestrator orch{cfg, compile_touch, gen};
        orch.set_fingerprint_provider([](std::string const& id) { return synth_fp("B", id); });
        ex::BuildStats st;
        orch.provision_all(view, &st);
        EXPECT_EQ(st.built, n);
        EXPECT_EQ(st.skipped, std::size_t{0});
    }
    // Lauf 4 -- OHNE Provider bei gesetzter build_version und vorhandenen Sidecars: fail-closed.
    // Das ist die deklarierte Uebergangs-Folge (F7): ohne CT-Erwartung wird ehrlich neu gebaut,
    // statt sich auf eine Marke zu verlassen, die ueber die Bau-Identitaet nichts aussagt.
    {
        ex::BuildOrchestrator orch{cfg, compile_touch, gen}; // KEIN set_fingerprint_provider
        ex::BuildStats        st;
        orch.provision_all(view, &st);
        EXPECT_EQ(st.skipped, std::size_t{0});
        EXPECT_EQ(st.built, n);
    }
}

// -- (g) F3/C5 STALE-SIDECAR-RAEUMUNG am echten Call-Site (provision_core) ----------------------------
// BEIDSEITIG, weil beide Richtungen falsch sein koennen und nur zusammen die Aussage tragen:
//   RAEUM-WEG    -- der neue Wert ist LEER (kein Provider / keine AlgoSigFn / keine Variant-Signatur), der
//                   Writer waere ein no-op: die Marke des VORGAENGER-Baus MUSS verschwinden. Genau hier lag
//                   C5: ein `.fingerprint` aus einem fremden Lauf ueberlebte den Neubau und konnte einen
//                   spaeteren Lauf mit passender Erwartung eine Binary ueberspringen lassen, die er nie
//                   gebaut hat.
//   SCHREIB-WEG  -- der neue Wert ist GEFUELLT: der Writer oeffnet mit trunc und ueberschreibt; geraeumt
//                   werden darf hier NICHTS. Ohne diese zweite Haelfte waere ein "immer loeschen" ebenfalls
//                   gruen -- und wuerde die Provenienz-Marken der Flotte stillschweigend entsorgen.
TEST(A2SkipGate, NeubauRaeumtStaleSidecarsBeidseitig) {
    auto           factory = std::make_shared<ex::ExperimentNodeFactory>();
    fs::path const base    = fresh_dir("g_stale_prune");

    ex::ExperimentTree tree{factory};
    tree.build({ex::AxisLevel{"traversal", {"ART"}, true, "", ""}, ex::AxisLevel{"node", {"v0"}, true, "", ""}});
    auto const view = tree.static_binary_view();
    ASSERT_EQ(view.size(), std::size_t{1});

    auto compile_touch = [](ex::BuildJob const& j) -> int {
        std::ofstream f{j.output, std::ios::binary | std::ios::trunc};
        f << "DLL";
        return 0;
    };
    auto gen = [](std::string const&) { return std::string{"//\n"}; };

    // Den Ausgabe-Pfad NICHT nachbauen (das waere eine zweite Wahrheit ueber die Job-Benennung), sondern
    // nach dem ersten Lauf am Verzeichnis ablesen: genau eine `.fingerprint`-Datei liegt dort.
    auto sidecar_of = [](fs::path const& dir, char const* suffix) {
        fs::path        gefunden;
        std::error_code ec;
        for (auto const& e : fs::recursive_directory_iterator{dir, ec}) {
            if (e.path().extension().string() == suffix) gefunden = e.path();
        }
        return gefunden;
    };

    ex::BuildConfig cfg{1, 8, base / "src", base / "dll"};
    cfg.build_version = "m3v2+cxx=g++-16+opt=O2"; // KONSTANT: die `.version` darf nie geraeumt werden

    // Lauf 1 -- Erstbau MIT Provider: der Anker entsteht.
    {
        ex::BuildOrchestrator orch{cfg, compile_touch, gen};
        orch.set_fingerprint_provider([](std::string const&) { return kFpA; });
        ex::BuildStats st;
        orch.provision_all(view, &st);
        ASSERT_EQ(st.built, std::size_t{1});
    }
    fs::path const fp_pfad = sidecar_of(base / "dll", ".fingerprint");
    ASSERT_FALSE(fp_pfad.empty()) << "Lauf 1 muss ein .fingerprint-Sidecar hinterlassen";
    ASSERT_TRUE(fs::exists(fp_pfad));
    fs::path const dll_pfad = fs::path{fp_pfad}.replace_extension();
    ASSERT_TRUE(fs::exists(dll_pfad)) << "der Sidecar-Stamm muss die gebaute Binary sein";
    fs::path const version_pfad = ex::version_sidecar_path(dll_pfad);
    ASSERT_TRUE(fs::exists(version_pfad));

    // Zwei weitere Alt-Marken pflanzen, die dieser Aufbau sonst nie erzeugt (keine AlgoSigFn, leere
    // Variant-Signatur) -- sie stehen fuer den Bestand eines FRUEHEREN, anders konfigurierten Laufs.
    write_file(ex::algo_sidecar_path(dll_pfad), "algo_sig=ALT");
    write_file(ex::variant_sidecar_path(dll_pfad), "bv=1;page_kind=0");

    // Lauf 2 (RAEUM-WEG) -- OHNE Provider: expected leer => Neubau (fail-closed). Die drei Marken mit
    // leerem Neu-Wert muessen weg sein, die `.version` (gefuellt) bleibt.
    {
        ex::BuildOrchestrator orch{cfg, compile_touch, gen}; // KEIN Provider, KEINE AlgoSigFn
        ex::BuildStats        st;
        orch.provision_all(view, &st);
        ASSERT_EQ(st.built, std::size_t{1});
    }
    EXPECT_FALSE(fs::exists(fp_pfad)) << "C5: der 128-hex des Vorgaenger-Baus haette einen spaeteren Lauf "
                                         "falsch skippen lassen koennen";
    EXPECT_FALSE(fs::exists(ex::algo_sidecar_path(dll_pfad)));
    EXPECT_FALSE(fs::exists(ex::variant_sidecar_path(dll_pfad)));
    EXPECT_TRUE(fs::exists(version_pfad)) << "die gefuellte `.version` wird ueberschrieben, nie geraeumt";
    EXPECT_EQ(read_file(version_pfad), cfg.build_version);

    // Lauf 3 (SCHREIB-WEG) -- MIT Provider und gefuellter Variant-Signatur: beide Marken entstehen neu,
    // die `.version` bleibt unveraendert. Kein Wert darf hier verschwinden.
    ex::BuildConfig cfg3   = cfg;
    cfg3.build_variant_sig = "bvset=1;bv=1;page_type[{bplus;page_kind=0}]";
    {
        ex::BuildOrchestrator orch{cfg3, compile_touch, gen};
        orch.set_fingerprint_provider([](std::string const&) { return kFpB; });
        ex::BuildStats st;
        orch.provision_all(view, &st);
        ASSERT_EQ(st.built, std::size_t{1});
    }
    ASSERT_TRUE(fs::exists(fp_pfad));
    EXPECT_EQ(read_file(fp_pfad), kFpB) << "der Schreib-Weg setzt den neuen Anker (trunc, kein Anhaengen)";
    ASSERT_TRUE(fs::exists(ex::variant_sidecar_path(dll_pfad)));
    EXPECT_EQ(read_file(ex::variant_sidecar_path(dll_pfad)), cfg3.build_variant_sig);
    EXPECT_TRUE(fs::exists(version_pfad));
    EXPECT_EQ(read_file(version_pfad), cfg.build_version);
    // `.algos` bleibt abwesend: keine AlgoSigFn injiziert => leerer Wert => nichts zu schreiben, nichts
    // zu konservieren. Die Raeumung eines NICHT vorhandenen Sidecars ist ein no-op (kein Fehler).
    EXPECT_FALSE(fs::exists(ex::algo_sidecar_path(dll_pfad)));

    // Lauf 4 -- Gegenprobe zur Wirkung: derselbe Provider wie Lauf 3 => der frische Anker traegt, es wird
    // uebersprungen (die Raeumung hat den Skip-Weg nicht beschaedigt).
    {
        ex::BuildOrchestrator orch{cfg3, compile_touch, gen};
        orch.set_fingerprint_provider([](std::string const&) { return kFpB; });
        ex::BuildStats st;
        orch.provision_all(view, &st);
        EXPECT_EQ(st.skipped, std::size_t{1});
        EXPECT_EQ(st.built, std::size_t{0});
    }
}
