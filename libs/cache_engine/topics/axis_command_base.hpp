#pragma once
// CMD-1-a (#267/#251) — Compile-time-Command-Basis der Achsen.
//
// Zweck: Command-Basis der Achsen compile-time; Mess-Visitor-Slot;
// Limitations-Auskunft. Datum: 2026-07-06.

#include "axis_base.hpp"

#include <anatomy/observer_aggregate.hpp>

#include <concepts>
#include <string_view>

namespace comdare::cache_engine::topics {

using ::comdare::cache_engine::anatomy::ObservableAxis;

/// Compile-time-Command-Vertrag für Achsen: AxisBase-Pflicht-API plus statische Identität.
template <class Axis>
concept AxisCommand = AxisBaseConcept<Axis> && requires {
    { Axis::name() } -> std::convertible_to<std::string_view>;
};

/// Visitor-Vertrags-Concept (CMD-1-b, Querschnitt M): beobachtbare Achsen verlangen visit_observable<Axis>(),
/// nicht beobachtbare Achsen bleiben fuer den Mess-Slot ein ehrlicher No-op. BEWUSST OHNE AxisCommand-Konjunkt:
/// der Mess-Slot wird auch fuer Organ-HUELLEN (ObservableComposedContainer/-Search via Composition::search_algo,
/// anatomy_execution_context.hpp observe_all) instanziiert, die keine AxisBase-Statik tragen — das Konjunkt
/// braeche alle Referenz-Kompositionen im Default-Build (Review wf_f1604ba3, CONFIRMED-critical).
template <class Axis, class Visitor>
concept MeasurementVisitable = !ObservableAxis<Axis> || requires(Visitor&& v) { v.template visit_observable<Axis>(); };

/// Statischer Mess-Visitor-Slot. Nicht beobachtbare Achsen bleiben ein ehrlicher No-op.
template <class Axis, class V>
    requires MeasurementVisitable<Axis, V>
constexpr void axis_accept_measurement(V&& v) {
    if constexpr (ObservableAxis<Axis>) {
        v.template visit_observable<Axis>();
    } else {
        (void)v;
    }
}

/// Compile-time-Limitations-Auskunft; Runtime-Pendant bleibt IResourceControllableTier
/// (anatomy/resource_controllable_tier.hpp:56, `tier_query_resource_caps` @:63) und wird hier nicht dupliziert.
template <class Axis>
struct AxisLimitations {
    static_assert(AxisCommand<Axis>, "AxisLimitations verlangt eine AxisCommand-Achse");

    static constexpr bool             observable      = ObservableAxis<Axis>;
    static constexpr bool             original_module = Axis::is_original_module();
    static constexpr std::string_view compiler        = Axis::get_compiler();
};

} // namespace comdare::cache_engine::topics

#include "../src/concepts/axis_original_code_mixin_base.hpp"
#include <axes/alloc/axis_06_allocator_std_malloc.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt4.hpp>

namespace comdare::cache_engine::topics::cmd1a_selftest {

struct PaperManifest {
    static constexpr std::string_view kCompiler = "cmd1a-selftest";
};

struct OriginalMixinAxis : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<PaperManifest> {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "cmd1a_original_mixin_axis"; }
    [[nodiscard]] static constexpr bool             is_original_module() noexcept { return true; }
};

static_assert(AxisCommand<::comdare::cache_engine::alloc::StdMalloc>);
static_assert(AxisCommand<::comdare::cache_engine::nodes::axis_btree_order::BtreeOrderKt4>);
static_assert(AxisCommand<OriginalMixinAxis>);

} // namespace comdare::cache_engine::topics::cmd1a_selftest
