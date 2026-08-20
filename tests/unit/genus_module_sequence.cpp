// L-76b Genus-Permutations-DLL-Modulquelle (Sequence-Gattung) — materialisiert via COMDARE_DEFINE_SEQUENCE_MODULE
// die 4 extern-"C"-ABI-Symbole → perm_sequence_d10.dll, geladen vom gattungs-agnostischen AnatomyModuleLoader,
// getrieben von test_dgenus_dll via dynamic_cast<ISequenceTier*> (tier_push_back/tier_at/tier_observe_sequence).
//
// REPO-COMMITTET (vorher nur ephemer in build/). 10 geteilte Slots = int (von SequenceAnatomy nicht instanziiert);
// der axis_growth-Slot nutzt den Default DoublingGrowth (real getrieben → growth_events>0 über die DLL-Grenze).
// Build: siehe tests/unit/genus_dll_roundtrip.ps1 (committet).
// SUPERSEDED 2026-07-11: genus_dll_roundtrip.ps1 entfernt (Behelfsweg-Bereinigung); DLL jetzt via CMakeLists
// gebaut (SHARED perm_sequence_d10), Round-Trip = registriertes ctest-Target test_dgenus_dll (Goal-V6 L-76-Block).

#include <cache_engine/abi/sequence_module_abi_v1.hpp>

// cppcheck kennt die COMDARE-Codegen-Emitter-Makros nicht (Definition via Include-Kette, kein -I im Lint-Lauf).
// cppcheck-suppress unknownMacro
COMDARE_DEFINE_SEQUENCE_MODULE(int, int, int, int, int, int, int,
                               int) // Bau-INC-2d: 8 geteilte (isa raus) + Growth-Default

// A-11/golden-102 (19.08.2026): STEMPEL-PFLICHT -- der Loader weist stempellose Module mit status 13
// (version_lines_symbol_missing) ab; Emission ohne Stempel faellt. Diese Fixture traegt deshalb ihre
// EIGENEN, ehrlichen Zeilen (2-arg-Form: KEINE Mess-Deklaration -- sie kompiliert kein Tooling ein).
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
COMDARE_ANATOMY_VERSION_STAMP("system_fixture=perm_sequence_d10@1.0.0.c", "fixture_kern=sequence_genus_dll@1.0.0.c")
