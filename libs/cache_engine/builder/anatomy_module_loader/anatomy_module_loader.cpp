// V41.F.6.1.R5.E — AnatomyModuleLoader Plattform-Implementation
//
// Plattform-spezifischer dlopen/LoadLibrary-Code + Version-Check.
//
// @task #708 V41.F.6.1.R5.E

#include "anatomy_module_loader.hpp"

// Review #15 Fix 2: die Wertklassen-Gates brauchen die Partition der Hybrid-Klassifikation
// (ist_abi_sichtbares_genus). Die Kante builder/ -> hybrid/ ist die ERLAUBTE Richtung -- Praezedenz
// ist der HY-A3-Include in genus_build_admission.hpp; die Schicht-Wache (lint_layer_includes.sh)
// verbietet nur die Gegenrichtung hybrid/ -> builder/.
#include "hybrid/heuristik_adapter_klassifikation.hpp"

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

// Funktion-Pointer-Typen der SECHS Pflicht-extern-"C"-Symbole aus anatomy_module_abi_v1.hpp:
// die vier Ur-Pflicht-Typen hier, die zwei Identitaets-Symbole (PfnGattung/PfnGenus) weiter unten,
// dazwischen der Typ des OPTIONALEN Stempel-Symbols.
using PfnAbiVersion = std::uint64_t (*)();
using PfnAbiMagic   = std::uint64_t (*)();
using PfnCreate     = ana::IAnatomyBase* (*)();
using PfnDestroy    = void (*)(ana::IAnatomyBase*);
// A-11/golden-102: das SIEBTE PFLICHT-Symbol (comdare_anatomy_version_lines). Bis 19.08.2026 war es
// optional ("fehlt es, bleibt der Handle-Zeiger nullptr und die Ladung gilt weiterhin als erfolgreich");
// seit A-11 wird ein Modul ohne Stempel-Symbol -- oder mit nullptr-Antwort -- am dlopen-Weg abgewiesen
// (status_version_lines_symbol_missing). Das Pruefdock (mess_konsistenz_gate.hpp) bleibt als
// Tiefenverteidigung fuer direkt konstruierte Handles bestehen.
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

    // A-F4 (Review #15): SCOPE-GUARD AB ERWERB. Jeder Pfad ab hier gibt Handle und -- sobald die
    // Factory gelaufen ist -- Instanz ueber GENAU EINE Stelle frei, destroy VOR unload (der
    // Handle-Vertrag). Vorher trug jeder Fehlerpfad sein eigenes native_unload, und der Riegel-Pfad
    // loggte VOR dem Cleanup: dll_path.string() kann werfen (bad_alloc), und ein Wurf zwischen
    // Erwerb und Freigabe haette Instanz+Handle gehalten (unter noexcept: terminate MIT gehaltenen
    // Ressourcen). Diagnose gehoert deshalb NACH das Cleanup -- jetzt strukturell erzwungen.
    struct ErwerbsGuard {
        void*              native  = nullptr;
        ana::IAnatomyBase* anatomy = nullptr;
        PfnDestroy         destroy = nullptr;
        void               jetzt_freigeben() noexcept {
            if (anatomy != nullptr && destroy != nullptr) { destroy(anatomy); }
            anatomy = nullptr;
            destroy = nullptr;
            if (native != nullptr) {
                native_unload(native);
                native = nullptr;
            }
        }
        void an_handle_uebergeben() noexcept {
            native  = nullptr;
            anatomy = nullptr;
            destroy = nullptr;
        }
        ~ErwerbsGuard() { jetzt_freigeben(); }
    };
    ErwerbsGuard guard{native};

    // Die vier Ur-Pflicht-Symbole resolven (die Pflicht-Symbole 5+6 -- gattung/genus -- folgen erst
    // NACH Magic/Version, damit Alt-Module ihr altes Fehlerbild behalten; Schrittliste im Header).
    auto* sym_version = native_symbol(native, "comdare_anatomy_abi_version");
    auto* sym_magic   = native_symbol(native, "comdare_anatomy_abi_magic");
    auto* sym_create  = native_symbol(native, "comdare_create_anatomy");
    auto* sym_destroy = native_symbol(native, "comdare_destroy_anatomy");

    if (!sym_version || !sym_magic || !sym_create || !sym_destroy) { return status_symbol_not_found; }

    auto pfn_version = reinterpret_cast<PfnAbiVersion>(sym_version);
    auto pfn_magic   = reinterpret_cast<PfnAbiMagic>(sym_magic);
    auto pfn_create  = reinterpret_cast<PfnCreate>(sym_create);
    auto pfn_destroy = reinterpret_cast<PfnDestroy>(sym_destroy);

    // Magic-Check (Pre-Version: sicherer Sanity-Check vor Version-Vergleich)
    if (pfn_magic() != COMDARE_ANATOMY_ABI_MAGIC) { return status_magic_mismatch; }

    // Version-Check ueber den EINEN benannten Vertrag (V6/P3, K-6 2026-07-19): das Gate ist
    // AnatomyAbiVersion::host_compatible_with (anatomy_module_abi_v1_decl.hpp:305-308, Major identisch +
    // Modul-Minor <= Host-Minor) statt einer Inline-Reimplementation. Die beiden differenzierten
    // Status-Codes bleiben als reine DIAGNOSE des abgelehnten Falls erhalten (Verhalten byte-identisch).
    auto const module_version = abi::AnatomyAbiVersion::unpack(pfn_version());

    if (!abi::kHostAnatomyAbiVersion.host_compatible_with(module_version)) {
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
    // Fehlerpfad das Objekt erst wieder zerstoeren -- ein Ausstieg, den man genau einmal vergisst
    // (seit A-F4 haelt der ErwerbsGuard oben zusaetzlich JEDEN Pfad dicht; die Reihenfolge hier
    // bleibt trotzdem die fachlich richtige).
    // Und fachlich ist es der Punkt: diese beiden Symbole beantworten "was BIST du", und das gehoert
    // VOR "gib mir eine Instanz", nicht danach.
    auto* sym_gattung = native_symbol(native, "comdare_anatomy_gattung");
    if (!sym_gattung) { return status_gattung_symbol_missing; }
    auto* sym_genus = native_symbol(native, "comdare_anatomy_genus");
    if (!sym_genus) { return status_genus_symbol_missing; }
    std::uint8_t const roh_gattung = reinterpret_cast<PfnGattung>(sym_gattung)();
    std::uint8_t const roh_genus   = reinterpret_cast<PfnGenus>(sym_genus)();

    // Review #15 Fix 2 -- ZWEI WERTKLASSEN-GATES am ROHEN Byte, fail-closed, VOR der Factory.
    // Eines allein genuegt NICHT (am Objekt verifiziert): gattung_of() defaultet unbekannte Bytes
    // still auf Container, also ist ist_abi_sichtbares_genus(250) true -- und genus_bekannt(5) ist
    // true, denn der Reroute-Wert IST ein Enum-Wert. Der Konsistenz-Riegel unten prueft nur, dass
    // Symbol, Ableitung und Instanz EINIG sind, nie, ob ihr gemeinsamer Wert ZULAESSIG ist:
    //   (a) genus_bekannt weist jedes Nicht-Enum-Byte ab (z.B. 250), BEVOR es je in einen Enum-Wert
    //       gegossen wird oder gattung_of() erreicht;
    //   (b) ist_abi_sichtbares_genus weist die verbotene 5 ab (FunctionInterfaceReroute, "NIE von
    //       genus()", Weg C) -- ein Modul, das sich konsistent als Reroute ausweist, ist genau die
    //       Luege, die der Hybrid-Schnitt verbietet.
    if (!ana::genus_bekannt(roh_genus) ||
        !::comdare::cache_engine::hybrid::ist_abi_sichtbares_genus(static_cast<ana::AnatomyGenus>(roh_genus))) {
        // A-F4-Ordnung auch hier: erst freigeben (der Guard haelt nur das Handle -- kein Objekt
        // gebaut), DANN die Diagnose mit dem werfen-koennenden dll_path.string().
        guard.jetzt_freigeben();
        std::cerr << "[anatomy_loader] genus_not_abi_visible: comdare_anatomy_genus meldet "
                  << static_cast<int>(roh_genus)
                  << " -- kein bekanntes ABI-sichtbares Genus (bekannt 0..5, 5 ist Klassifikation) ("
                  << dll_path.string() << ")\n";
        return status_genus_not_abi_visible;
    }
    auto const modul_genus   = static_cast<ana::AnatomyGenus>(roh_genus);
    auto const modul_gattung = static_cast<ana::AnatomyGattung>(roh_gattung);

    // Factory aufrufen -- ab jetzt haelt der Guard AUCH die Instanz (destroy VOR unload).
    ana::IAnatomyBase* anatomy = pfn_create();
    if (!anatomy) { return status_factory_returned_null; }
    guard.anatomy = anatomy;
    guard.destroy = pfn_destroy;

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
        // A-F4: erst die Diagnose-Werte RETTEN (die Instanz stirbt gleich), dann Cleanup ueber den
        // Guard (destroy VOR unload), und ERST DANACH loggen -- dll_path.string() kann werfen und
        // darf das erst tun, wenn nichts mehr gehalten wird.
        int const instanz_genus = static_cast<int>(anatomy->genus());
        guard.jetzt_freigeben();
        std::cerr << "[anatomy_loader] identity_mismatch: Symbol comdare_anatomy_genus meldet "
                  << static_cast<int>(modul_genus) << ", die Instanz meldet " << instanz_genus
                  << "; Symbol comdare_anatomy_gattung meldet " << static_cast<int>(modul_gattung)
                  << ", gattung_of(genus) ergibt " << static_cast<int>(ana::gattung_of(modul_genus)) << " ("
                  << dll_path.string() << ")\n";
        return status_identity_mismatch;
    }

    // A-11/golden-102: das SIEBTE PFLICHT-Symbol, an der HISTORISCHEN Pull-Position (NACH Magic/Version/
    // gattung/genus/Factory/Identitaets-Riegel). Die Position ist tragend: erst wenn Magic + Major/Minor
    // passen, ist das POD-Layout dieses Moduls ueberhaupt als das unsere lesbar -- und die fruehen
    // Schloesser behalten ihr Fehlerbild (alt_major7/neue_magic/ohne_gattung/ohne_genus/luegner/
    // alt_magic_ohne_symbole treffen weiter IHRE Codes 4/5/9/10/11). Nur ein bis hierher fehlerfreies,
    // stempelloses Modul faellt neu auf 13: ein Modul, das nichts deklariert, wird ABGEWIESEN statt als
    // erfolgreiche Ladung mit nullptr-Handle weiterzureisen. Beide nullptr-Wege (Symbol fehlt; Symbol
    // liefert nullptr) sind derselbe Fall "deklariert nichts" -- damit ist version_lines() ab status_ok
    // garantiert non-null. Das Mess-Konsistenz-Gate bleibt Tiefenverteidigung fuer Direkt-Handles.
    auto* const sym_lines = native_symbol(native, "comdare_anatomy_version_lines");
    if (sym_lines == nullptr) { return status_version_lines_symbol_missing; }
    abi::AnatomyVersionLines const* lines = reinterpret_cast<PfnVersionLines>(sym_lines)();
    if (lines == nullptr) { return status_version_lines_symbol_missing; }

    // Handle aufbauen (RAII) -- das Eigentum wandert vom Guard in die Handle (beides noexcept).
    handle_out = AnatomyModuleHandle{native, anatomy, pfn_destroy, module_version, lines};
    guard.an_handle_uebergeben();
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
