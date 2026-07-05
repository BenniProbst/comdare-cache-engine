#pragma once
// AP-7b/#262 -- SwissGroupPool concept for the SwissTable Weg-B organ.
//
// Faithful substrate split for SwissTableSearchAlgo: the pool owns slots_/ctrl_/mask_/size_/tombstones_,
// rehashes, and exposes primitive slot/control-byte operations. H1/H2 probe navigation lives in
// SwissGroupProbeTraversalOrgan.

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace comdare::cache_engine::lookup::composable {

template <class S>
concept SwissGroupPool =
    requires {
        typename S::key_type;
        typename S::value_type;
        { S::kFibonacciMul } -> std::convertible_to<std::uint64_t>;
        { S::kGroupWidth } -> std::convertible_to<std::size_t>;
        { S::kInitialCapacity } -> std::convertible_to<std::size_t>;
        { S::kEmpty } -> std::convertible_to<std::uint8_t>;
        { S::kDeleted } -> std::convertible_to<std::uint8_t>;
    } && std::same_as<typename S::key_type, std::uint64_t> && std::same_as<typename S::value_type, std::uint64_t> &&
    (S::kGroupWidth == 16u) && (S::kInitialCapacity >= S::kGroupWidth) &&
    requires(S& s, S const& cs, std::size_t i, std::size_t n, typename S::key_type k, typename S::value_type v,
             std::uint8_t h2) {
        { cs.slot_count() } -> std::convertible_to<std::size_t>;
        { cs.group_count() } -> std::convertible_to<std::size_t>;
        { cs.occupied() } -> std::convertible_to<std::size_t>;
        { cs.tombstones() } -> std::convertible_to<std::size_t>;
        { cs.control_byte(i) } -> std::same_as<std::uint8_t>;
        { cs.slot_is_empty(i) } -> std::same_as<bool>;
        { cs.slot_is_deleted(i) } -> std::same_as<bool>;
        { cs.slot_is_occupied(i) } -> std::same_as<bool>;
        { cs.slot_key(i) } -> std::same_as<typename S::key_type>;
        { cs.slot_value(i) } -> std::same_as<typename S::value_type>;
        { cs.needs_rehash_for_insert() } -> std::same_as<bool>;
        { s.place_occupied(i, k, v, h2) } -> std::same_as<void>;
        { s.set_slot_value(i, v) } -> std::same_as<void>;
        { s.mark_deleted(i) } -> std::same_as<void>;
        { s.rehash(n) } -> std::same_as<void>;
        { s.clear() } -> std::same_as<void>;
    };

} // namespace comdare::cache_engine::lookup::composable
