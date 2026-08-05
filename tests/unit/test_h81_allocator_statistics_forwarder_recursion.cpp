// POSTEN 80 (LEDGER 05.08.2026 nacht-1) / A8-S5 Scheibe 01c Vorlauf (0) -- der dokumentierte
// "-O3 / MimallocAllocator / 'jmp .'"-Befund aus Scheibe 01b, NACHGEPRUEFT und an seiner ECHTEN
// Ursache gepinnt.
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4 + 6.
//
// DER BEFUND (01b, axis_bound_scratch.hpp Kopf): `AxisBoundBuffer<MimallocAllocator, T>` als
// Rueckgabewert liess g++ 15.3 unter -O3 eine Endlosschleife emittieren; -O1 zeigte es nicht,
// ExgenAllocator zeigte es nicht, ASan/UBSan meldeten nichts. Die Scheibe notierte das ehrlich als
// ungeklaerten Vorbehalt -- Compiler-Bug oder UB im Puffer waren die Verdachte.
//
// DIE URSACHE (diese Scheibe, am Objekt): WEDER Compiler-Bug NOCH UB im Puffer.
// `AllocatorStrategyBase::restore_statistics` (axis_06_allocator_strategy_base.hpp) ist ein
// CRTP-WEITERLEITER -- er ruft `derived().restore_statistics(s)`. Deklariert die Strategie keinen
// eigenen Member dieses Namens, findet die Namenssuche in Derived nur den GEERBTEN Basis-Member
// wieder: der Weiterleiter ruft sich selbst, unbeschraenkt. -O3 faltet die Endlos-Tail-Rekursion zur
// Endlosschleife ('jmp .'), -O1 laesst den rekursiven Selbstaufruf stehen. Bis 2026-08-05 trug
// ausschliesslich ExgenAllocator ein eigenes restore_statistics -- deshalb genau die beobachtete
// Asymmetrie Exgen-still/Mimalloc-haengt. Der Puffer war nur der erste Konsument, der den Pfad
// ueberhaupt betrat (Memento = Copy-Ctor = restore_statistics).
//
// WARUM DAS DIESE SCHEIBE BLOCKIERTE: 01c bindet die Organ-Algorithmen an die T6-Wahl der KOMPOSITION
// (Owner-KERN "Option B strikt", LEDGER 04.08. abend-11). Kompositionen fuehren real fremde Strategien
// (art_paper_binding_reference.hpp: MimallocAllocator). Jeder rebundene Core waere beim ersten Copy
// haengengeblieben -- der Fehler musste VOR der Fassaden-Arbeit weg, nicht umschifft werden.
//
// WAS DIESE TU PINNT (drei Aussagen, alle self-proving):
//   (1) POSITIV/LAUFZEIT: der Memento-Pfad laeuft ueber die reparierten Strategien real durch und
//       stellt den Snapshot wieder her -- die Aussage, die die Reparatur macht.
//   (2) FLAECHEN-BEWEIS: JEDE Strategie der Achsen-Registry deklariert ein EIGENES restore_statistics.
//       Die Liste kommt aus der Registry, nicht aus einer gepflegten Namensliste -- eine neue Strategie
//       ohne den Member bricht hier, nicht erst beim naechsten Haenger.
//   (3) GEGENPROBE/BISS: eine Stub-Strategie OHNE eigenen Member wird als Basis-Weiterleiter ERKANNT.
//       Ohne diese Probe waere (2) eine Wache, die nie beissen kann.
//
// Standalone (plain int main, KEIN gtest) -- konsistent mit den uebrigen S5-/Haertungs-Wachen.

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/axis_06_allocator_mimalloc.hpp>
#include <axes/alloc/axis_06_allocator_registry.hpp>
#include <axes/alloc/axis_06_allocator_strategy_base.hpp>
#include <axes/lookup/composable/axis_bound_scratch.hpp>

#include <boost/mp11.hpp>

#include <cstdint>
#include <cstdio>
#include <type_traits>

namespace mp    = boost::mp11;
namespace alloc = ::comdare::cache_engine::alloc;
namespace acpts = ::comdare::cache_engine::alloc::concepts;
namespace comp  = ::comdare::cache_engine::lookup::composable;

namespace {

// ---------------------------------------------------------------------------------------------
// (3) GEGENPROBE: der Bestand VOR der Reparatur, als Stub konserviert. Diese Strategie deklariert
//     KEIN eigenes restore_statistics -- exakt der Zustand, in dem 25 der 26 Achsen-Strategien waren.
//     Sie wird NIE aufgerufen (das haenge); geprueft wird ausschliesslich der Member-Zeiger-TYP.
// ---------------------------------------------------------------------------------------------
class WeiterleiterStubStrategie : public alloc::AllocatorStrategyBase<WeiterleiterStubStrategie> {
public:
    static constexpr bool             enabled = false;
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "weiterleiter_stub"; }

    [[nodiscard]] void* allocate(std::size_t, std::size_t = alignof(std::max_align_t)) { return nullptr; }
    void                deallocate(void*, std::size_t, std::size_t = alignof(std::max_align_t)) noexcept {}

    using snapshot_t = acpts::AllocationStatistics;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }
    // KEIN restore_statistics -- das IST die Gegenprobe.

private:
    snapshot_t stats_{};
};

/// Der Detektor: traegt der Member-Zeiger die BASIS als Klassen-Teil, hat die Strategie nichts
/// Eigenes deklariert -- dann ist der Weiterleiter eine Selbst-Rekursion.
template <class S>
inline constexpr bool deklariert_eigenes_restore_v =
    !std::is_same_v<decltype(&S::restore_statistics),
                    void (alloc::AllocatorStrategyBase<S>::*)(acpts::AllocationStatistics const&) noexcept>;

template <class S>
using deklariert_eigenes_restore = mp::mp_bool<deklariert_eigenes_restore_v<S>>;

// (3) BISS: der Stub MUSS als Weiterleiter erkannt werden, sonst pinnt (2) nichts.
static_assert(!deklariert_eigenes_restore_v<WeiterleiterStubStrategie>,
              "GEGENPROBE TOT: eine Strategie ohne eigenes restore_statistics wird NICHT mehr als "
              "Basis-Weiterleiter erkannt -- der Flaechen-Beweis unten kann dann nicht mehr beissen.");
// Positiv-Kontrolle desselben Detektors an der Strategie, die den Member schon immer trug.
static_assert(deklariert_eigenes_restore_v<alloc::ExgenAllocator>,
              "GEGENPROBE TOT: ExgenAllocator traegt sein eigenes restore_statistics nicht mehr.");
// Die Strategie des Original-Befundes, namentlich gepinnt (nicht nur ueber die Liste).
static_assert(deklariert_eigenes_restore_v<alloc::MimallocAllocator>,
              "REGRESSION Posten 80: MimallocAllocator ist wieder ohne eigenes restore_statistics -- der "
              "Memento-Pfad jedes mimalloc-gebundenen Stores/Puffers haengt dann erneut ('jmp .' unter -O3).");

// ---------------------------------------------------------------------------------------------
// (2) FLAECHEN-BEWEIS ueber die ACHSEN-REGISTRY (Typ-Liste abgeleitet, nie handgepflegt).
// ---------------------------------------------------------------------------------------------
using AlleStrategien = alloc::AllVendors;

static_assert(mp::mp_size<AlleStrategien>::value > 1, "Registry-Liste leer/degeneriert -- Beweis waere vakuoes.");
static_assert(mp::mp_all_of<AlleStrategien, deklariert_eigenes_restore>::value,
              "axis_06 FLAECHEN-BRUCH: mindestens eine Achsen-Strategie deklariert kein eigenes "
              "restore_statistics und wuerde den geerbten CRTP-Weiterleiter in die Selbst-Rekursion "
              "schicken. Member nachziehen (Vorlage: axis_06_allocator_exgen.hpp).");

int fehler = 0;

void pruefe(bool ok, char const* was) {
    std::printf("  [%s] %s\n", ok ? "OK" : "FEHLER", was);
    if (!ok) ++fehler;
}

} // namespace

int main() {
    std::printf("H81 -- axis_06 Statistik-Weiterleiter: Selbst-Rekursion (Posten 80 / 01b-Vorbehalt)\n");

    std::printf("(2) Flaechen-Beweis ueber AllStrategies: %zu Strategien, alle mit eigenem restore_statistics\n",
                static_cast<std::size_t>(mp::mp_size<AlleStrategien>::value));

    // (1) LAUFZEIT-BEWEIS am Original-Gegenstand des Befundes: der Memento-Pfad des
    //     AxisBoundBuffer, einmal ueber die Strategie, an der der Befund beobachtet wurde.
    //     VOR der Reparatur waere dieser main() nie zurueckgekehrt.
    {
        using MimBuf = comp::AxisBoundBuffer<alloc::MimallocAllocator, std::uint32_t>;

        MimBuf quelle;
        for (std::uint32_t i = 0; i < 64u; ++i) {
            quelle.push_back(i * 7u + 1u);
        }
        MimBuf const kopie(quelle); // <- der Pfad, der haengte (Copy-Ctor -> restore_statistics)

        pruefe(kopie.size() == quelle.size(),
               "Memento-Kopie ueber MimallocAllocator kehrt zurueck und traegt die Werte");

        bool werte_gleich = true;
        for (std::size_t i = 0; i < kopie.size(); ++i) {
            if (kopie[i] != quelle[i]) werte_gleich = false;
        }
        pruefe(werte_gleich, "Kopier-Inhalt byte-gleich zur Quelle");

#ifdef COMDARE_CE_ENABLE_STATISTICS
        // Der Memento-Vertrag selbst: restore_statistics setzt den Zaehler-Stand der Quelle,
        // verwirft also die transiente Kopier-Pollution. Das ist die AUSSAGE der Reparatur --
        // ein blosses "kehrt zurueck" waere zu schwach.
        pruefe(kopie.allocator_statistics().allocation_count == quelle.allocator_statistics().allocation_count,
               "restore_statistics wirkt: Kopie traegt den Allokations-Stand der Quelle (keine COW-Pollution)");
#endif
    }

    // Zweite Strategie derselben Klasse, um zu zeigen, dass es keine mimalloc-Einzelheilung war.
    {
        using ExgBuf = comp::AxisBoundBuffer<alloc::ExgenAllocator, std::uint32_t>;
        ExgBuf quelle;
        for (std::uint32_t i = 0; i < 16u; ++i) {
            quelle.push_back(i);
        }
        ExgBuf const kopie(quelle);
        pruefe(kopie.size() == 16u, "Kontroll-Pfad ExgenAllocator unveraendert lauffaehig");
    }

    std::printf(fehler == 0 ? "H81: BESTANDEN\n" : "H81: FEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
