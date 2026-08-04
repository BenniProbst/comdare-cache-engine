#pragma once
// V41.F.6.1 axis_q1_queuing CRTP-Basis + Concept-Guard (2026-05-26)
//
// @topic queuing
// @achse Q1 buffer_strategy

#include "concepts/axis_q1_queuing_concept.hpp"
#include <topics/organ_axis.hpp>               // INC-1a: OrganAxis<Derived>-Dach (axis_kind()==organ)
#include <topics/organ_axis_error_classes.hpp> // FK-5: der Fehlerraum neben dem Versionsraum
#include "../../axis_base.hpp"

#include <type_traits>

namespace comdare::cache_engine::queuing::axis_q1_queuing {

/**
 * @brief BufferStrategyBase — CRTP-Basis fuer Q1-Wrapper
 *
 * Concept-Guard via static_assert im Konstruktor (CRTP-Henne-Ei-Pattern aus
 * Allocator-Achse: Concept-Check in `requires` Template-Klausel funktioniert
 * NICHT zur Vererbung — daher static_assert hier).
 *
 * Erbt von ::topics::AxisBase fuer cross-axis Pflicht-Property get_compiler()
 * (Default "original", per Wrapper ueberschreibbar).
 */
template <typename Derived>
class BufferStrategyBase : public ::comdare::cache_engine::topics::OrganAxis<Derived> {
public:
    /// FK-5 (A15 EBENE 4, Fehlerraum) -- WELCHE D2-Fehlerklassen diese Organ-Achse ueberhaupt
    /// hervorbringen kann. Deklariert an DERSELBEN Stelle, die schon algo_version erzwingt (K2-Muster):
    /// eine Stelle je Achse statt einer Zeile je Varianten-Datei.
    /// Boden: die Puffer-Strategie traegt reale Ein-/Ausgaben.
    [[nodiscard]] static constexpr auto error_classes() noexcept {
        return ::comdare::cache_engine::topics::kOrganAxisErrorFloor;
    }

protected:
    BufferStrategyBase() noexcept {
        // Inkrementeller Tier-Binary-Cache (Bauplan §2): Pflicht-algo_version je Kompositions-Organ-Variante — ohne
        // sie kann der Rebuild-/Neu-Mess-Selektor die Binary nicht organ-genau invalidieren. CRTP-Ctor-Guard;
        // universell zusaetzlich via build_axis_variant_version_table() (Typ-Ebene, alle 17 Kompositions-Registries).
        static_assert(
            requires { Derived::algo_version; },
            "Kompositions-Organ-Variante ohne 'static constexpr std::string_view algo_version' "
            "(Bauplan §2): Rebuild-Selektor kann nicht organ-genau invalidieren.");

        // FK-5 (E-24 C9): der Fehlerraum-Zwilling der Versions-Wache darueber -- existiert die
        // Deklaration, und ist sie nicht leer? Beides bricht compile-hart MIT dem Typ-Namen.
        ::comdare::cache_engine::topics::assert_organ_axis_error_classes<Derived>();
        static_assert(concepts::BufferStrategy<Derived>,
                      "Pflicht: Derived muss BufferStrategy erfuellen "
                      "(put/get/size/is_empty/clear + element_type/size_type/topic_tag)");
        static_assert(::comdare::cache_engine::topics::AxisBaseConcept<Derived>,
                      "Pflicht: Derived erfuellt AxisBaseConcept (get_compiler() Default 'original' + "
                      "is_original_module = false via AxisBase)");
    }
    // V41.F.6.1.P2.C ENTFERNT: Defaults kommen via AxisBase (cross-axis generisch).
};

} // namespace comdare::cache_engine::queuing::axis_q1_queuing
