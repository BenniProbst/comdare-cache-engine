# paper_p07_wormhole — Kuratierte Wormhole-Snapshot (P2.D.tr.s3 Batch 1)

**Paper:** Wu, X., Ni, F., Jiang, S. *Wormhole: A Fast Ordered Index for
In-memory Data Management.* USENIX ATC 2019.

**Source:** `ext/traversal/P07-Wormhole/wormhole/wh.c` (GPL-3, siehe LICENSE).

**API-Mapping (3/4 originall, 1 Lücke):**
- insert → wh_put (Z3679)
- lookup → wh_get (Z3730)
- erase  → wh_del (Z3692)
- clear  → **LUECKE** (Wormhole ist persistent — kein Bulk-Clear im Paper, Re-Impl als Drain-Loop)

**Wrapper-Klasse:** OriginalWormholeSearchAlgo (S07).
is_original_module() = false (1/4 Lücke).
