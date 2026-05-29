# Doku 22 — F15-Messpipeline + vollständige Such-Algorithmen-Bibliothek (Stand 2026-05-29)

**Zweck:** elaborate Architektur- + Ergebnis-Skizze für die Diplomarbeit-Kapitel zu (1) dem
Library-Framework der Achsen-Algorithmen und (2) der F15-Messung „bringt die CacheEngine messbaren
Wert?". Liefert die reproduzierbaren Bausteine + ein konkretes empirisches Resultat. (Volltext schreibt
der Autor — diese Doku ist die technische Vorlage, `[[user-manual-workflow]]`.)

Querverweise: Doku 14 (Anatomie/Organ-Metapher), Doku 18 (autoritative Paper-Map), Doku 21 (F.6
Plugin-Controller + 3-Stufen + R5.G), `docs/sessions/20260529-r7x-…` (Session-Log der 22 Einheiten).

---

## 1. Säule 1 — Die austauschbare Achsen-Algorithmen-Bibliothek (Kern-Beitrag)

Die Anatomie umfasst (G.1-Build-Auswertung, `cmake/axis_hierarchy_summary.cmake`):
**15 Topics · 22 Achsen · ~140 Wrapper · Permutations-Raum ≥ 1e15 (kombinatorisch).**

### 1.1 axis_03a search_algo — vollständige Paradigmen-Palette (14 Strategien)

Jeder Algorithmus ist auf das einheitliche `std::map<key,value>`-Vergleichs-Interface zusammen-
geschnitten (`[[std-map-unified-interface]]`); die Achse beschreibt das INNEN-Verhalten der Suche.

| Paradigma | Wrapper | Paper |
|-----------|---------|-------|
| Direct-Address | Array256SearchAlgo, Array65535SearchAlgo | ART (Leis ICDE 2013) |
| Sorted-Vector (lower_bound) | VectorU8U8SearchAlgo, VectorU16U16SearchAlgo | HOT / START |
| Such-METHODE: SIMD-Partition | KArySearchAlgo | Schlegel/Gemulla/Lehner DaMoN 2009 |
| Such-METHODE: verteilungsbewusst | InterpolationSearchAlgo | Perl/Itai/Avni CACM 1978 |
| Such-METHODE: Cache-Layout | EytzingerSearchAlgo | Khuong/Morin JEA 2017 |
| Geordnete Struktur (probabilistisch) | SkipListSearchAlgo | Pugh CACM 1990 |
| Hash (UNGEORDNET, O(1)) | HashSearchAlgo | Knuth TAOCP 3 §6.4 |
| Paper-Original-Trie | OriginalArt/Hot/Start/Surf/Wormhole | ICDE/SIGMOD/EuroSys |

Damit ist das Spektrum comparison-based (dense/sorted/such-methode/struktur) + hash-based +
trie-based vollständig abgedeckt. Achsen-Eigenschaften (`supports_range_scan`, `supports_simd`,
`is_dense`, `density_class`) erlauben Compile-Time-`mp_filter`-Selektion: z.B. ist HashSearchAlgo der
einzige `supports_range_scan()==false`-Wrapper (ungeordnet) — eine distinkte, automatisch filterbare
Klassifikation.

### 1.2 Warum das der Kern-Beitrag ist
Ein Forscher, der nur EINE neue Such-Methode untersucht, muss nicht den ganzen Suchalgorithmus +
alle anderen Achsen neu bauen: er steckt seinen Wrapper in die axis_03a-Registry, und die
Metaprogrammierung (AdHocComposition + PermutationEngine) rekombiniert ihn original-getreu mit allen
übrigen Achsen zu vergleichbaren, mess-fähigen Kompositionen (`[[thesis-core-contribution-axis-library]]`).

---

## 2. Säule 2 — Die F15-Messpipeline (end-to-end, ausführbar)

```
 Achsen-Bibliothek (N search_algo) 
   → AdHocComposition<17 Achsen> (Metaprogramm-Komposition)
   → R5.G Auto-Emitter (apps/adhoc_emitter): enumeriert den Raum, emittiert pro Permutation ein .cpp
   → CMake comdare_build_adhoc_modules: baut JEDE Permutation als SHARED-DLL
   → AnatomyModuleLoader::load_all (dlopen/LoadLibrary)
   → IMeasurableWorkload::run_workload (Stufe B — Messung IN der DLL)
   → make_execution_result (Samples → ExecutionResult + p50/p99 via latency_stats)
   → multi_compare_against_baseline (Welch-t-Test pro Paar + Holm-Bonferroni-FWER-Korrektur)
   → summarize (win_rate = F15-Headline) 
   → result_aggregator / report_to_csv·json (Export)
```

### 2.1 Statistische Säulen (Namespace `builder::commands::stats`, header-only)
- `latency_stats`: Nearest-Rank-Perzentile (p50/p99/min/max/mean), non-mutierend.
- `welch_t_test`: Welch (ungleiche Varianzen) → t, df (Welch-Satterthwaite), zweiseitiger p (Student-t-CDF).
- `multiple_comparison`: Bonferroni + Holm-Bonferroni — FWER-Kontrolle bei m Vergleichen (sonst
  Zufalls-Signifikanzen bei m≫1).
- `multi_compare`: N Kandidaten vs Baseline → Report{name, raw_p, adjusted_p, significant, faster}.
- `summarize`: win_rate = Anteil, der die Baseline FWER-korrigiert signifikant schlägt.

### 2.2 Ausführbares Tool
`apps/f15_compare` (`comdare-f15-compare <dll-dir> [--alpha= --baseline= --ops= --batches= --csv= --json=]`)
verkettet die ganze Pipeline über ein Verzeichnis materialisierter Permutations-DLLs.

---

## 3. Reproduzierbares empirisches Resultat (2026-05-29)

8 auto-materialisierte Permutations-DLLs (search_algo variiert), 128 Latenz-Samples/DLL, baseline =
Array256 (dense direct-addressed, u8), alpha=0.05:

| Komposition | search_algo | Verdict vs Baseline | Holm-p |
|-------------|-------------|---------------------|--------|
| AdHocComposition_0 | Array256 (dense u8) | Baseline (~27,6 µs) | — |
| AdHocComposition_3 | Array65535 (dense u16) | **schneller** | 3e-85 |
| AdHocComposition_1 | VectorU8U8 (sorted) | langsamer | 7e-91 |
| AdHocComposition_2 | VectorU16U16 (sorted) | langsamer | 9e-23 |
| AdHocComposition_4 | KAry (SIMD-Partition) | langsamer | 1e-08 |
| AdHocComposition_5 | Interpolation | langsamer | 3e-14 |
| AdHocComposition_6 | Eytzinger (Cache-Layout) | langsamer | 1e-71 |
| AdHocComposition_7 | SkipList | langsamer | 1e-85 |

**Alle 7 Vergleiche Holm-FWER-korrigiert signifikant.** win_rate = 0,143 (1 von 7 schlägt die Baseline).
Interpretation für diesen Workload (kleiner u8/u16-Keyraum, lookup-lastig): direct-addressed Arrays
dominieren die vergleichsbasierten Verfahren um ~8× — eine messbare, statistisch rigorose Achsen-Wirkung.
Das beantwortet F15 konkret: die CacheEngine erlaubt es, diesen Wert pro Achsen-Wahl MESSBAR zu machen.

**Reproduktion:** Doku 21 §6 (2-Pass-Build) + `comdare-f15-compare build/.../generated/r5g_autobuilt_modules`.

---

## 4. Stand der Architektur-Anforderungen (V41.F.6.1)

ERFÜLLT + verifiziert: Plugin-Controller/Prüfling-Slot (E11-AbstractFactory) · 3-Stufen-Dreigliedrigkeit
(F.5) · R5.G Auto-Materialisierung + Skalierung · Such-Bibliothek (14 Paradigmen) · R7.3 Queuing+
Concurrency · R7.4 Allocator-Adapter · A4 AoSoA-Layout · G.1 Build-Hierarchie · komplette F15-Mess-
Auswertung + CLI + empirisches Resultat.

VERBLEIBEND (Mehr-Session / gated / user-manuell): voller kartesischer Mehr-Achsen-Raum-Build +
Hardware-Counter (PMC) · R5.B (Achsen operativ machen, damit run_workload den GANZEN Algorithmus statt
nur search_algo misst — gated, Doku 21 §5) · F.2/F.3 Namespace-Restrukturierung · E11-Master-Facade +
E10 per-Untermodul-STATIC/SHARED (gated auf E4.1-Submodul-Befüllung) · weitere Tree-STRUKTUR-Paper ·
D1/D2 Diplomarbeit-Volltext (Autor).
