#pragma once
// V41.F.6.1.R7.5.h axis_01 ClusteredIndexOrganization (Index-Order = Storage-Order)

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

namespace comdare::cache_engine::index_organization {

/// ClusteredIndexOrganization — Storage-Order entspricht Index-Order (1 Primary Key).
/// MySQL InnoDB Default, PostgreSQL CLUSTER. Optimal fuer Range-Scans entlang
/// Primary-Key (sequentielle Disk-Reads). Daten getrennt vom Index-Tree.
class ClusteredIndexOrganization : public IndexOrganizationStrategyBase<ClusteredIndexOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::storage_order_tag;
    using family_id = std::integral_constant<int, 1>;

    static constexpr bool enabled = flags::clustered_enabled;

    [[nodiscard]] static constexpr bool             is_clustered() noexcept { return true; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return false; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "index_org_clustered"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "ClusteredIndexOrganization (Index-Order = Storage-Order, MySQL InnoDB)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "CLUSTERED"; }

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (index_organization-Achse, Pfad-A-operativ, T13).
    // Distinktes Zugriffsmuster je Strategie: Clustered = SEQUENTIAL — Index-Order == Storage-Order,
    // Range-Scan liest Records linear vorwaerts (cache-freundlich, niedrige Latenz). Real distinkt
    // gegenueber NonClustered (random) durch HW-Prefetch + L1/L2-Hit-Verhalten; KEINE konstante Zeit.
    [[nodiscard]] static std::uint64_t index_org_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // Clustered: sequential, Storage-Order = Index-Order
            s += v;
        }
        return s;
    }

    // honest-100% (#24 Option A) — Zaehl-Schwester zu index_org_scan (Observer-Pfad B): reiner sequentieller
    // Summen-Scan OHNE Predicate und OHNE Indirektion. Beide Zaehler bleiben GENUIN 0 (nicht aus Flags abgeleitet,
    // sondern weil der Scan-Code keine solche Op ausfuehrt). Return == plain index_org_scan.
    [[nodiscard]] static std::uint64_t index_org_scan_counted(unsigned char const* buf, std::size_t n,
                                                              std::size_t record_size, std::uint64_t& predicate_evals,
                                                              std::uint64_t& indirect_lookups) noexcept {
        (void)predicate_evals;  // Clustered: sequentieller Summen-Scan → kein Predicate (bleibt 0)
        (void)indirect_lookups; // Clustered: Index-Order == Storage-Order → keine Indirektion (bleibt 0)
        std::uint64_t s = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // Clustered: sequential, Storage-Order = Index-Order
            s += v;
        }
        return s;
    }
};

} // namespace comdare::cache_engine::index_organization

namespace comdare::cache_engine::index_organization {
static_assert(concepts::IndexOrganizationStrategy<ClusteredIndexOrganization>);
static_assert(concepts::CacheEnginePermutationStrategy<ClusteredIndexOrganization>);
} // namespace comdare::cache_engine::index_organization
