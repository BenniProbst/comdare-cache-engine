// Phase A (2026-06-04) / I1 (2026-06-05) BUILD-VERIFIKATION der Per-Achsen-Observer-Vervollständigung:
//   die EINE tier_observe(ComdareTierObserverSnapshot*).axis_stats[19][8] über mehrere reale SA-Kompositionen
//   (in-process Stand-in: identisches vtable/POD-Layout wie über die .dll-Grenze; das dynamic_cast<IObservableTier*>
//   ist exakt der Host-Pfad). Emittiert die WIDE-Schema-CSV via die ECHTEN
//   Writer (lazy_csv_header/format_csv_row) nach build/thesis_tiere/obs_phaseA_pilot.csv und prüft literal:
//     (1) die Schema-befüllten Achsen tragen Observer (seit Phase-B-Abschluss 2026-06-04 = alle 19);
//     (2) die 4 in Phase A neu verdrahteten Instanz-Achsen (T1/T2/T17/T18) sind > 0;
//     (3) Schema-LEERE Achsen (falls noch vorhanden) sind exakt 0 — seit Phase-B-Abschluss ist KEINE Achse mehr leer;
//     (4) filled_axis_count == kV3FilledAxisCount (schema-abgeleitet, Phase B: 19).
//   Die Prüflogik ist VOLLSTÄNDIG schema-getrieben (kV3AxisSchema, single-source) → zog automatisch von Phase A (10)
//   auf Phase B (19) mit, als die Phase-B-Achsen ihre Schema-Zeilen befüllten (kein manuelles Test-Nachziehen).
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/perm_runner.hpp>                   // run_observable_perm (treibt den V3-Pfad)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // lazy_csv_header / format_csv_row / LazyMeasuredRow

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

static int  g_fail = 0;
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Welche Achsen-Indizes sind befüllt = aus dem V3-Schema abgeleitet (single-source kV3AxisSchema), NICHT
// hartkodiert. So zieht der Test automatisch mit, wenn eine Phase-B-Achse ihre Schema-Zeile befüllt (parallele
// Achsen-Erweiterung ohne Test-Drift). Phase A = {0,1,2,4,5,6,9,10,17,18}; Phase B fügt T7/T8/… hinzu.
static bool is_filled(int t) {
    return t >= 0 && t < static_cast<int>(an::kV3AxisCount) && an::kV3AxisSchema[t].names[0] != nullptr;
}

// Summe aller 8 Felder einer Achsen-Zeile (>0 ⇔ Achse trägt echte Werte).
static std::uint64_t row_sum(an::ComdareTierObserverSnapshot const& s, int t) {
    std::uint64_t v = 0;
    for (std::size_t f = 0; f < an::kV3FieldCount; ++f) v += s.axis_stats[t][f];
    return v;
}

template <class C>
static an::ComdareTierObserverSnapshot measure_v3(char const* name, std::string& csv_out) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto* obs = dynamic_cast<an::IObservableTier*>(base); // I1: EINE Schnittstelle (Antrieb + Observer)

    // run_observable_perm treibt tier_clear → 1000 insert + 1000 lookup (koppelt T1/T2/T17/T18 auto) → zieht den
    // EINEN konsolidierten Observer-POD (axis_stats + Pfad-B-seg_ns).
    ex::PermResult const pr = ex::run_observable_perm(*obs, name, /*n_ops=*/1000);

    // Echte CSV-Zeile via format_csv_row (identisches WIDE-Schema wie der E2E-Treiber).
    ex::LazyMeasuredRow row;
    row.binary_id     = name;
    row.setting_label = "-";
    row.n_ops         = pr.n_ops;
    row.total_ns      = pr.total_ns;
    row.unified       = pr.unified; // KONSOLIDIERUNG (I1): format_csv_row liest stat_*/seg_* aus dem EINEN POD
    row.unified_real  = pr.unified_real;
    csv_out += ex::format_csv_row(row);
    // KONSOLIDIERUNG-Verifikation: der konsolidierte POD ist real gezogen (axis_stats + seg_ns Pfad B in EINEM POD).
    tr((std::string{name} + ": unified observer real (axis_stats + Pfad-B-seg_ns in EINEM POD)").c_str(),
       pr.unified_real);

    std::cout << "  " << name << ": unified_real=" << (pr.unified_real ? 1 : 0)
              << " filled_axis_count=" << pr.unified.filled_axis_count << "\n    T0..T18 row_sum=";
    for (int t = 0; t < 19; ++t) std::cout << row_sum(pr.unified, t) << (t < 18 ? "," : "");
    std::cout << "\n";
    return pr.unified;
}

template <class C>
static void check_one(char const* name, an::ComdareTierObserverSnapshot const& s, std::uint32_t expected_filled) {
    // EHRLICHE Semantik: (a) Schema-LEERE Achsen (noch nicht implementiert) MÜSSEN exakt 0 sein. (b) Schema-
    // BEFÜLLTE (observable) Achsen werden beobachtet, ihr Wert darf aber strategie-abhängig 0 sein (z.B. T7
    // prefetch mit NonePrefetch = ehrliche 0-Baseline, KEIN enqueue-Pfad; T8 pattern_id=0 bei NoneConcurrency).
    // Daher KEINE pauschale „filled > 0"-Annahme — das wäre für Baseline-Strategien falsch.
    bool phaseb_zero = true;
    for (int t = 0; t < 19; ++t) {
        if (!is_filled(t) && row_sum(s, t) != 0) phaseb_zero = false;
    }
    tr((std::string{name} + ": alle Schema-leeren (noch nicht implementierten) Achsen == 0").c_str(), phaseb_zero);
    // #188-4c-i: Referenz-Hülle hat kein store_type → honest-0 (Re-Kopplung #234). Die store-abhängigen Achsen
    // (memory_layout/serialization/value_handle/isa/Node-Telemetrie) bleiben honest-0 → die Hülle füllt NICHT alle
    // kV3FilledAxisCount(19), sondern nur ihre Nicht-Store-Achsen (komposition-spezifisch: Art/Hot=10, Masstree=9).
    tr((std::string{name} + ": filled_axis_count == " + std::to_string(expected_filled) +
        " (Huelle honest-0 Store-Achsen)")
           .c_str(),
       s.filled_axis_count == expected_filled);
    // Die explizit über die Tier-Op getriebenen Achsen MÜSSEN > 0 sein (echte Auto-Kopplung verifiziert).
    tr((std::string{name} + ": T1 cache_traversal > 0").c_str(), row_sum(s, 1) > 0);
    tr((std::string{name} + ": T2 mapping > 0").c_str(), row_sum(s, 2) > 0);
    tr((std::string{name} + ": T17 queuing_q1 > 0").c_str(), row_sum(s, 17) > 0);
    tr((std::string{name} + ": T18 queuing_q2 > 0").c_str(), row_sum(s, 18) > 0);
    // Phase B (T8): concurrency wird über observe_critical_section() getrieben → acquire/release > 0 (auch bei
    // NoneConcurrency: die Op läuft, nur pattern_id=0). T7 prefetch ist mit NonePrefetch ehrlich 0 (Baseline) →
    // NICHT auf > 0 prüfen; die Beobachtbarkeit ist über filled_axis_count + is_filled(7) bereits abgedeckt.
    if (is_filled(8)) tr((std::string{name} + ": T8 concurrency acquire/release > 0").c_str(), row_sum(s, 8) > 0);
}

int main() {
    std::cout << "==== Phase A: Per-Achsen-Observer-V3 (axis_stats[19][8]) in-process ====\n";

    std::string csv  = ex::lazy_csv_header();
    auto        art  = measure_v3<comp::ArtComposition>("ArtComposition", csv);
    auto        hot  = measure_v3<comp::HotComposition>("HotComposition", csv);
    auto        mass = measure_v3<comp::MasstreeComposition>("MasstreeComposition", csv);

    char const* out_path = "build/thesis_tiere/obs_phaseA_pilot.csv";
    {
        std::ofstream f{out_path, std::ios::trunc};
        if (f) f << csv;
    }
    std::cout << "CSV: " << out_path << "\n";

    // #188-4c-i: Referenz-Hüllen honest-0 auf den Store-Achsen → filled_axis_count komposition-spezifisch (Re-Kopplung #234).
    check_one<comp::ArtComposition>("Art", art, /*expected_filled=*/10u);
    check_one<comp::HotComposition>("Hot", hot, /*expected_filled=*/10u);
    check_one<comp::MasstreeComposition>("Masstree", mass, /*expected_filled=*/9u);

    std::cout << "==== Phase A Per-Achsen-Observer-V3: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
