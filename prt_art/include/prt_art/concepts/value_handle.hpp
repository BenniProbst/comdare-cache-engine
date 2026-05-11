#pragma once
// value_handle.hpp - REV 5.1 ValueHandle Pflicht-Datenpfad (Termin 2 H3)
// Inline / External / ChainRef - 3 Auspraegungen (Compile-Time + Runtime parametrisierbar)

#include <cstddef>
#include <cstdint>
#include <variant>

namespace comdare::prt_art {

/// InlineValue - kleiner Wert direkt im terminalen Knoten (Cache-Line-Hot-Pfad)
template <typename Value>
struct InlineValue {
    Value value;
};

/// ExternalValue - Pointer auf externen Payload-Speicher (grosser Wert, Cold-Storage)
template <typename Value>
struct ExternalValue {
    Value* ptr = nullptr;
    std::size_t size_bytes = 0;
};

/// ChainRef - verkettete Referenz fuer Multi-Value (vertagt P3, Termin 4 Scope-Freeze)
template <typename Value>
struct ChainRef {
    std::uintptr_t chain_head = 0;
    std::uint32_t chain_count = 0;
};

/// ValueHandle - Tagged-Union ueber drei Auspraegungen
template <typename Value>
using ValueHandle = std::variant<
    InlineValue<Value>,
    ExternalValue<Value>,
    ChainRef<Value>>;

}  // namespace comdare::prt_art
