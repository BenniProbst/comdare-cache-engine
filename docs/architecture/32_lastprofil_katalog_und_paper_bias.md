# Lastprofil-Katalog (33-Paper-Analyse) + Paper-Bias

**Direktive (User 2026-06-08):** Jedes Paper wählt ein Lastprofil, das SEINEN Algorithmus gut dastehen lässt.
Um diesen Bann zu brechen, müssen **ALLE Lastprofile über ALLE Achsen/Lebewesen** laufen (Workload = dynamische
Achse 2 im B+-Baum). Quelle: 33-Paper-Workflow (`wn7b2fu44`, 34 Agenten), Roh-Befunde je Paper im Workflow-Output.
Bezug Memory `feedback_all_papers_loadprofiles_xml_all_axes`.

## 14 distinkte Lastprofile (XML-Kandidaten → `load_profiles/*.xml`)

| LP | id | op-mix (ins/look/era/scan/rmw) | key-dist | neg% | records / n_ops | Sweep | abgeleitet aus |
|----|----|------|------|------|------|------|------|
| LP01 | bulk-insert-dense | 100/0/0/0/0 | uniform-seq | 0 | 10M/10M | records 100..30M | P01,P02-build,P03,P05,P12/14 |
| LP02 | bulk-insert-sparse-prng | 100/0/0/0/0 | uniform-rand | 0 | 10M/10M | — | P01,P02,P08 |
| LP03 | insert-value-length-sweep | 100/0/0/0/0 | uniform-seq | 0 | 1M/1M | value_len [1..10000B] | P01,P08 |
| LP04 | static-read-positive-uniform | 0/100/0/0/0 | uniform | 0 | 10M/100M | — | P02,P05,P11,P12/14,P16,P25 |
| LP05 | static-read-zipfian | 0/100/0/0/0 | zipfian(.99) | 0 | 100M/200M | node_size | P10,P20,P25,P28 |
| **LP06** | **static-read-negsweep** | 0/100/0/0/0 | uniform | **{0,25,50,75,100}** | 1M/3M | **neg% (primär)** | **P04-CoCo,P06,P22** |
| LP07 | static-read-real-string-corpus | 0/100/0/0/0 | real-corpus | 50 | 1M/3M | datasets | P04,P06,P10 |
| LP08 | range-scan-heavy | 0/0/0/100/0 | uniform-seq | 0 | 16M/16M | records 128..256M | P01,P10,P12/14,P32 |
| LP09 | insert-lookup-balanced-5050 | 50/50/0/0/0 | uniform | 0 | 10M/20M | — | P03-Masstree,P29 |
| LP10 | delete-heavy-post-build | 50/0/50/0/0 | uniform-rand | 0 | 1M/1M | (erase=100 Variante) | P01,P08 |
| LP11 | mixed-ycsb-oltp | 25/50/0/0/25 | zipfian(.99) | 0 | 10M/60M | read_ratio {0..100} | P20,P10,P19 |
| LP12 | concurrent-rmw-stresstest | 25/9/36/9/9 | uniform | 0 | 100K/10M | threads {1..16} | P07-Wormhole,P29 |
| LP13 | concurrent-read-scaling | 0/100/0/0/0 | uniform | 0 | 10M/200M | threads {1..128} | P07,P08,P25,P28 |
| LP14 | dynamic-realistic-trace | 9/91/0/0/0 | tpcc-nonuniform | 0 | 1M/100K | tree_size | P19,P20 |

> Sweeps mit Bezug zu BESTEHENDEN dynamischen Achsen falten dorthin: `threads` (LP12/13) → bestehende
> concurrency.thread_count-Dim; `node_size`/`value_len` → Layout-/Allocator-Achse. Die **Workload-Achse 2** trägt
> die 14 Basis-Profile + die profil-DEFINIERENDEN Sweeps (LP06 neg% ×5, LP11 read_ratio ×5).

## 13 Archetypen
static-build-read-positive · static-read-negsweep · point-lookup-zipfian · range-scan-heavy · write-heavy-bulk ·
insert-lookup-5050 · delete-heavy-post-build · mixed-ycsb-oltp · concurrent-rmw-stress · concurrent-read-scaling ·
real-string-corpus-lookup · dynamic-mixed-trace · not-applicable (Survey/Theorie/HW).

## Paper-Bias (der Kern-Befund — wissenschaftliche Rechtfertigung des Cross-Runs)
1. **Static-read-only-Trie/Filter** (HOT/CoCo/START/B2tree/SuRF/CSS/Kuehn): rahmt „build-once, 100% positive
   Lookups", meidet Updates → begünstigt immutable/komprimierte Strukturen (bei Updates teuer). HOT erklärt seinen
   Workload explizit „statisch-read-only" TROTZ vorhandener ROWEX-Inserts.
2. **Negativ-Query-Sweep als Heimspiel** (CoCo/B2tree/SuRF): macht `negative_query_pct` zur Primär-Achse — weil
   komprimierte Tries/Filter bei Misses früh abbrechen (LCP-Mismatch/Filter-Reject). B+-Baum ohne Filter ist hier
   strukturell benachteiligt → Cross-Run über neg%-Stufen für ALLE Achsen ist kritisch.
3. **Zipfian/Cache-Heimspiel** (LeanStore/Mahling/Kuehn): zipfian(.99) auf engem Hot-Range begünstigt
   Cache-bewusste Reordering-Layouts (Kuehn ordnet Baum nach Leaf-Frequenz um — Workload IST auf die Optimierung
   zugeschnitten). Uniform zerstört den Hotpath-Vorteil.
4. **Write/Update-Heimspiel** (ART/Masstree/Wormhole/OLC-ART/LeanStore/RCU): insert-heavy / 50:50 / concurrent-mixed
   mit Thread-Scaling (1..128) — wo statische Strukturen gar nicht antreten.
5. **Prefetch/HW-Heimspiel** (CSB/Chen-Fractal/Schmidt): Sweeps über node_size + prefetch on/off + stride — misst
   Cache/TLB statt Algorithmus-Logik.
6. **14 N/A-Paper** (P09,P13,P15,P16-theory,P17,P18,P21,P23,P24,P26,P27,P30,P31,P33): kein messbarer Index-Workload
   (Survey/Theorie/HW-Prefetch/Reclamation) — liefern Achsen-KONZEPTE (LOUDS, cache-oblivious, prefetch-distance),
   KEIN eigenes Lastprofil.

**FAZIT:** Jedes der 14 Profile MUSS über ALLE Lebewesen laufen. Besonders: read-only-Trie-Algorithmen auch unter
LP01/LP10/LP11/LP12 (insert/delete/rmw/concurrent); dynamische Engines auch unter LP04/LP06/LP07 (static-read,
neg-sweep, real-corpus) — sonst bleibt jeder „Sieger nur im selbstgewählten Heimprofil".

## Scope-Entscheidungen (offene Fragen, vorab im uint64-B+-Baum-Scope aufgelöst)
- **Real-String-Corpora (LP07, Q5/Q8):** vorerst AUSSER Scope (unser Op-Modell ist uint64-keys); `key_distribution`
  bleibt {uniform,zipfian,latest}. LP07 + lognormal/arbitrary = späterer Dataset-Loader-Slot (flag, nicht jetzt).
- **Zeit-gebunden (LP09/LP12, Q2):** deterministisch auf feste n_ops + Seed gemappt (keine Sekunden-Bindung).
- **RMW (Q3):** unser `rmw` = lookup→modify→insert-overwrite (host-seitig), bereits definiert.
- **Scan (Q4):** `scan_length_max`; `full-iteration` = `records`-Sentinel.
- **value_length (LP03, Q6):** Workload-Dimension, später (beeinflusst Space-Observer) — vorerst fester Wert.
- **threads (LP12/13):** falten in die bestehende concurrency.thread_count-Dim, KEINE eigene Workload-Variante.
- **N/A-Paper-Konzepte (Q9):** sind ACHSEN (prefetch/layout/LOUDS), bereits im Achsen-System — kein Lastprofil.
