# axis_08_concurrency — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** Mix — 1× A (standalone is_original-faehig: OLC via unodb, Apache-2.0) + Rest D/E (Theorie-Anker-Paper ohne dediziertes is_original-Linking: Baselines, generische CAS-/Seqlock-Pattern, RCU/Hazard-Pointer als lokale Konzept-Adaptionen).

## §1 Pflicht-Note
Nur `OlcOptimisticConcurrency` hat echtes is_original-Linking-Potenzial (OSS, permissiv Apache-2.0, unodb-Original, P08). Alle uebrigen Wrapper sind Re-Impl/Baselines: `NoneConcurrency` und `BlockingConcurrency`/`ReaderWriterConcurrency` sind synchronisationsfreie bzw. lehrbuch-klassische Lock-Baselines; `LockFreeConcurrency`/`WaitFreeConcurrency` sind generische CAS-/Seqlock-Pattern mit Theorie-Ankerpaper (kein Original-Code); `RcuConcurrency` (P29, LGPL-2.1-or-later) und `HazardPointerConcurrency` (P30, NO LICENSE) referenzieren lokale Forschungs-Codes, sind aber `is_original = ✗` (P30 lizenz-blockiert mangels Lizenz).

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| NoneConcurrency | No synchronization (single-threaded baseline) | — | — | — | nein | none | ✗ |
| BlockingConcurrency | Coarse-grained mutex (pessimistic locking) | Solution of a Problem in Concurrent Programming Control | CACM 8(9) 1965 | 10.1145/365559.365617 | nein | none | ✗ |
| ReaderWriterConcurrency | Single-writer/multi-reader lock (shared_mutex) | Concurrent Control with "Readers" and "Writers" | CACM 14(10) 1971 | 10.1145/362759.362813 | nein | none | ✗ |
| OlcOptimisticConcurrency | Optimistic Lock Coupling (OLC) | The ART of Practical Synchronization; OLC (Verallgemeinerung) | DaMoN 2016; DEBULL 2019 | 10.1145/2933349.2933352 | lokal (P08) | Apache-2.0 | ✓ |
| LockFreeConcurrency | Lock-free via CAS-Loops (system-wide progress) | Simple, Fast, and Practical Non-Blocking… Concurrent Queue Algorithms | PODC 1996 | 10.1145/248052.248106 | nein | none | ✗ |
| WaitFreeConcurrency | Wait-free synchronization (bounded per-thread steps) | Wait-Free Synchronization; Seqlock-Realisierung | TOPLAS 1991; Gelato 2005 | 10.1145/114005.102808 | nein | none | ✗ |
| RcuConcurrency | Read-Copy-Update (deferred grace-period reclamation) | Read-Copy Update; User-Level Implementations of RCU | OLS 2001; TPDS 2012 | 10.1109/TPDS.2011.159 | lokal (P29) | LGPL-2.1-or-later | ✗ |
| HazardPointerConcurrency | Hazard Pointers (safe memory reclamation, ABA) | Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects | TPDS 15(6) 2004 | 10.1109/TPDS.2004.8 | lokal (P30) | none | ✗ |

## §3 Compliance-Status
Alle 8 Wrapper haben eine Paper-Referenz ODER sind als Baseline/Lehrbuch-Konzept gekennzeichnet → Habich-Pflicht erfuellt:
- **Baseline (kein Paper, bewusster Vergleichs-Nullpunkt):** NoneConcurrency.
- **Lehrbuch-/Klassiker-Anker:** BlockingConcurrency (Dijkstra CACM 1965), ReaderWriterConcurrency (Courtois et al. CACM 1971).
- **Generische Pattern mit Theorie-Anker (Map §5, confidence = medium):** LockFreeConcurrency (Michael/Scott PODC 1996 als Anker; generisches CAS-Pattern, kein einzelner Algorithmus), WaitFreeConcurrency (Herlihy TOPLAS 1991 als Theorie-Anker; generisches Seqlock-Pattern).
- **is_original-Kandidat (Map §3, R7.6.c):** OlcOptimisticConcurrency — Repo github.com/laurynas-biveinis/unodb, Apache-2.0 (P08), `is_original = ✓`. Einziger is_original-faehiger Wrapper dieser Achse.
- **Lokale Forschungs-Codes, aber NICHT is_original:** RcuConcurrency (P29 userspace-rcu/liburcu, LGPL-2.1-or-later) und HazardPointerConcurrency (P30 huangjiahua/haz_ptr) — P30 ist lizenz-blockiert (NO LICENSE), daher kein Original-Linking.
- **Keine §4-Header-Korrekturen** betreffen axis_08 (keine Fehlattribution gelistet).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_08_concurrency, §3 is_original-Kandidaten, §5 offene Punkte, §6 P-ID-Katalog)
- Doku 17 §4.5 (Klassifikation A/C/D/E)
- Lokaler Katalog Forschungsarbeiten/code/REPO_INVENTAR_FINAL.md:
  - P08 — ARTSync / OLC in unodb (Leis DaMoN 2016, Apache-2.0)
  - P29 — userspace-rcu / liburcu (McKenney OLS 2001, LGPL-2.1)
  - P30 — huangjiahua/haz_ptr (Michael TPDS 2004, NO LICENSE)
