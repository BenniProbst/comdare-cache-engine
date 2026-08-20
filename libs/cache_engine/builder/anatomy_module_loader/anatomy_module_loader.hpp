#pragma once
// V41.F.6.1.R5.E — AnatomyModuleLoader (dlopen/LoadLibrary fuer Permutations-Binary)
//
// User-Direktive 2026-05-27 (Doku 14 §44.7 + §45.7):
// R5.E ModuleLoader::load_anatomy(dll_path) -> IAnatomyBase* mit
// dlopen/LoadLibrary + ABI-Version-Check.
//
// Aufgabe: laedt eine Anatomy-Permutations-.so/.dll die per
// COMDARE_DEFINE_ANATOMY_MODULE generiert wurde, resolved die SECHS Pflicht-Symbole
// (comdare_anatomy_abi_version/magic/create_anatomy/destroy_anatomy und -- seit Q2/V-06 -- die
// zwei Identitaets-Symbole comdare_anatomy_gattung/comdare_anatomy_genus), prueft
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

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // R6 Ink.2b: leichte ABI-Schnittstelle (entkoppelt von abi_adapter.hpp)

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::anatomy_loader {

// ─────────────────────────────────────────────────────────────────────────────
// errno-style Status (analog comdare::builder::loader)
// ─────────────────────────────────────────────────────────────────────────────

inline constexpr int status_ok                    = 0;
inline constexpr int status_file_not_found        = 1;
inline constexpr int status_load_failed           = 2;
inline constexpr int status_symbol_not_found      = 3;
inline constexpr int status_magic_mismatch        = 4;
inline constexpr int status_abi_major_mismatch    = 5;
inline constexpr int status_abi_minor_too_new     = 6;
inline constexpr int status_factory_returned_null = 7;
inline constexpr int status_null_module           = 8;
// Q2/V-06 (18.08.2026) -- die zwei IDENTITAETS-SYMBOLE sind PFLICHT. ZWEI Codes statt einem, aus
// demselben Grund, aus dem es zwei Alt-Major-Fixtures gibt: jedes Schloss muss EINZELN beobachtbar
// sein. Ein Sammel-Code "identitaet_fehlt" haette gesagt, DASS etwas fehlt, nicht WAS -- und genau
// diese Auskunft braucht der, der ein Modul baut und es nicht geladen bekommt.
inline constexpr int status_gattung_symbol_missing = 9;
inline constexpr int status_genus_symbol_missing   = 10;
// Der KONSISTENZ-RIEGEL. Ein Modul, dessen Symbol-Wert nicht zu der Instanz passt, die seine eigene
// Factory liefert, ist WIDERSPRUECHLICH -- und ein Widerspruch, den der Loader durchlaesst, wandert
// als falsche Identitaet in Stempel, Lager und Messreihe. Deshalb fail-closed hier, nicht spaeter.
inline constexpr int status_identity_mismatch = 11;
// Review #15 Fix 2 (18.08.2026) -- die WERTKLASSEN-Haelfte des Riegels, VOR der Factory: das
// genus-Symbol darf nur ein BEKANNTES (anatomy_base.hpp genus_bekannt) UND ABI-SICHTBARES Genus
// (hybrid::ist_abi_sichtbares_genus -- nie FunctionInterfaceReroute, Weg C) melden. Ohne dieses
// Gate passierte eine in sich KONSISTENTE Luege (Symbol, gattung_of und Instanz einig auf 250 oder
// auf 5) den Konsistenz-Riegel unten: der prueft nur, dass zwei Antworten GLEICH sind, nie, dass
// ihr Wert ZULAESSIG ist. Eigener Code aus demselben Grund wie bei 9/10: die Diagnose muss sagen,
// WAS abgelehnt wurde, nicht nur DASS.
inline constexpr int status_genus_not_abi_visible = 12;
// A-11/golden-102 (19.08.2026) -- DIE STEMPEL-PFLICHT. comdare_anatomy_version_lines ist das SIEBTE
// PFLICHT-Symbol ("aus sechs werden sieben"): ein Modul ohne Stempel-Symbol -- oder mit einem Symbol,
// das nullptr liefert -- deklariert NICHTS und wird am dlopen-Weg abgewiesen, statt als erfolgreiche
// Ladung mit nullptr-Handle weiterzureisen. Position der Pruefung: NACH Magic/Version/gattung/genus/
// Factory/Identitaets-Riegel (die fruehen Schloesser behalten ihr Fehlerbild -- Alt-Module treffen
// weiter IHRE Codes 4/5/9/10/11); nur bis dahin fehlerfreie, stempellose Module fallen neu auf 13.
// Eigener Code aus demselben Grund wie bei 9/10: die Diagnose muss sagen, WAS fehlt.
inline constexpr int status_version_lines_symbol_missing = 13;

[[nodiscard]] constexpr const char* status_name(int s) noexcept {
    switch (s) {
        case status_ok: return "ok";
        case status_file_not_found: return "file_not_found";
        case status_load_failed: return "load_failed";
        case status_symbol_not_found: return "symbol_not_found";
        case status_magic_mismatch: return "magic_mismatch";
        case status_abi_major_mismatch: return "abi_major_mismatch";
        case status_abi_minor_too_new: return "abi_minor_too_new";
        case status_factory_returned_null: return "factory_returned_null";
        case status_null_module: return "null_module";
        case status_gattung_symbol_missing: return "gattung_symbol_missing";
        case status_genus_symbol_missing: return "genus_symbol_missing";
        case status_identity_mismatch: return "identity_mismatch";
        case status_genus_not_abi_visible: return "genus_not_abi_visible";
        case status_version_lines_symbol_missing: return "version_lines_symbol_missing";
        default: return "unknown";
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

    AnatomyModuleHandle(void* native_handle, ::comdare::cache_engine::anatomy::IAnatomyBase* anatomy_ptr,
                        void (*destroy_fn)(::comdare::cache_engine::anatomy::IAnatomyBase*),
                        ::comdare::cache_engine::abi::AnatomyAbiVersion          module_version,
                        ::comdare::cache_engine::abi::AnatomyVersionLines const* version_lines = nullptr) noexcept
        : native_{native_handle}, anatomy_{anatomy_ptr}, destroy_{destroy_fn}, module_version_{module_version},
          version_lines_{version_lines} {}

    // Move-only
    AnatomyModuleHandle(AnatomyModuleHandle const&)            = delete;
    AnatomyModuleHandle& operator=(AnatomyModuleHandle const&) = delete;

    AnatomyModuleHandle(AnatomyModuleHandle&& other) noexcept
        : native_{other.native_}, anatomy_{other.anatomy_}, destroy_{other.destroy_},
          module_version_{other.module_version_}, version_lines_{other.version_lines_} {
        other.native_        = nullptr;
        other.anatomy_       = nullptr;
        other.destroy_       = nullptr;
        other.version_lines_ = nullptr;
    }

    AnatomyModuleHandle& operator=(AnatomyModuleHandle&& other) noexcept {
        if (this != &other) {
            unload();
            native_              = other.native_;
            anatomy_             = other.anatomy_;
            destroy_             = other.destroy_;
            module_version_      = other.module_version_;
            version_lines_       = other.version_lines_;
            other.native_        = nullptr;
            other.anatomy_       = nullptr;
            other.destroy_       = nullptr;
            other.version_lines_ = nullptr;
        }
        return *this;
    }

    ~AnatomyModuleHandle() { unload(); }

    [[nodiscard]] bool valid() const noexcept { return native_ != nullptr && anatomy_ != nullptr; }

    /// anatomy() — Zugriff auf die geladene IAnatomyBase-Instanz.
    /// Lebenszeit: bis Destruktor / unload() / move-assignment.
    [[nodiscard]] ::comdare::cache_engine::anatomy::IAnatomyBase*       anatomy() noexcept { return anatomy_; }
    [[nodiscard]] ::comdare::cache_engine::anatomy::IAnatomyBase const* anatomy() const noexcept { return anatomy_; }

    /// module_version() — ABI-Version der geladenen .so/.dll.
    [[nodiscard]] ::comdare::cache_engine::abi::AnatomyAbiVersion module_version() const noexcept {
        return module_version_;
    }

    /// version_lines() -- M-1/D-2 (06.08.2026): die vom geladenen Modul DEKLARIERTEN Stempel-Zeilen
    /// (Organ/System/Mess + Fingerprint + die drei Entry-Arrays).
    ///
    /// A-11/golden-102 (19.08.2026): AB status_ok GARANTIERT NON-NULL. Der Loader weist ein Modul ohne
    /// Stempel-Symbol -- oder mit nullptr-liefernder Antwort -- mit status_version_lines_symbol_missing
    /// (13) ab; eine erfolgreiche Ladung traegt hier also immer einen gueltigen POD. nullptr steht nur
    /// noch an einer default-konstruierten oder entladenen Handle.
    ///
    /// WARUM DAS KEIN ABI-SCHRITT IST: gelesen wird ausschliesslich das bereits bestehende
    /// Probe-Symbol comdare_anatomy_version_lines (anatomy_module_abi_v1_decl.hpp) und das bereits
    /// bestehende POD-Layout 7. Der Loader verlangt die sechs bisherigen Pflicht-Symbole plus (seit
    /// A-11) dieses SIEBTE -- die EMITTER-Aritaet der DEFINE-Makros ist unberuehrt (der Stempel ist
    /// ein ZUSAETZLICHER Makro-Call am Emissions-Ort, kein Makro-Umbau). COMDARE_ANATOMY_ABI_MAJOR
    /// und kAnatomyVersionLinesLayout bleiben unberuehrt.
    ///
    /// LEBENSZEIT: der POD ist im MODUL ein `static constexpr` (Makro-Materialisierung) -- er lebt, solange
    /// das dlopen-Handle lebt, also genau so lange wie diese Handle. Nach unload() ist der Zeiger tot;
    /// deshalb wird er dort mit genullt.
    [[nodiscard]] ::comdare::cache_engine::abi::AnatomyVersionLines const* version_lines() const noexcept {
        return version_lines_;
    }

    /// unload() — explizite Freigabe (Destruktor ruft das auch).
    void unload() noexcept;

private:
    void*                                           native_           = nullptr;
    ::comdare::cache_engine::anatomy::IAnatomyBase* anatomy_          = nullptr;
    void (*destroy_)(::comdare::cache_engine::anatomy::IAnatomyBase*) = nullptr;
    ::comdare::cache_engine::abi::AnatomyAbiVersion module_version_   = {0, 0};
    // M-1/D-2: zeigt in den Adressraum des geladenen Moduls -> MUSS in unload() mit genullt werden.
    ::comdare::cache_engine::abi::AnatomyVersionLines const* version_lines_ = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleLoader — load/load_all + Version-Check
// ─────────────────────────────────────────────────────────────────────────────

class AnatomyModuleLoader {
public:
    /// load() — Loadet `dll_path`. Bei Erfolg: status_ok, handle_out gesetzt.
    /// Bei Misserfolg: errno-style status, handle_out unveraendert.
    ///
    /// Validierungs-Schritte (in dieser Reihenfolge; seit A-11/golden-102 sind SIEBEN Symbole Pflicht):
    /// 1. Datei existiert
    /// 2. dlopen/LoadLibrary erfolgreich
    /// 3. Die vier Ur-Pflicht-Symbole resolvable (abi_version/abi_magic/create/destroy)
    /// 4. Magic-Number == COMDARE_ANATOMY_ABI_MAGIC
    /// 5. Major-Version match (Host vs Modul)
    /// 6. Modul-Minor <= Host-Minor
    /// 7. comdare_anatomy_gattung resolvable (Pflicht-Symbol 5; NACH Magic/Version, damit
    ///    Alt-Module ihr altes Fehlerbild behalten)
    /// 8. comdare_anatomy_genus resolvable (Pflicht-Symbol 6)
    /// 9. Wertklassen-Gates am rohen genus-Byte, VOR der Factory (Review #15 Fix 2):
    ///    genus_bekannt UND hybrid::ist_abi_sichtbares_genus -- sonst status_genus_not_abi_visible
    /// 10. Factory comdare_create_anatomy() liefert non-null
    /// 11. Identitaets-Riegel: Instanz-genus() == Symbol-Wert UND Gattungs-Symbol ==
    ///     gattung_of(genus) -- sonst status_identity_mismatch
    /// 12. STEMPEL-PFLICHT (A-11): comdare_anatomy_version_lines resolvable UND non-null (Pflicht-
    ///     Symbol 7; an der HISTORISCHEN Pull-Position, also NACH allen fruehen Schloessern) --
    ///     sonst status_version_lines_symbol_missing
    [[nodiscard]] static int load(std::filesystem::path const& dll_path, AnatomyModuleHandle& handle_out) noexcept;

    /// load_all() — Loadet alle anatomy-Pilot-DLLs in einem Verzeichnis.
    /// Pattern: comdare_anatomy_perm_*.so/.dll/.dylib.
    [[nodiscard]] static int load_all(std::filesystem::path const& dir, std::vector<AnatomyModuleHandle>& out_handles);

    /// platform_suffix() — Plattform-Suffix fuer Dateinamen.
    [[nodiscard]] static std::string platform_suffix();
};

} // namespace comdare::cache_engine::builder::anatomy_loader
