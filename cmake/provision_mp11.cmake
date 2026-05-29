# cmake/provision_mp11.cmake
# V41.F.6.1 (2026-05-29) — Standalone-Provisioning von Boost.MP11 aus einem Prerequisite-Archiv.
#
# Ausfuehrung (ausserhalb eines vollen configure, z.B. aus dem Bootstrap-Skript):
#   cmake [-DCOMDARE_BOOST_ARCHIVE=<pfad>] -P cmake/provision_mp11.cmake
#
# Extrahiert NUR den header-only mp11-Teilbaum (boost/mp11.hpp + boost/mp11/) aus einem vollen
# Boost- ODER standalone-mp11-Archiv (tar.bz2/tar.gz/tar.xz/zip/7z via libarchive) nach
#   cmake/third_party/boost_mp11/include/boost/
# Diese vendored Header werden anschliessend von boost_mp11_setup.cmake (Pfad 1) direkt genutzt.
# KEIN Python, KEINE Git-Submodules.

# Repo-Wurzel = Verzeichnis ueber cmake/ (dieses Skript liegt in cmake/).
get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_vendor_dir "${_repo_root}/cmake/third_party/boost_mp11")
set(_vendor_inc "${_vendor_dir}/include")
set(_vendor_hdr "${_vendor_inc}/boost/mp11.hpp")
set(_prereq_dir "${_repo_root}/prerequisites")

if(EXISTS "${_vendor_hdr}")
    message(STATUS "provision_mp11: vendored Header bereits vorhanden -> ${_vendor_inc} (nichts zu tun)")
    return()
endif()

# Archiv ermitteln: explizit gesetzt > prerequisites/-Glob.
set(_archive "")
if(DEFINED COMDARE_BOOST_ARCHIVE AND EXISTS "${COMDARE_BOOST_ARCHIVE}")
    set(_archive "${COMDARE_BOOST_ARCHIVE}")
else()
    file(GLOB _cands
        "${_prereq_dir}/boost_*.tar.bz2" "${_prereq_dir}/boost_*.tar.gz"
        "${_prereq_dir}/boost_*.tar.xz"  "${_prereq_dir}/boost_*.7z"
        "${_prereq_dir}/boost_*.zip"     "${_prereq_dir}/mp11*.tar.gz"
        "${_prereq_dir}/mp11*.zip")
    if(_cands)
        list(GET _cands 0 _archive)
    endif()
endif()

if(NOT _archive)
    message(FATAL_ERROR
        "provision_mp11: kein Archiv gefunden. Lege boost_1_91_0.{tar.bz2,7z,zip} (o.ae.) in "
        "'${_prereq_dir}/' ab oder setze -DCOMDARE_BOOST_ARCHIVE=<pfad>.")
endif()

message(STATUS "provision_mp11: extrahiere mp11 aus ${_archive}")
set(_tmp "${_vendor_dir}/_mp11_extract")
file(REMOVE_RECURSE "${_tmp}")
file(MAKE_DIRECTORY "${_tmp}")
file(ARCHIVE_EXTRACT
    INPUT "${_archive}"
    DESTINATION "${_tmp}"
    PATTERNS "*/boost/mp11.hpp" "*/boost/mp11/*" "boost/mp11.hpp" "boost/mp11/*")

file(GLOB_RECURSE _found "${_tmp}/boost/mp11.hpp" "${_tmp}/*/boost/mp11.hpp")
if(NOT _found)
    file(REMOVE_RECURSE "${_tmp}")
    message(FATAL_ERROR "provision_mp11: Archiv enthielt kein boost/mp11.hpp: ${_archive}")
endif()
list(GET _found 0 _mp11_hpp)
get_filename_component(_boostdir "${_mp11_hpp}" DIRECTORY)
file(MAKE_DIRECTORY "${_vendor_inc}/boost")
file(COPY "${_mp11_hpp}" DESTINATION "${_vendor_inc}/boost")
if(EXISTS "${_boostdir}/mp11")
    file(COPY "${_boostdir}/mp11" DESTINATION "${_vendor_inc}/boost")
endif()
file(REMOVE_RECURSE "${_tmp}")

if(EXISTS "${_vendor_hdr}")
    message(STATUS "provision_mp11: OK -> ${_vendor_inc}")
else()
    message(FATAL_ERROR "provision_mp11: Extraktion fehlgeschlagen (kein ${_vendor_hdr}).")
endif()
