// V41.F.6.1 F.6 Phase A: Test der migrierten Serialisierungs-Primitive (axis_10)
// @doku 19 §2.2 (signaling_bits prt-art -> cache-engine BASIS)

#include <gtest/gtest.h>

#include <topics/serialization/axis_10_serialization/axis_10_serialization_primitives.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace s10 = ::comdare::cache_engine::serialization::axis_10_serialization;

TEST(F6_SerPrimitives, VarLenRoundTripSmall) {
    std::byte buf[9];
    for (std::uint64_t v : {std::uint64_t{0}, std::uint64_t{1}, std::uint64_t{127}, std::uint64_t{128},
                            std::uint64_t{300}, std::uint64_t{16384}, std::uint64_t{1u << 30}}) {
        std::size_t n = s10::VarLenEncoder::encode(v, buf, sizeof(buf));
        ASSERT_GT(n, 0u);
        auto dec = s10::VarLenEncoder::decode(std::span<std::byte const>(buf, n));
        EXPECT_EQ(dec.value, v);
        EXPECT_EQ(dec.consumed_bytes, n);
    }
}

TEST(F6_SerPrimitives, VarLenRoundTripMax9Byte) {
    // Encoder ist per Design 1..9 Bytes (= max 63 Bit, 9*7). Genuegt fuer Payload-Groessen
    // (size_t < 2^63). Voll-64-Bit braeuchte 10 Bytes — bewusst NICHT unterstuetzt.
    std::byte     buf[9];
    std::uint64_t v = 0x7FFFFFFFFFFFFFFFull; // 2^63 - 1 = groesster 9-Byte-VarInt
    std::size_t   n = s10::VarLenEncoder::encode(v, buf, sizeof(buf));
    ASSERT_EQ(n, 9u);
    auto dec = s10::VarLenEncoder::decode(std::span<std::byte const>(buf, n));
    EXPECT_EQ(dec.value, v);
    EXPECT_EQ(dec.consumed_bytes, 9u);
}

TEST(F6_SerPrimitives, SignalingStreamRoundTrip) {
    s10::SignalingStream     st;
    std::array<std::byte, 3> p1{std::byte{1}, std::byte{2}, std::byte{3}};
    std::array<std::byte, 2> p2{std::byte{9}, std::byte{8}};
    st.append(s10::SignalKind::Normal, std::span<std::byte const>(p1.data(), p1.size()));
    st.append(s10::SignalKind::Special, std::span<std::byte const>(p2.data(), p2.size()));
    EXPECT_EQ(st.entry_count(), 2u);

    auto raw = st.raw();
    auto e1  = s10::SignalingStream::decode_one(raw, 0);
    EXPECT_EQ(e1.signal, s10::SignalKind::Normal);
    ASSERT_EQ(e1.payload.size(), 3u);
    EXPECT_EQ(static_cast<std::uint8_t>(e1.payload[0]), 1u);

    auto e2 = s10::SignalingStream::decode_one(raw, e1.next_offset);
    EXPECT_EQ(e2.signal, s10::SignalKind::Special);
    ASSERT_EQ(e2.payload.size(), 2u);
    EXPECT_EQ(static_cast<std::uint8_t>(e2.payload[0]), 9u);
    EXPECT_EQ(e2.next_offset, raw.size());
}

TEST(F6_SerPrimitives, ClearResets) {
    s10::SignalingStream     st;
    std::array<std::byte, 1> p{std::byte{7}};
    st.append(s10::SignalKind::Normal, std::span<std::byte const>(p.data(), p.size()));
    st.clear();
    EXPECT_EQ(st.entry_count(), 0u);
    EXPECT_EQ(st.byte_size(), 0u);
}
