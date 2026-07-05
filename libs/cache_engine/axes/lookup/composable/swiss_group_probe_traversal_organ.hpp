#pragma once
// AP-7b/#262 -- SwissGroupProbeTraversalOrgan: scalar SwissTable H1/H2 group probe.
//
// Static traversal organ over SwissGroupPoolStore. The lookup/insert/erase loops are a faithful scalar port of
// axis_03a_search_algo_swisstable.hpp (AP-7a): same hash split, 16-byte groups, probe order, 7/8 grow trigger,
// and tombstone reuse. No SIMD in this increment.

#include "swiss_group_pool_concept.hpp"
#include "swiss_group_pool_store.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

template <class T, class Pool>
concept SwissGroupProbeTraversal =
    SwissGroupPool<Pool> && requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
        { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<Pool>(cp, k) } -> std::same_as<std::optional<typename Pool::value_type>>;
        { T::template erase_from<Pool>(p, k) } -> std::same_as<bool>;
    };

struct SwissGroupProbeTraversalOrgan {
    static constexpr std::uint64_t kFibonacciMul = 11400714819323198485ULL;
    static constexpr std::size_t   kNpos         = static_cast<std::size_t>(-1);

    template <class Pool>
    [[nodiscard]] static constexpr std::uint64_t mixed_hash(typename Pool::key_type k) noexcept {
        return static_cast<std::uint64_t>(k) * kFibonacciMul;
    }

    [[nodiscard]] static constexpr std::uint8_t h2_fingerprint(std::uint64_t hash) noexcept {
        return static_cast<std::uint8_t>(hash & 0x7Fu);
    }

    template <class Pool>
    [[nodiscard]] static std::size_t group_start_for(Pool const& p, std::uint64_t hash, std::size_t probe) noexcept {
        std::size_t const group_mask = p.group_count() - 1u;
        std::size_t const h1_group   = static_cast<std::size_t>(hash >> 7u) & group_mask;
        return ((h1_group + probe) & group_mask) * Pool::kGroupWidth;
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        if (p.needs_rehash_for_insert()) p.rehash(p.slot_count() * 2u);
        insert_impl(p, k, v);
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::optional<typename Pool::value_type> result = std::nullopt;
        std::uint64_t const                      hash   = mixed_hash<Pool>(k);
        std::uint8_t const                       h2     = h2_fingerprint(hash);
        std::size_t const                        groups = p.group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(p, hash, probe);
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == h2 && p.slot_key(pos) == k) {
                    result = p.slot_value(pos);
                    goto done;
                }
            }
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                if (p.control_byte(group_start + i) == Pool::kEmpty) goto done;
            }
        }
    done:
        return result;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::uint64_t const hash   = mixed_hash<Pool>(k);
        std::uint8_t const  h2     = h2_fingerprint(hash);
        std::size_t const   groups = p.group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(p, hash, probe);
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == h2 && p.slot_key(pos) == k) {
                    p.mark_deleted(pos);
                    return true;
                }
            }
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                if (p.control_byte(group_start + i) == Pool::kEmpty) return false;
            }
        }
        return false;
    }

private:
    template <class Pool>
    static void insert_impl(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::uint64_t const hash          = mixed_hash<Pool>(k);
        std::uint8_t const  h2            = h2_fingerprint(hash);
        std::size_t const   groups        = p.group_count();
        std::size_t         first_deleted = kNpos;
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(p, hash, probe);
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == h2 && p.slot_key(pos) == k) {
                    p.set_slot_value(pos, v);
                    return;
                }
            }
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == Pool::kDeleted && first_deleted == kNpos) first_deleted = pos;
            }
            for (std::size_t i = 0; i < Pool::kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == Pool::kEmpty) {
                    std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                    p.place_occupied(target, k, v, h2);
                    return;
                }
            }
        }

        p.rehash(p.slot_count() * 2u);
        insert_impl(p, k, v);
    }
};

static_assert(SwissGroupProbeTraversal<SwissGroupProbeTraversalOrgan, SwissGroupPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
