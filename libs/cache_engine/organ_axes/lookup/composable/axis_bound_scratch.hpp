#pragma once
// A8-S5 Familie 01b (composable-Rest, 2026-08-04) -- die EINE Speicher-Naht der Kompositions-Schicht.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4
// (Schnitt-Regel: Speicher NUR ueber das Allokator-Achsen-Interface, inline CT, kein std::variant).
//
// **WOZU:** Die Kompositions-Schale (ComposedSearch, die Traversal-Organe, die Memento-Puffer) haelt
// TRANSIENTEN und HALTENDEN Speicher, der bis zu dieser Scheibe am Default-Allokator hing -- also an der
// Allokator-Achse VORBEI. Diese Datei liefert die zwei Bausteine, mit denen die Schale ihren Speicher
// ueber die Achse fuehrt, ohne dafuer jedes Organ zum Store umzubauen:
//
//   (1) composition_allocator_t<Store>  -- WELCHE Allokator-Strategie gilt fuer eine Komposition.
//   (2) AxisBoundBuffer<Alloc, T>       -- ein SELBST-BESITZENDER, achsen-gebundener dynamischer Puffer.
//
// **WARUM (2) seinen Allokator SELBST besitzt (und nicht nur einen Adapter haelt):** Der
// StdAllocatorAdapter der Achse haelt einen Zeiger auf die Strategie (axis_06_allocator_strategy_base.hpp:198).
// Ein Puffer, der nur den ADAPTER haelt, ist damit an die Lebensdauer eines fremden Strategie-Objekts
// gekettet -- und genau das geht bei einem MEMENTO schief: der abi_adapter haelt den Memento des Such-Organs
// als eigenen MEMBER (saved_container_algorithm_m_), also potenziell laenger als das Organ, aus dem er
// stammt. Ausserdem ist der Adapter NICHT default-konstruierbar (bewusst -- Posten 64 schloss genau den
// nullptr-Pfad), ein blosser `std::vector<T, Adapter>` waere also weder default-konstruierbar noch
// zuweisbar, beides fordert der Adapter aber vom memento_t. AxisBoundBuffer loest das, indem er die
// Strategie als ERSTEN Member fuehrt und den Vektor an das EIGENE Objekt bindet -- exakt das Muster der
// Pool-Stores aus Scheibe 01a (btree_node_pool_store.hpp:19/:86: allocator_ vor den Vektoren, Copy
// rebindet + verwirft die Kopier-Pollution per Memento-Restore, Move nicht deklariert).
//
// ABGRENZUNG zu den Pool-Stores (01a): die tragen ihr Substrat und melden es als T6-Store-Statistik. Dieser
// Puffer ist SCHALEN-Speicher (Memento-Liste, Scan-Zwischenablage) und fuehrt daher einen EIGENEN Zaehler --
// die Trennung ist Absicht, nicht Nachlaessigkeit: sie haelt die T6-Doppelzaehlung fern, die der Owner-Entscheid
// zu Posten 68 (LEDGER 04.08. abend-11, "Option B strikt") als eigene Einsammel-Naht behandelt sehen will.
//
// KEIN std::variant, kein Runtime-Switch, keine virtuelle Funktion -- reine CT-Komposition (Auflage 5).

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

/// Die Allokator-Achsen-Strategie des SCHALEN-Speichers (Memento-Liste, Scan-Zwischenablage).
///
/// **BENANNTER ACHSEN-DEFAULT, nicht die T6-Wahl des Stores -- und das ist eine ENTSCHEIDUNG, kein Versehen:**
///
/// (1) PRAEZEDENZ/SEQUENZ: Die Scheiben 01d (cache_traversal) und 03 (mapping/value_handle) binden ihre Organe
///     ebenfalls ueber einen BENANNTEN Achsen-Default an ExgenAllocator -- "Achsen-Interface JA, Kompositions-
///     T6-Wahl NEIN" (LEDGER 04.08. abend-10, SUBSTANZ-VERMERK). Der ECHTE Kompositions-Rebind ist der
///     Gegenstand des 01c-DESIGN-VORLAUFS mit der Owner-Vorgabe "Option B strikt" (abend-11). Diese Zeile ist
///     der deklarierte ZWISCHENSTAND: sie faellt bei der Durchbindung nicht weg, sie WANDERT.
///
/// (2) HARTER BEFUND AM OBJEKT (2026-08-04, Scheibe 01b) -- **AUFGEKLAERT UND BEHOBEN am 2026-08-05,
///     Scheibe 01c Vorlauf (0); der Absatz bleibt als Lehre stehen, deprecated statt geloescht:**
///     Die Kompositions-Bindung war hier zuerst gebaut (`typename Store::allocator_type`) und LIEF NICHT.
///     `AxisBoundBuffer<MimallocAllocator, T>` als RUECKGABEWERT (der Memento-Pfad) liess g++ 15.3 unter
///     -O3 eine Endlosschleife emittieren (`jmp .`; -O1 zeigte es nicht, ExgenAllocator zeigte es nicht,
///     ASan/UBSan meldeten nichts). 01b konnte die Ursache nicht bestimmen und notierte den Verdacht auf
///     Compiler-Bug oder UB im Puffer.
///     URSACHE (01c, am Objekt): WEDER das eine NOCH das andere. `AllocatorStrategyBase::restore_statistics`
///     ist ein CRTP-WEITERLEITER; deklariert die Strategie keinen EIGENEN Member dieses Namens, findet die
///     Namenssuche in Derived nur den geerbten Basis-Member wieder -- unbeschraenkte Selbst-Rekursion, von
///     -O3 zur Endlosschleife gefaltet, von -O1 als Selbstaufruf stehen gelassen (daher "-O1 = 0" fuer eine
///     Textsuche nach 'jmp .'). Bis 05.08. trug ausschliesslich ExgenAllocator den Member -- daher exakt die
///     beobachtete Asymmetrie. Der Puffer war nur der erste Konsument, der den Pfad ueberhaupt betrat.
///     Behoben durch Nachziehen des Members in allen 25 betroffenen Strategien + self-proving static_assert
///     in den drei Weiterleitern der Basis; gepinnt in tests/unit/test_h81_*.cpp (Flaechen-Beweis ueber die
///     Achsen-Registry + Laufzeit-Beweis genau an DIESEM Puffer + Gegenprobe).
///     LEHRE, die bleibt: der Befund waere unter einer stillen Umgehung nie sichtbar geworden.
///     OFFEN (bewusst NICHT in 01c-1): die Zeile unten auf `typename Store::allocator_type` zu drehen ist
///     jetzt technisch frei -- sie ist aber eine SCHALEN-Durchbindung mit eigenem Kontrast-Beweis und
///     gehoert damit in die 01c-Folgescheiben, nicht in den Pilot.
///
/// Wichtig: der Default ist der ACHSEN-Default, NIE std::allocator -- die Schnitt-Regel (Dossier 3.4) gilt
/// hier vollstaendig; nur die Frage WELCHE Strategie bleibt bis 01c beim benannten Default.
using shell_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

/// Die Allokator-Achsen-Strategie des Schalen-Speichers EINER Komposition. Die Store-abhaengige Form bleibt
/// bewusst als EIN Ableitungspunkt stehen (nicht 5x ExgenAllocator hingeschrieben): wenn 01c die
/// Durchbindung baut, dreht sich GENAU diese Zeile.
///
/// A8-S5 PHASE B (2026-08-05) -- DIE ZEILE IST GEDREHT. Sie liefert nicht mehr den Achsen-DEFAULT,
/// sondern die Strategie, die der STORE dieser Komposition fuehrt. Das war die von 01c-1 hier
/// schriftlich hinterlegte Restarbeit (s. Absatz oben), und Phase B ist ihr Anlass: seit dem
/// Organ-Pfad-Faden traegt der Knoten-Pool eines Pool-Organs die T6-Wahl der Komposition real.
/// Bliebe diese Zeile stehen, waere das Organ in sich WIDERSPRUECHLICH -- sein Pool allozierte ueber
/// mimalloc, seine Schale (memento_t, Scan-Zwischenablage des Traversal-Organs) weiter ueber den
/// Achsen-Default: exakt die stille zweite Strategie im selben Tier, die Owner-KERN abend-11
/// abstellt. `allocator_type` des Composed*-Organs, das ueber diesen Alias gebildet wird, meldete
/// ausserdem eine Strategie, die sein Speicher gar nicht benutzt.
/// LEVEL-0-NEUTRALITAET: am Achsen-Default ist `Store::allocator_type` GENAU shell_allocator_t --
/// unten compile-hart gepinnt. Der golden-Pfad bewegt sich um kein Byte.
/// EHRLICHE GRENZE, die dabei sichtbar wurde: nicht jeder Store, ueber dem eine Schale gebildet
/// wird, TRAEGT eine Achsen-Bindung -- die heap-freien Form-A-Stores (z.B. NodeTypeSlotStore) haben
/// bewusst keinen `allocator_type`, weil sie keinen einzigen Byte dynamisch holen. Fuer sie bleibt
/// der benannte Achsen-DEFAULT die richtige Antwort (die Schale ueber ihnen alloziert ja doch, und
/// zwar ueber die Achse -- nur eben ohne eine Wahl, die es am Store zu erben gaebe). Das ist keine
/// Umgehung, sondern die Fallunterscheidung, die die Form-A/Form-B-Trennung ohnehin macht.
namespace detail {
/// Traegt der Store eine Achsen-Bindung, von der die Schale erben kann? (Form B ja, Form A nein.)
template <class Store>
concept StoreTraegtAchsenBindung = requires { typename Store::allocator_type; };

template <class Store, bool kVomStore>
struct composition_allocator {
    using type = shell_allocator_t; // Form A (heap-freier Store): benannter Achsen-Default.
};
template <class Store>
struct composition_allocator<Store, true> {
    using type = typename Store::allocator_type; // Form B: die T6-Wahl DIESER Komposition.
};
} // namespace detail

template <class Store>
using composition_allocator_t =
    typename detail::composition_allocator<Store, detail::StoreTraegtAchsenBindung<Store>>::type;

/// PHASE B Level-0 + Gegenprobe, self-proving an zwei Sonden statt an einer Behauptung: ein Store am
/// Achsen-Default liefert den Achsen-Default (golden-Pfad unbewegt), ein Store mit fremder Bindung
/// liefert die FREMDE -- sonst waere die gedrehte Zeile eine teure Attrappe. Die fremde Sonde ist
/// bewusst ein reiner Marker-Typ: dieser Header soll keine zweite Allokator-Strategie inkludieren.
struct PhaseBSchalenSondeDefault {
    using allocator_type = shell_allocator_t;
};
struct PhaseBSchalenSondeFremd {
    struct FremdeStrategieMarke;
    using allocator_type = FremdeStrategieMarke;
};
static_assert(std::is_same_v<composition_allocator_t<PhaseBSchalenSondeDefault>, shell_allocator_t>,
              "PHASE B Level-0 (Schale): die gedrehte Zeile liefert am Achsen-Default nicht mehr den "
              "Achsen-Default -- der golden-Pfad haette einen anderen Schalen-Allokator.");
static_assert(
    std::is_same_v<composition_allocator_t<PhaseBSchalenSondeFremd>, PhaseBSchalenSondeFremd::FremdeStrategieMarke>,
    "PHASE B Level-1 (Schale) TOT: die Schale uebernimmt die Store-Bindung nicht -- dann laeuft "
    "im selben Organ weiter eine zweite, stille Strategie.");
struct PhaseBSchalenSondeFormA {}; // heap-frei, keine Achsen-Bindung am Store
static_assert(std::is_same_v<composition_allocator_t<PhaseBSchalenSondeFormA>, shell_allocator_t>,
              "PHASE B (Schale): ein Form-A-Store ohne Achsen-Bindung muss auf den benannten Achsen-Default "
              "fallen, nicht auf einen Compile-Fehler.");

/// Selbst-besitzender, an die Allokator-ACHSE gebundener dynamischer Puffer.
///
/// Vertrag: default-konstruierbar, kopier-konstruierbar, kopier-zuweisbar (der Memento-Bedarf des
/// abi_adapters), und JEDES Byte kommt aus `Alloc`. Move ist -- wie bei den Pool-Stores -- bewusst NICHT
/// deklariert: der user-definierte Copy unterdrueckt ihn implizit, damit faellt jede Bewegung sicher auf
/// den rebindenden Copy zurueck statt einen Vektor mitsamt fremdem Adapter-Zeiger zu verschieben.
template <class Alloc, class T>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class AxisBoundBuffer {
public:
    using allocator_type = Alloc;
    using value_type     = T;
    using std_alloc_type = typename Alloc::template StdAllocatorAdapter<T>;
    using container_type = std::vector<T, std_alloc_type>;

    AxisBoundBuffer() : items_(allocator_.template as_std_allocator<T>()) {}

    /// Copy: Strategie mitkopieren, den Vektor an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento-Restore) -- 1:1 btree_node_pool_store.hpp:86.
    AxisBoundBuffer(AxisBoundBuffer const& o)
        : allocator_(o.allocator_), items_(o.items_, allocator_.template as_std_allocator<T>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    AxisBoundBuffer& operator=(AxisBoundBuffer const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter fuehrt keine propagate_-Typedefs) -> items_ behaelt sein an
            // this-allocator_ gebundenes Adapter-Objekt; die Zuweisung re-alloziert ueber this-allocator_.
            items_ = o.items_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~AxisBoundBuffer() = default;

    // -- Sicht/Groesse ------------------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return items_.empty(); }
    [[nodiscard]] T const&    operator[](std::size_t i) const noexcept { return items_[i]; }
    [[nodiscard]] T&          operator[](std::size_t i) noexcept { return items_[i]; }
    [[nodiscard]] T const&    back() const noexcept { return items_.back(); }
    [[nodiscard]] auto        begin() const noexcept { return items_.begin(); }
    [[nodiscard]] auto        end() const noexcept { return items_.end(); }
    [[nodiscard]] auto        begin() noexcept { return items_.begin(); }
    [[nodiscard]] auto        end() noexcept { return items_.end(); }

    // -- Mutation -----------------------------------------------------------------------------------
    /// Darf werfen: der StdAllocatorAdapter uebersetzt den Strategie-nullptr in std::bad_alloc
    /// (Posten 64, axis_06_allocator_strategy_base.hpp) -- [[allocation-failure-exception]].
    void reserve(std::size_t n) { items_.reserve(n); }
    void push_back(T const& v) { items_.push_back(v); }
    template <class... Args>
    void emplace_back(Args&&... args) {
        items_.emplace_back(std::forward<Args>(args)...);
    }
    void insert_at(std::size_t i, T const& v) { items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(i), v); }
    void erase_at(std::size_t i) { items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(i)); }
    void pop_back() noexcept { items_.pop_back(); }
    void clear() noexcept { items_.clear(); }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// Der Verdrahtungs-Ausweis des Puffers: die ECHTE Achsen-Statistik seiner Strategie. Getrennt von der
    /// T6-Store-Statistik (s. Kopf-Doku) -- eine Summen-Regel gehoert in die Einsammel-Naht, nicht hierher.
    [[nodiscard]] allocator_snapshot_t allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    // allocator_ VOR items_ deklariert: Konstruktions-/Destruktions-Reihenfolge (items_ haelt via Adapter
    // &allocator_ und muss VOR der Strategie sterben). Dieselbe Reihenfolge wie in den Pool-Stores.
    Alloc          allocator_{};
    container_type items_;
};

} // namespace comdare::cache_engine::lookup::composable
