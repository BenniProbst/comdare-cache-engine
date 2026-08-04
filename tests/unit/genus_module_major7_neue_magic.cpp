// E-24 C10 (b) / G5 -- die ISOLIERUNGS-PROBE des Major-Schlosses: heutige Magic (".A8."), aber Version 7.0.
//
// WOZU: der Loader prueft Magic VOR Version (anatomy_module_loader.cpp). Ein echtes Alt-Modul bewegt
// beide Werte und wird deshalb schon vom Magic-Schloss abgewiesen -- der Major-Check bliebe dabei
// UNGEPRUEFT und koennte still kaputt sein, ohne dass irgendeine Probe es merkt. Dieses Modul haelt die
// Magic auf dem HEUTIGEN Wert fest (also passiert es Schloss 1) und behauptet Major 7: es kann nur noch
// am Major-Vergleich scheitern (abi::AnatomyAbiVersion::host_compatible_with -> Schritt 5 der
// 7-Schritt-Validierung, status_abi_major_mismatch).
//
// Es ist ausdruecklich KEIN realistisches Alt-Modul, sondern ein Diagnose-Praeparat: die einzige
// Konfiguration, in der sich das zweite Schloss ueberhaupt einzeln beobachten laesst.
//
// Die Magic wird hier NICHT als Literal wiederholt, sondern aus COMDARE_ANATOMY_ABI_MAGIC gezogen --
// sie soll mit jedem kuenftigen Major MITWANDERN, sonst degeneriert diese Probe beim naechsten Bump
// still zur zweiten Magic-Probe. Nur der Major ist hier ein Literal, denn genau er soll abweichen.

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // Export-Makro + Magic + IAnatomyBase
#include <anatomy/set_abi_adapter.hpp>                     // SetAbiAdapter
#include <anatomy/set_anatomy.hpp>                         // SetAnatomy / SetComposition
#include <anatomy/set_default_organ.hpp>                   // SortedArrayKeySet

#include <cstdint>
#include <new>

namespace {

/// Der Major, den dieses Praeparat behauptet: HOST-MAJOR MINUS 1. Abgeleitet statt hartkodiert, damit die
/// Probe auch nach dem naechsten Bump noch "genau ein Major zu alt" bedeutet und nicht "zufaellig 7".
inline constexpr std::uint64_t kBehaupteterMajor = static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR) - 1;
inline constexpr std::uint64_t kBehaupteterMinor = 0;

static_assert(kBehaupteterMajor != static_cast<std::uint64_t>(COMDARE_ANATOMY_ABI_MAJOR),
              "Die Isolierungs-Probe muss einen ANDEREN Major behaupten als der Host fuehrt.");

using ProbeComposition =
    ::comdare::cache_engine::anatomy::SetComposition<::comdare::cache_engine::anatomy::SortedArrayKeySet, int, int, int,
                                                     int, int, int, int, int, int, int, int, int>;

} // namespace

extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_abi_version() noexcept {
    return (kBehaupteterMajor << 32) | kBehaupteterMinor;
}

/// HEUTIGE Magic -- damit passiert dieses Modul Schloss 1 und laeuft in Schloss 2.
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
