# comdare-succinct — eigene SDSL-Lite-Portierung (Aufgabe #105)

add_library(comdare_succinct INTERFACE)
add_library(comdare::succinct ALIAS comdare_succinct)
target_include_directories(comdare_succinct INTERFACE include)
target_compile_features(comdare_succinct INTERFACE cxx_std_23)
