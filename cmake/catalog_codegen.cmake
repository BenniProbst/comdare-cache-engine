# Limits-Entkopplung: generierter Default-Source-Katalog.
#
# Das Tool liegt unter apps/catalog_codegen_tool und existiert erst nach
# add_subdirectory(apps). Der Header bleibt strikt im Build-Baum.
#
# GN-2/§26.6 (2026-07-19, Design BESTAETIGT): dieser Codegen bleibt BEWUSST auf dem 320er-m3v2-Profil --
# die golden-N-REFERENZ (FullSourceCatalog, 2^17=131072) ist vom MATERIALISIERTEN Katalog entkoppelt und
# wird NIE materialisiert (GB-TU/ICE). Die Entkopplung ist seit GN-2 nicht mehr nur Konvention: der
# generierte Header ruft make_catalog_source_gen<GeneratedFullSourceCatalog>() und laeuft damit durch den
# negativen Instanziierungs-Guard in profile_facade/source_catalog.hpp (static_assert gegen die Vollform +
# Kardinalitaets-Grenze kMaxMaterializableCatalogCardinality). Ein Repoint dieses Profils auf eine Form
# jenseits der Grenze bricht den Build compile-time, billig und laut -- NICHT erst als GB-TU im mp_product.

get_filename_component(_comdare_catalog_codegen_ce_root
    "${CMAKE_CURRENT_LIST_DIR}/.."
    ABSOLUTE)

# PROJECT_BINARY_DIR (nicht CMAKE_BINARY_DIR): im super-Sub-Build schreibt ce damit in sein
# eigenes Build-Verzeichnis (_cache_engine_external) — dieselbe Konvention wie die Axis-Codegen-
# Header des Root-CMakeLists (CMAKE_CURRENT_BINARY_DIR im Root-Scope).
set(_comdare_limits_generated_dir "${PROJECT_BINARY_DIR}/generated")
set(_comdare_limits_generated_header
    "${_comdare_limits_generated_dir}/generated_source_catalog.hpp")
set(_comdare_limits_profile_xml
    "${_comdare_catalog_codegen_ce_root}/libs/cache_engine/algorithm_profiles/thesis_profiles/m3v2_study.profile.xml")

if(NOT TARGET comdare_catalog_codegen_cli)
    message(FATAL_ERROR
        "catalog_codegen.cmake: comdare_catalog_codegen_cli fehlt; Modul nach add_subdirectory(apps) einbinden.")
endif()
set_property(GLOBAL APPEND PROPERTY COMDARE_PAPER_CODEGEN_CONSUMER_TARGETS
    comdare_catalog_codegen_cli)

if(NOT TARGET comdare_limits_generated_source_catalog)
    add_custom_command(
        OUTPUT "${_comdare_limits_generated_header}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_comdare_limits_generated_dir}"
        COMMAND $<TARGET_FILE:comdare_catalog_codegen_cli>
                --profile "${_comdare_limits_profile_xml}"
                --out "${_comdare_limits_generated_header}"
        DEPENDS comdare_catalog_codegen_cli "${_comdare_limits_profile_xml}"
        COMMENT "Limits-Entkopplung: generated_source_catalog.hpp aus m3v2_study.profile.xml"
        VERBATIM)
    add_custom_target(comdare_limits_generated_source_catalog
        DEPENDS "${_comdare_limits_generated_header}")
endif()

function(comdare_attach_generated_catalog target)
    if(NOT TARGET ${target})
        message(FATAL_ERROR
            "comdare_attach_generated_catalog: Target '${target}' existiert nicht.")
    endif()
    add_dependencies(${target} comdare_limits_generated_source_catalog)
    # Der generierte Header re-inkludiert "source_catalog.hpp" (generische Katalog-Templates).
    # Seit der P0-Hebung liegt dieser Header in libs/cache_engine/profile_facade (nicht mehr im
    # Test-Harness) — die attach-Funktion traegt deshalb BEIDE Konsum-Voraussetzungen.
    target_include_directories(${target} PRIVATE
        "${_comdare_limits_generated_dir}"
        "${_comdare_catalog_codegen_ce_root}/libs/cache_engine/profile_facade")
endfunction()
