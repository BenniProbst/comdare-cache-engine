// V41 R5.C.3 / #704 — Cross-Axis-Constraint ISA (axis_09) × SIMD-Extension (axis_09b).
//
// Belegt, dass der Hardware-Konfigurator KEINE physisch unmoeglichen Algorithmus-Varianten mehr emittiert:
// das produktiv konsumierte FilteredIsa09xExt09bxPlatform12 enthaelt nur ISA×SIMD-kompatible Tupel
// (z.B. Avx512 nur x86_64, Neon nur aarch64, Rvv nur riscv64), waehrend NoExt mit JEDER ISA erhalten bleibt.
// Reine Compile-Time-Constraint ([[no-runtime-switch]]).

#include <gtest/gtest.h>

#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/hardware/axis_09_isa/axis_09_isa_registry.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_registry.hpp>

#include <boost/mp11.hpp>

namespace mp  = boost::mp11;
namespace hw  = ::comdare::cache_engine::hardware;
namespace isa = ::comdare::cache_engine::hardware::axis_09_isa;
namespace ext = ::comdare::cache_engine::hardware::axis_09b_simd_extension;

// ── (1) Der Compat-Predicate direkt: kompatible Paare erlaubt, physisch unmoegliche verboten ──
static_assert(hw::isa_simd_compatible<isa::Amd64Isa,   ext::Avx512SimdExtension>(), "x86_64 + AVX512 ist kompatibel");
static_assert(hw::isa_simd_compatible<isa::Amd64Isa,   ext::Sse2SimdExtension>(),   "x86_64 + SSE2 kompatibel");
static_assert(hw::isa_simd_compatible<isa::Amd64Isa,   ext::Avx2SimdExtension>(),   "x86_64 + AVX2 kompatibel");
static_assert(hw::isa_simd_compatible<isa::Aarch64Isa, ext::NeonSimdExtension>(),   "aarch64 + Neon kompatibel");
static_assert(hw::isa_simd_compatible<isa::Aarch64Isa, ext::Sve2SimdExtension>(),   "aarch64 + SVE2 kompatibel");
static_assert(hw::isa_simd_compatible<isa::RiscVIsa,   ext::RvvSimdExtension>(),    "riscv64 + RVV kompatibel");
// NoExt ist mit JEDER ISA kompatibel
static_assert(hw::isa_simd_compatible<isa::Amd64Isa,   ext::NoSimdExtension>(),     "NoExt + x86_64");
static_assert(hw::isa_simd_compatible<isa::Aarch64Isa, ext::NoSimdExtension>(),     "NoExt + aarch64");
static_assert(hw::isa_simd_compatible<isa::RiscVIsa,    ext::NoSimdExtension>(),     "NoExt + riscv64");
static_assert(hw::isa_simd_compatible<isa::PowerPcIsa,  ext::NoSimdExtension>(),     "NoExt + ppc64le");
// Physisch UNMOEGLICH:
static_assert(!hw::isa_simd_compatible<isa::Amd64Isa,   ext::NeonSimdExtension>(),  "Neon NICHT auf x86_64");
static_assert(!hw::isa_simd_compatible<isa::Amd64Isa,   ext::Sve2SimdExtension>(),  "SVE2 NICHT auf x86_64");
static_assert(!hw::isa_simd_compatible<isa::Amd64Isa,   ext::RvvSimdExtension>(),   "RVV NICHT auf x86_64");
static_assert(!hw::isa_simd_compatible<isa::Aarch64Isa, ext::Avx512SimdExtension>(),"AVX512 NICHT auf aarch64");
static_assert(!hw::isa_simd_compatible<isa::Aarch64Isa, ext::Sse2SimdExtension>(),  "SSE2 NICHT auf aarch64");
static_assert(!hw::isa_simd_compatible<isa::Aarch64Isa, ext::Avx2SimdExtension>(),  "AVX2 NICHT auf aarch64");
static_assert(!hw::isa_simd_compatible<isa::RiscVIsa,   ext::Avx512SimdExtension>(),"AVX512 NICHT auf riscv64");

// ── (2) Filter aktiv + korrekt auf dem kartesischen Produkt ──
using Raw      = hw::TopicConfigSet::CartesianIsa09xExt09bxPlatform12;
using Filtered = hw::TopicConfigSet::FilteredIsa09xExt09bxPlatform12;

template <class P>
using IsCompatTuple = mp::mp_bool<hw::isa_simd_compatible<mp::mp_at_c<P, 0>, mp::mp_at_c<P, 1>>()>;

static_assert(mp::mp_size<Filtered>::value < mp::mp_size<Raw>::value, "Filter MUSS Tupel entfernen");
static_assert(mp::mp_size<Filtered>::value > 0,                       "Filter darf nicht ALLES entfernen");
static_assert(mp::mp_all_of<Filtered, IsCompatTuple>::value,          "KEIN unmoegliches Paar im Filtered");
static_assert(!mp::mp_all_of<Raw, IsCompatTuple>::value,              "Roh enthielt unmoegliche Paare (Defekt)");

TEST(R5C3IsaSimdCrossConstraint, FilterRemovesPhysicallyImpossiblePairs) {
    // Die static_asserts oben sind der eigentliche Beweis (Compile-Time). Hier nur Diagnose-Sichtbarkeit.
    constexpr std::size_t raw_n = mp::mp_size<Raw>::value;
    constexpr std::size_t flt_n = mp::mp_size<Filtered>::value;
    EXPECT_LT(flt_n, raw_n) << "Filtered=" << flt_n << " Raw=" << raw_n;
    EXPECT_GT(flt_n, 0u);
    SUCCEED();
}
