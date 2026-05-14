#pragma once
// i_fanout.hpp - REV 5.1 IFanout HAT ISearchPages (NICHT INodes direkt!)
// Quelle: U02 + extract_masstree_inode.md
//
// FRUEHER: composes [N] INode via contains (FALSCH!)
// JETZT: contains [N] ISearchPage; INodes sitzen AUF Pages

#include <cstddef>
#include <cstdint>
#include <vector>

namespace comdare::prt_art {

template <typename K, typename V> class ISearchPage;
class ISearchPagesStrategy;
class ISearchPagesStrategyPattern;

/// IFanout<K, V> - Verzweigungs-Abstraktion (Konzept)
template <typename Key, typename Value>
class IFanout {
protected:
    std::vector<ISearchPage<Key, Value>*> pages_;
    ISearchPagesStrategy* pages_strategy_ = nullptr;
    ISearchPagesStrategyPattern* pages_strategy_pattern_ = nullptr;

public:
    virtual ~IFanout() = default;

    [[nodiscard]] std::size_t pages_count() const noexcept { return pages_.size(); }

    void insert_page(ISearchPage<Key, Value>* page) {
        if (page != nullptr) {
            pages_.push_back(page);
        }
    }

    void remove_page(ISearchPage<Key, Value>* page) noexcept {
        if (page == nullptr) return;
        auto it = std::find(pages_.begin(), pages_.end(), page);
        if (it != pages_.end()) {
            pages_.erase(it);
        }
    }

    [[nodiscard]] ISearchPage<Key, Value>* lookup_page(std::uint8_t byte) const noexcept {
        // Default: linear scan. Konkretisierungen ueberschreiben mit besseren Strukturen.
        // (Implementierung wird in den 6 ISearchPagesStrategy-Konkretisierungen ergaenzt.)
        (void)byte;
        return pages_.empty() ? nullptr : pages_.front();
    }

    [[nodiscard]] ISearchPagesStrategy* pages_strategy() const noexcept {
        return pages_strategy_;
    }

    [[nodiscard]] ISearchPagesStrategyPattern* pages_strategy_pattern() const noexcept {
        return pages_strategy_pattern_;
    }
};

}  // namespace comdare::prt_art
