# A01-hoard — Allokator-Adapter (Berger et al. 2000)

**Paper:** Hoard — A Scalable Memory Allocator for Multithreaded Applications (ASPLOS 2000)
**Original-Repo:** ext/A01-hoard (Apache-2.0)
**Profile:** `cache_engine/algorithm_profiles/allocators/hoard.profile.xml`
**Stand:** V26.B (2026-05-14) — Adapter-Skelett

## Aufgabe
Wrappt Hoard als `IAllocator` mit Superblock-basierter Strategie.
expected_workload=YCSB_A.

## Status
- [ ] hoard_adapter.hpp
- [ ] hoard_traits.hpp
