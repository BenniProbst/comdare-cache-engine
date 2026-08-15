# TRAEGER-STUFE 4: HYBRID -- leeres INTERFACE-Ziel (Skeleton, DESIGN #29 Par. 4.2,
# super 85c1174d). Stufen-Vertrag compile-hart: Stufe 4 haengt NUR an Stufe 3
# (KON43-01). Bleibt reines Skelett, bis die niedrigeren Stufen stehen (KON43-01/1).
if(NOT TARGET comdare_tier_emission)
    message(FATAL_ERROR "TRAEGER-STUFENKETTE VERLETZT: comdare_hybrid (Stufe 4) braucht "
        "comdare_tier_emission (Stufe 3). add_subdirectory(tier) muss VOR "
        "add_subdirectory(hybrid) stehen -- Kette siehe libs/traeger/CMakeLists.txt.")
endif()
add_library(comdare_hybrid INTERFACE)
target_link_libraries(comdare_hybrid INTERFACE comdare_tier_emission)
