#pragma once
// cpuid_platform_probe.hpp - AP-3/#237 Bridge: CpuidProbeResults -> IPlatformProbe
// #265-b (AP-3-Follow-up-Konsum, 2026-07-06): die Bruecke konsumiert jetzt ZUSAETZLICH die vendored
// foundation-Zelle comdare::platform (SIMDDetector, Matrix-Reuse) als Laufzeit-Zweitquelle — echte
// ARM-NEON/SVE/SVE2-Erkennung via hwcap statt Compile-Time-Fallback — sowie den abgeleiteten
// ce-Baustein sysfs_cache_probe (Linux) fuer die L1/L2/L3-Hierarchie, wo CPUID nichts liefert.
// Quellen-Kette je Wert: probe_cpuid() -> vendored SIMDDetector::detect() -> sysfs (erster Treffer > 0 gewinnt).

#include "../platform/i_platform_probe.hpp"
#include "cpuid_probe.hpp"
#include "sysfs_cache_probe.hpp"

#include <comdare/platform/SIMDDetect.hpp>

namespace comdare::cache_engine::platform_probe {

class CpuidPlatformProbe final : public platform::IPlatformProbe {
public:
    platform::PlatformPropertySet discover_and_measure() override {
        auto        raw   = probe_cpuid();
        auto const& vend  = ::comdare::platform::SIMDDetector::detect(); // vendored, cached
        auto const  sysfs = probe_sysfs_cache();                         // Linux-Fallback, sonst inert

        platform::PlatformPropertySet props{};

#if defined(__x86_64__) || defined(_M_X64)
        props.usable_simd_width_bytes = raw.has_avx512f ? 64u : (raw.has_avx2 ? 32u : 16u);
        props.has_software_prefetch   = true;
#elif defined(__aarch64__)
        // #265-b: Laufzeit-Erkennung aus der vendored Zelle (hwcap) — SVE konservativ >= NEON-Breite
        // (die variable SVE-Vektorlaenge traegt CPUFeatures nicht; keine fabrizierte Breite).
        props.usable_simd_width_bytes = (vend.neon || vend.sve || vend.sve2) ? 16u : 0u;
        props.has_software_prefetch   = true; // AArch64: PRFM ist Architektur-Bestandteil
#else
        props.usable_simd_width_bytes = 0u;
#endif

        // AP-13: Topologie/Pinning bleibt bewusst ungesetzt (has_hybrid_cores,
        // cpu_core_atom_perf_separation, preferred_pinning_policy).
        props.measured_metrics["cpu.family"]   = static_cast<double>(raw.cpu_family);
        props.measured_metrics["cpu.model"]    = static_cast<double>(raw.cpu_model);
        props.measured_metrics["cpu.stepping"] = static_cast<double>(raw.cpu_stepping);
        // #265-b Quellen-Kette (erster Treffer > 0 gewinnt): CPUID -> vendored -> sysfs. Auf ARM liefert
        // CPUID nichts; die vendored Zelle traegt dort hwcap-Flags, die Groessen kommen aus sysfs.
        auto first_nonzero = [](std::uint32_t a, std::uint32_t b, std::uint32_t c) {
            return (a != 0u) ? a : ((b != 0u) ? b : c);
        };
        props.measured_metrics["cache_line_bytes"] =
            static_cast<double>(first_nonzero(raw.cache_line_bytes, vend.cache_line_size, sysfs.line_bytes));
        props.measured_metrics["cache.l1d_kb"] =
            static_cast<double>(first_nonzero(0u, vend.l1_data_cache_kb, sysfs.l1_data_kb));
        props.measured_metrics["cache.l2_kb"] = static_cast<double>(first_nonzero(0u, vend.l2_cache_kb, sysfs.l2_kb));
        props.measured_metrics["cache.l3_kb"] = static_cast<double>(first_nonzero(0u, vend.l3_cache_kb, sysfs.l3_kb));
        props.measured_metrics["cache.sysfs_available"] = sysfs.available ? 1.0 : 0.0;

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
        // #265-b: ARM-Flags aus der vendored LAUFZEIT-Erkennung (hwcap) ODER dem bisherigen raw-Pfad —
        // je nachdem, wer den Host real erkannt hat (beide sind auf x86 ehrlich 0).
        props.measured_metrics["feature.neon"] = (raw.has_neon || vend.neon) ? 1.0 : 0.0;
        props.measured_metrics["feature.sve"]  = (raw.has_sve || vend.sve) ? 1.0 : 0.0;
        props.measured_metrics["feature.sve2"] = (raw.has_sve2 || vend.sve2) ? 1.0 : 0.0;

        return props;
    }
};

} // namespace comdare::cache_engine::platform_probe