#pragma once
/**
 * @file SIMDDetect.hpp
 * @brief CPUID-based CPU feature detection
 *
 * Part of comdare-platform (ModuleBaseline0-Foundation)
 */

#include <cstdint>
#include <string>
#include <array>
#include <thread>

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#define COMDARE_PLATFORM_X86 1
#elif (defined(__x86_64__) || defined(__i386__)) && !defined(__EMSCRIPTEN__)
#include <cpuid.h>
#define COMDARE_PLATFORM_X86 1
#endif

#if defined(__linux__) && (defined(__aarch64__) || defined(__ARM_NEON))
#include <sys/auxv.h>
#include <asm/hwcap.h>
#define COMDARE_PLATFORM_ARM_LINUX 1
#elif defined(__APPLE__) && defined(__aarch64__)
#include <sys/sysctl.h>
#define COMDARE_PLATFORM_ARM_APPLE 1
#endif

#if defined(__riscv)
#define COMDARE_PLATFORM_RISCV 1
#if defined(__linux__)
#include <fstream>
#endif
#endif

namespace comdare::platform {

/**
 * @brief CPU feature flags detected via CPUID
 */
struct CPUFeatures {
    // SSE Family
    bool sse2 = false;
    bool sse3 = false;
    bool ssse3 = false;
    bool sse41 = false;
    bool sse42 = false;

    // AVX Family
    bool avx = false;
    bool avx2 = false;

    // AVX-512 Family
    bool avx512f = false;
    bool avx512bw = false;
    bool avx512vl = false;
    bool avx512dq = false;

    // Bit Manipulation
    bool bmi1 = false;
    bool bmi2 = false;
    bool adx = false;

    // Other
    bool popcnt = false;
    bool lzcnt = false;
    bool fma = false;

    // OS Support (x86)
    bool os_avx_support = false;
    bool os_avx512_support = false;

    // ARM Features
    bool neon = false;
    bool sve = false;
    bool sve2 = false;
    bool dotprod = false;
    bool atomics_lse = false;  // Large System Extensions (atomic CAS etc.)
    bool crc32_arm = false;
    bool aes_arm = false;
    bool sha2_arm = false;

    // RISC-V Features
    bool rvv = false;  // RISC-V Vector extension

    // Platform
    enum class Architecture : uint8_t {
        Unknown = 0,
        X86,
        X86_64,
        ARM32,
        ARM64,
        RISCV32,
        RISCV64
    };
    Architecture arch = Architecture::Unknown;

    // Cache Info
    uint32_t cache_line_size = 64;
    uint32_t l1_data_cache_kb = 0;
    uint32_t l2_cache_kb = 0;
    uint32_t l3_cache_kb = 0;

    // CPU Info
    std::string vendor;
    std::string brand;
    uint32_t family = 0;
    uint32_t model = 0;
    uint32_t stepping = 0;

    // Topology (detected at runtime)
    uint32_t physical_cores = 0;        // Physical CPU cores
    uint32_t logical_cores = 0;         // Logical cores (including SMT)
    uint32_t simd_units_per_core = 1;   // SIMD/FMA execution ports per core

    /// Platform pointer width in bits
    static constexpr uint32_t pointer_bits = sizeof(void*) * 8;
    static constexpr bool is_64bit = sizeof(void*) >= 8;
};

/**
 * @brief SIMD capability level
 */
enum class SIMDLevel {
    Scalar,   // No SIMD
    NEON,     // ARM 128-bit
    SSE42,    // x86 128-bit
    AVX2,     // x86 256-bit
    SVE,      // ARM scalable (128-2048 bit)
    AVX512    // x86 512-bit
};

/**
 * @brief CPU feature detector (singleton)
 */
class SIMDDetector {
public:
    /**
     * @brief Get detected CPU features (thread-safe, cached)
     */
    [[nodiscard]] static const CPUFeatures& detect() {
        static CPUFeatures features = detect_impl();
        return features;
    }

    /**
     * @brief Get best available SIMD level
     */
    [[nodiscard]] static SIMDLevel best_level() {
        const auto& f = detect();
        // x86
        if (f.avx512f && f.avx512bw && f.os_avx512_support) return SIMDLevel::AVX512;
        if (f.avx2 && f.os_avx_support) return SIMDLevel::AVX2;
        if (f.sse42) return SIMDLevel::SSE42;
        // ARM
        if (f.sve) return SIMDLevel::SVE;
        if (f.neon) return SIMDLevel::NEON;
        return SIMDLevel::Scalar;
    }

    /**
     * @brief Check if specific level is supported
     */
    [[nodiscard]] static bool supports(SIMDLevel level) {
        return static_cast<int>(best_level()) >= static_cast<int>(level);
    }

    /**
     * @brief Get level name as string
     */
    [[nodiscard]] static const char* level_name(SIMDLevel level) {
        switch (level) {
            case SIMDLevel::AVX512: return "AVX-512";
            case SIMDLevel::SVE:    return "ARM SVE";
            case SIMDLevel::AVX2:   return "AVX2";
            case SIMDLevel::SSE42:  return "SSE4.2";
            case SIMDLevel::NEON:   return "ARM NEON";
            default:                return "Scalar";
        }
    }

    /**
     * @brief Get optimal vector width in bytes
     */
    [[nodiscard]] static size_t optimal_vector_width() {
        switch (best_level()) {
            case SIMDLevel::AVX512: return 64;
            case SIMDLevel::SVE:    return 32;  // Conservative, SVE is 128-2048
            case SIMDLevel::AVX2:   return 32;
            case SIMDLevel::NEON:   return 16;
            case SIMDLevel::SSE42:  return 16;
            default:                return 8;
        }
    }

    /**
     * @brief Get optimal alignment
     */
    [[nodiscard]] static size_t optimal_alignment() {
        return optimal_vector_width();
    }

    /**
     * @brief Get number of SIMD/FMA execution units per physical core
     *
     * Detected via microarchitecture identification (vendor/family/model).
     * Intel Haswell+: 2, AMD Zen 2+: 2, Apple Silicon: 4.
     */
    [[nodiscard]] static uint32_t simd_units_per_core() {
        return detect().simd_units_per_core;
    }

    /**
     * @brief Get number of physical CPU cores
     */
    [[nodiscard]] static uint32_t physical_core_count() {
        return detect().physical_cores;
    }

    /**
     * @brief Get number of logical CPU cores (including SMT/HyperThreading)
     */
    [[nodiscard]] static uint32_t logical_core_count() {
        return detect().logical_cores;
    }

    /**
     * @brief Get total SIMD execution units across all physical cores
     */
    [[nodiscard]] static uint32_t total_simd_execution_units() {
        const auto& f = detect();
        return f.physical_cores * f.simd_units_per_core;
    }

    /**
     * @brief Recommended maximum thread count for SIMD-intensive workloads
     *
     * Beyond this count, threads contend for SIMD execution ports,
     * causing pipeline stalls and fallback to scalar execution.
     * (COMDARE Overprovisioning Rule)
     */
    [[nodiscard]] static uint32_t max_simd_threads() {
        return total_simd_execution_units();
    }

private:
#ifdef COMDARE_PLATFORM_X86
    static void cpuid(int info[4], int func_id) {
#ifdef _MSC_VER
        __cpuid(info, func_id);
#else
        __cpuid(func_id, info[0], info[1], info[2], info[3]);
#endif
    }

    static void cpuidex(int info[4], int func_id, int sub_id) {
#ifdef _MSC_VER
        __cpuidex(info, func_id, sub_id);
#else
        __cpuid_count(func_id, sub_id, info[0], info[1], info[2], info[3]);
#endif
    }

    static uint64_t xgetbv(uint32_t xcr) {
#ifdef _MSC_VER
        return _xgetbv(xcr);
#else
        uint32_t eax, edx;
        __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
        return (static_cast<uint64_t>(edx) << 32) | eax;
#endif
    }

    static void detect_x86(CPUFeatures& f) {
        int info[4];

        cpuid(info, 0);
        int max_id = info[0];

        char vendor[13] = {0};
        *reinterpret_cast<int*>(vendor) = info[1];
        *reinterpret_cast<int*>(vendor + 4) = info[3];
        *reinterpret_cast<int*>(vendor + 8) = info[2];
        f.vendor = vendor;

        if (max_id >= 1) {
            cpuid(info, 1);

            f.stepping = info[0] & 0xF;
            f.model = (info[0] >> 4) & 0xF;
            f.family = (info[0] >> 8) & 0xF;
            if (f.family == 0xF) {
                f.family += (info[0] >> 20) & 0xFF;
            }
            if (f.family >= 6) {
                f.model += ((info[0] >> 16) & 0xF) << 4;
            }

            f.sse2 = (info[3] >> 26) & 1;
            f.sse3 = info[2] & 1;
            f.ssse3 = (info[2] >> 9) & 1;
            f.sse41 = (info[2] >> 19) & 1;
            f.sse42 = (info[2] >> 20) & 1;
            f.popcnt = (info[2] >> 23) & 1;
            f.avx = (info[2] >> 28) & 1;
            f.fma = (info[2] >> 12) & 1;

            // Cache line size from CLFLUSH (leaf 1, EBX bits 8-15)
            f.cache_line_size = ((info[1] >> 8) & 0xFF) * 8;
            if (f.cache_line_size == 0) f.cache_line_size = 64;

            bool osxsave = (info[2] >> 27) & 1;
            if (osxsave) {
                uint64_t xcr0 = xgetbv(0);
                f.os_avx_support = ((xcr0 & 0x6) == 0x6);
                f.os_avx512_support = ((xcr0 & 0xE6) == 0xE6);
            }
        }

        if (max_id >= 7) {
            cpuidex(info, 7, 0);
            f.bmi1 = (info[1] >> 3) & 1;
            f.avx2 = (info[1] >> 5) & 1;
            f.bmi2 = (info[1] >> 8) & 1;
            f.avx512f = (info[1] >> 16) & 1;
            f.avx512dq = (info[1] >> 17) & 1;
            f.adx = (info[1] >> 19) & 1;
            f.avx512bw = (info[1] >> 30) & 1;
            f.avx512vl = (info[1] >> 31) & 1;
        }

        // Cache topology via CPUID leaf 4 (Intel) or leaf 0x8000001D (AMD)
        detect_x86_cache(f, max_id);

        cpuid(info, 0x80000000);
        int max_ext_id = info[0];

        if (max_ext_id >= 0x80000004) {
            char brand[49] = {0};
            cpuid(reinterpret_cast<int*>(brand), 0x80000002);
            cpuid(reinterpret_cast<int*>(brand + 16), 0x80000003);
            cpuid(reinterpret_cast<int*>(brand + 32), 0x80000004);
            f.brand = brand;
        }

        if (max_ext_id >= 0x80000001) {
            cpuid(info, 0x80000001);
            f.lzcnt = (info[2] >> 5) & 1;
        }

        // AMD cache info via extended leaf
        if (f.l1_data_cache_kb == 0 && max_ext_id >= static_cast<int>(0x8000001D)) {
            detect_x86_cache_amd(f);
        }

        // Topology and SIMD execution unit detection
        detect_x86_topology(f);
        detect_x86_simd_eu(f);
    }

    static void detect_x86_cache(CPUFeatures& f, int max_id) {
        if (max_id < 4) return;
        // CPUID leaf 4: Deterministic Cache Parameters (Intel)
        for (int sub = 0; sub < 16; ++sub) {
            int info[4];
            cpuidex(info, 4, sub);
            int type = info[0] & 0x1F;
            if (type == 0) break;  // No more caches
            int level = (info[0] >> 5) & 0x7;
            uint32_t line_size = (info[1] & 0xFFF) + 1;
            uint32_t partitions = ((info[1] >> 12) & 0x3FF) + 1;
            uint32_t ways = ((info[1] >> 22) & 0x3FF) + 1;
            uint32_t sets = info[2] + 1;
            uint32_t size_kb = (ways * partitions * line_size * sets) / 1024;
            if (level == 1 && (type == 1 || type == 3)) {
                f.l1_data_cache_kb = size_kb;
            } else if (level == 2) {
                f.l2_cache_kb = size_kb;
            } else if (level == 3) {
                f.l3_cache_kb = size_kb;
            }
        }
    }

    static void detect_x86_cache_amd(CPUFeatures& f) {
        // CPUID leaf 0x8000001D: Cache Topology (AMD)
        for (int sub = 0; sub < 16; ++sub) {
            int info[4];
            cpuidex(info, static_cast<int>(0x8000001D), sub);
            int type = info[0] & 0x1F;
            if (type == 0) break;
            int level = (info[0] >> 5) & 0x7;
            uint32_t line_size = (info[1] & 0xFFF) + 1;
            uint32_t partitions = ((info[1] >> 12) & 0x3FF) + 1;
            uint32_t ways = ((info[1] >> 22) & 0x3FF) + 1;
            uint32_t sets = info[2] + 1;
            uint32_t size_kb = (ways * partitions * line_size * sets) / 1024;
            if (level == 1 && (type == 1 || type == 3)) {
                f.l1_data_cache_kb = size_kb;
            } else if (level == 2) {
                f.l2_cache_kb = size_kb;
            } else if (level == 3) {
                f.l3_cache_kb = size_kb;
            }
        }
    }
    static void detect_x86_topology(CPUFeatures& f) {
        f.logical_cores = std::thread::hardware_concurrency();
        if (f.logical_cores == 0) f.logical_cores = 1;

        // CPUID leaf 0xB (Extended Topology Enumeration)
        int info[4];
        cpuidex(info, 0xB, 0);
        uint32_t level_type = (info[2] >> 8) & 0xFF;

        if (level_type == 1) {  // SMT level
            uint32_t smt_width = info[1] & 0xFFFF;
            if (smt_width > 0) {
                f.physical_cores = f.logical_cores / smt_width;
                if (f.physical_cores == 0) f.physical_cores = 1;
                return;
            }
        }

        // Fallback: HTT flag from CPUID leaf 1
        cpuid(info, 1);
        bool htt = (info[3] >> 28) & 1;
        f.physical_cores = (htt && f.logical_cores > 1)
            ? f.logical_cores / 2
            : f.logical_cores;
        if (f.physical_cores == 0) f.physical_cores = 1;
    }

    static void detect_x86_simd_eu(CPUFeatures& f) {
        if (f.vendor.find("GenuineIntel") != std::string::npos) {
            // Intel Haswell+ (family 6, model >= 60): 2 FMA/vector ports
            f.simd_units_per_core = (f.family == 6 && f.model >= 60) ? 2 : 1;
        } else if (f.vendor.find("AuthenticAMD") != std::string::npos) {
            // AMD Zen 2+ (family >= 25, or family 23 model >= 49): 2x256-bit FMA
            // Zen 1 (family 23, model < 49): 2x128-bit = 1 effective 256-bit unit
            f.simd_units_per_core = (f.family >= 25 || (f.family == 23 && f.model >= 49)) ? 2 : 1;
        }
    }
#endif // COMDARE_PLATFORM_X86

#ifdef COMDARE_PLATFORM_ARM_LINUX
    static void detect_arm_linux(CPUFeatures& f) {
        unsigned long hwcap = getauxval(AT_HWCAP);
        unsigned long hwcap2 = getauxval(AT_HWCAP2);

#ifdef HWCAP_NEON
        f.neon = (hwcap & HWCAP_NEON) != 0;
#endif
#ifdef HWCAP_ASIMD
        f.neon = (hwcap & HWCAP_ASIMD) != 0;  // AArch64 NEON
#endif
#ifdef HWCAP_ATOMICS
        f.atomics_lse = (hwcap & HWCAP_ATOMICS) != 0;
#endif
#ifdef HWCAP_CRC32
        f.crc32_arm = (hwcap & HWCAP_CRC32) != 0;
#endif
#ifdef HWCAP_AES
        f.aes_arm = (hwcap & HWCAP_AES) != 0;
#endif
#ifdef HWCAP_SHA2
        f.sha2_arm = (hwcap & HWCAP_SHA2) != 0;
#endif
#ifdef HWCAP_ASIMDDP
        f.dotprod = (hwcap & HWCAP_ASIMDDP) != 0;
#endif
#ifdef HWCAP_SVE
        f.sve = (hwcap & HWCAP_SVE) != 0;
#endif
#ifdef HWCAP2_SVE2
        f.sve2 = (hwcap2 & HWCAP2_SVE2) != 0;
#endif
        (void)hwcap2;
        f.vendor = "ARM";

        // Topology (ARM typically no SMT)
        f.logical_cores = std::thread::hardware_concurrency();
        if (f.logical_cores == 0) f.logical_cores = 1;
        f.physical_cores = f.logical_cores;
        f.simd_units_per_core = 2;  // Most Cortex-A7x+ have 2 NEON units
    }
#endif // COMDARE_PLATFORM_ARM_LINUX

#ifdef COMDARE_PLATFORM_ARM_APPLE
    static void detect_arm_apple(CPUFeatures& f) {
        f.neon = true;  // All Apple Silicon has NEON
        f.aes_arm = true;
        f.sha2_arm = true;
        f.atomics_lse = true;
        f.crc32_arm = true;
        f.dotprod = true;
        f.simd_units_per_core = 4;  // Apple Silicon: 4 NEON execution units per core
        f.vendor = "Apple";

        char brand[256] = {0};
        size_t len = sizeof(brand);
        if (sysctlbyname("machdep.cpu.brand_string", brand, &len, nullptr, 0) == 0) {
            f.brand = brand;
        }

        // Topology (Apple Silicon has no SMT)
        f.logical_cores = std::thread::hardware_concurrency();
        if (f.logical_cores == 0) f.logical_cores = 1;
        f.physical_cores = f.logical_cores;
    }
#endif // COMDARE_PLATFORM_ARM_APPLE

#ifdef COMDARE_PLATFORM_RISCV
    static void detect_riscv(CPUFeatures& f) {
        f.vendor = "RISC-V";
        f.logical_cores = std::thread::hardware_concurrency();
        if (f.logical_cores == 0) f.logical_cores = 1;
        f.physical_cores = f.logical_cores;
#if defined(__riscv_v)
        f.rvv = true;
#endif
        // Runtime detection via /proc/cpuinfo on Linux
#if defined(__linux__)
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (cpuinfo.is_open()) {
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.find("isa") != std::string::npos) {
                    // ISA string contains extension letters, e.g. rv64imafdc_v
                    if (line.find("_v") != std::string::npos ||
                        line.find("rv64imafdc_v") != std::string::npos ||
                        line.find("rv64gcv") != std::string::npos) {
                        f.rvv = true;
                    }
                }
                if (line.find("uarch") != std::string::npos ||
                    line.find("model name") != std::string::npos) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos && pos + 2 < line.size()) {
                        f.brand = line.substr(pos + 2);
                    }
                }
            }
        }
#endif
    }
#endif

    static CPUFeatures detect_impl() {
        CPUFeatures f;

        // Detect architecture
#if defined(__x86_64__) || defined(_M_X64)
        f.arch = CPUFeatures::Architecture::X86_64;
#elif defined(__i386__) || defined(_M_IX86)
        f.arch = CPUFeatures::Architecture::X86;
#elif defined(__aarch64__) || defined(_M_ARM64)
        f.arch = CPUFeatures::Architecture::ARM64;
#elif defined(__arm__) || defined(_M_ARM)
        f.arch = CPUFeatures::Architecture::ARM32;
#elif defined(__riscv) && (__riscv_xlen == 64)
        f.arch = CPUFeatures::Architecture::RISCV64;
#elif defined(__riscv) && (__riscv_xlen == 32)
        f.arch = CPUFeatures::Architecture::RISCV32;
#endif

        // Platform-specific detection
#ifdef COMDARE_PLATFORM_X86
        detect_x86(f);
#endif
#ifdef COMDARE_PLATFORM_ARM_LINUX
        detect_arm_linux(f);
#endif
#ifdef COMDARE_PLATFORM_ARM_APPLE
        detect_arm_apple(f);
#endif
#ifdef COMDARE_PLATFORM_RISCV
        detect_riscv(f);
#endif

        // Fallback topology if platform-specific detection didn't run
        if (f.logical_cores == 0) {
            f.logical_cores = std::thread::hardware_concurrency();
            if (f.logical_cores == 0) f.logical_cores = 1;
        }
        if (f.physical_cores == 0) {
            f.physical_cores = f.logical_cores;
        }

        return f;
    }
};

} // namespace comdare::platform
