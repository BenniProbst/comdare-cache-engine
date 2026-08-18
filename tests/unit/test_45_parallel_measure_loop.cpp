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
#include <stdexcept> // Welle B/1: fail-closed bei unbekanntem Modus-Token
#include <string>
#include <thread>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;
// A-05/V-12: die Registry-Zeile als Zustand (WorkModeInfo/WorkMode) -- fuer den state-direkten Zugang
// zur Parallelitaets-Mechanik, deren Token-Eingang mit dem work_mode-Umbau entfallen ist.
namespace mm = comdare::cache_engine::measurement;

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
        check_true("(2) build => 0 (sequentiell)", ex::resolve_measure_parallelism({"build"}) == 0);
        check_true("(2) compare => 0 (sequentiell)", ex::resolve_measure_parallelism({"compare"}) == 0);
        check_true("(2) undeklariert => 0 (measure-Default, sequentiell)", ex::resolve_measure_parallelism({}) == 0);

        // -- A-05/V-12 (18.08.2026): DER TOKEN "debug" IST WEG -- UND DAS WIRD HIER POSITIV GEPRUEFT -----
        //
        // Bis zum work_mode-Umbau stand hier `resolve_measure_parallelism({"debug"}) > 0`. Der Token
        // existiert nicht mehr; der Aufruf laeuft heute in die fail-closed-Wache der Registry. Diese Zeile
        // WAR also nicht "kaputt" -- sie beschrieb eine Welt, die es nicht mehr gibt. Statt sie zu
        // streichen, dreht sie sich um: der Abgang des Tokens ist jetzt selbst die Zusage, und zwar mit
        // der EXAKTEN Meldung, damit ein stiller Rueckfall auf measure auffiele.
        {
            bool        geworfen = false;
            std::string gesehen;
            try {
                (void)ex::resolve_measure_parallelism({"debug"});
            } catch (std::invalid_argument const& e) {
                geworfen = true;
                gesehen  = e.what();
            }
            check_true("(2/V-12) Token 'debug' existiert nicht mehr => wirft (kein stiller measure-Ersatz)", geworfen);
            std::string const soll =
                "run_methodology_for_ids: unbekannter Modus-Token \"debug\" -- gueltig sind {build, measure, compare, "
                "release}. FAIL-CLOSED (Welle B/1): ein stiller measure-Ersatz wuerde eine Messung ausloesen, die "
                "niemand angefordert hat.";
            check_eq("(2/V-12) und zwar mit der EXAKTEN Meldung (Token + gueltige Menge + Grund)", gesehen, soll);
        }

        // -- DIE MECHANIK HINTER DEM GATE, STATE-DIREKT GETRIEBEN ---------------------------------------
        //
        // Das Gate verlangt (measurement_on && !single_thread). KEINE heutige Registry-Zeile erfuellt das
        // -- "debug" war die einzige. Ueber den Token-Weg ist alles dahinter deshalb unerreichbar. Geprueft
        // wird es trotzdem, und zwar am ZUSTAND: eine Zeile mit genau dieser Eigenschaft, so wie der
        // ausgebaute Modus sie trug. Das haelt Env-Auswertung und nproc-Default unter Beobachtung, bis der
        // --debug-Eingang (S-8/W2) sie wieder erreichbar macht.
        {
            mm::WorkModeInfo const misst_parallel{mm::WorkMode::Measure,   "measure", "Measure", "Debug",
                                                  /*measurement_on=*/true,
                                                  /*single_thread=*/false};
            ::unsetenv("COMDARE_MEASURE_PARALLEL");
            check_true("(2s) misst+parallel, ohne Env => nproc>0",
                       ex::resolve_measure_parallelism_of_mode(misst_parallel) > 0);

            ::setenv("COMDARE_MEASURE_PARALLEL", "5", 1);
            check_eq("(2s) misst+parallel + Env=5 => 5", ex::resolve_measure_parallelism_of_mode(misst_parallel),
                     std::size_t{5});
            check_true("(2s) measure-Zeile + Env=5 => 0 (Gate schlaegt Env)",
                       ex::resolve_measure_parallelism({"measure"}) == 0);

            ::setenv("COMDARE_MEASURE_PARALLEL", "nonsense", 1);
            check_true("(2s) misst+parallel + unparsebares Env => nproc>0 (Default, kein Abbruch)",
                       ex::resolve_measure_parallelism_of_mode(misst_parallel) > 0);
            ::unsetenv("COMDARE_MEASURE_PARALLEL");

            // Gegenprobe, damit (2s) nicht bloss den Injektor misst: DIESELBE Zeile mit single_thread=true
            // faellt zurueck auf 0. Ohne sie koennte das Gate ausgebaut sein und alles hier bliebe gruen.
            mm::WorkModeInfo const misst_1thread{mm::WorkMode::Measure, "measure", "Measure", "Debug", true, true};
            ::setenv("COMDARE_MEASURE_PARALLEL", "5", 1);
            check_true("(2s) Gegenprobe: misst+1-Thread => 0 trotz Env=5 (das Gate lebt)",
                       ex::resolve_measure_parallelism_of_mode(misst_1thread) == 0);
            ::unsetenv("COMDARE_MEASURE_PARALLEL");
        }

        // (2c Welle B/1, 2026-08-07) FAIL-CLOSED: ein UNBEKANNTES Token (Tippfehler im Profil) fiel bis heute STILL
        //   auf measure zurueck -- resolve_measure_parallelism lieferte klaglos 0 und der Lauf MASS, obwohl niemand
        //   diesen Modus angefordert hatte. Jetzt wirft die Registry (run_methodology_for_ids).
        auto wirft_bei = [](std::vector<std::string> const& ids) {
            try {
                (void)ex::resolve_measure_parallelism(ids);
            } catch (std::invalid_argument const&) { return true; }
            return false;
        };
        check_true("(2c) unbekannter Token 'mesure' => wirft (kein stiller measure-Ersatz)", wirft_bei({"mesure"}));
        check_true("(2c) unbekannter Token 'profiling' => wirft", wirft_bei({"profiling"}));
        check_true("(2c) leeres Token '' => wirft (Token != leere Liste)", wirft_bei({""}));
        // Gegenprobe: die GUELTIGEN Tokens und die LEERE Liste werfen NIE (kein Fehlalarm, byte-neutral).
        // V-12: "debug" steht hier NICHT mehr -- er ist seit dem work_mode-Umbau ungueltig und wird oben
        // als POSITIV-Fall gefuehrt. Die Menge kommt aus der Registry, nicht aus dieser Zeile.
        {
            bool alle_gueltigen_still = true;
            for (std::size_t i = 0; i < mm::kWorkModeCount; ++i)
                if (wirft_bei({std::string(mm::kWorkModeRegistry[i].id)})) alle_gueltigen_still = false;
            check_true("(2c) Gegenprobe: ALLE Registry-Tokens + leere Liste werfen nicht",
                       alle_gueltigen_still && !wirft_bei({}));
        }

        // (2b smoke=>debug-Entkopplung 2026-07-22, A-05/V-12-Nachzug 18.08.2026): der METHODIK-Override
        //   (profile_run_entry-Naht override.empty() ? tp.run_methodology : override) hat Vorrang vor dem
        //   Katalog. GEPRUEFT WIRD DIE NAHT, NICHT MEHR IHRE WIRKUNG AUF DIE PARALLELITAET: seit V-12 ist
        //   KEIN Token mehr "misst und laeuft parallel", also liefern Katalog und Override denselben Wert 0
        //   -- ein Vergleich der ZAHLEN koennte den Vorrang nicht mehr zeigen. Sichtbar bleibt er an der
        //   fail-closed-Wache: ein ungueltiger Override wirft, obwohl der Katalog gueltig ist. Genau das
        //   beweist, dass der Override den Katalog verdraengt. Die Parallelitaets-Haelfte deckt (2s) am
        //   Zustand ab; die Emit-Seite deckt test_experiment_plan_director.
        ::setenv("COMDARE_MEASURE_PARALLEL", "6", 1);
        std::vector<std::string> const catalog_measure{"measure"}; // all_axes_golden-Katalog
        std::vector<std::string> const methodik_ovr{"mesure"};     // Override mit Tippfehler
        auto const                     eff = methodik_ovr.empty() ? catalog_measure : methodik_ovr; // Naht :413
        check_true("(2b) Override verdraengt den Katalog: ungueltiger Override wirft trotz gueltigem Katalog",
                   wirft_bei(eff) && !wirft_bei(catalog_measure));
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
