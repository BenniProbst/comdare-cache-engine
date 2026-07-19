// Prospektiver Achsen-Dach-Striktheits-Guard (Bau-INC-1a, Q1-Ruling 2026-07-17).
//
// **Zweck:** Das gemeinsame Achsen-Dach topics::Axis<Derived> (Layer Supertype als
// Marker-Concept + CRTP-Tag) vereinigt die Familien-Wurzeln (Mess-SystemAxis,
// Konfig-CebSystemAxis) OHNE Familien-Semantik zu mischen (Blut-Direktive). Dieser
// Guard beweist das POSITIV und faengt jede kuenftige Degradation (vtable im Dach,
// Zustands-Einschleppung, Diskriminator-Drift) automatisch beim Build/ctest.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner static_assert-Test; keine Aenderung an
// kCompositionAxisNames/serialize_composition_path/golden_fullpilot_320/POD/ABI-6.

#include <cache_engine/concepts/scheduling_strategy.hpp>
#include <cache_engine/measurement/ceb_system_axis.hpp>
#include <cache_engine/measurement/compiler_system_axis.hpp>
#include <cache_engine/measurement/extension_hardware_system_axis.hpp>
#include <cache_engine/measurement/hardware_isa_system_axis.hpp>
#include <cache_engine/measurement/load_framework_system_axis.hpp>
#include <cache_engine/measurement/optimization_level_sub_axis.hpp>
#include <cache_engine/measurement/scheduling_system_axis.hpp>
#include <cache_engine/measurement/simd_sub_axis.hpp>
#include <cache_engine/measurement/compiler_atomic_sub_axis.hpp>
#include <cache_engine/measurement/target_isa_system_axis.hpp>
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

// ── Block F (INC-1d, DEPRECATED Kontrast — das flache Alt-Modell, abgeloest durch die simd-Unter-Achse Block F.2;
//    extension_hardware_system_axis.hpp haelt die Typen nur noch kompilierbar): Erweiterungshardware- + Hardware-
//    Host-Achse (Q2-Option-C) ────────────────────────────────────────────────────────────────────────────────
static_assert(cem::ExtensionHardwareSystemAxisConcept<cem::GenericExtensionHardwareAxis>);
static_assert(cem::ExtensionHardwareSystemAxisConcept<cem::Avx2ExtensionHardwareAxis>);
static_assert(cem::ExtensionHardwareSystemAxisConcept<cem::Avx512ExtensionHardwareAxis>);
static_assert(cem::HardwareIsaSystemAxisConcept<cem::Amd64HostIsaAxis>);
static_assert(cem::GenericExtensionHardwareAxis::axis_kind() == cet::AxisKind::system_config);
static_assert(cem::GenericExtensionHardwareAxis::axis_label() == std::string_view{"extension_hardware"});
static_assert(cem::Amd64HostIsaAxis::axis_label() == std::string_view{"hardware"});
// Flag-Deckung zur codegen-Praezedenz (permutation_codegen_tool.cpp simd_flags: avx512->-mavx512f, avx2->-mavx2)
static_assert(cem::Avx512ExtensionHardwareAxis::gcc_march_flag() == std::string_view{"-mavx512f"});
static_assert(cem::Avx2ExtensionHardwareAxis::gcc_march_flag() == std::string_view{"-mavx2"});
// Default = generisch: KEINE Flags (Ist-Verhalten der Mess-DLLs bleibt byte-identisch).
static_assert(cem::GenericExtensionHardwareAxis::gcc_march_flag().empty());
static_assert(cem::GenericExtensionHardwareAxis::clang_march_flag().empty());

// ── Block F.2 (F-SIMD, 2026-07-18): simd als Unter-Achse UNTER extension_hardware (symmetrisch zu opt_level/compiler) ──
// compile-time CRTP+Concept, keine vtable; parent_axis_label()=="extension_hardware" verankert die Unter-Achsen-
// Zugehoerigkeit. no_extension/avx2/avx512 sind OPTIONEN der simd-Unter-Achse, KEINE Geschwister-System-Achsen.
static_assert(cem::SimdSubAxisConcept<cem::SimdNoExtOption>);
static_assert(cem::SimdSubAxisConcept<cem::SimdAvx2Option>);
static_assert(cem::SimdSubAxisConcept<cem::SimdAvx512Option>);
static_assert(cem::SimdAvx2Option::axis_label() == std::string_view{"simd"});
static_assert(cem::SimdAvx2Option::parent_axis_label() == std::string_view{"extension_hardware"});
static_assert(cem::SimdAvx512Option::gcc_march_flag() == std::string_view{"-mavx512f"});
static_assert(cem::SimdAvx2Option::gcc_march_flag() == std::string_view{"-mavx2"});
static_assert(cem::SimdNoExtOption::gcc_march_flag().empty());
static_assert(cem::DefaultSimdOption::simd_id() == std::string_view{"no_extension"});

// ── Block G (INC-1e): die Mess-Telemetrie-Schicht ist vollstaendig unterm Dach verankert ──────────────────
static_assert(cet::AxisConcept<cem::ObserverSnapshotSystemAxis>);
static_assert(cet::AxisConcept<cem::PmcSystemAxis>);
static_assert(cem::ObserverSnapshotSystemAxis::axis_kind() == cet::AxisKind::system_measurement);
static_assert(cem::PmcSystemAxis::axis_kind() == cet::AxisKind::system_measurement);

// ── Block H (INC-1f): Last-Framework-Achse (H-9) + Single-Source des Unter-Achsen-Labels ──────────────────
static_assert(cem::LoadFrameworkSystemAxisConcept<cem::YcsbLoadFrameworkAxis>);
static_assert(cem::YcsbLoadFrameworkAxis::axis_label() == std::string_view{"load_framework"});
static_assert(cem::YcsbLoadFrameworkAxis::framework_id() == std::string_view{"ycsb"});
// Die Zwei-Phasen-Konvention "workload.workload_id=..." haengt an diesem String — Drift bricht hier.
static_assert(cem::YcsbLoadFrameworkAxis::sub_axis_label() == std::string_view{"workload"});

// ── Block I (INC-1h): Compiler-System-Achse (Q3: volle 5. Achse, gcc|clang, beide Treiber) ────────────────
static_assert(cem::CompilerSystemAxisConcept<cem::GccCompilerAxis>);
static_assert(cem::CompilerSystemAxisConcept<cem::ClangCompilerAxis>);
static_assert(cem::GccCompilerAxis::axis_label() == std::string_view{"compiler"});
static_assert(cem::GccCompilerAxis::compiler_id() == std::string_view{"gcc"});
static_assert(cem::ClangCompilerAxis::compiler_id() == std::string_view{"clang"});
// Das Dialekt-Gate haengt an dieser Reflexion: clang kennt -fno-gnu-unique NICHT.
static_assert(cem::GccCompilerAxis::supports_fno_gnu_unique());
static_assert(!cem::ClangCompilerAxis::supports_fno_gnu_unique());

// ── Block J (Bau-INC-2c.opt-a): opt_level als dynamische Unter-Achse UNTER der Compiler-Haupt-Achse (OF-1/2/3) ──
// Compile-time-Schicht (CRTP+Concept, keine vtable), binary_id-neutral (H-10-Sidecar). parent_axis_label=="compiler"
// verankert die Unter-Achsen-Zugehoerigkeit; opt_level ist KEINE Geschwister-System-Achse.
static_assert(cem::OptimizationLevelSubAxisConcept<cem::OptO0Option>);
static_assert(cem::OptimizationLevelSubAxisConcept<cem::OptO3Option>);
static_assert(cem::OptimizationLevelSubAxisConcept<cem::OptOfastOption>);
static_assert(cem::OptO2Option::axis_label() == std::string_view{"opt_level"});
static_assert(cem::OptOfastOption::parent_axis_label() == std::string_view{"compiler"});
static_assert(cem::OptO3Option::gcc_opt_flag() == std::string_view{"-O3"});
// Ruling 2026-07-18 (Option B): CEB-Default = O3 (IEEE-754-deterministisch, beweglich, kein Pin); Ofast additive
// Extreme, bricht den Determinismus (-fallow-store-data-races/-funsafe-math).
static_assert(cem::DefaultOptLevelOption::opt_level_id() == std::string_view{"O3"});
static_assert(cem::DefaultOptLevelOption::is_ieee754_deterministic());
static_assert(cem::OptO3Option::is_ieee754_deterministic());
static_assert(!cem::OptOfastOption::is_ieee754_deterministic());

// ── Block K (INC-0): atomic128 (-mcx16) als dynamische Unter-Achse UNTER der Compiler-Haupt-Achse ─────────────
// Compile-time-Schicht (CRTP+Concept, keine vtable), binary_id-neutral (system_config). parent_axis_label=="compiler"
// (NICHT extension_hardware: CMPXCHG16B ist Baseline-x86-64, keine optionale Erweiterung; INC-0-Ruling "Flags→Compiler").
static_assert(cem::CompilerAtomicSubAxisConcept<cem::NoCx16Option>);
static_assert(cem::CompilerAtomicSubAxisConcept<cem::Cx16Option>);
static_assert(cem::Cx16Option::axis_label() == std::string_view{"atomic128"});
static_assert(cem::Cx16Option::parent_axis_label() == std::string_view{"compiler"});
static_assert(cem::Cx16Option::gcc_flag() == std::string_view{"-mcx16"});
static_assert(cem::Cx16Option::clang_flag() == std::string_view{"-mcx16"});
static_assert(cem::NoCx16Option::gcc_flag().empty()); // Default = kein Flag, Ist-Verhalten byte-identisch
static_assert(cem::Cx16Option::axis_kind() == cet::AxisKind::system_config);

// ── Block L (INC-2d): target_isa als build-treibende eigene System-Achse (Ziel-ISA, Cross-Compile) ────────────
// isa verlaesst mit INC-2d die binary_id-Komposition; TargetIsaSystemAxis TREIBT den Bau (Ziel-ISA), abgegrenzt von
// HardwareIsaSystemAxis (Host-Deskriptor "hardware"). Default X86_64TargetIsa = host==target = keine Cross-Flags =
// golden byte-identisch. system_config (binary_id-neutral). CRTP+Concept, keine vtable.
static_assert(cem::TargetIsaSystemAxisConcept<cem::X86_64TargetIsa>);
static_assert(cem::TargetIsaSystemAxisConcept<cem::Aarch64TargetIsa>);
static_assert(cem::X86_64TargetIsa::axis_label() == std::string_view{"target_isa"});
static_assert(cem::X86_64TargetIsa::axis_kind() == cet::AxisKind::system_config);
static_assert(cem::X86_64TargetIsa::is_native() && cem::X86_64TargetIsa::target_triple().empty());
static_assert(cem::Aarch64TargetIsa::target_triple() == std::string_view{"-target aarch64-linux-gnu"});
// Zwei ISA-System-Achsen, ABER verschiedene Labels (kollisionsfrei): target_isa (Ziel) != hardware (Host).
static_assert(cem::X86_64TargetIsa::axis_label() != cem::Amd64HostIsaAxis::axis_label());

TEST(StriktheitAxisDachGuard, DiskriminatorenSindDisjunkt) {
    EXPECT_NE(static_cast<unsigned>(cet::AxisKind::organ), static_cast<unsigned>(cet::AxisKind::system_measurement));
    EXPECT_NE(static_cast<unsigned>(cet::AxisKind::system_measurement),
              static_cast<unsigned>(cet::AxisKind::system_config));
    EXPECT_NE(static_cast<unsigned>(cet::AxisKind::organ), static_cast<unsigned>(cet::AxisKind::system_config));
}
