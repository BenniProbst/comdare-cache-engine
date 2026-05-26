# paper_p05_start — Kuratierte START-Snapshot (P2.D.tr s1)

**Paper:** Mertens, R. et al. *START: Self-Tuning Adaptive Radix Tree.*
ICDE 2024.

**Source:** `ext/P05-START/START/sosd-competitor-adapter-START.h` (SOSD-Adapter
ueber Adaptive Radix Tree, MIT).

**API-Mapping (2/4 originall, 2 Luecken):**
- insert → insertLater (echte Function-Def im sosd-Adapter, nicht insertKey-Call)
- lookup → EqualityLookup
- erase → **LUECKE** (kein remove im sosd-Adapter, eigene Re-Impl)
- clear → **LUECKE** (kein clear im sosd-Adapter, eigene Re-Impl)

**Wrapper-Klasse:** OriginalStartSearchAlgo.

is_original_module() = false (Lücken). Cache-engine Wrapper macht funktional erase/clear,
markiert sie aber NICHT als Original-Paper-Code.
