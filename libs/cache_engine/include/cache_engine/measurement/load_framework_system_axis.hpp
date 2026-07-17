// measurement/load_framework_system_axis.hpp -- Last/Last-Frameworks als CEB-Konfig-System-Achse
// (Bau-INC-1f, H-9, 2026-07-17).
//
// Die WAHL des Last-Frameworks ist eine System-Achse in der CEB (User-Entscheid H-9); die
// einzelnen Workload-Parameter bleiben die DYNAMISCHE Unter-Achse (is_static=false, binary_id-
// neutral), deren Eigenschaften ueber das Pruef-Dock als Settings an die Tier-Binary gehen.
// Diese Achse ist die Single-Source des Unter-Achsen-Labels "workload" (setting_label-Konvention
// "workload.workload_id=X" -> two_phase_valid, cache_engine_builder_iterator.hpp:789-793).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct LoadFrameworkSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "load_framework"; }

    /// Framework-Kennung (Serialisierungs-Ordner-Etikett).
    [[nodiscard]] static constexpr std::string_view framework_id() noexcept { return Derived::do_framework_id(); }

    /// Label der dynamischen Unter-Achse (H-9): Single-Source fuer die AxisLevel-/setting_label-
    /// Konvention "workload.workload_id=..." (Zwei-Phasen-Gueltigkeit haengt an diesem String).
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "workload"; }

protected:
    constexpr LoadFrameworkSystemAxis() noexcept = default;
};

template <class A>
concept LoadFrameworkSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, LoadFrameworkSystemAxis<A>> &&
    std::is_empty_v<LoadFrameworkSystemAxis<A>> && (!std::is_polymorphic_v<LoadFrameworkSystemAxis<A>>) && requires {
        { A::framework_id() } -> std::same_as<std::string_view>;
        { A::sub_axis_label() } -> std::same_as<std::string_view>;
    };

/// YCSB ist das erste (und aktuell einzige) angebundene Last-Framework (ycsb_a..ycsb_f-Profile).
struct YcsbLoadFrameworkAxis final : LoadFrameworkSystemAxis<YcsbLoadFrameworkAxis> {
    [[nodiscard]] static constexpr std::string_view do_framework_id() noexcept { return "ycsb"; }
};

static_assert(LoadFrameworkSystemAxisConcept<YcsbLoadFrameworkAxis>);

} // namespace comdare::cache_engine::measurement
