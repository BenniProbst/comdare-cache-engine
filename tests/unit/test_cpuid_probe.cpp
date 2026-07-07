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
#elif defined(__aarch64__)
    // #265-b-Angleich (Architektur-Audit 07.07.): PRFM ist AArch64-Architektur-
    // Bestandteil -> die Probe setzt has_software_prefetch unbedingt true
    // (cpuid_platform_probe.hpp); Breite = 16 bei NEON/SVE (vendored hwcap), sonst 0.
    // Vorher erwartete der Sammel-#else hier FALSE -> der arm64-Smoke (node7,
    // #270b/#276) waere an seinem eigenen Konsumenten-Test rot geworden.
    EXPECT_TRUE(props.has_software_prefetch);
    EXPECT_TRUE(props.usable_simd_width_bytes == 0 || props.usable_simd_width_bytes == 16);
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

// ── #265-b (AP-3-Follow-up-Konsum): sysfs-Laufzeitprobe + Bruecken-Quellen-Kette ────────────────────────────
#include <cache_engine/platform_probe/sysfs_cache_probe.hpp>
#include <cache_engine/platform_probe/cpuid_platform_probe.hpp>

#if defined(__linux__)
TEST(SysfsCacheProbe, LinuxHierarchyRealAndPlausible) {
    auto const info = pp::probe_sysfs_cache();
    ASSERT_TRUE(info.available) << "Linux ohne /sys/devices/system/cpu/cpu0/cache/ — unerwartet";
    EXPECT_TRUE(info.line_bytes == 32u || info.line_bytes == 64u || info.line_bytes == 128u || info.line_bytes == 256u)
        << "coherency_line_size=" << info.line_bytes;
    EXPECT_GT(info.l1_data_kb, 0u);
    EXPECT_LT(info.l1_data_kb, 4096u); // L1d > 4 MiB gibt es nicht — Parser-Plausibilitaet
    // Kreuzcheck: liefern CPUID und sysfs BEIDE eine Line-Size, muessen sie uebereinstimmen.
    auto const raw = pp::probe_cpuid();
    if (raw.cache_line_bytes != 0u && info.line_bytes != 0u) EXPECT_EQ(raw.cache_line_bytes, info.line_bytes);
}
#endif

TEST(CpuidPlatformProbe, CacheMetricsViaSourceChain) {
    pp::CpuidPlatformProbe probe;
    auto                   props = probe.discover_and_measure();
    ASSERT_TRUE(props.measured_metrics.count("cache.l1d_kb"));
    ASSERT_TRUE(props.measured_metrics.count("cache.l2_kb"));
    ASSERT_TRUE(props.measured_metrics.count("cache.l3_kb"));
    ASSERT_TRUE(props.measured_metrics.count("cache.sysfs_available"));
#if defined(__linux__)
    // Auf Linux (x86 wie ARM) muss die Kette CPUID->vendored->sysfs echte Werte liefern.
    EXPECT_GT(props.measured_metrics["cache.l1d_kb"], 0.0);
    EXPECT_GT(props.measured_metrics["cache_line_bytes"], 0.0);
    EXPECT_EQ(props.measured_metrics["cache.sysfs_available"], 1.0);
#endif
#if defined(__aarch64__)
    // #265-b-Kern: ARM-Flags kommen zur LAUFZEIT aus der vendored hwcap-Erkennung.
    EXPECT_EQ(props.measured_metrics["feature.neon"], 1.0); // jeder AArch64-Linux-Host traegt NEON/ASIMD
#endif
}

// Cross-Review 265-b: host-unabhaengige Parser-Unit-Tests (der sysfs-Realtest oben ist host-abhaengig).
TEST(SysfsCacheProbe, SizeParserUnits) {
    namespace d = pp::detail;
    EXPECT_EQ(d::parse_size_kb("32K"), 32u);
    EXPECT_EQ(d::parse_size_kb("36864K"), 36864u);
    EXPECT_EQ(d::parse_size_kb("8M"), 8192u);
    EXPECT_EQ(d::parse_size_kb("1G"), 1048576u);
    EXPECT_EQ(d::parse_size_kb(""), 0u);
    EXPECT_EQ(d::parse_size_kb("64"), 0u);  // ohne Einheit -> unverwertbar
    EXPECT_EQ(d::parse_size_kb("64X"), 0u); // unbekanntes Suffix
    EXPECT_EQ(d::parse_u32("64"), 64u);
    EXPECT_EQ(d::parse_u32(""), 0u);
}
