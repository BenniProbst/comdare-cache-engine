#pragma once
// AP15-2 (2026-07-06) -- IAllocatorProxyTier: ABI-stabiles get_allocator-Proxy-Sub-Interface.
//
// Non-dagger std::map-Vollstaendigkeitsbaustein: get_allocator() gehoert NICHT in das std::map-Oracle
// des Pruef-Docks, weil es keine funktionale Datenoperation ist. Der Host kann die Faehigkeit separat
// via dynamic_cast<IAllocatorProxyTier*> abfragen; alt-gebaute DLLs ohne diese Faehigkeit liefern nullptr
// und degradieren sauber.
//
// Der Host-Allocator kann die ABI-Grenze nicht als Instanz queren: idriveable_tier.hpp:17 haelt die
// Grenze bewusst bei vtable + uint64-Parametern. Dieses Interface liefert deshalb keinen Allocator
// by-value, sondern einen ehrlichen Identitaets- und Statistik-Proxy als flachen uint64-POD.
// Siehe docs/ENTWICKLER-FUNCTION-HANDLE-HOPS.md.

#include <cstdint>
#include <type_traits>

namespace comdare::cache_engine::anatomy {

// -----------------------------------------------------------------------------
// ComdareAllocatorProxyV1 -- flacher, ABI-stabiler POD fuer get_allocator().
// flags: bit0=identity_present, bit1=stats_route_present, bit2=is_original_module.
// allocator_family_id: axis_06-Familie der Komposition, 0 = unknown.
// -----------------------------------------------------------------------------
struct ComdareAllocatorProxyV1 {
    std::uint64_t format_version        = 0;
    std::uint64_t allocator_family_id   = 0;
    std::uint64_t flags                 = 0;
    std::uint64_t total_bytes_allocated = 0;
    std::uint64_t total_bytes_in_use    = 0;
    std::uint64_t allocation_count      = 0;
    std::uint64_t live_nodes            = 0;

    [[nodiscard]] constexpr bool operator==(ComdareAllocatorProxyV1 const&) const noexcept = default;
};

static_assert(std::is_standard_layout_v<ComdareAllocatorProxyV1>,
              "ABI-Pflicht: Allocator-Proxy-POD muss standard_layout sein");
static_assert(std::is_trivially_copyable_v<ComdareAllocatorProxyV1>,
              "ABI-Pflicht: Allocator-Proxy-POD muss memcpy-faehig (trivially_copyable) sein");

/// ABI-Version des Allocator-Proxy-Formats. Major-Bump bei Layout-Aenderung (neue Felder = neuer POD V2).
inline constexpr std::uint64_t kAllocatorProxyFormatVersion = 1;

// -----------------------------------------------------------------------------
// IAllocatorProxyTier -- optionales get_allocator-Proxy-Sub-Interface.
// -----------------------------------------------------------------------------
class IAllocatorProxyTier {
public:
    virtual ~IAllocatorProxyTier() = default;

    /// Liefert Identitaet + schmalen Statistik-Snapshot des getriebenen Allocator-Pfads.
    /// Nicht vorhandene Faehigkeiten bleiben ehrlich 0 und loeschen das jeweilige flags-Bit.
    virtual void tier_get_allocator(ComdareAllocatorProxyV1* out) const noexcept = 0;
};

} // namespace comdare::cache_engine::anatomy
