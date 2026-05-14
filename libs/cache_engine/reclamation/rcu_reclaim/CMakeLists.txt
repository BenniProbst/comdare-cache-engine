# rcu_reclaim — Eigene RCU-Implementation (Aufgabe #104)
# Ersetzt liburcu (P29) durch C++23-Header-only

add_library(comdare_rcu INTERFACE)
add_library(comdare::rcu ALIAS comdare_rcu)
target_include_directories(comdare_rcu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_rcu INTERFACE cxx_std_23)
