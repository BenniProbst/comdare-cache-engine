// measurement/target_isa_system_axis.hpp -- target_isa als build-treibende CEB-System-Achse
// (INC-2d, User-Ruling 2026-07-18: "isa ist definitiv eine eigene System-Achse; x86->ARM64 ist ein cross
//  compile, konfigurierbar in den System-Achsen, zulaessig"; Variante A "TargetIsaSystemAxis wie empfohlen").
//
// ROLLE: die ZIEL-ISA (wofuer kompiliert wird) -- verschieden von der HardwareIsaSystemAxis (HOST-Deskriptor
// "hardware", was ausfuehrt). isa verlaesst mit INC-2d die binary_id-Komposition (18->17) und wird eine eigene
// build-treibende System-Achse: sie GIBT die Ziel-ISA FREI (Freigabe-Prinzip), die Erweiterungshardware-/atomic128-
// Organe sind <= Ziel-Zulassung (Ziel != x86_64 => avx*/cx16 degradieren). Cross-Compile x86->ARM64 ist laut Ruling
// zulaessig; der echte aarch64-Materialisierungslauf ist ein separater Toolchain-Handover (aarch64-linux-gnu-g++ +
// sysroot). Der Default X86_64TargetIsa = host==target = KEINE Cross-Flags => golden byte-identisch.
//
// FLUSS (Muster SimdSubAxis::gcc_march_flag / CompilerAtomicSubAxis::gcc_flag): die Auspraegung reflektiert
// compile-time die Cross-Flags (-target/--sysroot/-march) an der make_gpp_compile_fn-Naht (Facade
// perm_target_isa_cflags()); Provenienz +target=<ziel> im build_version/H-10-Sidecar, NIE binary_id. Der isa-ORGAN-
// Typ (Amd64Isa etc., axis_09) BLEIBT als Codegen-Traeger (telemetry-/INC-2c-treu); seine Selektion wandert von der
// binary_id-Permutation zu dieser build-config-Achse. Metaprog: CRTP + Concept, static-dispatch, keine vtable.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <array>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

/// target_isa -- eigene Haupt-System-Achse (Ziel-ISA fuer Cross-Compile). Jede Auspraegung liefert die compile-time
/// Cross-Flags je Dialekt (gcc/clang teilen -target/--sysroot/-march). Analog HardwareIsaSystemAxis (Host), aber
/// build-TREIBEND statt reiner Deskriptor.
template <class Derived>
struct TargetIsaSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "target_isa"; }

    /// Deklarative Ziel-ISA-Kennung (Ordner-/Sidecar-Etikett; +target=<id>).
    [[nodiscard]] static constexpr std::string_view target_isa_id() noexcept { return Derived::do_target_isa_id(); }

    /// true, wenn Ziel==Host (kein Cross-Compile) -- dann sind alle Cross-Flags leer, Ist-Verhalten byte-identisch.
    [[nodiscard]] static constexpr bool is_native() noexcept { return Derived::do_is_native(); }

    /// Cross-Compile-Flags (leer bei is_native()). gcc/clang teilen die Schreibweise; ein leerer Wert = kein Flag.
    [[nodiscard]] static constexpr std::string_view target_triple() noexcept { return Derived::do_target_triple(); }
    [[nodiscard]] static constexpr std::string_view target_march() noexcept { return Derived::do_target_march(); }

protected:
    constexpr TargetIsaSystemAxis() noexcept = default;
};

template <class A>
concept TargetIsaSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, TargetIsaSystemAxis<A>> &&
    std::is_empty_v<TargetIsaSystemAxis<A>> && (!std::is_polymorphic_v<TargetIsaSystemAxis<A>>) && requires {
        { A::target_isa_id() } -> std::same_as<std::string_view>;
        { A::is_native() } -> std::same_as<bool>;
        { A::target_triple() } -> std::same_as<std::string_view>;
        { A::target_march() } -> std::same_as<std::string_view>;
    };

// ── Die Ziel-ISA-Auspraegungs-Familie. Jede eine leere CRTP-Struct (Design-Space-Vokabular); der Planer/die XML
//    waehlt+permutiert, nichts gepinnt. Weitere ISAs (riscv64/power) folgen mit der Runner-/Toolchain-Matrix. ──

/// x86-64 = Host==Target (prod1/prod2): KEINE Cross-Flags, Binary generisch fuer den Bau-Host. Default.
struct X86_64TargetIsa final : TargetIsaSystemAxis<X86_64TargetIsa> {
    [[nodiscard]] static constexpr std::string_view do_target_isa_id() noexcept { return "x86_64"; }
    [[nodiscard]] static constexpr bool             do_is_native() noexcept { return true; }
    [[nodiscard]] static constexpr std::string_view do_target_triple() noexcept { return ""; }
    [[nodiscard]] static constexpr std::string_view do_target_march() noexcept { return ""; }
};

/// aarch64 = Cross-Compile (Host x86 -> Target ARM64). Braucht zur echten Materialisierung den Cross-Treiber
/// (aarch64-linux-gnu-g++) + --sysroot (Toolchain-Handover); dieser Kanal traegt die Ziel-Flags deklarativ.
struct Aarch64TargetIsa final : TargetIsaSystemAxis<Aarch64TargetIsa> {
    [[nodiscard]] static constexpr std::string_view do_target_isa_id() noexcept { return "aarch64"; }
    [[nodiscard]] static constexpr bool             do_is_native() noexcept { return false; }
    [[nodiscard]] static constexpr std::string_view do_target_triple() noexcept { return "-target aarch64-linux-gnu"; }
    [[nodiscard]] static constexpr std::string_view do_target_march() noexcept { return "-march=armv8-a"; }
};

/// CEB-Default = x86_64 (host==target, kein Cross) -- beweglicher Startwert, KEIN Pin; die XML/der Planer waehlt
/// die Ziel-ISA (Cross nur mit vorhandener Toolchain, sonst Compiler-Compiler-Fehler ins Log, kein Absturz).
using DefaultTargetIsa = X86_64TargetIsa;

/// Single-Source der gueltigen target_isa-ids (analog kAllSimdIds/kAllOptLevelIds/kAllAtomic128Ids).
inline constexpr std::array<std::string_view, 2> kAllTargetIsaIds = {X86_64TargetIsa::target_isa_id(),
                                                                     Aarch64TargetIsa::target_isa_id()};

static_assert(TargetIsaSystemAxisConcept<X86_64TargetIsa>);
static_assert(TargetIsaSystemAxisConcept<Aarch64TargetIsa>);
static_assert(X86_64TargetIsa::axis_label() == std::string_view{"target_isa"});
static_assert(X86_64TargetIsa::is_native() && X86_64TargetIsa::target_triple().empty() &&
              X86_64TargetIsa::target_march().empty());
static_assert(!Aarch64TargetIsa::is_native());
static_assert(Aarch64TargetIsa::target_triple() == std::string_view{"-target aarch64-linux-gnu"});
static_assert(DefaultTargetIsa::target_isa_id() == std::string_view{"x86_64"});
// Abgrenzung zum Host-Deskriptor: target_isa (build-treibend) != "hardware" (HardwareIsaSystemAxis, Host).
static_assert(X86_64TargetIsa::do_axis_label() != std::string_view{"hardware"});

} // namespace comdare::cache_engine::measurement
