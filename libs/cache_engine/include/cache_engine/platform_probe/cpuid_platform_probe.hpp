#pragma once
// cpuid_platform_probe.hpp - AP-3/#237 Bridge: CpuidProbeResults -> IPlatformProbe

#include "../platform/i_platform_probe.hpp"
#include "cpuid_probe.hpp"

namespace comdare::cache_engine::platform_probe {

class CpuidPlatformProbe final : public platform::IPlatformProbe {
public:
    platform::PlatformPropertySet discover_and_measure() override {
        auto raw = probe_cpuid();

        platform::PlatformPropertySet props{};

#if defined(__x86_64__) || defined(_M_X64)
        props.usable_simd_width_bytes = raw.has_avx512f ? 64u : (raw.has_avx2 ? 32u : 16u);
        props.has_software_prefetch   = true;
#elif defined(__aarch64__)
        // AP-3-Follow: vollstaendige ARM/NEON/SVE-Erkennung via auxv/sysfs statt Compile-Time-Fallback.
        props.usable_simd_width_bytes = raw.has_neon ? 16u : 0u;
#else
        props.usable_simd_width_bytes = 0u;
#endif

        // AP-13: Topologie/Pinning bleibt bewusst ungesetzt (has_hybrid_cores,
        // cpu_core_atom_perf_separation, preferred_pinning_policy).
        props.measured_metrics["cpu.family"]       = static_cast<double>(raw.cpu_family);
        props.measured_metrics["cpu.model"]        = static_cast<double>(raw.cpu_model);
        props.measured_metrics["cpu.stepping"]     = static_cast<double>(raw.cpu_stepping);
        props.measured_metrics["cache_line_bytes"] = static_cast<double>(raw.cache_line_bytes);

        props.measured_metrics["feature.sse2"]      = raw.has_sse2 ? 1.0 : 0.0;
        props.measured_metrics["feature.sse42"]     = raw.has_sse42 ? 1.0 : 0.0;
        props.measured_metrics["feature.avx"]       = raw.has_avx ? 1.0 : 0.0;
        props.measured_metrics["feature.avx2"]      = raw.has_avx2 ? 1.0 : 0.0;
        props.measured_metrics["feature.avx512f"]   = raw.has_avx512f ? 1.0 : 0.0;
        props.measured_metrics["feature.avx512bw"]  = raw.has_avx512bw ? 1.0 : 0.0;
        props.measured_metrics["feature.avx512vl"]  = raw.has_avx512vl ? 1.0 : 0.0;
        props.measured_metrics["feature.bmi1"]      = raw.has_bmi1 ? 1.0 : 0.0;
        props.measured_metrics["feature.bmi2"]      = raw.has_bmi2 ? 1.0 : 0.0;
        props.measured_metrics["feature.popcnt"]    = raw.has_popcnt ? 1.0 : 0.0;
        props.measured_metrics["feature.pclmulqdq"] = raw.has_pclmulqdq ? 1.0 : 0.0;
        props.measured_metrics["feature.neon"]      = raw.has_neon ? 1.0 : 0.0;
        props.measured_metrics["feature.sve"]       = raw.has_sve ? 1.0 : 0.0;
        props.measured_metrics["feature.sve2"]      = raw.has_sve2 ? 1.0 : 0.0;

        return props;
    }
};

} // namespace comdare::cache_engine::platform_probe