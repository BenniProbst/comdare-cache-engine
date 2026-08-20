// E-24 C10 (a) -- Genus-Permutations-DLL-Modulquelle der ADAPTER-Gattung. Die vierte und bis hier
// FEHLENDE Modul-Autor-Seite der Container-Gattung: perm_set_d9 / perm_sequence_d10 / perm_view_d11
// existieren seit L-76a als committete DLL-Quellen, ein Adapter-Gegenstueck gab es nie -- der
// DLL-Roundtrip lief deshalb ueber drei der vier Container-Genera und die vierte Gattung war nur
// in-process belegt (test_e24_c4_genus_pruef_docks).
//
// Materialisiert via COMDARE_DEFINE_ADAPTER_MODULE die vier extern-"C"-ABI-Pflicht-Symbole
// (comdare_anatomy_abi_version/magic/create_anatomy/destroy_anatomy) -> SHARED-Lib perm_adapter_d12,
// die derselbe gattungs-agnostische AnatomyModuleLoader laedt und test_e24_c10_genus_dll_roundtrip
// per dynamic_cast<IAdapterTier*> ueber das AdapterPruefDock treibt.
//
// BELEGUNG: DequeInner<> als inner_container-Substrat -- das ist die FIFO-Entnahme-Disziplin
// (adapter_anatomy.hpp: AdapterAnatomy::get() == pop_front, DequeInner front == das zuerst Abgelegte).
// Das Adapter-Konformitaets-Orakel (genus_conformance_gate.hpp) erkennt sie ueber die Disziplin-Probe
// mit drei unterscheidbaren Werten und prueft gegen std::queue. Die zehn geteilten/delegierten Achsen
// sind int-Filler wie in genus_module_sequence.cpp -- die Adapter-Gattung treibt real nur das
// inner_container-Organ (R5.B-Grenze, ehrlich).

#include <cache_engine/abi/adapter_module_abi_v1.hpp>

// AdapterComposition<T0..T9, Inner>: 10 geteilt/delegiert (INC-2d: isa raus) + inner_container.
// cppcheck kennt die COMDARE-Codegen-Emitter-Makros nicht (Definition via Include-Kette, kein -I im Lint-Lauf).
// cppcheck-suppress unknownMacro
COMDARE_DEFINE_ADAPTER_MODULE(int, int, int, int, int, int, int, int, int, int,
                              ::comdare::cache_engine::anatomy::DequeInner<>)

// A-11/golden-102 (19.08.2026): STEMPEL-PFLICHT -- der Loader weist stempellose Module mit status 13
// (version_lines_symbol_missing) ab; Emission ohne Stempel faellt. Diese Fixture traegt deshalb ihre
// EIGENEN, ehrlichen Zeilen (2-arg-Form: KEINE Mess-Deklaration -- sie kompiliert kein Tooling ein).
#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
COMDARE_ANATOMY_VERSION_STAMP("system_fixture=perm_adapter_d12@1.0.0.c", "fixture_kern=adapter_genus_dll@1.0.0.c")
