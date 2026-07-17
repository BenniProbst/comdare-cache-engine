// Prospektiver Achsen-Dach-Striktheits-Guard (Bau-INC-1a, Q1-Ruling 2026-07-17).
//
// **Zweck:** Das gemeinsame Achsen-Dach topics::Axis<Derived> (Layer Supertype als
// Marker-Concept + CRTP-Tag) vereinigt die Familien-Wurzeln (Mess-SystemAxis,
// Konfig-CebSystemAxis) OHNE Familien-Semantik zu mischen (Blut-Direktive). Dieser
// Guard beweist das POSITIV und faengt jede kuenftige Degradation (vtable im Dach,
// Zustands-Einschleppung, Diskriminator-Drift) automatisch beim Build/ctest.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner static_assert-Test; keine Aenderung an
// kCompositionAxisNames/serialize_composition_path/golden_fullpilot_320/POD/ABI-4.

#include <cache_engine/concepts/scheduling_strategy.hpp>
#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <cache_engine/measurement/scheduling_system_axis.hpp>
#include <cache_engine/measurement/system_axis.hpp>

#include <gtest/gtest.h>

#include <string_view>
#include <type_traits>

namespace cet = ::comdare::cache_engine::topics;
namespace cem = ::comdare::cache_engine::measurement;

// ── Block A: das Dach selbst ist zustandslos + vtable-frei (Layer Supertype bleibt semantik-frei) ─────────
struct DachProbe final : cet::Axis<DachProbe> {
    [[nodiscard]] static constexpr cet::AxisKind axis_kind() noexcept { return cet::AxisKind::organ; }
};
static_assert(cet::AxisConcept<DachProbe>);
static_assert(std::is_empty_v<cet::Axis<DachProbe>>);
static_assert(!std::is_polymorphic_v<cet::Axis<DachProbe>>);
static_assert(std::is_empty_v<DachProbe>);

// ── Block B: die Konfig-Wurzel CebSystemAxis ist Geschwister unter dem Dach (system_config) ───────────────
struct KonfigProbe final : cem::CebSystemAxis<KonfigProbe> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "konfig_probe"; }
};
static_assert(cem::CebSystemAxisConcept<KonfigProbe>);
static_assert(cet::AxisConcept<KonfigProbe>);
static_assert(KonfigProbe::axis_kind() == cet::AxisKind::system_config);
static_assert(KonfigProbe::axis_label() == std::string_view{"konfig_probe"});
static_assert(std::is_empty_v<KonfigProbe>);
static_assert(!std::is_polymorphic_v<KonfigProbe>);

// ── Block C: die Mess-Wurzel SystemAxis haengt unter dem Dach, ihre Semantik bleibt unveraendert ──────────
static_assert(cet::AxisConcept<cem::WallClockSystemAxis>);
static_assert(cem::SystemAxisConcept<cem::WallClockSystemAxis>);
static_assert(cem::WallClockSystemAxis::axis_kind() == cet::AxisKind::system_measurement);
// Empty-Base-Optimierung haelt: das Dach vergroessert die konkreten Mess-Achsen NICHT.
static_assert(sizeof(cem::WallClockSystemAxis) == sizeof(std::int64_t) + sizeof(std::uint64_t));
static_assert(std::is_empty_v<cem::SystemAxis<cem::WallClockSystemAxis>>);

// ── Block D: die Familien schneiden sich NICHT (Separation of Hierarchies) ────────────────────────────────
static_assert(!std::is_base_of_v<cem::SystemAxis<KonfigProbe>, KonfigProbe>);
static_assert(!std::is_base_of_v<cem::CebSystemAxis<cem::WallClockSystemAxis>, cem::WallClockSystemAxis>);

// ── Block E (INC-1c): Scheduling-Konfig-System-Achse #37 — compile-time-CRTP, vtable-frei ─────────────────
static_assert(cem::SchedulingSystemAxisConcept<cem::DefaultSchedulingSystemAxis>);
static_assert(cem::CebSystemAxisConcept<cem::DefaultSchedulingSystemAxis>);
static_assert(cet::AxisConcept<cem::DefaultSchedulingSystemAxis>);
static_assert(cem::DefaultSchedulingSystemAxis::axis_kind() == cet::AxisKind::system_config);
static_assert(cem::DefaultSchedulingSystemAxis::axis_label() == std::string_view{"scheduling"});
static_assert(cem::DefaultSchedulingSystemAxis::worker_pool_layout() ==
              ::comdare::cache_engine::concepts::WorkerPoolLayout::ThreadPerCore);
static_assert(cem::DefaultSchedulingSystemAxis::simd_worker_count_limit() == 2);
static_assert(std::is_empty_v<cem::DefaultSchedulingSystemAxis>);
static_assert(!std::is_polymorphic_v<cem::DefaultSchedulingSystemAxis>);
// Kontrast (bewusste, DEPRECATED Grenze — nie im Hot-Path): die historische Runtime-vtable IST polymorph.
static_assert(std::is_polymorphic_v<::comdare::cache_engine::concepts::ISchedulingStrategy>);

TEST(StriktheitAxisDachGuard, DiskriminatorenSindDisjunkt) {
    EXPECT_NE(static_cast<unsigned>(cet::AxisKind::organ), static_cast<unsigned>(cet::AxisKind::system_measurement));
    EXPECT_NE(static_cast<unsigned>(cet::AxisKind::system_measurement),
              static_cast<unsigned>(cet::AxisKind::system_config));
    EXPECT_NE(static_cast<unsigned>(cet::AxisKind::organ), static_cast<unsigned>(cet::AxisKind::system_config));
}
