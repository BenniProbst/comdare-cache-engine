// Review #15 Fix 3 (A2.5, 18.08.2026) -- die DRITTE Alt-Fixture: Alt-Magic (.A7.-Muster) und NUR die
// vier alten Symbole, KEINE Identitaets-Symbole. Erwartung: status_magic_mismatch, NICHT
// status_gattung_symbol_missing.
//
// WAS SIE PINNT, was die beiden bestehenden Alt-Fixtures NICHT pinnen koennen: die dokumentierte
// Loader-REIHENFOLGE "Identitaets-Symbole erst NACH Magic/Version" (anatomy_module_loader.hpp,
// Schrittliste 4-8). genus_module_alt_major7.cpp TRAEGT die zwei neuen Symbole absichtlich -- an ihm
// ist die Reihenfolge deshalb unbeobachtbar (jede Ordnung ergaebe magic_mismatch). Erst ein Modul,
// dem BEIDE fehlen UND dessen Magic alt ist, unterscheidet die Ordnungen: zieht der Loader die
// Symbole zu frueh, antwortet er 9 (gattung_symbol_missing) statt 4 (magic_mismatch).
//
// DAS IST DER BENANNTE REALFALL: ein Modul, gebaut gegen die ABI VOR Q2/V-06, traegt die zwei neuen
// Symbole schlicht nicht. Seine Diagnose muss die ALTE bleiben ("deine ABI ist zu alt"), nicht die
// neue ("dir fehlt ein Symbol") -- sonst schickte der Umbau jeden Alt-Modul-Autor auf die falsche
// Faehrte. Die zwei bestehenden Alt-Fixtures belegen, dass die Symbol-Pflicht die alten Fehlerbilder
// nicht verdraengt WENN die Symbole da sind; diese dritte belegt es fuer den Fall, dass sie FEHLEN.
//
// Mutations-Ehrlichkeit (wie genus_module_alt_major7.cpp): die Factory baut einen ECHTEN
// SetAbiAdapter. Werte-Herkunft der Alt-Identitaet: anatomy_module_abi_v1_decl.hpp, Major-7-Absatz
// (dieselben Literale wie in genus_module_alt_major7.cpp -- die Datei muss den ALTEN Wert behaupten
// koennen, waehrend der Host den neuen fuehrt).

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // Export-Makro + IAnatomyBase
#include <anatomy/set_abi_adapter.hpp>                     // SetAbiAdapter
#include <anatomy/set_anatomy.hpp>                         // SetAnatomy / SetComposition
#include <anatomy/set_default_organ.hpp>                   // SortedArrayKeySet

#include <cstdint>
#include <new>

namespace {

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

// comdare_anatomy_gattung und comdare_anatomy_genus existieren hier ABSICHTLICH NICHT: dieses Modul
// stellt den Stand VOR Q2/V-06 dar -- ein Alt-Modul KANN die zwei Symbole nicht tragen.
