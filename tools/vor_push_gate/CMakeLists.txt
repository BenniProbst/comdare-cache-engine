# tools/vor_push_gate -- VOR-PUSH-GATE: welche CI-Jobs sind lokal gefahren, welche nicht?
#
# BAUFORM, und warum sie so ist (Owner-Entscheid 10.08.2026):
#   "Der Auftrag gilt fuer shell skripte dort, wo auch ein statischer cmake und binary
#    Hilfsprogramme performante und portable Loesungen bieten koennen. CI Skripte gehoeren
#    formal nicht zum Kernprojekt und sind daher erlaubt, aber .sh und .bat und .py wollen
#    wir vermeiden."
# Also kein neues .sh. Die Hausform fuer ein eigenstaendiges Hilfsprogramm ist tools/<name>/
# mit duennem main.cpp -- dasselbe Muster wie tools/axis_version_lock (Content-Digest-Wache)
# und tools/best_binary_selector. Die Substanz liegt im Kopf vor_push_gate.hpp, damit die
# Testwerkbank sie ohne Prozess-Start pruefen kann (tests/unit/test_vor_push_gate.cpp).
#
# KEINE ABHAENGIGKEIT ZUR ENGINE, und das ist die tragende Eigenschaft: dieses Werkzeug laeuft
# VOR einem Bau. Haenge es an comdare::cache_engine, und es braucht genau das Bauverzeichnis,
# dessen Fehlen es gerade melden soll. Es zieht nur die Standardbibliothek.
#
# DESHALB AUCH DER EINZEILEN-WEG, der ohne jedes Bauverzeichnis auskommt:
#   g++ -std=c++23 -O1 -o vor-push-gate tools/vor_push_gate/main.cpp
# Das ist der normale Gebrauchsweg vor einem Push. Das CMake-Ziel hier ist der Hausweg fuer
# alle, die ohnehin einen konfigurierten Baum haben.
#
# EXCLUDE_FROM_ALL (INERT-by-default, wie axis_version_lock): der Standard-Build baut das
# Werkzeug nicht mit. Es ist Entwickler-/CI-Infrastruktur, kein Teil der Mess-Kette und keiner
# der vier Binary-Typen (Planer/CEB/Tier/Hybrid).
#
# ASCII-only, Zeilen <= 120 Byte.

add_executable(comdare_vor_push_gate EXCLUDE_FROM_ALL main.cpp)
set_target_properties(comdare_vor_push_gate PROPERTIES OUTPUT_NAME "vor-push-gate")
target_compile_features(comdare_vor_push_gate PRIVATE cxx_std_23)
target_include_directories(comdare_vor_push_gate PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(COMMAND comdare_set_platform_defines)
    comdare_set_platform_defines(comdare_vor_push_gate)
endif()

# -Werror=return-type / -Werror=switch: dieselbe Stufe-1-Haertung wie super Code/ci_wachen.
# Der Mutant "ein Zweig faellt heraus und laeuft durch" hat hier kein Analogon, weil jede
# Verzweigung ein Ergebnis RETURNT -- ein fehlendes return bricht bereits den Bau. Bewusst nur
# auf diesem Ziel: eine Zusicherung dieses Werkzeugs, keine hausweite Umstellung.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(comdare_vor_push_gate PRIVATE -Werror=return-type -Werror=switch)
endif()
