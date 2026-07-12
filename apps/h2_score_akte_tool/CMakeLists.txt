# GO-5 Fork 7 — h2_score_akte_tool: Generator der TOOL-BERECHNETEN H2-Code-Qualitaets-Akte
# (sota_h2_scores.xml, Dossier 20260712-go5 A.7 Option (b)). Standalone-CLI (Muster
# is_original_validator/catalog_codegen_tool); faehrt cppcheck ueber die vendorten ext/-Quellen.
# BEWUSST NICHT in den Default-Build/Codegen-Kanal verdrahtet: der Lauf ist ein expliziter
# Regenerier-Akt (cppcheck-Verfuegbarkeit vorausgesetzt), kein Build-Schritt.

add_executable(comdare_h2_score_akte_cli main.cpp)
set_target_properties(comdare_h2_score_akte_cli PROPERTIES
    OUTPUT_NAME "comdare-h2-score-akte")
target_compile_features(comdare_h2_score_akte_cli PRIVATE cxx_std_23)
target_include_directories(comdare_h2_score_akte_cli PRIVATE
    "${PROJECT_SOURCE_DIR}/libs/cache_engine/profile_facade"
    "${PROJECT_SOURCE_DIR}/libs/common/serialization")
