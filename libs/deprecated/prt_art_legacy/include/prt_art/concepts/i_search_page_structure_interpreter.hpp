#pragma once
// i_search_page_structure_interpreter.hpp
// REV 5.1 Singleton-Interpreter pro Layout-Variante (Facade-Pattern)
// Praezedenz: LLVM TargetInfo, Protobuf MessageLite, FAST 3-Level

#include <cstddef>
#include <cstdint>
#include <span>

namespace comdare::prt_art {

class ISearchPageStructure;

/// IteratorMode - 3 Iterator-Semantik-Modi (Termin 2 API-Entscheidung)
enum class IteratorMode : std::uint8_t {
    Default,         ///< keine garantierte Reihenfolge (nur Punkt-Suche)
    LocallyOrdered,  ///< geordnet innerhalb einer Seite
    Lex              ///< global lexikographisch (Prefix Enumeration + Range)
};

/// SlotHandle - opaque Slot-Adresse innerhalb einer ISearchPage
struct SlotHandle {
    std::uint8_t slot_index = 0;
    bool valid = false;
};

/// InterpretResult - Auflösung eines (Structure, key)-Paares
enum class InterpretKind : std::uint8_t {
    NotFound,
    NextSlot,     ///< Weiterleiten an Slot in dieser Page
    NextPage,     ///< Weiterleiten an child ISearchPage
    NextLayer,    ///< Weiterleiten an neuen Layer-Root (Masstree)
    Terminal      ///< Value gefunden
};

struct InterpretResult {
    InterpretKind kind = InterpretKind::NotFound;
    SlotHandle slot{};
    const void* next_target = nullptr;  ///< Typisiert beim Konsumenten
};

/// ISearchPageStructureInterpreter - Singleton-Facade
/// Tree haelt EINEN Verweis pro Layout-Variante (nicht pro INode).
/// Spart 8 B vtable-Pointer pro INode (kritisch bei 16-B-INodes).
class ISearchPageStructureInterpreter {
public:
    virtual ~ISearchPageStructureInterpreter() = default;

    /// Haupt-Such-Methode (Facade)
    [[nodiscard]] virtual InterpretResult interpret(
        const ISearchPageStructure& structure,
        std::span<const std::byte> key) const = 0;

    /// Slot-Aufloesung via permutation_index
    [[nodiscard]] virtual SlotHandle next_slot(
        const ISearchPageStructure& structure,
        std::span<const std::byte> key) const = 0;

    /// SIMD-Verfuegbarkeit der Konkretisierung (Compile-Time)
    [[nodiscard]] virtual bool supports_simd() const noexcept = 0;
};

}  // namespace comdare::prt_art
