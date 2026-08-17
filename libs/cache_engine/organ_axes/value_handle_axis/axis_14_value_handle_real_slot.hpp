#pragma once
// T11 value_handle — REALE Pool/Version/Chain-Slot-Struktur (User-Direktive 2026-06-04 §4.3).
//
// AUFTRAG (§4.3): „value_handle = echter Pool/Version/Chain-Deref gegen reale Slot-Struktur (statt synthetisch)."
//
// KONTEXT: value_handle (T11) ist ein OBSERVER-Organ (getrieben in fill_observer_v3 + tier_insert-Build-Hook,
// NICHT im funktionalen tier_lookup/insert-Hotpath). In der M3-Matrix ist die Strategie `value_handle_inline`
// auf 'inline' GEPINNT (alle 320 Lebewesen). Diese Datei macht die NICHT-inline-Strategien (ExternalPool /
// VersionedPointer / ImmutableSharedRef / ChainRef) REAL: jede traegt eine ECHTE, persistente Slot-Struktur,
// gegen die store_value(key,value) + deref_value(key) wirklich indizieren/dereferenzieren (kein Roh-Puffer mehr).
//
// LEITPLANKEN (verbatim §4.3):
//  (1) rein ADDITIV — 'inline' (M3-Pin) bleibt unberuehrt + messneutral: fuer is_inline()-Strategien existiert
//      KEINE store_value/deref_value-Methode (Concept-Detektion `requires` schlaegt fehl) → der abi_adapter-Build-
//      Hook (`if constexpr (requires { vh_organ_.store_value(...) })`) ruft NICHTS → die Inline-Strategie haelt
//      keine neue Struktur und wird auch nicht angefasst. EXAKT wie filter: None-artige ohne insert_key bleiben heil.
//  (2) static value_access_scan (seg19 Pfad-A) wird NICHT angefasst — diese Datei fuegt NUR Instanz-Methoden hinzu,
//      die static-Signatur in den 5 Strategie-Headern bleibt bit-identisch.
//  (3) R1-Memento: die Struktur ist dynamisch wachsend (copy-constructible + copy-assignable + operator==) ->
//      ueber den ObservableValueHandle-Wrapper bit-exakt snapshot-/restore-faehig (saved_vh_ in tier_save_all/
//      tier_rollback_all, geleert in tier_clear) — analog saved_flt_/flt_organ_.
//      A8-S5-03 (2026-08-04) PRAEZISIERT: der Wachstums-Speicher kommt seit dem Schnitt REAL ueber das
//      Allokator-ACHSEN-Interface (value_handle_slot_allocator_t + StdAllocatorAdapter) statt ueber den
//      Default-Allokator -- Dossier 20260803-a8_f2 Abschn. 3.4. Die Memento-Zusage bleibt WORTGLEICH gueltig
//      und wird dabei SCHAERFER: die Kopie rebindet auf ihr EIGENES allocator_ (btree_node_pool_store.hpp:19),
//      es entsteht also keine stille Default-Allokator-Kopie und kein Adapter, der auf die Quelle zeigt.
//  (4) Lehrbuch-Pattern, zero-cost: die per-Strategie-Auswahl (Pool vs Versioned-Pool vs Chain) ist eine
//      reine `if constexpr`-Compile-Zeit-Selektion ueber Strategy::is_inline() + Strategy::name() (Strategy-Pattern,
//      [[no-runtime-switch]]). Inline-Strategien instanziieren `EmptyRealSlot` (leer, 0-Footprint).
//
// @topic value_handle @achse 14 @saeule 2 @task §4.3-REAL @related [[per-service-vip]] (irrelevant) — vgl. axis_filter (P5 #124)

#include "concepts/axis_14_value_handle_concept.hpp"

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp> // A8-S5-03: Versorger-Achse des Slot-Backings (s.u.)

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::value_handle_axis {

/// A8-S5 Familie 03_placement (Dossier 20260803-a8_f2 Abschn. 3.4, Schnitt-Regel): das reale Slot-Backing
/// bezieht seinen Speicher ueber das Allokator-ACHSEN-Interface (axis_06) statt ueber den Default-Allokator.
/// DIESE Zeile ist die EINE Stelle, die den Versorger benennt -- Pool-, Chain- und Leer-Backing sowie der
/// Form-B-Ausweis der Observable-Huelle lesen sie, damit kein zweiter Name driften kann.
///
/// WARUM ExgenAllocator: derselbe Achsen-Default, den die bereits konformen Pool-Stores des Repos fuehren
/// (btree_node_pool_store.hpp:56, tree_node_pool_store.hpp, surf_fst_map_pool_store.hpp). Das Backing ist
/// single-threaded getrieben (Build-Hook in tier_insert), passend zur Exgen-Sub-Achse AA4. Bei abgeschaltetem
/// Vendor-Flag faellt die Strategie intern auf portable_aligned_alloc zurueck: derselbe libc-Heap wie vorher,
/// aber ueber das Achsen-Interface.
///
/// ABHAENGIGKEITSRICHTUNG (Dossier 3.3): value_handle -> alloc. Der Allokator ist die UNTERSTE Versorger-Achse;
/// organ_axes/alloc/ zieht keinen value_handle-Header -> kein Zyklus.
///
/// OFFEN (bewusst NICHT hier entschieden): Kompositions-Rebind statt Achsen-Default -- eigener Entscheid der
/// S5-Scheibe 01c (S5-Planung, LEDGER-Nachtrag 04.08. abend-4).
using value_handle_slot_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// EmptyRealSlot — fuer is_inline()-Strategien. Traegt KEINE store_value/deref_value-Methode → der Build-Hook im
// abi_adapter (requires-detektiert) greift nicht → Inline bleibt EXAKT unveraendert + messneutral (Leitplanke 1).
// Leere Struktur OHNE jeden Heap-Anteil (0 Footprint) -- sie erfuellt damit die STAERKERE Schnitt-Form (A)
// der S5-Gate-Wache (heap-frei), nicht nur Form (B). operator== = immer true (leere Inline-Struktur ist konstant).
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
struct EmptyRealSlot {
    void                      clear() noexcept {}
    [[nodiscard]] std::size_t slot_count() const noexcept { return 0; }
    [[nodiscard]] std::size_t pool_size() const noexcept { return 0; }
    [[nodiscard]] std::size_t chain_nodes() const noexcept { return 0; }
    [[nodiscard]] bool        operator==(EmptyRealSlot const&) const noexcept = default;
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// PoolValueSlot — REALE externe Pool-Indirektion (ExternalPool / ImmutableSharedRef / VersionedPointer).
//   - Der Node-Slot (slots_) haelt NUR einen Pool-INDEX (kein Inline-Value).
//   - Der eigentliche Value liegt im externen Pool (pool_).
//   - deref_value(key) = (1) Slot → Pool-Index, (2) Pool-Index → Value  ⇒ GENAU 1 abhaengige Deref (pointer chase).
// VERSIONED (Versioned=true): der Pool-Eintrag traegt zusaetzlich ein MVCC-Version-Tag (version_), das beim
// store_value inkrementiert + beim deref_value abgestreift wird (Masstree/SMART-charakteristisch) — die ECHTE,
// nicht synthetische Tag-Strip-Operation gegen die reale Struktur.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
template <bool Versioned>
struct PoolValueSlot {
    struct Slot {
        std::uint64_t key        = 0;
        std::uint64_t pool_index = 0;
    };
    struct PoolEntry {
        std::uint64_t value   = 0;
        std::uint64_t version = 0;
    };

    /// A8-S5-03 Form-B-Ausweis: der Speicher dieses Backings laeuft ueber die Allokator-Achse (real verdrahtet,
    /// s. Member unten + slot_allocator_statistics()).
    using allocator_type = value_handle_slot_allocator_t;

private:
    using slot_alloc = typename allocator_type::template StdAllocatorAdapter<Slot>;
    using pool_alloc = typename allocator_type::template StdAllocatorAdapter<PoolEntry>;

public:
    // -- A8-S5-03 Lebensdauer-Vertrag der Achsen-Verdrahtung (Muster btree_node_pool_store.hpp:19/:84-105) --
    // Der StdAllocatorAdapter haelt einen Zeiger auf allocator_. Daraus folgt:
    //   (a) allocator_ MUSS vor slots_/pool_ deklariert sein (Member-Reihenfolge unten),
    //   (b) beide Vektoren werden IMMER mit dem Adapter DIESES allocator_ konstruiert (der Adapter ist nicht
    //       default-konstruierbar -- es gibt kein stilles Zurueckfallen auf einen Default-Allokator),
    //   (c) die R1-Memento-Kopie REBINDET auf das EIGENE allocator_ -- genau das verlangt Leitplanke 3:
    //       saved_vh_.emplace(vh_organ_) (abi_adapter.hpp:1857) und vh_organ_ = *saved_vh_ (:1896/:1918)
    //       muessen ein VOLLSTAENDIG eigenstaendiges Backing erzeugen, nicht eines, das auf den Allokator
    //       des Partners zeigt (das waere ein dangling Adapter, sobald einer der beiden stirbt),
    //   (d) Move bewusst NICHT deklariert: die benutzerdeklarierte Kopie unterdrueckt den impliziten Move,
    //       ein std::move degradiert zur rebindenden Kopie statt den Fremd-Adapter zu stehlen.
    // std::vector propagiert den Allokator bei copy-assign nicht (POCCA=false ist der allocator_traits-Default)
    // -- das Ziel behaelt in operator= seinen eigenen Adapter. Genau das ist gewollt.
    PoolValueSlot()
        : slots_(allocator_.template as_std_allocator<Slot>()),
          pool_(allocator_.template as_std_allocator<PoolEntry>()) {}
    PoolValueSlot(PoolValueSlot const& o)
        : allocator_(o.allocator_), slots_(o.slots_, allocator_.template as_std_allocator<Slot>()),
          pool_(o.pool_, allocator_.template as_std_allocator<PoolEntry>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        // Memento-Symmetrie (analog btree_node_pool_store.hpp:91): die transiente Kopier-Allokation der
        // Vollkopie ist kein Mess-Ereignis der Achse -> Statistik auf den Quell-Stand zuruecksetzen.
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    PoolValueSlot& operator=(PoolValueSlot const& o) {
        if (this != &o) {
            slots_ = o.slots_; // Allokator propagiert NICHT -> dieses Objekt behaelt seinen Adapter
            pool_  = o.pool_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~PoolValueSlot() = default;

    /// Build (Setup, NICHT gemessen): den Value extern in den Pool legen + den Slot auf den Pool-Index zeigen lassen.
    /// Vorhandener Key wird ueberschrieben (in-place Update); Versioned bumpt dabei die Version (neue Snapshot-Sicht).
    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept -- pool_/slots_.push_back kann allozieren
    // ([[allocation-failure-exception]]: melden statt terminate).
    // A8-S5-03 PRAEZISIERT (Auflage 11, Fehlerklassen), Posten 70 NACHGEFUEHRT (2026-08-04): der
    // Wachstums-Fehlerpfad laeuft seit dem Schnitt ueber die Allokator-ACHSE. Die Strategie meldet OOM per
    // nullptr (axis_06_allocator_exgen.hpp); der StdAllocatorAdapter reicht ihn seit POSTEN 64 NICHT mehr
    // durch, sondern uebersetzt ihn an genau EINER Stelle in std::bad_alloc
    // (axis_06_allocator_strategy_base.hpp, StdAllocatorAdapter::allocate). Der konkrete std::bad_alloc-Wurf
    // ist damit NICHT entfallen, sondern hat nur den Traeger gewechselt: Adapter statt Default-Allokator --
    // und das F57/Muster-B-Versprechen "melden statt terminate" gilt hier wieder buchstaeblich. Die
    // Fehlerklasse der Achse bleibt kOrganAxisErrorFloor (ValueHandleStrategyBase::error_classes, FK-5);
    // der frueher hier notierte offene Punkt zur nullptr-Konversion ist ERLEDIGT (Posten 64).
    void store_value(std::uint64_t key, std::uint64_t value) {
        for (auto& sl : slots_) {
            if (sl.key == key) { // Update: Value im Pool ersetzen
                pool_[static_cast<std::size_t>(sl.pool_index)].value = value;
                if constexpr (Versioned) ++pool_[static_cast<std::size_t>(sl.pool_index)].version; // MVCC-Bump
                return;
            }
        }
        std::uint64_t const idx = static_cast<std::uint64_t>(pool_.size()); // neuer Pool-Eintrag am Ende
        pool_.push_back(PoolEntry{value, Versioned ? std::uint64_t{1} : std::uint64_t{0}});
        slots_.push_back(Slot{key, idx});
    }

    /// REALER Deref: Slot → Pool-Index → Value. Genau 1 abhaengige Indirektion (Pointer-Chase). Bei Versioned
    /// wird das Version-Tag abgestreift (Snapshot-Sichtbarkeit fliesst ins Ergebnis, damit der Read echt MVCC-quert).
    /// Liefert std::pair{gefunden, value} per out-param-freie Konvention: -1-Sentinel als „nicht gefunden".
    [[nodiscard]] bool deref_value(std::uint64_t key, std::uint64_t* out_value) const noexcept {
        for (auto const& sl : slots_) {
            if (sl.key == key) {                                                 // (1) Slot-Read → Pool-Index
                auto const& pe = pool_[static_cast<std::size_t>(sl.pool_index)]; // (2) abhaengiger Pool-Deref
                if (out_value != nullptr) {
                    if constexpr (Versioned)
                        *out_value = pe.value + pe.version; // MVCC-Tag-Strip ins Ergebnis
                    else
                        *out_value = pe.value;
                }
                return true;
            }
        }
        return false;
    }

    void clear() noexcept {
        slots_.clear();
        pool_.clear();
    }
    [[nodiscard]] std::size_t slot_count() const noexcept { return slots_.size(); }
    [[nodiscard]] std::size_t pool_size() const noexcept { return pool_.size(); }
    [[nodiscard]] std::size_t chain_nodes() const noexcept { return 0; }

    [[nodiscard]] bool operator==(PoolValueSlot const& o) const noexcept {
        if (slots_.size() != o.slots_.size() || pool_.size() != o.pool_.size()) return false;
        for (std::size_t i = 0; i < slots_.size(); ++i)
            if (slots_[i].key != o.slots_[i].key || slots_[i].pool_index != o.slots_[i].pool_index) return false;
        for (std::size_t i = 0; i < pool_.size(); ++i)
            if (pool_[i].value != o.pool_[i].value || pool_[i].version != o.pool_[i].version) return false;
        return true;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// A8-S5-03 VERDRAHTUNGS-BELEG (Nicht-Vertrags-Methode, analog btree_node_pool_store.hpp:128): die
    /// Statistik der Versorger-Strategie DIESES Backings -- macht die Form-B-Aussage der S5-Gate-Wache am
    /// Objekt pruefbar (ein bloss deklarierter allocator_type bliebe hier auf 0 stehen; das ist die in
    /// tests/unit/s5_family_alloc_conformance.hpp:31 deklarierte Grenze der Form B).
    /// NICHT im T10-Mess-Pfad: T10 misst access/indirect_deref der Huelle, nicht diesen privaten Versorger.
    [[nodiscard]] typename allocator_type::snapshot_t slot_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // allocator_ VOR slots_/pool_ (Lebensdauer des Zeigers im StdAllocatorAdapter, s. Ctor-Kommentar oben).
    allocator_type                     allocator_{};
    std::vector<Slot, slot_alloc>      slots_; ///< Node-Slots: (key -> Pool-Index), EINZIGE Slot-Struktur.
    std::vector<PoolEntry, pool_alloc> pool_;  ///< Externer Pool: der Value liegt HIER (+ MVCC-Version).
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// ChainValueSlot — REALE verkettete externe Referenz (ChainRef, Multi-Value-Schluessel).
//   - Der Node-Slot haelt einen CHAIN-HEAD-Index in den Chain-Knoten-Pool (chain_).
//   - Jeder Chain-Knoten haelt (value, next_index) — eine echte Pool-Linked-List.
//   - deref_value(key) = (1) Slot → Head-Index, (2) Head-Knoten-Deref → Value  ⇒ GENAU 2 abhaengige Derefs
//     (teuerste Variante der Achse, doppeltes Pointer-Chasing). Mehrere Werte je Key werden als Chain verlaengert
//     (store_value prepend-t einen neuen Head — Multi-Value-Semantik); deref_value liefert den NEUESTEN (Head-)Value.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
struct ChainValueSlot {
    static constexpr std::uint64_t kNil = ~std::uint64_t{0};
    struct Slot {
        std::uint64_t key        = 0;
        std::uint64_t head_index = kNil;
    };
    struct ChainNode {
        std::uint64_t value      = 0;
        std::uint64_t next_index = kNil;
    };

    /// A8-S5-03 Form-B-Ausweis: der Speicher dieses Backings laeuft ueber die Allokator-Achse (real verdrahtet,
    /// s. Member unten + slot_allocator_statistics()).
    using allocator_type = value_handle_slot_allocator_t;

private:
    using slot_alloc  = typename allocator_type::template StdAllocatorAdapter<Slot>;
    using chain_alloc = typename allocator_type::template StdAllocatorAdapter<ChainNode>;

public:
    // -- A8-S5-03 Lebensdauer-Vertrag der Achsen-Verdrahtung -- identisch PoolValueSlot (dort ausfuehrlich
    //    kommentiert): allocator_ vor den Vektoren; Vektoren IMMER ueber den Adapter dieses allocator_;
    //    R1-Memento-Kopie rebindet auf das eigene allocator_; Move bewusst nicht deklariert.
    ChainValueSlot()
        : slots_(allocator_.template as_std_allocator<Slot>()),
          chain_(allocator_.template as_std_allocator<ChainNode>()) {}
    ChainValueSlot(ChainValueSlot const& o)
        : allocator_(o.allocator_), slots_(o.slots_, allocator_.template as_std_allocator<Slot>()),
          chain_(o.chain_, allocator_.template as_std_allocator<ChainNode>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    ChainValueSlot& operator=(ChainValueSlot const& o) {
        if (this != &o) {
            slots_ = o.slots_;
            chain_ = o.chain_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~ChainValueSlot() = default;

    /// Build (Setup, NICHT gemessen): einen NEUEN Chain-Knoten an den Pool anhaengen + als neuen Head des Keys
    /// verketten (Multi-Value-Prepend). Existiert der Key noch nicht → neuer Slot mit diesem Knoten als Head.
    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept -- chain_/slots_.push_back kann allozieren.
    // A8-S5-03 PRAEZISIERT (Auflage 11), Posten 70 NACHGEFUEHRT (2026-08-04): Fehlerpfad jetzt ueber die
    // Allokator-Achse -- deren nullptr wird seit Posten 64 im StdAllocatorAdapter in std::bad_alloc
    // uebersetzt. Begruendung identisch PoolValueSlot::store_value (der dortige offene Punkt ist erledigt).
    void store_value(std::uint64_t key, std::uint64_t value) {
        std::uint64_t const node_idx = static_cast<std::uint64_t>(chain_.size());
        for (auto& sl : slots_) {
            if (sl.key == key) { // bestehende Chain: neuen Knoten als Head davorhaengen
                chain_.push_back(ChainNode{value, sl.head_index});
                sl.head_index = node_idx;
                return;
            }
        }
        chain_.push_back(ChainNode{value, kNil}); // erster Knoten der neuen Chain
        slots_.push_back(Slot{key, node_idx});
    }

    /// REALER Deref: Slot → Head-Index → Chain-Knoten → Value. Genau 2 abhaengige Derefs (verkettetes Chasing).
    [[nodiscard]] bool deref_value(std::uint64_t key, std::uint64_t* out_value) const noexcept {
        for (auto const& sl : slots_) {
            if (sl.key == key) { // (1) Slot-Read → Head-Index
                if (sl.head_index == kNil) return false;
                auto const& node = chain_[static_cast<std::size_t>(sl.head_index)]; // (2) Head-Knoten-Deref → Value
                if (out_value != nullptr) *out_value = node.value;
                return true;
            }
        }
        return false;
    }

    void clear() noexcept {
        slots_.clear();
        chain_.clear();
    }
    [[nodiscard]] std::size_t slot_count() const noexcept { return slots_.size(); }
    [[nodiscard]] std::size_t pool_size() const noexcept { return 0; }
    [[nodiscard]] std::size_t chain_nodes() const noexcept { return chain_.size(); }

    [[nodiscard]] bool operator==(ChainValueSlot const& o) const noexcept {
        if (slots_.size() != o.slots_.size() || chain_.size() != o.chain_.size()) return false;
        for (std::size_t i = 0; i < slots_.size(); ++i)
            if (slots_[i].key != o.slots_[i].key || slots_[i].head_index != o.slots_[i].head_index) return false;
        for (std::size_t i = 0; i < chain_.size(); ++i)
            if (chain_[i].value != o.chain_[i].value || chain_[i].next_index != o.chain_[i].next_index) return false;
        return true;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    /// A8-S5-03 VERDRAHTUNGS-BELEG -- identisch PoolValueSlot::slot_allocator_statistics (dort begruendet).
    [[nodiscard]] typename allocator_type::snapshot_t slot_allocator_statistics() const noexcept {
        return allocator_.statistics();
    }
#endif

private:
    // allocator_ VOR slots_/chain_ (Lebensdauer des Zeigers im StdAllocatorAdapter).
    allocator_type                      allocator_{};
    std::vector<Slot, slot_alloc>       slots_; ///< Node-Slots: (key -> Chain-Head-Index).
    std::vector<ChainNode, chain_alloc> chain_; ///< Chain-Knoten-Pool: (value, next_index) -- verkettete Liste.
};

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// real_slot_for<Strategy> — Compile-Zeit-Selektion der realen Slot-Struktur je Strategie (Strategy-Pattern,
// [[no-runtime-switch]], zero-cost). Inline → EmptyRealSlot (kein Build, messneutral). VersionedPointer →
// PoolValueSlot<true> (MVCC-Tag). ChainRef → ChainValueSlot (2 Derefs). Sonst extern-1-Deref → PoolValueSlot<false>.
// Die Auswahl nutzt is_inline() + name() (die kanonischen Strategie-Identifier, axis_14_*-Header).
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
namespace detail_real_slot {
[[nodiscard]] constexpr bool name_eq(std::string_view a, std::string_view b) noexcept { return a == b; }
} // namespace detail_real_slot

template <class Strategy>
[[nodiscard]] consteval int real_slot_kind() noexcept {
    if (Strategy::is_inline()) return 0; // 0 = Empty (Inline, messneutral)
    if (detail_real_slot::name_eq(Strategy::name(), "value_handle_chain_ref")) return 2;         // 2 = Chain (2 Derefs)
    if (detail_real_slot::name_eq(Strategy::name(), "value_handle_versioned_pointer")) return 3; // 3 = versioned Pool
    return 1; // 1 = Pool (1 Deref, extern/shared)
}

template <class Strategy>
struct real_slot_selector {
    static constexpr int kind = real_slot_kind<Strategy>();
    using type                = std::conditional_t<
        kind == 0, EmptyRealSlot,
        std::conditional_t<kind == 2, ChainValueSlot,
                           std::conditional_t<kind == 3, PoolValueSlot<true>, PoolValueSlot<false>>>>;
};

template <class Strategy>
using real_slot_t = typename real_slot_selector<Strategy>::type;

// Das reale Slot-Backing ist fuer JEDE Strategie kopierbar + vergleichbar (R1-Memento, Leitplanke 3).
static_assert(std::is_copy_constructible_v<EmptyRealSlot> && std::is_copy_assignable_v<EmptyRealSlot>);
static_assert(std::is_copy_constructible_v<PoolValueSlot<true>> && std::is_copy_assignable_v<PoolValueSlot<true>>);
static_assert(std::is_copy_constructible_v<PoolValueSlot<false>> && std::is_copy_assignable_v<PoolValueSlot<false>>);
static_assert(std::is_copy_constructible_v<ChainValueSlot> && std::is_copy_assignable_v<ChainValueSlot>);

} // namespace comdare::cache_engine::value_handle_axis
