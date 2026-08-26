# cmake/compiler_flags.cmake
# REV 5.3 Phase 6 - Compiler-Flags pro OS/Compiler
# Multi-OS-konforme Flag-Setzung; KEIN /OS-spezifisches Setting ohne Fallback.

# O2-STANDARD (Owner-Entscheid 21.08.2026: "O2 ist Standard fuer alle Builds, O3 wird unter
# Warnung angeboten"; Owner-Primaerquelle Thesis-Kommentar 728fc74, 21.08.2026: "... ist das
# nicht im Ermessen des Entwicklers, was er bei der Verwendung des Systems unter Warnung der
# Konsequenzen verwendet? Maximale Optimierung muss waehlbar bleiben."):
# Release der Haus-Bauwelt (CEB/Planer/Tests) traegt -O2. -O3 bleibt AUSDRUECKLICH waehlbar --
# als Opt-in COMDARE_OPT_O3=ON mit lauter Configure-WARNING, die die KONSEQUENZEN nennt.
# Historie: bis 22.08.2026 trug Release hier -O3 (UEBERHOLT; Doku-Doktrin, nichts geloescht).
# Die Tier-Binary-Emission (build_orchestrator-opt_flag-Kanal / XML-Achse opt_level) ist davon
# UNBERUEHRT -- ihr beweglicher CEB-Default lebt in system_axes/optimization_level_sub_axis.hpp.
# [NACHGEFUEHRT VO3-1(b), Owner-KERN X4 25.08.2026: die Release-Flags gelten seither HAUS-WEIT --
#  inkl. Vendor ext/, FetchContent-Fremdbau und super-Embed -- per CACHE-FORCE (Block unten); die
#  Bauwelt-Stufe ist als Feld 'vendoropt' des Toolchain-Glieds [5] identitaetswirksam. Historie oben.]
option(COMDARE_OPT_O3
    "Release-Builds mit -O3 statt des O2-Standards uebersetzen (Opt-in unter Warnung)" OFF)
if(COMDARE_OPT_O3)
    message(WARNING
        "COMDARE_OPT_O3=ON: Release-Builds tragen -O3 statt des O2-Standards "
        "(Owner-Entscheid 21.08.2026: 'O2 ist Standard fuer alle Builds, O3 wird unter Warnung "
        "angeboten'). KONSEQUENZEN: (1) O3-Artefakte tragen einen ANDEREN Toolchain-Fingerprint "
        "(Glied [5], Feld 'vendoropt' -- seit VO3-1(b), X4 25.08.2026) -- Messreihen sind mit "
        "O2-Standard-Bestand NICHT direkt vergleichbar (kein falscher Cache-Skip, fail-closed); "
        "(2) aggressivere Transformationen (u.a. Auto-Vektorisierung) aendern Codegroesse und "
        "Laufzeitcharakteristik; der IEEE-754-Determinismus bleibt bei -O3 erhalten (erst -Ofast "
        "braeche ihn, das bleibt der XML-Achse vorbehalten); (3) binary_id und golden-320 bleiben "
        "unberuehrt; (4) MSVC-Release bleibt /O2 (kein /O3-Aequivalent).")
    message(STATUS "Release-Optimierung: -O3 (COMDARE_OPT_O3-Opt-in unter Warnung)")
else()
    message(STATUS "Release-Optimierung: -O2 (O2-Standard, Owner-Entscheid 21.08.2026)")
endif()

# VO3-1(b) (Owner-KERN X4 25.08.2026: "(b) global O2 und nur auf XML Eingangswunsch O3 mit bauen"):
# die Release-Opt-Stufe wird EINMAL hier am Datei-Top-Level aufgeloest (gehisste Single-Source; die
# Warn-Funktion unten liest sie per Directory-Scope, KEINE Doppel-Berechnung) und per CACHE-FORCE
# GLOBAL durchgesetzt -- Haus, Vendor ext/ (mimalloc static.c), FetchContent-Fremdbau (gtest) und
# super-Embed-Ziele bauen Release mit DERSELBEN Stufe. Beweis der Reichweite: CMake-Reichweiten-
# Proben A/B/C (backups-workflow/20260825-vo3-1-global-o2/design/probes/): eine NORMALE Variable
# erreicht die super-Embed-Ziele NICHT und laesst die CMakeCache luegen; nur CACHE ... FORCE macht
# beides wahr. Deklarierte per-Target-Ausnahme bleibt GENAU test_all19_segment_timer (mess-
# charakteristisch, tests/unit/CMakeLists.txt) -- das OS-2-Gate (cmake/vo31_optflag_gate.cmake)
# haelt diese Ausnahmemenge geschlossen.
if(COMDARE_OPT_O3)
    set(_COMDARE_release_opt "-O3")
    set(_COMDARE_release_opt_id "O3")
else()
    set(_COMDARE_release_opt "-O2")
    set(_COMDARE_release_opt_id "O2")
endif()
if(NOT MSVC)
    foreach(_comdare_lang C CXX)
        set(_comdare_soll "${_COMDARE_release_opt} -DNDEBUG")
        # Kein stiller Dreh: ein Cache-Wert, der weder die CMake-Vorgabe (-O3 -DNDEBUG) noch das
        # Haus-Soll ist, wurde von jemandem gesetzt -- er wird LAUT ersetzt, nicht still.
        if(DEFINED CMAKE_${_comdare_lang}_FLAGS_RELEASE
           AND NOT CMAKE_${_comdare_lang}_FLAGS_RELEASE STREQUAL "-O3 -DNDEBUG"
           AND NOT CMAKE_${_comdare_lang}_FLAGS_RELEASE STREQUAL "${_comdare_soll}")
            message(WARNING
                "CMAKE_${_comdare_lang}_FLAGS_RELEASE='${CMAKE_${_comdare_lang}_FLAGS_RELEASE}' wird "
                "durch den Haus-Standard '${_comdare_soll}' ersetzt (VO3-1(b), Owner-KERN X4 "
                "25.08.2026) -- der offizielle O3-Weg ist COMDARE_OPT_O3=ON.")
        endif()
        set(CMAKE_${_comdare_lang}_FLAGS_RELEASE "${_comdare_soll}"
            CACHE STRING "Release-Flags (VO3-1(b) Haus-Standard, cmake/compiler_flags.cmake)" FORCE)
    endforeach()
    message(STATUS
        "COMDARE: CMAKE_C/CXX_FLAGS_RELEASE = ${_COMDARE_release_opt} -DNDEBUG (global, VO3-1(b))")
endif()
# MSVC bleibt unberuehrt: die CMake-Vorgabe (/O2 /Ob2 /DNDEBUG) ist bereits O2-Klasse, ein /O3
# existiert nicht (WARNING-Punkt (4) oben).
#
# A-F1/VO3-1(b): die vendoropt-IDENTITAET (Glied [5], neues Feld 'vendoropt') wird GLOBAL definiert
# -- genau an der gehissten Quelle, Single-Source mit den Release-Flags. Das Makro steht in KEINEM
# CT-Preimage (kCeb-/kPlaner-Fingerprint bewegungslos; Beweisauflage: test_d4 + Frozen-Anker im
# K17); der CT-Slot COMDARE_TOOLCHAIN_STAMP_GLIED bleibt unberuehrt (der reist NUR per Tier-rsp,
# toolchain_stamp_naht.hpp). Konsument: active_vendoropt() ebendort (#error-Wache bei fehlendem
# Define).
add_compile_definitions("COMDARE_FACADE_VENDOROPT_ID=\"${_COMDARE_release_opt_id}\"")

# Helper: setze Compile-Optionen pro Compiler-Familie
function(COMDARE_set_default_warnings target)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE
            /W4              # Hohe Warn-Stufe (NICHT /Wall - zu viel Rauschen)
            /permissive-     # Striktes C++23-Standard-Verhalten
            /Zc:__cplusplus  # Korrekter __cplusplus Wert
            /utf-8           # UTF-8 Source-Encoding
            /EHsc            # Standard-Exception-Handling
            $<$<CONFIG:Debug>:/Od /Zi>
            $<$<CONFIG:Release>:/O2>)
        target_compile_definitions(${target} PRIVATE
            _CRT_SECURE_NO_WARNINGS
            NOMINMAX
            WIN32_LEAN_AND_MEAN)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        # O2-STANDARD: _COMDARE_release_opt ist seit VO3-1(b) (26.08.2026) an den DATEI-TOP-LEVEL
        # GEHISST (EINE Quelle fuer CACHE-FORCE-Flags, vendoropt-Define und diese per-Target-Stufe);
        # hier gilt Directory-Scope-Lookup, KEINE Doppel-Berechnung. Die Aufloesung ist weiterhin
        # nie leer (unbedingtes if/else am Top-Level) -- kein stiller Compiler-Default -O0 moeglich.
        # Historie: bis 26.08.2026 wurde der Wert hier lokal berechnet (Doku-Doktrin, nichts geloescht).
        # B16/K12 (2026-08-21): die C++-ONLY-Kategorien sind per COMPILE_LANGUAGE:CXX gegatet.
        # Bis B16 traf diese Funktion nur reine C++-Test-Executables und das Gate war unnoetig;
        # mit der Produktions-Deckung erreichte sie erstmals ein Target mit einer C-TU und gcc
        # meldete je C-Objekt 6 cc1-Warnungen ("not valid for C"). Fuer reine C++-Targets ist
        # der Ausdruck verhaltensgleich (gleiche Flags, gleiche Haerte).
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor>
            -Wcast-align
            $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
            $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
            -Wmissing-declarations
            # S1 NULL-KOSTEN-RATSCHE: zu Fehlern werden NUR Kategorien, die am
            # Objekt nachweislich treffer-frei sind. Gemessen am Vollbau ueber
            # gcc 15.3 und clang 22.1 x Debug/Release: 0 Treffer je Kategorie in
            # 4 von 4 Zellen, Nenner je 594 kompilierte CXX-Objekte. Jede Zeile
            # hier hat einen Koeder, der sie zum Beissen bringt, und einen
            # konformen Gegenkoeder, der gruen bleibt.
            #
            # KEIN globales -Werror. Das machte auch Kategorien mit heutigen
            # Treffern hart rot, namentlich -Wstringop-overflow= (Aufgabe #71,
            # heute 1 Treffer unter gcc-Release). Solche Kategorien werden
            # verifiziert und festgeschrieben, NICHT gehaertet.
            #
            # -Wcast-align fehlt hier mit Absicht, und NICHT weil es Treffer
            # haette: auf x86-64 meldet ein Koeder (reinterpret_cast char* nach
            # int*) weder unter g++ noch unter clang++ etwas, 0 in beiden
            # Stufen. Derselbe Koeder unter -Wcast-align=strict meldet g++ 2 und
            # clang++ 1 -- der Koeder ist also scharf, die Null ist ein
            # Plattform-Artefakt. Damit ist die Kategorie hier nicht bewertbar,
            # und eine Haertung waere keine Zusicherung, sondern nur Dekoration.
            # Bewertbar wird sie erst auf einer Architektur mit echten
            # Ausrichtungs-Anforderungen.
            $<$<COMPILE_LANGUAGE:CXX>:-Werror=non-virtual-dtor>
            $<$<COMPILE_LANGUAGE:CXX>:-Werror=overloaded-virtual>
            -Werror=pedantic
            $<$<COMPILE_LANGUAGE:CXX>:-Werror=old-style-cast>
            $<$<CONFIG:Debug>:-O0 -g3>
            $<$<CONFIG:Release>:${_COMDARE_release_opt}>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g>)
    endif()
endfunction()

# Helper: setze C++23 Standard (verlaesslich Compiler-uebergreifend)
function(COMDARE_set_cpp23 target)
    target_compile_features(${target} PUBLIC cxx_std_23)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PUBLIC /std:c++latest)
    endif()
endfunction()

# Helper: Plattform-Defines fuer Sub-Projekte (target_type-bewusst)
function(COMDARE_set_platform_defines target)
    get_target_property(_type ${target} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        set(_scope INTERFACE)
    else()
        set(_scope PUBLIC)
    endif()

    target_compile_definitions(${target} ${_scope}
        COMDARE_OS_NAME="${COMDARE_OS}"
        COMDARE_ARCH_NAME="${COMDARE_ARCH}"
        COMDARE_BLOCK_AO_PLATFORM_NAME="${COMDARE_BLOCK_AO_PLATFORM}"
        COMDARE_CACHE_LINE_SIZE=${COMDARE_CACHE_LINE_SIZE})
    if(COMDARE_OS_WINDOWS)
        target_compile_definitions(${target} ${_scope} COMDARE_OS_WINDOWS=1)
    endif()
    if(COMDARE_OS_LINUX)
        target_compile_definitions(${target} ${_scope} COMDARE_OS_LINUX=1)
    endif()
    if(COMDARE_OS_MACOS)
        target_compile_definitions(${target} ${_scope} COMDARE_OS_MACOS=1)
    endif()
    if(COMDARE_ARCH_X86_64)
        target_compile_definitions(${target} ${_scope} COMDARE_ARCH_X86_64=1)
    endif()
    if(COMDARE_ARCH_ARM64)
        target_compile_definitions(${target} ${_scope} COMDARE_ARCH_ARM64=1)
    endif()
    if(COMDARE_ARCH_RISCV64)
        target_compile_definitions(${target} ${_scope} COMDARE_ARCH_RISCV64=1)
    endif()
endfunction()
