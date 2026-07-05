// V34.A.1 (2026-05-21) - Tests fuer CacheEngineExecutionEngineAdapter
//
// @subsystem CE
// @phase_owner CEB

#include "cache_engine/abi/cache_engine_execution_engine_adapter.hpp"
#include "execute_engine_command.hpp"
#include "workload.hpp"

#include <gtest/gtest.h>

namespace ce_abi = comdare::cache_engine::abi; // ce_abi avoids libstdc++ <cxxabi.h> global abi alias via gtest
namespace cmd    = comdare::cache_engine::builder::commands;

TEST(CacheEngineExecutionEngineAdapter, EngineName) {
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter;
    EXPECT_EQ(adapter.engine_name(), "CacheEngine-EE-A");
}

TEST(CacheEngineExecutionEngineAdapter, LookupOnEmptyReturnsNullopt) {
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter;
    auto                                        result = adapter.lookup("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST(CacheEngineExecutionEngineAdapter, InsertThenLookup) {
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter;
    EXPECT_TRUE(adapter.insert("k1", 42));
    auto val = adapter.lookup("k1");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42u);
}

TEST(CacheEngineExecutionEngineAdapter, AsEngineCallableYCSBC) {
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter;
    auto                                        callable = adapter.as_engine_callable();

    // 10 Lookups -> alle miss (Empty Backend) aber success
    for (std::size_t i = 0; i < 10; ++i) {
        auto out = callable(i, cmd::WorkloadKind::YCSB_C_ReadOnly, 42);
        EXPECT_TRUE(out.success);
        EXPECT_EQ(out.cache_misses_delta, 1u); // Empty Backend = miss
    }
    EXPECT_EQ(adapter.ops_executed(), 10u);
}

TEST(CacheEngineExecutionEngineAdapter, AsEngineCallableYCSBA_MixedWriteRead) {
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter;
    auto                                        callable = adapter.as_engine_callable();

    for (std::size_t i = 0; i < 10; ++i) {
        auto out = callable(i, cmd::WorkloadKind::YCSB_A_Read50Write50, 42);
        EXPECT_TRUE(out.success);
    }
    EXPECT_EQ(adapter.ops_executed(), 10u);
    // Mindestens einige Writes haben das Backend gefuellt
    EXPECT_GT(adapter.backend().size(), 0u);
}

TEST(CacheEngineExecutionEngineAdapter, IntegrationWithExecuteEngineCommand) {
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter;
    cmd::Workload                               w{};
    w.kind            = cmd::WorkloadKind::YCSB_C_ReadOnly;
    w.operation_count = 100;
    w.seed            = 42;

    cmd::ExecuteEngineCommand command(adapter.engine_name(), w, adapter.as_engine_callable());
    int                       rc = command.execute();
    EXPECT_EQ(rc, 0);
    EXPECT_TRUE(command.result().success);
    EXPECT_EQ(adapter.ops_executed(), 100u);
    // Memory-Footprint = bytes_touched aufsummiert = 100 * 32 = 3200
    EXPECT_EQ(command.result().memory_footprint_bytes, 100u * 32u);
}

TEST(CacheEngineExecutionEngineAdapter, CustomBackendInjection) {
    // Beispiel mit explizit injiziertem Backend
    ce_abi::DefaultMapBackend<std::string, std::uint64_t> backend;
    backend.insert("preloaded", 999);
    ce_abi::CacheEngineExecutionEngineAdapter<> adapter(std::move(backend));
    auto                                        val = adapter.lookup("preloaded");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 999u);
}
