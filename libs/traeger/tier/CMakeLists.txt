# TRAEGER-STUFE 3: TIER -- leeres INTERFACE-Ziel (Skeleton, DESIGN #29 Par. 4.2,
# super 85c1174d). Das TIER-Unterprojekt besitzt Emission + Modul-Skelett + Harness,
# NICHT die Fach-Substanz (die steckt zur Laufzeit ABI-stabil in der CEB, KON25-08).
# Stufen-Vertrag compile-hart: Stufe 3 haengt NUR an Stufe 2 (KON43-01). Bleibt reines
# Skelett, bis die niedrigere Stufe steht (KON43-01/1) -- kein Bau vor S-8/S-9.
if(NOT TARGET comdare_ceb)
    message(FATAL_ERROR "TRAEGER-STUFENKETTE VERLETZT: comdare_tier_emission (Stufe 3) "
        "braucht comdare_ceb (Stufe 2). add_subdirectory(ceb) muss VOR add_subdirectory(tier) "
        "stehen -- Kette siehe libs/traeger/CMakeLists.txt.")
endif()
add_library(comdare_tier_emission INTERFACE)
target_link_libraries(comdare_tier_emission INTERFACE comdare_ceb)
