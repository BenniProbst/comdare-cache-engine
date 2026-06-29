// test_cacheline_policy_selector — #174 (Goal §8.3 / AP-X2 / TODO-3 Versprechen 2)
// Standalone-Treiber (kein gtest, kein Python) für CacheLinePolicySelector (GoF Strategy).
//
// BELEG: 2 verschiedene Eingangs-Profile (scan-lastig vs punkt-lastig) → 2 VERSCHIEDENE,
// gegen die realen SA-Tier-Caps geklammerte ComdareResourceControlV1; apply_to setzt den Wert real.
// Zusätzlich: Aggregation der ECHTEN Pilot-CSV je search_algo (zeigt den punkt-lastigen Pol real).
//
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_cacheline_policy_selector.cpp

#include "anatomy/resource_controllable_tier.hpp"
#include "builder/cacheline_policy/cacheline_policy_selector.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace an = comdare::cache_engine::anatomy;
namespace cp = comdare::cache_engine::builder::cacheline_policy;

static int  g_fail = 0;
static void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}
template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (static_cast<std::uint64_t>(got) == static_cast<std::uint64_t>(want));
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << static_cast<std::uint64_t>(got);
    if (!ok) std::cout << "  (erwartet: " << static_cast<std::uint64_t>(want) << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

static void dump(char const* tag, an::ComdareResourceControlV1 const& r) {
    std::cout << "    " << tag << ": thread=" << r.thread_count << " prefetch=" << r.prefetch_distance
              << " pool=" << r.pool_budget_bytes << " batch=" << r.batch_size << " inline=" << r.inline_threshold_bytes
              << "\n";
}

// Reales SearchAlgorithm-Tier-Cap-Double — spiegelt abi_adapter.hpp:177-201 (64/64/1GiB/4096/256).
class SaTierDouble : public an::IResourceControllableTier {
public:
    an::ComdareResourceControlV1 applied{};
    void                         tier_query_resource_caps(an::ComdareResourceControlV1* out) const noexcept override {
        if (!out) return;
        *out                         = an::ComdareResourceControlV1{};
        out->thread_count            = 64;
        out->prefetch_distance       = 64;
        out->pool_budget_bytes       = (std::uint64_t{1} << 30); // 1 GiB
        out->batch_size              = 4096;
        out->inline_threshold_bytes  = 256;
        out->controllable_axis_count = 5;
    }
    std::uint64_t tier_apply_resource_control(an::ComdareResourceControlV1 const* in) noexcept override {
        if (!in) return 0;
        an::ComdareResourceControlV1 caps{};
        tier_query_resource_caps(&caps);
        std::uint64_t n  = 0;
        auto          a1 = [&](std::uint64_t v, std::uint64_t cap, std::uint64_t& dst) {
            if (v != 0) {
                dst = (cap != 0 && v > cap) ? cap : v;
                ++n;
            }
        };
        a1(in->thread_count, caps.thread_count, applied.thread_count);
        a1(in->prefetch_distance, caps.prefetch_distance, applied.prefetch_distance);
        a1(in->pool_budget_bytes, caps.pool_budget_bytes, applied.pool_budget_bytes);
        a1(in->batch_size, caps.batch_size, applied.batch_size);
        a1(in->inline_threshold_bytes, caps.inline_threshold_bytes, applied.inline_threshold_bytes);
        return n;
    }
};

int main(int argc, char** argv) {
    cp::CacheLinePolicySelector<> selector; // Default-Objective = ScanOptimizing

    // ── Profil 1: SCAN-LASTIG (YCSB-E: scan=0.95, lookup=0.05), großer Working-Set 1<<20 ──
    auto prof_scan = cp::WorkloadProfileAggregate::from_shares(
        /*ins*/ 0.0, /*lk*/ 0.05, /*er*/ 0.0, /*scan*/ 0.95, /*rmw*/ 0.0,
        /*num_ops*/ 100000, /*working_set*/ (std::uint64_t{1} << 20)); // 1048576 Records

    // ── Profil 2: PUNKT-LASTIG (YCSB-C: lookup=1.0), kleiner Working-Set 16384 (wie Pilot-CSV) ──
    auto prof_point = cp::WorkloadProfileAggregate::from_shares(
        /*ins*/ 0.0, /*lk*/ 1.0, /*er*/ 0.0, /*scan*/ 0.0, /*rmw*/ 0.0,
        /*num_ops*/ 100000, /*working_set*/ 16384);

    an::ComdareResourceControlV1 desired_scan  = selector.select(prof_scan);
    an::ComdareResourceControlV1 desired_point = selector.select(prof_point);

    std::cout << "== gewuenschte (ungeklammerte) Steuerung je Profil ==\n";
    std::cout << "  scan_share(scan)=" << prof_scan.scan_share() << "  scan_share(point)=" << prof_point.scan_share()
              << "\n";
    dump("scan ", desired_scan);
    dump("point", desired_point);

    // Distinktheit der gewünschten Werte (Heuristik trennt die Pole).
    check("scan != point (Heuristik liefert distinkte PODs)", !(desired_scan == desired_point));
    check_eq("scan  prefetch_distance = 16", desired_scan.prefetch_distance, 16u);
    check_eq("point prefetch_distance = 2", desired_point.prefetch_distance, 2u);
    // scan: batch = ws = 1048576 (vor Cap); point: batch = 64 (HW-Linie).
    check_eq("scan  batch_size (vor Cap) = 1048576", desired_scan.batch_size, 1048576u);
    check_eq("point batch_size = 64", desired_point.batch_size, 64u);

    // ── apply_to am Prüf-Dock: clamp gegen reale SA-Caps (64/64/1GiB/4096/256) ──
    SaTierDouble  tier_scan, tier_point;
    std::uint64_t n_scan  = selector.apply(prof_scan, &tier_scan);
    std::uint64_t n_point = selector.apply(prof_point, &tier_point);

    std::cout << "== nach apply_to (gegen Caps 64/64/1GiB/4096/256 geklammert) ==\n";
    dump("scan ", tier_scan.applied);
    dump("point", tier_point.applied);

    // scan: prefetch 16 (<=64), batch 1048576 -> Cap 4096, pool 1048576*64=67108864 (<1GiB), inline 16 (read-lastig).
    check_eq("scan  applied prefetch = 16", tier_scan.applied.prefetch_distance, 16u);
    check_eq("scan  applied batch -> Cap 4096", tier_scan.applied.batch_size, 4096u);
    check_eq("scan  applied pool = 67108864", tier_scan.applied.pool_budget_bytes, 67108864u);
    check_eq("scan  applied inline = 16 (read)", tier_scan.applied.inline_threshold_bytes, 16u);

    // point: prefetch 2, batch 64, pool 16384*64=1048576, inline 16 (read-lastig).
    check_eq("point applied prefetch = 2", tier_point.applied.prefetch_distance, 2u);
    check_eq("point applied batch = 64", tier_point.applied.batch_size, 64u);
    check_eq("point applied pool = 1048576", tier_point.applied.pool_budget_bytes, 1048576u);

    // Distinktheit NACH dem Klammern (das eigentliche Versprechen).
    check("applied scan != applied point (geklammert distinkt)", !(tier_scan.applied == tier_point.applied));

    // #angewandte Achsen: scan setzt prefetch+pool+batch+inline = 4 (thread=0 -> nicht); point ebenso 4.
    check_eq("scan  #angewandte Achsen = 4", n_scan, 4u);
    check_eq("point #angewandte Achsen = 4", n_point, 4u);

    // Degradierung: nullptr -> 0 (kein Crash).
    check_eq("apply(nullptr) = 0", selector.apply(prof_scan, nullptr), 0u);

    // ── Env-Limit überschreibt Cap, wenn strenger (z.B. nur 1024 Batch erlaubt) ──
    an::ComdareResourceControlV1 env{};
    env.batch_size = 1024; // System: Batch auf 1024 begrenzen
    cp::CacheLinePolicySelector<> selector_env(env);
    SaTierDouble                  tier_env;
    (void)selector_env.apply(prof_scan, &tier_env);
    check_eq("env-Limit: scan batch -> 1024 (strenger als Cap 4096)", tier_env.applied.batch_size, 1024u);

    // ── Strategy-Austausch: LatencyOptimizing zieht gemischte Last auf Punkt-Profil ──
    auto prof_mixed = cp::WorkloadProfileAggregate::from_shares(0.30, 0.40, 0.0, 0.30, 0.0, 100000,
                                                                65536); // scan_share=0.30 -> "gemischt"
    cp::CacheLinePolicySelector<cp::ScanOptimizing>    sel_scanopt;
    cp::CacheLinePolicySelector<cp::LatencyOptimizing> sel_latopt;
    auto                                               mixed_scanopt = sel_scanopt.select(prof_mixed);
    auto                                               mixed_latopt  = sel_latopt.select(prof_mixed);
    std::cout << "== Strategy-Austausch bei gemischter Last (scan_share=0.30) ==\n";
    dump("scan-opt ", mixed_scanopt);
    dump("lat-opt  ", mixed_latopt);
    check_eq("scan-opt mixed prefetch = 8", mixed_scanopt.prefetch_distance, 8u);
    check_eq("lat-opt  mixed prefetch = 2", mixed_latopt.prefetch_distance, 2u);
    check("Strategy-Austausch ergibt distinkte PODs", !(mixed_scanopt == mixed_latopt));

    // ── (b) ECHTE Pilot-CSV je search_algo aggregieren (optional: Pfad als argv[1]) ──
    std::filesystem::path csv =
        (argc > 1) ? std::filesystem::path(argv[1])
                   : std::filesystem::path("C:/Users/benja/OneDrive/Desktop/Diplomarbeit - Datenbanken/Code/external/"
                                           "comdare-cache-engine/build/thesis_tiere/m3v2_sota_pilot_measurements.csv");
    std::map<std::string, cp::WorkloadProfileAggregate> by_algo;
    int                                                 rows = cp::aggregate_csv_by_search_algo(csv, by_algo);
    std::cout << "== CSV-Aggregat (echte Pilot-CSV) ==\n";
    if (rows < 0) {
        std::cout << "    [INFO] Pilot-CSV nicht gefunden/lesbar (" << csv.string()
                  << ") - CSV-Pfad-Beleg uebersprungen (Heuristik-Beleg oben ist unabhaengig).\n";
    } else {
        std::cout << "    Zeilen=" << rows << "  Gruppen=" << by_algo.size() << "\n";
        for (auto const& [algo, agg] : by_algo) {
            auto d = selector.select(agg);
            std::cout << "    search_algo=" << algo << "  scan_share=" << agg.scan_share()
                      << "  ws=" << agg.working_set_n << "  -> prefetch=" << d.prefetch_distance
                      << " batch=" << d.batch_size << "\n";
        }
        // Pilot-CSV ist lookup-only -> jede Gruppe MUSS scan_share==0 (punkt-lastig) ergeben.
        bool all_point = true;
        for (auto const& [algo, agg] : by_algo)
            if (agg.scan_share() != 0.0) all_point = false;
        check("Pilot-CSV-Aggregat ist durchgaengig punkt-lastig (lookup-only, verifiziert)", all_point);
    }

    std::cout << "\n==== #174 CacheLinePolicySelector-Test: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
