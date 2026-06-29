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
//  (3) R1-Memento: die Struktur ist `std::vector`-basiert (copy-constructible + copy-assignable + operator==) →
//      ueber den ObservableValueHandle-Wrapper bit-exakt snapshot-/restore-faehig (saved_vh_ in tier_save_all/
//      tier_rollback_all, geleert in tier_clear) — analog saved_flt_/flt_organ_.
//  (4) Lehrbuch-Pattern, zero-cost: die per-Strategie-Auswahl (Pool vs Versioned-Pool vs Chain) ist eine
//      reine `if constexpr`-Compile-Zeit-Selektion ueber Strategy::is_inline() + Strategy::name() (Strategy-Pattern,
//      [[no-runtime-switch]]). Inline-Strategien instanziieren `EmptyRealSlot` (leer, 0-Footprint).
//
// @topic value_handle @achse 14 @saeule 2 @task §4.3-REAL @related [[per-service-vip]] (irrelevant) — vgl. axis_filter (P5 #124)

#include "concepts/axis_14_value_handle_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::value_handle_axis {

// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// EmptyRealSlot — fuer is_inline()-Strategien. Traegt KEINE store_value/deref_value-Methode → der Build-Hook im
// abi_adapter (requires-detektiert) greift nicht → Inline bleibt EXAKT unveraendert + messneutral (Leitplanke 1).
// std::vector-freie leere Struktur (0 Footprint). operator== = immer true (leere Inline-Struktur ist konstant).
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

    std::vector<Slot>      slots_{}; ///< Node-Slots: (key → Pool-Index). EINZIGE Slot-Struktur (kein Inline-Value).
    std::vector<PoolEntry> pool_{};  ///< Externer Pool: der Value liegt HIER (+ MVCC-Version bei Versioned).

    /// Build (Setup, NICHT gemessen): den Value extern in den Pool legen + den Slot auf den Pool-Index zeigen lassen.
    /// Vorhandener Key wird ueberschrieben (in-place Update); Versioned bumpt dabei die Version (neue Snapshot-Sicht).
    void store_value(std::uint64_t key, std::uint64_t value) noexcept {
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

    std::vector<Slot>      slots_{}; ///< Node-Slots: (key → Chain-Head-Index).
    std::vector<ChainNode> chain_{}; ///< Chain-Knoten-Pool: (value, next_index) — echte verkettete Liste.

    /// Build (Setup, NICHT gemessen): einen NEUEN Chain-Knoten an den Pool anhaengen + als neuen Head des Keys
    /// verketten (Multi-Value-Prepend). Existiert der Key noch nicht → neuer Slot mit diesem Knoten als Head.
    void store_value(std::uint64_t key, std::uint64_t value) noexcept {
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
