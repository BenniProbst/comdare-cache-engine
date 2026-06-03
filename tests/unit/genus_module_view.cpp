// L-76c Genus-Permutations-DLL-Modulquelle (View-Gattung) — materialisiert via COMDARE_DEFINE_VIEW_MODULE die 4
// extern-"C"-ABI-Symbole → perm_view_d11.dll, geladen vom gattungs-agnostischen AnatomyModuleLoader, getrieben von
// test_dgenus_dll via dynamic_cast<IViewTier*> (tier_bind/tier_read/tier_observe_view).
//
// REPO-COMMITTET (vorher nur ephemer in build/). 4 geteilte Slots = int; extent/layout/accessor nutzen die
// Defaults (DynamicExtent/LayoutRight/DefaultAccessor — read real über die layout/accessor-Policy, non-owning).
// Build: siehe tests/unit/genus_dll_roundtrip.ps1 (committet).

#include <cache_engine/abi/view_module_abi_v1.hpp>

COMDARE_DEFINE_VIEW_MODULE(int, int, int, int)
