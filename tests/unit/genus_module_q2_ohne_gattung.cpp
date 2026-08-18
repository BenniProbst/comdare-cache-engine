// Review #15 Fix 1 / E-1 (A2.5, 18.08.2026) -- das Modul OHNE comdare_anatomy_gattung: die einzige
// Konfiguration, in der status_gattung_symbol_missing (9) am ECHTEN dlopen-Ladeweg real ERZEUGT wird.
//
// Bis zu diesem Fixture prueften die Tests nur die NAME-Zuordnung des Codes 9 (status_name) --
// erzeugt hat ihn nie jemand. Ein Loader, der das Pflicht-Symbol 5 gar nicht mehr zoege, waere
// gruen geblieben.
//
// Mutations-Ehrlichkeit (wie genus_module_alt_major7.cpp): HEUTIGE Magic, HEUTIGE Version, echter
// SetAbiAdapter, und das SECHSTE Symbol (comdare_anatomy_genus) ist DA und antwortet korrekt.
// Es fehlt GENAU EINES -- comdare_anatomy_gattung. Fiele das Schloss weg, wuerde dieses Modul
// vollstaendig laden; die Ablehnung ist also eindeutig diesem einen Schloss zuzuordnen.

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

// comdare_anatomy_gattung existiert hier ABSICHTLICH NICHT -- das ist der gesamte Defekt.

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint8_t comdare_anatomy_genus() noexcept {
    return static_cast<std::uint8_t>(::comdare::cache_engine::anatomy::AnatomyGenus::Set);
}
