# adapters — Adapter-Pattern fuer Originalcode (Habich-Direktive)
#
# Pro Paper ein Adapter, der zwischen Original-API und unseren C++23-
# Concepts (FanOutContainer, Page, Node, Traversal, ValueHandleStrategy,
# MemoryLayout, ConcurrencyDiscipline) uebersetzt.
#
# Adapter sind reine C++23-Schalen — kein Original-Compiler-Code hier.
# Kompilation mit Hauptcompiler.
#
# REV 7.7 V25.C (2026-05-14): 11 SOTA-Algorithmus-Adapter
# REV 7.7 V26.B (2026-05-14): 10 Allokator-Adapter

# SOTA-Algorithmus-Adapter (V25.C)
add_subdirectory(P01-ART)
add_subdirectory(P02-HOT)
add_subdirectory(P03-Masstree)
add_subdirectory(P04-CoCo-trie)
add_subdirectory(P05-START)
add_subdirectory(P07-Wormhole)
add_subdirectory(P10-SuRF)
add_subdirectory(P20-BTreesAreBack)
add_subdirectory(P25-Mahling)
add_subdirectory(P29-RCU)
add_subdirectory(P30-HazardPointers)

# Allokator-Adapter (V26.B)
add_subdirectory(A01-hoard)
add_subdirectory(A03-michael-lockfree)
add_subdirectory(A04-mimalloc)
add_subdirectory(A05-jemalloc)
add_subdirectory(A06-tcmalloc)
add_subdirectory(A07-snmalloc)
add_subdirectory(A08-scalloc)
add_subdirectory(A10-rpmalloc)
add_subdirectory(A11-lrmalloc)
add_subdirectory(A20-dlmalloc)
