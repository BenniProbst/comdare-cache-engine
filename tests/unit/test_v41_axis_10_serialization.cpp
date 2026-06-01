// V41.F.6.1.R7.5.c axis_10_serialization Tests (Goldstandard-konform)
//
// @memory [[axis-gold-standard-checklist]]
// @task #720 V41.F.6.1.R7.5.c (Optional-Topics: serialization)

#include <gtest/gtest.h>

#include <topics/serialization/axis_10_serialization/axis_10_serialization_raw_binary.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_var_len.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_succinct.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_compressed.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_registry.hpp>
#include <topics/serialization/axis_10_serialization/axis_10_serialization_subaxes_sr1_to_sr3.hpp>
#include <axes/serialization_axis/axis_10_serialization_flags.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>

#include <boost/mp11.hpp>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ax10 = ::comdare::cache_engine::serialization::axis_10_serialization;
namespace ser  = ::comdare::cache_engine::serialization;
namespace mp   = ::boost::mp11;

TEST(R7_5_c_Axis10, AllSerializersSatisfyConcepts) {
    static_assert(ax10::concepts::SerializationStrategy<ax10::RawBinarySerialization>);
    static_assert(ax10::concepts::SerializationStrategy<ax10::VarLenSerialization>);
    static_assert(ax10::concepts::SerializationStrategy<ax10::SuccinctSerialization>);
    static_assert(ax10::concepts::SerializationStrategy<ax10::CompressedSerialization>);
    static_assert(ax10::concepts::CacheEnginePermutationStrategy<ax10::RawBinarySerialization>);
    static_assert(ax10::concepts::CacheEnginePermutationStrategy<ax10::VarLenSerialization>);
    static_assert(ax10::concepts::CacheEnginePermutationStrategy<ax10::SuccinctSerialization>);
    static_assert(ax10::concepts::CacheEnginePermutationStrategy<ax10::CompressedSerialization>);
    SUCCEED();
}

TEST(R7_5_c_Axis10, SupportsCompressionDifferentiated) {
    static_assert(ax10::RawBinarySerialization::supports_compression()  == false);
    static_assert(ax10::VarLenSerialization::supports_compression()     == true);
    static_assert(ax10::SuccinctSerialization::supports_compression()   == true);
    static_assert(ax10::CompressedSerialization::supports_compression() == true);
    SUCCEED();
}

TEST(R7_5_c_Axis10, FlagSuffixUppercase) {
    static_assert(ax10::RawBinarySerialization::flag_suffix()  == std::string_view{"RAW_BINARY"});
    static_assert(ax10::VarLenSerialization::flag_suffix()     == std::string_view{"VAR_LEN"});
    static_assert(ax10::SuccinctSerialization::flag_suffix()   == std::string_view{"SUCCINCT"});
    static_assert(ax10::CompressedSerialization::flag_suffix() == std::string_view{"COMPRESSED"});
    SUCCEED();
}

TEST(R7_5_c_Axis10, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax10::RawBinarySerialization::axis_tag,  ax10::subaxes::byte_order_tag>);
    static_assert(std::is_same_v<ax10::VarLenSerialization::axis_tag,     ax10::subaxes::byte_order_tag>);
    static_assert(std::is_same_v<ax10::SuccinctSerialization::axis_tag,   ax10::subaxes::density_tag>);
    static_assert(std::is_same_v<ax10::CompressedSerialization::axis_tag, ax10::subaxes::compression_tag>);
    SUCCEED();
}

TEST(R7_5_c_Axis10, RegistryHas4Serializers) {
    static_assert(mp::mp_size<ax10::AllSerializers>::value == 4);
    static_assert(mp::mp_size<ax10::EnabledSerializers>::value > 0);
    SUCCEED();
}

TEST(R7_5_c_Axis10, FamilyIdsDistinct) {
    static_assert(ax10::RawBinarySerialization::family_id::value  == 1);
    static_assert(ax10::VarLenSerialization::family_id::value     == 2);
    static_assert(ax10::SuccinctSerialization::family_id::value   == 3);
    static_assert(ax10::CompressedSerialization::family_id::value == 4);
    SUCCEED();
}

TEST(R7_5_c_Axis10, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(ax10::flags::raw_binary_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax10::flags::var_len_enabled),    const bool>);
    static_assert(std::is_same_v<decltype(ax10::flags::succinct_enabled),   const bool>);
    static_assert(std::is_same_v<decltype(ax10::flags::compressed_enabled), const bool>);
    SUCCEED();
}

TEST(R7_5_c_Serialization, TopicConfigSetExposesAxis10) {
    static_assert(mp::mp_size<ser::TopicConfigSet::StaticAxisVariants_10>::value > 0);
    static_assert(std::is_same_v<ser::TopicConfigSet::StaticAxisVariants,
                                  ser::TopicConfigSet::StaticAxisVariants_10>);
    SUCCEED();
}

// V41 R5.B (Doku 22 §3.2/§4) — serialization-Achse RUNTIME-OPERATIV: behaviorale `serialize_scan`-Laufzeit-API.
// Vor R5.B war axis_10 trait-only (nur constexpr supports_compression()/name() …) → eine Variation in
// run_workload waere HOHL gewesen. Dieser Test belegt: (1) Baseline-Korrektheit (raw = Feld-Summe),
// (2) Determinismus, (3) paarweise DISTINKTE Ergebnisse der 4 Strategien auf demselben Input = echter
// Verhaltensunterschied. Damit ist die Achse als 4. Mess-Dimension (abi_adapter Segment 4) variierbar.
TEST(R5B_Axis10_SerializeScan, RuntimeOperativeBehaviorallyDistinct) {
    constexpr std::size_t kN = 512, kRec = 64;
    std::vector<unsigned char> buf(kN * kRec, 0u);
    std::uint64_t field_sum = 0;
    for (std::size_t i = 0; i < kN; ++i) {
        std::uint32_t const v = static_cast<std::uint32_t>(i * 2654435761u + 12345u);
        std::memcpy(buf.data() + i * kRec, &v, sizeof(v));   // 32-Bit-Feld je Datensatz
        field_sum += v;
    }
    unsigned char const* p = buf.data();
    std::uint64_t const raw  = ax10::RawBinarySerialization::serialize_scan(p, kN, kRec);
    std::uint64_t const comp = ax10::CompressedSerialization::serialize_scan(p, kN, kRec);
    std::uint64_t const vlen = ax10::VarLenSerialization::serialize_scan(p, kN, kRec);
    std::uint64_t const succ = ax10::SuccinctSerialization::serialize_scan(p, kN, kRec);

    // (1) Baseline-Korrektheit: RawBinary = exakte Summe der 32-Bit-Felder.
    EXPECT_EQ(raw, field_sum);
    // (2) Determinismus: identischer Re-Lauf → identisches Ergebnis (reine Funktion des Puffers).
    EXPECT_EQ(comp, ax10::CompressedSerialization::serialize_scan(p, kN, kRec));
    EXPECT_EQ(vlen, ax10::VarLenSerialization::serialize_scan(p, kN, kRec));
    EXPECT_EQ(succ, ax10::SuccinctSerialization::serialize_scan(p, kN, kRec));
    // (3) Paarweise DISTINKT → echter Verhaltensunterschied der 4 Strategien (Achse nicht mehr trait-only).
    EXPECT_NE(raw, comp);  EXPECT_NE(raw, vlen);  EXPECT_NE(raw, succ);
    EXPECT_NE(comp, vlen); EXPECT_NE(comp, succ); EXPECT_NE(vlen, succ);
}
