#pragma once
// A8-S5 Familie 05_write_path_io / RESTSTRECKE queuing (2026-08-05) -- DIE EINE STELLE, an der die
// Q1-Achse ihren Allokator-ACHSEN-Default benennt, und der Zell-Array-Halter fuer die zwei Organe,
// deren Zustand kein std-Container ist.
//
// @topic queuing @achse Q1 buffer_strategy
//
// WARUM ES DIESE DATEI GIBT (und warum sie NICHT "utils" heisst): der B-5-Schnitt verlangt, dass der
// Organ-Zustand JEDES Organs ueber das Allokator-Achsen-Interface laeuft (Dossier
// docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4; Owner-KERN
// LEDGER 04.08.2026 abend-6/abend-11). 13 der 15 Q1-Organe brauchen dafuer denselben zwei Zeilen
// langen Vorspann (Achsen-Default + Adapter-Alias). Stuende er 13x in 13 Dateien, waere die
// Achsen-Bindung eine Konvention; hier ist sie EINE Deklaration, die alle 13 lesen.
//
// FORM-ENTSCHEID DER ACHSE (LEDGER 04.08.2026 abend-11 "Option B strikt" -- warum diese Achse ihn
// NICHT in der 01c-Fassaden-Form fahren kann): die Q1-Organe sind REGISTRY-Organe. tools/axis_registry_gen
// reflektiert type_name<W>() in die committete cache_engine_axis_registry.xml (Feld type=/wrapper=,
// Zeilen 102-116) und der F30-Guard verlangt, dass type= mit dem COMDARE_DEFINE_ORGAN_LOCATION-Literal
// beginnt. Ein Template-Kopf machte aus `FIFOQueueBuffer` den Alias einer Template-Id und drehte zwei
// XML-Felder -- gegen die Byte-Stabilitaets-Auflage und gegen test_axis_registry_roundtrip. Die volle
// 01c-Konstruktion (Core + Fassade + Rebound-Leaf) waere der Weg, der auch die Kompositions-Wahl der
// T6-Achse durchbindet -- sie braucht aber die REBIND-NAHT im Kompositions-Punkt, und die liegt im
// abi_adapter, der in dieser Scheibe ausdruecklich NICHT angefasst wird. Diese Achse faehrt deshalb
// das 01d-/03er-Muster: FEST an den BENANNTEN Achsen-Default gebunden, nie an std::allocator. Das ist
// die Form B, die die Familien-Wache liest -- und die Naht fuer "Option B strikt" ist die EINE Zeile
// unten, nicht 13 verstreute Container-Deklarationen.

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>

#include <cstddef>
#include <memory>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

/// DER BENANNTE ACHSEN-DEFAULT der Q1-Puffer. `ExgenAllocator` ist die Default-Strategie der
/// Allokator-Achse (axis_06); mit deaktivierter Achse ist ihr Unterbau real der System-Allokator --
/// der UNTERSCHIED ist, dass jede Anforderung durch die Achse laeuft und dort GEZAEHLT wird (T6).
/// Genau das war die Blindstelle: 15 enabled Q1-Puffer allozierten am T6-Zaehler vorbei.
using queuing_buffer_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<queuing_buffer_allocator_t>,
              "A8-S5 queuing: der benannte Achsen-Default erfuellt das axis_06-Achsen-Concept nicht mehr -- "
              "dann liefe der Puffer-Speicher wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

/// Der std-Allokator-Adapter der Achse fuer Element-Typ T. Kurzform, damit die Container-Deklarationen
/// in den Organen lesbar bleiben; die Mechanik (nullptr -> std::bad_alloc, Posten 64) steckt im Adapter.
template <class T>
using queuing_buffer_alloc_t = typename queuing_buffer_allocator_t::template StdAllocatorAdapter<T>;

namespace detail {

/// AchsGEBUNDENER Zell-Array-Halter fuer die beiden LOCK-FREIEN Organe (LockFreeMPMCBuffer,
/// OriginalLockFreeMpmcConcurrentQueue).
///
/// FORM-ENTSCHEID, AM OBJEKT BEGRUENDET (Auftrags-Auflage "lockfree: SORGFALT"):
///   * Form A (inline CT-Array) SCHEIDET AUS: die Kapazitaet ist ein LAUFZEIT-Aspekt
///     (iterable_aspect_t, kIterableCapacities bis 65536) und wird per set_iterable_aspect neu
///     gesetzt. Eine CT-Kappe ueber dem Maximum waere ein 65536*sizeof(Cell)-Inline-Block in JEDER
///     Komposition -- keine staerkere Aussage, sondern eine falsche.
///   * std::vector<Cell, Adapter> SCHEIDET AUS: Cell haelt ein std::atomic. Der Reconfigure-Pfad
///     (set_iterable_aspect) braucht eine Zuweisung, und die Zuweisung eines vector mit
///     propagate_on_container_move_assignment=false instanziiert den elementweisen Move-Pfad --
///     std::atomic ist weder move-konstruierbar noch move-zuweisbar, das braeche ill-formed. Der
///     Halter haette also die lock-freie Zell-Struktur veraendern muessen, um den Container zu
///     bedienen: genau die Grenze, die der Auftrag als "nicht brechen" markiert.
///   * GEWAEHLT: EIN Block ueber allocator_traits der Achse, elementweise construct/destroy. Die
///     lock-freie SEMANTIK ist unberuehrt -- Allokation passiert ausschliesslich im Konstruktor und
///     in set_iterable_aspect, und beide sind am Objekt bereits als "nur sicher wenn keine
///     Producer/Consumer aktiv" (Reconfigure-Time) dokumentiert. Der CAS-/sequence-Protokollpfad
///     sieht diese Klasse nie.
///
/// WURF-VERTRAG (Posten 64): reset() alloziert ueber den StdAllocatorAdapter der Achse. Die Strategie
/// meldet OOM per nullptr, der Adapter uebersetzt das an EINER Stelle in std::bad_alloc
/// (axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate). Die
/// [[allocation-failure-exception]]-Aussagen der beiden Organe bleiben damit WAHR; nur die QUELLE des
/// Wurfs wandert vom Default-Allokator an die Achse. Cell ist noexcept-default-konstruierbar
/// (std::atomic + Ganzzahl), die Konstruktions-Schleife kann also nicht mittendrin werfen.
///
/// COW-/MOVE-DISZIPLIN: der Halter zeigt auf die Strategie-INSTANZ des Organs. Kopie UND Move sind
/// deshalb geloescht -- beide zoegen einen Fremd-Zeiger mit. Die beiden Organe loeschen ihre eigenen
/// Copy-/Move-Operationen ohnehin (atomics), der Halter macht dieselbe Aussage eine Ebene tiefer.
template <class Cell>
class AxisCellArray {
    using cell_alloc = queuing_buffer_alloc_t<Cell>;
    using traits     = std::allocator_traits<cell_alloc>;

public:
    AxisCellArray() noexcept                       = default;
    AxisCellArray(AxisCellArray const&)            = delete;
    AxisCellArray& operator=(AxisCellArray const&) = delete;
    AxisCellArray(AxisCellArray&&)                 = delete;
    AxisCellArray& operator=(AxisCellArray&&)      = delete;
    ~AxisCellArray() { release(); }

    /// Verwirft den alten Block und besorgt einen neuen von `n` default-konstruierten Zellen ueber die
    /// Achse. `strat` MUSS die Strategie-Instanz des besitzenden Organs sein (Lebensdauer-Kopplung).
    /// SONDERFALL [[allocation-failure-exception]]: Adapter-Wurf bei OOM (s. Klassen-Doku).
    void reset(queuing_buffer_allocator_t* strat, std::size_t n) {
        release();
        strat_ = strat;
        cell_alloc  a{strat_};
        Cell* const p = traits::allocate(a, n); // wirft std::bad_alloc statt nullptr (Posten 64)
        for (std::size_t i = 0; i < n; ++i) traits::construct(a, p + i);
        data_ = p;
        n_    = n;
    }

    [[nodiscard]] Cell&       operator[](std::size_t i) noexcept { return data_[i]; }
    [[nodiscard]] Cell const& operator[](std::size_t i) const noexcept { return data_[i]; }
    [[nodiscard]] std::size_t size() const noexcept { return n_; }
    [[nodiscard]] bool        empty() const noexcept { return data_ == nullptr; }

private:
    void release() noexcept {
        if (data_ == nullptr) return;
        cell_alloc a{strat_};
        for (std::size_t i = n_; i > 0; --i) traits::destroy(a, data_ + (i - 1));
        traits::deallocate(a, data_, n_);
        data_ = nullptr;
        n_    = 0;
    }

    queuing_buffer_allocator_t* strat_ = nullptr;
    Cell*                       data_  = nullptr;
    std::size_t                 n_     = 0;
};

} // namespace detail

} // namespace comdare::cache_engine::queuing::axis_q1_queuing
