# REV 7.7 V23.E (2026-05-14) — libs/deprecated/
# Folly-experimental/-Idiom invertiert: Code hier ist scheduled for removal.
# Build nur bei expliziter Aktivierung.

option(COMDARE_BUILD_DEPRECATED "Build deprecated code (will be removed)" OFF)

if(COMDARE_BUILD_DEPRECATED)
  message(WARNING "Building deprecated code. Will be removed in future release.")
  add_subdirectory(prt_art_legacy)
endif()
