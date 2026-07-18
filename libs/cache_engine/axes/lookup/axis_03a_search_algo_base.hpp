#pragma once
// V41.F.6.1 axis_03a_search_algo CRTP-Basis + Concept-Guard (2026-05-26)
//
// @topic traversal @achse 03a

#include "concepts/axis_03a_search_algo_concept.hpp"
#include <topics/axis_base.hpp>
#include <topics/organ_axis.hpp>               // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include <axes/cacheline/cacheline_config.hpp> // KF-5: per-Organ Cache-Line-Unterachse

#include <type_traits>

namespace comdare::cache_engine::lookup {

/**
 * @brief SearchAlgoBase — CRTP-Basis fuer 03a-Wrapper
 *
 * Concept-Guard via static_assert im Konstruktor (CRTP-Henne-Ei-Pattern aus
 * Allocator-Achse).
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
// KF-5 (2026-06-02): defaulted NTTP CacheLineCfg + CacheLineAware<Cfg> → jeder Such-Algo-Organ ist
// cacheline-fähig. Default {} = unverändert (nicht-brechend, ODR-sicher).
template <typename Derived, ::comdare::cache_engine::cacheline::CacheLineConfig CacheLineCfg =
                                ::comdare::cache_engine::cacheline::CacheLineConfig{}>
class SearchAlgoBase : public ::comdare::cache_engine::topics::OrganAxis<Derived>,
                       public ::comdare::cache_engine::cacheline::CacheLineAware<CacheLineCfg> {
protected:
    SearchAlgoBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");
        static_assert(concepts::SearchAlgoVariant<Derived>,
                      "Pflicht: Derived muss SearchAlgoVariant erfuellen "
                      "(insert/lookup/erase/occupied_count/density_percent/clear)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + "
                      "is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

} // namespace comdare::cache_engine::lookup
