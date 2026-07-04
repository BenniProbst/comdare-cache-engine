// test_cpuid_probe.cpp - REV 5.3 K07a Discover-Phase Atom: CpuidProbeAtom
// Wird auf i7-1270P / x86_64 ausgefuehrt und verifiziert grundlegende Discovery.
// Auf ARM/RISC-V: erwartet non-x86 Fallback ohne Crash.

#include "cache_engine/platform_probe/cpuid_probe.hpp"
#include "cache_engine/platform_probe/cpuid_platform_probe.hpp"

#include <gtest/gtest.h>

namespace pp = comdare::cache_engine::platform_probe;

TEST(CpuidProbe, ProbeDoesNotCrash) {
    auto result = pp::probe_cpuid();
    SUCCEED();
}

TEST(CpuidProbe, VendorIsSensible) {
    auto result = pp::probe_cpuid();

#if defined(__x86_64__) || defined(_M_X64)
    // Auf x86_64 sollte Vendor entweder Intel oder AMD sein
    EXPECT_TRUE(result.vendor == "GenuineIntel" || result.vendor == "AuthenticAMD" ||
                result.vendor.find("Hygon") != std::string::npos)
        << "Unbekannter Vendor: " << result.vendor;
#else
    // ARM/RISC-V: non-x86 Fallback
    EXPECT_EQ(result.vendor, "non-x86");
#endif
}

TEST(CpuidProbe, CacheLineBytesIsValid) {
    auto result = pp::probe_cpuid();
    // Erwartet: 64 (x86, ARM-Standard), 128 (Apple Silicon), 256 (A64FX)
    EXPECT_TRUE(result.cache_line_bytes == 64 || result.cache_line_bytes == 128 || result.cache_line_bytes == 256)
        << "Unerwartete Cache-Line-Size: " << result.cache_line_bytes;
}

TEST(CpuidPlatformProbe, YieldsSanePropertySet) {
    pp::CpuidPlatformProbe probe;
    auto                   props = probe.discover_and_measure();

#if defined(__x86_64__) || defined(_M_X64)
    auto raw = pp::probe_cpuid();
    EXPECT_GE(props.usable_simd_width_bytes, static_cast<std::uint16_t>(16));
    EXPECT_TRUE(props.has_software_prefetch);
    EXPECT_FALSE(raw.vendor.empty());
    EXPECT_GT(raw.cpu_family, static_cast<std::uint16_t>(0));
    EXPECT_EQ(props.measured_metrics.at("feature.avx2"), raw.has_avx2 ? 1.0 : 0.0);
#else
    EXPECT_FALSE(props.has_software_prefetch);
    EXPECT_TRUE(props.usable_simd_width_bytes == 0 || props.usable_simd_width_bytes == 16);
#endif

    EXPECT_FALSE(props.has_hybrid_cores);
    EXPECT_FALSE(props.cpu_core_atom_perf_separation);
    EXPECT_EQ(props.preferred_pinning_policy, static_cast<std::uint16_t>(0));
}

#if defined(__x86_64__) || defined(_M_X64)
TEST(CpuidProbe, BrandStringNonEmpty) {
    auto result = pp::probe_cpuid();
    EXPECT_FALSE(result.brand_string.empty());
    // Block AO i7-1270P sollte "Intel" + "1270P" enthalten falls passend
    // (nicht erzwungen - laeuft auch auf anderen CPUs)
}

TEST(CpuidProbe, IntelAlderLakeHasAvx2NotAvx512) {
    auto result = pp::probe_cpuid();

    if (result.brand_string.find("1270P") != std::string::npos ||
        result.brand_string.find("12th Gen") != std::string::npos ||
        result.brand_string.find("13th Gen") != std::string::npos ||
        result.brand_string.find("14th Gen") != std::string::npos) {
        // Alder Lake / Raptor Lake: AVX2 ja, AVX-512 nein (post-Microcode)
        EXPECT_TRUE(result.has_avx2) << "Intel Alder/Raptor Lake erwartet has_avx2=true";
        EXPECT_FALSE(result.has_avx512f) << "Intel Alder/Raptor Lake erwartet has_avx512f=false (post-Microcode)";
    }
}
#endif

#if defined(__APPLE__) && defined(__aarch64__)
TEST(CpuidProbe, AppleSiliconHasNeonAnd128ByteCacheLine) {
    auto result = pp::probe_cpuid();
    EXPECT_TRUE(result.has_neon);
    EXPECT_EQ(result.cache_line_bytes, 128u);
}
#endif
