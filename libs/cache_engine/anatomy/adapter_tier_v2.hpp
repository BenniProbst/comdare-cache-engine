#pragma once
// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- die PER-ACHSEN-Wire-Form der ADAPTER-Gattungs-
// Ebene: AdapterObserverAggregate<11> + ihr EIGENES Abfrage-Sub-Interface IAdapterTierV2.
//
// APPEND-ONLY (Auflage 5): adapter_tier.hpp bleibt unberuehrt -- AdapterObserverSnapshotV1
// byte-identisch, IAdapterTier vtable-gleich. IAdapterTierV2 ist EIGENSTAENDIG und wird vom
// AdapterAbiAdapter ZUSAETZLICH geerbt; Host-Abfrage 1x kalt per dynamic_cast.
//
// ADAPTER-SYMMETRIE (Katalog Sektion 2 Punkt 5) -- die beiden Ist-Asymmetrien der Adapter-Ebene, hier
// geschlossen:
//   (1) observable_axis_count: AdapterObserverSnapshotV1 ist der EINZIGE V1-POD ohne dieses Feld
//       (adapter_tier.hpp:26-39; Set/Sequence/View tragen es). Der V1-POD bleibt eingefroren -- die
//       Symmetrie stellt der gemeinsame Meta-Block der Wire-Form her (genus_observer_aggregate.hpp).
//   (2) underflow_count: pop auf LEER war bis C6-V unbeobachtet. C6-V hat die Meldung an die Achse
//       gebaut (adapter_anatomy.hpp, note_underflow); hier quert sie die Grenze -- in der
//       inner_container-Zeile, Spalte [6] (InnerContainerStatistics-Belegung, s. Schema unten).
//       Sie steht bewusst NICHT zusaetzlich in genus_stats: EIN Ereignis, EINE Spalte, keine
//       Doppel-Buchung.

#include "anatomy_base.hpp"             // AnatomyGenus
#include "genus_axis_row_writer.hpp"    // write_genus_axis_row + E1-Milli
#include "genus_observer_aggregate.hpp" // GenusObserverAggregate<G, N>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// AdapterObserverAggregate<N> -- die per-Achsen-Wire-Form der Adapter-Gattungs-Ebene.
template <std::size_t N>
using AdapterObserverAggregate = GenusObserverAggregate<AnatomyGenus::Adapter, N>;

/// LIVE Slot-Zahl der Adapter-Ebene (== AdapterComposition::slot_count == 11, INC-2d).
/// AUSDRUECKLICHER Negativ-Bezug (Katalog Sektion 2 Punkt 6): die frozen Legacy-Konstante
/// kAdapterCompositionSlotCount steht bei 13 und ist NICHT die Quelle dieser Kante -- die Kante kommt
/// aus der LIVE Komposition, und der Einsammler kreuzt beide compile-hart.
inline constexpr std::size_t kAdapterGenusSlotCount = 11;

/// Die konkrete Wire-Form.
using AdapterObserverAggregateWire = AdapterObserverAggregate<kAdapterGenusSlotCount>;

static_assert(std::is_standard_layout_v<AdapterObserverAggregateWire>,
              "ABI-Pflicht: die Adapter-Gattungs-Wire-Form muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<AdapterObserverAggregateWire>,
              "ABI-Pflicht: die Adapter-Gattungs-Wire-Form muss memcpy-faehig sein");
static_assert(sizeof(AdapterObserverAggregateWire) == 864,
              "ABI-Bruch: sizeof(AdapterObserverAggregate<11>) != 864 -- Layout-Aenderung erfordert einen "
              "koordinierten ABI-Major-Bump (anatomy_module_abi_v1_decl.hpp)");

/// Slot-Index der Achse inner_container (die EINE Adapter-eigene Achse, Paragraf 28 Invertebrate-Spalte).
inline constexpr std::size_t kAdapterInnerContainerSlot = 10;
/// Spalte, in der underflow_count auf dem Draht steht (Symmetrie-Punkt 2, s. Kopf).
inline constexpr std::size_t kAdapterUnderflowField = 6;

/// kAdapterAxisSchema -- Reihenfolge == GenusBindingTraits<Adapter>::axis_names()
/// (search_algo, cache_traversal, memory_layout, allocator, prefetch, concurrency, serialization,
/// value_handle, io_dispatch, migration_policy, inner_container).
inline constexpr GenusAxisFieldNames kAdapterAxisSchema[kAdapterGenusSlotCount] = {
    /*  0 search_algo      */ {},
    /*  1 cache_traversal  */ {},
    /*  2 memory_layout    */ {},
    /*  3 allocator        */
    {{"bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt", "fail", "external_frag_milli", "internal_frag_milli",
      nullptr}},
    /*  4 prefetch         */ {},
    /*  5 concurrency      */ {},
    /*  6 serialization    */ {},
    /*  7 value_handle     */ {},
    /*  8 io_dispatch      */ {},
    /*  9 migration_policy */ {},
    /* 10 inner_container  */
    // Vollbelegung aller acht Spalten (InnerContainerStatistics, Katalog-Mindest-Feldsatz C-E).
    // substrate_ops ist der UNION-Slot: je Substrat DISJUNKT belegt (VectorInner -> elements_shifted,
    // HeapInner -> sift_ops, DequeInner -> ehrliche 0 als O(1)-Beleg). Der Spaltenname bleibt neutral,
    // die Bedeutung steht am Substrat -- der Wert waere sonst zwischen den Substraten unvergleichbar
    // BENANNT statt unvergleichbar GEMESSEN.
    {{"push", "pop", "front_reads", "back_reads", "current_occupancy", "peak_occupancy", "underflow_count",
      "substrate_ops"}},
};

/// Die Adapter-Ebene hat am Ist keinen flachen Gattungs-Zaehler ausserhalb des V1-PODs und der
/// inner_container-Zeile. underflow_count steht bewusst NUR in der Achsen-Zeile (keine Doppel-Buchung).
inline constexpr GenusStatFieldNames kAdapterGenusStatSchema = {};

inline constexpr std::size_t kAdapterSchemaFilledAxisCount = genus_count_filled_axes(kAdapterAxisSchema);

/// IAdapterTierV2 -- das EIGENE, rein beobachtende Abfrage-Sub-Interface der Adapter-Gattungs-Ebene.
class IAdapterTierV2 {
public:
    virtual ~IAdapterTierV2() = default;

    /// Schreibt die per-Achsen-Wire-Form nach *out. out != nullptr. noexcept (reines Auslesen).
    virtual void tier_observe_container_axes(AdapterObserverAggregateWire* out) const noexcept = 0;
};

/// fill_adapter_observer_aggregate() -- flache, imperative Slot-fuer-Slot-Einsammlung.
template <class Anatomy>
constexpr void fill_adapter_observer_aggregate(Anatomy const& a, AdapterObserverAggregateWire& out) noexcept {
    static_assert(Anatomy::composition_t::slot_count == kAdapterGenusSlotCount,
                  "C6: die Adapter-Wire-Kante und die LIVE Kompositions-Slot-Zahl muessen zusammenfallen");

    auto const  ax     = a.observe_axes();
    std::size_t filled = 0;
    if (write_genus_axis_row(out.axis_stats[0], ax.search_algo) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[1], ax.cache_traversal) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[2], ax.memory_layout) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[3], ax.allocator) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[4], ax.prefetch) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[5], ax.concurrency) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[6], ax.serialization) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[7], ax.value_handle) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[8], ax.io_dispatch) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[9], ax.migration_policy) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[kAdapterInnerContainerSlot], ax.inner_container) > 0) ++filled;

    // E13: die Form TRAEGT das Per-Slot-Timing; erhoben wird es vom Dock-Treiber (Nach-C6-Fenster).
    for (std::size_t t = 0; t < kAdapterGenusSlotCount; ++t) out.seg_ns[t] = -1;
    out.seg_framework_ns = -1;
    out.seg_run_total_ns = -1;

    // SYMMETRIE-PUNKT 1: die Adapter-Ebene meldet observable_axis_count -- erstmals cross-boundary.
    out.observable_axis_count = Anatomy::observable_axis_count();
    out.total_slots           = Anatomy::composition_t::slot_count; // LIVE, nicht die frozen 13
    out.filled_axis_count     = static_cast<std::uint64_t>(filled);
}

} // namespace comdare::cache_engine::anatomy
