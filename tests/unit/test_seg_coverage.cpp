// P-MD3 (2026-06-18) — SEGMENT-COVERAGE-VERSÖHNUNG. Belegt LITERAL, dass die Per-Achsen-Segment-Zeit (seg_ns[19])
// sich gegen einen KOMMENSURABLEN Nenner (die eigene Wall-Clock des Segment-Mess-Laufs, seg_run_total_ns) zu
// nahe 100% versöhnen lässt — mit einem EXPLIZIT benannten Rest (seg_framework_ns = Loop-/Instrumentierungs-Overhead).
//
// BEFUND-Kontext (FEEDBACK_IMPL-AGENT §P-MD3): in der M3-Matrix war sum(seg_*_ns)/total_ns ~33,6%. URSACHE = der
// Nenner total_ns ist die Wall-Clock eines GANZ ANDEREN Laufs (run_observable_perm: tier_insert/tier_lookup über die
// Pflicht-std::map-ABI), während seg_ns aus dem separaten Pfad-B-Segment-Lauf (fill_segment_timing_v3) stammt. Diese
// zwei Größen sind INKOMMENSURABEL (verschiedene Op-Zahl, -Mix, -Codepfad) → ihre Quote ist bedeutungslos.
//
// FIX (additiv): fill_segment_timing_v3 erhebt jetzt eine ÄUSSERE Wall-Clock um die gemessenen Batches
// (seg_run_total_ns) und weist seg_framework_ns = seg_run_total_ns − Σseg_ns aus. Dieser Test prüft:
//   (1) für mehrere reale Tiere: Σseg_ns + seg_framework_ns ≡ seg_run_total_ns (exakte Identität, by-construction);
//   (2) Coverage = Σseg_ns / seg_run_total_ns > 0.90 ODER der benannte Rest deckt den verbleibenden Anteil
//       (Coverage + framework_anteil ≡ 1.0) — die ABNAHME-Bedingung von P-MD3;
//   (3) die latenz-dominanten Store-Organe (node_type T4 / memory_layout T5) tragen bei store-backed AdHoc plausible,
//       NICHT verschwindende Anteile (> 0); Reference-Huellen tragen diese Store-Achsen seit #188-4c-i honest-0;
//   (4) das alte irreführende Verhältnis sum(seg)/total_ns wird zum Vergleich AUSGEGEBEN (Diagnose der ~33%).
// Alle Zahlen werden LITERAL ausgegeben (keine Erfolgsmarke ohne Ausgabe).
//
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/composition_factory.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/experiment_tree/perm_runner.hpp>                   // run_observable_perm (treibt den Mess-Pfad)
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp> // format_csv_row / LazyMeasuredRow

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>
#include <topics/traversal/axis_03a_search_algo/axis_03a_search_algo_array256.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace an    = ::comdare::cache_engine::anatomy;
namespace comp  = ::comdare::cache_engine::compositions;
namespace ce03a = ::comdare::cache_engine::traversal::axis_03a_search_algo;
namespace ex    = ::comdare::cache_engine::builder::experiment;

using StoreBackedAdHocComposition = an::AdHocComposition<
    ce03a::Array256SearchAlgo, comp::ArtComposition::cache_traversal, comp::ArtComposition::mapping,
    comp::ArtComposition::path_compression, comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
    comp::ArtComposition::allocator, comp::ArtComposition::prefetch, comp::ArtComposition::concurrency,
    comp::ArtComposition::serialization, comp::ArtComposition::telemetry, comp::ArtComposition::value_handle,
    comp::ArtComposition::isa, comp::ArtComposition::index_organization, comp::ArtComposition::io_dispatch,
    comp::ArtComposition::migration_policy, comp::ArtComposition::filter, comp::ArtComposition::queuing_q1,
    comp::ArtComposition::queuing_q2>;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

template <class C>
static void check_coverage(char const* name, bool store_axes_backed) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto* obs = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&tier));
    if (!obs) {
        tr(std::string(name) + ": IObservableTier via dynamic_cast vorhanden", false);
        return;
    }

    // Der reale Host-Mess-Pfad: tier_clear → n_ops insert + n_ops lookup (total_ns) → EIN konsolidierter Observer-POD
    // (axis_stats + seg_ns[19] + seg_framework_ns + seg_run_total_ns).
    ex::PermResult const                   pr = ex::run_observable_perm(*obs, name, /*n_ops=*/4000);
    an::ComdareTierObserverSnapshot const& s  = pr.unified;
    tr(std::string(name) + ": unified observer real", pr.unified_real);

    std::int64_t seg_sum = 0;
    for (int t = 0; t < 19; ++t) seg_sum += s.seg_ns[t];

    std::int64_t const run_total = s.seg_run_total_ns;
    std::int64_t const framework = s.seg_framework_ns;

    // (1) exakte Identität: Σseg_ns + framework ≡ run_total (by-construction, klemmt nur theoretischen Jitter auf 0).
    bool const identity = (run_total > 0) && (seg_sum + framework == run_total);
    tr(std::string(name) + ": Σseg_ns + seg_framework_ns == seg_run_total_ns (exakte Identität)", identity);

    double const cov_own  = (run_total > 0) ? (static_cast<double>(seg_sum) / static_cast<double>(run_total)) : 0.0;
    double const fw_share = (run_total > 0) ? (static_cast<double>(framework) / static_cast<double>(run_total)) : 0.0;
    double const cov_total =
        (pr.total_ns > 0) ? (static_cast<double>(seg_sum) / static_cast<double>(pr.total_ns)) : 0.0;

    // LITERAL-Ausgabe: kommensurable Coverage vs. das alte irreführende sum(seg)/total_ns.
    std::cout << "    " << name << "  seg_sum=" << seg_sum << "  seg_run_total_ns=" << run_total
              << "  seg_framework_ns=" << framework << "\n"
              << "      coverage(seg_sum/seg_run_total)=" << cov_own << "  framework_share=" << fw_share
              << "  (coverage+framework=" << (cov_own + fw_share) << ")\n"
              << "      [Diagnose alt] sum(seg)/total_ns=" << cov_total
              << "  (inkommensurabel: total_ns=" << pr.total_ns << " ist ein ANDERER Lauf)\n"
              << "      Top-Organe: seg[T0 search_algo]=" << s.seg_ns[0] << "  seg[T4 node_type]=" << s.seg_ns[4]
              << "  seg[T5 memory_layout]=" << s.seg_ns[5] << "  seg[T2 mapping]=" << s.seg_ns[2]
              << "  seg[T1 cache_traversal]=" << s.seg_ns[1] << "\n";

    // (2) ABNAHME (echter Regressions-Waechter): die Identitaet MUSS gelten (Rest exakt benannt, kein verlorener Anteil)
    //     UND die Coverage MUSS > 0.90 sein. (Frueher war der OR-Zweig 'cov+fw>=0.999' by-construction immer wahr und
    //     haette auch coverage=5% gruen gemeldet — jetzt ein scharfer Schwellwert, der eine echte Coverage-Regression faengt.)
    bool const accept = identity && (cov_own > 0.90);
    tr(std::string(name) + ": Identitaet (Rest benannt) UND Coverage > 0.90 (P-MD3-Abnahme, scharf)", accept);

    // (3) die latenz-dominanten Organe tragen plausible, NICHT-verschwindende Zeit, sofern ein Store existiert.
    tr(std::string(name) + ": seg[T0 search_algo] > 0 (algorithmische Organ-Zeit, nicht 0)", s.seg_ns[0] > 0);
    if (store_axes_backed) {
        tr(std::string(name) + ": seg[T4 node_type] > 0 (store-backed algorithmische Organ-Zeit)", s.seg_ns[4] > 0);
        tr(std::string(name) + ": seg[T5 memory_layout] > 0 (store-backed algorithmische Organ-Zeit)", s.seg_ns[5] > 0);
    } else {
        // #188-4c-i: Huellen-Komposition container_-authoritativ -> store-Achsen honest-0 (bis observe-Hooks #234).
        tr(std::string(name) + ": seg[T4 node_type] == 0 (Huellen honest-0)", s.seg_ns[4] == 0);
        tr(std::string(name) + ": seg[T5 memory_layout] == 0 (Huellen honest-0)", s.seg_ns[5] == 0);
        tr(std::string(name) + ": seg[T6 allocator] == 0 (Huellen honest-0)", s.seg_ns[6] == 0);
    }
}

int main() {
    std::cout << "==== P-MD3 Segment-Coverage-Versöhnung (seg_sum / seg_run_total_ns) ====\n";
    check_coverage<StoreBackedAdHocComposition>("AdHocArray256StoreBacked", true);
    check_coverage<comp::ArtComposition>("Art", false);
    check_coverage<comp::HotComposition>("Hot", false);
    check_coverage<comp::MasstreeComposition>("Masstree", false);

    if (g_fail == 0)
        std::cout << "==== P-MD3 Segment-Coverage: ALLE OK ====\n";
    else
        std::cout << "==== P-MD3 Segment-Coverage: " << g_fail << " FEHLER ====\n";
    return g_fail == 0 ? 0 : 1;
}
