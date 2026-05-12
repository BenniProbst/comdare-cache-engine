#pragma once
// search_algorithm_type_collection - Variadic-Type-Collection fuer SearchEngine
// (REV 7 §4.2(b) + §4.3)

#include "type_collection_traits.hpp"
#include "../fingerprint/fixed_length_fingerprint.hpp"

#include <atomic>
#include <cstdint>
#include <type_traits>

namespace comdare {

template <typename... Ts>
class search_algorithm_type_collection {
public:
    using traits        = type_collection_traits<Ts...>;
    using key_t         = typename traits::key_t;
    using value_t       = typename traits::value_t;
    using binary_key_t  = decltype(fingerprint::to_binary_string(std::declval<key_t>()));

    static constexpr bool        key_is_implicit = traits::key_is_implicit;
    static constexpr std::size_t param_count     = traits::param_count;

    // Implicit-Key-Generierung (Fall 1): monotonisch hochzaehlender uint64
    [[nodiscard]] static key_t next_implicit_key() noexcept
        requires (key_is_implicit)
    {
        return implicit_key_counter_.fetch_add(1, std::memory_order_relaxed);
    }

    [[nodiscard]] static binary_key_t to_binary(key_t const& k) noexcept {
        return fingerprint::to_binary_string(k);
    }

private:
    static inline std::atomic<std::uint64_t> implicit_key_counter_{0};
};

}  // namespace comdare
