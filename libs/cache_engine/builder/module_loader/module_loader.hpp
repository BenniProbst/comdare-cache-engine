#pragma once
// ModuleLoader - plattform-agnostisches dlopen/LoadLibrary fuer
// Permutations-Module (Phase 7.2.D, 2026-05-13)
//
// Aufgabe: laedt eine Permutation-DLL/.so, resolved das Symbol
// `comdare_get_module_v1` und liefert den ABI-Modul-Pointer.
// RAII: Destruktor entlaedt das Modul.

#include <cache_engine/abi/module_abi_v1.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace comdare::builder::loader {

// errno-style Status (analog prt_art/identity/status.hpp)
inline constexpr int status_ok               = 0;
inline constexpr int status_file_not_found   = 1;
inline constexpr int status_load_failed      = 2;
inline constexpr int status_symbol_not_found = 3;
inline constexpr int status_abi_mismatch     = 4;
inline constexpr int status_null_module      = 5;

class ModuleHandle {
public:
    ModuleHandle() = default;
    explicit ModuleHandle(void* native_handle, comdare_permutation_module_v1 const* module) noexcept
        : native_{native_handle}, module_{module} {}

    // Non-copyable, movable
    ModuleHandle(ModuleHandle const&)            = delete;
    ModuleHandle& operator=(ModuleHandle const&) = delete;
    ModuleHandle(ModuleHandle&& other) noexcept : native_{other.native_}, module_{other.module_} {
        other.native_ = nullptr;
        other.module_ = nullptr;
    }
    ModuleHandle& operator=(ModuleHandle&& other) noexcept {
        if (this != &other) {
            unload();
            native_       = other.native_;
            module_       = other.module_;
            other.native_ = nullptr;
            other.module_ = nullptr;
        }
        return *this;
    }
    ~ModuleHandle() { unload(); }

    [[nodiscard]] bool                                 valid() const noexcept { return module_ != nullptr; }
    [[nodiscard]] comdare_permutation_module_v1 const* get() const noexcept { return module_; }
    [[nodiscard]] std::uint64_t fingerprint() const noexcept { return module_ ? module_->permutation_fingerprint : 0; }

    void unload() noexcept; // plattform-spezifisch

private:
    void*                                native_ = nullptr;
    comdare_permutation_module_v1 const* module_ = nullptr;
};

// ModuleLoader: loadet eine einzige DLL und gibt einen ModuleHandle zurueck
class ModuleLoader {
public:
    // Loadet `dll_path`. Bei Erfolg: status_ok, handle_out gesetzt.
    // Bei Misserfolg: errno-style status, handle_out unveraendert.
    [[nodiscard]] static int load(std::filesystem::path const& dll_path, ModuleHandle& handle_out) noexcept;

    // Loadet alle .dll/.so in einem Verzeichnis (Pattern: comdare_perm_*.dll/.so)
    [[nodiscard]] static int load_all(std::filesystem::path const& dir, std::vector<ModuleHandle>& out_handles);

    // Plattform-Suffix: ".dll" auf Windows, ".so" auf POSIX
    [[nodiscard]] static std::string platform_suffix();
};

} // namespace comdare::builder::loader
