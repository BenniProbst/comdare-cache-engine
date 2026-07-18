#pragma once
// V41.F.6.1.R7.5.h axis_01 IotIndexOrganization (IOT, Daten in Leaf-Pages)

#include "axis_01_index_organization_strategy_base.hpp"
#include "axis_01_index_organization_subaxes_io1_to_io3.hpp"
#include "concepts/axis_01_index_organization_cache_engine_permutation_concept.hpp"
#include <axes/index_organization/axis_01_index_organization_flags.hpp>
#include <topics/search_engine/concepts/topic_search_engine_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::index_organization {

/// IotIndexOrganization — Daten direkt in Index-Leaf-Pages eingebettet (Oracle IOT).
/// Spart Pointer-Indirektion vs NonClustered. Aequivalent zu SQL Server
/// CLUSTERED INDEX (Daten in B+Tree-Leaves). Standard fuer ART/HOT/Wormhole
/// (Daten direkt im Trie-Knoten).
class IotIndexOrganization : public IndexOrganizationStrategyBase<IotIndexOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::data_embedding_tag;
    using family_id = std::integral_constant<int, 3>;

    static constexpr bool enabled = flags::index_organized_table_enabled;

    [[nodiscard]] static constexpr bool             is_clustered() noexcept { return true; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return true; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "index_org_index_organized_table"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::index_organization::IotIndexOrganization",
                                  "axes/index_organization/axis_01_index_organization_index_organized_table.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "IotIndexOrganization (Oracle IOT, Daten in B+Tree-Leaves, ART/HOT-style)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "INDEX_ORGANIZED_TABLE"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (index_organization-Achse, Pfad-A-operativ, T13).
    // Distinktes Zugriffsmuster je Strategie: IOT = EMBEDDED — Daten direkt in den Index-Leaf-Pages,
    // KEIN Pointer-Hop (data_embedded_in_leaf()==true). Der Scan liest pro Record sequentiell SOWOHL
    // den eingebetteten Index-Key ALS AUCH das Daten-Feld (zwei Felder/Record), modelliert die
    // hoehere Per-Record-Touch-Last des fetten Leaf-Layouts. Sequentiell wie Clustered, aber mit
    // realer Zusatz-Last durch das zweite eingebettete Feld — KEINE konstante Zeit. Bei record_size<8
    // faellt der embedded Key auf Feld-0 zurueck (kein OOB).
    [[nodiscard]] static std::uint64_t index_org_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::size_t const key_off = (record_size >= 2u * sizeof(std::uint32_t)) ? sizeof(std::uint32_t) : 0u;
        std::uint64_t     s       = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t key;
            std::uint32_t data;
            std::memcpy(&key, buf + i * record_size, sizeof(key));             // IOT: eingebetteter Index-Key
            std::memcpy(&data, buf + i * record_size + key_off, sizeof(data)); // IOT: Daten im selben Leaf-Slot
            s += key + data;
        }
        return s;
    }

    // honest-100% (#24 Option A) — Zaehl-Schwester zu index_org_scan (Observer-Pfad B): Daten im Leaf eingebettet
    // (data_embedded_in_leaf()==true), KEIN Pointer-Hop, sequentiell wie Clustered → beide Zaehler bleiben GENUIN 0.
    // Behebt die frueher aus has_secondary_indexes()==true synthetisierte indirect_lookups=n-FABRIKATION: laut
    // Anhang D (Iot) „spart IOT die Pointer-Indirektion" → indirect_lookups MUSS 0 sein (thesis-treu, honest-100%).
    [[nodiscard]] static std::uint64_t index_org_scan_counted(unsigned char const* buf, std::size_t n,
                                                              std::size_t record_size, std::uint64_t& predicate_evals,
                                                              std::uint64_t& indirect_lookups) noexcept {
        (void)predicate_evals;  // IOT: sequentieller Embedded-Scan → kein Predicate (bleibt 0)
        (void)indirect_lookups; // IOT: data_embedded_in_leaf → kein Pointer-Hop (bleibt 0)
        std::size_t const key_off = (record_size >= 2u * sizeof(std::uint32_t)) ? sizeof(std::uint32_t) : 0u;
        std::uint64_t     s       = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t key;
            std::uint32_t data;
            std::memcpy(&key, buf + i * record_size, sizeof(key));             // IOT: eingebetteter Index-Key
            std::memcpy(&data, buf + i * record_size + key_off, sizeof(data)); // IOT: Daten im selben Leaf-Slot
            s += key + data;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::index_organization

namespace comdare::cache_engine::index_organization {
static_assert(concepts::IndexOrganizationStrategy<IotIndexOrganization>);
static_assert(concepts::CacheEnginePermutationStrategy<IotIndexOrganization>);
} // namespace comdare::cache_engine::index_organization
