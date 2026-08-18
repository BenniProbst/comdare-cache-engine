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
// M-1/D-2: das OPTIONALE 5. Symbol (comdare_anatomy_version_lines). NICHT Loader-Pflicht -- fehlt es, bleibt
// der Handle-Zeiger nullptr und die Ladung gilt weiterhin als erfolgreich. Die Entscheidung, ob eine Binary
// OHNE Deklaration messfaehig ist, faellt NICHT hier (der Loader ist ein reiner dlopen-Wrapper), sondern am
// Pruefdock (mess_konsistenz_gate.hpp) -- dort fail-closed.
using PfnVersionLines = abi::AnatomyVersionLines const* (*)();
// Q2/V-06: die zwei Identitaets-Symbole liefern die uint8-Enum-WERTE, nicht die Enum-Typen -- ueber die
// C-ABI-Grenze reist ein Zahlentyp, die Bedeutung liegt im Header, den beide Seiten teilen.
using PfnGattung = std::uint8_t (*)();
using PfnGenus   = std::uint8_t (*)();

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleHandle::unload — RAII-Cleanup in korrekter Reihenfolge
// ─────────────────────────────────────────────────────────────────────────────

void AnatomyModuleHandle::unload() noexcept {
    // 1. Anatomie-Pointer ZUERST freigeben (Pflicht: gleiche .so/.dll-Heap)
    if (anatomy_ && destroy_) { destroy_(anatomy_); }
    anatomy_ = nullptr;
    destroy_ = nullptr;
    // M-1/D-2: der Stempel-POD lebt IM Modul (static constexpr der Makro-Materialisierung). Nach dem
    // dlclose unten waere der Zeiger baumelnd -- er wird deshalb VOR dem Entladen genullt, nicht danach.
    version_lines_ = nullptr;

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

int AnatomyModuleLoader::load(std::filesystem::path const& dll_path, AnatomyModuleHandle& handle_out) noexcept {
    std::error_code ec;
    if (!std::filesystem::exists(dll_path, ec)) { return status_file_not_found; }

    void* native = native_load(dll_path);
    if (!native) { return status_load_failed; }

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

    // Version-Check ueber den EINEN benannten Vertrag (V6/P3, K-6 2026-07-19): das Gate ist
    // AnatomyAbiVersion::host_compatible_with (anatomy_module_abi_v1_decl.hpp:305-308, Major identisch +
    // Modul-Minor <= Host-Minor) statt einer Inline-Reimplementation. Die beiden differenzierten
    // Status-Codes bleiben als reine DIAGNOSE des abgelehnten Falls erhalten (Verhalten byte-identisch).
    auto const module_version = abi::AnatomyAbiVersion::unpack(pfn_version());

    if (!abi::kHostAnatomyAbiVersion.host_compatible_with(module_version)) {
        native_unload(native);
        return (module_version.major != abi::kHostAnatomyAbiVersion.major) ? status_abi_major_mismatch
                                                                           : status_abi_minor_too_new;
    }

    // Q2/V-06 (18.08.2026) -- DIE ZWEI IDENTITAETS-SYMBOLE, PFLICHT, an genau dieser Stelle.
    // WARUM NACH MAGIC UND VERSION: erst wenn Magic und Major/Minor passen, ist dieses Modul
    // ueberhaupt als unseres lesbar. Zoege der Loader die neuen Symbole VOR diesen Pruefungen, bekaeme
    // ein echtes Alt-Modul ein Symbol-Fehlerbild statt seines Magic-/Major-Fehlerbildes -- die beiden
    // Alt-Major-Fixtures (genus_module_alt_major7, genus_module_major7_neue_magic) pruefen genau diese
    // beiden Schloesser, und sie muessen sie weiter EINZELN treffen.
    // WARUM VOR DER FACTORY: hier ist noch kein Objekt gebaut. Nach pfn_create() muesste jeder
    // Fehlerpfad das Objekt erst wieder zerstoeren -- ein Ausstieg, den man genau einmal vergisst.
    // Und fachlich ist es der Punkt: diese beiden Symbole beantworten "was BIST du", und das gehoert
    // VOR "gib mir eine Instanz", nicht danach.
    auto* sym_gattung = native_symbol(native, "comdare_anatomy_gattung");
    if (!sym_gattung) {
        native_unload(native);
        return status_gattung_symbol_missing;
    }
    auto* sym_genus = native_symbol(native, "comdare_anatomy_genus");
    if (!sym_genus) {
        native_unload(native);
        return status_genus_symbol_missing;
    }
    auto const modul_gattung = static_cast<ana::AnatomyGattung>(reinterpret_cast<PfnGattung>(sym_gattung)());
    auto const modul_genus   = static_cast<ana::AnatomyGenus>(reinterpret_cast<PfnGenus>(sym_genus)());

    // Factory aufrufen
    ana::IAnatomyBase* anatomy = pfn_create();
    if (!anatomy) {
        native_unload(native);
        return status_factory_returned_null;
    }

    // Q2/V-06 -- DER KONSISTENZ-RIEGEL, am GELADENEN Objekt und nicht am Symbolnamen.
    // Das Modul hat zweimal geantwortet: einmal statisch ueber comdare_anatomy_genus (VOR der Factory)
    // und einmal dynamisch ueber die Instanz, die seine Factory geliefert hat. Beide Antworten muessen
    // dieselbe sein -- sonst traegt das Binary eine andere Identitaet, als es ausweist, und genau diese
    // Luege wandert ungeprueft in Stempel, Lagerpfad und Messreihe.
    // Die GATTUNG wird nicht verglichen, sondern GEPRUEFT: sie ist per gattung_of aus dem Genus
    // ableitbar, also ist jede Abweichung ein Widerspruch in sich und keine zweite Meinung.
    // Beim HYBRID melden beide Seiten den ZIEL-Genus (Weg C, nie FunctionInterfaceReroute) -- der
    // Riegel gilt fuer ihn unveraendert, ohne Sonderfall.
    if (anatomy->genus() != modul_genus || ana::gattung_of(modul_genus) != modul_gattung) {
        std::cerr << "[anatomy_loader] identity_mismatch: Symbol comdare_anatomy_genus meldet "
                  << static_cast<int>(modul_genus) << ", die Instanz meldet "
                  << static_cast<int>(anatomy->genus()) << "; Symbol comdare_anatomy_gattung meldet "
                  << static_cast<int>(modul_gattung) << ", gattung_of(genus) ergibt "
                  << static_cast<int>(ana::gattung_of(modul_genus)) << " (" << dll_path.string() << ")\n";
        pfn_destroy(anatomy);
        native_unload(native);
        return status_identity_mismatch;
    }

    // M-1/D-2: das OPTIONALE Stempel-Symbol NACH der Version-Validierung ziehen. Reihenfolge ist tragend:
    // erst wenn Magic + Major/Minor passen, ist das POD-Layout dieses Moduls ueberhaupt als das unsere
    // lesbar. Ein Fehlen ist KEIN Lade-Fehler (der Loader kennt nur 4 Pflicht-Symbole) -- der Zeiger bleibt
    // dann nullptr, und das Mess-Konsistenz-Gate am Pruefdock entscheidet fail-closed darueber.
    abi::AnatomyVersionLines const* lines = nullptr;
    if (auto* sym_lines = native_symbol(native, "comdare_anatomy_version_lines"); sym_lines != nullptr)
        lines = reinterpret_cast<PfnVersionLines>(sym_lines)();

    // Handle aufbauen (RAII)
    handle_out = AnatomyModuleHandle{native, anatomy, pfn_destroy, module_version, lines};
    return status_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// AnatomyModuleLoader::load_all — Bulk-Load aus Directory
// ─────────────────────────────────────────────────────────────────────────────

int AnatomyModuleLoader::load_all(std::filesystem::path const& dir, std::vector<AnatomyModuleHandle>& out_handles) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) { return status_file_not_found; }

    auto const                         suffix = platform_suffix();
    std::vector<std::filesystem::path> candidates;
    for (auto const& entry : std::filesystem::directory_iterator{dir, ec}) {
        if (!entry.is_regular_file()) continue;
        auto const& p  = entry.path();
        auto const  fn = p.filename().string();
        // Pattern aus anatomy_codegen.cmake: comdare_anatomy_perm_<fingerprint>.so/.dll
        if (fn.find("comdare_anatomy_perm_") != 0) continue;
        if (p.extension() != suffix) continue;
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
        int const           s = load(p, h);
        if (s == status_ok) {
            out_handles.push_back(std::move(h));
        } else {
            std::cerr << "AnatomyModuleLoader: failed to load " << p.string() << " (status=" << s << " "
                      << status_name(s) << ")\n";
            last_status = s;
        }
    }
    return last_status;
}

} // namespace comdare::cache_engine::builder::anatomy_loader
