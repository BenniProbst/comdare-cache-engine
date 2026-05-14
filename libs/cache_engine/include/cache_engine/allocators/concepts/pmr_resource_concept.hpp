#pragma once
// CacheEnginePmrResource - std::pmr::memory_resource-konform (REV 7 §3.1 A2)
//
// Erlaubt Verwendung jedes Comdare-Allokator-Bausteins als
// std::pmr::polymorphic_allocator<T> Backend (runtime polymorph).
// Pflicht-Konformanz pro REV 7 §3 / N3916.

#include "i_allocation_strategy.hpp"

#include <cstddef>
#include <memory_resource>

namespace comdare::cache_engine::allocator {

template <IAllocationStrategy Strategy>
class CacheEnginePmrResource : public std::pmr::memory_resource {
public:
    explicit CacheEnginePmrResource(Strategy* s) noexcept : strategy_{s} {}

    [[nodiscard]] Strategy* strategy() const noexcept { return strategy_; }

protected:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        auto* p = strategy_->raw_allocate(bytes, alignment);
        if (!p) throw std::bad_alloc{};
        return p;
    }

    void do_deallocate(void* p, std::size_t bytes, std::size_t alignment) override {
        strategy_->raw_deallocate(p, bytes, alignment);
    }

    bool do_is_equal(memory_resource const& other) const noexcept override {
        auto const* o = dynamic_cast<CacheEnginePmrResource const*>(&other);
        return o && o->strategy_ == strategy_;
    }

private:
    Strategy* strategy_ = nullptr;
};

}  // namespace comdare::cache_engine::allocator
