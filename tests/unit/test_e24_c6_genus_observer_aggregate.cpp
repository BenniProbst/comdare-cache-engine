// test_e24_c6_genus_observer_aggregate -- E-24 C6 (b-Teil, ABI-EREIGNIS-SERIE).
//
// GEGENSTAND: das gemeinsame Wire-LAYOUT GenusObserverAggregate<G, N>
// (libs/cache_engine/anatomy/genus_observer_aggregate.hpp), auf dem die vier benannten Gattungs-Formen
// SetObserverAggregate<13> / SequenceObserverAggregate<9> / AdapterObserverAggregate<11> /
// ViewObserverAggregate<5> aufsetzen.
//
//   (A) ABI-PFLICHT      -- standard_layout + trivially_copyable (memcpy ueber die Modul-Grenze).
//   (B) LAYOUT-PIN       -- sizeof je Kante gegen die LITERALE Zahl UND gegen den gerechneten Ausdruck;
//                           Offset-Reihenfolge axis_stats -> seg_ns -> genus_stats -> Meta gepinnt.
//                           Ein Layout-Schnitt nach dem Freeze bricht hier compile-/laufzeit-hart.
//   (C) GENUS-TAG        -- gleiche Kante, VERSCHIEDENE Typen: ein Set-Aggregat ist kein Sequence-Aggregat
//                           (die Cross-Genus-Unmoeglichkeit gilt auch fuer die Mess-Flaeche).
//   (D) SCHEMA-ABLEITUNG -- filled_axis_count kommt aus der Schema-Tabelle, nicht aus einer Konstanten.
//   (E) HONEST-SEMANTIK  -- seg_ns ist int64: "nicht erhoben" (negativ) bleibt von "gemessen 0"
//                           unterscheidbar (D2-Doktrin: nie eine stille 0).
//
// Bau: plain int main() (kein gtest), Boost::mp11 + generated-Achsen-Includes.

#include "anatomy/genus_observer_aggregate.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

// TU-lokale Test-Fixtures: anonymer Namespace verhindert ODR-Kollisionen gleichnamiger Fixture-Typen
// ueber die e24-Test-TUs (cppcheck ctuOneDefinitionRuleViolation).
namespace {

namespace cea = comdare::cache_engine::anatomy;

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

// Die vier realen Kanten (13/9/11/5 == GenusBindingTraits<G>::slot_count, container_framework.hpp:93-96).
using SetAgg      = cea::GenusObserverAggregate<cea::AnatomyGenus::Set, 13>;
using SequenceAgg = cea::GenusObserverAggregate<cea::AnatomyGenus::Sequence, 9>;
using AdapterAgg  = cea::GenusObserverAggregate<cea::AnatomyGenus::Adapter, 11>;
using ViewAgg     = cea::GenusObserverAggregate<cea::AnatomyGenus::View, 5>;

// =============================================================================================
// (A) ABI-PFLICHT -- die Form muss memcpy-faehig sein, sonst quert sie die Modul-Grenze nicht.
// =============================================================================================

static_assert(std::is_standard_layout_v<SetAgg>);
static_assert(std::is_trivially_copyable_v<SetAgg>);
static_assert(std::is_standard_layout_v<SequenceAgg>);
static_assert(std::is_trivially_copyable_v<SequenceAgg>);
static_assert(std::is_standard_layout_v<AdapterAgg>);
static_assert(std::is_trivially_copyable_v<AdapterAgg>);
static_assert(std::is_standard_layout_v<ViewAgg>);
static_assert(std::is_trivially_copyable_v<ViewAgg>);

// Kein vtable, kein STL-Member: alignof bleibt 8 (reine 8-Byte-Ganzzahlen).
static_assert(alignof(SetAgg) == 8);
static_assert(alignof(ViewAgg) == 8);

// =============================================================================================
// (B) LAYOUT-PIN -- literale Zahl UND gerechneter Ausdruck. Beide muessen zusammenfallen.
//     Rechnung je Kante N: N*8*8 (axis_stats) + N*8 (seg_ns) + 4*8 (genus_stats) + 5*8 (Meta).
// =============================================================================================

static_assert(cea::kGenusAxisFieldCount == 8, "C6-Kante: 8 Spalten je Achsen-Slot (SA-Praezedenz kV3FieldCount)");
static_assert(cea::kGenusStatFieldCount == 4, "C6-Kante: 4 flache Gattungs-Zaehler-Spalten");

static_assert(sizeof(SetAgg) == 1008, "C6-Layout Set<13>: 13*64 + 13*8 + 32 + 40 == 1008");
static_assert(sizeof(SequenceAgg) == 720, "C6-Layout Sequence<9>: 9*64 + 9*8 + 32 + 40 == 720");
static_assert(sizeof(AdapterAgg) == 864, "C6-Layout Adapter<11>: 11*64 + 11*8 + 32 + 40 == 864");
static_assert(sizeof(ViewAgg) == 432, "C6-Layout View<5>: 5*64 + 5*8 + 32 + 40 == 432");

static_assert(sizeof(SetAgg) == SetAgg::expected_size_bytes());
static_assert(sizeof(SequenceAgg) == SequenceAgg::expected_size_bytes());
static_assert(sizeof(AdapterAgg) == AdapterAgg::expected_size_bytes());
static_assert(sizeof(ViewAgg) == ViewAgg::expected_size_bytes());

// Kein Padding zwischen den Bloecken: die Offsets folgen luecklos aufeinander.
static_assert(offsetof(SetAgg, axis_stats) == 0);
static_assert(offsetof(SetAgg, seg_ns) == 13 * 8 * 8);
static_assert(offsetof(SetAgg, genus_stats) == 13 * 8 * 8 + 13 * 8);
static_assert(offsetof(SetAgg, observable_axis_count) == 13 * 8 * 8 + 13 * 8 + 32);

// =============================================================================================
// (C) GENUS-TAG -- gleiche Kante, verschiedene Typen (Cross-Genus-Verwechslung compile-unmoeglich).
// =============================================================================================

static_assert(!std::is_same_v<cea::GenusObserverAggregate<cea::AnatomyGenus::Set, 13>,
                              cea::GenusObserverAggregate<cea::AnatomyGenus::Sequence, 13>>,
              "C6: das Genus-Tag muss die Formen unterscheiden, auch bei gleicher Kante");
static_assert(SetAgg::genus == cea::AnatomyGenus::Set);
static_assert(SetAgg::slot_count == 13);
static_assert(ViewAgg::genus == cea::AnatomyGenus::View);
static_assert(ViewAgg::slot_count == 5);

// =============================================================================================
// (D) SCHEMA-ABLEITUNG -- Fixture-Schema mit drei benannten Zeilen von fuenf.
// =============================================================================================

constexpr cea::GenusAxisFieldNames kProbeSchema[5] = {
    {{"a0", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {{"c0", "c1", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
    {{"e0", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}},
};
static_assert(cea::genus_count_filled_axes(kProbeSchema) == 3,
              "C6: filled_axis_count wird aus der Schema-Tabelle ABGELEITET, nie hartkodiert");

} // namespace

int main() {
    std::cout << "=== E-24 C6 -- GenusObserverAggregate<G, N> (gemeinsames Gattungs-Wire-Layout) ===\n";

    std::cout << "\n[A] ABI-Pflicht (compile-time gepinnt)\n";
    check_true("SetAgg standard_layout + trivially_copyable",
               std::is_standard_layout_v<SetAgg> && std::is_trivially_copyable_v<SetAgg>);

    std::cout << "\n[B] Layout-Pins (Byte-Groessen je Gattungs-Kante)\n";
    check_eq("sizeof(SetObserverAggregate<13>)", sizeof(SetAgg), std::size_t{1008});
    check_eq("sizeof(SequenceObserverAggregate<9>)", sizeof(SequenceAgg), std::size_t{720});
    check_eq("sizeof(AdapterObserverAggregate<11>)", sizeof(AdapterAgg), std::size_t{864});
    check_eq("sizeof(ViewObserverAggregate<5>)", sizeof(ViewAgg), std::size_t{432});

    std::cout << "\n[C] memcpy ueber eine simulierte Modul-Grenze (Wert-Identitaet)\n";
    SetAgg src{};
    src.axis_stats[0][0]      = 4711;
    src.axis_stats[12][7]     = 815;
    src.seg_ns[3]             = -1; // "nicht erhoben" bleibt unterscheidbar von "gemessen 0"
    src.seg_ns[4]             = 0;  // ehrliche Null: gemessen, aber 0 ns
    src.genus_stats[0]        = 99;
    src.observable_axis_count = 1;
    src.total_slots           = 13;
    src.filled_axis_count     = 2;
    src.seg_run_total_ns      = 12345;

    unsigned char wire[sizeof(SetAgg)];
    std::memcpy(wire, &src, sizeof(SetAgg));
    SetAgg dst{};
    std::memcpy(&dst, wire, sizeof(SetAgg));

    check_true("memcpy-Roundtrip liefert einen wertgleichen Snapshot", dst == src);
    check_eq("dst.axis_stats[12][7]", dst.axis_stats[12][7], std::uint64_t{815});
    check_eq("dst.total_slots", dst.total_slots, std::uint64_t{13});

    std::cout << "\n[D] Honest-Semantik von seg_ns (D2-Doktrin: nie eine stille 0)\n";
    check_true("seg_ns[3] < 0 == 'nicht erhoben'", dst.seg_ns[3] < 0);
    check_true("seg_ns[4] == 0 == 'gemessen, 0 ns'", dst.seg_ns[4] == 0);
    check_true("beide Zustaende sind verschieden", dst.seg_ns[3] != dst.seg_ns[4]);

    std::cout << "\n[E] Default-Zustand ist vollstaendig genullt (keine Zufallswerte auf dem Draht)\n";
    SetAgg      fresh{};
    bool        all_zero = true;
    auto const* raw      = reinterpret_cast<unsigned char const*>(&fresh);
    for (std::size_t i = 0; i < sizeof(SetAgg); ++i) {
        if (raw[i] != 0) all_zero = false;
    }
    check_true("value-initialisiertes Aggregat ist byteweise 0", all_zero);

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE PRUEFUNGEN GRUEN" : "FEHLER") << " (" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
