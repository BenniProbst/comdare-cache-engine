// test_value_handle.cpp - ValueHandle Inline / External / ChainRef Round-Trip

#include "prt_art/concepts/value_handle.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

namespace pa = comdare::prt_art;

TEST(ValueHandle, InlineHoldsValue) {
    pa::ValueHandle<int> handle = pa::InlineValue<int>{42};
    ASSERT_TRUE(std::holds_alternative<pa::InlineValue<int>>(handle));
    EXPECT_EQ(std::get<pa::InlineValue<int>>(handle).value, 42);
}

TEST(ValueHandle, ExternalHoldsPointer) {
    int                  payload = 99;
    pa::ValueHandle<int> handle  = pa::ExternalValue<int>{&payload, sizeof(payload)};
    ASSERT_TRUE(std::holds_alternative<pa::ExternalValue<int>>(handle));
    EXPECT_EQ(std::get<pa::ExternalValue<int>>(handle).ptr, &payload);
}

TEST(ValueHandle, ChainRefHoldsChainHead) {
    pa::ValueHandle<int> handle = pa::ChainRef<int>{0xDEADBEEF, 4};
    ASSERT_TRUE(std::holds_alternative<pa::ChainRef<int>>(handle));
    EXPECT_EQ(std::get<pa::ChainRef<int>>(handle).chain_head, 0xDEADBEEFu);
    EXPECT_EQ(std::get<pa::ChainRef<int>>(handle).chain_count, 4u);
}
