// Phase A (2026-06-04) BUILD-VERIFIKATION der Per-Achsen-Observer-Vervollständigung:
//   IObservableTierV3::tier_observe_v3 → ComdareTierObserverSnapshotV3.axis_stats[19][8] über mehrere reale
//   SA-Kompositionen (in-process Stand-in: identische vtable/POD-Layout wie über die .dll-Grenze; das
//   dynamic_cast<IObservableTierV3*> ist exakt der Host-Pfad). Emittiert die WIDE-Schema-CSV via die ECHTEN
//   Writer (lazy_csv_header/format_csv_row) nach build/thesis_tiere/obs_phaseA_pilot.csv und prüft literal:
//     (1) die 10 Phase-A-Achsen (T0 search_algo, T1 cache_traversal, T2 mapping, T4 node_type, T5 memory_layout,
//         T6 allocator, T9 serialization, T10 telemetry, T17 queuing_q1, T18 queuing_q2) tragen je >0 statistics;
//     (2) die 4 NEU verdrahteten (T1/T2/T17/T18) sind > 0;
//     (3) die 9 Phase-B-Achsen (T3,T7,T8,T11..T16) sind 0 (erwartet — noch nicht implementiert);
//     (4) filled_axis_count == 10.
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/perm_runner.hpp>                    // run_observable_perm (treibt den V3-Pfad)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>  // lazy_csv_header / format_csv_row / LazyMeasuredRow

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace ex   = ::comdare::cache_engine::builder::experiment;

static int g_fail = 0;
static void tr(char const* w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

// Phase-A: genau diese 10 Achsen-Indizes sind befüllt; alle anderen müssen 0 sein (Phase B).
static constexpr int kFilled[10] = {0, 1, 2, 4, 5, 6, 9, 10, 17, 18};
static bool is_filled(int t) { for (int x : kFilled) if (x == t) return true; return false; }

// Summe aller 8 Felder einer Achsen-Zeile (>0 ⇔ Achse trägt echte Werte).
static std::uint64_t row_sum(an::ComdareTierObserverSnapshotV3 const& s, int t) {
    std::uint64_t v = 0; for (std::size_t f = 0; f < an::kV3FieldCount; ++f) v += s.axis_stats[t][f]; return v;
}

template <class C>
static an::ComdareTierObserverSnapshotV3 measure_v3(char const* name, std::string& csv_out) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto* base = static_cast<an::IAnatomyBase*>(&tier);
    auto* obs  = dynamic_cast<an::IObservableTier*>(base);
    auto* obs3 = dynamic_cast<an::IObservableTierV3*>(base);   // der echte Host-Abfrage-Pfad

    // run_observable_perm treibt tier_clear → 1000 insert + 1000 lookup (koppelt T1/T2/T17/T18 auto) → zieht V3.
    ex::PermResult const pr = ex::run_observable_perm(*obs, name, /*n_ops=*/1000, obs3);

    // Echte CSV-Zeile via format_csv_row (identisches WIDE-Schema wie der E2E-Treiber).
    ex::LazyMeasuredRow row;
    row.binary_id     = name;
    row.setting_label = "-";
    row.n_ops         = pr.n_ops;
    row.total_ns      = pr.total_ns;
    row.v3            = pr.v3;
    row.v3_real       = pr.v3_real;
    csv_out += ex::format_csv_row(row);

    std::cout << "  " << name << ": v3_real=" << (pr.v3_real ? 1 : 0)
              << " filled_axis_count=" << pr.v3.filled_axis_count << "\n    T0..T18 row_sum=";
    for (int t = 0; t < 19; ++t) std::cout << row_sum(pr.v3, t) << (t < 18 ? "," : "");
    std::cout << "\n";
    return pr.v3;
}

template <class C>
static void check_one(char const* name, an::ComdareTierObserverSnapshotV3 const& s) {
    // (1) alle 10 Phase-A-Achsen > 0; (3) alle 9 Phase-B-Achsen == 0.
    bool filled_ok = true, phaseb_zero = true;
    for (int t = 0; t < 19; ++t) {
        std::uint64_t const sum = row_sum(s, t);
        if (is_filled(t)) { if (sum == 0) filled_ok = false; }
        else              { if (sum != 0) phaseb_zero = false; }
    }
    tr((std::string{name} + ": alle 10 Phase-A-Achsen > 0").c_str(), filled_ok);
    tr((std::string{name} + ": alle 9 Phase-B-Achsen == 0 (erwartet)").c_str(), phaseb_zero);
    tr((std::string{name} + ": filled_axis_count == 10").c_str(), s.filled_axis_count == an::kV3FilledAxisCount);
    // (2) die 4 NEU verdrahteten Achsen (T1/T2/T17/T18) explizit > 0.
    tr((std::string{name} + ": T1 cache_traversal > 0").c_str(), row_sum(s, 1) > 0);
    tr((std::string{name} + ": T2 mapping > 0").c_str(),         row_sum(s, 2) > 0);
    tr((std::string{name} + ": T17 queuing_q1 > 0").c_str(),     row_sum(s, 17) > 0);
    tr((std::string{name} + ": T18 queuing_q2 > 0").c_str(),     row_sum(s, 18) > 0);
}

int main() {
    std::cout << "==== Phase A: Per-Achsen-Observer-V3 (axis_stats[19][8]) in-process ====\n";

    std::string csv = ex::lazy_csv_header();
    auto art  = measure_v3<comp::ArtComposition>("ArtComposition", csv);
    auto hot  = measure_v3<comp::HotComposition>("HotComposition", csv);
    auto mass = measure_v3<comp::MasstreeComposition>("MasstreeComposition", csv);

    char const* out_path = "build/thesis_tiere/obs_phaseA_pilot.csv";
    { std::ofstream f{out_path, std::ios::trunc}; if (f) f << csv; }
    std::cout << "CSV: " << out_path << "\n";

    check_one<comp::ArtComposition>("Art", art);
    check_one<comp::HotComposition>("Hot", hot);
    check_one<comp::MasstreeComposition>("Masstree", mass);

    std::cout << "==== Phase A Per-Achsen-Observer-V3: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
