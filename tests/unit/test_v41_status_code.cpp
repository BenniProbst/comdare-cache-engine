// V41.F.6.1 F.6 Phase A: Test der migrierten errno-style Status-Codes (status_code.hpp)
//
// @memory [[hybrid-search-engine-interface]] (IO returns int errno-style)
// @doku 19 §2.2 (status.hpp prt-art -> cache-engine BASIS)

#include <gtest/gtest.h>

#include <cache_engine/concepts/status_code.hpp>

#include <type_traits>

namespace ce = ::comdare::cache_engine;

TEST(F6_StatusCode, StatusTIsInt) {
    static_assert(std::is_same_v<ce::status_t, int>);
    SUCCEED();
}

TEST(F6_StatusCode, OkIsZeroErrorsArePositive) {
    static_assert(ce::status_ok == 0);
    static_assert(ce::status_key_already_exists == 1);
    static_assert(ce::status_key_not_found == 2);
    static_assert(ce::status_out_of_memory == 3);
    static_assert(ce::status_invalid_argument == 4);
    static_assert(ce::status_capacity_exceeded == 5);
    static_assert(ce::status_locked == 6);
    static_assert(ce::status_out_of_range == 7);
    static_assert(ce::status_empty_container == 8);
    static_assert(ce::status_concurrent_modification == 9);
    static_assert(ce::status_io_error == 10);
    SUCCEED();
}

TEST(F6_StatusCode, Predicates) {
    static_assert(ce::status_is_ok(ce::status_ok));
    static_assert(!ce::status_is_ok(ce::status_io_error));
    static_assert(ce::status_is_error(ce::status_key_not_found));
    static_assert(!ce::status_is_error(ce::status_ok));
    SUCCEED();
}

TEST(F6_StatusCode, ConstexprUsableAtCompileTime) {
    constexpr ce::status_t s = ce::status_ok;
    static_assert(ce::status_is_ok(s));
    SUCCEED();
}
