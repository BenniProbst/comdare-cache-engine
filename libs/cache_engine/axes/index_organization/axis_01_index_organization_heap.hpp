#pragma once
// V41.F.6.1.R7.5.h axis_01 HeapIndexOrganization (baseline kein Index)

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

/// HeapIndexOrganization — Storage-Order = Insert-Order, KEIN Index (Baseline).
/// PostgreSQL Heap (unbenannt), HBase Heap-Tables. Lookup = Full-Scan O(n).
class HeapIndexOrganization : public IndexOrganizationStrategyBase<HeapIndexOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::storage_order_tag;
    using family_id = std::integral_constant<int, 0>;

    static constexpr bool enabled = flags::heap_enabled;

    [[nodiscard]] static constexpr bool             is_clustered() noexcept { return false; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return false; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "index_org_heap"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::index_organization::HeapIndexOrganization",
                                  "axes/index_organization/axis_01_index_organization_heap.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "HeapIndexOrganization (no index, storage=insert-order, baseline)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "HEAP"; }
    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (index_organization-Achse, Pfad-A-operativ, T13).
    // Distinktes Zugriffsmuster je Strategie: Heap = UNORDERED — kein Index (Baseline), Storage =
    // Insert-Order. Lookup ist O(n) Full-Scan MIT Predicate-Evaluation pro Record (jedes Record muss
    // gegen den Suchschluessel geprueft werden, da keine Sortierung den Abbruch erlaubt). Modelliert
    // durch sequentiellen Scan + datenabhaengigen Vergleich/Branch je Record. Real distinkt gegenueber
    // Clustered (reiner Summen-Scan ohne Predicate) durch die zusaetzliche, datenabhaengige Vergleichs-
    // last und Branch-Misprediction — KEINE konstante Zeit. Der „Suchschluessel" ist synthetisch fix,
    // die daraus folgende Branch-/Compare-Last aber real und strategie-typisch.
    [[nodiscard]] static std::uint64_t index_org_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        constexpr std::uint32_t kMatchProbe = 0x55555555u; // synthetischer Full-Scan-Suchschluessel
        std::uint64_t           s           = 0;
        std::uint64_t           matches     = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // Heap: unordered Full-Scan, Insert-Order
            s += v;
            if ((v ^ kMatchProbe) < v) { // datenabhaengiger Predicate-Branch (kein Frueh-Abbruch ohne Index)
                ++matches;
            }
        }
        return s + matches;
    }

    // honest-100% (#24 Option A) — Zaehl-Schwester zu index_org_scan (Observer-Pfad B, index_org_observe): identischer
    // O(n) Full-Scan mit datenabhaengigem Predicate-Branch je Record, meldet aber die REAL ausgefuehrte Predicate-Eval-
    // Zahl (== n) zurueck statt sie aus is_clustered() zu synthetisieren. Heap hat KEINEN Sekundaer-Index →
    // indirect_lookups bleibt 0. matches bleibt in return s+matches gefaltet, damit der Compare nicht wegoptimiert
    // wird (sonst Rueckfall in Synthese). Signatur bewusst != kanonische index_org_scan-Form (kein Mess-Kern; die
    // Kern-Reinheit T13 in test_striktheit_scan_kernel_purity prueft ausschliesslich index_org_scan).
    [[nodiscard]] static std::uint64_t index_org_scan_counted(unsigned char const* buf, std::size_t n,
                                                              std::size_t record_size, std::uint64_t& predicate_evals,
                                                              std::uint64_t& indirect_lookups) noexcept {
        (void)indirect_lookups;                            // Heap: kein Sekundaer-Index → keine Indirektion (bleibt 0)
        constexpr std::uint32_t kMatchProbe = 0x55555555u; // synthetischer Full-Scan-Suchschluessel
        std::uint64_t           s           = 0;
        std::uint64_t           matches     = 0;
        for (std::size_t i = 0; i < n; ++i) {
            std::uint32_t v;
            std::memcpy(&v, buf + i * record_size, sizeof(v)); // Heap: unordered Full-Scan, Insert-Order
            if ((v ^ kMatchProbe) < v) {                       // datenabhaengiger Predicate-Branch (kein Frueh-Abbruch)
                ++matches;
            }
            ++predicate_evals; // EIN real ausgefuehrter Predicate-Eval je gescanntem Record
            s += v;
        }
        return s + matches;
    }
};

} // namespace comdare::cache_engine::index_organization

namespace comdare::cache_engine::index_organization {
static_assert(concepts::IndexOrganizationStrategy<HeapIndexOrganization>);
static_assert(concepts::CacheEnginePermutationStrategy<HeapIndexOrganization>);
} // namespace comdare::cache_engine::index_organization
