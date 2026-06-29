// V41 R5.C.3 / #704 — Cross-Axis-Constraints ISA (axis_09) × {SIMD-Extension (axis_09b), Plattform (axis_12)}.
//
// Belegt, dass der Hardware-Konfigurator KEINE physisch unmoeglichen Algorithmus-Varianten mehr emittiert. ZWEI
// orthogonale physische Constraints im selben Permutationsraum:
//   (A) ISA×SIMD  — Avx512 nur x86_64, Neon nur aarch64, Rvv nur riscv64; NoExt mit JEDER ISA.
//   (B) ISA×Plattform — Amd64-ISA nur auf x86_64/GENERIC-Plattform, Aarch64-ISA nur auf aarch64/GENERIC, ...
// Das produktiv konsumierte FilteredIsa09xExt09bxPlatform12 wendet BEIDE an (verkettete mp_remove_if); erst dadurch
// faellt auch das SIMD-kompatible-aber-plattform-unmoegliche Tupel <Amd64, Avx2, Aarch64-Plattform> heraus.
// Reine Compile-Time-Constraint ([[no-runtime-switch]]).

#include <gtest/gtest.h>

#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_registry.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_registry.hpp>

#include <boost/mp11.hpp>

#include <cstddef>
#include <iostream>

namespace mp   = boost::mp11;
namespace hw   = ::comdare::cache_engine::hardware;
namespace isa  = ::comdare::cache_engine::hardware::axis_09_isa;
namespace ext  = ::comdare::cache_engine::hardware::axis_09b_simd_extension;
namespace plat = ::comdare::cache_engine::hardware::axis_12_general_hardware;

// ── (1) Der Compat-Predicate direkt: kompatible Paare erlaubt, physisch unmoegliche verboten ──
static_assert(hw::isa_simd_compatible<isa::Amd64Isa, ext::Avx512SimdExtension>(), "x86_64 + AVX512 ist kompatibel");
static_assert(hw::isa_simd_compatible<isa::Amd64Isa, ext::Sse2SimdExtension>(), "x86_64 + SSE2 kompatibel");
static_assert(hw::isa_simd_compatible<isa::Amd64Isa, ext::Avx2SimdExtension>(), "x86_64 + AVX2 kompatibel");
static_assert(hw::isa_simd_compatible<isa::Aarch64Isa, ext::NeonSimdExtension>(), "aarch64 + Neon kompatibel");
static_assert(hw::isa_simd_compatible<isa::Aarch64Isa, ext::Sve2SimdExtension>(), "aarch64 + SVE2 kompatibel");
static_assert(hw::isa_simd_compatible<isa::RiscVIsa, ext::RvvSimdExtension>(), "riscv64 + RVV kompatibel");
// NoExt ist mit JEDER ISA kompatibel
static_assert(hw::isa_simd_compatible<isa::Amd64Isa, ext::NoSimdExtension>(), "NoExt + x86_64");
static_assert(hw::isa_simd_compatible<isa::Aarch64Isa, ext::NoSimdExtension>(), "NoExt + aarch64");
static_assert(hw::isa_simd_compatible<isa::RiscVIsa, ext::NoSimdExtension>(), "NoExt + riscv64");
static_assert(hw::isa_simd_compatible<isa::PowerPcIsa, ext::NoSimdExtension>(), "NoExt + ppc64le");
// Physisch UNMOEGLICH:
static_assert(!hw::isa_simd_compatible<isa::Amd64Isa, ext::NeonSimdExtension>(), "Neon NICHT auf x86_64");
static_assert(!hw::isa_simd_compatible<isa::Amd64Isa, ext::Sve2SimdExtension>(), "SVE2 NICHT auf x86_64");
static_assert(!hw::isa_simd_compatible<isa::Amd64Isa, ext::RvvSimdExtension>(), "RVV NICHT auf x86_64");
static_assert(!hw::isa_simd_compatible<isa::Aarch64Isa, ext::Avx512SimdExtension>(), "AVX512 NICHT auf aarch64");
static_assert(!hw::isa_simd_compatible<isa::Aarch64Isa, ext::Sse2SimdExtension>(), "SSE2 NICHT auf aarch64");
static_assert(!hw::isa_simd_compatible<isa::Aarch64Isa, ext::Avx2SimdExtension>(), "AVX2 NICHT auf aarch64");
static_assert(!hw::isa_simd_compatible<isa::RiscVIsa, ext::Avx512SimdExtension>(), "AVX512 NICHT auf riscv64");

// ── (2) Filter aktiv + korrekt auf dem kartesischen Produkt ──
using Raw      = hw::TopicConfigSet::CartesianIsa09xExt09bxPlatform12;
using Filtered = hw::TopicConfigSet::FilteredIsa09xExt09bxPlatform12;

template <class P>
using IsCompatTuple = mp::mp_bool<hw::isa_simd_compatible<mp::mp_at_c<P, 0>, mp::mp_at_c<P, 1>>()>;

static_assert(mp::mp_size<Filtered>::value < mp::mp_size<Raw>::value, "Filter MUSS Tupel entfernen");
static_assert(mp::mp_size<Filtered>::value > 0, "Filter darf nicht ALLES entfernen");
static_assert(mp::mp_all_of<Filtered, IsCompatTuple>::value, "KEIN unmoegliches Paar im Filtered");
static_assert(!mp::mp_all_of<Raw, IsCompatTuple>::value, "Roh enthielt unmoegliche Paare (Defekt)");

TEST(R5C3IsaSimdCrossConstraint, FilterRemovesPhysicallyImpossiblePairs) {
    // Die static_asserts oben sind der eigentliche Beweis (Compile-Time). Hier nur Diagnose-Sichtbarkeit.
    constexpr std::size_t raw_n = mp::mp_size<Raw>::value;
    constexpr std::size_t flt_n = mp::mp_size<Filtered>::value;
    EXPECT_LT(flt_n, raw_n) << "Filtered=" << flt_n << " Raw=" << raw_n;
    EXPECT_GT(flt_n, 0u);
    SUCCEED();
}

// ── (3) ISA × Plattform-Compat-Predicate: familien-gleiche Paare + GENERIC erlaubt, familien-fremde verboten ──
static_assert(hw::isa_platform_compatible<isa::Amd64Isa, plat::X86_64HardwareProfile>(),
              "x86_64-ISA auf x86_64-Plattform");
static_assert(hw::isa_platform_compatible<isa::Aarch64Isa, plat::Aarch64HardwareProfile>(),
              "aarch64-ISA auf aarch64-Plattform");
// GENERIC-Plattform ist familien-agnostisch -> mit JEDER ISA kompatibel:
static_assert(hw::isa_platform_compatible<isa::Amd64Isa, plat::GenericHardwareProfile>(), "x86_64-ISA auf GENERIC");
static_assert(hw::isa_platform_compatible<isa::Aarch64Isa, plat::GenericHardwareProfile>(), "aarch64-ISA auf GENERIC");
static_assert(hw::isa_platform_compatible<isa::RiscVIsa, plat::GenericHardwareProfile>(), "riscv64-ISA auf GENERIC");
static_assert(hw::isa_platform_compatible<isa::PowerPcIsa, plat::GenericHardwareProfile>(), "ppc64le-ISA auf GENERIC");
// Physisch UNMOEGLICH (familien-fremd):
static_assert(!hw::isa_platform_compatible<isa::Amd64Isa, plat::Aarch64HardwareProfile>(),
              "x86_64-ISA NICHT auf aarch64-Plattform");
static_assert(!hw::isa_platform_compatible<isa::Aarch64Isa, plat::X86_64HardwareProfile>(),
              "aarch64-ISA NICHT auf x86_64-Plattform");
static_assert(!hw::isa_platform_compatible<isa::RiscVIsa, plat::X86_64HardwareProfile>(),
              "riscv64-ISA NICHT auf x86_64-Plattform");
static_assert(!hw::isa_platform_compatible<isa::PowerPcIsa, plat::Aarch64HardwareProfile>(),
              "ppc64le-ISA NICHT auf aarch64-Plattform");

// ── (4) DETERMINISTISCHER Filter-Beweis ueber das VOLLE Achsen-Produkt (build-config-unabhaengig) ──
// AllIsas(4) × AllPlatforms(3) = 12 Tupel. Physisch moeglich sind GENAU 6: GENERIC(Wildcard)×4-ISAs +
// X86_64×{Amd64} + AARCH64×{Aarch64}. Dieser Beweis haengt NICHT von den COMDARE_AXIS_*-Build-Flags ab.
using AllIsaList          = isa::AllIsas;
using AllPlatList         = plat::AllPlatforms;
using FullIsaPlat         = mp::mp_product<mp::mp_list, AllIsaList, AllPlatList>;
using FullIsaPlatFiltered = mp::mp_remove_if<FullIsaPlat, hw::IsaPlatformIncompatible2>;

template <class P>
using IsCompatIsaPlat2 = mp::mp_bool<hw::isa_platform_compatible<mp::mp_at_c<P, 0>, mp::mp_at_c<P, 1>>()>;

static_assert(mp::mp_size<FullIsaPlat>::value == 12, "AllIsas(4) × AllPlatforms(3)");
static_assert(mp::mp_size<FullIsaPlatFiltered>::value == 6, "genau 6 physisch moegliche ISA×Plattform-Paare");
static_assert(!mp::mp_all_of<FullIsaPlat, IsCompatIsaPlat2>::value, "Voll-Produkt enthielt unmoegliche Paare (Defekt)");
static_assert(mp::mp_all_of<FullIsaPlatFiltered, IsCompatIsaPlat2>::value,
              "Voll-Filtered nur physisch moegliche Paare");

// ── (5) Produktiv konsumierte Sets (Enabled-basiert): config-INVARIANTE Aussagen ──
// (>0 / strikte Reduktion NICHT, da config-abhaengig; nur immer-gueltige Invarianten.)
using RawPlat      = hw::TopicConfigSet::CartesianIsa09xPlatform12;
using FilteredPlat = hw::TopicConfigSet::FilteredIsa09xPlatform12;

static_assert(mp::mp_size<FilteredPlat>::value <= mp::mp_size<RawPlat>::value, "Filter darf nur entfernen");
static_assert(mp::mp_all_of<FilteredPlat, IsCompatIsaPlat2>::value, "Enabled-Filtered: kein familien-fremdes Paar");

// Das produktiv konsumierte 3-Wege-Set traegt JETZT BEIDE Constraints — jedes Tupel ist ISA×SIMD- UND
// ISA×Plattform-konsistent (vormalige Luecke: SIMD-kompatibel-aber-plattform-unmoeglich):
template <class P>
using IsCompatIsaPlat3 = mp::mp_bool<hw::isa_platform_compatible<mp::mp_at_c<P, 0>, mp::mp_at_c<P, 2>>()>;

static_assert(mp::mp_all_of<Filtered, IsCompatTuple>::value, "Filtered: jedes Tupel ISA×SIMD-konsistent");
static_assert(mp::mp_all_of<Filtered, IsCompatIsaPlat3>::value, "Filtered: jedes Tupel ISA×Plattform-konsistent");

TEST(R5C3IsaPlatformCrossConstraint, BothConstraintsApplied) {
    // Beweise sind die static_asserts (Compile-Time); hier nur Diagnose-Sichtbarkeit.
    constexpr std::size_t full_n = mp::mp_size<FullIsaPlat>::value;
    constexpr std::size_t full_f = mp::mp_size<FullIsaPlatFiltered>::value;
    EXPECT_EQ(full_n, 12u);
    EXPECT_EQ(full_f, 6u) << "GENERIC×4 + X86_64×Amd64 + AARCH64×Aarch64";
    EXPECT_LE(mp::mp_size<FilteredPlat>::value, mp::mp_size<RawPlat>::value);
    SUCCEED();
}

// ── (6) L-74a: DETERMINISTISCHER 3-Wege-Filter-Beweis über das VOLLE Achsen-Produkt (build-config-unabhängig) ──
// AllIsas(4) × AllExtensions(8) × AllPlatforms(3) = 96 RAW. Nach BEIDEN verketteten Constraints (ISA×SIMD + ISA×
// Plattform) bleibt die physisch-mögliche Teilmenge — das ist die L-74a-„(4 ISA × 8 SE × 3 HW)=96 → ~25"-Reduktion
// DESSELBEN 17-Slot-Binary-Build-Raums (kein eigener Genus). Build-config-INVARIANT (volle AllXxx-Listen, nicht
// die flag-abhängigen EnabledXxx) → der literale Reduktions-Faktor ist reproduzierbar (nicht flag-abhängig).
using AllExtList    = ext::AllExtensions;
using Full3         = mp::mp_product<mp::mp_list, AllIsaList, AllExtList, AllPlatList>;
using Full3Filtered = mp::mp_remove_if<mp::mp_remove_if<Full3, hw::IsaSimdIncompatible>, hw::IsaPlatformIncompatible3>;

static_assert(mp::mp_size<Full3>::value == 96, "AllIsas(4) × AllExtensions(8) × AllPlatforms(3) = 96 RAW");
static_assert(mp::mp_size<Full3Filtered>::value < 96, "L-74a: der Cross-Constraint reduziert 96 deutlich");
static_assert(mp::mp_size<Full3Filtered>::value > 0, "Filter darf nicht ALLES entfernen");
static_assert(mp::mp_all_of<Full3Filtered, IsCompatTuple>::value, "Full3-Filtered: jedes Tupel ISA×SIMD-konsistent");
static_assert(mp::mp_all_of<Full3Filtered, IsCompatIsaPlat3>::value,
              "Full3-Filtered: jedes Tupel ISA×Plattform-konsistent");
// bekannte gültige/ungültige Tripel (die L-74a-Build-Achsen-Beispiele):
static_assert(mp::mp_contains<Full3Filtered,
                              mp::mp_list<isa::Amd64Isa, ext::Avx2SimdExtension, plat::X86_64HardwareProfile>>::value,
              "Amd64 + Avx2 + x86_64-Plattform ist physisch möglich (bleibt)");
static_assert(
    !mp::mp_contains<Full3Filtered,
                     mp::mp_list<isa::Amd64Isa, ext::Avx512SimdExtension, plat::Aarch64HardwareProfile>>::value,
    "Amd64 + Avx512 + aarch64-Plattform ist physisch unmöglich (SIMD-ok, aber plattform-fremd → raus)");

TEST(R5C3IsaSimdCrossConstraint, Full3WayReductionLiteral) {
    // L-74a literal via mp_size: 96 → N (build-config-unabhängig). Der exakte N ist die physisch-mögliche Teilmenge.
    constexpr std::size_t raw96 = mp::mp_size<Full3>::value;
    constexpr std::size_t flt   = mp::mp_size<Full3Filtered>::value;
    std::cout << "[L-74a 3-Wege-Cross-Constraint] RAW(4 ISA × 8 SE × 3 HW) = " << raw96
              << " → FILTERED (physisch möglich) = " << flt << "\n";
    EXPECT_EQ(raw96, 96u);
    EXPECT_LT(flt, 96u);
    EXPECT_GT(flt, 0u);
    SUCCEED();
}
