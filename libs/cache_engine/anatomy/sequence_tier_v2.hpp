#pragma once
// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- die PER-ACHSEN-Wire-Form der SEQUENCE-Gattungs-
// Ebene: SequenceObserverAggregate<9> + ihr EIGENES Abfrage-Sub-Interface ISequenceTierV2.
//
// APPEND-ONLY (Auflage 5): sequence_tier.hpp bleibt unberuehrt -- SequenceObserverSnapshotV1
// byte-identisch, ISequenceTier vtable-gleich. ISequenceTierV2 ist EIGENSTAENDIG (erbt ISequenceTier
// NICHT) und wird vom SequenceAbiAdapter ZUSAETZLICH geerbt; Host-Abfrage 1x kalt per dynamic_cast.
//
// E1-DETAIL-ENTSCHEID (Lead, hiermit vollzogen; Anschluss an den Manager-Entscheid E1 LEDGER:3828):
// growth_factor_milli wird FELD der C6-Wire-Form -- Spalte [7] der growth_policy-Zeile.
//   WARUM UEBERHAUPT: growth_factor() ist die EINZIGE double-Groesse der Achse growth_policy und
//   zugleich ihr IDENTIFIZIERENDES Merkmal. Ohne sie steht auf dem Draht zwar die Wirkung des
//   Algorithmus (growth_events/elements_copied/peak_slack), aber nicht, WELCHER Algorithmus sie
//   erzeugt hat -- und der Owner-KERN NACHTRAG 3 (a) macht genau den ALGORITHMUS zum Mess-Objekt
//   (LEDGER:3814/:3815: "Auswertung der Qualitaet eines bestimmten Algorithmus ueber eine Achse").
//   Die Wirkung ohne ihren Erzeuger einzufrieren waere die stille Luecke.
//   WARUM NICHT IM POD: der in-process-Satz GrowthStatistics bleibt bei seinen 7 Feldern
//   (growth_policy_observable.hpp:42-52, Katalog-Mindest-Feldsatz C-A) -- growth_factor_milli ist
//   KEIN Zaehler, sondern eine EIGENSCHAFT der eingestellten Policy. Sie gehoert deshalb nicht in
//   die Zaehler-Struktur, sondern auf die Projektions-Ebene: der Einsammler holt sie direkt von der
//   HUELLE (ObservableGrowth::growth_factor_milli(), :102-105) und legt sie in die freie Spalte [7].
//   Der Zeilen-Schreiber laesst genau diese Spalte dafuer frei (genus_axis_row_writer.hpp).
//   FORM: E1-Milli-Fixpunkt (x1000). FixedChunkGrowth meldet 0.0 als additiven Sentinel -> 0; das ist
//   der ehrliche Wert "diese Policy hat keinen multiplikativen Faktor", keine fehlende Messung.
//   GATING: `if constexpr` -- eine NACKTE Policy ohne die Huelle hat den Member nicht, dann bleibt
//   Spalte [7] bei 0 UND die Zeile wird ueber den Schreiber-Rueckgabewert korrekt eingeordnet.

#include "anatomy_base.hpp"             // AnatomyGenus
#include "genus_axis_row_writer.hpp"    // write_genus_axis_row + E1-Milli
#include "genus_observer_aggregate.hpp" // GenusObserverAggregate<G, N>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// SequenceObserverAggregate<N> -- die per-Achsen-Wire-Form der Sequence-Gattungs-Ebene.
template <std::size_t N>
using SequenceObserverAggregate = GenusObserverAggregate<AnatomyGenus::Sequence, N>;

/// LIVE Slot-Zahl der Sequence-Ebene (== SequenceComposition::slot_count == 9, INC-2d).
inline constexpr std::size_t kSequenceGenusSlotCount = 9;

/// Die konkrete Wire-Form.
using SequenceObserverAggregateWire = SequenceObserverAggregate<kSequenceGenusSlotCount>;

static_assert(std::is_standard_layout_v<SequenceObserverAggregateWire>,
              "ABI-Pflicht: die Sequence-Gattungs-Wire-Form muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<SequenceObserverAggregateWire>,
              "ABI-Pflicht: die Sequence-Gattungs-Wire-Form muss memcpy-faehig sein");
static_assert(sizeof(SequenceObserverAggregateWire) == 720,
              "ABI-Bruch: sizeof(SequenceObserverAggregate<9>) != 720 -- Layout-Aenderung erfordert einen "
              "koordinierten ABI-Major-Bump (anatomy_module_abi_v1_decl.hpp)");

/// Der Spalten-Index, in dem growth_factor_milli auf dem Draht steht (E1-Detail-Entscheid, s. Kopf).
/// Benannte Konstante statt Literal: der Einsammler und die Schema-Zeile beziehen sich auf DENSELBEN Index.
inline constexpr std::size_t kSequenceGrowthSlot             = 8; // Slot-Index der Achse growth_policy
inline constexpr std::size_t kSequenceGrowthFactorMilliField = 7; // die vom Zeilen-Schreiber frei gelassene Spalte

/// kSequenceAxisSchema -- Reihenfolge == GenusBindingTraits<Sequence>::axis_names()
/// (memory_layout, allocator, prefetch, concurrency, serialization, value_handle, io_dispatch,
/// migration_policy, growth_policy).
inline constexpr GenusAxisFieldNames kSequenceAxisSchema[kSequenceGenusSlotCount] = {
    /* 0 memory_layout    */ {},
    /* 1 allocator        */
    {{"bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt", "fail", "external_frag_milli", "internal_frag_milli",
      nullptr}},
    /* 2 prefetch         */ {},
    /* 3 concurrency      */ {},
    /* 4 serialization    */ {},
    /* 5 value_handle     */ {},
    /* 6 io_dispatch      */ {},
    /* 7 migration_policy */ {},
    /* 8 growth_policy    */
    // Spalten 0..6 == GrowthStatistics (Katalog-Mindest-Feldsatz C-A); Spalte 7 == der E1-Detail-Entscheid.
    {{"growth_events", "elements_copied", "bytes_copied", "requested_total", "granted_total", "final_capacity",
      "peak_slack", "growth_factor_milli"}},
};

/// Die Sequence-Ebene hat am Ist KEINEN flachen Gattungs-Zaehler ausserhalb des V1-PODs. Alle vier
/// genus_stats-Spalten bleiben unbenannt -- und unbenannt heisst hier ausdruecklich "kein Wert", nicht "0".
inline constexpr GenusStatFieldNames kSequenceGenusStatSchema = {};

inline constexpr std::size_t kSequenceSchemaFilledAxisCount = genus_count_filled_axes(kSequenceAxisSchema);

/// ISequenceTierV2 -- das EIGENE, rein beobachtende Abfrage-Sub-Interface der Sequence-Gattungs-Ebene.
class ISequenceTierV2 {
public:
    virtual ~ISequenceTierV2() = default;

    /// Schreibt die per-Achsen-Wire-Form nach *out. out != nullptr. noexcept (reines Auslesen).
    virtual void tier_observe_sequence_axes(SequenceObserverAggregateWire* out) const noexcept = 0;
};

/// fill_sequence_observer_aggregate() -- flache, imperative Slot-fuer-Slot-Einsammlung.
template <class Anatomy>
constexpr void fill_sequence_observer_aggregate(Anatomy const& a, SequenceObserverAggregateWire& out) noexcept {
    static_assert(Anatomy::composition_t::slot_count == kSequenceGenusSlotCount,
                  "C6: die Sequence-Wire-Kante und die LIVE Kompositions-Slot-Zahl muessen zusammenfallen");

    auto const  ax     = a.observe_axes();
    std::size_t filled = 0;
    if (write_genus_axis_row(out.axis_stats[0], ax.memory_layout) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[1], ax.allocator) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[2], ax.prefetch) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[3], ax.concurrency) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[4], ax.serialization) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[5], ax.value_handle) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[6], ax.io_dispatch) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[7], ax.migration_policy) > 0) ++filled;
    bool const growth_written = write_genus_axis_row(out.axis_stats[kSequenceGrowthSlot], ax.growth_policy) > 0;
    if (growth_written) ++filled;

    // E1-DETAIL-ENTSCHEID (s. Kopf): der Faktor kommt von der HUELLE, nicht aus dem Zaehler-POD.
    // Traegt der Slot keine beobachtende Huelle, blendet `if constexpr` die Abfrage restlos aus und
    // Spalte [7] bleibt bei 0 -- kein synthetisierter Wert.
    if constexpr (requires(typename Anatomy::composition_t::growth_policy const& g) { g.growth_factor_milli(); }) {
        out.axis_stats[kSequenceGrowthSlot][kSequenceGrowthFactorMilliField] =
            a.growth_policy_organ().growth_factor_milli();
    }

    // E13: die Form TRAEGT das Per-Slot-Timing; erhoben wird es vom Dock-Treiber (Nach-C6-Fenster).
    // Bis dahin steht die EHRLICHE Marke "nicht erhoben" (negativ), nicht die Luege "0 ns".
    for (std::size_t t = 0; t < kSequenceGenusSlotCount; ++t) out.seg_ns[t] = -1;
    out.seg_framework_ns = -1;
    out.seg_run_total_ns = -1;

    out.observable_axis_count = Anatomy::observable_axis_count();
    out.total_slots           = Anatomy::composition_t::slot_count; // LIVE, nicht frozen
    out.filled_axis_count     = static_cast<std::uint64_t>(filled);
}

} // namespace comdare::cache_engine::anatomy
