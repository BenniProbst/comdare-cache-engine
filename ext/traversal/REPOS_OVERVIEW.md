# Traversal Repositories Overview (Phase V41.F.6.1.struct ext/-Reorganisation)

**Stand:** 2026-05-26 nach ext/-Topic-Reorganisation
**Pflicht-Klone abgeschlossen:** 12 von ~15 Such-Algorithmus-Papern (P01-P30)
**Roll-out Status:** 5/12 als Original-Wrappers integriert (P01/P02/P05/P07/P10), 1 deferred (P03), 6 pending

## Geklonte Repositories (Pflicht, Einbindung als regulaerer Source-Snapshot — Pattern A)

| ID | Verzeichnis | Repo | Lizenz | Wrapper-Status | Originall |
|----|-------------|------|--------|:--------------:|:---------:|
| **P01** | `P01-ART/unodb/` | [laurynas-biveinis/unodb](https://github.com/laurynas-biveinis/unodb) | MIT | ✅ OriginalArtSearchAlgo | 4/4 |
| **P02** | `P02-HOT/hot/` | [speedskater/hot](https://github.com/speedskater/hot) | BSD-3 | ✅ OriginalHotSearchAlgo | 2/4 |
| **P03** | `P03-Masstree/masstree-beta/` | [kohler/masstree-beta](https://github.com/kohler/masstree-beta) | MIT | ⚠️ DEFERRED → s3 Batch 2 mit Template-Adapter (rotaki-Pattern) | (1/4 via Template) |
| **P04** | `P04-CoCo-trie/CoCo-trie/` | [aboffa/CoCo-trie](https://github.com/aboffa/CoCo-trie) | MIT | DEFERRED — Read-Only Bulk-Loaded Index, 0/4 originall (Pseudocode-Pattern) | 0/4 |
| **P05** | `P05-START/START/` | (Mertens et al. ICDE 2024 internal) | TBD | ✅ OriginalStartSearchAlgo | 2/4 |
| **P06** | `P06-B2tree/{b2-tree-master,bart-master}/` | TUM-DB (no-license) | research | PENDING — Audit fuer Function-Bodies + Lizenz-Klaerung | (?) |
| **P07** | `P07-Wormhole/wormhole/` | [wuxb45/wormhole](https://github.com/wuxb45/wormhole) | GPL-3 | ✅ OriginalWormholeSearchAlgo | 3/4 |
| **P10** | `P10-SuRF/SuRF/` | [efficient/SuRF](https://github.com/efficient/SuRF) | Apache-2.0 | ✅ OriginalSurfSearchAlgo | 1/4 |
| **P20** | `P20-BTreesAreBack/leanstore/` | [leanstore/leanstore](https://github.com/leanstore/leanstore) | MIT | PENDING — substantielles Repo, Audit fuer minimal-Insert/Lookup-API |
| **P25** | `P25-Mahling/prefetching/` | (Mahling et al.) | TBD | DEFERRED — Prefetching-Paper, gehoert eigentlich zu `prefetch/`-Topic nicht traversal |
| **P29** | `P29-RCU/userspace-rcu/` | [urcu/userspace-rcu](https://github.com/urcu/userspace-rcu) | LGPL-2.1 (mit BSD-2 Bibliotheken) | DEFERRED — gehoert eigentlich zu `concurrency/`-Topic nicht traversal |
| **P30** | `P30-HazardPointers/haz_ptr/` | (no-license, akademisch) | research | DEFERRED — gehoert zu `concurrency/`-Topic; HazPtrSlice/Holder/Domain Pattern (Michael 2002) |

**Total Disk:** ~varies (mit `--depth 1` schmal gehalten)

## Lizenz-Kompatibilitaet

✅ **MIT/BSD-2/BSD-3/Apache-2.0**: P01/P02/P03/P04/P10/P20 — direkt kompatibel mit comdare Apache-2.0
⚠️ **GPL-3 (P07 Wormhole):** copyleft — separate compilation/linkage moeglich aber rechtlich pruefen vor commercial use
⚠️ **LGPL-2.1 (P29 RCU):** Header-only/static-link inkompatibel mit closed-source distribution — fuer Diplomarbeit akademisch unbedenklich
⚠️ **No-license (P06 B²tree, P25 Mahling, P30 HazardPointers):** TUM-internal/Forschungs-Code — fuer Diplomarbeit-Mess-Reihen OK (Architekt-Direktive 2026-05-14 NOTICE), KEIN commercial redistribute

## Wrapper-Klassen (V41.F.6.1.P2.D.tr.s2 + s3)

| Family | Wrapper-Klasse | Mixin-Pattern | Compiler |
|:------:|----------------|:-------------:|:--------:|
| S04 | `OriginalArtSearchAlgo` | 4/4 Full-Original | gcc-9.5 |
| S05 | `OriginalHotSearchAlgo` | 2/4 Partial (insert+lookup, erase+clear Re-Impl) | gcc-9.5 |
| S06 | `OriginalStartSearchAlgo` | 2/4 Partial | gcc-9.5 |
| S07 | `OriginalWormholeSearchAlgo` | 3/4 (insert+lookup+erase, clear Re-Impl) | gcc-9.5 |
| S08 | `OriginalSurfSearchAlgo` | 1/4 (Read-Only, nur lookup) | gcc-9.5 |
| S09 | `OriginalMasstreeSearchAlgo` (PENDING) | siehe Multi-File-Mapping-Plan unten | gcc-9.5 |

## Architektur-Hinweis (KRITISCH — User-Direktive 2026-05-26)

> "Eine search algo Achse innerhalb der Search Algo Permutation ist Schwachsinn,
> es ist nicht moeglich diese Komponente in unserem derzeitigen Bottom Up Ansatz
> spaeter wieder zu verknuepfen."

**Konsequenz:** Die existing monolithischen Original-Wrappers (S04-S08) sind als
**Stufe-2 Pruefling-Referenz-Adapter** zu verstehen (kompletter Algorithmus als
Habich-Validation-Baseline), NICHT als Stufe-1 CE-Permutations-Komponenten.

**Folge-Sprint geplant:** axis_03a_search_algo Restrukturierung in feingranulare
Sub-Achsen (Node-Type / Layout / Concurrency / etc.), sodass ART/HOT/Wormhole/etc.
als **Compositions** der Sub-Achsen rekonstruierbar sind. Siehe Doku 13 Teil J
(P2.D.tr.s3 Batch 1 + Architektur-Kritik).

## Masstree Multi-File-Mapping (P03 Spezial-Pattern)

Masstree-Templates haben Out-of-Class-Definitionen verteilt ueber mehrere Header:

| Function | Source-File | Notiz |
|---|---|---|
| `bool basic_table<P>::get(...)` | `masstree_get.hh:60-68` | NICHT in masstree.hh |
| `tcursor<P>::find_insert(...)` | `masstree_insert.hh` | Templated, Cursor-Pattern |
| `tcursor<P>::find_locked(...)` | `masstree_remove.hh` | |
| `unlocked_tcursor<P>::find_unlocked(...)` | `masstree_tcursor.hh` | Lock-free Read-Path |

**Roll-out Plan s3 Batch 2:** Multi-File-Manifest mit allen 4 Pfaden + Wrapper
nach rotaki/masstree-wrapper Pattern (gold-standard, MIT). Tool-Audit-Erweiterung
fuer Out-of-Class-Template-Definitions noetig.

## Pending Roll-out (Batch 2+ Kandidaten)

1. **P03 Masstree** (Template-Adapter via rotaki-Pattern, 1/4 ueber get)
2. **P04 CoCo-trie** (Pseudocode-Pattern, 0/4 weil Read-Only Construct-Only)
3. **P06 B²tree** (Audit pending — substantielles TUM-Repo)
4. **P20 BTreesAreBack/leanstore** (Audit pending — minimal-API extrahieren)
5. **P25 Mahling** → **Topic-Migration zu prefetch/**
6. **P29 RCU** → **Topic-Migration zu concurrency/**
7. **P30 HazardPointers** → **Topic-Migration zu concurrency/**
