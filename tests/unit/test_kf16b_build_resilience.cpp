// test_kf16b_build_resilience — KF-16b (2026-06-02)
// Belegt: (A) Default cores_per_build=4 + KEINE Oversubscription (parallel_jobs × cores ≤ Kerne, gekappt);
// (B) RAM-Admission (freier RAM gemessen, weiterer Build nur wenn genug RAM, mind. 1 läuft immer); (C)
// inkrementell-resumierbar (aktuelle DLLs uebersprungen). Stub-CompileFn/FreeRamFn -> deterministisch.
// A2-EICHUNG (GATE 5, F7, 2026-08-05): das Aktualitaets-KRITERIUM in (C) ist seither der `.fingerprint`-Anker
// statt der build_version im `.version`-Sidecar. [HISTORIK bis 2026-08-05: "versions-aktuelle DLLs".]
// Die Resilienz-Aussage selbst ist unveraendert; sie wird nur ueber den injizierten FingerprintFn gefahren.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf16b_build_resilience.cpp

#include "builder/build_orchestrator/build_orchestrator.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include "builder/experiment_tree/experiment_tree.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

// Baut eine StaticBinaryView mit n statischen Binaries (traversal=ART × node={v0..v{n-1}}).
static ex::ExperimentTree make_tree(std::shared_ptr<ex::ExperimentNodeFactory> const& f, std::size_t n) {
    std::vector<std::string> nodes;
    for (std::size_t i = 0; i < n; ++i) nodes.push_back("v" + std::to_string(i));
    ex::ExperimentTree t{f};
    t.build({ex::AxisLevel{"traversal", {"ART"}, true, "", ""}, ex::AxisLevel{"node", nodes, true, "", ""}});
    return t;
}

int main() {
    std::cout << "KF-16b: BuildOrchestrator-Härtung (4 Threads, RAM-Admission, inkrementell):\n";
    constexpr std::uint64_t GB = 1024ull * 1024 * 1024;

    // ── (A) Default 4 Threads + KEINE Oversubscription ──
    check_eq("Default cores_per_build == 4", (ex::BuildConfig{}).cores_per_build, std::size_t{4});
    check_eq("parallel_jobs(4 je Build, 16 Kerne)", (ex::BuildConfig{4, 16, {}, {}}).parallel_jobs(),
             std::size_t{4}); // 4×4=16
    check_eq("parallel_jobs(4 je Build, 8 Kerne)", (ex::BuildConfig{4, 8, {}, {}}).parallel_jobs(),
             std::size_t{2}); // 2×4=8
    check_eq("parallel_jobs(4 je Build, 6 Kerne)", (ex::BuildConfig{4, 6, {}, {}}).parallel_jobs(),
             std::size_t{1}); // 1×4=4≤6
    // Oversubscription-Schutz: cores_per_build > Kerne → gekappt auf Kerne
    check_eq("effective_cores_per_build(4, 2 Kerne) gekappt",
             (ex::BuildConfig{4, 2, {}, {}}).effective_cores_per_build(), std::size_t{2});
    check_eq("parallel_jobs(4, 2 Kerne) == 1 (1×2=2≤2, keine Oversubscription)",
             (ex::BuildConfig{4, 2, {}, {}}).parallel_jobs(), std::size_t{1});

    auto                        factory = std::make_shared<ex::ExperimentNodeFactory>();
    std::filesystem::path const base    = ::comdare::test::user_tmp_dir() / "comdare_kf16b";
    std::error_code             ec;
    std::filesystem::remove_all(base, ec);

    // ── (B) RAM-Admission: 10 GB frei, 4 GB/Build → max 2 gleichzeitig (RAM-gebunden, nicht CPU) ──
    {
        ex::ExperimentTree tree = make_tree(factory, 8);
        ex::BuildConfig    cfg{1, 16, base / "ram_src", base / "ram_dll"}; // CPU erlaubt 16 parallel
        cfg.ram_per_build_bytes = 4 * GB;
        std::atomic<int> inflight{0};
        auto             compile_ram = [&](ex::BuildJob const&) -> int {
            ++inflight;
            std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Build hält RAM
            --inflight;
            return 0;
        };
        auto free_stub = [&]() -> std::uint64_t {
            std::uint64_t used = static_cast<std::uint64_t>(inflight.load()) * 4 * GB;
            return (used >= 10 * GB) ? 0 : (10 * GB - used);
        };
        ex::BuildOrchestrator orch{cfg, compile_ram, [](std::string const&) { return std::string{"//\n"}; }, free_stub};
        ex::BuildStats        st;
        auto                  res = orch.provision_all(tree.static_binary_view(), &st);
        check_eq("RAM: 8 Binaries gebaut", st.succeeded, std::size_t{8});
        check_eq("RAM-gebunden: Peak-Parallelität == 2 (trotz parallel_jobs=16)", st.peak_concurrency, std::size_t{2});
        check_true("RAM-Low-Water-Mark gemessen (≤ 10GB, > 0)",
                   st.min_free_ram_bytes <= 10 * GB && st.min_free_ram_bytes > 0);
    }

    // -- (C) Inkrementell/resumierbar: fingerprint-aktuelle DLLs ueberspringen --
    // A2-EICHUNG (GATE 5, F7, 2026-08-05): die Resume-Aussage dieses Abschnitts ist unveraendert -- "eine
    // bestehende, aktuelle DLL wird uebersprungen, eine veraltete neu gebaut". Nur das Kriterium ist ein
    // anderes: nicht mehr die build_version im `.version`-Sidecar, sondern der erwartete CT-Fingerprint
    // gegen `.fingerprint`. Deshalb traegt hier ein Test-FingerprintFn die "Bau-Welt" (v1/v2) statt
    // cfg.build_version. Die build_version bleibt gesetzt und KONSTANT -- sie beweist mit, dass sie den
    // Skip weder herbeifuehrt noch verhindert.
    {
        ex::ExperimentTree tree = make_tree(factory, 5);
        auto               view = tree.static_binary_view();
        // Stub-Compile erzeugt die DLL-Datei (touch) → dll_is_current sieht sie + Sidecar.
        auto compile_touch = [](ex::BuildJob const& j) -> int {
            std::ofstream f{j.output, std::ios::binary | std::ios::trunc};
            f << "DLL";
            return 0;
        };
        auto gen = [](std::string const&) { return std::string{"//\n"}; };
        // Deterministischer Test-Fingerprint je (Welt, binary_id) -- 128 Hex-Zeichen, wie der echte K7b-Anker.
        auto fp_of = [](char welt, std::string const& id) {
            std::uint64_t h = 0xcbf29ce484222325ULL;
            h ^= static_cast<std::uint64_t>(static_cast<unsigned char>(welt));
            h *= 0x100000001b3ULL;
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
            for (int k = 0; k < 8; ++k) out += block;
            return out;
        };

        ex::BuildConfig cfg{4, 8, base / "inc_src", base / "inc_dll"};
        cfg.build_version = "v-konstant"; // ueber ALLE drei Laeufe unveraendert
        ex::BuildOrchestrator orch1{cfg, compile_touch, gen};
        orch1.set_fingerprint_provider([&](std::string const& id) { return fp_of('1', id); });
        ex::BuildStats s1;
        orch1.provision_all(view, &s1);
        check_eq("v1 Erstbau: 5 gebaut, 0 übersprungen", s1.built, std::size_t{5});
        check_eq("v1 Erstbau: skipped == 0", s1.skipped, std::size_t{0});

        // Re-Provision in derselben Fingerprint-Welt -> ALLE ueberspringen (Resume nach Absturz).
        ex::BuildOrchestrator orch2{cfg, compile_touch, gen};
        orch2.set_fingerprint_provider([&](std::string const& id) { return fp_of('1', id); });
        ex::BuildStats s2;
        orch2.provision_all(view, &s2);
        check_eq("v1 Re-Run: 5 uebersprungen (fingerprint-aktuell)", s2.skipped, std::size_t{5});
        check_eq("v1 Re-Run: 0 neu gebaut", s2.built, std::size_t{0});

        // Andere Bau-Welt (anderer Fingerprint bei GLEICHER build_version) -> ALLE neu bauen.
        ex::BuildOrchestrator orch3{cfg, compile_touch, gen};
        orch3.set_fingerprint_provider([&](std::string const& id) { return fp_of('2', id); });
        ex::BuildStats s3;
        orch3.provision_all(view, &s3);
        check_eq("v2: 5 neu gebaut (Fingerprint geaendert)", s3.built, std::size_t{5});
        check_eq("v2: 0 übersprungen", s3.skipped, std::size_t{0});

        // Ein einzelner entfernter Anker -> GENAU diese eine Binary wird neu gebaut (der Resume-Kern:
        // per-Binary, nicht pauschal). Frueher war der Beweis "rm .version"; die Marke ist jetzt der Anker.
        {
            // Flaches Layout (per_binary_subdirs=false): die Anker heissen `perm_<stem>.dll.fingerprint`.
            std::size_t entfernt = 0;
            for (auto const& e : std::filesystem::recursive_directory_iterator{base / "inc_dll"}) {
                if (entfernt == 0 && e.is_regular_file() && e.path().extension() == ".fingerprint") {
                    std::filesystem::remove(e.path(), ec);
                    ++entfernt;
                }
            }
            check_eq("Resume-Kern: genau 1 Anker entfernt", entfernt, std::size_t{1});
            ex::BuildOrchestrator orch4{cfg, compile_touch, gen};
            orch4.set_fingerprint_provider([&](std::string const& id) { return fp_of('2', id); });
            ex::BuildStats s4;
            orch4.provision_all(view, &s4);
            check_eq("rm .fingerprint: GENAU 1 neu gebaut", s4.built, std::size_t{1});
            check_eq("rm .fingerprint: die uebrigen 4 uebersprungen", s4.skipped, std::size_t{4});
        }
    }

    std::cout << "\n==== KF-16b BuildOrchestrator-Härtung: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
