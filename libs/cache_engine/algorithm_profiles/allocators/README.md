# Allokator-Profile (V26.A)

**Eingefuehrt:** V26.A (2026-05-14) — analog SOTA-Algorithmus-Profile in `../sota/`

## Schema

Pro Allokator (A01-A23) ein Profile-XML mit:
- `id`, `family_ref` (A-Nummer aus dem `ext/allocator/A*`-Verzeichnis)
- `metadata` (Autoren, Lizenz, Quelle)
- `axes`: granularity, numa, thread_local, fragmentation_strategy, thread_safety
- `abi`: C-API + C++-Overload
- `expected_workload`: Default-Workload-Affinitaet (YCSB_A..F)

Jede Akte traegt im Kopf den Vermerk `DOKU-AKTE (#48 Scheibe 2, 2026-07-22): kein Code-Leser;
Allocator-Wahl = CT-Adapter (Statischer-Dispatch-Doktrin).` Das ist Absicht: die Allokator-Wahl faellt
zur Compile-Zeit ueber CT-Adapter, nicht ueber einen XML-Laufzeit-Leser. Ein Produktions-Parser fuer
dieses Verzeichnis waere ein Runtime-Switch gegen die Doktrin und ist deshalb bewusst NICHT gebaut.

## Zwei Mengen -- bitte nicht verwechseln

Dieses Verzeichnis fuehrt **23 Doku-Akten** (die literaturgestuetzte Abdeckung der Allokator-Achse).
Davon sind **10 implementiert**: nur fuer sie existieren ein Adapter-Skelett unter `adapters/` und
Original-Code unter `ext/allocator/`. Genau diese 10 sind es, die die Diplomarbeit in der Tabelle
`tab:allocator-profiles` als "die zehn lauffaehigen Profile" tabelliert.

Die fruehere Fassung dieser Datei ueberschrieb die 10er-Liste mit "Mitglieder (10)" und verschwieg
damit die 13 reinen Doku-Akten -- die Zahl war nicht falsch, aber sie benannte eine andere Menge als
die Ueberschrift versprach. Beide Mengen stehen jetzt getrennt in der Tabelle (Spalte `impl.`).

## Mitglieder (23)

`impl.` = Adapter-Skelett unter `adapters/` + Original-Code unter `ext/allocator/` vorhanden
(= die 10 in der Diplomarbeit tabellierten Profile). Lizenz-Spalte wortgleich aus dem `<license>`
der jeweiligen Akte; `none` = kein OSS-Code (Lehrbuch-/Kernel-Klassiker), `unknown` = in der Akte
nicht belegt und hier bewusst nicht erraten.

| Profile | Family | Lizenz | expected_workload | impl. |
|---|---|---|---|---|
| hoard | A01 | Apache-2.0 | YCSB_A | ja |
| slab | A02 | none | YCSB_A | -- |
| michael_lockfree | A03 | LGPL-2.1-or-later | YCSB_B | ja |
| mimalloc | A04 | MIT | YCSB_A | ja |
| jemalloc | A05 | BSD-2 | YCSB_B | ja |
| tcmalloc | A06 | Apache-2.0 | YCSB_C | ja |
| snmalloc | A07 | MIT | YCSB_A | ja |
| scalloc | A08 | BSD-3 | YCSB_A | ja |
| numalloc | A09 | unknown | YCSB_B | -- |
| rpmalloc | A10 | Public Domain | YCSB_C | ja |
| lrmalloc | A11 | MIT | YCSB_B | ja |
| cama | A12 | none | YCSB_A | -- |
| starmalloc | A13 | Apache-2.0 | YCSB_B | -- |
| tcmalloc_warehouse | A14 | Apache-2.0 | YCSB_C | -- |
| hmalloc | A15 | none | YCSB_B | -- |
| pim_malloc | A16 | MIT | YCSB_E | -- |
| crystalline | A17 | unknown | YCSB_B | -- |
| exgen_malloc | A18 | unknown | YCSB_C | -- |
| buddy | A19 | none | YCSB_C | -- |
| dlmalloc | A20 | Public Domain (CC0) | YCSB_C | ja |
| ptmalloc2 | A21 | LGPL-2.1+ | YCSB_B | -- |
| pmr_resource | A22 | Standard-Library | YCSB_B | -- |
| vmem_magazines | A23 | none | YCSB_A | -- |

Diese Tabelle wird maschinell geprueft: `tests/unit/test_allocator_profile_bestand.cpp` haelt die
Ueberschrifts-Zahl, die Tabellenzeilen und die `impl.`-Spalte gegen den realen Aktenbestand und gegen
`adapters/`. Wer eine Akte hinzufuegt, muss Zahl UND Zeile hier nachziehen, sonst wird der Test rot --
das ist Absicht, denn ein Verzeichnis ohne Produktions-Leser verrottet sonst unbemerkt.

## Querverweis

- `ext/allocator/REPOS_OVERVIEW.md` (Quell-Repos)
- `adapters/A*` (V26.B Adapter-Skelette; vorhanden fuer die 10 implementierten)
- `ext/allocator/A*` (Original-Code, kompiliert mit Original-Compiler)
- `libs/cache_engine/axes/alloc/PAPER_REFERENCES.md` (Paper, Venue, Jahr, DOI je Wrapper)
