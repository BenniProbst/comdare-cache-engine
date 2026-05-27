# cmake/boost_mp11_setup.cmake
# V41.F.6.1.A (2026-05-25) - Boost.MP11 via FetchContent (KEINE Git-Submodules)
#
# Memory-Direktive: KEIN Python in Build-Pipeline, KEINE Git-Submodules.
# Boost.MP11 (header-only) wird via FetchContent geholt; bei Offline-Build kann
# ein lokaler Tarball in cmake/third_party/boost_mp11-1.86.0.tar.gz abgelegt werden.
#
# Verwendung: target_link_libraries(<target> PRIVATE Boost::mp11)
# Erzeugt INTERFACE-Target Boost::mp11 mit Include-Directory.

include(FetchContent)
include(fetchcontent_stale_cleanup)
comdare_clean_stale_fetchcontent_subbuild(boost_mp11)

set(_mp11_local "${CMAKE_SOURCE_DIR}/cmake/third_party/boost_mp11-1.86.0.tar.gz")

if(EXISTS "${_mp11_local}")
    message(STATUS "Boost.MP11: lokaler Tarball gefunden: ${_mp11_local}")
    FetchContent_Declare(
        boost_mp11
        URL "${_mp11_local}")
else()
    message(STATUS "Boost.MP11: lade boost-1.86.0 mp11-Subrepo via FetchContent von GitHub")
    FetchContent_Declare(
        boost_mp11
        GIT_REPOSITORY https://github.com/boostorg/mp11.git
        GIT_TAG        boost-1.86.0
        GIT_SHALLOW    TRUE)
endif()

FetchContent_MakeAvailable(boost_mp11)

# boost::mp11 ist header-only — manuell INTERFACE-Target anlegen
# (boost-cmake hat unterschiedliche Targets je Version, eigenes Target ist robust)
if(NOT TARGET Boost::mp11)
    add_library(Boost_mp11 INTERFACE)
    add_library(Boost::mp11 ALIAS Boost_mp11)
    target_include_directories(Boost_mp11 INTERFACE
        "${boost_mp11_SOURCE_DIR}/include")
    target_compile_features(Boost_mp11 INTERFACE cxx_std_23)
endif()

message(STATUS "Boost.MP11 ready: ${boost_mp11_SOURCE_DIR}/include")
