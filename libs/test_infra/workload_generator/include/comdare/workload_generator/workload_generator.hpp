#pragma once
// WorkloadGenerator - Synthetic + YCSB-style Workloads (REV 7 §7 + Phase 7.1)
//
// Generiert Test-Workloads als comdare_workload_descriptor_v1 fuer den
// CacheEngineBuilder. Unterstuetzt:
//   - YCSB-A/B/C/D/E/F (read/update/scan/insert mix)
//   - Synthetic uniform/zipfian/sequential key distributions
//   - String + integer keys

#include <cache_engine/abi/module_abi_v1.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace comdare::workload_generator {

enum class YcsbWorkload : std::uint8_t {
    A, // 50% read, 50% update, zipfian
    B, // 95% read, 5% update, zipfian
    C, // 100% read, zipfian
    D, // 95% read latest, 5% insert
    E, // 95% scan, 5% insert, zipfian
    F, // 50% read, 50% read-modify-write, zipfian
};

enum class KeyDistribution : std::uint8_t {
    Uniform,
    Zipfian,
    Sequential,
    Latest,
};

enum class OperationKind : std::uint8_t {
    Read            = 0,
    Update          = 1,
    Insert          = 2,
    Scan            = 3,
    ReadModifyWrite = 4,
    Erase           = 5,
};

struct WorkloadConfig {
    std::uint64_t   num_keys         = 100000;
    std::uint64_t   num_operations   = 1000000;
    std::uint32_t   key_size_bytes   = 16;
    std::uint32_t   value_size_bytes = 64;
    KeyDistribution key_distribution = KeyDistribution::Uniform;
    double          zipfian_theta    = 0.99; // Zipfian skew (0..1, höher = skewed)
    std::uint64_t   random_seed      = 42;
};

struct Operation {
    OperationKind op;
    std::uint64_t key_id;
    std::uint32_t scan_length = 0; // nur fuer Scan
};

class WorkloadGenerator {
public:
    explicit WorkloadGenerator(WorkloadConfig config) noexcept : config_{config} {}

    // Generiert YCSB-Standard-Workload
    [[nodiscard]] std::vector<Operation> generate_ycsb(YcsbWorkload workload);

    // Generiert pure key-distribution (alle Reads)
    [[nodiscard]] std::vector<Operation> generate_uniform_reads();
    [[nodiscard]] std::vector<Operation> generate_zipfian_reads();
    [[nodiscard]] std::vector<Operation> generate_sequential_reads();

    // Erzeugt comdare_workload_descriptor_v1 fuer ABI
    [[nodiscard]] comdare_workload_descriptor_v1 to_abi_descriptor(std::span<Operation const> ops) const noexcept;

    [[nodiscard]] WorkloadConfig const& config() const noexcept { return config_; }

private:
    [[nodiscard]] std::uint64_t next_uniform_key(std::mt19937_64& gen) const;
    [[nodiscard]] std::uint64_t next_zipfian_key(std::mt19937_64& gen) const;
    [[nodiscard]] std::uint64_t next_sequential_key(std::uint64_t op_index) const;
    [[nodiscard]] OperationKind sample_op_kind(YcsbWorkload workload, std::mt19937_64& gen) const;

    WorkloadConfig config_;
};

} // namespace comdare::workload_generator
