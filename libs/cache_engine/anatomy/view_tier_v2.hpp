#pragma once
// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- die PER-ACHSEN-Wire-Form der VIEW-Gattungs-Ebene:
// ViewObserverAggregate<5> + ihr EIGENES Abfrage-Sub-Interface IViewTierV2.
//
// APPEND-ONLY (Auflage 5): view_tier.hpp bleibt unberuehrt -- ViewObserverSnapshotV1 byte-identisch,
// IViewTier vtable-gleich (insbesondere bleibt die bewusste API-Asymmetrie "kein clear/insert/erase"
// bestehen; die View besitzt keinen Speicher). IViewTierV2 ist EIGENSTAENDIG und wird vom ViewAbiAdapter
// ZUSAETZLICH geerbt; Host-Abfrage 1x kalt per dynamic_cast.
//
// DIE VIEW IST DIE GATTUNG MIT DEN MEISTEN EIGENEN ACHSEN (drei von fuenf Slots: extent_policy,
// layout_policy, accessor_policy). Genau sie waren bis C6-V messtechnisch STUMM -- C6-V baute die drei
// Statistics-PODs, C6 hebt sie auf den Draht. Damit traegt diese Gattung als einzige DREI befuellte
// Achsen-Zeilen und ist der schaerfste Beleg dafuer, dass die Promotion inhaltlich etwas transportiert.
//
// ELISIONS-BEWEIS AUF DEM DRAHT (Katalog C-B): bounds_checks_performed == 0 bei STATISCHER Ausdehnung
// ist ein MESSERGEBNIS, nicht eine Luecke -- die Achse fuehrt dann keinen dynamischen Grenz-Check aus,
// weil die Grenze im Typ steht. Die Zeile ist trotzdem BEFUELLT (der Schreiber meldet 3 Spalten), und
// genau daran ist die ehrliche 0 von "kein Wert" unterscheidbar. Der Speicher-Sicherheits-Guard der
// Anatomie bleibt davon unberuehrt -- er ist kein Achsen-Ereignis.

#include "anatomy_base.hpp"             // AnatomyGenus
#include "genus_axis_row_writer.hpp"    // write_genus_axis_row + E1-Milli
#include "genus_observer_aggregate.hpp" // GenusObserverAggregate<G, N>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// ViewObserverAggregate<N> -- die per-Achsen-Wire-Form der View-Gattungs-Ebene.
template <std::size_t N>
using ViewObserverAggregate = GenusObserverAggregate<AnatomyGenus::View, N>;

/// LIVE Slot-Zahl der View-Ebene (== ViewComposition::slot_count == 5, INC-2d).
inline constexpr std::size_t kViewGenusSlotCount = 5;

/// Die konkrete Wire-Form.
using ViewObserverAggregateWire = ViewObserverAggregate<kViewGenusSlotCount>;

static_assert(std::is_standard_layout_v<ViewObserverAggregateWire>,
              "ABI-Pflicht: die View-Gattungs-Wire-Form muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ViewObserverAggregateWire>,
              "ABI-Pflicht: die View-Gattungs-Wire-Form muss memcpy-faehig sein");
static_assert(sizeof(ViewObserverAggregateWire) == 432,
              "ABI-Bruch: sizeof(ViewObserverAggregate<5>) != 432 -- Layout-Aenderung erfordert einen "
              "koordinierten ABI-Major-Bump (anatomy_module_abi_v1_decl.hpp)");

/// Slot-Indizes der drei View-eigenen Achsen (benannt statt als Literal verstreut).
inline constexpr std::size_t kViewExtentSlot   = 2;
inline constexpr std::size_t kViewLayoutSlot   = 3;
inline constexpr std::size_t kViewAccessorSlot = 4;

/// kViewAxisSchema -- Reihenfolge == GenusBindingTraits<View>::axis_names()
/// (memory_layout, value_handle, extent_policy, layout_policy, accessor_policy).
inline constexpr GenusAxisFieldNames kViewAxisSchema[kViewGenusSlotCount] = {
    /* 0 memory_layout   */ {},
    /* 1 value_handle    */ {},
    /* 2 extent_policy   */
    {{"bounds_checks_performed", "oob_rejects", "rebind_count", nullptr, nullptr, nullptr, nullptr, nullptr}},
    /* 3 layout_policy   */
    {{"index_translations", "non_contiguous_steps", "max_offset_jump", nullptr, nullptr, nullptr, nullptr, nullptr}},
    /* 4 accessor_policy */
    {{"access_count", "unaligned_accesses", "conversion_ops", nullptr, nullptr, nullptr, nullptr, nullptr}},
};

/// Die View-Ebene hat am Ist keinen flachen Gattungs-Zaehler ausserhalb des V1-PODs.
inline constexpr GenusStatFieldNames kViewGenusStatSchema = {};

inline constexpr std::size_t kViewSchemaFilledAxisCount = genus_count_filled_axes(kViewAxisSchema);

/// IViewTierV2 -- das EIGENE, rein beobachtende Abfrage-Sub-Interface der View-Gattungs-Ebene.
class IViewTierV2 {
public:
    virtual ~IViewTierV2() = default;

    /// Schreibt die per-Achsen-Wire-Form nach *out. out != nullptr. noexcept (reines Auslesen).
    virtual void tier_observe_view_axes(ViewObserverAggregateWire* out) const noexcept = 0;
};

/// fill_view_observer_aggregate() -- flache, imperative Slot-fuer-Slot-Einsammlung.
template <class Anatomy>
constexpr void fill_view_observer_aggregate(Anatomy const& a, ViewObserverAggregateWire& out) noexcept {
    static_assert(Anatomy::composition_t::slot_count == kViewGenusSlotCount,
                  "C6: die View-Wire-Kante und die LIVE Kompositions-Slot-Zahl muessen zusammenfallen");

    auto const  ax     = a.observe_axes();
    std::size_t filled = 0;
    if (write_genus_axis_row(out.axis_stats[0], ax.memory_layout) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[1], ax.value_handle) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[kViewExtentSlot], ax.extent_policy) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[kViewLayoutSlot], ax.layout_policy) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[kViewAccessorSlot], ax.accessor_policy) > 0) ++filled;

    // E13: die Form TRAEGT das Per-Slot-Timing; erhoben wird es vom Dock-Treiber (Nach-C6-Fenster).
    for (std::size_t t = 0; t < kViewGenusSlotCount; ++t) out.seg_ns[t] = -1;
    out.seg_framework_ns = -1;
    out.seg_run_total_ns = -1;

    out.observable_axis_count = Anatomy::observable_axis_count();
    out.total_slots           = Anatomy::composition_t::slot_count; // LIVE, nicht frozen
    out.filled_axis_count     = static_cast<std::uint64_t>(filled);
}

} // namespace comdare::cache_engine::anatomy
