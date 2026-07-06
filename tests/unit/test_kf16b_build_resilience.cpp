// test_kf16b_build_resilience — KF-16b (2026-06-02)
// Belegt: (A) Default cores_per_build=4 + KEINE Oversubscription (parallel_jobs × cores ≤ Kerne, gekappt);
// (B) RAM-Admission (freier RAM gemessen, weiterer Build nur wenn genug RAM, mind. 1 läuft immer); (C)
// inkrementell-resumierbar (versions-aktuelle DLLs übersprungen). Stub-CompileFn/FreeRamFn → deterministisch.
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
    t.build({ex::AxisLevel{"traversal", {"ART"}, true, ""}, ex::AxisLevel{"node", nodes, true, ""}});
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

    // ── (C) Inkrementell/resumierbar: versions-aktuelle DLLs überspringen ──
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

        ex::BuildConfig cfg{4, 8, base / "inc_src", base / "inc_dll"};
        cfg.build_version = "v1";
        ex::BuildOrchestrator orch1{cfg, compile_touch, gen};
        ex::BuildStats        s1;
        orch1.provision_all(view, &s1);
        check_eq("v1 Erstbau: 5 gebaut, 0 übersprungen", s1.built, std::size_t{5});
        check_eq("v1 Erstbau: skipped == 0", s1.skipped, std::size_t{0});

        // Re-Provision mit gleicher Version → ALLE überspringen (Resume nach Absturz).
        ex::BuildOrchestrator orch2{cfg, compile_touch, gen};
        ex::BuildStats        s2;
        orch2.provision_all(view, &s2);
        check_eq("v1 Re-Run: 5 übersprungen (versions-aktuell)", s2.skipped, std::size_t{5});
        check_eq("v1 Re-Run: 0 neu gebaut", s2.built, std::size_t{0});

        // Neue Version → ALLE neu bauen.
        ex::BuildConfig cfg2 = cfg;
        cfg2.build_version   = "v2";
        ex::BuildOrchestrator orch3{cfg2, compile_touch, gen};
        ex::BuildStats        s3;
        orch3.provision_all(view, &s3);
        check_eq("v2: 5 neu gebaut (Version geändert)", s3.built, std::size_t{5});
        check_eq("v2: 0 übersprungen", s3.skipped, std::size_t{0});
    }

    std::cout << "\n==== KF-16b BuildOrchestrator-Härtung: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
