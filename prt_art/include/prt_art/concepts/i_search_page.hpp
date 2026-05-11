#pragma once
// i_search_page.hpp - REV 5.1 ISearchPage = Knoten-Objekt (logische Such-Seite)
// Quelle: U02 + Masstree leaf (256 B = 4 Cache-Lines, W=15)

#include "prt_art/concepts/i_node.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace comdare::prt_art {

class ISearchPageStructure;
class ISearchPageStructureInterpreter;
template <typename K, typename V> class ICachePage;

/// Default-Fanout-Width (Masstree W=15; ART Node16 W=16)
inline constexpr std::uint8_t kDefaultFanoutWidth = 15;

/// ISearchPage<K, V> - Knoten-Objekt
/// Fanout-Width ist Member-Konstante (default 15), Konkretisierungen koennen
/// die Konstante override-en (z.B. ART Node16, HOT k-constrained Variabel).
template <typename Key, typename Value>
class ISearchPage {
public:
    using NodeT = INode<Key, Value>;

protected:
    std::array<NodeT, kDefaultFanoutWidth> nodes_{};
    std::uint8_t node_count_ = 0;                   ///< nkeys_ in Masstree
    ISearchPageStructure* structure_ = nullptr;     ///< Strategy + Facade

public:
    virtual ~ISearchPage() = default;

    [[nodiscard]] std::uint8_t node_count() const noexcept { return node_count_; }

    [[nodiscard]] virtual std::uint8_t fanout_width() const noexcept {
        return kDefaultFanoutWidth;
    }

    [[nodiscard]] ISearchPageStructure* structure() const noexcept { return structure_; }

    [[nodiscard]] bool contains_node(std::uint8_t slot) const noexcept {
        return slot < node_count_;
    }
};

}  // namespace comdare::prt_art
