// measurement/hardware_isa_system_axis.hpp -- Hardware/ISA als CEB-Konfig-System-Achse
// (Bau-INC-1d-RAHMEN, e18-Doktrin + Q2-Ruling 2026-07-17).
//
// Rolle: HOST-DESKRIPTOR + Mess-Gate ("Binary-ISA ⊆ Host-Capability") — diese Achse beschreibt,
// WELCHER Host ausfuehrt; sie treibt NICHT den Bau (das tut die Erweiterungshardware-Achse als
// Flag-Quelle an der CompileFn-Naht) und beruehrt NIE die binary_id. Das Laufzeit-Capability-
// Gate (cpuid-gestuetzt) kommt mit dem Planer-/Runner-Konsumenten (PF2-Matrix).

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

template <class Derived>
struct HardwareIsaSystemAxis : CebSystemAxis<Derived> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "hardware"; }

    /// Host-ISA-Kennung (Serialisierungs-Ordner-Ebene Host -> OS -> Compiler -> ISA).
    [[nodiscard]] static constexpr std::string_view host_isa() noexcept { return Derived::do_host_isa(); }

protected:
    constexpr HardwareIsaSystemAxis() noexcept = default;
};

template <class A>
concept HardwareIsaSystemAxisConcept =
    CebSystemAxisConcept<A> && std::derived_from<A, HardwareIsaSystemAxis<A>> &&
    std::is_empty_v<HardwareIsaSystemAxis<A>> && (!std::is_polymorphic_v<HardwareIsaSystemAxis<A>>) && requires {
        { A::host_isa() } -> std::same_as<std::string_view>;
    };

/// x86-64-Host (prod1/prod2-Bein); weitere Hosts (aarch64/riscv64) folgen mit der Runner-Matrix.
struct Amd64HostIsaAxis final : HardwareIsaSystemAxis<Amd64HostIsaAxis> {
    [[nodiscard]] static constexpr std::string_view do_host_isa() noexcept { return "x86_64"; }
};

static_assert(HardwareIsaSystemAxisConcept<Amd64HostIsaAxis>);

} // namespace comdare::cache_engine::measurement
