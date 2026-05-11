#pragma once
// i_node.hpp - REV 5.1 INode KORRIGIERT als Mikro-Verweis (Slot-Eintrag)
// Quelle: U02 + extract_masstree_inode.md (Masstree Slot-Granularitaet)
//
// FALSCH (REV 5): INode = "Verzweigungs-/Daten-Einheit" (zu grob, Container)
// KORREKT (REV 5.1): INode = Slot-Eintrag in einer Liste eines IFanouts,
//                            Tagged Union mit Discriminator (Masstree keylenx_)

#include "prt_art/concepts/value_handle.hpp"

#include <cstdint>
#include <variant>

namespace comdare::prt_art {

// Forward declarations
template <typename K, typename V> class ISearchPage;
template <typename K, typename V> class IRootNode;

/// NodeRefKind - Discriminator-Werte (entspricht Masstree keylenx_)
enum class NodeRefKind : std::uint8_t {
    ValueRef,    ///< Slot zeigt auf Value (terminal). Masstree: lv_.value
    ChildRef,    ///< Slot zeigt auf naechsten Knoten. Masstree: child_[i+1]
    LayerRef,    ///< Slot zeigt auf neuen Layer-Root. Masstree: lv_.layer (keylenx==128)
    SiblingRef,  ///< Slot zeigt auf B-link-Sibling. Masstree: next_/prev_
    SuffixRef    ///< Slot zeigt auf externen Suffix. Masstree: ksuf_ + offset (keylenx==64)
};

/// SuffixPtr - externer Suffix-Anker
struct SuffixPtr {
    std::uintptr_t ksuf = 0;
    std::uint32_t offset = 0;
};

/// INode<K, V> - Mikro-Verweis = Slot-Eintrag auf einer ISearchPage
///
/// REV 5.1: HAT KEINE eigenstaendige Speicher-Adresse!
/// Adressiert via permutation_index durch ISearchPageStructureInterpreter (Facade).
template <typename Key, typename Value>
class INode {
public:
    using ChildRefPtr   = ISearchPage<Key, Value>*;
    using LayerRefPtr   = IRootNode<Key, Value>*;
    using SiblingRefPtr = ISearchPage<Key, Value>*;
    using ValueRefHandle = ValueHandle<Value>;

    /// Tagged-Union ueber 5 Slot-Varianten
    using Payload = std::variant<
        ValueRefHandle,
        ChildRefPtr,
        LayerRefPtr,
        SiblingRefPtr,
        SuffixPtr>;

protected:
    ISearchPage<Key, Value>* placement_page_ = nullptr;
    std::uint8_t slot_index_ = 0;          ///< logischer Index ueber permutation_
    NodeRefKind ref_kind_ = NodeRefKind::ValueRef;
    Payload payload_;

public:
    constexpr INode() = default;

    [[nodiscard]] NodeRefKind kind() const noexcept { return ref_kind_; }
    [[nodiscard]] std::uint8_t slot_index() const noexcept { return slot_index_; }

    [[nodiscard]] ISearchPage<Key, Value>* placement_page() const noexcept {
        return placement_page_;
    }

    [[nodiscard]] const Payload& payload() const noexcept { return payload_; }
};

}  // namespace comdare::prt_art
