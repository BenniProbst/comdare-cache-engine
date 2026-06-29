#include "experiment_runner.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace comdare::builder::runner {

ExperimentResult ExperimentRunner::run_one(loop::PermutationDescriptor const&    descriptor,
                                           std::filesystem::path const&          module_binary_path,
                                           comdare_workload_descriptor_v1 const& workload,
                                           comdare_cache_engine_v1*              engine_ref) const {
    ExperimentResult result;
    result.permutation_id = descriptor.id;
    result.fingerprint    = descriptor.fingerprint;

    try {
        // Phase 4 LINK + LOAD
        abi::PermutationModule module{module_binary_path};
        // Phase 5 EXECUTE
        void* instance = module.create_instance(engine_ref);
        // Phase 6 MEASURE
        module.run_workload(instance, &workload, &result.record);
        // Phase 7 TEARDOWN
        module.destroy_instance(instance);
        result.succeeded = true;
    } catch (std::exception const& ex) {
        result.error_message = ex.what();
        result.succeeded     = false;
    }
    return result;
}

std::vector<ExperimentResult> ExperimentRunner::run_all(std::vector<loop::PermutationDescriptor> const& descriptors,
                                                        std::filesystem::path const&          module_binary_dir,
                                                        comdare_workload_descriptor_v1 const& workload,
                                                        comdare_cache_engine_v1*              engine_ref) const {
    std::vector<ExperimentResult> results;
    results.reserve(descriptors.size());
    for (auto const& d : descriptors) {
        std::ostringstream name;
        name << "comdare_perm_" << std::hex << d.fingerprint;
        // Plattform-spezifische Erweiterung (.so / .dll / .dylib)
        std::filesystem::path bin_path = module_binary_dir / name.str();
#ifdef _WIN32
        bin_path += ".dll";
#elif defined(__APPLE__)
        bin_path += ".dylib";
#else
        bin_path += ".so";
#endif
        results.push_back(run_one(d, bin_path, workload, engine_ref));
    }
    return results;
}

} // namespace comdare::builder::runner
