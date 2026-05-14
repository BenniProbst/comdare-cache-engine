// SPDX-License-Identifier: Apache-2.0
// ExperimentDriver — Implementation (REV 7.6 + Diagnose-Restore)

#include "experiment_driver.hpp"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace comdare::builder {

namespace {

[[nodiscard]] std::filesystem::path build_dir_for(std::filesystem::path const& output_dir) {
    return output_dir / "build-perms";
}

}  // anonymous

std::filesystem::path const& ExperimentDriver::build_dir() const noexcept {
    static thread_local std::filesystem::path cached;
    cached = build_dir_for(opts_.output_dir);
    return cached;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 1: ENUMERATION (XML-Parsing + Permutations-Enumeration)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::phase1_enumerate(
    std::vector<loop::PermutationDescriptor>& out_descriptors)
{
    try {
        xml::XmlConfigParser parser;
        auto cfg = parser.parse(opts_.config_dir);

        if (opts_.verbose) {
            std::cout << "\n[Phase 1] Parsed XML configs:\n";
            std::cout << "  cache_engine_permutations:     "
                      << cfg.cache_engine_permutations.size() << "\n";
            std::cout << "  search_algorithm_permutations: "
                      << cfg.search_algorithm_permutations.size() << "\n";
            std::cout << "  allocator_permutations:        "
                      << cfg.allocator_permutations.size() << "\n";
            std::cout << "  test_data_sets:                "
                      << cfg.test_data_sets.size() << "\n";
        }

        loop::PermutationLoop pl;
        out_descriptors = pl.enumerate(cfg);

        if (opts_.verbose) {
            std::cout << "  Enumerated " << out_descriptors.size()
                      << " permutations.\n";
        }
    } catch (std::exception const& e) {
        std::cerr << "[Phase 1] XML-Parse error: " << e.what() << "\n";
        return status_xml_parse_failed;
    }
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 2: CODEGEN (Modul-Source-Generation + Aggregator-CMakeLists)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::phase2_generate(
    std::vector<loop::PermutationDescriptor> const& descriptors)
{
    try {
        std::filesystem::create_directories(opts_.output_dir);
        codegen::CodegenOptions co;
        co.output_root  = opts_.output_dir / "generated";
        co.comdare_root = opts_.comdare_root;
        codegen::CodegenEngine cg{co};

        if (opts_.verbose) {
            std::cout << "\n[Phase 2] Generating " << descriptors.size()
                      << " modules ...\n";
        }

        std::vector<std::uint64_t> fingerprints;
        fingerprints.reserve(descriptors.size());
        for (auto const& d : descriptors) {
            cg.generate_module(d.cache_engine_perm, d.search_algorithm_perm,
                                d.allocator_perm, d.fingerprint);
            fingerprints.push_back(d.fingerprint);
            if (opts_.verbose) {
                std::cout << "  Generated: " << d.id << "\n";
            }
        }

        // REV 7.6 V10.5 — Auto-Pickup von algorithm_profiles/sota/*.profile.xml
        // Wenn vorhanden, generiere zusaetzlich pro Profil ein Modul.
        auto profiles_dir = opts_.config_dir / "algorithm_profiles" / "sota";
        if (!std::filesystem::is_directory(profiles_dir)) {
            // Fallback: relative zum comdare_root cache-engine
            profiles_dir = opts_.comdare_root / "cache_engine" / "algorithm_profiles" / "sota";
        }
        if (std::filesystem::is_directory(profiles_dir)) {
            xml::XmlConfigParser parser;
            auto profiles = parser.load_sota_profiles(profiles_dir);

            // REV 7.6 V10.6 — Messreihen-Mode-Filter (Defined/Full)
            std::size_t filtered_out = 0;
            if (opts_.messreihen_mode == ExperimentDriverOptions::MessreihenMode::Defined &&
                !opts_.sota_profile_filter.empty())
            {
                auto& filter = opts_.sota_profile_filter;
                profiles.erase(
                    std::remove_if(profiles.begin(), profiles.end(),
                        [&](xml::AlgorithmProfile const& p) {
                            bool keep = std::find(filter.begin(), filter.end(), p.id) != filter.end();
                            if (!keep) ++filtered_out;
                            return !keep;
                        }),
                    profiles.end());
            }

            if (opts_.verbose) {
                std::cout << "  [V10.5/V10.6] Auto-Pickup: " << profiles.size()
                          << " SOTA-Profile aus " << profiles_dir.string()
                          << " (mode="
                          << (opts_.messreihen_mode == ExperimentDriverOptions::MessreihenMode::Full ? "full" : "defined");
                if (filtered_out > 0) std::cout << ", " << filtered_out << " gefiltert";
                std::cout << ")\n";
            }
            for (auto const& prof : profiles) {
                // Deterministischer Fingerprint aus Profil-id
                std::uint64_t prof_fp = std::hash<std::string>{}(prof.id) ^ 0xC0FFEE02ull;
                cg.generate_module_from_profile(prof, prof_fp);
                fingerprints.push_back(prof_fp);
                if (opts_.verbose) {
                    std::cout << "    Profile-Modul: " << prof.id
                              << " (paper_ref=" << prof.paper_ref
                              << ", fp=0x" << std::hex << prof_fp << std::dec << ")\n";
                }
            }
        }

        cg.generate_aggregate_cmake(fingerprints);

        if (opts_.verbose) {
            std::cout << "  Aggregator: " << cg.aggregate_cmake_path().string() << "\n";
        }
    } catch (std::exception const& e) {
        std::cerr << "[Phase 2] codegen error: " << e.what() << "\n";
        return status_codegen_failed;
    }
    return status_ok;
}

// REV 7.6 V13.2 — Hot-Compile-Helper fuer enable_runtime_codegen
// Erkennt fehlende Permutations-Module und kompiliert sie via cmake-Aufruf
// auf der bereits generierten source library nach.
namespace {
[[nodiscard]] bool module_binary_exists(std::filesystem::path const& dir,
                                         std::uint64_t fingerprint) {
    auto check_one = [&](std::string const& ext) {
        std::ostringstream name;
        name << "comdare_perm_" << std::hex << fingerprint << ext;
        return std::filesystem::exists(dir / name.str());
    };
#if defined(_WIN32)
    return check_one(".dll") || check_one(".lib");
#elif defined(__APPLE__)
    return check_one(".dylib") || check_one(".so");
#else
    return check_one(".so");
#endif
}

int hot_compile_missing_modules(
    std::filesystem::path const& generated_dir,
    std::filesystem::path const& build_dir,
    std::vector<std::uint64_t> const& fingerprints,
    bool verbose)
{
    int hot_compiled = 0;
    auto const dll_dir_debug = build_dir / "Debug";
    auto const dll_dir = std::filesystem::is_directory(dll_dir_debug)
                          ? dll_dir_debug : build_dir;
    for (auto fp : fingerprints) {
        if (module_binary_exists(dll_dir, fp)) continue;
        std::ostringstream target;
        target << "comdare_perm_" << std::hex << fp;
        std::ostringstream cmd;
        cmd << "cmake --build \"" << build_dir.string() << "\" --target " << target.str();
        if (verbose) {
            std::cout << "    [V13.2 hot-compile] " << cmd.str() << "\n";
        }
        int rc = std::system(cmd.str().c_str());
        if (rc == 0) ++hot_compiled;
    }
    return hot_compiled;
}
}  // anonymous

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: COMPILE (cmake configure + cmake --build)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::phase3_compile() {
    auto const bdir = build_dir_for(opts_.output_dir);
    std::filesystem::create_directories(bdir);

    auto const generated = opts_.output_dir / "generated";

    if (opts_.verbose) {
        std::cout << "\n[Phase 3] Configuring cmake subbuild under "
                  << bdir.string() << " ...\n";
    }

    auto run_cmd = [this](std::string const& cmd) -> int {
        if (opts_.verbose) {
            std::cout << "  $ " << cmd << "\n";
        }
        return std::system(cmd.c_str());
    };

    std::string configure_cmd =
        "cmake -S \"" + generated.string() + "\" -B \"" + bdir.string() + "\"";
    int rc = run_cmd(configure_cmd);
    if (rc != 0) {
        std::cerr << "[Phase 3] cmake configure failed with exit " << rc << "\n";
        return status_cmake_configure_failed;
    }

    std::string build_cmd =
        "cmake --build \"" + bdir.string() + "\" --target comdare_all_permutations";
    rc = run_cmd(build_cmd);
    if (rc != 0) {
        std::cerr << "[Phase 3] cmake --build failed with exit " << rc << "\n";
        return status_cmake_build_failed;
    }

    if (opts_.verbose) {
        std::cout << "[Phase 3] OK\n";
    }
    return status_ok;
}

// REV 7.6 V13.3 — ABI-Vertrags-Pruefung pro geladenem Modul
// (no-op wenn enable_functional_tests=false). Prueft fuer jeden Handle:
//   - create_instance(nullptr) liefert non-null
//   - run_workload mit empty WorkloadDescriptor schreibt rec.version == ABI_VERSION
//   - destroy_instance ist sicher aufrufbar
int ExperimentDriver::phase4b_functional_tests(
    std::span<loader::ModuleHandle> handles)
{
    if (!opts_.enable_functional_tests) return status_ok;
    if (opts_.verbose) {
        std::cout << "\n[Phase 4b V13.3] Functional-Test ABI-Vertraege fuer "
                  << handles.size() << " Module ...\n";
    }
    int passed = 0;
    int failed = 0;
    comdare_workload_descriptor_v1 empty_workload{};
    for (auto& h : handles) {
        auto const* m = h.get();
        if (!m) { ++failed; continue; }
        void* inst = m->create_instance(nullptr);
        if (!inst) { ++failed; continue; }
        comdare_measurement_record_v1 rec{};
        m->run_workload(inst, &empty_workload, &rec);
        bool const abi_ok = (rec.version == COMDARE_ABI_VERSION);
        m->destroy_instance(inst);
        if (abi_ok) ++passed; else ++failed;
    }
    if (opts_.verbose) {
        std::cout << "[Phase 4b V13.3] Functional-Tests: "
                  << passed << " passed / " << failed << " failed\n";
    }
    return failed > 0 ? status_workload_failed : status_ok;
}

// REV 7.6 V13.2 — Public Method: hot-compile fehlende Permutations-Module
// Aufruf nach phase2_generate (oder zwischen phase2 und phase3) wenn
// enable_runtime_codegen aktiviert ist.
int ExperimentDriver::phase3_hot_compile_missing(
    std::vector<std::uint64_t> const& fingerprints)
{
    if (!opts_.enable_runtime_codegen) return status_ok;
    auto const bdir = build_dir_for(opts_.output_dir);
    auto const generated = opts_.output_dir / "generated";
    if (opts_.verbose) {
        std::cout << "\n[Phase 3 V13.2] Hot-Compile fehlende Module ueber "
                  << fingerprints.size() << " Permutationen ...\n";
    }
    int hot_compiled = hot_compile_missing_modules(
        generated, bdir, fingerprints, opts_.verbose);
    if (opts_.verbose) {
        std::cout << "[Phase 3 V13.2] Hot-Compile abgeschlossen: "
                  << hot_compiled << " neu kompiliert\n";
    }
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 4: LINK + LOAD (load_all alle DLLs/.sos aus build-Verzeichnis)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::phase4_load(std::vector<loader::ModuleHandle>& out_handles) {
    auto const bdir = build_dir_for(opts_.output_dir);

    // Multi-config-Generator (VS): DLLs unter <bdir>/Debug
    // Single-config-Generator (Ninja/Make): DLLs unter <bdir>
    auto const dll_dir_debug = bdir / "Debug";
    auto const dll_dir =
        std::filesystem::is_directory(dll_dir_debug) ? dll_dir_debug : bdir;

    if (opts_.verbose) {
        std::cout << "\n[Phase 4] Loading permutation modules from "
                  << dll_dir.string() << " ...\n";
    }

    int const load_status = loader::ModuleLoader::load_all(dll_dir, out_handles);
    if (out_handles.empty()) {
        std::cerr << "[Phase 4] No modules loaded (status=" << load_status << ")\n";
        return status_no_modules_loaded;
    }

    if (opts_.verbose) {
        std::cout << "[Phase 4] Loaded " << out_handles.size() << " modules.\n";
        if (load_status != loader::status_ok) {
            std::cerr << "[Phase 4] Note: some modules failed to load (last status="
                      << load_status << ")\n";
        }
    }
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 5 + 6: EXECUTE + MEASURE (pro Modul: create_instance + run_workload +
//                                 destroy_instance; sammle in Aggregator)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::phase5_run_workload(
    std::span<loader::ModuleHandle> handles,
    WorkloadOptions const& wopts,
    std::vector<loop::PermutationDescriptor> const& descriptors,
    experiment::ResultAggregator& aggregator)
{
    workload_generator::WorkloadGenerator gen{wopts.config};
    auto default_ops = gen.generate_ycsb(wopts.workload);
    auto default_workload_abi = gen.to_abi_descriptor(default_ops);

    if (opts_.verbose) {
        std::cout << "\n[Phase 5+6] Running workload (default: " << default_ops.size()
                  << " ops) against " << handles.size() << " modules ...\n";
    }

    std::unordered_map<std::uint64_t, std::string> fp_to_id;
    for (auto const& d : descriptors) fp_to_id[d.fingerprint] = d.id;

    // REV 7.6 V11.2 — Profile-aware Workload-Routing
    // Pro Profil-Modul wird der Workload aus dem Traversal-Tag des Profils
    // abgeleitet. Mapping (traversal -> YCSB-Workload):
    //   RANGE_SCAN / RANGE_HOT_PATH    -> YCSB-E (range queries)
    //   HOT_PATH / PRTART_HOT_PATH      -> YCSB-A (50/50 read/update Zipfian)
    //   STANDARD (default)               -> YCSB-C (read-only)
    // Fallback fuer Nicht-Profil-Module: WorkloadOptions::workload (wopts)
    auto pick_workload_for_traversal = [](std::string const& traversal) {
        if (traversal.find("RANGE") != std::string::npos)   return workload_generator::YcsbWorkload::E;
        if (traversal.find("HOT_PATH") != std::string::npos) return workload_generator::YcsbWorkload::A;
        return workload_generator::YcsbWorkload::C;
    };

    // Pre-Compute alle Profile-Workload-Mappings (deterministic Fingerprint -> Workload)
    std::unordered_map<std::uint64_t, workload_generator::YcsbWorkload> fp_to_workload;
    auto profiles_dir = opts_.config_dir / "algorithm_profiles" / "sota";
    if (!std::filesystem::is_directory(profiles_dir)) {
        profiles_dir = opts_.comdare_root / "cache_engine" / "algorithm_profiles" / "sota";
    }
    if (std::filesystem::is_directory(profiles_dir)) {
        xml::XmlConfigParser parser;
        auto profiles = parser.load_sota_profiles(profiles_dir);
        for (auto const& p : profiles) {
            std::uint64_t fp = std::hash<std::string>{}(p.id) ^ 0xC0FFEE02ull;
            auto it = p.axes.find("traversal");
            std::string traversal = (it != p.axes.end()) ? it->second : "STANDARD";
            fp_to_workload[fp] = pick_workload_for_traversal(traversal);
        }
    }

    std::uint64_t total_records = 0;
    for (auto& h : handles) {
        auto const* m = h.get();
        if (!m) continue;
        void* inst = m->create_instance(nullptr);
        if (!inst) continue;

        // V11.2 — wenn Modul aus Profil stammt, eigenen Workload bauen
        auto wl_it = fp_to_workload.find(h.fingerprint());
        comdare_workload_descriptor_v1 const* wl_for_this_module = &default_workload_abi;
        std::vector<workload_generator::Operation> profile_ops;
        comdare_workload_descriptor_v1 profile_workload_abi{};
        if (wl_it != fp_to_workload.end()) {
            profile_ops        = gen.generate_ycsb(wl_it->second);
            profile_workload_abi = gen.to_abi_descriptor(profile_ops);
            wl_for_this_module = &profile_workload_abi;
            if (opts_.verbose) {
                std::cout << "    [V11.2] Profile-Modul fp=0x" << std::hex << h.fingerprint()
                          << std::dec << " benutzt YCSB-" << static_cast<int>(wl_it->second) << "\n";
            }
        }

        comdare_measurement_record_v1 rec{};
        m->run_workload(inst, wl_for_this_module, &rec);

        // Phase 7 TEARDOWN (per-Modul-Instance)
        m->destroy_instance(inst);

        experiment::PermutationResult pr;
        auto it = fp_to_id.find(h.fingerprint());
        pr.permutation_id = (it != fp_to_id.end()) ? it->second
                                                    : ("fp_" + std::to_string(h.fingerprint()));
        pr.fingerprint = h.fingerprint();
        pr.record      = rec;
        pr.succeeded   = true;
        aggregator.add(std::move(pr));
        ++total_records;
    }

    if (opts_.verbose) {
        std::cout << "[Phase 5+6] Collected " << total_records << " measurement records.\n";
    }
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 7: PERSIST (CSV + JSON export)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::phase7_export(experiment::ResultAggregator const& agg) {
    try {
        auto const csv_path  = opts_.output_dir / "measurements.csv";
        auto const json_path = opts_.output_dir / "measurements.json";
        agg.export_csv(csv_path);
        agg.export_json(json_path);

        if (opts_.verbose) {
            std::cout << "\n[Phase 7] Wrote " << csv_path.string() << "\n";
            std::cout << "[Phase 7] Wrote " << json_path.string() << "\n";
        }
    } catch (std::exception const& e) {
        std::cerr << "[Phase 7] export error: " << e.what() << "\n";
        return status_export_failed;
    }
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// Convenience: Full Pipeline (Phase 1-7, mit allen Diagnose-Outputs)
// ─────────────────────────────────────────────────────────────────────────────
int ExperimentDriver::run_pipeline_full(WorkloadOptions const& wopts) {
    std::vector<loop::PermutationDescriptor> descriptors;
    int rc = phase1_enumerate(descriptors);
    if (rc != status_ok) return rc;

    if (opts_.enumerate_only) {
        if (opts_.verbose) {
            std::cout << "\n[--enumerate-only] Listing all descriptors:\n";
            for (auto const& d : descriptors) {
                std::cout << "    " << d.id
                          << " (fp=0x" << std::hex << std::setw(16) << std::setfill('0')
                          << d.fingerprint << std::dec << ")\n";
            }
        }
        return status_ok;
    }

    rc = phase2_generate(descriptors);
    if (rc != status_ok) return rc;

    if (opts_.skip_build) {
        if (opts_.verbose) {
            std::cout << "\n[--skip-build] Generation done, cmake not invoked.\n";
        }
        return status_ok;
    }

    rc = phase3_compile();
    if (rc != status_ok) return rc;

    std::vector<loader::ModuleHandle> handles;
    rc = phase4_load(handles);
    if (rc != status_ok) return rc;

    experiment::ResultAggregator aggregator;
    rc = phase5_run_workload(handles, wopts, descriptors, aggregator);
    if (rc != status_ok) return rc;

    rc = phase7_export(aggregator);
    if (rc != status_ok) return rc;

    // Module-Handles werden durch RAII (Vector-Destructor) entladen
    handles.clear();
    if (opts_.verbose) {
        std::cout << "[Phase 7] Unloaded all modules.\n";
    }

    return status_ok;
}

}  // namespace comdare::builder
