#pragma once
// CacheEngineAllocator - std::allocator-konformer Wrapper (REV 7 §3.1 A1)
//
// Erlaubt Verwendung jedes Comdare-Allokator-Bausteins als
// std::container<T, CacheEngineAllocator<T, Strategy>>.

#include "i_allocation_strategy.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>

namespace comdare::cache_engine::allocator {

template <typename T, IAllocationStrategy Strategy>
class CacheEngineAllocator {
public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;

    CacheEngineAllocator() noexcept = delete;
    explicit CacheEngineAllocator(Strategy* s) noexcept : strategy_{s} {}

    template <typename U>
    CacheEngineAllocator(CacheEngineAllocator<U, Strategy> const& other) noexcept
        : strategy_{other.strategy_} {}

    [[nodiscard]] T* allocate(size_type n) {
        auto* p = strategy_->raw_allocate(n * sizeof(T), alignof(T));
        if (!p) throw std::bad_alloc{};
        return static_cast<T*>(p);
    }

    void deallocate(T* p, size_type n) noexcept {
        strategy_->raw_deallocate(static_cast<void*>(p), n * sizeof(T), alignof(T));
    }

    template <typename U>
    struct rebind { using other = CacheEngineAllocator<U, Strategy>; };

    [[nodiscard]] Strategy* strategy() const noexcept { return strategy_; }

    template <typename U>
    bool operator==(CacheEngineAllocator<U, Strategy> const& other) const noexcept {
        return strategy_ == other.strategy_;
    }
    template <typename U>
    bool operator!=(CacheEngineAllocator<U, Strategy> const& other) const noexcept {
        return !(*this == other);
    }

private:
    template <typename U, IAllocationStrategy S>
    friend class CacheEngineAllocator;

    Strategy* strategy_ = nullptr;
};

}  // namespace comdare::cache_engine::allocator
