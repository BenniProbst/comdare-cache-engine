// measurement/target_isa_sub_axes.hpp -- die Unter-System-Achsen am target_isa-Komplex-Wrapper
// (O-8 Schritt 6 / Bauplan D2.6, IV.2.2; Owner OD-2: der Komplex-Wrapper "erhaelt allein die
// zugehoerigen target_isa Unter-System-Achsen, die zuvor vereinbart waren").
//
// DIE URSPRUENGLICH VEREINBARTEN DREI waren scheduling, numa_node und page. scheduling existierte
// bereits als Haupt-Achse und wird in scheduling_system_axis.hpp umgehaengt; numa_node und page hatten
// bis hierher GAR KEINEN Typ -- sie lebten nur als XSD-Elemente (experiment_schema.xsd, O-2-Block) und
// als Bauplan-Namen. Dieser Header gibt ihnen den Typ.
//
// DAZU TRITT core_class (Paket OD-11-RT, Owner-KERN 06.08.2026). Der Owner verbatim -- WORTLAUT
// UNVERAENDERT, auch der darin genannte alte Proben-Name: "numa page ist eine Cache-Seiten
// Koordination von Cache-Seiten lokalitaet. Jetzt brauchen wir ein pendant numa_process_probe dazu,
// welche sich damit beschaeftigt, wo Programme ausgefuehrt werden, nicht welche Speicherseiten wo
// liegen, sie sind aber beide strukturell aehnliche Unterachsen." Die PROBE heisst seit dem
// Folge-KERN desselben Tages ("Ich moechte numa_process_probe besser numa_cpu_pin_process_probe
// nennen") numa_cpu_pin_process_probe; die ACHSE hier heisst unveraendert core_class -- der Owner
// hat das Erhebungs-VERFAHREN umbenannt, nicht den erhobenen Gegenstand. numa_node
// und page tragen die SPEICHER-Lokalitaet, core_class die AUSFUEHRUNGS-Lokalitaet. Der Name kommt aus
// dem Plan und ist nicht neu erfunden: docs/plaene/20260806-PLAN-hybrid-architektur-pmc-achsen-
// zuordnung.md:382 -- "Ein `core_class`-Geschwister neben `numa_node`/`page` braucht keine einzige
// Struktur-Aenderung."
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
// core_class hat ihre eigene, groessere Namensfallen-Menge -- siehe kTargetIsaSubAxisForbiddenLabels.
//
// -- ABGRENZUNG core_class GEGEN scheduling (der einzige echte Kollisions-Kandidat) ---------------
// scheduling ist stage="ct" und ein kind="fixed_enum_tuple" (system_axis_registry.xml:29-37); eines
// seiner sub_dim heisst hetero_core_dispatch und traegt mit {None,HybridAware,PCoresOnly,ECoresOnly}
// dasselbe P/E-Vokabular (concepts/scheduling_strategy.hpp:39). Das ist trotzdem eine ANDERE Sache:
// scheduling traegt die POLICY (was der Bau tun SOLL, compile-time festgelegt), core_class traegt das
// FAKTUM (was die Maschine an Kern-Klassen HAT, zur Laufzeit erhoben). Eine Policy ohne Faktum ist
// eine Behauptung -- es gibt heute keine Instanz im Bestand, die feststellt, ob die Maschine
// ueberhaupt verschiedene Kern-Klassen fuehrt. Genau diese Haelfte entsteht mit core_class.
// Die Trennung ist auch mechanisch: scheduling ist stage="ct" und damit NICHT permutierbar, core_class
// ist stage="runtime" wie numa_node/page und traegt eine machine_resolved WERTE-MENGE.
//
// Metaprog: CRTP + Concept, static-dispatch, keine vtable.

#pragma once

#include <cache_engine/measurement/ceb_sub_axis.hpp>
#include <cache_engine/measurement/target_isa_complex_axis.hpp>

#include <array>
#include <concepts>
#include <cstddef>
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

/// core_class -- die AUSFUEHRUNGS-Lokalitaet als System-Capability: welche Kern-KLASSEN diese Maschine
/// fuehrt (Hybrid-P/E, ungleiche L3-Domaenen, oder ehrlich uniform) und welche logischen Prozessoren zu
/// welcher Klasse gehoeren. Strukturell das Pendant zu numa_node: dort die Frage "wo liegen die
/// Speicherseiten", hier "wo laeuft der Code" (Owner-KERN 06.08.2026).
/// Ihre Durchsetzung liegt NICHT hier: der Aktuator ist builder/measurement/thread_pinning.hpp
/// (ScopedThreadPin), die Bau-seitige Policy ist scheduling.hetero_core_dispatch. Diese Achse ist die
/// System-SEITE des Angebots -- exakt die Rollenteilung, die page gegenueber dem AllocPageHint-NTTP hat.
struct CoreClassSubAxis final : TargetIsaSubAxis<CoreClassSubAxis> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "core_class"; }
    [[nodiscard]] static constexpr std::string_view do_option_source() noexcept { return "machine_resolved"; }
};

/// DIE MITGLIEDSCHAFT der offenen (machine_resolved) Unter-Achsen am Komplex-Wrapper als TYP-LISTE.
///
/// WARUM SIE JETZT ENTSTEHT (B7-Lehre, Codex-Review 02.08.2026, hier nachgezogen): bis zum Eintritt von
/// core_class stand die Mitgliedschaft ZWEIMAL nebeneinander -- als `std::array<..., 2>` hier und als
/// zwei handgeschriebene emit_open-Aufrufe im Registry-Generator. Eine DRITTE Unter-Achse waere dadurch
/// entweder unemittiert geblieben ODER unbewacht in die XML gelaufen; genau das ist die Falle, die B7
/// fuer operating_system bereits geschlossen hat. Ab jetzt gilt hier dasselbe: wer ein Mitglied
/// eintraegt, aendert Label-Liste UND XML-Emission in einem Zug.
/// scheduling ist BEWUSST NICHT Mitglied: es ist stage="ct" mit einem festen sub_dim-Tupel und wohnt in
/// seinem eigenen Header -- diese Liste fuehrt ausschliesslich die machine_resolved-Unter-Achsen.
using TargetIsaOpenSubAxes = SubAxisMembership<TargetIsaAxisTag, NumaNodeSubAxis, PageSubAxis, CoreClassSubAxis>;

/// Single-Source der Unter-Achsen-Labels am Komplex-Wrapper (ohne scheduling: das wohnt in seinem
/// eigenen Header, weil es die Achse schon vorher gab).
/// B7: ABGELEITET aus TargetIsaOpenSubAxes -- die Labels stehen nicht mehr ein zweites Mal da, und die
/// Array-GROESSE ist eine Folge der Mitgliedschaft (vorher eine unabhaengig gepflegte 2).
inline constexpr std::array<std::string_view, TargetIsaOpenSubAxes::size> kTargetIsaSubAxisLabels =
    TargetIsaOpenSubAxes::labels();

namespace detail {

/// Die Labels muessen paarweise verschieden sein -- zwei gleiche waeren zwei XML-Zeilen mit derselben
/// id und damit eine still ueberschriebene Unter-Achse.
[[nodiscard]] consteval bool target_isa_sub_axis_labels_are_distinct() {
    for (std::size_t i = 0; i < kTargetIsaSubAxisLabels.size(); ++i) {
        if (kTargetIsaSubAxisLabels[i].empty()) return false;
        for (std::size_t j = i + 1; j < kTargetIsaSubAxisLabels.size(); ++j)
            if (kTargetIsaSubAxisLabels[i] == kTargetIsaSubAxisLabels[j]) return false;
    }
    return true;
}

/// NAMENSFALLEN-Katalog am target_isa-Komplex (Muster kOperatingSystemSubAxisForbiddenLabels). Jeder
/// Eintrag steht fuer eine REALE Verwechslung im Bestand, nicht fuer eine hypothetische:
///   page_type/page_topology -- axis_01_page_type ist der Baum-Knoten-PageKind (IV-BLOCKER-5);
///   numa_topology           -- der Sammelbegriff, unter dem numa_node und page zusammenfielen;
///   scheduling/hetero_core_dispatch/worker_pool_layout -- die CT-Unter-Achse und zwei ihrer sub_dim
///       (system_axis_registry.xml:29-37). hetero_core_dispatch traegt dasselbe P/E-Vokabular wie
///       core_class, ist aber die POLICY zur Bauzeit; core_class ist das FAKTUM zur Laufzeit. Wer
///       eines der beiden nach dem anderen benennt, macht aus zwei Aussagen eine;
///   core/cpu/core_type/cpu_core/cpu_atom -- die sysfs-/CPUID-Rohnamen der QUELLE. Eine Achse, die wie
///       ihre Quelle heisst, bindet die Achse an ein Erhebungs-Verfahren statt an ihre Bedeutung
///       (cpu_core/cpu_atom sind ueberdies Intel-Namen und schliessen die AMD-Seite sprachlich aus);
///   pinning_policy/core_layout -- die MODELL-seitigen Namen (platform/core_layout.hpp:40
///       PinningPolicyId, IPinningPolicy). Das ist die Durchsetzung, nicht das Angebot.
inline constexpr std::array<std::string_view, 13> kTargetIsaSubAxisForbiddenLabels = {
    "page_type", "page_topology", "numa_topology",  "scheduling",  "hetero_core_dispatch", "core", "cpu", "core_type",
    "cpu_core",  "cpu_atom",      "pinning_policy", "core_layout", "worker_pool_layout"};

[[nodiscard]] consteval bool target_isa_sub_axis_labels_avoid_traps() {
    for (auto const& label : kTargetIsaSubAxisLabels)
        for (auto const& forbidden : kTargetIsaSubAxisForbiddenLabels)
            if (label == forbidden) return false;
    return true;
}

} // namespace detail

static_assert(TargetIsaSubAxisConcept<NumaNodeSubAxis>);
static_assert(TargetIsaSubAxisConcept<PageSubAxis>);
static_assert(TargetIsaSubAxisConcept<CoreClassSubAxis>);

// EINHAENGUNGS-WACHE: alle drei haengen wirklich am target_isa-Komplex, und zwar ueber den TYP.
static_assert(NumaNodeSubAxis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(PageSubAxis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(CoreClassSubAxis::parent_axis_label() == std::string_view{"target_isa"});
static_assert(std::same_as<NumaNodeSubAxis::parent_axis, TargetIsaAxisTag>);
static_assert(std::same_as<PageSubAxis::parent_axis, TargetIsaAxisTag>);
static_assert(std::same_as<CoreClassSubAxis::parent_axis, TargetIsaAxisTag>);
// Tiefe 1 = direkte Unter-Achse (die offene Rekursion aus ceb_sub_axis.hpp, hier als Gegenprobe).
static_assert(axis_depth_v<NumaNodeSubAxis> == 1 && axis_depth_v<PageSubAxis> == 1 &&
              axis_depth_v<CoreClassSubAxis> == 1);

// MITGLIEDSCHAFTS-WACHEN (B7): die Zahl sitzt auf der LISTE, nicht auf einem Array fester Groesse --
// nur so ist sie nicht tautologisch. Wer eine vierte offene Unter-Achse eintraegt, bricht hier und muss
// den Eintrag quittieren, statt die Zahl nachzuziehen.
static_assert(TargetIsaOpenSubAxes::size == 3,
              "Der target_isa-Komplex fuehrt GENAU DREI offene (machine_resolved) Unter-Achsen: "
              "numa_node und page (Speicher-Lokalitaet, O-8 Schritt 6) und core_class "
              "(Ausfuehrungs-Lokalitaet, Owner-KERN 06.08.2026). scheduling ist stage=ct und "
              "AUSDRUECKLICH kein Mitglied dieser Liste.");
static_assert(kTargetIsaSubAxisLabels.size() == TargetIsaOpenSubAxes::size,
              "kTargetIsaSubAxisLabels ist ABGELEITET aus TargetIsaOpenSubAxes -- die beiden duerfen "
              "nicht auseinanderlaufen (sonst waere die Anzahl-Wache wieder tautologisch).");
static_assert(std::same_as<TargetIsaOpenSubAxes::parent_axis, TargetIsaAxisTag>);
static_assert(TargetIsaOpenSubAxes::parent_axis::axis_label() == std::string_view{"target_isa"});
static_assert(detail::target_isa_sub_axis_labels_are_distinct(),
              "Zwei target_isa-Unter-Achsen tragen dasselbe Label -- in der Registry-XML waeren das zwei "
              "<sub_axis>-Zeilen mit derselben id, von denen jeder Leser eine still verliert.");

// NAMENSFALLE: "page", nicht "page_type" (axis_01_page_type ist der Baum-Knoten-PageKind).
static_assert(PageSubAxis::axis_label() != std::string_view{"page_type"});
static_assert(PageSubAxis::axis_label() != std::string_view{"page_topology"});
static_assert(NumaNodeSubAxis::axis_label() != std::string_view{"numa_topology"});
// NAMENSFALLE core_class: der einzige Kollisions-Kandidat im Bestand ist scheduling mit seinem sub_dim
// hetero_core_dispatch -- dieselben P/E-Woerter, andere Sache (CT-Policy vs RT-Faktum, Kopf-Block).
static_assert(CoreClassSubAxis::axis_label() != std::string_view{"scheduling"});
static_assert(CoreClassSubAxis::axis_label() != std::string_view{"hetero_core_dispatch"});
static_assert(CoreClassSubAxis::axis_label() != std::string_view{"core_type"});
static_assert(detail::target_isa_sub_axis_labels_avoid_traps(),
              "Ein target_isa-Unter-Achsen-Label kollidiert mit einem Begriff aus dem Verbots-Katalog "
              "(kTargetIsaSubAxisForbiddenLabels): scheduling/hetero_core_dispatch/worker_pool_layout "
              "sind die CT-POLICY, cpu_core/cpu_atom/core_type sind die QUELLEN-Rohnamen, "
              "pinning_policy/core_layout ist die Modell-seitige DURCHSETZUNG -- keines davon ist die "
              "System-seitige Werte-Menge dieser Achse.");

// OPTIONS-HERKUNFT: alle drei sind maschinen-aufgeloest (kein CT-Options-Katalog). Der Wert ist das
// Etikett, das die Registry-XML traegt -- er ist an DIESE Konstante gekoppelt, nicht an ein XML-Literal.
static_assert(NumaNodeSubAxis::option_source() == std::string_view{"machine_resolved"});
static_assert(PageSubAxis::option_source() == std::string_view{"machine_resolved"});
static_assert(CoreClassSubAxis::option_source() == std::string_view{"machine_resolved"});

} // namespace comdare::cache_engine::measurement
