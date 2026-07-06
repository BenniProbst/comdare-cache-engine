#pragma once
// AP-7b/#262 -- ComposedSwissSearch<Traversal, Pool>: SwissTable group-probe organ + SwissGroupPool.
//
// Narrow shell like ComposedHashSearch, but separated because SwissGroupPool is ctrl-byte/group based rather
// than HashBucketPool open addressing. Provides the common std::map-like organ interface plus DEG-1 harvest.

#include "swiss_group_pool_concept.hpp"
#include "swiss_group_probe_traversal_organ.hpp"

#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

template <class Traversal, class Pool>
    requires SwissGroupProbeTraversal<Traversal, Pool>
class ComposedSwissSearch {
public:
    using key_type   = typename Pool::key_type;
    using value_type = typename Pool::value_type;

    void insert(key_type k, value_type v) { Traversal::template insert_into<Pool>(pool_, k, v); }
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        return Traversal::template lookup_in<Pool>(pool_, k);
    }
    bool                      erase(key_type k) { return Traversal::template erase_from<Pool>(pool_, k); }
    [[nodiscard]] std::size_t occupied_count() const noexcept { return pool_.occupied(); }

    template <class Sink>
    std::size_t for_each_record(Sink&& sink) const {
        std::size_t visited = 0;
        for (std::size_t i = 0; i < pool_.slot_count(); ++i) {
            if (!pool_.slot_is_occupied(i)) continue;
            sink(pool_.slot_key(i), pool_.slot_value(i));
            ++visited;
        }
        return visited;
    }

    void                      clear() noexcept { pool_.clear(); }
    [[nodiscard]] Pool const& pool() const noexcept { return pool_; }

    template <class P = Pool>
    [[nodiscard]] auto store_allocator_statistics() const noexcept
        requires requires(P const& p) { p.store_allocator_statistics(); }
    {
        return pool_.store_allocator_statistics();
    }

private:
    Pool pool_{};
};

} // namespace comdare::cache_engine::lookup::composable
