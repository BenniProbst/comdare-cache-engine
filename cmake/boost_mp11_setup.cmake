# cmake/boost_mp11_setup.cmake
# V41.F.6.1.A (2026-05-25) - Boost.MP11 fuer Permutations-Metaprogrammierung.
# V41.F.6.1   (2026-05-29) - Prerequisite-Mechanismus: vendored mp11 -> Archiv-Auto-Extract -> FetchContent.
#
# Memory-Direktiven: KEIN Python in Build-Pipeline (nur CMake + sh/bat), KEINE Git-Submodules.
#
# Boost.MP11 ist header-only (standalone seit Boost 1.66, keine weiteren Boost-Abhaengigkeiten).
# Aufloesungs-Reihenfolge (autonom — laeuft im configure-Schritt = Teil der "Einrichtung"):
#
#   (1) VENDORED   : cmake/third_party/boost_mp11/include/boost/mp11.hpp vorhanden -> direkt nutzen.
#                    (Standard-Fall: kleine Header sind im Repo getrackt -> Offline-Build out-of-the-box.)
#   (2) PREREQ-ARCHIV: ein Boost-/mp11-Archiv in prerequisites/ (boost_*.{tar.bz2,tar.gz,tar.xz,7z,zip})
#                    ODER -DCOMDARE_BOOST_ARCHIVE=<pfad> -> file(ARCHIVE_EXTRACT) der mp11-Header ->
#                    vendored Verzeichnis befuellen -> nutzen. (Autonomes "Entpacken bei Fehlen".)
#   (3) FETCHCONTENT: GitHub boostorg/mp11 @ GIT_TAG (nur falls Internet-Egress vorhanden).
#   (4) FATAL      : klare Anleitung, ein Archiv in prerequisites/ abzulegen.
#
# Verwendung: target_link_libraries(<target> PRIVATE Boost::mp11)

include(FetchContent)

# V41.P1-Gateway: cache-engine-Wurzel robust (CMAKE_SOURCE_DIR = Superprojekt-Wurzel im
# add_subdirectory-Kontext → Offline-Boost-Prerequisite würde sonst nicht gefunden).
set(_ce_root "${CMAKE_CURRENT_LIST_DIR}/..")
set(_mp11_vendor_dir "${_ce_root}/cmake/third_party/boost_mp11")
set(_mp11_vendor_inc "${_mp11_vendor_dir}/include")
set(_mp11_vendor_hdr "${_mp11_vendor_inc}/boost/mp11.hpp")
set(_mp11_prereq_dir "${_ce_root}/prerequisites")
set(_mp11_pinned_tag "boost-1.91.0")   # Fallback-Tag (passt zum bereitgestellten Prerequisite-Boost 1.91.0)

# --- Hilfsfunktion: mp11-Header aus einem (vollen Boost- ODER standalone-mp11-) Archiv extrahieren -------
function(_comdare_provision_mp11_from_archive _archive _out_ok)
    message(STATUS "Boost.MP11: provisioniere aus Prerequisite-Archiv: ${_archive}")
    set(_tmp "${_mp11_vendor_dir}/_mp11_extract")
    file(REMOVE_RECURSE "${_tmp}")
    file(MAKE_DIRECTORY "${_tmp}")
    # libarchive (in CMake integriert) unterstuetzt tar.bz2/tar.gz/tar.xz/zip/7z.
    # PATTERNS begrenzt die Extraktion auf den mp11-Teilbaum (kein 200-MB-Voll-Boost-Entpacken).
    file(ARCHIVE_EXTRACT
        INPUT "${_archive}"
        DESTINATION "${_tmp}"
        PATTERNS "*/boost/mp11.hpp" "*/boost/mp11/*" "boost/mp11.hpp" "boost/mp11/*")
    file(GLOB_RECURSE _found "${_tmp}/boost/mp11.hpp" "${_tmp}/*/boost/mp11.hpp")
    if(NOT _found)
        message(WARNING "Boost.MP11: Archiv enthielt kein boost/mp11.hpp: ${_archive}")
        file(REMOVE_RECURSE "${_tmp}")
        set(${_out_ok} FALSE PARENT_SCOPE)
        return()
    endif()
    list(GET _found 0 _mp11_hpp)
    get_filename_component(_boostdir "${_mp11_hpp}" DIRECTORY)   # .../boost
    file(MAKE_DIRECTORY "${_mp11_vendor_inc}/boost")
    file(COPY "${_mp11_hpp}" DESTINATION "${_mp11_vendor_inc}/boost")
    if(EXISTS "${_boostdir}/mp11")
        file(COPY "${_boostdir}/mp11" DESTINATION "${_mp11_vendor_inc}/boost")
    endif()
    file(REMOVE_RECURSE "${_tmp}")
    if(EXISTS "${_mp11_vendor_hdr}")
        set(${_out_ok} TRUE PARENT_SCOPE)
    else()
        set(${_out_ok} FALSE PARENT_SCOPE)
    endif()
endfunction()

set(_mp11_ready FALSE)
set(_mp11_include "")

# --- (1) VENDORED -----------------------------------------------------------------------------------------
if(EXISTS "${_mp11_vendor_hdr}")
    message(STATUS "Boost.MP11: vendored Header gefunden -> ${_mp11_vendor_inc}")
    set(_mp11_include "${_mp11_vendor_inc}")
    set(_mp11_ready TRUE)
endif()

# --- (2) PREREQUISITE-ARCHIV (autonom entpacken) ----------------------------------------------------------
if(NOT _mp11_ready)
    set(_archive "")
    if(DEFINED COMDARE_BOOST_ARCHIVE AND EXISTS "${COMDARE_BOOST_ARCHIVE}")
        set(_archive "${COMDARE_BOOST_ARCHIVE}")
    else()
        file(GLOB _cands
            "${_mp11_prereq_dir}/boost_*.tar.bz2" "${_mp11_prereq_dir}/boost_*.tar.gz"
            "${_mp11_prereq_dir}/boost_*.tar.xz"  "${_mp11_prereq_dir}/boost_*.7z"
            "${_mp11_prereq_dir}/boost_*.zip"     "${_mp11_prereq_dir}/mp11*.tar.gz"
            "${_mp11_prereq_dir}/mp11*.zip")
        if(_cands)
            list(GET _cands 0 _archive)
        endif()
    endif()
    if(_archive)
        _comdare_provision_mp11_from_archive("${_archive}" _ok)
        if(_ok)
            message(STATUS "Boost.MP11: aus Archiv provisioniert -> ${_mp11_vendor_inc}")
            set(_mp11_include "${_mp11_vendor_inc}")
            set(_mp11_ready TRUE)
        endif()
    endif()
endif()

# --- (3) FETCHCONTENT (Internet-Egress-Fallback) ----------------------------------------------------------
if(NOT _mp11_ready)
    include(fetchcontent_stale_cleanup)
    comdare_clean_stale_fetchcontent_subbuild(boost_mp11)
    message(STATUS "Boost.MP11: kein lokales Archiv — FetchContent von GitHub @ ${_mp11_pinned_tag} (Egress noetig)")
    FetchContent_Declare(
        boost_mp11
        GIT_REPOSITORY https://github.com/boostorg/mp11.git
        GIT_TAG        ${_mp11_pinned_tag}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(boost_mp11)
    set(_mp11_include "${boost_mp11_SOURCE_DIR}/include")
    set(_mp11_ready TRUE)
endif()

# --- (4) Verifikation -------------------------------------------------------------------------------------
if(NOT _mp11_ready OR NOT EXISTS "${_mp11_include}/boost/mp11.hpp")
    message(FATAL_ERROR
        "Boost.MP11 nicht verfuegbar.\n"
        "  -> Lege ein Boost- oder mp11-Archiv in '${_mp11_prereq_dir}/' ab "
        "(z.B. boost_1_91_0.tar.bz2 oder boost_1_91_0.7z),\n"
        "     oder setze -DCOMDARE_BOOST_ARCHIVE=<pfad>,\n"
        "     oder stelle Internet-Egress zu github.com bereit (FetchContent-Fallback).\n"
        "  Bootstrap-Helfer: tools/prerequisites/bootstrap_prerequisites.{sh,bat}")
endif()

# --- header-only INTERFACE-Target -------------------------------------------------------------------------
# (boost-cmake hat unterschiedliche Targets je Version, eigenes Target ist robust)
if(NOT TARGET Boost::mp11)
    add_library(Boost_mp11 INTERFACE)
    add_library(Boost::mp11 ALIAS Boost_mp11)
    target_include_directories(Boost_mp11 INTERFACE "${_mp11_include}")
    target_compile_features(Boost_mp11 INTERFACE cxx_std_23)
endif()

message(STATUS "Boost.MP11 ready: ${_mp11_include}")
