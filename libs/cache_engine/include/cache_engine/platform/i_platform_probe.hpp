#pragma once
// IPlatformProbe + IPlatformPropertyClassifier + ICacheEngineOptionPublisher
// REV 3 K3.2 generisch (OHNE plattform-spezifische Klassen wie RyzenX3DProbe)
// Termin 7 / 13_saeule_b_plattform_modell §1 + 10_korrektur §K3.2

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace comdare::cache_engine::platform {

// Plattform-Property-Set, das aus Auto-Discovery + Auto-Vermessung entsteht
struct PlatformPropertySet {
    bool                          has_asymmetric_l3             = false;
    bool                          has_hybrid_cores              = false;
    bool                          has_hbm_tier                  = false;
    bool                          has_software_prefetch         = false;
    bool                          has_hardware_transactional    = false;
    bool                          cpu_core_atom_perf_separation = false;
    std::uint16_t                 preferred_pinning_policy      = 0; // PinningPolicyId
    std::uint16_t                 usable_simd_width_bytes       = 0;
    std::map<std::string, double> measured_metrics; // freie Plattform-Properties
};

// Auto-Discovery (statisch + dynamisch) — generisch fuer JEDE Plattform
class IPlatformProbe {
public:
    virtual ~IPlatformProbe() = default;

    // Discover (statisch via CPUID/sysfs/proc) + Measure (dynamisch via Mikrobenchmark)
    virtual PlatformPropertySet discover_and_measure() = 0;
};

// Klassifiziert die Discovery-Ergebnisse zu konkreten Properties
class IPlatformPropertyClassifier {
public:
    virtual ~IPlatformPropertyClassifier() = default;

    // Wandelt rohe Probe-Ergebnisse in Properties um (z.B. "verschiedene L3-Werte → has_asymmetric_l3")
    virtual PlatformPropertySet classify(PlatformPropertySet const& raw) = 0;
};

// Stellt klassifizierte Properties allen Permutations-Modulen zur Verfuegung
class ICacheEngineOptionPublisher {
public:
    virtual ~ICacheEngineOptionPublisher() = default;

    // Pro Modul nur die Properties, die seine Bausteine konsumieren koennen (compile-time-gefiltert)
    virtual void publish_options_to_module(std::uint64_t module_id, PlatformPropertySet const& props) = 0;

    [[nodiscard]] virtual PlatformPropertySet const* options_for_module(std::uint64_t module_id) const noexcept = 0;
};

} // namespace comdare::cache_engine::platform
