# PAKET W3-C (Ledger Sec.32-F4/GN-8/O-4) - axis_version_lock: eigenstaendiges C++23-Executable, das den
# Content-Digest der Heuristik-Strategie-Header gegen tools/axis_version_lock/axis_version.lock prueft
# (--check) bzw. das Lock neu schreibt (--write). Wiederverwendung der vorhandenen consteval-SHA-256
# (libs/cache_engine/src/sha256/ctsha.hpp). KEINE Fremdbibliothek, kein Python.
#
# EXCLUDE_FROM_ALL (INERT-by-default, Anti-Phantom): der Standard-Build baut das Tool NICHT mit; es wird
# explizit per Target gebaut (analog axis_registry_gen / gen_golden_fullpilot). Der CI-Job (W3-A) baut das
# Target und ruft --check. golden/TABU bleiben unberuehrt (das Tool liest nur Header, schreibt nur das Lock).

add_executable(comdare_axis_version_lock EXCLUDE_FROM_ALL axis_version_lock.cpp)
target_compile_features(comdare_axis_version_lock PRIVATE cxx_std_23)
target_include_directories(comdare_axis_version_lock PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"       # <sha256/ctsha.hpp>
    # S-14a: BESTANDS-Parser <cache_engine/measurement/algo_semver.hpp> fuer bump_ok des OrganDetail
    # (X.Y.Z[.flags]-Grammatik; EIN Parser, keine Zweitgrammatik im Tool). Leichtgewichtig: zieht nur
    # flag_grammar_catalog.hpp + simd_feature_flag.hpp + std -- KEINE Registry-/Varianten-Tabelle.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    # S-14a Fixup 3 (D1): <builder/overlay_source_set.hpp> -- DIE EINE QUELLE der Overlay-
    # Grundgesamtheit (kOverlaySourceSet + Endungs-Listen), dieselbe, aus der der Fingerprint-
    # Codegen tools/overlay_source_hash_gen seine Dateimenge liest. Keine Zweitliste im Tool.
    "${PROJECT_SOURCE_DIR}/libs/cache_engine")
# Boost::mp11 kommt ueber builder/experiment_tree/axis_path_serialization.hpp herein (dort wohnt
# kCompositionAxisNames, die kanonische Organ-Ordnung der Schnitt-Wachen) -- Header-only, derselbe
# Weg wie beim Overlay-Codegen (s. tools/overlay_source_hash_gen/CMakeLists.txt).
target_link_libraries(comdare_axis_version_lock PRIVATE Boost::mp11)
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_axis_version_lock)
endif()
