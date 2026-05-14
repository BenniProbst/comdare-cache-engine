#pragma once
// i_root_node.hpp - REV 5.1 IRootNode (KONZEPTIONELLE Wurzel auf SearchPage)
// Vorher faelschlich IRootPage - korrigiert auf IRootNode laut User-Review.

#include <cstdint>

namespace comdare::prt_art {

template <typename K, typename V> class IFanout;
template <typename K, typename V> class ISearchPage;

/// Algorithmus-Signatur (P01 ART / P02 HOT / P03 Masstree / etc.)
struct AlgorithmSignature {
    std::uint32_t algorithm_id = 0;
    std::uint8_t version_major = 0;
    std::uint8_t version_minor = 0;
};

/// IRootNode<K, V> - KONZEPTIONELLE Wurzel des Such-Algorithmus
///
/// REV 5.1: liegt AUF einer ISearchPage darunter (placement)
/// composes 1 IFanout (KEIN direkter Speicher!)
template <typename Key, typename Value>
class IRootNode {
protected:
    IFanout<Key, Value>* fanout_ = nullptr;
    ISearchPage<Key, Value>* placement_page_ = nullptr;

public:
    virtual ~IRootNode() = default;

    [[nodiscard]] IFanout<Key, Value>* root_fanout() const noexcept { return fanout_; }

    void set_root_fanout(IFanout<Key, Value>* fan) noexcept { fanout_ = fan; }

    [[nodiscard]] virtual AlgorithmSignature algorithm_signature() const = 0;

    [[nodiscard]] virtual bool supports_concurrent_root_swap() const noexcept = 0;

    [[nodiscard]] ISearchPage<Key, Value>* placement_page() const noexcept {
        return placement_page_;
    }
};

}  // namespace comdare::prt_art
