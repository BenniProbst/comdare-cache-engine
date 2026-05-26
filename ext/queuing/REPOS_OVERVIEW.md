# Queuing Repositories Overview (V41.F.6.1.P2.D.q.s2 Pilot-Sprint)

**Stand:** 2026-05-26 nach P2.D.q.s2 Pilot (concurrentqueue Submodule)
**Pflicht-Klone:** 1 von ~4 geplanten Queuing-Repos (Pilot-Phase)
**Einbindung:** Pattern B (git submodule, header-only — analog Allokator-mimalloc)

## Geklonte Repositories

| ID | Verzeichnis | Repo | Lizenz | Submodule? | Wrapper-Status | Originall |
|----|-------------|------|--------|:----------:|:--------------:|:---------:|
| **Q01** | `Q01-concurrentqueue/` | [cameron314/concurrentqueue](https://github.com/cameron314/concurrentqueue) | BSD-2 Simplified | ✅ git submodule (header-only) | ✅ OriginalLockFreeMpmcConcurrentQueue (Q15) | 2/6 |

**Total Disk:** ~500K (concurrentqueue header-only, kein Build-System extern noetig)

## Geplante Repos (Folge-Sprint P2.D.q.s2.t2 — User-Aktion `git submodule add` noetig)

| Repo (URL-Vorschlag) | Lizenz | Vorgesehene Wrappers | Notiz |
|---|---|---|---|
| [facebook/rocksdb](https://github.com/facebook/rocksdb) | Apache 2.0 / GPL-2 dual | SkiplistBuffer + AdaptiveLsmFlush | substantieller Build, viele deps |
| [wangziqi2013/BwTree](https://github.com/wangziqi2013/BwTree) (akademisch) | MIT-aehnlich | DeltaChain | mittel-komplex |
| [kohler/masstree-beta](https://github.com/kohler/masstree-beta) | MIT | EpochBuffer (QSBR-Pattern) | mittel, gemeinsam mit traversal/P03 Masstree |
| [google/leveldb](https://github.com/google/leveldb) (optional) | BSD-3 | SkiplistBuffer Alt-Variante | redundant zu RocksDB |

## Lizenz-Kompatibilitaet (Pilot)

✅ **BSD-2 (Q01 concurrentqueue):** direkt kompatibel mit Apache-2.0

## Wrapper-Klassen (V41.F.6.1.P2.D.q.s2)

| Family | Wrapper-Klasse | Mixin-Pattern | Compiler | API-Mapping |
|:------:|----------------|:-------------:|:--------:|---|
| Q15 | `OriginalLockFreeMpmcConcurrentQueue` | 2/6 Partial (put+get, 4 Luecken: emplace+peek_front+peek_back+clear) | gcc-9.5 | put→enqueue, get→try_dequeue |

## Architektur-Hinweis

Queuing-Topic ist der **erste Topic** mit echter Pattern-B-Einbindung (git submodule)
fuer externe Paper-Sources. Konvention: header-only Repos bevorzugt (minimal Build-Aufwand,
kein add_library), komplexe Repos (RocksDB) als Folge-Sprint mit Compiler-Cache + Library-Build.

## Cross-Reference zu Doku

- Doku 13 Teil G §39: Audit-Ergebnis P2.D.q (0 ext-Sources im Allocator-Stil)
- Doku 13 Teil H §43-§48: ext/-Reorganisation + Pattern A/B Definition
- Doku 13 Teil I §49-§52: P2.D.q.s2 Pilot Endstand
