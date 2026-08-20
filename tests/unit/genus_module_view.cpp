// L-76c Genus-Permutations-DLL-Modulquelle (View-Gattung) — materialisiert via COMDARE_DEFINE_VIEW_MODULE die 4
// extern-"C"-ABI-Symbole → perm_view_d11.dll, geladen vom gattungs-agnostischen AnatomyModuleLoader, getrieben von
// test_dgenus_dll via dynamic_cast<IViewTier*> (tier_bind/tier_read/tier_observe_view).
//
// REPO-COMMITTET (vorher nur ephemer in build/). 4 geteilte Slots = int; extent/layout/accessor nutzen die
// Defaults (DynamicExtent/LayoutRight/DefaultAccessor — read real über die layout/accessor-Policy, non-owning).
// Build: siehe tests/unit/genus_dll_roundtrip.ps1 (committet).
// SUPERSEDED 2026-07-11: genus_dll_roundtrip.ps1 entfernt (Behelfsweg-Bereinigung); DLL jetzt via CMakeLists
// gebaut (SHARED perm_view_d11), Round-Trip = registriertes ctest-Target test_dgenus_dll (Goal-V6 L-76-Block).

#include <cache_engine/abi/view_module_abi_v1.hpp>

COMDARE_DEFINE_VIEW_MODULE(int, int, int)

// A-11/golden-102 (19.08.2026): STEMPEL-PFLICHT -- der Loader weist stempellose Module mit status 13
// (version_lines_symbol_missing) ab; Emission ohne Stempel faellt. Diese Fixture traegt deshalb ihre
// EIGENEN, ehrlichen Zeilen (2-arg-Form: KEINE Mess-Deklaration -- sie kompiliert kein Tooling ein).
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
COMDARE_ANATOMY_VERSION_STAMP("system_fixture=perm_view_d11@1.0.0.c", "fixture_kern=view_genus_dll@1.0.0.c")
