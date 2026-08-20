// tests/unit/r3_mess_gate_stamp_module.cpp -- R-3 BISS, die MODUL-SEITE (07.08.2026).
//
// EIN Quelltext, ZWEI .so-Targets. Die Stempel-LITERALE sind in beiden Bauten byte-identisch; der
// einzige Unterschied ist der PRAEPROZESSOR-ZUSTAND der Mess-Gates:
//   r3_biss_modul_gates_an   COMDARE_MEASUREMENT_ON=1, COMDARE_CE_ENABLE_STATISTICS=1, ...
//   r3_biss_modul_gates_aus  keines davon (COMDARE_R3_BISS_GATES_AUS -> die #undef-Praeparation unten)
//
// GENAU DAS ist der Befund, den R-3 heilt: das Preimage von anatomy_fingerprint_hex rechnete bis
// hierher NUR ueber die drei String-LITERALE plus die injizierten Glieder [5]/[6]/[7]. Kein
// Gate-Makro stand darin. Also lieferten beide Bauten -- verschiedene Binaries, verschiedene
// Mess-Ausstattung -- DENSELBEN sha512_line. dll_is_current ist ein Sidecar-Vergleich ueber genau
// diese Zahl (build_orchestrator.hpp): der Falsch-Skip ist mechanisch, nicht hypothetisch.
//
// WARUM DIE ABSCHALTUNG PER #undef UND NICHT PER "target ohne -D" GESCHIEHT: die Wurzel-CMakeLists
// setzt COMDARE_MEASUREMENT_ON=1 per add_compile_definitions GLOBAL (MEASUREMENT_MODE=ON ist der
// Default). Ein Target kann eine Verzeichnis-Definition nicht wieder abwaehlen. Die #undef-Praeparation
// VOR jedem Include ist der im Baum bereits sanktionierte, portable Weg fuer genau diesen Zustand --
// Praezedenz test_a8s4_release_pfad_neutralitaet.cpp (dort woertlich "der PORTABLE Weg (kein
// -U/-D-Kunstgriff in CMake, keine Compiler-Abhaengigkeit)"). Der so praeparierte Zustand IST der
// Release-Baumodus (COMDARE_RELEASE_MODE=ON), also ein real erreichbarer Bau, kein Kunstgriff.
//
// ASCII-only.

#ifdef COMDARE_R3_BISS_GATES_AUS
// Exakt der Zustand von COMDARE_RELEASE_MODE=ON: MEASUREMENT_MODE=OFF erzwingt STATISTICS=OFF, und
// EXPERIMENT_MODE_ON verlangt seinerseits MEASUREMENT_ON (abi_adapter.hpp:9-10) und faellt mit.
#undef COMDARE_MEASUREMENT_ON
#undef COMDARE_CE_ENABLE_STATISTICS
#undef COMDARE_EXPERIMENT_MODE_ON
#undef COMDARE_MEASUREMENT_TOOLING_WALLCLOCK
#undef COMDARE_MEASUREMENT_TOOLING_MACRO
#undef COMDARE_MEASUREMENT_TOOLING_MICRO
#if defined(COMDARE_MEASUREMENT_ON) && COMDARE_MEASUREMENT_ON
#error "R-3 BISS: die GATES-AUS-Variante muss ohne COMDARE_MEASUREMENT_ON uebersetzen."
#endif
#ifdef COMDARE_CE_ENABLE_STATISTICS
#error "R-3 BISS: die GATES-AUS-Variante muss ohne COMDARE_CE_ENABLE_STATISTICS uebersetzen."
#endif
#else
// Die GATES-AN-Variante bekommt ihre Defines vom Target; die Wachen halten fest, dass sie WIRKLICH
// ankommen -- ein Biss, dessen beide Seiten still denselben Zustand haetten, waere kein Biss.
#if !defined(COMDARE_MEASUREMENT_ON) || !COMDARE_MEASUREMENT_ON
#error "R-3 BISS: die GATES-AN-Variante braucht COMDARE_MEASUREMENT_ON=1 (target_compile_definitions)."
#endif
#ifndef COMDARE_CE_ENABLE_STATISTICS
#error "R-3 BISS: die GATES-AN-Variante braucht COMDARE_CE_ENABLE_STATISTICS=1."
#endif
#endif

#include <cache_engine/abi/anatomy_module_abi_v1.hpp>

// Die drei Stempel-Literale. Sie stehen als Makros da, damit BEIDE Bauten nachweislich dieselben
// Zeichen sehen (der Runner prueft die drei Zeilen zusaetzlich auf Byte-Gleichheit -- waeren sie
// verschieden, waere ein Fingerprint-Unterschied trivial und bewiese nichts ueber die Gates).
#define COMDARE_R3_BISS_ORGAN_LIT "search_algo=k_ary@1.0.0.c;filter=bloom@2.3.4.c"
#define COMDARE_R3_BISS_SYSTEM_LIT                                                                                     \
    "target_isa=code@1.0.0.c;operating_system=code@1.0.0.c;external_utils=code@1.0.0.c;[simd=code@1.0.0.c]"
#define COMDARE_R3_BISS_MEASURE_LIT "measurement_tooling=wallclock@1.0.0.c;[load_framework=ycsb@1.0.0.c]"

// S-6a: Argument-Folge MESS, SYSTEM, ORGAN.
COMDARE_ANATOMY_VERSION_STAMP_M(COMDARE_R3_BISS_MEASURE_LIT, COMDARE_R3_BISS_SYSTEM_LIT, COMDARE_R3_BISS_ORGAN_LIT)
