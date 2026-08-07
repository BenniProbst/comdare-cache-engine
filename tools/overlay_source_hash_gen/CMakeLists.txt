# E-E (07.08.2026) -- overlay_source_hash_gen: der Pre-Build-Codegen des Overlay-Glieds [7].
#
# NICHT EXCLUDE_FROM_ALL (anders als die Registry-Generatoren): sein Ergebnis ist ein Compile-Define,
# das JEDE Uebersetzungseinheit mit einem Anatomy-Fingerprint braucht. Ein Werkzeug, das der
# Standard-Bau nicht baut, koennte diese Zusage nicht halten.
#
# Er zieht NUR den Schnitt-Header + die SHA-512-Primitive -- bewusst KEINEN Achsen-Code: der Codegen
# laeuft VOR der Uebersetzung dessen, was er hasht, und darf davon nicht abhaengen. Boost::mp11 kommt
# ueber experiment_tree/axis_path_serialization.hpp herein (dort wohnt kCompositionAxisNames, die
# kanonische Organ-Ordnung, gegen die die Schnitt-Wachen pruefen).

add_executable(comdare_overlay_source_hash_gen main.cpp)
target_compile_features(comdare_overlay_source_hash_gen PRIVATE cxx_std_23)
target_include_directories(comdare_overlay_source_hash_gen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder")
target_link_libraries(comdare_overlay_source_hash_gen PRIVATE Boost::mp11)
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_overlay_source_hash_gen)
endif()
