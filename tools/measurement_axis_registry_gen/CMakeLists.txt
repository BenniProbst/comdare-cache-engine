# PAKET W2-B (2026-07-19) - measurement_axis_registry_gen: eigenstaendiges C++-Executable, das die Mess-
# Achsen-Registry-XML measurement_axis_registry.xml per compile-time-Reflektion der realen Mess-Achsen-Typen
# erzeugt (Ledger §28/§30, Mess-Art-Registry im Mess-Modul). NUR ein Build-Target; die XML-Generierung ist ein
# Laufzeit-Schritt (KEINE CMake-Logik). EXCLUDE_FROM_ALL: der Standard-Build baut das Tool nicht mit; es wird
# explizit per Target (bzw. ueber die ctest-COMDARE_TEST_TARGETS-Schleife) gebaut.
#
# Reflektiert die 16 MeasurementCategory-Achsen + 3 Kollektoren + dynamische Mess-Dimensionen; zieht KEINE
# Organ-Topic-Registries (leichte TU).

add_executable(comdare_measurement_axis_registry_gen EXCLUDE_FROM_ALL main.cpp)
target_compile_features(comdare_measurement_axis_registry_gen PRIVATE cxx_std_23)
target_include_directories(comdare_measurement_axis_registry_gen PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/include"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/src"
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/builder"
    "${PROJECT_SOURCE_DIR}/libs/common"
    "${PROJECT_BINARY_DIR}/generated")
if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_measurement_axis_registry_gen)
endif()
