#pragma once
// ResultAggregator - sammelt + vergleicht MeasurementRecords pro Permutation
// (REV 7 §5.1 Phase 6 MEASURE + Phase 7.3)

#include <cache_engine/abi/module_abi_v1.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace comdare::experiment {

struct PermutationResult {
    std::string                   permutation_id;
    std::uint64_t                 fingerprint;
    comdare_measurement_record_v1 record{};
    bool                          succeeded = false;
    std::string                   error_message;
    // REV 7.6 V20.1 — Welcher Workload tatsaechlich gegen das Modul lief.
    // Pro Permutation gesetzt durch ExperimentDriver Phase 5 (V20.2).
    // Fuer Profile-Module: aus expected_workload (V19.3) oder traversal-Heuristik (V11.2).
    // Fuer Nicht-Profile-Module: leer (= WorkloadOptions-Default).
    std::string workload_used;
};

struct ComparisonReport {
    std::string baseline_id;
    std::string candidate_id;
    double      throughput_speedup; // candidate/baseline
    double      memory_ratio;       // candidate/baseline (lower = better)
    double      latency_ratio;      // candidate/baseline
    double      cache_miss_ratio;   // candidate/baseline
};

class ResultAggregator {
public:
    // Sammelt Resultate
    void add(PermutationResult result);

    // Setze Baseline (z.B. PRT-ART-Default)
    void set_baseline(std::string baseline_id);

    // Vergleicht alle Permutationen gegen Baseline
    [[nodiscard]] std::vector<ComparisonReport> compare_against_baseline() const;

    // Export
    void export_csv(std::filesystem::path const& path) const;
    void export_json(std::filesystem::path const& path) const;

    [[nodiscard]] std::size_t                           result_count() const noexcept { return results_.size(); }
    [[nodiscard]] std::vector<PermutationResult> const& results() const noexcept { return results_; }

private:
    [[nodiscard]] std::optional<PermutationResult> find_baseline() const;

    std::vector<PermutationResult> results_;
    std::string                    baseline_id_;
};

} // namespace comdare::experiment
