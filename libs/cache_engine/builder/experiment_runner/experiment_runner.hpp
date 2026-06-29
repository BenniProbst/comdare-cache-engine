#pragma once
// ExperimentRunner - laed Permutations-Modul + fuehrt Workload aus + sammelt Resultate
// (REV 7 §5.1 Phase 4 LINK+LOAD + Phase 5 EXECUTE + Phase 6 MEASURE + Phase 7 TEARDOWN)

#include "../permutation_loop/permutation_loop.hpp"

#include <cache_engine/abi/module_abi_v1.hpp>
#include <cache_engine/abi/module_loader.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace comdare::builder::runner {

struct ExperimentResult {
    std::string                   permutation_id;
    std::uint64_t                 fingerprint;
    comdare_measurement_record_v1 record{};
    bool                          succeeded = false;
    std::string                   error_message;
};

class ExperimentRunner {
public:
    [[nodiscard]] ExperimentResult run_one(loop::PermutationDescriptor const&    descriptor,
                                           std::filesystem::path const&          module_binary_path,
                                           comdare_workload_descriptor_v1 const& workload,
                                           comdare_cache_engine_v1*              engine_ref) const;

    [[nodiscard]] std::vector<ExperimentResult> run_all(std::vector<loop::PermutationDescriptor> const& descriptors,
                                                        std::filesystem::path const&          module_binary_dir,
                                                        comdare_workload_descriptor_v1 const& workload,
                                                        comdare_cache_engine_v1*              engine_ref) const;
};

} // namespace comdare::builder::runner
