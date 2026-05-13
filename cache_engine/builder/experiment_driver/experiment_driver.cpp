// SPDX-License-Identifier: Apache-2.0
#include "experiment_driver.hpp"

#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace comdare::builder {

namespace {
std::filesystem::path build_dir_for(std::filesystem::path const& output_dir) {
    return output_dir / "build-perms";
}
}  // namespace

std::filesystem::path const& ExperimentDriver::build_dir() const noexcept {
    static thread_local std::filesystem::path cached = build_dir_for(opts_.output_dir);
    cached = build_dir_for(opts_.output_dir);
    return cached;
}

int ExperimentDriver::phase1_enumerate(
    std::vector<loop::PermutationDescriptor>& out_descriptors)
{
    try {
        xml::XmlConfigParser parser;
        auto cfg = parser.parse(opts_.config_dir);
        loop::PermutationLoop pl;
        out_descriptors = pl.enumerate(cfg);
    } catch (std::exception const& e) {
        std::cerr << "Phase 1 XML-Parse error: " << e.what() << "\n";
        return status_xml_parse_failed;
    }
    return status_ok;
}

int ExperimentDriver::phase2_generate(
    std::vector<loop::PermutationDescriptor> const& descriptors)
{
    try {
        std::filesystem::create_directories(opts_.output_dir);
        codegen::CodegenOptions co;
        co.output_root  = opts_.output_dir / "generated";
        co.comdare_root = opts_.comdare_root;
        codegen::CodegenEngine cg{co};

        std::vector<std::uint64_t> fingerprints;
        fingerprints.reserve(descriptors.size());
        for (auto const& d : descriptors) {
            cg.generate_module(d.cache_engine_perm, d.search_algorithm_perm,
                                d.allocator_perm, d.fingerprint);
            fingerprints.push_back(d.fingerprint);
        }
        cg.generate_aggregate_cmake(fingerprints);
    } catch (std::exception const& e) {
        std::cerr << "Phase 2 codegen error: " << e.what() << "\n";
        return status_codegen_failed;
    }
    return status_ok;
}

int ExperimentDriver::phase3_compile() {
    auto const bdir = build_dir_for(opts_.output_dir);
    std::filesystem::create_directories(bdir);

    auto const generated = opts_.output_dir / "generated";

    std::string configure_cmd =
        "cmake -S \"" + generated.string() + "\" -B \"" + bdir.string() + "\"";
    int rc = std::system(configure_cmd.c_str());
    if (rc != 0) return status_cmake_configure_failed;

    std::string build_cmd =
        "cmake --build \"" + bdir.string() + "\" --target comdare_all_permutations";
    rc = std::system(build_cmd.c_str());
    if (rc != 0) return status_cmake_build_failed;

    return status_ok;
}

int ExperimentDriver::phase4_load(std::vector<loader::ModuleHandle>& out_handles) {
    auto const bdir = build_dir_for(opts_.output_dir);
    auto const dll_dir_debug = bdir / "Debug";
    auto const dll_dir =
        std::filesystem::is_directory(dll_dir_debug) ? dll_dir_debug : bdir;

    int s = loader::ModuleLoader::load_all(dll_dir, out_handles);
    if (out_handles.empty()) return status_no_modules_loaded;
    return s;  // status_ok or last per-module status
}

int ExperimentDriver::phase5_run_workload(
    std::span<loader::ModuleHandle> handles,
    WorkloadOptions const& wopts,
    std::vector<loop::PermutationDescriptor> const& descriptors,
    experiment::ResultAggregator& aggregator)
{
    workload_generator::WorkloadGenerator gen{wopts.config};
    auto ops = gen.generate_ycsb(wopts.workload);
    auto workload_abi = gen.to_abi_descriptor(ops);

    std::unordered_map<std::uint64_t, std::string> fp_to_id;
    for (auto const& d : descriptors) fp_to_id[d.fingerprint] = d.id;

    for (auto& h : handles) {
        auto const* m = h.get();
        if (!m) continue;
        void* inst = m->create_instance(nullptr);
        if (!inst) continue;
        comdare_measurement_record_v1 rec{};
        m->run_workload(inst, &workload_abi, &rec);
        m->destroy_instance(inst);

        experiment::PermutationResult pr;
        auto it = fp_to_id.find(h.fingerprint());
        pr.permutation_id = (it != fp_to_id.end()) ? it->second
                                                    : ("fp_" + std::to_string(h.fingerprint()));
        pr.fingerprint = h.fingerprint();
        pr.record      = rec;
        pr.succeeded   = true;
        aggregator.add(std::move(pr));
    }
    return status_ok;
}

int ExperimentDriver::phase7_export(experiment::ResultAggregator const& agg) {
    try {
        agg.export_csv (opts_.output_dir / "measurements.csv");
        agg.export_json(opts_.output_dir / "measurements.json");
    } catch (std::exception const& e) {
        std::cerr << "Phase 7 export error: " << e.what() << "\n";
        return status_export_failed;
    }
    return status_ok;
}

int ExperimentDriver::run_pipeline_full(WorkloadOptions const& wopts) {
    std::vector<loop::PermutationDescriptor> descriptors;
    int rc = phase1_enumerate(descriptors);
    if (rc != status_ok) return rc;
    if (opts_.enumerate_only) {
        for (auto const& d : descriptors) {
            std::cout << "  " << d.id << " (fp=0x" << std::hex
                      << d.fingerprint << std::dec << ")\n";
        }
        return status_ok;
    }

    rc = phase2_generate(descriptors);
    if (rc != status_ok) return rc;
    if (opts_.skip_build) return status_ok;

    rc = phase3_compile();
    if (rc != status_ok) return rc;

    std::vector<loader::ModuleHandle> handles;
    rc = phase4_load(handles);
    if (rc != status_ok) return rc;

    experiment::ResultAggregator aggregator;
    rc = phase5_run_workload(handles, wopts, descriptors, aggregator);
    if (rc != status_ok) return rc;

    rc = phase7_export(aggregator);
    return rc;
}

}  // namespace comdare::builder
