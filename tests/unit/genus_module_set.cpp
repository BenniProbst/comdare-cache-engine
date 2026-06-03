// L-76a Genus-Permutations-DLL-Modulquelle (Set-Gattung) — die MODUL-AUTOR-Seite EINER repräsentativen Set-
// Permutation. Materialisiert via COMDARE_DEFINE_SET_MODULE die 4 extern-"C"-ABI-Symbole (abi_version/magic/
// create_anatomy/destroy_anatomy) → wird als SHARED-Lib (cl /LD /DCOMDARE_ANATOMY_MODULE_BUILD) zu perm_set_d9.dll,
// die der gattungs-agnostische AnatomyModuleLoader lädt und test_dgenus_dll via dynamic_cast<ISetTier*> treibt.
//
// REPO-COMMITTET (vorher nur ephemer in build/ generiert) → der Set/Sequence/View-DLL-Round-Trip ist damit aus
// committeter Quelle reproduzierbar (schließt den Phase-E-Audit-Vorbehalt "nur git-ignoriert scratch-verifiziert").
// search_algo-Kern = SortedArrayKeySet (echtes Binary-Search-Set-Organ, K=V); T1..T14 = int (von SetAnatomy nicht
// getrieben — die Set-Gattung treibt real nur das search_algo-Organ, R5.B-Grenze ehrlich). Build: siehe
// tests/unit/genus_dll_roundtrip.ps1 (committet).

#include <cache_engine/abi/set_module_abi_v1.hpp>
#include <anatomy/set_default_organ.hpp>

COMDARE_DEFINE_SET_MODULE(
    ::comdare::cache_engine::anatomy::SortedArrayKeySet,
    int, int, int, int, int, int, int, int, int, int, int, int, int, int)
