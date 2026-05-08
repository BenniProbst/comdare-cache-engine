# ext — Originalcode der Paper (Habich-Direktive)
#
# Pro Paper-Verzeichnis wird der ORIGINALCODE EXAKT KOPIERT eingebettet.
# Compiler-Layering (F-EXTRA-1):
#  - Originalcode-Bausteine werden mit IHREM Original-Compiler kompiliert
#  - Statisches Linken in den C++23-Modul-Wrapper (siehe adapters/)
#
# Pre-Build-Phase (Phase 4.B Cluster-Migration): pro ext/<paper>/ wird
# eine eigene CMakeLists.txt mit Original-Compiler-Toolchain registriert.

# Skelett — Inhalt kommt nach Repo-Einbindung in Phase 4.B
# add_subdirectory(P01-ART)   # nach Migration
# add_subdirectory(P02-HOT)
# ...
