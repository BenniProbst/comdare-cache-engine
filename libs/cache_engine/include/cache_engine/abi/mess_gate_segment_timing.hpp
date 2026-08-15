#pragma once
// abi/mess_gate_segment_timing.hpp -- B2 (15.08.2026): DIE EINE ABLEITUNG DES G3-GATES.
//
// ------------------------------------------------------------------------------------------------
// WAS HIER GEBAUT IST (B2, Gate-Trennung G2/G3 -- KON34/KON25-03 "Tier je an/aus")
// ------------------------------------------------------------------------------------------------
// Bis B2 teilten sich G2 (Observer: fill_observer_v3/axis_stats) und G3 (Feinkorn: die 18
// per-Achsen-Segment-Timer in fill_segment_timing_v3) EIN Gate, COMDARE_CE_ENABLE_STATISTICS.
// Die Mess-Naht (profile_facade/mess_achsen_naht.hpp, Abschnitt "EHRLICHE GRENZE DIESER SCHEIBE",
// bis B2) hat diese Grenze ausdruecklich benannt: [wallclock,macro] und [wallclock,micro] erzeugten
// denselben Gate-Zustand und unterschieden sich ausschliesslich im Deklarations-Define.
//
// AB B2 traegt G3 ein EIGENES Gate:
//     G2 (macro) : COMDARE_CE_ENABLE_STATISTICS      (#ifdef -- unveraendert, alle Bestands-Stellen)
//     G3 (micro) : COMDARE_CE_ENABLE_SEGMENT_TIMING  (wertbasiert, #if -- NUR die drei
//                  Segment-Lauf-Flaechen in anatomy/abi_adapter.hpp: fill_segment_timing_v3,
//                  fill_observer_pathb_driven_v3, reset_pathb_driven_organs_)
//
// ------------------------------------------------------------------------------------------------
// WARUM EINE ABLEITUNG UND KEIN NACKTES DEFINE -- DIE VERERBUNGS-REGEL
// ------------------------------------------------------------------------------------------------
// Drei Bau-Welten muessen denselben Header lesen koennen, ohne dass der Default sich bewegt:
//   (1) CMake-/CI-Bestandsbauten ([all]-Semantik): definieren NUR COMDARE_CE_ENABLE_STATISTICS.
//       Sie MUESSEN G3 weiter einkompilieren -- "die Trennung darf NICHTS am Default aendern".
//   (2) Der Perm-Pfad (mess_achsen_defines, KON37-01: die CEB baut hoehere Stufen NUR nach ihren
//       EIGENEN Messeigenschaften): entscheidet das G3-Gate IMMER MIT, sobald G2 emittiert wird --
//       -DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1 (micro in der Menge) bzw. =0 (Observer ohne micro).
//   (3) Release-/gate-lose TUs: kein Gate definiert -> G3 aus.
// Die Aufloesung: IST das Makro gesetzt, gilt sein WERT (die explizite Entscheidung der Stufe 2).
// Ist es NICHT gesetzt, ERBT G3 den Zustand des Alt-Gates G2 -- exakt die Bedeutung, die das
// kombinierte Gate bis B2 hatte. Ein Bestandsbau kann dadurch kein Byte seines Gate-Zustands
// verlieren; die Trennung wird erst wirksam, wo jemand sie ausdruecklich bestellt.
//
// WARUM DIE ABLEITUNG GENAU EINMAL WOHNT: zwei Konsumenten lesen das wirksame G3-Gate --
// anatomy/abi_adapter.hpp (die Schalt-Flaeche) und abi/mess_gates_glied.hpp (das neunte
// Preimage-Glied, das den REAL wirksamen TU-Zustand stempelt). Zwei getrennte Ableitungen waeren
// exakt die Drift-Klasse D-1: der Stempel behauptete einen Zustand, den der Bau nicht faehrt.
// Beide inkludieren deshalb DIESEN Header; die Ableitung ist idempotent (#pragma once + #ifndef).
//
// SCHICHTUNGS-WACHE (Regel "erst laute Compile-Fehler"): G3 ohne G2 ist kein baubarer Zustand --
// die Segment-Timer schreiben in Strukturen des Observer-Blocks, und die Naht-Zuordnung ist
// micro => G1+G2+G3 (Registry-Semantik). Ein von Hand gesetztes =1 ohne G2 bricht hier LAUT.
//
// header-only, ASCII-only, keine Includes -- reiner Praeprozessor.

#ifndef COMDARE_CE_ENABLE_SEGMENT_TIMING
#ifdef COMDARE_CE_ENABLE_STATISTICS
#define COMDARE_CE_ENABLE_SEGMENT_TIMING 1 // VERERBUNG: unentschieden folgt G3 dem Alt-Gate G2 (an)
#else
#define COMDARE_CE_ENABLE_SEGMENT_TIMING 0 // VERERBUNG: unentschieden folgt G3 dem Alt-Gate G2 (aus)
#endif
#endif

#if COMDARE_CE_ENABLE_SEGMENT_TIMING && !defined(COMDARE_CE_ENABLE_STATISTICS)
#error "B2: COMDARE_CE_ENABLE_SEGMENT_TIMING=1 (G3, Segment-Timer) verlangt COMDARE_CE_ENABLE_STATISTICS (G2, \
Observer) -- micro ist G1+G2+G3 (mess_achsen_naht.hpp). Abhilfe: G2 mitdefinieren oder G3 auf 0 setzen."
#endif
