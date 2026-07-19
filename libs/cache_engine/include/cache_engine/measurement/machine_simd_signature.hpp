// measurement/machine_simd_signature.hpp -- deklarierte per-Maschine SIMD-Flag-Signatur (Bau Section 40.a, E1).
//
// Verortung (Architekt-Ruling 2026-07-19): in der Host-Capability-Domaene von hardware_isa_system_axis.hpp
// -- die Maschinen-Signatur ist die feingranulare Auspraegung des Host-Deskriptors "welcher Host fuehrt
// aus". Jede Maschine deklariert EINZELN, welche SIMD-Flags ihre CPU besitzt (exakt aus dem live-
// verifizierten Referenzdoc docs/architektur/20260719-simd-flag-signaturen-REFERENZ.md, prod1/Zen5
// 100%-cpuinfo-Abgleich). Der grobe simd-Sub-Axis {no_extension,avx2,avx512} bleibt die Runner-Routing-
// Vorstufe; DIESE Signatur entscheidet den BAU (E4-Gate: Organ <= Signatur geschnitten Sinnhaftigkeit).
// Rein additiv, golden==131072 unberuehrt (Signatur -> CompileFn/H-10-Sidecar, NIE binary_id).
//
// Metaprog: CRTP + Concept (static-dispatch, keine vtable, is_empty). Die Signatur ist ein constexpr-span
// ueber einen benannten Flag-Array aus dem Single-Source-Katalog (simd_feature_flag.hpp). Der Bezug zur
// Ziel-ISA laeuft ueber host_isa() (Amd64HostIsaAxis) -- keine Parallelstruktur zum Host-Deskriptor.

#pragma once

#include <cache_engine/measurement/hardware_isa_system_axis.hpp>
#include <cache_engine/measurement/simd_feature_flag.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

// CRTP-Basis der Maschinen-Signatur: machine_id() (Etikett), host_isa() (Bein an hardware_isa),
// signature() (die einzelnen besessenen Flags). Rein deklarativ, empty, static-dispatch, keine vtable.
template <class Derived>
struct MachineSimdSignature {
    [[nodiscard]] static constexpr std::string_view machine_id() noexcept { return Derived::do_machine_id(); }
    [[nodiscard]] static constexpr std::string_view host_isa() noexcept { return Derived::do_host_isa(); }
    [[nodiscard]] static constexpr std::span<SimdFeatureFlag const> signature() noexcept {
        return Derived::do_signature();
    }

    /// Besitzt die Maschine ein bestimmtes Flag (cpuinfo-Name-Match)? Grundoperation des E4-Gates.
    [[nodiscard]] static constexpr bool has_flag(std::string_view cpuinfo) noexcept {
        for (auto const& f : Derived::do_signature())
            if (f.cpuinfo == cpuinfo) return true;
        return false;
    }

protected:
    constexpr MachineSimdSignature() noexcept = default;
};

template <class A>
concept MachineSimdSignatureConcept =
    std::derived_from<A, MachineSimdSignature<A>> && std::is_empty_v<MachineSimdSignature<A>> &&
    (!std::is_polymorphic_v<MachineSimdSignature<A>>) && requires {
        { A::machine_id() } -> std::same_as<std::string_view>;
        { A::host_isa() } -> std::same_as<std::string_view>;
        { A::signature() } -> std::same_as<std::span<SimdFeatureFlag const>>;
        { A::has_flag(std::string_view{}) } -> std::same_as<bool>;
    };

/// Zaehlt die AVX-512-Tier-Flags einer Signatur (Grob-Level-freie Feinzaehlung; E1-Verifikation).
[[nodiscard]] constexpr std::size_t count_avx512_flags(std::span<SimdFeatureFlag const> sig) noexcept {
    std::size_t n = 0;
    for (auto const& f : sig)
        if (f.tier == SimdFlagTier::Avx512) ++n;
    return n;
}

/// Referenziert die Signatur nur Katalog-bekannte Flags? (Vokabular-Kopplung Signatur -> Katalog.)
[[nodiscard]] constexpr bool signature_within_catalog(std::span<SimdFeatureFlag const> sig) noexcept {
    for (auto const& f : sig)
        if (!is_known_simd_flag(f.cpuinfo)) return false;
    return true;
}

// -- prod1: AMD Ryzen 9 9950X3D (Zen 5) -- voll AVX-512 nativ 512-bit, 13 avx512-Flags, KEIN fp16, --
//    VP2INTERSECT als Zen5-Neuzugang (live 100%-Abgleich gegen /proc/cpuinfo).
struct Prod1Zen5Signature final : MachineSimdSignature<Prod1Zen5Signature> {
    [[nodiscard]] static constexpr std::string_view do_machine_id() noexcept { return "prod1_zen5"; }
    [[nodiscard]] static constexpr std::string_view do_host_isa() noexcept { return Amd64HostIsaAxis::do_host_isa(); }
    [[nodiscard]] static constexpr std::span<SimdFeatureFlag const> do_signature() noexcept {
        static constexpr std::array kFlags{
            kAvx2,       kFma,        kF16c,        kAvxVnni,    kBmi2,         kPopcnt,          kGfni,
            kVaes,       kVpclmulqdq, kAvx512F,     kAvx512Cd,   kAvx512Vl,     kAvx512Dq,        kAvx512Bw,
            kAvx512Ifma, kAvx512Vbmi, kAvx512Vbmi2, kAvx512Vnni, kAvx512Bitalg, kAvx512Vpopcntdq, kAvx512Vp2intersect,
            kAvx512Bf16};
        return kFlags;
    }
};

// -- prod2: Intel Core i9-14900KS (Raptor Lake) -- AVX-512 fused-off; 256-bit + Begleiter bleiben. --
struct Prod2RaptorLakeSignature final : MachineSimdSignature<Prod2RaptorLakeSignature> {
    [[nodiscard]] static constexpr std::string_view do_machine_id() noexcept { return "prod2_raptor_lake"; }
    [[nodiscard]] static constexpr std::string_view do_host_isa() noexcept { return Amd64HostIsaAxis::do_host_isa(); }
    [[nodiscard]] static constexpr std::span<SimdFeatureFlag const> do_signature() noexcept {
        static constexpr std::array kFlags{kAvx2, kFma, kF16c, kAvxVnni, kBmi2, kPopcnt, kGfni, kVaes, kVpclmulqdq};
        return kFlags;
    }
};

// -- Odroid-H4-Klasse: Intel Alder Lake-N (Gracemont) -- kein AVX-512; 256-bit + Begleiter. --
struct OdroidGracemontSignature final : MachineSimdSignature<OdroidGracemontSignature> {
    [[nodiscard]] static constexpr std::string_view do_machine_id() noexcept { return "odroid_gracemont"; }
    [[nodiscard]] static constexpr std::string_view do_host_isa() noexcept { return Amd64HostIsaAxis::do_host_isa(); }
    [[nodiscard]] static constexpr std::span<SimdFeatureFlag const> do_signature() noexcept {
        static constexpr std::array kFlags{kAvx2, kFma, kF16c, kAvxVnni, kBmi2, kPopcnt, kGfni, kVaes, kVpclmulqdq};
        return kFlags;
    }
};

// -- Konzept-Erfuellung + Signatur-Wohlgeformtheit (alles compile-time) ---------------------------
static_assert(MachineSimdSignatureConcept<Prod1Zen5Signature>);
static_assert(MachineSimdSignatureConcept<Prod2RaptorLakeSignature>);
static_assert(MachineSimdSignatureConcept<OdroidGracemontSignature>);
static_assert(std::is_empty_v<Prod1Zen5Signature> && !std::is_polymorphic_v<Prod1Zen5Signature>);
// prod1: exakt 13 avx512-Flags, KEIN fp16, VP2INTERSECT vorhanden (Zen5-Neuzugang):
static_assert(count_avx512_flags(Prod1Zen5Signature::signature()) == 13);
static_assert(!Prod1Zen5Signature::has_flag("avx512_fp16"));
static_assert(Prod1Zen5Signature::has_flag("avx512_vp2intersect"));
static_assert(Prod1Zen5Signature::has_flag("avx512_vbmi2") && Prod1Zen5Signature::has_flag("gfni"));
// prod2/odroid: AVX-512 fused-off/absent, aber avx_vnni + Begleiter bleiben:
static_assert(count_avx512_flags(Prod2RaptorLakeSignature::signature()) == 0);
static_assert(count_avx512_flags(OdroidGracemontSignature::signature()) == 0);
static_assert(Prod2RaptorLakeSignature::has_flag("avx_vnni") && !Prod2RaptorLakeSignature::has_flag("avx512f"));
static_assert(OdroidGracemontSignature::has_flag("avx2") && !OdroidGracemontSignature::has_flag("avx512f"));
// jede Signatur bezieht nur Katalog-bekannte Flags (Single-Source-Kopplung):
static_assert(signature_within_catalog(Prod1Zen5Signature::signature()));
static_assert(signature_within_catalog(Prod2RaptorLakeSignature::signature()));
static_assert(signature_within_catalog(OdroidGracemontSignature::signature()));

} // namespace comdare::cache_engine::measurement
