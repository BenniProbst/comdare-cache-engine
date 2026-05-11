#pragma once
// CacheHierarchyManager — F4 Plattform-Adapter (REV 3 K3.2 generisch)
// Termin 7 / 02_uml_cache_engine §6 (umgebaut zu Auto-Discovery, ohne X3D/Hybrid-Spez.)

#include <cache_engine/platform/cache_topology.hpp>
#include <cache_engine/platform/i_platform_probe.hpp>

#include <cstdint>
#include <map>

namespace comdare::cache_engine {

enum class DataTemperature : std::uint8_t {
    Cold   = 0,
    Warm   = 1,
    Hot    = 2,
    Ultra  = 3,   // wenige, sehr haeufig zugegriffene Daten (Hot-Path-Layout)
};

enum class AllocatorTier : std::uint8_t {
    StandardDimm   = 0,
    HbmTier        = 1,    // generisch fuer HBM/HBM2/HBM3
    LargestL3Tier  = 2,    // generisch fuer V-Cache-CCDs (REV 3 K3.2)
    PinnedHighIpc  = 3,    // generisch fuer P-Cores
    Persistent     = 4,    // NVRAM/Optane-aehnlich
};

// Generischer F4-Manager — agiert auf PlatformPropertySet, NICHT auf konkreten CPUs
class ICacheHierarchyManager {
public:
    virtual ~ICacheHierarchyManager() = default;

    // Initialisierung aus Auto-Discovery-Ergebnissen
    virtual void configure(platform::PlatformPropertySet const& props) = 0;

    // Allocation pro Daten-Temperatur
    virtual void* allocate_for(DataTemperature temp, std::size_t size) = 0;
    virtual void  deallocate(void* ptr, std::size_t size) noexcept = 0;

    // Tier-Migration (z.B. Hot → Ultra → LargestL3Tier)
    virtual void migrate_tier(void* ptr, AllocatorTier from, AllocatorTier to) noexcept = 0;

    // Routing-Tabelle: welche Temperatur landet in welchem Tier?
    [[nodiscard]] virtual std::map<DataTemperature, AllocatorTier> tier_routing() const = 0;
};

}  // namespace comdare::cache_engine
