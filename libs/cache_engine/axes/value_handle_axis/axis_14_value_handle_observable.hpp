#pragma once
// Per-Achsen-Observer Phase B (2026-06-04) — ObservableValueHandle<Strategy>: ObservableAxis-Huelle um eine
// value_handle-Strategie (T11, axis_14). EXAKT analog axis_05_memory_layout_observable.hpp / axis_11_telemetry_
// observable.hpp / axis_io_dispatch_observable.hpp: die Strategie selbst (Inline/ExternalPool/ImmutableSharedRef/
// VersionedPointer/ChainRef) traegt zwar die verhaltens-tragende static Methode value_access_scan(), hat aber
// KEIN statistics()/snapshot_t (ist KEIN ObservableAxis). Die Mess-Mechanik gehoert daher in diese Huelle, die
// value_access_scan static durchreicht (der seg19-Aufrufer abi_adapter T11 `ValueHandle::value_access_scan`
// bleibt heil) UND einen Instanz-Driver observe_value_handle() ergaenzt, der beim Treiben trackt.
//
// @topic value_handle @achse 14 @saeule 2 (Per-Achsen-Observer) @task Phase-B
//
// **Achsen-Semantik (treu, ECHTER Slot-Scan statt Roh-Puffer):** Die value_handle-Achse charakterisiert, WIE der
// Value vom Node-Slot erreicht wird: Inline = direkt im Slot eingebettet (1 Read, KEINE Indirektion); ExternalPool/
// VersionedPointer = 1 abhaengige Deref (Pointer-Chase / Tag-Strip); ChainRef = 2 abhaengige Derefs (verkettetes
// Pointer-Chasing). Der Observer-Driver (observe_value_handle) wird vom abi_adapter ueber das ECHTE container_-Slot-
// Backing getrieben (store_observe_value_handle → NodeChunkedStore::organ_observe_value_handle scannt die real
// gespeicherten Key-Bytes, NICHT mehr eine flache Roh-Puffer-Simulation). Die Zaehler folgen der echten Op je Record:
//   - total_access_count : Σ aller Slot-Zugriffe (n je observe-Runde)        — jede Strategie >= 1 Zugriff/Record
//   - indirect_deref_count : Σ der ZUSAETZLICHEN abhaengigen Derefs gegenueber Inline (Inline = 0; ExternalPool/
//                            VersionedPointer = n; ChainRef = 2n) — strategie-charakteristischer Indirektions-Aufwand
//   - version_tag_strips : Σ der MVCC-Version-Tag-Maskierungen (nur VersionedPointer = n; sonst 0) — ehrlich 0 fuer
//                          Strategien ohne Version-Tag
//   - peak_chain_depth : Hoechststand der Deref-Kette je Zugriff (Inline=1, 1-Deref-Strategien=2, ChainRef=3)
// KEIN erfundener Wert — jeder Zaehler folgt der per Strategie statisch bestimmten Indirektions-Charakteristik
// (is_inline() + name()-basierte Strategie-Klassifikation; Compile-Time, [[no-runtime-switch]]).
//
// Gating exakt nach Praezedenz (axis_05): snapshot_t/statistics()/reset() unter COMDARE_CE_ENABLE_STATISTICS.
// Bei OFF: value_access_scan = nackter Pass-Through (0 Footprint), ObservableAxis<...> = false → observe_all()
// faellt auf EmptyAxisSnapshot zurueck (Release-Pfad, korrekt).

#include "concepts/axis_14_value_handle_concept.hpp"
#include "concepts/axis_14_value_handle_cache_engine_permutation_concept.hpp"
#include "axis_14_value_handle_real_slot.hpp" // §4.3: REALE Pool/Version/Chain-Slot-Struktur (additiv, messneutral)
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::value_handle_axis {

/// ABI-taugliches ValueHandle-Snapshot (NUR uint64 → standard_layout + trivially_copyable, Cross-ABI-POD-mappbar).
struct ValueHandleSnapshot {
    std::uint64_t total_access_count   = 0; ///< Σ aller Slot-Zugriffe (n je observe-Runde)
    std::uint64_t indirect_deref_count = 0; ///< Σ der ZUSAETZLICHEN abhaengigen Derefs ggue. Inline (0/n/2n)
    std::uint64_t version_tag_strips   = 0; ///< Σ der MVCC-Version-Tag-Maskierungen (nur VersionedPointer)
    std::uint64_t peak_chain_depth     = 0; ///< Hoechststand der Deref-Kettentiefe je Zugriff (1/2/3)

    [[nodiscard]] bool operator==(ValueHandleSnapshot const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ValueHandleSnapshot>);
static_assert(std::is_trivially_copyable_v<ValueHandleSnapshot>);

/// ObservableAxis-Huelle: value_handle-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) → direkt als Anatomie-Member `ObservableValueHandle<S> m;` haltbar
/// (umgeht das `{}`-Aggregat-Init-Problem nackter Strategie-Wrapper, test_d_v42_probe2).
template <class Strategy>
    requires concepts::ValueHandleStrategy<Strategy>
class ObservableValueHandle {
public:
    using strategy_type = Strategy;

    // statische Forwarding-/Instrumentierungs-Hülle (KEIN GoF-Decorator: hält keine Komponenten-Instanz, kein Voll-Interface): Strategie-Inspektion durchgereicht (composition_registry / axis_path_serialization
    // rufen C::value_handle::name()).
    [[nodiscard]] static constexpr bool             is_inline() noexcept { return Strategy::is_inline(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept
        requires requires { Strategy::family_name(); }
    {
        return Strategy::family_name();
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept
        requires requires { Strategy::flag_suffix(); }
    {
        return Strategy::flag_suffix();
    }
    [[nodiscard]] static constexpr std::string_view get_compiler() noexcept
        requires requires { Strategy::get_compiler(); }
    {
        return Strategy::get_compiler();
    }

    /// STATIC Pass-Through (Drop-in-Kompatibilität): die Strategie-Methode wird unveraendert durchgereicht, damit
    /// die Huelle als value_handle-Slot die bestehenden seg19-Aufrufer NICHT bricht (abi_adapter.hpp T11
    /// `ValueHandle::value_access_scan`). Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t value_access_scan(unsigned char const* buf, std::size_t n,
                                                         std::size_t record_size) noexcept {
        return Strategy::value_access_scan(buf, n, record_size);
    }

    // ── §4.3 (User 2026-06-04) — REALER Pool/Version/Chain-Deref gegen die echte Slot-Struktur ──────────────────
    // Die Huelle haelt die ECHTE Slot-Struktur-Instanz (real_slot_), compile-zeit-selektiert je Strategie
    // (axis_14_value_handle_real_slot.hpp): Inline → EmptyRealSlot (leer, 0 Footprint, messneutral); ExternalPool/
    // ImmutableSharedRef → PoolValueSlot<false> (1 abh. Deref); VersionedPointer → PoolValueSlot<true> (MVCC-Tag-Strip);
    // ChainRef → ChainValueSlot (2 abh. Derefs). store_value/deref_value/clear_slots existieren NUR, wenn die
    // Strategie real ist (EmptyRealSlot traegt KEIN store_value → der abi_adapter-Build-Hook greift nicht → Inline
    // bleibt EXAKT unveraendert + messneutral, Leitplanke 1). EXAKT analog axis_filter (P5 #124) strat_/insert_key.

    using real_slot_type = real_slot_t<Strategy>;

    /// Build (Setup, NICHT gemessen): den (key,value) in die REALE Slot-Struktur legen (Pool-Indirektion / Chain-
    /// Knoten / versioniertem Pool-Eintrag). Existiert NUR fuer Nicht-Inline-Strategien (EmptyRealSlot ohne store_value).
    void store_value(std::uint64_t key, std::uint64_t value) noexcept
        requires requires(real_slot_type s) { s.store_value(key, value); }
    {
        real_slot_.store_value(key, value);
    }

    /// REALER Deref gegen die echte Slot-Struktur: Slot → Pool-Index/Chain-Head → Value (1 bzw. 2 abh. Indirektionen,
    /// VersionedPointer mit MVCC-Tag-Strip). Liefert true + *out_value gdw. der Key real gespeichert ist. Existiert
    /// NUR fuer Nicht-Inline-Strategien (Inline = direkter Slot-Read, KEINE Indirektion → kein Deref-Organ noetig).
    [[nodiscard]] bool deref_value(std::uint64_t key, std::uint64_t* out_value) const noexcept
        requires requires(real_slot_type const cs) {
            { cs.deref_value(key, out_value) } -> std::convertible_to<bool>;
        }
    {
        return real_slot_.deref_value(key, out_value);
    }

    /// Leert die REALE Slot-Struktur (Memento/tier_clear-Symmetrie). NUR fuer Nicht-Inline (EmptyRealSlot::clear
    /// ist no-op, traegt aber clear() → der requires greift; Inline bleibt 0-Footprint).
    void clear_slots() noexcept
        requires requires(real_slot_type s) { s.clear(); }
    {
        real_slot_.clear();
    }

    /// Lesezugriff auf die reale Slot-Struktur (Test-Verifikation + Diagnose: slot_count/pool_size/chain_nodes).
    [[nodiscard]] real_slot_type const& real_slot() const noexcept { return real_slot_; }
    [[nodiscard]] real_slot_type&       real_slot() noexcept { return real_slot_; }

    /// Bit-exakter Vergleich der REALEN Slot-Struktur (Memento-Verifikation, §4.3, analog ObservableFilter). Vergleicht
    /// NUR die Struktur (real_slot_), NICHT die diagnostischen Stats — der Memento-Vertrag betrifft das Value-Backing.
    [[nodiscard]] bool operator==(ObservableValueHandle const& o) const noexcept { return real_slot_ == o.real_slot_; }

private:
    // Compile-Time Indirektions-Charakteristik je Strategie (statisch, [[no-runtime-switch]]):
    //   depth   = Tiefe der Deref-Kette je Zugriff (Inline=1; 1-Deref=2; ChainRef=3)
    //   strips  = Version-Tag-Maskierungen je Zugriff (nur VersionedPointer=1)
    // Ableitung aus is_inline() + name() (die name()-Strings sind die kanonischen Strategie-Identifier, oben
    // value_access_scan-dokumentiert). Unbekannte Nicht-Inline-Strategie → konservativ 1 Indirektion (depth 2).
    [[nodiscard]] static constexpr std::uint64_t chain_depth_() noexcept {
        if (Strategy::is_inline()) return 1; // direkter Slot-Read
        std::string_view const nm = Strategy::name();
        if (nm == "value_handle_chain_ref") return 3; // 2 abhaengige Derefs (Head → Value)
        return 2;                                     // 1 abhaengige Deref (Pool/MVCC/SharedRef)
    }
    [[nodiscard]] static constexpr bool has_version_tag_() noexcept {
        return Strategy::name() == "value_handle_versioned_pointer"; // MVCC-Tag-Strip vor jedem Deref
    }

public:
    /// Mess-Kopplung (der eigentliche „Driver", Instanz): treibt den ECHTEN value_access_scan ueber die uebergebenen
    /// Slot-Bytes (vom container_-Slot-Backing, NICHT mehr Roh-Puffer-Simulation) + trackt die strategie-
    /// charakteristische Indirektion. `record_size` = Stride im Slot-Backing. Der Observer-Treiber (abi_adapter::
    /// fill_observer_v3 via store_observe_value_handle) ruft dies → die Value-Zugriffs-Aktivitaet wird observable.
    [[nodiscard]] std::uint64_t observe_value_handle(unsigned char const* buf, std::size_t n,
                                                     std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::value_access_scan(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        std::uint64_t const depth = chain_depth_();
        stats_.total_access_count += static_cast<std::uint64_t>(n);
        stats_.indirect_deref_count += static_cast<std::uint64_t>(n) * (depth - 1); // ZUSAETZLICH ggue. Inline
        if (has_version_tag_()) stats_.version_tag_strips += static_cast<std::uint64_t>(n);
        if (depth > stats_.peak_chain_depth) stats_.peak_chain_depth = depth;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = ValueHandleSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif

private:
    // ── §4.3 — REALE Slot-Struktur-Instanz (IMMER vorhanden, auch ohne Statistics-Define) ──────────────────────
    // Traegt das echte Pool-/Versioned-Pool-/Chain-Slot-Backing (Nicht-Inline) bzw. EmptyRealSlot (Inline, 0
    // Footprint). std::vector-basiert (Pool/Chain) bzw. leer → copy-constructible + copy-assignable + operator==
    // → ObservableValueHandle kopierbar/vergleichbar → fuer den symmetrischen Memento (saved_vh_ in tier_save_all/
    // tier_rollback_all) snapshot-faehig (R1, Leitplanke 3). Default-konstruiert = leer (None-aequivalente Baseline).
    real_slot_type real_slot_{};
};

} // namespace comdare::cache_engine::value_handle_axis
