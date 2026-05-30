#pragma once
// V41.F.6.1.R5.E — AnatomyModuleLoader (dlopen/LoadLibrary fuer Permutations-Binary)
//
// User-Direktive 2026-05-27 (Doku 14 §44.7 + §45.7):
// R5.E ModuleLoader::load_anatomy(dll_path) -> IAnatomyBase* mit
// dlopen/LoadLibrary + ABI-Version-Check.
//
// Aufgabe: laedt eine Anatomy-Permutations-.so/.dll die per
// COMDARE_DEFINE_ANATOMY_MODULE generiert wurde, resolved die 4 Pflicht-Symbole
// (comdare_anatomy_abi_version/magic/create_anatomy/destroy_anatomy), prueft
// ABI-Kompatibilitaet und instantiiert einen SearchAlgorithmAbiAdapter via
// `comdare_create_anatomy()`.
//
// RAII: Destruktor ruft `comdare_destroy_anatomy(ptr)` UND
// `dlclose/FreeLibrary` in korrekter Reihenfolge (Pointer zuerst freigeben,
// dann Modul entladen).
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §46
// @task #708 V41.F.6.1.R5.E
// @related [[execution-engine-als-wurzel]] [[anatomie-gattungen]]

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>   // R6 Ink.2b: leichte ABI-Schnittstelle (entkoppelt von abi_adapter.hpp)

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::anatomy_loader {

// ─────────────────────────────────────────────────────────────────────────────
// errno-style Status (analog comdare::builder::loader)
// ─────────────────────────────────────────────────────────────────────────────

inline constexpr int status_ok                  = 0;
inline constexpr int status_file_not_found      = 1;
inline constexpr int status_load_failed         = 2;
inline constexpr int status_symbol_not_found    = 3;
inline constexpr int status_magic_mismatch      = 4;
inline constexpr int status_abi_major_mismatch  = 5;
inline constexpr int status_abi_minor_too_new   = 6;
inline constexpr int status_factory_returned_null = 7;
inline constexpr int status_null_module         = 8;

[[nodiscard]] constexpr const char* status_name(int s) noexcept {
    switch (s) {
        case status_ok:                  return "ok";
        case status_file_not_found:      return "file_not_found";
        case status_load_failed:         return "load_failed";
        case status_symbol_not_found:    return "symbol_not_found";
        case status_magic_mismatch:      return "magic_mismatch";
        case status_abi_major_mismatch:  return "abi_major_mismatch";
        case status_abi_minor_too_new:   return "abi_minor_too_new";
        case status_factory_returned_null: return "factory_returned_null";
        case status_null_module:         return "null_module";
        default:                          return "unknown";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleHandle — RAII: owns dlopen-Handle + IAnatomyBase*
// ─────────────────────────────────────────────────────────────────────────────

/// AnatomyModuleHandle — kapselt eine geladene Anatomy-Permutations-.so/.dll
/// und einen heap-allokierten `IAnatomyBase*` (via Factory).
///
/// **Lifecycle:**
/// - Default-Konstruktor: leere/ungueltige Handle (valid()==false)
/// - Move-only (Copy-Konstruktor + -Assign sind geloescht)
/// - Destruktor: ruft `comdare_destroy_anatomy(ptr)` UND `dlclose`/`FreeLibrary`
///
/// **Wichtig:** Die Reihenfolge der Cleanup-Operationen ist kritisch:
/// 1. Erst Anatomie-Instanz freigeben (`comdare_destroy_anatomy` muss innerhalb
///    der gleichen .so/.dll aufgerufen werden, sonst Heap-Mismatch)
/// 2. Dann Modul entladen (`dlclose`/`FreeLibrary`)
class AnatomyModuleHandle {
public:
    AnatomyModuleHandle() = default;

    AnatomyModuleHandle(void* native_handle,
                        ::comdare::cache_engine::anatomy::IAnatomyBase* anatomy_ptr,
                        void (*destroy_fn)(::comdare::cache_engine::anatomy::IAnatomyBase*),
                        ::comdare::cache_engine::abi::AnatomyAbiVersion module_version) noexcept
        : native_{native_handle},
          anatomy_{anatomy_ptr},
          destroy_{destroy_fn},
          module_version_{module_version} {}

    // Move-only
    AnatomyModuleHandle(AnatomyModuleHandle const&)            = delete;
    AnatomyModuleHandle& operator=(AnatomyModuleHandle const&) = delete;

    AnatomyModuleHandle(AnatomyModuleHandle&& other) noexcept
        : native_{other.native_},
          anatomy_{other.anatomy_},
          destroy_{other.destroy_},
          module_version_{other.module_version_} {
        other.native_  = nullptr;
        other.anatomy_ = nullptr;
        other.destroy_ = nullptr;
    }

    AnatomyModuleHandle& operator=(AnatomyModuleHandle&& other) noexcept {
        if (this != &other) {
            unload();
            native_         = other.native_;
            anatomy_        = other.anatomy_;
            destroy_        = other.destroy_;
            module_version_ = other.module_version_;
            other.native_   = nullptr;
            other.anatomy_  = nullptr;
            other.destroy_  = nullptr;
        }
        return *this;
    }

    ~AnatomyModuleHandle() { unload(); }

    [[nodiscard]] bool valid() const noexcept { return native_ != nullptr && anatomy_ != nullptr; }

    /// anatomy() — Zugriff auf die geladene IAnatomyBase-Instanz.
    /// Lebenszeit: bis Destruktor / unload() / move-assignment.
    [[nodiscard]] ::comdare::cache_engine::anatomy::IAnatomyBase* anatomy() noexcept {
        return anatomy_;
    }
    [[nodiscard]] ::comdare::cache_engine::anatomy::IAnatomyBase const* anatomy() const noexcept {
        return anatomy_;
    }

    /// module_version() — ABI-Version der geladenen .so/.dll.
    [[nodiscard]] ::comdare::cache_engine::abi::AnatomyAbiVersion module_version() const noexcept {
        return module_version_;
    }

    /// unload() — explizite Freigabe (Destruktor ruft das auch).
    void unload() noexcept;

private:
    void*                                                                              native_         = nullptr;
    ::comdare::cache_engine::anatomy::IAnatomyBase*                                    anatomy_        = nullptr;
    void (*destroy_)(::comdare::cache_engine::anatomy::IAnatomyBase*)                  = nullptr;
    ::comdare::cache_engine::abi::AnatomyAbiVersion                                    module_version_ = {0, 0};
};

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleLoader — load/load_all + Version-Check
// ─────────────────────────────────────────────────────────────────────────────

class AnatomyModuleLoader {
public:
    /// load() — Loadet `dll_path`. Bei Erfolg: status_ok, handle_out gesetzt.
    /// Bei Misserfolg: errno-style status, handle_out unveraendert.
    ///
    /// Validierungs-Schritte (in dieser Reihenfolge):
    /// 1. Datei existiert
    /// 2. dlopen/LoadLibrary erfolgreich
    /// 3. Alle 4 Pflicht-Symbole resolvable
    /// 4. Magic-Number == COMDARE_ANATOMY_ABI_MAGIC
    /// 5. Major-Version match (Host vs Modul)
    /// 6. Modul-Minor <= Host-Minor
    /// 7. Factory comdare_create_anatomy() liefert non-null
    [[nodiscard]] static int load(std::filesystem::path const& dll_path,
                                   AnatomyModuleHandle& handle_out) noexcept;

    /// load_all() — Loadet alle anatomy-Pilot-DLLs in einem Verzeichnis.
    /// Pattern: comdare_anatomy_perm_*.so/.dll/.dylib.
    [[nodiscard]] static int load_all(std::filesystem::path const& dir,
                                       std::vector<AnatomyModuleHandle>& out_handles);

    /// platform_suffix() — Plattform-Suffix fuer Dateinamen.
    [[nodiscard]] static std::string platform_suffix();
};

}  // namespace comdare::cache_engine::builder::anatomy_loader
