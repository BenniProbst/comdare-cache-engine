// measurement/load_framework_measurement_axis.hpp -- Last/Last-Frameworks als MESS-Realm-Meta-Meta-
// HAUPT-Achse des Planers (Bau-INC-1f, H-9; K1-Umzug O-8 Schritt 4 / Paket A3).
//
// Die WAHL des Last-Frameworks ist die Achse; die einzelnen Workload-Parameter bleiben die DYNAMISCHE
// Unter-Achse (is_static=false, binary_id-neutral), deren Eigenschaften ueber das Pruef-Dock als
// Settings an die Tier-Binary gehen. Diese Achse ist die Single-Source des Unter-Achsen-Labels
// "workload" (setting_label-Konvention "workload.workload_id=X" -> two_phase_valid,
// cache_engine_builder_iterator.hpp:789-793).
//
// K1-UMZUG VOLLZOGEN (R-G, Ledger 69.1, Owner-KERN 26.07.): frueher erbte dieser Typ von
// CebSystemAxis und war damit im CODE eine CEB-Konfig-SYSTEM-Achse -- der Kommentar bestritt das
// zwar bereits ("KOMMENTAR-ONLY, kein Code"), axis_kind() sagte aber system_config. Die Basis ist
// jetzt MeasurementMetaMetaAxis und axis_kind() liefert measurement_meta_meta: Etikett und Code
// stimmen ab hier ueberein. Owner-Wortlaut: "load_framework wird rein den Mess-Achsen-Typen als
// weitere Meta-Meta-Hauptachse zugeordnet" -- der PLANER generiert die Loads und DELEGIERT sie ans
// CEB-Interface, das erfaehrt, welche Binary-Rekombination gegen welche Lasten zu messen ist.
// ERSATZLOS aus der System-Welt: raus aus kSystemAxisOrder/kSystemAxisCodeVersions (dort sichern
// jetzt Abgangs-static_asserts gegen eine stille Rueckkehr), raus aus dem System-Registry-Top-Level,
// rein in die Mess-Registry. AUCH KEIN Glied des external_utils-Hubs -- der traegt ausschliesslich
// System-Meta-Metas (Lane C C-1' instanziiert diese Achse ausdruecklich nicht).

#pragma once

#include <cache_engine/measurement/measurement_meta_meta_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct LoadFrameworkMeasurementAxis : MeasurementMetaMetaAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "load_framework"; }

    /// Framework-Kennung (Serialisierungs-Ordner-Etikett).
    [[nodiscard]] static constexpr std::string_view framework_id() noexcept { return Derived::do_framework_id(); }

    /// Label der dynamischen Unter-Achse (H-9): Single-Source fuer die AxisLevel-/setting_label-
    /// Konvention "workload.workload_id=..." (Zwei-Phasen-Gueltigkeit haengt an diesem String).
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "workload"; }

protected:
    constexpr LoadFrameworkMeasurementAxis() noexcept = default;
};

template <class A>
concept LoadFrameworkMeasurementAxisConcept =
    MeasurementMetaMetaAxisConcept<A> && std::derived_from<A, LoadFrameworkMeasurementAxis<A>> &&
    std::is_empty_v<LoadFrameworkMeasurementAxis<A>> && (!std::is_polymorphic_v<LoadFrameworkMeasurementAxis<A>>) &&
    requires {
        { A::framework_id() } -> std::same_as<std::string_view>;
        { A::sub_axis_label() } -> std::same_as<std::string_view>;
    };

/// YCSB ist das erste (und aktuell einzige) angebundene Last-Framework (ycsb_a..ycsb_f-Profile).
struct YcsbLoadFrameworkAxis final : LoadFrameworkMeasurementAxis<YcsbLoadFrameworkAxis> {
    [[nodiscard]] static constexpr std::string_view do_framework_id() noexcept { return "ycsb"; }
};

static_assert(LoadFrameworkMeasurementAxisConcept<YcsbLoadFrameworkAxis>);
// REALM-ANKER (K1, O-8 Schritt 4): der Umzug ist genau dann vollzogen, wenn der CODE ihn sagt --
// nicht der Kommentar. Ein Rueckfall auf CebSystemAxis (system_config) bricht hier.
// Der Realm-Wechsel gilt genau dann, wenn der CODE ihn sagt -- nicht der Kommentar. CebSystemAxis
// liefert axis_kind() HART als system_config; diese Zeile bricht daher bei jedem stillen Rueckfall
// auf die System-Wurzel. Absichtlich KEIN "!CebSystemAxisConcept"-Gegentest: der zoege
// ceb_system_axis.hpp in den Include-Graph der Mess-Seite und vermischte genau die Realms, die
// RF-1 getrennt haben will.
static_assert(YcsbLoadFrameworkAxis::axis_kind() == topics::AxisKind::measurement_meta_meta,
              "load_framework ist MESS-Realm-Meta-Meta (Ledger 69.1/70.1), nicht system_config.");

} // namespace comdare::cache_engine::measurement
