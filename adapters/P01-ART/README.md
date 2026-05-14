# P01-ART — Adapter (Leis/Kemper/Neumann 2013)

**Paper:** Adaptive Radix Tree for Fast Main-Memory Indexes (ICDE 2013)
**Original-Repo:** ext/P01-ART/unodb (Apache 2.0)
**Stand:** V25.C (2026-05-14) — Adapter-Skelett

## Aufgabe

Wrappt die `unodb::db<Key, Value>` API in unsere C++23 Concepts:
- `ICachePage` — Knoten-Slots
- `ISearchEngine` — Standard-Suche (uniform reads)
- `INode4`/`INode16`/`INode48`/`INode256` — ART-Node-Familie

## Konvention

Pro Adapter eine Header-only INTERFACE-Library mit prefix `comdare_adapter_p01_art_*`.
Konsumenten nutzen `target_link_libraries(... comdare::adapter::p01_art)`.

## Status

- [ ] art_db_adapter.hpp — Map zu unodb::db<>
- [ ] art_workload_runner.hpp — YCSB_C/YCSB_A Workload-Setup
- [ ] art_traits.hpp — Plattform-Probe (NUMA, AVX2)
