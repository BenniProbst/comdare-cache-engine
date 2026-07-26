#pragma once
// STRUKT-R ORG-18 ObservablePersistenceTarget<Strategy> -- ObservableAxis-Huelle um eine
// persistence_target-Strategie. Vorbild 1:1 axes/io_dispatch/axis_io_dispatch_observable.hpp.
//
// @topic io @achse persistence_target @saeule 2 (Per-Achsen-Observer)
//
// Warum eine Huelle: die Strategie selbst traegt die verhaltens-tragende static Op
// persistence_writeback_scan(), hat aber KEIN statistics()/snapshot_t (ist KEIN ObservableAxis). Die
// Mess-Mechanik gehoert daher hierher: die Huelle reicht die static Op unveraendert durch (Seg-Timer-
// Aufrufer im abi_adapter bleiben heil) UND ergaenzt einen Instanz-Driver observe_writeback(), der trackt.
//
// Memory-Direktive (reference_observable_wrapper_must_forward_concept_members): die Huelle MUSS alle
// Concept-Member der Strategie forwarden, sonst ist sie als Kompositions-Slot nicht einsetzbar
// (composition_registry / axis_path_serialization rufen C::persistence_target::name()).
//
// EHRLICHKEIT: die Zaehler folgen der echten Op-Schleife, kein erfundener Wert. Sie beschreiben
// Rueckschreib-VORBEREITUNG (Staging), nicht Platten-Zeit -- siehe die Kopf-Deklaration in
// axis_persistence_target_disk_writeback.hpp. has_device_writeback_path() wird mit-durchgereicht, damit
// die Mess-/Report-Seite die Staging-Natur des Segments maschinenlesbar erkennt.
//
// Gating exakt nach Praezedenz (axis_io_dispatch_observable): snapshot_t/statistics()/reset() unter
// COMDARE_CE_ENABLE_STATISTICS. Bei OFF: persistence_writeback_scan = nackter Pass-Through (0 Footprint),
// ObservableAxis<...> = false -> observe_all() faellt auf EmptyAxisSnapshot zurueck (Release-Pfad).

#include "concepts/axis_persistence_target_concept.hpp"
#include "concepts/axis_persistence_target_cache_engine_permutation_concept.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::persistence_target {

/// ABI-taugliches Snapshot (NUR uint64 -> standard_layout + trivially_copyable, Cross-ABI-POD-mappbar).
struct PersistenceTargetSnapshot {
    std::uint64_t writeback_rounds = 0; ///< Anzahl observe_writeback-Aufrufe (Rueckschreib-Runden)
    std::uint64_t bytes_staged     = 0; ///< kumulierte in den Staging-Puffer gesammelte Record-Bytes
    std::uint64_t records_staged   = 0; ///< kumulierte gestagte Records (Summe n)
    std::uint64_t device_flushes   = 0; ///< echte Geraete-Flushes; bleibt 0, solange kein Geraete-Pfad existiert
    std::uint64_t last_checksum    = 0; ///< letztes Scan-Ergebnis (Korrektheits-/Anti-Wegopt-Anker)

    [[nodiscard]] bool operator==(PersistenceTargetSnapshot const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<PersistenceTargetSnapshot>);
static_assert(std::is_trivially_copyable_v<PersistenceTargetSnapshot>);

/// ObservableAxis-Huelle: persistence_target-Strategie + Per-Achsen-Mess-Mechanik (gegated).
/// KEIN Aggregat (private member + Methoden) -> direkt als Anatomie-Member haltbar (umgeht das
/// {}-Aggregat-Init-Problem nackter Strategie-Wrapper, test_d_v42_probe2).
template <class Strategy>
    requires concepts::PersistenceTargetStrategy<Strategy>
class ObservablePersistenceTarget {
public:
    using strategy_type = Strategy;
    // topic_tag durchgereicht -> die Huelle erfuellt IoComponent/PersistenceTargetStrategy und ist als
    // persistence_target-Slot einsetzbar.
    using topic_tag = typename Strategy::topic_tag;

    // axis_tag/family_id/enabled durchgereicht -> die Huelle erfuellt AUCH das VOLLE
    // CacheEnginePermutationStrategy-Concept, ist also ein echter Drop-in-Slot-Wert.
    // BELEGTER GRUND (Memory reference_observable_wrapper_must_forward_concept_members): ohne diese drei
    // scheitert das Permutations-Concept an "the required type typename P::axis_tag is invalid" -- genau so
    // verifiziert am 26.07. mit c++ -std=c++23. axes/io_dispatch/axis_io_dispatch_observable.hpp reicht sie
    // BIS HEUTE NICHT durch (ObservableIoDispatch erfuellt das Voll-Concept daher nicht); hier bewusst
    // vollstaendig, weil ein Kompositions-Slot-Wert die volle Pflicht-Oberflaeche tragen soll.
    using axis_tag  = typename Strategy::axis_tag;
    using family_id = typename Strategy::family_id;

    static constexpr bool enabled = Strategy::enabled;

    // Statische Forwarding-/Instrumentierungs-Huelle (KEIN GoF-Decorator: haelt keine Komponenten-Instanz,
    // kein Voll-Interface): Strategie-Inspektion durchgereicht.
    [[nodiscard]] static constexpr bool             writes_back_to_disk() noexcept { return Strategy::writes_back_to_disk(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    static constexpr std::string_view               algo_version =
        Strategy::algo_version; // #50 Caching: algo_version-Weiterleitung (Organ-Provenienz, reflect_versions)
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
    /// Ehrlichkeits-Marke durchgereicht (nur wo die Strategie sie traegt): meldet, ob hinter dem Baustein
    /// ein echter Geraete-Schreibpfad steht. MemoryOnlyTarget traegt sie nicht (hat keinen Pfad zu melden).
    [[nodiscard]] static constexpr bool has_device_writeback_path() noexcept
        requires requires { Strategy::has_device_writeback_path(); }
    {
        return Strategy::has_device_writeback_path();
    }

    /// STATIC Pass-Through (Drop-in-Kompatibilitaet): die Strategie-Methode wird unveraendert durchgereicht,
    /// damit die Huelle als persistence_target-Slot die Seg-Timer-Aufrufer im abi_adapter NICHT bricht.
    /// Diese Variante trackt NICHT (static, kein Instanz-State).
    [[nodiscard]] static std::uint64_t persistence_writeback_scan(unsigned char const* buf, std::size_t n,
                                                                 std::size_t record_size) noexcept {
        return Strategy::persistence_writeback_scan(buf, n, record_size);
    }

    /// Mess-Kopplung (der eigentliche Driver, Instanz): delegiert an die Strategie + trackt. Der
    /// Observer-Treiber (abi_adapter::fill_observer_v3 / Tier-Op-Kopplung) ruft dies -> die
    /// Rueckschreib-Vorbereitung wird observable. Getrennt von der static-Variante, weil die bestehenden
    /// Seg-Timer-Aufrufer static bleiben muessen.
    [[nodiscard]] std::uint64_t observe_writeback(unsigned char const* buf, std::size_t n,
                                                 std::size_t record_size) noexcept {
        std::uint64_t const checksum = Strategy::persistence_writeback_scan(buf, n, record_size);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.writeback_rounds;
        stats_.records_staged += static_cast<std::uint64_t>(n);
        // Nur ein Baustein MIT Rueckschreib-Pflicht stagt ueberhaupt Bytes; MemoryOnlyTarget hat keinen
        // Pfad (persistence_writeback_scan == return 0) -> ehrlich 0 statt einer gerechneten Phantom-Zahl.
        if (Strategy::writes_back_to_disk())
            stats_.bytes_staged += static_cast<std::uint64_t>(n) * static_cast<std::uint64_t>(record_size);
        // device_flushes bleibt bewusst unberuehrt: solange has_device_writeback_path() false meldet, gibt
        // es keinen Geraete-Flush zu zaehlen. Eine Zahl hier waere eine Messwert-Luege.
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = PersistenceTargetSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

} // namespace comdare::cache_engine::persistence_target
