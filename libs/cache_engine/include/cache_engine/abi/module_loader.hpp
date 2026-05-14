#pragma once
// module_loader - dlopen/LoadLibrary Wrapper fuer Permutations-Module (REV 6 §5.28)
//
// Vom CacheEngineBuilder verwendet, um compilierte Permutations-Module
// zur Laufzeit zu laden + die ABI-stabile C-API anzubinden.

#include "module_abi_v1.hpp"

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    using dl_handle_t = HMODULE;
    inline dl_handle_t dl_open(char const* path) {
        return ::LoadLibraryA(path);
    }
    inline void* dl_sym(dl_handle_t h, char const* name) {
        return reinterpret_cast<void*>(::GetProcAddress(h, name));
    }
    inline void dl_close(dl_handle_t h) {
        if (h) ::FreeLibrary(h);
    }
    inline char const* dl_error_msg() { return "Windows LoadLibrary error"; }
#else
    #include <dlfcn.h>
    using dl_handle_t = void*;
    inline dl_handle_t dl_open(char const* path) {
        return ::dlopen(path, RTLD_NOW | RTLD_LOCAL);
    }
    inline void* dl_sym(dl_handle_t h, char const* name) {
        return ::dlsym(h, name);
    }
    inline void dl_close(dl_handle_t h) {
        if (h) ::dlclose(h);
    }
    inline char const* dl_error_msg() { return ::dlerror(); }
#endif

namespace comdare::abi {

class PermutationModule {
public:
    explicit PermutationModule(std::filesystem::path const& path) {
        handle_ = dl_open(path.string().c_str());
        if (!handle_) {
            throw std::runtime_error{"Could not load module: " +
                                     path.string() + " (" + dl_error_msg() + ")"};
        }
        auto get_fn = reinterpret_cast<comdare_get_module_v1_fn>(
            dl_sym(handle_, COMDARE_GET_MODULE_V1_SYMBOL));
        if (!get_fn) {
            dl_close(handle_);
            throw std::runtime_error{
                "Module does not export " COMDARE_GET_MODULE_V1_SYMBOL};
        }
        module_ = get_fn();
        if (!module_ || module_->abi_version != COMDARE_ABI_VERSION) {
            dl_close(handle_);
            throw std::runtime_error{"Module ABI version mismatch"};
        }
    }

    ~PermutationModule() {
        if (handle_) dl_close(handle_);
    }

    PermutationModule(PermutationModule const&) = delete;
    PermutationModule& operator=(PermutationModule const&) = delete;
    PermutationModule(PermutationModule&& other) noexcept
        : handle_{other.handle_}, module_{other.module_} {
        other.handle_ = nullptr;
        other.module_ = nullptr;
    }

    [[nodiscard]] comdare_permutation_module_v1 const& api() const noexcept {
        return *module_;
    }

    [[nodiscard]] void* create_instance(comdare_cache_engine_v1* engine) const {
        return module_->create_instance(engine);
    }
    void destroy_instance(void* inst) const {
        module_->destroy_instance(inst);
    }
    void run_workload(void* inst,
                      comdare_workload_descriptor_v1 const* workload,
                      comdare_measurement_record_v1* out) const {
        module_->run_workload(inst, workload, out);
    }
    void pull_live_counters(void* inst, comdare_hw_counters_v1* out) const {
        module_->pull_live_counters(inst, out);
    }

private:
    dl_handle_t handle_  = nullptr;
    comdare_permutation_module_v1 const* module_ = nullptr;
};

}  // namespace comdare::abi
