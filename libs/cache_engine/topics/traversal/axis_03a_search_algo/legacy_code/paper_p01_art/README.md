# paper_p01_art — Kuratierte ART-Snapshot (P2.D.tr s1)

**Paper:** Leis, V., Kemper, A., Neumann, T. *The Adaptive Radix Tree: ARTful
Indexing for Main-Memory Databases.* ICDE 2013.

**Source:** `ext/P01-ART/unodb/art.hpp` (unodb-Implementation, MIT-Lizenz).

**API-Mapping (alle 4 Functions originall):**
- insert → insert_internal
- lookup → get (Line 218, `[[nodiscard, gnu::pure]]`)
- erase → remove_internal
- clear → clear

**Wrapper-Klasse:** OriginalArtSearchAlgo (parallel zu Array256 Re-Impl).
