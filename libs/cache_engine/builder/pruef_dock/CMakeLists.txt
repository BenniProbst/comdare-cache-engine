# V41.F.6.1.R6 — Prüf-Dock (builder-seitiger per-Gattung ABI-Mess-Übergang, Doku 24 §8.8)
#
# Header-only INTERFACE-Target: verkabelt die bereits existierenden anatomy- + anatomy_commands- +
# anatomy_module_loader-Bausteine hinter einen uniformen per-Gattung-Vertrag (IPruefDock). KEIN Neubau,
# nur Benennung/Organisation der vorhandenen ABI-Mess-Schnittstelle.

add_library(comdare_ce_pruef_dock INTERFACE)
add_library(comdare::ce_pruef_dock ALIAS comdare_ce_pruef_dock)

target_include_directories(comdare_ce_pruef_dock INTERFACE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src")   # measurement/measurable_concept.hpp (transitiv über observable_composed_search)

target_compile_features(comdare_ce_pruef_dock INTERFACE cxx_std_23)

# Der Mess-Treiber konstruiert/zerstört AnatomyModuleHandle (unload() lebt im Loader-.cpp) → Loader-Lib linken.
if(TARGET comdare::anatomy_module_loader)
    target_link_libraries(comdare_ce_pruef_dock INTERFACE comdare::anatomy_module_loader)
endif()

if(TARGET Boost::mp11)
    target_link_libraries(comdare_ce_pruef_dock INTERFACE Boost::mp11)
endif()
