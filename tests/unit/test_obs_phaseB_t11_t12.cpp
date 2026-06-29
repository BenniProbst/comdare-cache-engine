// Phase B (2026-06-04) BUILD-VERIFIKATION der Per-Achsen-Observer T11 value_handle + T12 isa:
//   IObservableTier::tier_observe → ComdareTierObserverSnapshot.axis_stats[11]/[12] über mehrere reale
//   SA-Kompositionen. Treibt den Tier (tier_clear → insert/lookup) und prüft, dass die per-Pfad-B store-slot-
//   gescannten Observer-Hüllen (vh_organ_/isa_organ_) NICHT-leere Statistik liefern:
//     (1) T11 value_handle row_sum > 0 (total_access_count + indirect_deref_count + ...);
//     (2) T12 isa row_sum > 0 (simd_calls + elements_processed + ...);
//     (3) T11/T12 Schema-Felder sind benannt (kV3AxisSchema, single-source);
//     (4) value_handle: indirect_deref_count strategie-charakteristisch (Inline=0, sonst >0 erwartet je nach
//         Komposition — hier nur EHRLICH: >= 0, da Inline-Tiere legitim 0 Indirektion haben).
// ISOLIERT von den anderen Phase-B-Achsen (prüft NUR T11/T12) → unabhängig vom Stand paralleler Achsen-Agenten.
//
// Build: build/scratch_compile_obs_phaseB_t11_t12.ps1 (cl /std:c++latest /EHsc /DCOMDARE_MEASUREMENT_ON=1
//        /DCOMDARE_CE_ENABLE_STATISTICS=1 + voller ADHOC-Include-Satz).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>
#include <compositions/wormhole_reference.hpp>

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

static std::uint64_t row_sum(an::ComdareTierObserverSnapshot const& s, int t) {
    std::uint64_t v = 0;
    for (std::size_t f = 0; f < an::kV3FieldCount; ++f) v += s.axis_stats[t][f];
    return v;
}

template <class C>
static void check_one(char const* name) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    an::SearchAlgorithmAbiAdapter<Anatomy> tier;
    auto*                                  base = static_cast<an::IAnatomyBase*>(&tier);
    auto*                                  drv  = dynamic_cast<an::IDriveableTier*>(base);
    auto* obs = dynamic_cast<an::IObservableTier*>(base); // der echte Host-Abfrage-Pfad

    if (drv == nullptr || obs == nullptr) {
        tr(std::string{name} + ": IDriveableTier/IObservableTier verfügbar", false);
        return;
    }

    // Tier treiben: clear → 1000 insert (füllt den container_-Slot-Backing) + 1000 lookup.
    drv->tier_clear();
    for (std::uint64_t k = 0; k < 1000; ++k) (void)drv->tier_insert(k, k * 7u + 1u);
    std::uint64_t out_v = 0;
    for (std::uint64_t k = 0; k < 1000; ++k) (void)drv->tier_lookup(k, &out_v);

    an::ComdareTierObserverSnapshot v3{};
    obs->tier_observe(&v3);

    std::uint64_t const vh  = row_sum(v3, 11);
    std::uint64_t const isa = row_sum(v3, 12);
    std::cout << "  " << name << ": T11 value_handle row_sum=" << vh << " (access=" << v3.axis_stats[11][0]
              << " indirect=" << v3.axis_stats[11][1] << " vtag=" << v3.axis_stats[11][2]
              << " depth=" << v3.axis_stats[11][3] << ")"
              << "  T12 isa row_sum=" << isa << " (calls=" << v3.axis_stats[12][0] << " elems=" << v3.axis_stats[12][1]
              << " simd_iter=" << v3.axis_stats[12][2] << " scalar=" << v3.axis_stats[12][3]
              << " checksum=" << v3.axis_stats[12][4] << ")\n";

    tr(std::string{name} + ": T11 value_handle row_sum > 0", vh > 0);
    tr(std::string{name} + ": T11 total_access_count > 0", v3.axis_stats[11][0] > 0);
    tr(std::string{name} + ": T11 peak_chain_depth >= 1", v3.axis_stats[11][3] >= 1);
    tr(std::string{name} + ": T12 isa row_sum > 0", isa > 0);
    tr(std::string{name} + ": T12 simd_calls > 0", v3.axis_stats[12][0] > 0);
    tr(std::string{name} + ": T12 elements_processed > 0", v3.axis_stats[12][1] > 0);
}

int main() {
    std::cout << "==== Phase B: Per-Achsen-Observer T11 value_handle + T12 isa (axis_stats[11]/[12]) ====\n";

    // Schema-Sanity (single-source): T11/T12 Zeilen müssen benannt sein.
    tr("Schema: T11 value_handle benannt", an::kV3AxisSchema[11].names[0] != nullptr);
    tr("Schema: T12 isa benannt", an::kV3AxisSchema[12].names[0] != nullptr);

    check_one<comp::ArtComposition>("ArtComposition");
    check_one<comp::HotComposition>("HotComposition");
    check_one<comp::MasstreeComposition>("MasstreeComposition");
    check_one<comp::WormholeComposition>("WormholeComposition");

    std::cout << "==== Phase B T11/T12: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
