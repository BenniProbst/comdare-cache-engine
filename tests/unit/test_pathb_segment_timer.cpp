// Plan v2 Schritt 2/3 (2026-06-04): BUILD-VERIFIKATION des Pfad-B Per-Achsen-TIMINGS (IObservableTierV4::
// tier_observe_timed_v3 → ComdareSegmentLatencyV2.seg_ns[19]) über die EINE REALE, befüllte composite-Tier-
// Struktur (search_organ_ + container_.chunks_ via layout-honorierender Store). KEIN synthetischer Puffer.
// Prüft literal: (1) alle 19 seg_ns > 0 (jede Achse real getrieben); (2) Korrelation Observer>0 ⇒ seg_ns>0
// (Observer + Timer aus DERSELBEN realen Struktur); (3) DATA-Neutralität (tier_size + lookup unverändert nach
// dem Timing → memento-sicher); (4) Tier-Variation (Art/Hot/Masstree differieren in ≥1 Achse).
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

static int g_fail = 0;
static void tr(std::string const& w, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n"; if (!c) ++g_fail; }

template <class C>
static an::ComdareSegmentLatencyV2 measure(char const* name) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto* base = static_cast<an::IAnatomyBase*>(&tier);
    auto* drv  = dynamic_cast<an::IObservableTier*>(base);    // tier_insert/lookup/size (Gattungs-Antrieb)
    auto* v3   = dynamic_cast<an::IObservableTierV3*>(base);
    auto* v4   = dynamic_cast<an::IObservableTierV4*>(base);  // der echte Host-Abfrage-Pfad fürs Pfad-B-Timing
    an::ComdareSegmentLatencyV2 seg{};
    if (!drv || !v3 || !v4) { tr(std::string(name) + ": V3+V4 Sub-Interfaces via dynamic_cast vorhanden", false); return seg; }

    constexpr std::uint64_t kN = 16384;   // reale Records (Test-Maß; Produktions-Working-Set N=131072 im Lauf)
    for (std::uint64_t i = 0; i < kN; ++i) (void)drv->tier_insert(i, i * 7u + 1u);

    // V3-Observer VOR dem Timing (Produktions-Reihenfolge: V3 vor V4).
    an::ComdareTierObserverSnapshotV3 obs{};
    v3->tier_observe_v3(&obs);
    std::uint64_t const size_before = drv->tier_size();
    std::uint64_t lv_before = 0; bool const hit_before = drv->tier_lookup(1234, &lv_before);

    // V4: Pfad-B Per-Achsen-Timing über die schon befüllte reale Struktur.
    v4->tier_observe_timed_v3(&seg);

    // (3) DATA-Neutralität: tier_size + lookup-Ergebnis unverändert (Timing mutiert die Daten NICHT).
    std::uint64_t const size_after = drv->tier_size();
    std::uint64_t lv_after = 0; bool const hit_after = drv->tier_lookup(1234, &lv_after);
    tr(std::string(name) + ": DATA-Neutralität (tier_size + lookup unverändert)",
       size_after == size_before && size_after == kN && hit_before && hit_after && lv_after == lv_before && lv_before == 1234u * 7u + 1u);

    // (1) alle 19 seg_ns > 0.
    bool all_pos = true; int zero_t = -1;
    for (int t = 0; t < 19; ++t) if (seg.seg_ns[t] <= 0) { all_pos = false; if (zero_t < 0) zero_t = t; }
    tr(std::string(name) + ": alle 19 seg_ns > 0" + (all_pos ? "" : (" (erste 0 bei T" + std::to_string(zero_t) + ")")), all_pos);

    // (2) Korrelation: Achse mit Observer-Wert > 0 ⇒ seg_ns > 0 (gleiche reale Struktur).
    bool corr = true;
    for (int t = 0; t < 19; ++t) {
        std::uint64_t rs = 0; for (int f = 0; f < 8; ++f) rs += obs.axis_stats[t][f];
        if (rs > 0 && seg.seg_ns[t] <= 0) corr = false;
    }
    tr(std::string(name) + ": Korrelation (Observer>0 ⇒ seg_ns>0)", corr);

    std::cout << "    " << name << " seg_ns[T0,T4,T5,T7,T16] = "
              << seg.seg_ns[0] << "," << seg.seg_ns[4] << "," << seg.seg_ns[5] << ","
              << seg.seg_ns[7] << "," << seg.seg_ns[16] << "  total=" << seg.total_ns << "\n";
    return seg;
}

int main() {
    std::cout << "==== Pfad-B Per-Achsen-Timer (IObservableTierV4) ====\n";
    auto a = measure<comp::ArtComposition>("Art");
    auto h = measure<comp::HotComposition>("Hot");
    auto m = measure<comp::MasstreeComposition>("Masstree");

    // (4) Tier-Variation: mind. EINE Achse differiert über die 3 Tiere (sonst misst der Timer nichts Tier-Spezifisches).
    bool varied = false;
    for (int t = 0; t < 19; ++t) if (!(a.seg_ns[t] == h.seg_ns[t] && h.seg_ns[t] == m.seg_ns[t])) { varied = true; break; }
    tr("Tier-Variation (Art/Hot/Masstree differieren in >=1 Achse)", varied);

    if (g_fail == 0) std::cout << "==== Pfad-B Per-Achsen-Timer: ALLE OK ====\n";
    return g_fail == 0 ? 0 : 1;
}
