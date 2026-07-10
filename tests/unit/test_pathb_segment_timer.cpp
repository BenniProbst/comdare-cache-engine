// Plan v2 Schritt 2/3 (2026-06-04) / I1 (2026-06-05): BUILD-VERIFIKATION des Pfad-B Per-Achsen-TIMINGS über die
// EINE konsolidierte tier_observe(ComdareTierObserverSnapshot*) (axis_stats[19][8] + seg_ns[19]) über die EINE
// REALE, befüllte composite-Tier-Struktur (container_algorithm_ + Instanz-Organe via layout-honorierender Store).
// KEIN synthetischer Puffer. Prüft literal: (1) alle 19 seg_ns > 0 (jede Achse real getrieben); (2) Korrelation
// Observer>0 ⇒ seg_ns>0 (Observer + Timer aus DERSELBEN realen Struktur, EIN POD); (3) DATA-Neutralität (tier_size
// + lookup unverändert nach dem Observe → memento-sicher); (4) Tier-Variation (Art/Hot/Masstree differieren in ≥1 Achse).
// Build: cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1 + ADHOC-Include-Satz.

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Liefert den konsolidierten Observer-POD (axis_stats + Pfad-B-seg_ns) nach kN realen Inserts.
template <class C>
static an::ComdareTierObserverSnapshot measure(char const* name) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto* drv = dynamic_cast<an::IObservableTier*>(base); // I1: EINE Schnittstelle = Antrieb + Observer + Pfad-B-Timing
    an::ComdareTierObserverSnapshot u{};
    if (!drv) {
        tr(std::string(name) + ": IObservableTier via dynamic_cast vorhanden", false);
        return u;
    }

    constexpr std::uint64_t kN = 16384; // reale Records (Test-Maß; Produktions-Working-Set N=131072 im Lauf)
    for (std::uint64_t i = 0; i < kN; ++i) (void)drv->tier_insert(i, i * 7u + 1u);

    std::uint64_t const size_before = drv->tier_size();
    std::uint64_t       lv_before   = 0;
    bool const          hit_before  = drv->tier_lookup(1234, &lv_before);

    // I1: die EINE tier_observe — Q1-Sequenz liest axis_stats VOR dem Pfad-B-Timing (seg_ns), beide in EINEM POD.
    drv->tier_observe(&u);

    // (3) DATA-Neutralität: tier_size + lookup-Ergebnis unverändert (Observe/Timing mutiert die Daten NICHT).
    std::uint64_t const size_after = drv->tier_size();
    std::uint64_t       lv_after   = 0;
    bool const          hit_after  = drv->tier_lookup(1234, &lv_after);
    tr(std::string(name) + ": DATA-Neutralität (tier_size + lookup unverändert)",
       size_after == size_before && size_after == kN && hit_before && hit_after && lv_after == lv_before &&
           lv_before == 1234u * 7u + 1u);

    // (1) alle 19 seg_ns > 0.
    bool all_pos = true;
    int  zero_t  = -1;
    for (int t = 0; t < 19; ++t)
        if (u.seg_ns[t] <= 0) {
            all_pos = false;
            if (zero_t < 0) zero_t = t;
        }
    tr(std::string(name) + ": alle 19 seg_ns > 0" + (all_pos ? "" : (" (erste 0 bei T" + std::to_string(zero_t) + ")")),
       all_pos);

    // (2) Korrelation: Achse mit Observer-Wert > 0 ⇒ seg_ns > 0 (gleiche reale Struktur).
    bool corr = true;
    for (int t = 0; t < 19; ++t) {
        std::uint64_t rs = 0;
        for (int f = 0; f < 8; ++f) rs += u.axis_stats[t][f];
        if (rs > 0 && u.seg_ns[t] <= 0) corr = false;
    }
    tr(std::string(name) + ": Korrelation (Observer>0 ⇒ seg_ns>0)", corr);

    std::int64_t total = 0;
    for (int t = 0; t < 19; ++t) total += u.seg_ns[t];
    std::cout << "    " << name << " seg_ns[T0,T4,T5,T7,T16] = " << u.seg_ns[0] << "," << u.seg_ns[4] << ","
              << u.seg_ns[5] << "," << u.seg_ns[7] << "," << u.seg_ns[16] << "  total=" << total << "\n";
    return u;
}

// I1: zwei identisch befüllte Instanzen → der EINE tier_observe ist deterministisch (Q1: axis_stats werden VOR
// dem Timing gelesen → der Timing-Pass inflationiert die Observer-Zähler NICHT) und liefert axis_stats + seg_ns
// in EINEM POD.
template <class C>
static void measure_unified(char const* name) {
    using Anatomy                             = an::SearchAlgorithmAnatomy<C>;
    constexpr std::uint64_t                kN = 16384;
    an::SearchAlgorithmAbiAdapter<Anatomy> a, b;
    auto*                                  ad = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&a));
    auto*                                  bd = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&b));
    for (std::uint64_t i = 0; i < kN; ++i) {
        (void)ad->tier_insert(i, i * 7u + 1u);
        (void)bd->tier_insert(i, i * 7u + 1u);
    }
    an::ComdareTierObserverSnapshot sa{}, sb{};
    ad->tier_observe(&sa);
    bd->tier_observe(&sb);
    // axis_stats deterministisch über zwei identische Läufe (Q1: kein Timing-Inflate); Meta identisch.
    bool stats_eq = (sa.observable_axis_count == sb.observable_axis_count && sa.tier_fill_level == sb.tier_fill_level &&
                     sa.filled_axis_count == sb.filled_axis_count);
    for (int t = 0; t < 19 && stats_eq; ++t)
        for (int f = 0; f < 8; ++f)
            if (sa.axis_stats[t][f] != sb.axis_stats[t][f]) {
                stats_eq = false;
                break;
            }
    tr(std::string(name) + ": unified axis_stats deterministisch (Q1: kein Timing-Inflate)", stats_eq);
    bool seg_pos = true;
    for (int t = 0; t < 19; ++t)
        if (sa.seg_ns[t] <= 0) seg_pos = false;
    tr(std::string(name) + ": unified seg_ns[0..18] alle > 0 (Pfad-B-Timing im EINEN POD)", seg_pos);
}

int main() {
    std::cout << "==== Pfad-B Per-Achsen-Timer (konsolidierte tier_observe) ====\n";
    auto a = measure<comp::ArtComposition>("Art");
    auto h = measure<comp::HotComposition>("Hot");
    auto m = measure<comp::MasstreeComposition>("Masstree");

    std::cout << "---- KONSOLIDIERUNG I1: EINE tier_observe(ComdareTierObserverSnapshot) ----\n";
    measure_unified<comp::ArtComposition>("Art");
    measure_unified<comp::HotComposition>("Hot");
    measure_unified<comp::MasstreeComposition>("Masstree");

    // (4) Tier-Variation: mind. EINE Achse differiert über die 3 Tiere (sonst misst der Timer nichts Tier-Spezifisches).
    bool varied = false;
    for (int t = 0; t < 19; ++t)
        if (!(a.seg_ns[t] == h.seg_ns[t] && h.seg_ns[t] == m.seg_ns[t])) {
            varied = true;
            break;
        }
    tr("Tier-Variation (Art/Hot/Masstree differieren in >=1 Achse)", varied);

    if (g_fail == 0) std::cout << "==== Pfad-B Per-Achsen-Timer: ALLE OK ====\n";
    return g_fail == 0 ? 0 : 1;
}
