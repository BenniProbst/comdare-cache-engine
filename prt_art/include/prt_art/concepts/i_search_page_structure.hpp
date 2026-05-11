#pragma once
// i_search_page_structure.hpp - REV 5.1 Strategy + Facade-Singleton

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::prt_art {

class ISearchPageStructureInterpreter;

/// Encoding - 6 PRT-ART-Seitentyp-Kodierungen (Termin 4 Scope-Freeze)
enum class Encoding : std::uint8_t {
    Redirect,        ///< CoCo-trie - komprimierter Reststring
    DenseByte,       ///< ART - 1-Byte direkt-adressiert (Node4/16/48/256)
    MultilevelDense, ///< START - mehrbyteig + Cost-Modell
    SparsePatricia,  ///< HOT - k-constrained, diskriminierende Bits
    DecisionSpan,    ///< B²-Tree - Decision + Span Sub-Trees pro 64 KiB
    CustomAligned    ///< PRT-ART intern - cache-line-aligned, pool-relative
};

/// LayoutInvariantKind - Pflicht-Eigenschaften pro Encoding
enum class LayoutInvariantKind : std::uint16_t {
    Sorted          = 1 << 0,
    UniquePrefixes  = 1 << 1,
    CacheLineAlign  = 1 << 2,
    NodeAligned     = 1 << 3,
    Invariant_I3    = 1 << 4,   ///< Redirect-Knoten == eindeutige Restpfade
    Invariant_I4    = 1 << 5    ///< Variable Codierung erlaubt - Interpreter PFLICHT
};

struct LayoutInvariantSet {
    std::uint16_t flags = 0;

    [[nodiscard]] bool has(LayoutInvariantKind kind) const noexcept {
        return (flags & static_cast<std::uint16_t>(kind)) != 0;
    }

    void set(LayoutInvariantKind kind) noexcept {
        flags |= static_cast<std::uint16_t>(kind);
    }
};

/// ISearchPageStructure - Strategy + Singleton-Facade
class ISearchPageStructure {
protected:
    Encoding encoding_ = Encoding::DenseByte;
    LayoutInvariantSet layout_invariants_{};
    std::span<std::byte> raw_bytes_{};
    const ISearchPageStructureInterpreter* interpreter_ = nullptr;

public:
    virtual ~ISearchPageStructure() = default;

    [[nodiscard]] Encoding encoding() const noexcept { return encoding_; }
    [[nodiscard]] std::span<const std::byte> raw_bytes() const noexcept { return raw_bytes_; }
    [[nodiscard]] const LayoutInvariantSet& layout_invariants() const noexcept {
        return layout_invariants_;
    }
    [[nodiscard]] const ISearchPageStructureInterpreter* interpreter() const noexcept {
        return interpreter_;
    }

    [[nodiscard]] virtual bool check_invariants() const = 0;
    [[nodiscard]] virtual std::string_view encoding_name() const noexcept = 0;
};

}  // namespace comdare::prt_art
