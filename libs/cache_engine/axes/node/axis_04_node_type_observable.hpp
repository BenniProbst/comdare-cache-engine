#pragma once
// V42 L-74c Composition-Driver — ObservableNodeType<Strategy>: ObservableAxis-Huelle um eine node_type-
// Strategie (axis_04). Viertes + letztes Exemplar des Huellen-Musters (telemetry/memory_layout/serialization).
// Node256NodeType etc. tragen `node_find_scan()` (KF-6, ART-Format-divergenter Lookup) als static, haben aber
// kein statistics(). Die Huelle reicht die statische API durch (max_capacity/name/topic_tag — Pflicht fuer
// NodeTypeStrategy + Einsatz als N in ComposedStore<N,L,A>) + bietet observe_node_find() als Instanz-Driver.
//
// @topic nodes @achse 04 @saeule 2 (Per-Achsen-Observer) @task V42-L-74c
//
// **Achsen-Semantik (treu):** Die node_type-Achse misst den ART-Format-divergenten Lookup-Aufwand (Node256 =
// direkt-adressiert, Node48 = indirekt, etc.). Die Counter (find_count/keys_stored/queries_run/last_checksum)
// machen die Lookup-Aktivitaet observable; der Format-Latenz-Unterschied bleibt Wall-Clock (Pfad B).

#include "concepts/axis_04_node_type_concept.hpp"
#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace comdare::cache_engine::node {

/// ABI-taugliches Node-Type-Snapshot (standard_layout + trivially_copyable).
struct NodeTypeSnapshot {
    std::uint64_t find_count    = 0; ///< Anzahl observe_node_find-Aufrufe
    std::uint64_t keys_stored   = 0; ///< kumulierte gespeicherte Key-Zahl (min(n,cap))
    std::uint64_t queries_run   = 0; ///< kumulierte Query-Zahl
    std::uint64_t last_checksum = 0; ///< letztes node_find_scan-Ergebnis (Format-divergente Pruefsumme)

    [[nodiscard]] bool operator==(NodeTypeSnapshot const&) const noexcept = default;
};

/// ObservableAxis-Huelle: node_type-Strategie + Per-Achsen-Mess-Mechanik (gegated). KEIN Aggregat.
/// Als N in ComposedStore<N,L,A> einsetzbar (reicht max_capacity/name/topic_tag durch).
template <class Strategy>
    requires concepts::NodeTypeStrategy<Strategy>
class ObservableNodeType {
public:
    using strategy_type = Strategy;
    using topic_tag     = typename Strategy::topic_tag; // NodesComponent → NodeTypeStrategy erfuellt

    // Statische API durchgereicht (ComposedStore nutzt N::max_capacity() constexpr + N::name()).
    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return Strategy::max_capacity(); }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return Strategy::name(); }
    static constexpr std::string_view               algo_version =
        Strategy::algo_version; // #50 Caching: algo_version-Weiterleitung (Organ-Provenienz, reflect_versions)
    // INC-A #6: per-Organ-Codegen-Lokation. Der Wrapper-Typ ist die ObservableNodeType-Huelle (der
    // Registry-Enabled-Eintrag ist ObservableNodeType<Strategy>); der Header ist diese Huellen-Datei.
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::node::ObservableNodeType",
                                  "axes/node/axis_04_node_type_observable.hpp");
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

    /// STATIC Pass-Through (Drop-in): node_find_scan unveraendert durchgereicht. Trackt NICHT.
    [[nodiscard]] static std::uint64_t node_find_scan(std::uint8_t const* stored, std::size_t n,
                                                      std::uint8_t const* queries, std::size_t q) noexcept {
        return Strategy::node_find_scan(stored, n, queries, q);
    }

    /// Mess-Kopplung (Instanz-Driver): delegiert + trackt. Der Observer-Treiber ruft dies.
    [[nodiscard]] std::uint64_t observe_node_find(std::uint8_t const* stored, std::size_t n,
                                                  std::uint8_t const* queries, std::size_t q) noexcept {
        std::uint64_t const checksum = Strategy::node_find_scan(stored, n, queries, q);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.find_count;
        std::size_t const cap = Strategy::max_capacity();
        stats_.keys_stored += static_cast<std::uint64_t>((n < cap) ? n : cap);
        stats_.queries_run += static_cast<std::uint64_t>(q);
        stats_.last_checksum = checksum;
#endif
        return checksum;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = NodeTypeSnapshot;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    void                     reset() noexcept { stats_ = {}; }

private:
    snapshot_t stats_{};
#endif
};

} // namespace comdare::cache_engine::node
