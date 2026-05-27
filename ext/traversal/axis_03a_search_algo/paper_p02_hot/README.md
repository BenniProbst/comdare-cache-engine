# paper_p02_hot — Kuratierte HOT-Snapshot (P2.D.tr s1)

**Paper:** Binna, R., Zangerle, E., Pichl, M., Specht, G. *HOT: A Height
Optimized Trie Index for Main-Memory Database Systems.* PVLDB 11(3), 2018.

**Source:** `ext/P02-HOT/hot/libs/hot/rowex/include/hot/rowex/HOTRowex.hpp`
(BSD License — siehe LICENSE).

**API-Mapping (2/4 originall, 2 Luecken — User-Direktive P2.D.tr):**
- insert → insert (Z83 HOTRowex.hpp)
- lookup → lookup (Z61)
- erase → **LUECKE** (HOT ist append-only — kein remove im Paper, cache-engine
  liefert eigene Re-Impl, is_original_erase() = false)
- clear → **LUECKE** (kein clear im Paper, eigene Re-Impl, is_original_clear() = false)

**Wrapper-Klasse:** OriginalHotSearchAlgo.

**is_original_module()** = false (weil 2/4 Lücken). PermutationEngine kann via
`PaperOriginalValidated`-Concept-Filter ausschliessen.
