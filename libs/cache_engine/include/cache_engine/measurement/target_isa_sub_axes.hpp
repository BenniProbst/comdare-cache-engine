// measurement/target_isa_sub_axes.hpp -- die Unter-System-Achsen am target_isa-Komplex-Wrapper
// (O-8 Schritt 6 / Bauplan D2.6, IV.2.2; Owner OD-2: der Komplex-Wrapper "erhaelt allein die
// zugehoerigen target_isa Unter-System-Achsen, die zuvor vereinbart waren").
//
// DIE VEREINBARTEN DREI sind scheduling, numa_node und page. scheduling existierte bereits als
// Haupt-Achse und wird in scheduling_system_axis.hpp umgehaengt; numa_node und page hatten bis hierher
// GAR KEINEN Typ -- sie lebten nur als XSD-Elemente (experiment_schema.xsd, O-2-Block) und als
// Bauplan-Namen. Dieser Header gibt ihnen den Typ.
//
// -- ELTERN-BEZUG ALS TYP ------------------------------------------------------------------------
// Beide haengen ueber CebSubAxis<Derived, TargetIsaAxisTag> -- der Eltern-Bezug ist ein TYP, das
// parent_axis_label() ist daraus ABGELEITET (Paket A1). Die alte String-Konvention, bei der jede
// Unter-Achse ihr Eltern-Label von Hand hinschreibt, wird hier NICHT fortgesetzt: genau die hat die
// konkurrierenden Ordnungen ueberhaupt moeglich gemacht.
//
// -- WARUM KEIN CT-OPTIONS-KATALOG ---------------------------------------------------------------
// Anders als simd/opt_level/atomic128 haben diese beiden KEINE compile-statische Options-Menge: die
// zulaessige Node-Zahl bzw. Seitengroessen-Freigabe haengt an der konkreten Maschine und ist zur
// Bauzeit nicht erkennbar (so schon im XSD begruendet). Sie deklarieren deshalb eine option_source
// statt einer Options-Liste -- dasselbe Muster wie die workload-Unter-Achse des Last-Frameworks.
// Die LAUFZEIT-Aufloesung der Werte ist AUSDRUECKLICH nicht Teil dieses Fensters (OD-10, Folge-Paket
// vor Voll-Bau-4); hier entsteht nur die statische Einhaengung, also das ANGEBOT.
//
// -- NAMENSFALLE (Bauplan D2.6 / IV-BLOCKER-5) ---------------------------------------------------
// Die Seiten-Unter-Achse heisst "page", NICHT "page_type" und NICHT "page_topology": axis_01_page_type
// ist der Baum-Knoten-PageKind und eine voellig andere Achse. Der static_assert unten haelt das fest.
//
// Metaprog: CRTP + Concept, static-dispatch, keine vtable.

#pragma once

#include <cache_engine/measurement/ceb_sub_axis.hpp>
#include <cache_engine/measurement/target_isa_complex_axis.hpp>

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// Gemeinsame Schicht der target_isa-Unter-Achsen: Eltern-Typ fest, Options-Herkunft deklariert.
template <class Derived>
struct TargetIsaSubAxis : CebSubAxis<Derived, TargetIsaAxisTag> {
    /// Woher die zulaessigen Werte kommen (statt einer CT-Options-Liste). Reines Deklarations-Merkmal.
    [[nodiscard]] static constexpr std::string_view option_source() noexcept { return Derived::do_option_source(); }

protected:
    constexpr TargetIsaSubAxis() noexcept = default;
};

template <class A>
concept TargetIsaSubAxisConcept = CebSubAxisConcept<A> && std::derived_from<A, TargetIsaSubAxis<A>> && requires {
    { A::option_source() } -> std::same_as<std::string_view>;
};

/// numa_node -- Spiegel des Organ-Members alloc_hw.numa_node. Werte = die Node-Ids der Maschine.
struct NumaNodeSubAxis final : TargetIsaSubAxis<NumaNodeSubAxis> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "numa_node"; }
    [[nodiscard]] static constexpr std::string_view do_option_source() noexcept { return "machine_resolved"; }
};

/// page -- die Seitengroessen-Freigabe als System-Capability (4k/2m/1g). Ihre Durchsetzung liegt
/// organ-seitig im AllocPageHint-NTTP; diese Achse ist die System-SEITE der Freigabe.
struct PageSubAxis final : TargetIsaSubAxis<PageSubAxis> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "page"; }
    [[nodiscard]] static constexpr std::string_view do_option_source() noexcept { return "machine_resolved"; }
};

/// Single-Source der Unter-Achsen-Labels am Komplex-Wrapper (ohne scheduling: das wohnt in seinem
/// eigenen Header, weil es die Achse schon vorher gab).
inline constexpr std::array<std::string_view, 2> kTargetIsaSubAxisLabels = {NumaNodeSubAxis::axis_label(),
                                                                            PageSubAxis::axis_label()};

static_assert(TargetIsaSubAxisConcept<NumaNodeSubAxis>);
static_assert(TargetIsaSubAxisConcept<PageSubAxis>);

// EINHAENGUNGS-WACHE: beide haengen wirklich am target_isa-Komplex, und zwar ueber den TYP.
static_assert(NumaNodeSubAxis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(PageSubAxis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(std::same_as<NumaNodeSubAxis::parent_axis, TargetIsaAxisTag>);
static_assert(std::same_as<PageSubAxis::parent_axis, TargetIsaAxisTag>);
// Tiefe 1 = direkte Unter-Achse (die offene Rekursion aus ceb_sub_axis.hpp, hier als Gegenprobe).
static_assert(axis_depth_v<NumaNodeSubAxis> == 1 && axis_depth_v<PageSubAxis> == 1);

// NAMENSFALLE: "page", nicht "page_type" (axis_01_page_type ist der Baum-Knoten-PageKind).
static_assert(PageSubAxis::axis_label() != std::string_view{"page_type"});
static_assert(PageSubAxis::axis_label() != std::string_view{"page_topology"});
static_assert(NumaNodeSubAxis::axis_label() != std::string_view{"numa_topology"});

} // namespace comdare::cache_engine::measurement
