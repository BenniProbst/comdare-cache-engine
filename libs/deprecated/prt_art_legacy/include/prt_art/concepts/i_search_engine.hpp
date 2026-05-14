#pragma once
// i_search_engine.hpp - REV 5 ISearchEngine : IExecutingEngine

#include "prt_art/concepts/i_executing_engine.hpp"
#include "prt_art/concepts/i_root_node.hpp"

#include <optional>

namespace comdare::prt_art {

/// ISearchEngine - PRT-ART-Diplomarbeit-Fokus
template <typename Key, typename Value>
class ISearchEngine : public IExecutingEngine {
protected:
    IRootNode<Key, Value>* root_node_ = nullptr;

public:
    [[nodiscard]] virtual std::optional<Value> lookup(const Key& key) const = 0;
    [[nodiscard]] virtual bool insert(const Key& key, Value value) = 0;
    [[nodiscard]] virtual bool erase(const Key& key) = 0;
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;

    [[nodiscard]] bool empty() const noexcept { return size() == 0; }

    [[nodiscard]] IRootNode<Key, Value>* root_node() const noexcept {
        return root_node_;
    }
};

}  // namespace comdare::prt_art
