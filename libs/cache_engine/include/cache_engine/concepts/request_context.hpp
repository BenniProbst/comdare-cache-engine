#pragma once
// request_context.hpp - REV 5.2 Eingabe-Kontext von ISearchPageStrategy
// Quelle: U09 (UML) - Read-only Snapshot, by-const-ref weitergegeben

#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::cache_engine {

/// OperationKind - welche Such-Engine-Operation laeuft
enum class OperationKind : std::uint8_t {
    Lookup,
    Insert,
    Erase,
    Update,
    RangeScan,
    PrefixScan,
    BulkBuild,
    Compact
};

/// RequestContext - Read-only Snapshot vom Caller an ICacheEngine
/// (forward-declarations halten cache_engine unabhaengig von prt_art-Headern)
struct RequestContext {
    OperationKind operation_kind = OperationKind::Lookup;

    /// rohe Byte-View des Suchschluessels (key_view)
    std::span<const std::byte> key{};

    /// Hint fuer erwartete Value-Groesse (relevant fuer Inline-vs-External-Wahl)
    std::size_t value_size_hint = 0;

    /// Generische Pointer auf Caller-Strukturen (typisiert beim Konsumenten)
    /// In Phase 7 werden diese ueber std::any oder typed-handle aufgeloest.
    const void* search_engine = nullptr;
    const void* search_page = nullptr;
    const void* search_page_structure = nullptr;
    const void* requested_inode = nullptr;
};

}  // namespace comdare::cache_engine
