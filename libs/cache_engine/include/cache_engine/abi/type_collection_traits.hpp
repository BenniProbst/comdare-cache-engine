#pragma once
// type_collection_traits - Variadic-Template-Magie fuer (Key, Value) Ableitung
// (REV 7 §4.2(b) + §4.3)
//
// Fall 1: 1 Typ-Parameter T0
//         → key_t = std::uint64_t (auto-incrementing monotonic counter)
//         → value_t = T0
//         → key_is_implicit = true
// Fall 2: 2 Typ-Parameter (T0, T1)
//         → key_t = T0
//         → value_t = T1
// Fall 3: N>2 Typ-Parameter (T0, T1, ..., TN)
//         → key_t = T0
//         → value_t = std::tuple<T1, ..., TN>

#include <cstdint>
#include <tuple>
#include <type_traits>

namespace comdare {

template <typename... Ts>
struct type_collection_traits;

// Fall 1: 1 Param
template <typename T0>
struct type_collection_traits<T0> {
    using key_t   = std::uint64_t;
    using value_t = T0;
    static constexpr bool key_is_implicit = true;
    static constexpr std::size_t param_count = 1;
};

// Fall 2: 2 Params
template <typename T0, typename T1>
struct type_collection_traits<T0, T1> {
    using key_t   = T0;
    using value_t = T1;
    static constexpr bool key_is_implicit = false;
    static constexpr std::size_t param_count = 2;
};

// Fall 3: N>2 Params -> Value = tuple<T1..TN>
template <typename T0, typename T1, typename T2, typename... TR>
struct type_collection_traits<T0, T1, T2, TR...> {
    using key_t   = T0;
    using value_t = std::tuple<T1, T2, TR...>;
    static constexpr bool key_is_implicit = false;
    static constexpr std::size_t param_count = 3 + sizeof...(TR);
};

}  // namespace comdare
