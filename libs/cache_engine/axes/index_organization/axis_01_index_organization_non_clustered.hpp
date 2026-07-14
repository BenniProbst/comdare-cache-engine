#pragma once
// V41.F.6.1.R7.5.h axis_01 NonClusteredIndexOrganization (Secondary Index)

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

/// NonClusteredIndexOrganization — Index-Order != Storage-Order, N Secondary-Indizes.
/// SQL Server NONCLUSTERED, PostgreSQL CREATE INDEX. Lookup ueber Index +
/// zusaetzlicher Pointer-Hop zur Daten-Row. Standard fuer OLTP-Workloads mit
/// multiplen Suchfeldern.
class NonClusteredIndexOrganization : public IndexOrganizationStrategyBase<NonClusteredIndexOrganization> {
public:
    using topic_tag = ::comdare::cache_engine::search_engine::concepts::SearchEngineTopicTag;
    using axis_tag  = subaxes::index_count_tag;
    using family_id = std::integral_constant<int, 2>;

    static constexpr bool enabled = flags::non_clustered_enabled;

    [[nodiscard]] static constexpr bool             is_clustered() noexcept { return false; }
    [[nodiscard]] static constexpr bool             has_secondary_indexes() noexcept { return true; }
    [[nodiscard]] static constexpr bool             data_embedded_in_leaf() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "index_org_non_clustered"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::index_organization::NonClusteredIndexOrganization",
                                  "axes/index_organization/axis_01_index_organization_non_clustered.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "NonClusteredIndexOrganization (Secondary Index, N-pro-Tabelle, SQL Server/PostgreSQL)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "NON_CLUSTERED"; }

    // V41.F.6.1 — verhaltens-tragende Laufzeit-API (index_organization-Achse, Pfad-A-operativ, T13).
    // Distinktes Zugriffsmuster je Strategie: NonClustered = RANDOM-STRIDE — Index-Order != Storage-Order,
    // Lookup folgt einem Secondary-Index und springt per Pointer-Hop in unsortierter Storage-Order.
    // Modelliert via deterministischem LCG-Index (kein externer RNG, reproduzierbar). Die durch den
    // Random-Stride erzwungenen Cache-Misses (TLB/L2) erzeugen eine REALE, strategie-abhaengige
    // Mehrlatenz gegenueber Clustered/IOT — KEINE konstante/erfundene Zeit. Der Index-Hop ist
    // synthetisch (kein echter Secondary-B-Tree), die daraus folgende Speicher-Latenz aber real.
    [[nodiscard]] static std::uint64_t index_org_scan(unsigned char const* buf, std::size_t n,
                                                      std::size_t record_size) noexcept {
        std::uint64_t s   = 0;
        std::uint64_t idx = 0x9E3779B97F4A7C15ull; // Startzustand (Fibonacci-Hash), LCG-getrieben
        for (std::size_t i = 0; i < n; ++i) {
            idx                 = idx * 6364136223846793005ull + 1442695040888963407ull; // LCG (Knuth MMIX)
            std::size_t const r = (n == 0) ? 0 : static_cast<std::size_t>((idx >> 33) % n);
            std::uint32_t     v;
            std::memcpy(&v, buf + r * record_size, sizeof(v)); // NonClustered: random Storage-Hop
            s += v;
        }
        return s;
    }

    // honest-100% (#24 Option A, 2026-07-13) — Zaehl-Schwester zu index_org_scan (Observer-Pfad B): je Lookup ein
    // REALER Pointer-Hop in unsortierte Storage-Order (indirect_lookups++ == n) UND ein REALER Residual-Predicate-
    // Vergleich auf der geholten Daten-Row (predicate_evals++ == n) — analog Bookmark-/RID-Lookup gefolgt von einem
    // Residual-Filter (SQL Server NONCLUSTERED / PostgreSQL Index-Scan). Beide Zaehler == tatsaechlich ausgefuehrte
    // Ops, KEIN Flag-Wert. matches bleibt in return s+matches gefaltet, damit der Residual-Compare nicht wegoptimiert
    // wird (sonst Rueckfall in Synthese). Thesis-Beleg: Anhang D, NonClusteredIndexOrganization (Residual-Predicate
    // pro geholtem Record). Signatur bewusst != kanonische index_org_scan-Form (die Kern-Reinheit T13 prueft nur
    // index_org_scan).
    [[nodiscard]] static std::uint64_t index_org_scan_counted(unsigned char const* buf, std::size_t n,
                                                              std::size_t record_size, std::uint64_t& predicate_evals,
                                                              std::uint64_t& indirect_lookups) noexcept {
        constexpr std::uint32_t kResidualProbe = 0x55555555u; // Residual-Predicate-Suchschluessel (nach RID-Lookup)
        std::uint64_t           s              = 0;
        std::uint64_t           matches        = 0;
        std::uint64_t           idx            = 0x9E3779B97F4A7C15ull; // Startzustand (Fibonacci-Hash), LCG-getrieben
        for (std::size_t i = 0; i < n; ++i) {
            idx                 = idx * 6364136223846793005ull + 1442695040888963407ull; // LCG (Knuth MMIX)
            std::size_t const r = (n == 0) ? 0 : static_cast<std::size_t>((idx >> 33) % n);
            std::uint32_t     v;
            std::memcpy(&v, buf + r * record_size, sizeof(v)); // NonClustered: random Storage-Hop (Pointer-Indirektion)
            ++indirect_lookups;             // EIN real ausgefuehrter Index-Indirektions-Hop je Lookup
            if ((v ^ kResidualProbe) < v) { // Residual-Predicate auf der geholten Daten-Row
                ++matches;
            }
            ++predicate_evals; // EIN real ausgefuehrter Residual-Predicate-Eval je geholtem Record
            s += v;
        }
        return s + matches;
    }
};

} // namespace comdare::cache_engine::index_organization

namespace comdare::cache_engine::index_organization {
static_assert(concepts::IndexOrganizationStrategy<NonClusteredIndexOrganization>);
static_assert(concepts::CacheEnginePermutationStrategy<NonClusteredIndexOrganization>);
} // namespace comdare::cache_engine::index_organization
