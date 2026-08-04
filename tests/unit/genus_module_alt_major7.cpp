// E-24 C10 (b) / G5 -- das ALT-MODUL als reproduzierbare Quelle: eine Set-Permutations-DLL, die ihre
// ABI-Identitaet auf den Stand VOR C8 stellt (Major 7, Magic ".A7.").
//
// WOZU ZWEI Alt-Module (dieses + genus_module_major7_neue_magic.cpp): der Loader traegt ZWEI Schloesser,
// und sie feuern in einer festen Reihenfolge (anatomy_module_loader.cpp: erst Magic, dann Version). Ein
// echtes Alt-Modul bewegt BEIDE (der Magic-Wert kodiert den Major, anatomy_module_abi_v1_decl.hpp:
// "Magic kodiert den Major -> von .A7. auf .A8. bewegt") -- es wird deshalb schon vom ERSTEN Schloss
// abgewiesen und erreicht das zweite nie. Wer nur diese eine Probe faehrt, hat den Major-Check NICHT
// bewiesen, sondern nur den Magic-Check. Deshalb steht daneben ein Modul, das die HEUTIGE Magic traegt
// und allein am Major scheitert.
//
// DIESE Datei ist die HERKUNFTS-TREUE Nachbildung der gesicherten G5-Fixture
// major7-referenz-44bcda99.so (gebaut vom ce-Stand 44bcda99, sha256 32b45bc0...0fd1c94e): dieselbe
// Wire-Identitaet (Version 7.0, Magic ".A7."). Sie existiert, weil die gesicherte .so ein BINAERES
// Beweismittel ausserhalb des Repos ist -- reproduzierbar im ctest ist nur eine committete Quelle.
// Der Lauf gegen die echte gesicherte Fixture bleibt der Beleg des Fensters (Report), diese Quelle ist
// die dauerhafte Wache.
//
// WICHTIG (Mutations-Ehrlichkeit): die Factory baut einen ECHTEN SetAbiAdapter, kein nullptr-Stub.
// Faellt eines der beiden Schloesser weg, WUERDE dieses Modul laden -- die Probe scheitert also
// ausschliesslich an der Versions-Wache und nicht daran, dass ohnehin nichts zu laden waere.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // Export-Makro + IAnatomyBase
#include <anatomy/set_abi_adapter.hpp>                     // SetAbiAdapter
#include <anatomy/set_anatomy.hpp>                         // SetAnatomy / SetComposition
#include <anatomy/set_default_organ.hpp>                   // SortedArrayKeySet

#include <cstdint>
#include <new>

namespace {

/// Der ABI-Stand VOR E-24 C8, hier bewusst als LITERAL und nicht ueber die Header-Konstante: die Datei
/// muss den ALTEN Wert behaupten koennen, waehrend der Host den neuen fuehrt. Genau diese Differenz IST
/// die Probe. (Historien-Beleg der Werte: anatomy_module_abi_v1_decl.hpp, Major-7-Absatz.)
inline constexpr std::uint64_t kAltMajor = 7;
inline constexpr std::uint64_t kAltMinor = 0;
inline constexpr std::uint64_t kAltMagic = 0x434F4D444141372EULL; // "COMDA*A7*"

using AltComposition =
    ::comdare::cache_engine::anatomy::SetComposition<::comdare::cache_engine::anatomy::SortedArrayKeySet, int, int, int,
                                                     int, int, int, int, int, int, int, int, int>;

} // namespace

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {
    return (kAltMajor << 32) | kAltMinor;
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_magic() noexcept { return kAltMagic; }

extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::anatomy::IAnatomyBase*
comdare_create_anatomy() noexcept {
    using AnatomyType = ::comdare::cache_engine::anatomy::SetAnatomy<AltComposition>;
    return new (::std::nothrow)::comdare::cache_engine::anatomy::SetAbiAdapter<AnatomyType>{};
}

extern "C" COMDARE_ANATOMY_ABI_EXPORT void
comdare_destroy_anatomy(::comdare::cache_engine::anatomy::IAnatomyBase* ptr) noexcept {
    delete ptr;
}
