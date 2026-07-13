# V25.C (2026-05-14) — P29-RCU Adapter-Skelett
# V31.K3 (2026-05-14) — Inhalt: p29_rcu_adapter.hpp (LiburcuAdapter)
option(COMDARE_HAVE_LIBURCU "Activate ext/traversal/P29-RCU/userspace-rcu/ (LGPL-2.1+)" OFF)
add_library(comdare_adapter_p29_rcu INTERFACE)
add_library(comdare::adapter::p29_rcu ALIAS comdare_adapter_p29_rcu)
target_include_directories(comdare_adapter_p29_rcu INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(comdare_adapter_p29_rcu INTERFACE cxx_std_23)
if(COMDARE_HAVE_LIBURCU)
    target_compile_definitions(comdare_adapter_p29_rcu INTERFACE COMDARE_HAVE_LIBURCU=1)
    target_include_directories(comdare_adapter_p29_rcu INTERFACE
        "${PROJECT_SOURCE_DIR}/ext/traversal/P29-RCU/userspace-rcu/include")
endif()
