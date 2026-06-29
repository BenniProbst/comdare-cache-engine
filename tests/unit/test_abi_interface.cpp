// Tests fuer ABI-stabiles C++23-Modul-Interface (Phase 6.3)
// type_collection_traits + search_algorithm_type_collection + fingerprint +
// processing_strategy + configuration_permutation + resolve_baustein + module_abi_v1
// (Hinweis: die Legacy-ABI-Klassen execution_engine/search_engine wurden mit der
//  I1-Vereinheitlichung 2026-06-25 entfernt — Doc 36 §2.5/§4; ABI-Sicht = SearchAlgorithmAbiAdapter.)

#include <cache_engine/abi/type_collection_traits.hpp>
#include <cache_engine/abi/search_algorithm_type_collection.hpp>
#include <cache_engine/abi/processing_strategy.hpp>
#include <cache_engine/abi/configuration_permutation.hpp>
#include <cache_engine/abi/resolve_baustein.hpp>
#include <cache_engine/abi/module_abi_v1.hpp>
#include <cache_engine/fingerprint/fixed_length_fingerprint.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <tuple>
#include <type_traits>

// ─────────────────────────────────────────────────────────────────────────────
// type_collection_traits - Variadic-Template-Magie (REV 7 §4.2(b))
// ─────────────────────────────────────────────────────────────────────────────

TEST(TypeCollectionTraits, OneParamImpliesAutoKey) {
    using traits = comdare::type_collection_traits<int>;
    static_assert(std::is_same_v<traits::key_t, std::uint64_t>);
    static_assert(std::is_same_v<traits::value_t, int>);
    static_assert(traits::key_is_implicit);
    static_assert(traits::param_count == 1);
    SUCCEED();
}

TEST(TypeCollectionTraits, TwoParamsExplicitKeyValue) {
    using traits = comdare::type_collection_traits<std::string, int>;
    static_assert(std::is_same_v<traits::key_t, std::string>);
    static_assert(std::is_same_v<traits::value_t, int>);
    static_assert(!traits::key_is_implicit);
    static_assert(traits::param_count == 2);
    SUCCEED();
}

TEST(TypeCollectionTraits, ThreeParamsValueIsTuple) {
    using traits = comdare::type_collection_traits<std::string, int, double>;
    static_assert(std::is_same_v<traits::key_t, std::string>);
    static_assert(std::is_same_v<traits::value_t, std::tuple<int, double>>);
    static_assert(!traits::key_is_implicit);
    static_assert(traits::param_count == 3);
    SUCCEED();
}

TEST(TypeCollectionTraits, NParamsValueIsTuple) {
    using traits = comdare::type_collection_traits<std::string, int, double, char, long>;
    static_assert(std::is_same_v<traits::value_t, std::tuple<int, double, char, long>>);
    static_assert(traits::param_count == 5);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// search_algorithm_type_collection
// ─────────────────────────────────────────────────────────────────────────────

TEST(SearchAlgorithmTypeCollection, OneParamGeneratesImplicitKeys) {
    using col = comdare::search_algorithm_type_collection<std::string>;
    static_assert(col::key_is_implicit);
    auto k1 = col::next_implicit_key();
    auto k2 = col::next_implicit_key();
    auto k3 = col::next_implicit_key();
    EXPECT_EQ(k2, k1 + 1);
    EXPECT_EQ(k3, k2 + 1);
}

TEST(SearchAlgorithmTypeCollection, ToBinaryWorksForSimpleKey) {
    using col = comdare::search_algorithm_type_collection<std::uint64_t, std::string>;
    static_assert(!col::key_is_implicit);
    std::uint64_t const k      = 42;
    auto                binary = col::to_binary(k);
    EXPECT_EQ(binary.size(), comdare::fingerprint::kFixedKeyBytes);
}

// ─────────────────────────────────────────────────────────────────────────────
// fingerprint - to_binary_string Overloads (REV 7 §4.2(c))
// ─────────────────────────────────────────────────────────────────────────────

TEST(Fingerprint, SimpleTypeBinaryCast) {
    std::uint64_t const k      = 0xDEADBEEFCAFEBABE;
    auto                binary = comdare::fingerprint::to_binary_string(k);
    EXPECT_EQ(binary.size(), 16u);
}

TEST(Fingerprint, StringIsCappedAtFixedLength) {
    std::string const s      = "Hello, World! This is a long string > 16 bytes";
    auto              binary = comdare::fingerprint::to_binary_string(s);
    EXPECT_EQ(binary.size(), 16u);
    // First 16 bytes should match s.data() — count: "Hello, World! Th" = 16 chars
    EXPECT_EQ(static_cast<char>(binary[0]), 'H');
    EXPECT_EQ(static_cast<char>(binary[7]), 'W');
    EXPECT_EQ(static_cast<char>(binary[14]), 'T');
    EXPECT_EQ(static_cast<char>(binary[15]), 'h');
}

TEST(Fingerprint, ComplexTypeUsesHash) {
    struct ComplexKey {
        std::uint64_t a;
        std::uint64_t b;
        std::uint64_t c;
    };
    ComplexKey k{1, 2, 3};
    auto       binary = comdare::fingerprint::to_binary_string(k);
    EXPECT_EQ(binary.size(), 16u);
}

TEST(Fingerprint, FixedLengthFingerprintDeterministic) {
    std::uint64_t k  = 12345;
    auto          h1 = comdare::fingerprint::FixedLengthFingerprint<16>::hash(k);
    auto          h2 = comdare::fingerprint::FixedLengthFingerprint<16>::hash(k);
    EXPECT_EQ(h1, h2); // same input → same output
}

TEST(Fingerprint, DifferentInputsDifferentHash) {
    std::uint64_t k1 = 100;
    std::uint64_t k2 = 200;
    auto          h1 = comdare::fingerprint::FixedLengthFingerprint<16>::hash(k1);
    auto          h2 = comdare::fingerprint::FixedLengthFingerprint<16>::hash(k2);
    EXPECT_NE(h1, h2); // different input → likely different output
}

// ─────────────────────────────────────────────────────────────────────────────
// resolve_baustein - Compile-time-Fallback (REV 7 §6.2)
// ─────────────────────────────────────────────────────────────────────────────

struct TestBausteineTag {};

struct AlgoWithBaustein {
    template <typename Tag>
    struct baustein_t {
        int value = 42;
    };
};

struct AlgoWithoutBaustein {};

TEST(ResolveBaustein, HasMemberBausteinConcept) {
    static_assert(comdare::has_member_baustein<AlgoWithBaustein, TestBausteineTag>);
    static_assert(!comdare::has_member_baustein<AlgoWithoutBaustein, TestBausteineTag>);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// processing_strategy - Compile-time + Runtime
// ─────────────────────────────────────────────────────────────────────────────

TEST(ProcessingStrategy, DefaultConstructible) {
    comdare::processing_strategy<> strategy;
    EXPECT_EQ(strategy.current_mode, comdare::cache_engine::CacheEngineMode::HEURISTIC_STATIC);
    EXPECT_TRUE(strategy.verhalten.enable_telemetry_collection);
    EXPECT_TRUE(strategy.verhalten.enable_adaptive_resize);
    EXPECT_TRUE(strategy.verhalten.enable_hugepage_filler);
}

TEST(ProcessingStrategy, LimitsConfigurable) {
    comdare::processing_strategy<> strategy;
    strategy.limits.max_cache_pages       = 1024;
    strategy.limits.prefetch_distance_max = 8;
    EXPECT_EQ(strategy.limits.max_cache_pages, 1024u);
    EXPECT_EQ(strategy.limits.prefetch_distance_max, 8u);
}

// ─────────────────────────────────────────────────────────────────────────────
// module_abi_v1 - C-API POD-Structs (REV 6 §5.28.4)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ModuleAbi, AbiVersionConstantSet) { EXPECT_EQ(COMDARE_ABI_VERSION, 1); }

TEST(ModuleAbi, AllStructsArePod) {
    static_assert(std::is_standard_layout_v<comdare_workload_descriptor_v1>);
    static_assert(std::is_standard_layout_v<comdare_measurement_record_v1>);
    static_assert(std::is_standard_layout_v<comdare_hw_counters_v1>);
    static_assert(std::is_standard_layout_v<comdare_request_context_v1>);
    static_assert(std::is_standard_layout_v<comdare_cache_recommendation_v1>);
    static_assert(std::is_standard_layout_v<comdare_sub_engine_event_v1>);
    static_assert(std::is_standard_layout_v<comdare_platform_snapshot_v1>);
    static_assert(std::is_standard_layout_v<comdare_cache_engine_v1>);
    static_assert(std::is_standard_layout_v<comdare_permutation_module_v1>);
    SUCCEED();
}

TEST(ModuleAbi, FunctionPointersDefaultNull) {
    comdare_permutation_module_v1 m{};
    EXPECT_EQ(m.abi_version, 0u);
    EXPECT_EQ(m.create_instance, nullptr);
    EXPECT_EQ(m.destroy_instance, nullptr);
    EXPECT_EQ(m.run_workload, nullptr);
    EXPECT_EQ(m.pull_live_counters, nullptr);
}

TEST(ModuleAbi, SymbolNameIsExpected) {
    std::string symbol = COMDARE_GET_MODULE_V1_SYMBOL;
    EXPECT_EQ(symbol, "comdare_get_module_v1");
}

// ─────────────────────────────────────────────────────────────────────────────
// configuration_permutation
// ─────────────────────────────────────────────────────────────────────────────

TEST(ConfigurationPermutation, AggregatesTypes) {
    using strategy_t = comdare::processing_strategy<>;
    struct DummyConcurrency {};
    struct DummyScheduler {};
    struct DummyHeuristic {};
    struct DummyAllocator {};

    using config = comdare::configuration_permutation<strategy_t, DummyConcurrency, DummyScheduler, DummyHeuristic,
                                                      DummyAllocator>;
    static_assert(std::is_same_v<config::strategy_t, strategy_t>);
    static_assert(std::is_same_v<config::concurrency_t, DummyConcurrency>);
    SUCCEED();
}
