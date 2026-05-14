#pragma once
// cpuid_probe.hpp - REV 5.3 K07a Discover-Phase Atom: CpuidProbeAtom
// Multi-OS Probe: Windows + Linux + macOS, x86_64 + ARM64 + RISC-V

#include <array>
#include <cstdint>
#include <cstring>
#include <string>

#if defined(_MSC_VER)
#  include <intrin.h>
#endif

namespace comdare::cache_engine::platform_probe {

/// CpuidResult - Roh-Werte aus CPUID-Aufruf (x86) oder Fallback-Werten
struct CpuidResult {
    std::uint32_t eax = 0;
    std::uint32_t ebx = 0;
    std::uint32_t ecx = 0;
    std::uint32_t edx = 0;
};

/// ProbeResults - kompakte Auto-Discovery-Ergebnisse
struct CpuidProbeResults {
    std::string vendor;                ///< "GenuineIntel", "AuthenticAMD", "Apple", ...
    std::string brand_string;          ///< "Intel(R) Core(TM) i7-1270P @ 2.20GHz" etc.

    bool has_sse2 = false;
    bool has_sse42 = false;
    bool has_avx = false;
    bool has_avx2 = false;
    bool has_avx512f = false;
    bool has_avx512bw = false;
    bool has_avx512vl = false;
    bool has_bmi1 = false;
    bool has_bmi2 = false;
    bool has_popcnt = false;
    bool has_pclmulqdq = false;

    bool has_neon = false;             ///< ARM64
    bool has_sve = false;
    bool has_sve2 = false;

    std::uint8_t physical_cores = 0;
    std::uint8_t logical_cores = 0;
    std::uint32_t cache_line_bytes = 0;
};

/// x86-CPUID Wrapper (Windows MSVC + Linux GCC/Clang + macOS Clang)
inline CpuidResult cpuid(std::uint32_t leaf, std::uint32_t subleaf = 0) noexcept {
    CpuidResult r{};
#if defined(__x86_64__) || defined(_M_X64)
#  if defined(_MSC_VER)
    int regs[4] = {0, 0, 0, 0};
    __cpuidex(regs, static_cast<int>(leaf), static_cast<int>(subleaf));
    r.eax = static_cast<std::uint32_t>(regs[0]);
    r.ebx = static_cast<std::uint32_t>(regs[1]);
    r.ecx = static_cast<std::uint32_t>(regs[2]);
    r.edx = static_cast<std::uint32_t>(regs[3]);
#  else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx)
        : "a"(leaf), "c"(subleaf));
#  endif
#else
    (void)leaf;
    (void)subleaf;
#endif
    return r;
}

/// Probe ausfuehren - liefert CpuidProbeResults
inline CpuidProbeResults probe_cpuid() {
    CpuidProbeResults result{};

#if defined(__x86_64__) || defined(_M_X64)
    // Leaf 0: Vendor + max leaf
    auto leaf0 = cpuid(0);
    std::array<char, 13> vendor{};
    std::uint32_t v[3] = { leaf0.ebx, leaf0.edx, leaf0.ecx };
    std::memcpy(vendor.data(), v, 12);
    vendor[12] = '\0';
    result.vendor = vendor.data();

    // Brand string: Leaves 0x80000002-0x80000004
    std::array<char, 49> brand{};
    auto b1 = cpuid(0x80000002);
    auto b2 = cpuid(0x80000003);
    auto b3 = cpuid(0x80000004);
    std::uint32_t bb[12] = {
        b1.eax, b1.ebx, b1.ecx, b1.edx,
        b2.eax, b2.ebx, b2.ecx, b2.edx,
        b3.eax, b3.ebx, b3.ecx, b3.edx };
    std::memcpy(brand.data(), bb, 48);
    brand[48] = '\0';
    result.brand_string = brand.data();

    // Leaf 1: Standard Feature Flags
    auto leaf1 = cpuid(1);
    result.has_sse2     = (leaf1.edx & (1u << 26)) != 0;
    result.has_sse42    = (leaf1.ecx & (1u << 20)) != 0;
    result.has_avx      = (leaf1.ecx & (1u << 28)) != 0;
    result.has_popcnt   = (leaf1.ecx & (1u << 23)) != 0;
    result.has_pclmulqdq = (leaf1.ecx & (1u << 1)) != 0;

    // Leaf 7 sub 0: Extended Feature Flags
    auto leaf7 = cpuid(7, 0);
    result.has_avx2     = (leaf7.ebx & (1u << 5))  != 0;
    result.has_bmi1     = (leaf7.ebx & (1u << 3))  != 0;
    result.has_bmi2     = (leaf7.ebx & (1u << 8))  != 0;
    result.has_avx512f  = (leaf7.ebx & (1u << 16)) != 0;
    result.has_avx512bw = (leaf7.ebx & (1u << 30)) != 0;
    result.has_avx512vl = (leaf7.ebx & (1u << 31)) != 0;

    // Cache-Line-Size aus Leaf 1 EBX[15:8] (in 8-Byte-Units, x86-standard)
    result.cache_line_bytes = ((leaf1.ebx >> 8) & 0xFF) * 8;
    if (result.cache_line_bytes == 0) {
        result.cache_line_bytes = 64;  // x86-Default
    }
#else
    // ARM64 / RISC-V Fallback - vereinfacht
    result.vendor = "non-x86";
    result.brand_string = "non-x86";
#  if defined(__aarch64__)
    result.has_neon = true;
    result.cache_line_bytes = 64;
#    if defined(__APPLE__)
    result.cache_line_bytes = 128;  // Apple Silicon P-Cores
#    endif
#  elif defined(__riscv) && (__riscv_xlen == 64)
    result.cache_line_bytes = 64;
#  endif
#endif

    return result;
}

}  // namespace comdare::cache_engine::platform_probe
