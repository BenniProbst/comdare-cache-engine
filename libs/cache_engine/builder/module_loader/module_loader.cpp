// ModuleLoader - Plattform-spezifische Implementation
// Phase 7.2.D (2026-05-13)

#include "module_loader.hpp"

#include <algorithm>
#include <iostream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace comdare::builder::loader {

namespace {

[[nodiscard]] void* native_load(std::filesystem::path const& p) noexcept {
#if defined(_WIN32)
    return static_cast<void*>(LoadLibraryW(p.wstring().c_str()));
#else
    return ::dlopen(p.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void native_unload(void* h) noexcept {
    if (!h) return;
#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(h));
#else
    ::dlclose(h);
#endif
}

[[nodiscard]] void* native_symbol(void* h, char const* name) noexcept {
    if (!h) return nullptr;
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(h), name));
#else
    return ::dlsym(h, name);
#endif
}

} // anonymous namespace

void ModuleHandle::unload() noexcept {
    if (native_) {
        native_unload(native_);
        native_ = nullptr;
    }
    module_ = nullptr;
}

std::string ModuleLoader::platform_suffix() {
#if defined(_WIN32)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

int ModuleLoader::load(std::filesystem::path const& dll_path, ModuleHandle& handle_out) noexcept {
    std::error_code ec;
    if (!std::filesystem::exists(dll_path, ec)) { return status_file_not_found; }

    void* h = native_load(dll_path);
    if (!h) { return status_load_failed; }

    auto* sym = native_symbol(h, COMDARE_GET_MODULE_V1_SYMBOL);
    if (!sym) {
        native_unload(h);
        return status_symbol_not_found;
    }

    auto        fn         = reinterpret_cast<comdare_get_module_v1_fn>(sym);
    auto const* module_ptr = fn();
    if (!module_ptr) {
        native_unload(h);
        return status_null_module;
    }
    if (module_ptr->abi_version != COMDARE_ABI_VERSION) {
        native_unload(h);
        return status_abi_mismatch;
    }

    handle_out = ModuleHandle{h, module_ptr};
    return status_ok;
}

int ModuleLoader::load_all(std::filesystem::path const& dir, std::vector<ModuleHandle>& out_handles) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) { return status_file_not_found; }

    auto const                         suffix = platform_suffix();
    std::vector<std::filesystem::path> candidates;
    for (auto const& entry : std::filesystem::directory_iterator{dir, ec}) {
        if (!entry.is_regular_file()) continue;
        auto const& p  = entry.path();
        auto const  fn = p.filename().string();
        if (fn.find("comdare_perm_") != 0) continue;
        if (p.extension() != suffix) continue;
        candidates.push_back(p);
    }

    std::sort(candidates.begin(), candidates.end());

    int last_status = status_ok;
    for (auto const& p : candidates) {
        ModuleHandle h;
        int const    s = load(p, h);
        if (s == status_ok) {
            out_handles.push_back(std::move(h));
        } else {
            std::cerr << "ModuleLoader: failed to load " << p.string() << " (status=" << s << ")\n";
            last_status = s;
        }
    }
    return last_status;
}

} // namespace comdare::builder::loader
