# paper_p03_masstree — Kuratierte Masstree-Snapshot (P2.D.tr.s3 Batch 2 — Template-Adapter)

**Paper:** Mao, Y., Kohler, E., Morris, R. *Cache Craftiness for Fast
Multicore Key-Value Storage.* EuroSys 2012.

**Source:** `ext/traversal/P03-Masstree/masstree-beta/` (MIT-Lizenz).

**API-Pattern:** Template-Library mit Out-of-Class Definitions verteilt ueber mehrere
Header (Masstree-typisch). Wrapper instantiiert `Masstree::basic_table<table_params>`
nach rotaki/masstree-wrapper Gold-Standard (MIT).

**API-Mapping (4/4 originall via Template-Adapter):**
- insert → `tcursor<P>::find_insert(...)` (masstree_insert.hh)
- lookup → `basic_table<P>::get(...)` (masstree_get.hh)
- erase  → `tcursor<P>::find_locked(...)` (masstree_remove.hh, finish(-1, ...))
- clear  → `basic_table<P>::destroy(...)` + `initialize(...)` (masstree.hh)

**Wrapper-Klasse:** OriginalMasstreeSearchAlgo (S09).
is_original_module() = true (alle 4 via Template-Instantiation Paper-bound).

**User-Direktive 2026-05-26 (verbatim):**
> "Masstree ist ein zentraler Algorithmus, bitte recherchiere im Web und im Paper
> wie man ihn bedient, sodass wir einfach das korrekte template einsetzen und im
> wrapper verwenden... Wenn es im Wrapper einen template Adapter auf eine source
> bedarf statt klarem Interface, dann ist das eben so... Ist trotzdem original code,
> wenn wir ein Template Typ davon verwenden. Man kann in cmake trotz template types
> diese statisch zum build als Flag einsetzen und dann mit einem anderen compiler
> weiter bauen."

**Body-Strategie:**
- s2 (jetzt): rotaki/masstree-wrapper Template-Pattern eingebettet als Standalone
  Skelett (Wrapper kann mit Pseudocode-Body kompilieren ohne masstree-beta tatsaechlich
  zu linken — fuer Tests)
- s4 (Folge-Sprint): echte `#include "masstree/masstree.hh"` + `basic_table<params>`
  Instantiation + Linker gegen libmasstree.a (10 .cc-Files + globale Symbole)

**Multi-File Tool-Mapping (manifest.txt):**
Tool wird 4 verschiedene Headers durchsuchen statt nur masstree.hh:
- find_insert → masstree_insert.hh
- get         → masstree_get.hh
- find_locked → masstree_remove.hh
- destroy     → masstree.hh
