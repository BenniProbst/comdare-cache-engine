// test_e24_c6_set_wire_aggregate -- E-24 C6 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: SetObserverAggregate<13> + ISetTierV2 (libs/cache_engine/anatomy/set_tier_v2.hpp) und ihre
// Verdrahtung im SetAbiAdapter.
//
//   (A) APPEND-ONLY-BEWEIS -- SetObserverSnapshotV1 ist BYTE-IDENTISCH geblieben: sizeof + JEDER
//                             Feld-Offset einzeln gepinnt. Ein Alt-Leser liest nach C6 dieselben Bytes.
//   (B) EIGENE FLAECHE     -- die neue Sicht haengt NICHT an ISetTier: der Host holt sie 1x kalt per
//                             dynamic_cast<ISetTierV2*>. Eine Anatomie ohne die Flaeche liefert nullptr
//                             (sauberes Degradieren statt Absturz) -- an einer ECHTEN Fremd-Gattung
//                             gezeigt, nicht an einer Attrappe.
//   (C) EHRLICHKEIT        -- insert_attempt_count/erase_attempt_count queren erstmals die Grenze; die
//                             Duplikat-/Erase-Miss-Quote wird daraus host-seitig real ableitbar.
//   (D) LIVE-SLOTS         -- total_slots kommt aus Composition::slot_count, nicht aus einer Konstanten.
//   (E) D2 auf dem Draht   -- filled_axis_count zaehlt die REAL geschriebenen Zeilen; seg_ns traegt die
//                             ehrliche Marke "nicht erhoben" (negativ), nicht die Luege "0 ns".
//   (F) ACHSEN-ZEILE       -- ein Slot mit echter Achsen-Statistik landet real auf dem Draht (allocator,
//                             inkl. der beiden E1-Milli-Spalten).
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/set_abi_adapter.hpp"

#include "anatomy/sequence_abi_adapter.hpp" // Fremd-Gattung fuer die Degradier-Probe (echt, keine Attrappe)
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/set_default_organ.hpp" // SortedArrayKeySet: das echte search_algo-Kern-Organ
#include "anatomy/set_tier.hpp"          // SetObserverSnapshotV1 (der eingefrorene V1-POD)
#include "organ_axes/alloc/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;
namespace cac = comdare::cache_engine::alloc::concepts;

static int g_fail = 0;

template <class A, class B>
static void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

static void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

struct PlainAxis {};

/// Eine Allokator-Achse, die die ECHTE AllocationStatistics-Form liefert (inkl. der beiden double-Felder).
/// Fixture, aber die FORM ist die reale Achsen-Form -- die Spalten-Belegung wird also am echten Typ geprueft.
struct ZaehlenderAllokator {
    using snapshot_t = cac::AllocationStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return s_; }
    snapshot_t               s_{};
};

using SetComp =
    cea::SetComposition<cea::SortedArrayKeySet, PlainAxis, PlainAxis, PlainAxis, PlainAxis, ZaehlenderAllokator,
                        PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis>;
using SetOrgan   = cea::SetAnatomy<SetComp>;
using SetAdapter = cea::SetAbiAdapter<SetOrgan>;

/// Sequence-Gattung ohne ISetTierV2 -- die ECHTE Fremd-Flaeche fuer die Degradier-Probe.
using SeqComp    = cea::SequenceComposition<PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis, PlainAxis,
                                            PlainAxis, cea::DoublingGrowth>;
using SeqAdapter = cea::SequenceAbiAdapter<cea::SequenceAnatomy<SeqComp>>;

// =============================================================================================
// (A) APPEND-ONLY: der eingefrorene V1-POD ist BYTE-IDENTISCH geblieben.
// =============================================================================================

static_assert(sizeof(cea::SetObserverSnapshotV1) == 9 * 8, "V1-POD: 9 uint64-Felder, unveraendert");
static_assert(offsetof(cea::SetObserverSnapshotV1, insert_count) == 0);
static_assert(offsetof(cea::SetObserverSnapshotV1, contains_count) == 8);
static_assert(offsetof(cea::SetObserverSnapshotV1, contains_hit_count) == 16);
static_assert(offsetof(cea::SetObserverSnapshotV1, contains_miss_count) == 24);
static_assert(offsetof(cea::SetObserverSnapshotV1, erase_count) == 32);
static_assert(offsetof(cea::SetObserverSnapshotV1, current_size) == 40);
static_assert(offsetof(cea::SetObserverSnapshotV1, peak_size) == 48);
static_assert(offsetof(cea::SetObserverSnapshotV1, observable_axis_count) == 56);
static_assert(offsetof(cea::SetObserverSnapshotV1, organ_count) == 64);
static_assert(cea::kSetObserverSnapshotVersion == 1, "V1-Version-Konstante bleibt 1 (append-only)");

// Die NEUE Flaeche ist ein eigener Typ, KEINE Ableitung von ISetTier (sonst waere es ein vtable-Anhang).
static_assert(!std::is_base_of_v<cea::ISetTier, cea::ISetTierV2>);
static_assert(!std::is_base_of_v<cea::ISetTierV2, cea::ISetTier>);
// Der Adapter traegt beide Flaechen.
static_assert(std::is_base_of_v<cea::ISetTier, SetAdapter>);
static_assert(std::is_base_of_v<cea::ISetTierV2, SetAdapter>);
static_assert(std::is_base_of_v<cea::IAnatomyBase, SetAdapter>);

// Kante und LIVE Slot-Zahl fallen zusammen.
static_assert(cea::kSetGenusSlotCount == SetComp::slot_count);
static_assert(cea::SetObserverAggregateWire::slot_count == 13);
static_assert(cea::SetObserverAggregateWire::genus == cea::AnatomyGenus::Set);
static_assert(cea::kSetSchemaFilledAxisCount == 1, "am Ist traegt genau der allocator-Slot ein Schema");

} // namespace

int main() {
    std::cout << "=== E-24 C6 (c) -- SetObserverAggregate<13> + ISetTierV2 ===\n";

    SetAdapter adapter{};

    // Reale Set-Ops: 3 Inserts (davon 1 Duplikat) + 2 Erase-Versuche (davon 1 Treffer).
    check_true("insert(10) neu", adapter.tier_set_insert(10));
    check_true("insert(20) neu", adapter.tier_set_insert(20));
    check_true("insert(10) ist ein DUPLIKAT (kein neuer Key)", !adapter.tier_set_insert(10));
    check_true("erase(20) trifft", adapter.tier_set_erase(20));
    check_true("erase(99) ist ein MISS", !adapter.tier_set_erase(99));

    std::cout << "\n[A] Der eingefrorene V1-Pfad liefert unveraendert (Alt-Host-Sicht)\n";
    cea::SetObserverSnapshotV1 v1{};
    adapter.tier_observe_set(&v1);
    check_eq("v1.insert_count (nur ERFOLGREICHE)", v1.insert_count, std::uint64_t{2});
    check_eq("v1.erase_count (nur ERFOLGREICHE)", v1.erase_count, std::uint64_t{1});
    check_eq("v1.current_size", v1.current_size, std::uint64_t{1});
    check_eq("v1.organ_count", v1.organ_count, std::uint64_t{13});

    std::cout << "\n[B] Die NEUE Flaeche wird 1x kalt per dynamic_cast geholt\n";
    cea::IAnatomyBase* base = &adapter;
    auto*              v2   = dynamic_cast<cea::ISetTierV2*>(base);
    check_true("dynamic_cast<ISetTierV2*> auf der Set-Gattung trifft", v2 != nullptr);

    SeqAdapter         fremd{};
    cea::IAnatomyBase* fremd_base = &fremd;
    check_true("dieselbe Abfrage an der Sequence-Gattung liefert nullptr (sauberes Degradieren)",
               dynamic_cast<cea::ISetTierV2*>(fremd_base) == nullptr);
    check_true("die Sequence-Gattung traegt ihre EIGENE V1-Flaeche weiterhin",
               dynamic_cast<cea::ISequenceTier*>(fremd_base) != nullptr);

    if (v2 == nullptr) {
        std::cout << "\n=== FEHLER: ohne ISetTierV2 sind die Folge-Pruefungen gegenstandslos ===\n";
        return 1;
    }

    cea::SetObserverAggregateWire agg{};
    v2->tier_observe_set_axes(&agg);

    std::cout << "\n[C] Ehrlichkeits-Schliessung auf dem Draht (Katalog Sektion 2 Punkt 4)\n";
    check_eq("genus_stats[0] == insert_attempt_count", agg.genus_stats[0], std::uint64_t{3});
    check_eq("genus_stats[1] == erase_attempt_count", agg.genus_stats[1], std::uint64_t{2});
    // Erst DIESE Zahlen machen die Quoten bestimmbar -- vorher waren sie strukturell unerreichbar.
    check_eq("Duplikat-Versuche == attempt - erfolgreich", agg.genus_stats[0] - v1.insert_count, std::uint64_t{1});
    check_eq("Erase-Misses == attempt - erfolgreich", agg.genus_stats[1] - v1.erase_count, std::uint64_t{1});
    check_true("Schema benennt beide Spalten (kein unbenannter Wert auf dem Draht)",
               cea::kSetGenusStatSchema.names[0] != nullptr && cea::kSetGenusStatSchema.names[1] != nullptr);

    std::cout << "\n[D] total_slots kommt LIVE aus der Komposition\n";
    check_eq("agg.total_slots", agg.total_slots, std::uint64_t{13});
    check_eq("agg.total_slots == SetComp::slot_count", agg.total_slots, std::uint64_t{SetComp::slot_count});
    check_eq("agg.observable_axis_count (allocator ist die EINE beobachtbare Achse)", agg.observable_axis_count,
             std::uint64_t{1});

    std::cout << "\n[E] D2 auf dem Draht: gezaehlte Zeilen + ehrliche Timing-Marke\n";
    check_eq("agg.filled_axis_count (real geschriebene Zeilen)", agg.filled_axis_count, std::uint64_t{1});
    check_true("seg_ns[0] traegt 'nicht erhoben' (negativ), nicht die Luege 0", agg.seg_ns[0] < 0);
    check_true("seg_ns[12] ebenso", agg.seg_ns[12] < 0);
    check_true("seg_run_total_ns ebenso", agg.seg_run_total_ns < 0);

    std::cout << "\n[F] Die allocator-Zeile quert die Grenze -- inkl. der beiden E1-Milli-Spalten\n";
    // Der Achsen-Zustand wird ueber den REALEN Organ-Accessor der Anatomie gesetzt und dann ueber
    // GENAU DIESELBE Einsammlung projiziert, die der ABI-Adapter in tier_observe_set_axes() ruft
    // (fill_set_observer_aggregate) -- kein Nachbau, kein Zugriff auf die Kapselung des Adapters.
    SetOrgan organ{};
    (void)organ.insert(1);
    (void)organ.insert(2);
    organ.allocator_organ().s_.total_bytes_allocated  = 8192;
    organ.allocator_organ().s_.total_bytes_in_use     = 6144;
    organ.allocator_organ().s_.allocation_count       = 7;
    organ.allocator_organ().s_.external_fragmentation = 0.25;
    organ.allocator_organ().s_.internal_fragmentation = 0.125;
    cea::SetObserverAggregateWire agg2{};
    cea::fill_set_observer_aggregate(organ, agg2);
    check_eq("axis_stats[5][0] == bytes_alloc", agg2.axis_stats[5][0], std::uint64_t{8192});
    check_eq("axis_stats[5][1] == bytes_in_use", agg2.axis_stats[5][1], std::uint64_t{6144});
    check_eq("axis_stats[5][2] == alloc_cnt", agg2.axis_stats[5][2], std::uint64_t{7});
    check_eq("axis_stats[5][5] == external_frag_milli", agg2.axis_stats[5][5], std::uint64_t{250});
    check_eq("axis_stats[5][6] == internal_frag_milli", agg2.axis_stats[5][6], std::uint64_t{125});
    check_true("die Spalten tragen ihre Namen aus dem Schema",
               cea::kSetAxisSchema[5].names[5] != nullptr && cea::kSetAxisSchema[5].names[6] != nullptr);
    check_true("eine Achse OHNE Belegung bleibt namenlos UND ungeschrieben (D2, nicht stille 0)",
               cea::kSetAxisSchema[0].names[0] == nullptr && agg2.axis_stats[0][0] == 0);

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
