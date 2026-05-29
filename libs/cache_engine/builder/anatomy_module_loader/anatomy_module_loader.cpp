// V41.F.6.1.R5.E — AnatomyModuleLoader Plattform-Implementation
//
// Plattform-spezifischer dlopen/LoadLibrary-Code + Version-Check.
//
// @task #708 V41.F.6.1.R5.E

#include "anatomy_module_loader.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

namespace comdare::cache_engine::builder::anatomy_loader {

namespace ana = ::comdare::cache_engine::anatomy;
namespace abi = ::comdare::cache_engine::abi;

// ─────────────────────────────────────────────────────────────────────────────
// Plattform-Helper (dlopen/LoadLibrary Wrapper)
// ─────────────────────────────────────────────────────────────────────────────

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

// Funktion-Pointer-Typen entsprechen den 4 extern "C"-Symbolen in
// anatomy_module_abi_v1.hpp.
using PfnAbiVersion = std::uint64_t (*)();
using PfnAbiMagic   = std::uint64_t (*)();
using PfnCreate     = ana::IAnatomyBase* (*)();
using PfnDestroy    = void (*)(ana::IAnatomyBase*);

}  // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleHandle::unload — RAII-Cleanup in korrekter Reihenfolge
// ─────────────────────────────────────────────────────────────────────────────

void AnatomyModuleHandle::unload() noexcept {
    // 1. Anatomie-Pointer ZUERST freigeben (Pflicht: gleiche .so/.dll-Heap)
    if (anatomy_ && destroy_) {
        destroy_(anatomy_);
    }
    anatomy_ = nullptr;
    destroy_ = nullptr;

    // 2. Modul entladen
    if (native_) {
        native_unload(native_);
        native_ = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleLoader::platform_suffix
// ─────────────────────────────────────────────────────────────────────────────

std::string AnatomyModuleLoader::platform_suffix() {
#if defined(_WIN32)
    return ".dll";
#elif defined(__APPLE__)
    return ".dylib";
#else
    return ".so";
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleLoader::load — Single-File Load mit Version-Validation
// ─────────────────────────────────────────────────────────────────────────────

int AnatomyModuleLoader::load(std::filesystem::path const& dll_path,
                               AnatomyModuleHandle& handle_out) noexcept
{
    std::error_code ec;
    if (!std::filesystem::exists(dll_path, ec)) {
        return status_file_not_found;
    }

    void* native = native_load(dll_path);
    if (!native) {
        return status_load_failed;
    }

    // 4 Pflicht-Symbole resolven
    auto* sym_version = native_symbol(native, "comdare_anatomy_abi_version");
    auto* sym_magic   = native_symbol(native, "comdare_anatomy_abi_magic");
    auto* sym_create  = native_symbol(native, "comdare_create_anatomy");
    auto* sym_destroy = native_symbol(native, "comdare_destroy_anatomy");

    if (!sym_version || !sym_magic || !sym_create || !sym_destroy) {
        native_unload(native);
        return status_symbol_not_found;
    }

    auto pfn_version = reinterpret_cast<PfnAbiVersion>(sym_version);
    auto pfn_magic   = reinterpret_cast<PfnAbiMagic>(sym_magic);
    auto pfn_create  = reinterpret_cast<PfnCreate>(sym_create);
    auto pfn_destroy = reinterpret_cast<PfnDestroy>(sym_destroy);

    // Magic-Check (Pre-Version: sicherer Sanity-Check vor Version-Vergleich)
    if (pfn_magic() != COMDARE_ANATOMY_ABI_MAGIC) {
        native_unload(native);
        return status_magic_mismatch;
    }

    // Version-Check (Major-Match + Modul-Minor <= Host-Minor)
    auto const module_version =
        abi::AnatomyAbiVersion::unpack(pfn_version());

    if (module_version.major != abi::kHostAnatomyAbiVersion.major) {
        native_unload(native);
        return status_abi_major_mismatch;
    }
    if (module_version.minor > abi::kHostAnatomyAbiVersion.minor) {
        native_unload(native);
        return status_abi_minor_too_new;
    }

    // Factory aufrufen
    ana::IAnatomyBase* anatomy = pfn_create();
    if (!anatomy) {
        native_unload(native);
        return status_factory_returned_null;
    }

    // Handle aufbauen (RAII)
    handle_out = AnatomyModuleHandle{native, anatomy, pfn_destroy, module_version};
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleLoader::load_all — Bulk-Load aus Directory
// ─────────────────────────────────────────────────────────────────────────────

int AnatomyModuleLoader::load_all(std::filesystem::path const& dir,
                                   std::vector<AnatomyModuleHandle>& out_handles)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        return status_file_not_found;
    }

    auto const suffix = platform_suffix();
    std::vector<std::filesystem::path> candidates;
    for (auto const& entry : std::filesystem::directory_iterator{dir, ec}) {
        if (!entry.is_regular_file()) continue;
        auto const& p = entry.path();
        auto const fn = p.filename().string();
        // Pattern aus anatomy_codegen.cmake: comdare_anatomy_perm_<fingerprint>.so/.dll
        if (fn.find("comdare_anatomy_perm_") != 0) continue;
        if (p.extension() != suffix)                continue;
        candidates.push_back(p);
    }
    // NUMERISCHE Sortierung nach dem Zahl-Suffix (..._auto_<N>), NICHT lexikographisch:
    // sonst sortiert "..._10" zwischen "..._1" und "..._2", und der Lade-Index (= F15-Label-Index)
    // entspraeche nicht mehr dem Emissions-/Permutations-Index. Primaer nach Prefix-Gruppe
    // (alles vor den End-Ziffern) lexikographisch, sekundaer nach Suffix-Zahl aufsteigend.
    auto split_stem = [](std::filesystem::path const& p) {
        std::string s = p.stem().string();
        std::size_t i = s.size();
        while (i > 0 && std::isdigit(static_cast<unsigned char>(s[i - 1]))) --i;
        long long const num = (i < s.size()) ? std::stoll(s.substr(i)) : -1;
        return std::pair<std::string, long long>{s.substr(0, i), num};
    };
    std::sort(candidates.begin(), candidates.end(),
        [&](std::filesystem::path const& a, std::filesystem::path const& b) {
            auto const [pa, na] = split_stem(a);
            auto const [pb, nb] = split_stem(b);
            if (pa != pb) return pa < pb;
            return na < nb;
        });

    int last_status = status_ok;
    for (auto const& p : candidates) {
        AnatomyModuleHandle h;
        int const s = load(p, h);
        if (s == status_ok) {
            out_handles.push_back(std::move(h));
        } else {
            std::cerr << "AnatomyModuleLoader: failed to load " << p.string()
                      << " (status=" << s << " " << status_name(s) << ")\n";
            last_status = s;
        }
    }
    return last_status;
}

}  // namespace comdare::cache_engine::builder::anatomy_loader
