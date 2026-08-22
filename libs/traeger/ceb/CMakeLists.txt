# TRAEGER-STUFE 2: CEB -- leeres INTERFACE-Ziel (Skeleton, DESIGN #29 Par. 4.2,
# super 85c1174d). Stufen-Vertrag compile-hart: Stufe 2 haengt NUR an Stufe 1
# (KON43-01); heutiger Bestand lebt als builder/ im Monolithen (Besitzkarte Par. 2a).
if(NOT TARGET comdare_planner)
    message(FATAL_ERROR "TRAEGER-STUFENKETTE VERLETZT: comdare_ceb (Stufe 2) braucht "
        "comdare_planner (Stufe 1). add_subdirectory(planner) muss VOR add_subdirectory(ceb) "
        "stehen -- Kette siehe libs/traeger/CMakeLists.txt.")
endif()
add_library(comdare_ceb INTERFACE)
target_link_libraries(comdare_ceb INTERFACE comdare_planner)
# EIGENE Include-Wurzel <traeger/ceb/...> (DESIGN #29 R5: neue Traeger-Projekte erben KEINE
# Alt-Wurzeln; die PUBLIC-Wurzel des Monolithen wird NICHT erweitert). Erster Kopf: M6-Katalog-
# Skelett parameter_filter_registry.hpp (#91-Vollzug W2-D, design91-v2 Abschnitt 3/M6).
target_include_directories(comdare_ceb INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
