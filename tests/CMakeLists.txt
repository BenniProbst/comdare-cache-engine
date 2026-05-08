# tests — Google Tests (F-EXTRA-8: generisch + spezifisch)

if(COMDARE_BUILD_TESTS)
  add_subdirectory(unit)
  add_subdirectory(integration)
  add_subdirectory(generic_module_tests)
  add_subdirectory(module_specific_tests)
endif()
