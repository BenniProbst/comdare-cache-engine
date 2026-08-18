// Review #15 Fix 1 / E-1 (A2.5, 18.08.2026) -- das Modul OHNE comdare_anatomy_genus: die einzige
// Konfiguration, in der status_genus_symbol_missing (10) am ECHTEN dlopen-Ladeweg real ERZEUGT wird.
//
// Gegenstueck zu genus_module_q2_ohne_gattung.cpp, gespiegelt: das FUENFTE Symbol
// (comdare_anatomy_gattung) ist DA und antwortet korrekt, das SECHSTE fehlt. Zusammen pinnen die
// beiden Fixtures, dass die zwei Codes 9/10 EINZELN beobachtbar sind und der Loader die Symbole in
// der dokumentierten Reihenfolge zieht (erst gattung, dann genus) -- ein Sammel-Code oder eine
// vertauschte Zuordnung wuerde genau hier rot.
//
// Mutations-Ehrlichkeit: HEUTIGE Magic, HEUTIGE Version, echter SetAbiAdapter. Es fehlt GENAU
// EINES -- fiele das Schloss weg, wuerde dieses Modul vollstaendig laden.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // Export-Makro + Magic/Major/Minor + IAnatomyBase
#include <anatomy/set_abi_adapter.hpp>                     // SetAbiAdapter
#include <anatomy/set_anatomy.hpp>                         // SetAnatomy / SetComposition
#include <anatomy/set_default_organ.hpp>                   // SortedArrayKeySet

#include <cstdint>
#include <new>

namespace {

using ProbeComposition =
    ::comdare::cache_engine::anatomy::SetComposition<::comdare::cache_engine::anatomy::SortedArrayKeySet, int, int, int,
                                                     int, int, int, int, int, int, int, int, int>;

} // namespace

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {
    return (static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) << 32) |
           static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MINOR);
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept {
    return COMDARE_ANATOMY_ABI_MAGIC;
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*
comdare_create_anatomy() noexcept {
    using AnatomyType = ::comdare::cache_engine::anatomy::SetAnatomy<ProbeComposition>;
    return new (::std::nothrow)::comdare::cache_engine::anatomy::SetAbiAdapter<AnatomyType>{};
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT void
comdare_destroy_anatomy(::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {
    delete ptr;
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint8_t comdare_anatomy_gattung() noexcept {
    return static_cast<std::uint8_t>(::comdare::cache_engine::anatomy::AnatomyGattung::Container);
}

// comdare_anatomy_genus existiert hier ABSICHTLICH NICHT -- das ist der gesamte Defekt.
