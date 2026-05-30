#pragma once
// V41.F.6.1.R7.1.a.2 Topic-Config-Set fuer Hardware-Topic
//
// @topic hardware
//
// Analog queuing-Topic (auch 2 Achsen): TopicConfigSet bietet beide Achsen
// separat fuer PermutationEngine + ein kombiniertes Cartesian-Product fuer
// ISA x Plattform.
//
// **Doku-Referenz:** Doku 11 §15.7 (TopicConfigSet + CacheEngineBuilder),
// §15.4 (PermutationEngine)
//
// Pflicht-Member analog axis_06_allocator/topic_allocator_config_set.hpp:
//   - StaticAxisVariants_<id> pro Achse
//   - StaticAxisVariants als Default (zeigt auf Haupt-Achse)
//   - CartesianISAxPlatform fuer kombinierte Permutationen
//   - AspectIterations<V> Pflicht-Template fuer F.6.1.E (heute Skelett)
//   - aspect_values<V>() leerer span (heute Skelett)

#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_registry.hpp>

// R7.5.i.2: axis_09_isa = Haupt-CPU-ISA (Amd64/Aarch64/RiscV/PowerPc, User-Direktive
// 2026-05-27: "ISA der CPU selbst"). SIMD-Extensions sind separate Sub-Achse.
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>

// R7.5.j: axis_09b_simd_extension = SIMD/Accelerator-Sub-Achse (NoExt/Sse2/Avx2/Avx512/
// Neon/Sve2/Rvv/CudaGh200, User-Direktive 2026-05-27: "Erweiterungsbausteine als Sub-Achse").
// Pro Permutation 0..1 Extension; Compat-Constraint zur Haupt-ISA.
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_registry.hpp>

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::hardware {

namespace mp = boost::mp11;

// ── R5.C.3 / #704 Cross-Axis-Constraint: ISA (axis_09) × SIMD-Extension (axis_09b) ──────────────────────────
// Korrektheit des Permutationsraums (Compile-Time, [[no-runtime-switch]]): eine SIMD-Extension passt NUR zu
// einer ISA, deren cpu_family() sie unterstuetzt. Sonst emittiert der Hardware-Konfigurator physisch unmoegliche
// Varianten (z.B. Neon-SIMD auf x86_64, Avx512 auf aarch64). NoExt ist mit JEDER ISA kompatibel (compatible_*
// alle true). Unbekannte ISA -> konservativ erlaubt (kein stiller Verlust).
template <class Isa, class Ext>
[[nodiscard]] constexpr bool isa_simd_compatible() noexcept {
    std::string_view const fam = Isa::cpu_family();
    if (fam == "x86_64")  return Ext::compatible_with_x86();
    if (fam == "aarch64") return Ext::compatible_with_arm();
    if (fam == "riscv64") return Ext::compatible_with_riscv();
    if (fam == "ppc64le") return Ext::compatible_with_powerpc();
    return true;
}

/// mp_remove_if-Praedikat: true fuer ein PHYSISCH UNMOEGLICHES <Isa, Ext, Platform>-Tupel (wird ausgefiltert).
/// Platform (Tupel-Position 2) ist orthogonal zur ISA×SIMD-Constraint und fliesst NICHT ein.
template <class PermTuple>
using IsaSimdIncompatible =
    mp::mp_bool<!isa_simd_compatible<mp::mp_at_c<PermTuple, 0>, mp::mp_at_c<PermTuple, 1>>()>;

// ── R5.C.3 / #704 (Erweiterung) Cross-Axis-Constraint: ISA (axis_09) × Plattform-Familie (axis_12) ──────────
// ZWEITE physische Constraint im selben Permutationsraum: eine Mikroarchitektur-ISA laeuft NUR auf einer
// Plattform DERSELBEN CPU-Familie. Sonst emittiert der Konfigurator unmoegliche Varianten (Amd64-ISA auf einer
// Aarch64-Plattform, Aarch64-ISA auf einer x86_64-Plattform, ...). Die GENERIC-Plattform ist familien-agnostisch
// (portabler Baseline-Build) und mit JEDER ISA kompatibel. Re-nutzt die VORHANDENEN semantischen Accessoren beider
// Achsen (Isa::cpu_family() + Platform::flag_suffix()) — KEINE neue Familie-Property (kein Bloat, P2.C-Lektion).
// Unbekannte Plattform -> konservativ erlaubt (kein stiller Verlust, analog isa_simd_compatible).
template <class Isa, class Platform>
[[nodiscard]] constexpr bool isa_platform_compatible() noexcept {
    std::string_view const plat = Platform::flag_suffix();
    if (plat == "GENERIC") return true;                       // familien-agnostische Baseline-Plattform
    std::string_view const fam = Isa::cpu_family();
    if (plat == "X86_64")  return fam == "x86_64";
    if (plat == "AARCH64") return fam == "aarch64";
    if (plat == "RISCV64") return fam == "riscv64";
    if (plat == "PPC64LE") return fam == "ppc64le";
    return true;                                              // unbekannte Plattform -> konservativ erlaubt
}

/// mp_remove_if-Praedikat fuer das 2-Tupel <Isa, Platform> (Plattform an Position 1).
template <class PermTuple>
using IsaPlatformIncompatible2 =
    mp::mp_bool<!isa_platform_compatible<mp::mp_at_c<PermTuple, 0>, mp::mp_at_c<PermTuple, 1>>()>;

/// mp_remove_if-Praedikat fuer das 3-Tupel <Isa, Ext, Platform> (Plattform an Position 2).
template <class PermTuple>
using IsaPlatformIncompatible3 =
    mp::mp_bool<!isa_platform_compatible<mp::mp_at_c<PermTuple, 0>, mp::mp_at_c<PermTuple, 2>>()>;

/**
 * @brief TopicConfigSet — zentrale Konfiguration fuer Topic `hardware`
 *
 * Wird von PermutationEngine<TopicConfigSets...> konsumiert. Kombiniert
 * statische Achs-Varianten beider Achsen (axis_09 ISA + axis_12 Platform)
 * mit optional dynamischen iterable_aspect_t pro Wrapper (F.6.1.E).
 */
struct TopicConfigSet {
    // axis_09_isa: Haupt-CPU-ISA (R7.5.i.2 nach User-Korrektur: Amd64/Aarch64/RiscV/PowerPc)
    using StaticAxisVariants_09 = axis_09_isa::EnabledIsas;

    // axis_09b_simd_extension: SIMD/Accelerator-Sub-Achse (R7.5.j NEU)
    // (NoExt/Sse2/Avx2/Avx512/Neon/Sve2/Rvv/CudaGh200) — pro Permutation 0..1 mit Compat-Check
    using StaticAxisVariants_09b = axis_09b_simd_extension::EnabledExtensions;

    // axis_12_general_hardware: Plattform-Familie (Generic/X86_64/Aarch64)
    using StaticAxisVariants_12 = axis_12_general_hardware::EnabledPlatforms;

    // Default-StaticAxisVariants — PermutationEngine 1-Topic-Variante nimmt axis_12
    // (Plattform-Konfiguration ist die uebergeordnete Achse, ISA als Sub-Permutation)
    using StaticAxisVariants = StaticAxisVariants_12;

    // ROH-Produkt axis_09 (Haupt-ISA) x axis_12 (Plattform) — bewusst UNGEFILTERT erhalten (Diagnose/Rueckwaerts-
    // Kompatibilitaet; enthaelt physisch unmoegliche Paare wie Amd64-ISA x Aarch64-Plattform).
    using CartesianIsa09xPlatform12 = mp::mp_product<
        mp::mp_list,
        StaticAxisVariants_09,
        StaticAxisVariants_12
    >;

    // R5.C.3 / #704 (Erweiterung): PRODUKTIV konsumierbares ISA×Plattform-Set — der ISA×Plattform-Cross-Constraint
    // ist angewandt (mp_remove_if IsaPlatformIncompatible2). Familien-fremde Paare (Amd64 x Aarch64-Plattform usw.)
    // sind ausgefiltert; GENERIC-Plattform + jede-ISA bleibt erhalten.
    using FilteredIsa09xPlatform12 =
        mp::mp_remove_if<CartesianIsa09xPlatform12, IsaPlatformIncompatible2>;

    // ROH-Produkt axis_09 x axis_09b x axis_12 (Haupt-ISA x SIMD-Ext x Plattform) — bewusst UNGEFILTERT
    // erhalten (Diagnose/Rueckwaerts-Kompatibilitaet; enthaelt physisch unmoegliche Paare).
    using CartesianIsa09xExt09bxPlatform12 = mp::mp_product<
        mp::mp_list,
        StaticAxisVariants_09,
        StaticAxisVariants_09b,
        StaticAxisVariants_12
    >;

    // R5.C.3 / #704: PRODUKTIV konsumiertes Set — BEIDE Cross-Constraints sind JETZT ANGEWANDT (verkettete
    // mp_remove_if): zuerst ISA×SIMD (Sse2/Avx2/Avx512 nur x86_64, Neon/Sve2 nur aarch64, Rvv nur riscv64),
    // DANN ISA×Plattform (Amd64-ISA nicht auf Aarch64-Plattform usw.). Erst die Verkettung schliesst die
    // urspruengliche Luecke, dass das 3-Tupel zwar SIMD-, aber NICHT plattform-konsistent war (z.B. <Amd64,
    // Avx2, Aarch64-Plattform>: SIMD-kompatibel, aber physisch unmoeglich). NoExt + GENERIC + passende-ISA bleibt.
    using FilteredIsa09xExt09bxPlatform12 =
        mp::mp_remove_if<
            mp::mp_remove_if<CartesianIsa09xExt09bxPlatform12, IsaSimdIncompatible>,
            IsaPlatformIncompatible3>;

    /**
     * @brief Pro-Vendor iterable Aspekt-Typ (F.6.1.E hybride Laufzeit-Permutation)
     *
     * Default: void (kein iterable_aspect_t). Vendor kann selbst typename
     * iterable_aspect_t = std::size_t (etc.) definieren — dann generiert
     * PermutationEngine Hybrid-Variant (1 Binary + Runtime-Loop).
     */
    template <class Vendor>
    using AspectIterations = std::conditional_t<
        requires { typename Vendor::iterable_aspect_t; },
        void,  // bis F.6.1.E aktiv: immer void (Skelett)
        void
    >;

    /**
     * @brief Pro-Vendor iterable Werte (F.6.1.E Stufe 3+)
     *
     * Heute leerer span. F.6.1.E ergaenzt: wenn Vendor::iterable_values()
     * vorhanden, liefere diese (Compile-Time-Auswertung via if constexpr).
     */
    template <class /*Vendor*/>
    static constexpr auto aspect_values() noexcept {
        return std::array<int, 0>{};
    }
};

}  // namespace comdare::cache_engine::hardware
