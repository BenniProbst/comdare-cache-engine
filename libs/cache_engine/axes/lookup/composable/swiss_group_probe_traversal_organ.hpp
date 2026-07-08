#pragma once
// AP-7b/#262 -- SwissGroupProbeTraversalOrgan: scalar SwissTable H1/H2 group probe.
//
// Static traversal organ over SwissGroupPoolStore. The lookup/insert/erase loops are a faithful scalar port of
// axis_03a_search_algo_swisstable.hpp (AP-7a): same hash split, 16-byte groups, probe order, 7/8 grow trigger,
// and tombstone reuse. No SIMD in this increment.

#include "swiss_group_pool_concept.hpp"
#include "swiss_group_pool_store.hpp"

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

struct ScalarGroupMatch {
    [[nodiscard]] static constexpr std::uint16_t group_match_mask(std::uint8_t const* ctrl16,
                                                                  std::uint8_t        needle) noexcept {
        std::uint16_t mask = 0;
        for (std::size_t i = 0; i < 16u; ++i) {
            if (ctrl16[i] == needle) mask = static_cast<std::uint16_t>(mask | (std::uint16_t{1} << i));
        }
        return mask;
    }
};

template <class I>
concept SwissGroupMatcher = requires(std::uint8_t const* ctrl16, std::uint8_t needle) {
    { I::group_match_mask(ctrl16, needle) } noexcept -> std::same_as<std::uint16_t>;
};

template <class T, class Pool, class I = ScalarGroupMatch>
concept SwissGroupProbeTraversal =
    SwissGroupPool<Pool> && SwissGroupMatcher<I> &&
    requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
        { T::template insert_into<Pool, I>(p, k, v) } -> std::same_as<void>;
        { T::template lookup_in<Pool, I>(cp, k) } -> std::same_as<std::optional<typename Pool::value_type>>;
        { T::template erase_from<Pool, I>(p, k) } -> std::same_as<bool>;
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

    template <class Pool, class I = ScalarGroupMatch>
    static void insert_into(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        if (p.needs_rehash_for_insert()) p.rehash(p.slot_count() * 2u);
        insert_impl<Pool, I>(p, k, v);
    }

    template <class Pool, class I = ScalarGroupMatch>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type k) {
        std::optional<typename Pool::value_type> result = std::nullopt;
        std::uint64_t const                      hash   = mixed_hash<Pool>(k);
        std::uint8_t const                       h2     = h2_fingerprint(hash);
        std::size_t const                        groups = p.group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(p, hash, probe);
            std::uint16_t     h2_mask     = I::group_match_mask(p.control_group_ptr(group_start), h2);
            while (h2_mask != 0u) {
                std::size_t const i   = lowest_bit_index(h2_mask);
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == h2 && p.slot_key(pos) == k) {
                    result = p.slot_value(pos);
                    goto done;
                }
                clear_lowest_bit(h2_mask);
            }
            std::uint16_t const empty_mask = I::group_match_mask(p.control_group_ptr(group_start), Pool::kEmpty);
            if (empty_mask != 0u) goto done;
        }
    done:
        return result;
    }

    template <class Pool, class I = ScalarGroupMatch>
    static bool erase_from(Pool& p, typename Pool::key_type k) {
        std::uint64_t const hash   = mixed_hash<Pool>(k);
        std::uint8_t const  h2     = h2_fingerprint(hash);
        std::size_t const   groups = p.group_count();
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(p, hash, probe);
            std::uint16_t     h2_mask     = I::group_match_mask(p.control_group_ptr(group_start), h2);
            while (h2_mask != 0u) {
                std::size_t const i   = lowest_bit_index(h2_mask);
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == h2 && p.slot_key(pos) == k) {
                    p.mark_deleted(pos);
                    return true;
                }
                clear_lowest_bit(h2_mask);
            }
            std::uint16_t const empty_mask = I::group_match_mask(p.control_group_ptr(group_start), Pool::kEmpty);
            if (empty_mask != 0u) return false;
        }
        return false;
    }

private:
    [[nodiscard]] static std::size_t lowest_bit_index(std::uint16_t mask) noexcept {
        return static_cast<std::size_t>(std::countr_zero(static_cast<unsigned>(mask)));
    }

    static void clear_lowest_bit(std::uint16_t& mask) noexcept {
        mask = static_cast<std::uint16_t>(mask & static_cast<std::uint16_t>(mask - 1u));
    }

    template <class Pool, class I = ScalarGroupMatch>
    static void insert_impl(Pool& p, typename Pool::key_type k, typename Pool::value_type v) {
        std::uint64_t const hash          = mixed_hash<Pool>(k);
        std::uint8_t const  h2            = h2_fingerprint(hash);
        std::size_t const   groups        = p.group_count();
        std::size_t         first_deleted = kNpos;
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(p, hash, probe);
            std::uint16_t     h2_mask     = I::group_match_mask(p.control_group_ptr(group_start), h2);
            while (h2_mask != 0u) {
                std::size_t const i   = lowest_bit_index(h2_mask);
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == h2 && p.slot_key(pos) == k) {
                    p.set_slot_value(pos, v);
                    return;
                }
                clear_lowest_bit(h2_mask);
            }
            std::uint16_t deleted_mask = I::group_match_mask(p.control_group_ptr(group_start), Pool::kDeleted);
            while (deleted_mask != 0u) {
                std::size_t const i   = lowest_bit_index(deleted_mask);
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == Pool::kDeleted && first_deleted == kNpos) first_deleted = pos;
                clear_lowest_bit(deleted_mask);
            }
            std::uint16_t empty_mask = I::group_match_mask(p.control_group_ptr(group_start), Pool::kEmpty);
            while (empty_mask != 0u) {
                std::size_t const i   = lowest_bit_index(empty_mask);
                std::size_t const pos = group_start + i;
                if (p.control_byte(pos) == Pool::kEmpty) {
                    std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                    p.place_occupied(target, k, v, h2);
                    return;
                }
                clear_lowest_bit(empty_mask);
            }
        }

        p.rehash(p.slot_count() * 2u);
        insert_impl<Pool, I>(p, k, v);
    }
};

static_assert(SwissGroupProbeTraversal<SwissGroupProbeTraversalOrgan, SwissGroupPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
