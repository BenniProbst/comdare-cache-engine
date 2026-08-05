#pragma once
// A8-S5 Familie 02b (2026-08-04) -- die EINE Stelle, die den Speicher-Versorger der composable-Filter-Organe
// benennt. Schnitt-Regel Dossier docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md
// Abschn. 3.4: "Speicher NUR ueber das Allokator-Achsen-Interface".
//
// @topic filter @achse axis_filter @schicht composable (Organ-statt-Tier)
//
// WARUM EINE EIGENE DATEI: die Familie 02b besteht aus VIER Headern (exact_prefix_filter_store,
// surf_suffix_bits, surf_louds_bitvector, louds_sparse_filter_store), die zusammen ZWEI Organe bilden
// (S1 exakt, S2 succinct LOUDS). Stuende der Versorger-Name in einem davon, muessten die uebrigen ihn
// entweder importieren (S1 haengte dann an S2-Substrat -- architektonisch falsch, S1 ist die
// GROUND-TRUTH-Base und kennt LOUDS nicht) oder ihn wiederholen -- und ein zweiter Name kann driften.
// Die 02a-Scheibe loeste dasselbe Problem mit je EINER Zeile je Achse (real_trie.hpp
// path_compression_trie_allocator_t, serialization_primitives.hpp serialization_stream_allocator_t);
// bei vier Dateien EINER Achse ist die gemeinsame Zeile die konsequente Form desselben Entscheids.
//
// WARUM ExgenAllocator: derselbe Achsen-Default, den die bereits konformen Pool-Stores des Repos fuehren
// (btree_node_pool_store.hpp:56, tree_node_pool_store.hpp, surf_fst_map_pool_store.hpp) und den die
// S5-Scheiben 03/01d/02a fuer ihre Nicht-Template-Organe gewaehlt haben. Beide Filter-Organe sind
// single-threaded getrieben (build_from_sorted_keys ist ein Bulk-Load, contains/range_may_exist sind
// read-only) -- die Sub-Achse AA4 des Exgen (Single-Threaded Specialized) passt. Bei abgeschaltetem
// Vendor-Flag faellt die Strategie intern auf portable_aligned_alloc zurueck: derselbe libc-Heap wie
// vorher, aber ueber das Achsen-Interface.
//
// WARUM HIER KEIN Kompositions-Allokator (Abgrenzung zum HERZ-Fall der node-Achse, 02a): GEPRUEFT am
// Objekt -- KEINE der vier Klassen ist Template ueber einen Allokator-Parameter A. LoudsSparseFilterStore
// ist Template ueber <SurfSuffixType ST, unsigned HashLen, unsigned RealLen> (die Suffix-TUNUNG),
// SurfSuffixBits ueber dieselben drei, SurfBitVector/SurfRank/SurfSelect und ExactPrefixFilterStore sind
// gar keine Templates. Die Organe werden auch nicht ueber eine Komposition instanziiert: die
// COMPOSITION-getragene filter-Achse ist axis_filter_registry.hpp (Bloom/Cuckoo/RangeSurf/Xor) und die
// ist heap-frei (std::array-Bitmaps). Der benannte Achsen-Default ist deshalb hier der richtige Schnitt;
// die Kompositions-Durchbindung ist Gegenstand des 01c-Design-Vorlaufs (LEDGER-Nachtrag 04.08. abend-11,
// T6 = Option B strikt).
//
// EINE STRATEGIE-INSTANZ JE ORGAN (Owner-KERN 04.08. abend-11): jedes der beiden Organe haelt GENAU EINE
// Instanz von filter_surf_allocator_t; alle seine Puffer -- auch die verschachtelten Level-Vektoren und die
// Puffer der eingebetteten Teil-Objekte (SurfRank/SurfSelect/SurfSuffixBits) -- haengen an DIESER Instanz.
// Mehrere Strategie-Instanzen INNERHALB eines Organs waeren mehrere getrennte Allokations-Buecher und damit
// genau das, was der Owner-KERN ausschliesst ("multiple Allokatoren liegen HINTER dem einen
// Allokator-Achsen-Interface").
//
// ABHAENGIGKEITSRICHTUNG (Dossier 3.3): filter -> alloc. Der Allokator ist die UNTERSTE Versorger-Achse;
// axes/alloc/ zieht keinen filter-Header -> kein Zyklus.
//
// FEHLSCHLAG-VERTRAG (Posten 64): die Achsen-Strategie meldet OOM per nullptr; der StdAllocatorAdapter
// uebersetzt das an EINER Stelle in std::bad_alloc (axis_06_allocator_strategy_base.hpp,
// StdAllocatorAdapter::allocate). Die [[allocation-failure-exception]]-Aussagen der Organe bleiben damit
// WAHR -- der Wurf kommt jetzt vom Adapter statt vom Default-Allokator; failure_count der Strategie ist
// zum Wurf-Zeitpunkt bereits gezaehlt. Fehlerklasse unveraendert der FK-5-Boden der Allokator-Achse
// (kOrganAxisErrorFloor).

#include <axes/alloc/axis_06_allocator_exgen.hpp>

#include <vector>

namespace comdare::cache_engine::filter_axis::composable {

/// Der EINE benannte Speicher-Versorger der composable-Filter-Organe (S1 + S2).
using filter_surf_allocator_t = ::comdare::cache_engine::alloc::ExgenAllocator;

/// std::allocator-konformer Adapter der Achsen-Strategie fuer Element-Typ T.
/// NICHT default-konstruierbar (haelt einen Zeiger auf die Strategie-Instanz) -- genau das ist die
/// compile-harte Absicherung dagegen, dass irgendein Puffer still auf einen Default-Allokator zurueckfaellt.
template <class T>
using surf_axis_adapter_t = typename filter_surf_allocator_t::template StdAllocatorAdapter<T>;

/// Der Vektor-Typ dieser Familie: IMMER ueber die Achse, nie ueber std::allocator.
template <class T>
using surf_vec_t = std::vector<T, surf_axis_adapter_t<T>>;

} // namespace comdare::cache_engine::filter_axis::composable
