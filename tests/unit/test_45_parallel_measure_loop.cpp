// test_45_parallel_measure_loop -- #45 (§16.2-M1/§61-MODI): der Debug-Modus paralleler Mess-Loop. Testet die drei
// Primitive OHNE echte DLL-Messung (schnell, deterministisch):
//   (1) collect_ordered: pool<=1 => sequentiell (max_concurrency==1), pool>1 => parallel (max_concurrency>1); die
//       Outcomes IMMER in INDEX-Ordnung (Determinismus-Wache: parallel==sequentiell strukturell); je Worker EIN ctx.
//   (2) resolve_measure_parallelism: debug => Pool>0 (COMDARE_MEASURE_PARALLEL bzw. nproc), measure/release/undeklariert
//       => 0 (sequentiell/byte-neutral); der Env-Override greift NUR im Debug (Gate schlaegt Env).
//   (3) parse_result_line_to_node_value: wohlgeformte 160-Feld-Zeile => korrekter NodeValue; sonst std::nullopt.
// Build: plain main (KEIN gtest), Return 0/1 -- registriert via COMDARE_MCE24_PLAIN_TESTS.

#include "builder/experiment_tree/parallel_measure_pool.hpp" // collect_ordered
#include "builder/experiment_tree/measure_parallelism.hpp"   // resolve_measure_parallelism
#include "builder/experiment_tree/result_ingest.hpp"         // parse_result_line_to_node_value

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;

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
    // (1) collect_ordered: Determinismus + Concurrency-Wache + per-Worker-ctx.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    constexpr std::size_t kN = 40;
    // process schlaeft kurz -> erzwingt Ueberlappung im Parallel-Fall (sonst liefe ein Worker alles vor den anderen).
    auto make_ctx_counting = [](std::atomic<int>* ctx_count) {
        return [ctx_count] {
            if (ctx_count) ctx_count->fetch_add(1);
            return std::make_unique<int>(0); // ctx = irgendeine per-Worker-Ressource (Platzhalter fuer den pmc)
        };
    };
    auto process = [](std::unique_ptr<int>&, std::size_t i) -> std::size_t {
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // Ueberlappungs-Fenster
        return i * 10; // deterministischer Wert je Index (fuer die Ordnungs-/Determinismus-Pruefung)
    };

    // -- Sequentiell (pool=1): max_concurrency==1, EIN ctx. --
    {
        std::atomic<int> ctx_count{0};
        std::size_t      mc      = 999;
        auto             results = ex::collect_ordered<std::size_t>(kN, 1, make_ctx_counting(&ctx_count), process, &mc);
        check_eq("(1seq) max_concurrency == 1", mc, std::size_t{1});
        check_eq("(1seq) genau 1 ctx erzeugt", ctx_count.load(), 1);
        bool ordered = (results.size() == kN);
        for (std::size_t i = 0; i < results.size(); ++i) ordered = ordered && (results[i] == i * 10);
        check_true("(1seq) Ergebnisse in INDEX-Ordnung", ordered);
    }

    // -- Parallel (pool=4): max_concurrency>1, mehrere ctx, IDENTISCHE Index-Ordnung (Determinismus vs. sequentiell). --
    {
        std::atomic<int> ctx_count{0};
        std::size_t      mc      = 0;
        auto             results = ex::collect_ordered<std::size_t>(kN, 4, make_ctx_counting(&ctx_count), process, &mc);
        auto results2 = ex::collect_ordered<std::size_t>(kN, 4, make_ctx_counting(nullptr), process, nullptr);
        check_true("(1par) max_concurrency > 1 (parallel bewiesen)", mc > 1);
        check_true("(1par) mehr als 1 ctx erzeugt (je Worker einer)", ctx_count.load() > 1);
        bool ordered = (results.size() == kN);
        for (std::size_t i = 0; i < results.size(); ++i) ordered = ordered && (results[i] == i * 10);
        check_true("(1par) Ergebnisse in INDEX-Ordnung (== sequentiell strukturell)", ordered);
        check_true("(1par) zwei Parallel-Laeufe byte-gleich (Determinismus)", results == results2);
        std::cout << "  [INFO] parallel max_concurrency=" << mc << " ctx_erzeugt=" << ctx_count.load() << "\n";
    }

    // -- n==0 Randfall. --
    {
        std::size_t mc      = 7;
        auto        results = ex::collect_ordered<std::size_t>(0, 4, make_ctx_counting(nullptr), process, &mc);
        check_true("(1edge) n==0 -> leeres Ergebnis, max_concurrency==0", results.empty() && mc == 0);
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // (2) resolve_measure_parallelism: der Debug-Gate.
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        ::unsetenv("COMDARE_MEASURE_PARALLEL");
        check_true("(2) measure => 0 (sequentiell)", ex::resolve_measure_parallelism({"measure"}) == 0);
        check_true("(2) release => 0 (sequentiell)", ex::resolve_measure_parallelism({"release"}) == 0);
        check_true("(2) undeklariert => 0 (measure-Default, sequentiell)", ex::resolve_measure_parallelism({}) == 0);
        check_true("(2) debug (ohne Env) => nproc>0 (parallel)", ex::resolve_measure_parallelism({"debug"}) > 0);

        ::setenv("COMDARE_MEASURE_PARALLEL", "5", 1);
        check_eq("(2) debug + COMDARE_MEASURE_PARALLEL=5 => 5", ex::resolve_measure_parallelism({"debug"}),
                 std::size_t{5});
        check_true("(2) measure + Env=5 => 0 (Gate schlaegt Env)", ex::resolve_measure_parallelism({"measure"}) == 0);

        ::setenv("COMDARE_MEASURE_PARALLEL", "nonsense", 1);
        check_true("(2) debug + unparsebares Env => nproc>0 (Default, kein Abbruch)",
                   ex::resolve_measure_parallelism({"debug"}) > 0);
        ::unsetenv("COMDARE_MEASURE_PARALLEL");

        // (2b smoke=>debug-Entkopplung 2026-07-22): der METHODIK-Override (profile_run_entry-Naht
        //   override.empty() ? tp.run_methodology : override) hat Vorrang -- ein measure-KATALOG misst PARALLEL, wenn
        //   die Methodik debug ist; OHNE Override bleibt der measure-Katalog 1-Thread. Beweis der Entkopplung
        //   Bau-Profil != Methodik-Profil (Runtime-Seite; die Emit-Seite deckt test_experiment_plan_director ab).
        ::setenv("COMDARE_MEASURE_PARALLEL", "6", 1);
        std::vector<std::string> const catalog_measure{"measure"}; // all_axes_golden-Katalog
        std::vector<std::string> const methodik_debug{"debug"};    // m3_smoke_coverage-Methodik-Override
        auto const                     eff = methodik_debug.empty() ? catalog_measure : methodik_debug; // Naht :413
        check_eq("(2b) measure-Katalog + debug-Override => 6 (parallel; Override hat Vorrang)",
                 ex::resolve_measure_parallelism(eff), std::size_t{6});
        check_true("(2b) measure-Katalog OHNE Override => 0 (1-Thread, byte-neutral)",
                   ex::resolve_measure_parallelism(catalog_measure) == 0);
        ::unsetenv("COMDARE_MEASURE_PARALLEL");
    }

    // ════════════════════════════════════════════════════════════════════════════════════════════════
    // (3) parse_result_line_to_node_value: die reine Parse-Naht (byte-identisch zum ingest-Round-Trip).
    // ════════════════════════════════════════════════════════════════════════════════════════════════
    {
        // Wohlgeformte 160-Feld-Zeile: binary_id + 159 Datenfelder, Feld[k] == k (1-indexiert nach der id).
        std::string line = "tb";
        for (int k = 1; k <= 159; ++k) line += ";" + std::to_string(k);
        auto parsed = ex::parse_result_line_to_node_value(line);
        check_true("(3) 160-Feld-Zeile geparst (has_value)", parsed.has_value());
        if (parsed) {
            check_eq("(3) binary_id == 'tb'", parsed->first, std::string{"tb"});
            check_true("(3) NodeValue.has_result", parsed->second.has_result);
            auto const& o = parsed->second.observer;
            // axis_stats[t][fi] == Feld 1 + t*8 + fi ; seg_ns[t] == 137+t ; Meta 154..157 ; P-MD3 158/159.
            check_eq("(3) axis_stats[0][0] == 1", o.axis_stats[0][0], std::uint64_t{1});
            check_eq("(3) axis_stats[0][7] == 8", o.axis_stats[0][7], std::uint64_t{8});
            check_eq("(3) axis_stats[1][0] == 9", o.axis_stats[1][0], std::uint64_t{9});
            check_eq("(3) seg_ns[0] == 137", o.seg_ns[0], std::int64_t{137});
            check_eq("(3) observable_axis_count == 154", o.observable_axis_count, std::uint64_t{154});
            check_eq("(3) seg_run_total_ns == 159", o.seg_run_total_ns, std::int64_t{159});
            check_eq("(3) legacy search_lookup == axis_stats[0][0] == 1", o.search_lookup_count, std::uint64_t{1});
            check_eq("(3) legacy alloc_bytes_allocated == axis_stats[6][0] == 49", o.alloc_bytes_allocated,
                     std::uint64_t{49});
        }
        check_true("(3) 159-Feld-Zeile (falsch) => nullopt",
                   !ex::parse_result_line_to_node_value("tb;1;2").has_value());
        check_true("(3) leere Zeile => nullopt", !ex::parse_result_line_to_node_value("").has_value());
        check_true("(3) Kommentar '#' => nullopt", !ex::parse_result_line_to_node_value("#x").has_value());
    }

    std::cout << "\n==== #45 paralleler Mess-Loop (collect_ordered/resolve/parse): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
