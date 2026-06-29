#pragma once
// module_abi_v1 - ABI-stabile C-API fuer Permutations-Module (REV 6 §5.28.4)
//
// Jede compilierte ExecutionEngine-Permutation exportiert dieses API als
// reine C-Schnittstelle (POD-structs + function-pointers). Erlaubt:
//   - Versionierung via abi_version field
//   - Compiler-Mix (Master + Modul koennen verschiedene Compiler nutzen)
//   - dlopen/LoadLibrary-basiertes Loading durch CacheEngineBuilder

#include <cstddef>
#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

#define COMDARE_ABI_VERSION 1

typedef struct comdare_workload_descriptor_v1 {
    std::uint32_t version;
    std::uint64_t workload_id;
    std::uint64_t num_operations;
    std::uint32_t key_size_bytes;
    std::uint32_t value_size_bytes;
    void const*   dataset_ptr;
    std::uint64_t dataset_bytes;
} comdare_workload_descriptor_v1;

typedef struct comdare_measurement_record_v1 {
    std::uint32_t version;
    std::uint64_t op_count;
    std::uint64_t total_cycles;
    std::uint64_t cache_misses_l1;
    std::uint64_t cache_misses_l2;
    std::uint64_t cache_misses_l3;
    std::uint64_t dtlb_misses;
    std::uint64_t coherence_invalidations;
    std::uint64_t energy_micro_joules;
    std::uint64_t bytes_allocated;
    std::uint64_t bytes_in_use_peak;
    double        external_fragmentation;
    double        internal_fragmentation;
} comdare_measurement_record_v1;

typedef struct comdare_hw_counters_v1 {
    std::uint64_t timestamp_ns;
    std::uint64_t cycles;
    std::uint64_t instructions;
    std::uint64_t cache_misses[3]; // L1, L2, L3
    std::uint64_t tlb_misses[2];   // d-TLB, i-TLB
} comdare_hw_counters_v1;

typedef struct comdare_request_context_v1 {
    std::uint32_t version;
    std::uint64_t request_id;
    std::uint8_t  operation_kind; // 0=lookup, 1=insert, 2=erase, 3=range_scan
    std::uint8_t  key_size_class;
    std::uint16_t reserved;
    std::uint32_t key_hash;
} comdare_request_context_v1;

typedef struct comdare_cache_recommendation_v1 {
    std::uint8_t  action_kind; // 0=no_action, 1=prefetch, 2=migrate, 3=pin
    std::uint8_t  prefetch_distance;
    std::uint16_t reserved;
    std::uint32_t hint_target_core_id;
    std::uint64_t hint_target_address;
} comdare_cache_recommendation_v1;

typedef struct comdare_sub_engine_event_v1 {
    std::uint8_t  event_kind;        // 0=warn, 1=info, 2=critical
    std::uint8_t  source_sub_engine; // C01..C12
    std::uint16_t reserved;
    std::uint64_t payload;
} comdare_sub_engine_event_v1;

typedef struct comdare_platform_snapshot_v1 {
    std::uint32_t version;
    std::uint32_t numa_node_count;
    std::uint32_t cpu_count;
    std::uint32_t l1_size_bytes;
    std::uint32_t l2_size_bytes;
    std::uint32_t l3_size_bytes;
    std::uint32_t hugepage_size_bytes;
    std::uint32_t cache_line_bytes;
    std::uint8_t  has_avx2;
    std::uint8_t  has_avx512;
    std::uint8_t  has_neon;
    std::uint8_t  has_sve2;
    std::uint32_t reserved;
} comdare_platform_snapshot_v1;

// CacheEngine als Visitor-Pattern-Service vom Master in Modul gereicht
typedef struct comdare_cache_engine_v1 {
    std::uint32_t abi_version;
    void*         engine_state;
    comdare_cache_recommendation_v1 (*advise)(void* engine_state, comdare_request_context_v1 const* ctx);
    void (*notify)(void* engine_state, comdare_sub_engine_event_v1 const* event);
    void (*snapshot)(void* engine_state, comdare_platform_snapshot_v1* out);
} comdare_cache_engine_v1;

// Permutations-Modul exportiert dieses Struct als entry-point
typedef struct comdare_permutation_module_v1 {
    std::uint32_t abi_version;
    std::uint64_t permutation_fingerprint;
    void* (*create_instance)(comdare_cache_engine_v1* engine);
    void (*destroy_instance)(void* instance);
    void (*run_workload)(void* instance, comdare_workload_descriptor_v1 const* workload,
                         comdare_measurement_record_v1* out_record);
    void (*pull_live_counters)(void* instance, comdare_hw_counters_v1* out_counters);
} comdare_permutation_module_v1;

// Symbol that each permutation module MUST export
typedef comdare_permutation_module_v1 const* (*comdare_get_module_v1_fn)(void);
#define COMDARE_GET_MODULE_V1_SYMBOL "comdare_get_module_v1"

#ifdef __cplusplus
} // extern "C"
#endif
