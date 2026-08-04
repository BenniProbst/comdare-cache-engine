#pragma once
// A8-S5 GATE-MUSTER (Pilot: Familie 04_execution, 2026-08-04) -- die WIEDERVERWENDBARE Haelfte der
// Familien-Konformitaets-Wache. Folge-Familien (01_read_path, 02_layout, 03_placement, 05_write_path_io)
// legen NUR eine eigene TU mit ihrer Typ-Liste an; an DIESER Datei aendert sich dabei nichts.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4
// (Schnitt-Regel: Speicher NUR ueber das Allokator-Achsen-Interface, inline CT, kein std::variant)
// + Abschn. 5-S5 ("GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie + Perf-Sanity").
//
// WAS die Wache pinnt -- auf TYP-EBENE, gegen die REALEN Kompositions-Typen aus den Achsen-Registries,
// NIE gegen Literale, Dateinamen oder Quelltext-Muster (Manager-Entscheid: static_assert-Form, KEIN
// Quelltext-Scanner): jedes Familien-Organ erfuellt GENAU EINE der beiden zulaessigen Schnitt-Formen.
//
//   (A) HEAP-FREI: das Organ besitzt ueberhaupt keinen dynamisch allozierten Speicher. Das ist die
//       staerkere der beiden Formen und der richtige Schnitt ueberall dort, wo eine COMPILE-TIME-Kappe
//       existiert (Beispiel: die Pfad-Trajektorie mit kMaxTrackedSlots).
//   (B) UEBER DIE ALLOKATOR-ACHSE: das Organ deklariert einen allocator_type, und dieser Typ erfuellt
//       das Allokator-Achsen-Concept AllocatorStrategy (axes/alloc/concepts/axis_06_allocator_concept.hpp).
//       Das ist die Form fuer unbounded Container -- Muster: BTreeNodePoolStore mit
//       StdAllocatorAdapter-Rebind + COW-/Memento-Verhalten (btree_node_pool_store.hpp).
//
// WARUM (A) STRUKTURELL PRUEFBAR IST statt nur behauptet: ein besitzender Standard-Container
// (std::vector/map/unordered_map -- mit JEDEM Allokator) macht seine Huelle weder trivial destruierbar
// noch trivial kopierbar, und zwar transitiv ueber jede Member-Ebene. "is_trivially_destructible &&
// is_trivially_copyable" ist damit ein TYP-BEWEIS, dass im Organ kein besitzender Container steckt --
// keine Selbstauskunft eines Flags, keine Textsuche im Quelltext. Was die Form NICHT abdeckt und was
// sie deshalb auch nicht behauptet: ein roher, selbst verwalteter Zeiger auf Fremdspeicher (der waere
// trivial kopierbar). Fuer die Achsen-Organe der Familien ist genau der Container-Fall der reale
// Bestand aus B-5, und den faengt die Form vollstaendig.
//
// GRENZE DER FORM B (Review-F2, deklariert): AxisAllocatorBoundOrgan prueft, dass ein DEKLARIERTER
// allocator_type das AllocatorStrategy-Concept erfuellt -- NICHT, dass jede Member-Allokation real
// darueber laeuft. Ein Organ mit Default-Allokator-Container UND einer blossen using-Zeile bestuende
// via (B), fiele aber via (A) durch; weil FamilyAllocConform (A)||(B) ist, MUSS eine Folge-Familie,
// die Form B nutzt, die reale Verdrahtung zusaetzlich belegen (Referenz-Anker: BTreeNodePoolStore --
// allocator_type + StdAllocatorAdapter-Rebind + COW-restore am Objekt). Form B ist eine
// Deklarations-Wache, Form A der harte Typ-Beweis.
//
// SELBSTBEWEIS: die Negativ-Probe am Ende dieser Datei MUSS durchfallen und die Positiv-Proben MUESSEN
// halten -- sonst pinnt die Wache nichts (die Lehre "gruene Tests zementieren alte Ordnung": eine Wache,
// die nie beissen kann, ist eine Erfolgs-Meldung ohne Aussage).

#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>

#include <boost/mp11.hpp>

#include <cstdint>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::tests::s5 {

namespace mp = boost::mp11;

/// Form (A): das Organ besitzt keinen dynamisch allozierten Speicher (kein besitzender Container).
template <class T>
concept HeapFreeOrgan = std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>;

/// Form (B): das Organ fuehrt seinen Speicher ueber das Allokator-ACHSEN-Interface.
template <class T>
concept AxisAllocatorBoundOrgan = requires {
    typename T::allocator_type;
} && ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<typename T::allocator_type>;

/// Die Familien-Konformitaet: (A) ODER (B). Alles andere ist ein Default-Allokator-Container am
/// Allokator-Achsen-Interface vorbei -- genau der B-5-Bestand, den S5 abbaut.
template <class T>
concept FamilyAllocConform = HeapFreeOrgan<T> || AxisAllocatorBoundOrgan<T>;

/// mp11-Praedikat zu FamilyAllocConform (fuer mp_all_of ueber eine Registry-Liste).
template <class T>
using is_family_alloc_conform = mp::mp_bool<FamilyAllocConform<T>>;

/// Der EINE Ausdruck, den eine Familien-TU auf ihre Typ-Liste anwendet.
template <class List>
inline constexpr bool family_alloc_conform_v = mp::mp_all_of<List, is_family_alloc_conform>::value;

/// Welche der beiden Formen ein Organ erfuellt -- fuer den literalen Lauf-Ausweis der Familien-TU.
/// (Ein Organ darf beide erfuellen; dann gewinnt die staerkere Aussage A in der Anzeige.)
template <class T>
[[nodiscard]] constexpr char const* family_alloc_form() noexcept {
    if constexpr (HeapFreeOrgan<T>) {
        return "A heap-frei";
    } else if constexpr (AxisAllocatorBoundOrgan<T>) {
        return "B ueber Allokator-Achse";
    } else {
        return "KEINE (Default-Allokator-Container)";
    }
}

namespace selfproof {

/// NEGATIV-PROBE: genau der Bestand, den S5 abbaut -- ein Default-Allokator-Container als Member.
/// Diese Probe MUSS durchfallen, sonst pinnt die Wache nichts.
struct DefaultAllocHeapProbe {
    std::vector<std::uint64_t> owned{};
};

/// POSITIV-PROBE Form (A): reiner Inline-Zustand.
struct InlineOnlyProbe {
    std::uint64_t a = 0;
    std::uint64_t b[4]{};
};

static_assert(!FamilyAllocConform<DefaultAllocHeapProbe>,
              "S5-Gate-Muster kaputt: ein Default-Allokator-Container erfuellt die Familien-Konformitaet -- "
              "die Wache kann nicht mehr beissen und ihre gruenen Laeufe sind wertlos.");
static_assert(FamilyAllocConform<InlineOnlyProbe>,
              "S5-Gate-Muster kaputt: reiner Inline-Zustand erfuellt Form (A) nicht mehr.");
static_assert(!family_alloc_conform_v<mp::mp_list<InlineOnlyProbe, DefaultAllocHeapProbe>>,
              "S5-Gate-Muster kaputt: die Listen-Form uebersieht ein nicht-konformes Organ.");
static_assert(family_alloc_conform_v<mp::mp_list<InlineOnlyProbe>>,
              "S5-Gate-Muster kaputt: die Listen-Form lehnt ein konformes Organ ab.");

} // namespace selfproof

} // namespace comdare::cache_engine::tests::s5
