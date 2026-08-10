# =============================================================================
#  FIXTURE-PRUEFLING -- das Registrierungs-Snippet (W0a/S6, 2026-08-10)
# =============================================================================
#
# Dies ist ein echter Pruefling im Sinne des Plugin-Controllers: er wird ueber
# -DCOMDARE_CE_PRUEFLINGE=<dieses Verzeichnis> geladen, laeuft im Scope der
# CE-Root-CMakeLists und sichert seine Faehigkeit ausdruecklich zu.
#
# Er liegt bewusst IN ce: der ce-CI-Baum hat keinen prt-art-Klon und soll auch
# keinen bekommen (Netz, Token, Bauzeit). Ohne ein CE-eigenes Fixture bliebe der
# Fall "Gatter = TRUE" das, was er seit dem 29.05.2026 war -- erschlossen und
# nie gefahren.
#
# T-3 (Nenner aus fremder Quelle) ist gewahrt: die Grundgesamtheit der
# Registrierungs-Wache ist `git ls-files`, nicht dieses Snippet. Die Testdatei
# heisst test_*.cpp und geht damit regulaer in den SOLL ein. Sie deshalb
# umzubenennen, um sie aus dem SOLL zu tricksen, waere Verstecken vor der
# eigenen Wache.

comdare_pruefling_deklarieren(
    NAME         fixture_min
    FAEHIGKEITEN pruefling_slots_v1
    INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include"
    TEST_SOURCES "${CMAKE_CURRENT_LIST_DIR}/test_pruefling_fixture_ladung.cpp"
    BELEG        fixture/slot_min.hpp)
