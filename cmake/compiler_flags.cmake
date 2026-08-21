# cmake/compiler_flags.cmake
# REV 5.3 Phase 6 - Compiler-Flags pro OS/Compiler
# Multi-OS-konforme Flag-Setzung; KEIN /OS-spezifisches Setting ohne Fallback.

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
            $<$<CONFIG:Release>:-O3>
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
