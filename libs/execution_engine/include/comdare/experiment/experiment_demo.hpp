#pragma once
// ExperimentDemo - End-to-End Mini-Experiment (Phase 7.4)
//
// Verbindet WorkloadGenerator + AllocatorFamily + Benchmark-Suite zu einer
// vollstaendigen Mini-Permutation, die jeden Allokator gegen einen Workload
// laufen laesst und Measurements sammelt.

#include <comdare/workload_generator/workload_generator.hpp>
#include <comdare/benchmark_suite/benchmark_runner.hpp>
#include <comdare/experiment/result_aggregator.hpp>
#include <cache_engine/abi/module_abi_v1.hpp>
#include <cache_engine/allocators/concepts/i_allocation_strategy.hpp>

#include <cstdint>
#include <span>
#include <string>

namespace comdare::experiment {

// Laeuft einen Workload gegen einen einzelnen Allocator + sammelt Stats
template <comdare::cache_engine::allocator::IAllocationStrategy Strategy>
[[nodiscard]] PermutationResult
run_single_experiment(std::string const& permutation_id, std::uint64_t fingerprint, Strategy& strategy,
                      std::span<workload_generator::Operation const> ops, std::uint32_t value_size_bytes) {
    PermutationResult result;
    result.permutation_id = permutation_id;
    result.fingerprint    = fingerprint;

    benchmark_suite::BenchmarkRunner runner{1024 * 32, 4096}; // klein fuer Demo

    // Pre-allocate keyspace
    std::vector<void*> allocations;
    allocations.reserve(ops.size() / 4); // initial estimate

    auto       handle       = runner.begin_measurement("experiment");
    auto const start_cycles = std::chrono::steady_clock::now();

    std::uint64_t ops_executed = 0;
    for (auto const& op : ops) {
        switch (op.op) {
            case workload_generator::OperationKind::Insert:
            case workload_generator::OperationKind::Update:
            case workload_generator::OperationKind::ReadModifyWrite: {
                void* p = strategy.raw_allocate(value_size_bytes, 16);
                if (p) {
                    allocations.push_back(p);
                    runner.record_event(handle, benchmark_suite::EventKind::Allocation, value_size_bytes);
                }
                break;
            }
            case workload_generator::OperationKind::Erase:
                if (!allocations.empty()) {
                    void* p = allocations.back();
                    allocations.pop_back();
                    strategy.raw_deallocate(p, value_size_bytes, 16);
                    runner.record_event(handle, benchmark_suite::EventKind::Deallocation, value_size_bytes);
                }
                break;
            case workload_generator::OperationKind::Read:
            case workload_generator::OperationKind::Scan:
                // Read-only — no allocation
                break;
        }
        ++ops_executed;
    }
    auto const end_cycles = std::chrono::steady_clock::now();
    auto const ns_taken   = std::chrono::duration_cast<std::chrono::nanoseconds>(end_cycles - start_cycles).count();
    runner.end_measurement(handle, static_cast<std::uint64_t>(ns_taken));

    // Cleanup
    for (auto* p : allocations) { strategy.raw_deallocate(p, value_size_bytes, 16); }

    // Stats sammeln
    auto stats                           = strategy.statistics();
    result.record.version                = COMDARE_ABI_VERSION;
    result.record.op_count               = ops_executed;
    result.record.total_cycles           = static_cast<std::uint64_t>(ns_taken);
    result.record.bytes_allocated        = stats.total_bytes_allocated;
    result.record.bytes_in_use_peak      = stats.total_bytes_allocated;
    result.record.external_fragmentation = stats.external_fragmentation;
    result.record.internal_fragmentation = stats.internal_fragmentation;
    result.succeeded                     = true;
    return result;
}

} // namespace comdare::experiment
