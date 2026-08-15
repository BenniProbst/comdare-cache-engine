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
