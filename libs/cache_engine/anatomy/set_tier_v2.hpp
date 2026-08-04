#pragma once
// E-24 C6 (2026-08-04, b-Teil / ABI-EREIGNIS-SERIE) -- die PER-ACHSEN-Wire-Form der SET-Gattungs-Ebene:
// SetObserverAggregate<13> + ihr EIGENES Abfrage-Sub-Interface ISetTierV2.
//
// APPEND-ONLY (Auflage 5) -- was dieser Header NICHT tut:
//   * er fasst set_tier.hpp NICHT an. SetObserverSnapshotV1 bleibt BYTE-IDENTISCH eingefroren, ISetTier
//     behaelt seine vtable Methode fuer Methode. Ein Host, der nur V1 kennt, merkt von C6 nichts.
//   * er haengt NICHTS an ISetTier an. ISetTierV2 ist ein EIGENSTAENDIGES Sub-Interface (Praezedenz
//     IMigratableTier, observable_tier.hpp:214-222), das der genus-typisierte SetAbiAdapter ZUSAETZLICH
//     erbt. Der Host fragt es 1x KALT je Modul per dynamic_cast<ISetTierV2*> ab; eine Alt-DLL liefert
//     nullptr -> der Host degradiert auf die V1-Flaeche statt abzustuerzen.
//
// WAS DIE FORM NEU TRAEGT (und der eingefrorene V1-POD nicht mehr aufnehmen kann):
//   (1) die PER-ACHSEN-Sicht: 13 Zeilen a 8 Spalten. Bis C5 endete observe_axes() an der Modul-Grenze.
//   (2) E13-Per-Slot-Timing seg_ns[13] (LEDGER:3828, Owner-KERN Programm-Messbaeume).
//   (3) die SET-EHRLICHKEITS-SCHLIESSUNG insert_attempt_count / erase_attempt_count (Katalog Sektion 2
//       Punkt 4). C6-V hat sie IN-PROCESS geschlossen (set_anatomy.hpp:54-55); hier queren sie erstmals
//       die ABI-Grenze. OHNE sie bleiben Duplikat-Quote und Erase-Miss-Quote host-seitig dauerhaft
//       unbestimmbar -- und ein Freeze haette genau das zementiert.
//   (4) total_slots aus der LIVE Composition::slot_count (Katalog Punkt 6) + observable_axis_count.
//
// SCHEMA-EHRLICHKEIT (D2): kSetAxisSchema benennt GENAU die Zeilen, fuer die write_genus_axis_row()
// eine Belegung traegt. Am Ist ist das der allocator-Slot (AllocationStatistics inkl. der beiden
// E1-Milli-Spalten). Die uebrigen Zeilen bleiben NAMENLOS -- und namenlos heisst hier ausdruecklich
// "diese Achse liefert cross-boundary (noch) keinen Wert", nicht "der Wert ist 0". Der Beleg dafuer
// steht neben der Zahl: filled_axis_count zaehlt zur LAUFZEIT, wie viele Zeilen real geschrieben
// wurden, und observable_axis_count, wie viele Achsen ueberhaupt beobachtbar sind (R5.B-Grenze).
// Die Wire-Anbindung der uebrigen Achsen-Formen ist A8-S3-Gegenstand (Katalog Sektion 4).

#include "anatomy_base.hpp"             // AnatomyGenus
#include "genus_axis_row_writer.hpp"    // write_genus_axis_row + E1-Milli
#include "genus_observer_aggregate.hpp" // GenusObserverAggregate<G, N>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

/// SetObserverAggregate<N> -- die per-Achsen-Wire-Form der Set-Gattungs-Ebene. Alias auf das gemeinsame
/// Layout mit dem Genus-Tag Set: dieselbe Kante, aber ein ANDERER Typ als SequenceObserverAggregate<N>.
template <std::size_t N>
using SetObserverAggregate = GenusObserverAggregate<AnatomyGenus::Set, N>;

/// Die LIVE Slot-Zahl der Set-Gattungs-Ebene (== SetComposition::slot_count == GenusBindingTraits<Set>::
/// slot_count == 13, INC-2d). Als benannte Konstante, damit die Wire-Kante und die Kompositions-Kante
/// EINEN Namen teilen -- ein Slot-Schnitt bricht dann an einer Stelle, nicht an dreien verschieden.
inline constexpr std::size_t kSetGenusSlotCount = 13;

/// Die konkrete Wire-Form (die einzige, die je die ABI-Grenze quert).
using SetObserverAggregateWire = SetObserverAggregate<kSetGenusSlotCount>;

static_assert(std::is_standard_layout_v<SetObserverAggregateWire>,
              "ABI-Pflicht: die Set-Gattungs-Wire-Form muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<SetObserverAggregateWire>,
              "ABI-Pflicht: die Set-Gattungs-Wire-Form muss memcpy-faehig sein");
static_assert(sizeof(SetObserverAggregateWire) == 1008,
              "ABI-Bruch: sizeof(SetObserverAggregate<13>) != 1008 -- Layout-Aenderung erfordert einen "
              "koordinierten ABI-Major-Bump (anatomy_module_abi_v1_decl.hpp)");

/// kSetAxisSchema -- Spalten-Namen je Achsen-Slot, Reihenfolge == GenusBindingTraits<Set>::axis_names()
/// (search_algo, cache_traversal, path_compression, node_type, memory_layout, allocator, prefetch,
/// concurrency, serialization, index_organization, io_dispatch, migration_policy, filter).
/// SINGLE SOURCE fuer die CSV-Spaltennamen stat_set_<achse>_<feld>.
inline constexpr GenusAxisFieldNames kSetAxisSchema[kSetGenusSlotCount] = {
    /*  0 search_algo        */ {},
    /*  1 cache_traversal    */ {},
    /*  2 path_compression   */ {},
    /*  3 node_type          */ {},
    /*  4 memory_layout      */ {},
    /*  5 allocator          */
    {{"bytes_alloc", "bytes_in_use", "alloc_cnt", "dealloc_cnt", "fail", "external_frag_milli", "internal_frag_milli",
      nullptr}},
    /*  6 prefetch           */ {},
    /*  7 concurrency        */ {},
    /*  8 serialization      */ {},
    /*  9 index_organization */ {},
    /* 10 io_dispatch        */ {},
    /* 11 migration_policy   */ {},
    /* 12 filter             */ {},
};

/// kSetGenusStatSchema -- Spalten-Namen der flachen Gattungs-Zaehler (genus_stats).
/// Beide belegten Spalten sind die C6-Ehrlichkeits-Schliessung; die Quoten rechnet der Host:
///   Duplikat-Quote   = (insert_attempt - insert) / insert_attempt   [insert kommt aus V1]
///   Erase-Miss-Quote = (erase_attempt  - erase)  / erase_attempt
inline constexpr GenusStatFieldNames kSetGenusStatSchema = {
    {"insert_attempt_count", "erase_attempt_count", nullptr, nullptr}};

/// Zahl der Achsen-Zeilen mit benanntem Schema (abgeleitet, nie hartkodiert).
inline constexpr std::size_t kSetSchemaFilledAxisCount = genus_count_filled_axes(kSetAxisSchema);

/// ISetTierV2 -- das EIGENE Abfrage-Sub-Interface der Set-Gattungs-Ebene fuer die per-Achsen-Wire-Form.
///
/// EIGENSTAENDIG (erbt NICHT ISetTier): der Antrieb laeuft weiter ueber ISetTier, diese Flaeche ist
/// REIN BEOBACHTEND. Damit bleibt die V1-vtable unberuehrt und der Host kann beide Flaechen unabhaengig
/// voneinander abfragen (Praezedenz IMigratableTier).
class ISetTierV2 {
public:
    virtual ~ISetTierV2() = default;

    /// Schreibt die per-Achsen-Wire-Form nach *out. out != nullptr. noexcept (reines Auslesen).
    virtual void tier_observe_set_axes(SetObserverAggregateWire* out) const noexcept = 0;
};

/// fill_set_observer_aggregate() -- die EINSAMMLUNG. Flach und imperativ Slot fuer Slot
/// (Rekursions-Invariante, Owner-KERN NACHTRAG 3 (d), LEDGER:3815) -- kein rekursiver Abstieg.
///
/// TEMPLATE statt virtueller Methode, weil die Achsen-Typen composition-abhaengig sind: die Projektion
/// findet IM Modul-Binary statt, ueber die Grenze geht nur der flache POD.
template <class Anatomy>
constexpr void fill_set_observer_aggregate(Anatomy const& a, SetObserverAggregateWire& out) noexcept {
    static_assert(Anatomy::composition_t::slot_count == kSetGenusSlotCount,
                  "C6: die Set-Wire-Kante und die LIVE Kompositions-Slot-Zahl muessen zusammenfallen");

    auto const  ax     = a.observe_axes();
    std::size_t filled = 0;
    if (write_genus_axis_row(out.axis_stats[0], ax.search_algo) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[1], ax.cache_traversal) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[2], ax.path_compression) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[3], ax.node_type) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[4], ax.memory_layout) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[5], ax.allocator) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[6], ax.prefetch) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[7], ax.concurrency) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[8], ax.serialization) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[9], ax.index_organization) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[10], ax.io_dispatch) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[11], ax.migration_policy) > 0) ++filled;
    if (write_genus_axis_row(out.axis_stats[12], ax.filter) > 0) ++filled;

    // E13: die Form TRAEGT das Per-Slot-Timing; ERHOBEN wird es vom Dock-Treiber (Fuellstands-Sweep +
    // per-Op-Latenz, Katalog Sektion 2 Punkt 9 -- eigenes Nach-C6-Fenster). Bis dahin steht hier die
    // EHRLICHE Marke "nicht erhoben" (negativ), NICHT die Luege "0 ns". Das Nachruesten der Erhebung
    // ist dann KEIN Wire-Ereignis mehr -- genau dafuer steht das Feld schon jetzt im Layout.
    for (std::size_t t = 0; t < kSetGenusSlotCount; ++t) out.seg_ns[t] = -1;
    out.seg_framework_ns = -1;
    out.seg_run_total_ns = -1;

    // Die Ehrlichkeits-Schliessung quert die Grenze (Schema kSetGenusStatSchema).
    auto const flat    = a.observe_all();
    out.genus_stats[0] = flat.insert_attempt_count;
    out.genus_stats[1] = flat.erase_attempt_count;

    out.observable_axis_count = Anatomy::observable_axis_count();
    out.total_slots           = Anatomy::composition_t::slot_count; // LIVE, nicht frozen
    out.filled_axis_count     = static_cast<std::uint64_t>(filled);
}

} // namespace comdare::cache_engine::anatomy
