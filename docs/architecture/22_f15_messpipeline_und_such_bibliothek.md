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

## 3. Reproduzierbares empirisches Resultat (2026-05-29) — volle Paradigmen-Palette

10 auto-materialisierte Permutations-DLLs (search_algo variiert über ALLE Such-Paradigmen),
128 Latenz-Samples/DLL, alpha=0.05. Ranking nach mittlerer Latenz (`comdare-f15-compare`-Ausgabe):

| Rang | search_algo | Paradigma | mean (ns) |
|------|-------------|-----------|-----------|
| #1 | Array65535 | dense direct-addressed (u16) | 12 614 |
| #2 | Array256 | dense direct-addressed (u8) | 13 420 |
| #3 | HashSearchAlgo | open-addressing Hash, O(1) | 31 993 |
| #4 | InterpolationSearchAlgo | verteilungsbewusst | 35 901 |
| #5 | EytzingerSearchAlgo | cache-conscious BFS-Layout | 46 247 |
| #6 | VectorU8U8 | sorted lower_bound | 159 273 |
| #7 | VectorU16U16 | sorted lower_bound | 167 334 |
| #8 | LinearScanSearchAlgo | unsortiert linear, O(n) | 183 820 |
| #9 | KArySearchAlgo | k-ary/SIMD-Partition | 190 157 |
| #10 | SkipListSearchAlgo | probabilistische Struktur | 233 812 |

**Spanne langsamste/schnellste = 18,5×.** Interpretation für diesen Workload (kleiner u8/u16-Keyraum,
lookup-lastig): dense Arrays dominieren; Hashing ist konkurrenzfähig (Rang 3); die smarten Such-Methoden
(Interpolation, Eytzinger) schlagen die naive sorted-vector-Suche deutlich; KAry/SkipList/Linear sind
für diese kleine Datenmenge am langsamsten. Beim paarweisen Vergleich gegen eine dense-Baseline sind
alle vergleichsbasierten Verfahren Holm-FWER-korrigiert signifikant langsamer (p von ~1e-8 bis ~1e-130).
Das beantwortet F15 konkret + quantitativ: die CacheEngine macht die Wirkung der Achsen-Wahl MESSBAR —
hier ~18,5× zwischen bester und schlechtester Such-Strategie bei identischem std::map-Interface.

**Reproduktion:** Doku 21 §6 (2-Pass-Build) + `comdare-f15-compare build/.../generated/r5g_autobuilt_modules`.
(Hinweis: das obige 10-Zeilen-Ranking stammt aus dem 10-Paradigmen-Lauf; das Messset ist seither auf
12 erweitert — siehe §3.1.)

### 3.1 Addendum (2026-05-29): Messset auf 12 Paradigmen erweitert + interpretierbare Labels

Der adhoc_emitter-Pilot-Raum (apps/adhoc_emitter) variiert nun **12** search_algo-Varianten
(SA0..SA11): die bisherigen 10 + **SA10 BinarySearchTreeSearchAlgo** (unbalancierter BST) +
**SA11 BTreeSearchAlgo** (balancierter, block-orientierter Mehrwege-B-Baum, t=4). Damit misst F15 die
geordnete Struktur in allen drei Balance-Auspraegungen (unbalanciert · probabilistisch · deterministisch+
block-orientiert) — der Effekt von Balancierung und Cache-Block-Orientierung wird am einheitlichen
std::map-Interface direkt quantifizierbar.

**Interpretierbarkeits-Fix (Loader):** `AnatomyModuleLoader::load_all` sortiert die Permutations-DLLs
jetzt **numerisch** nach dem `..._auto_<N>`-Suffix statt lexikographisch. Vorher sortierte `_10`/`_11`
zwischen `_1` und `_2`, sodass der F15-Label-Index (Lade-Reihenfolge) nicht mehr dem Permutations-/
SA-Index entsprach — bei 0..9 fiel das nur zufaellig nicht auf. Jetzt gilt deterministisch
`AdHocComposition_i ↔ SAi`, das Ranking ist ohne Umrechnung lesbar (verifiziert: 11/11 loader-Tests +
FullF15DriverOverRealDlls ueber alle 12 DLLs gruen).

**Empirischer Befund (12 DLLs, ops=20000, batches=128, seed=42; alle 11 Nicht-Baseline Holm-signifikant):**
Stabil ueber Laeufe sind die Extreme — InterpolationSearchAlgo (SA5) am schnellsten, der sortierte
u16-Vektor (SA2, O(n)-Insert-Shifts) am langsamsten, Spanne ~85×; die Mittelfeld-Raenge variieren
wall-clock-bedingt zwischen Laeufen (der Seed steuert die Keys, nicht das CPU-Timing → absolute
ns-Werte sind nicht bit-reproduzierbar, die Signifikanz-Aussage und die Extrem-Ordnung schon).
Bemerkenswert + ehrlich: bei diesem **in-memory-Zufalls-Workload mit kleinem uint16-Keyraum** schlaegt
der simple BST (SA10) den B-Baum (SA11) deutlich — der theoretische B-Baum-Vorteil (flachere Baeume,
weniger Cache-Misses) materialisiert sich erst bei Block-/Disk-orientiertem Zugriff, sehr grossen
Datenmengen oder degeneriert-sortiertem Insert, nicht bei kleinen random-in-memory-Lasten, wo die
hoeheren Knoten-Konstanten (Array-Shifts, komplexeres CLRS-Delete) dominieren. Genau diese
Differenzierung sichtbar und messbar zu machen ist der Zweck der Achsen-Bibliothek (F15).

### 3.2 Addendum (2026-05-29): R5.B — ZWEITE Mess-Dimension (search_algo × allocator)

Die These verlangt zwei Mess-Dimensionen: individuelle Achsen UND ganze Algorithmen. Bisher uebte
`run_workload` nur die search_algo-Achse aus → nur EINE Dimension. R5.B macht eine zweite Achse
operativ:

1. **Behavioral-distinkter Allocator** (Voraussetzung): `PoolResourceAllocator` (eigener
   `std::pmr::unsynchronized_pool_resource`) ist der erste axis_06-Wrapper, der sich OHNE externes
   Vendor-Linking real von System-malloc unterscheidet (alle uebrigen fallen ohne Linking auf
   `portable_aligned_alloc` = System zurueck → eine Variation waere hohl gewesen).
2. **Komposit-`run_workload`** (`abi_adapter.hpp`): Segment 1 = search-Lookups (search_algo-Achse),
   Segment 2 = 2048 alloc/dealloc je Batch ueber `composition_t::allocator` (allocator-Achse). Die
   Batch-Latenz misst damit die Komposition entlang BEIDER variierten Achsen.
3. **2-Achsen-Pilot** (adhoc_emitter): `search_algo` (12) × `allocator` {StdMalloc, PoolResource} =
   **24 Permutationen**. Interpretierbar via emittiertem `manifest.txt` (idx → search+allocator, in
   exakter `for_each_composition_type`-Reihenfolge; gerade idx = std_malloc, ungerade idx =
   pool_resource) — ohne `composition_name`/ABI zu aendern.

**Empirisches Resultat (24 DLLs, ops=20000, batches=128, seed=42; alle 23 Nicht-Baseline
Holm-FWER-signifikant: 6 schneller / 17 langsamer / 0 nicht-signifikant):** Der Pool-Allocator schlaegt
System-malloc bei den meisten Such-Algos KONSISTENT um ~2–3× (array256 565µs→200µs = 2,8×; k_ary
1,21ms→375µs = 3,2×; bst 365µs→181µs = 2,0×; vector_u16u16 1,27ms→561µs = 2,3×) — die alloc/dealloc-
Churn-Phase profitiert von den Size-Class-Free-Lists des Pools gegenueber dem per-call-Overhead von
malloc. Schnellste Komposition absolut: idx 21 = **bst + pool_resource** (181µs). Ehrliche Einschraenkung:
zwei Messungen (idx 13 eytzinger+pool, idx 14 skip_list+std, beide ~21,6ms) sind Wall-Clock-Ausreisser
(inkonsistent mit ihren Allocator-Paaren) — dieselbe Nicht-Bit-Reproduzierbarkeit wie in §3.1; die
konsistente ~2–3×-Pool-vs-System-Differenz ueber die uebrigen Such-Algos ist der belastbare Befund.

Damit ist die **zweidimensionale** Messung demonstriert: search-Dimension (~119× Spanne ueber Such-Algos)
UND allocator-Dimension (~2–3× Pool-vs-System je Such-Algo) — beides an EINEM `std::map`-Interface, pro
Achse UND in der Komposition messbar. R5.B ist damit fuer die allocator-Achse erfuellt.

**Praezisierung (verifiziert 2026-05-29) — warum die allocator-Achse der Sonderfall war:** Die
allocator-Achse besass BEREITS eine aufrufbare Laufzeit-API (`allocate`/`deallocate`) UND eine
off-the-shelf verhaltens-distinkte Implementierung (`std::pmr::unsynchronized_pool_resource`). Eine
Code-Inspektion der uebrigen Achsen zeigt: `memory_layout` (axis_05), `serialization` (axis_10) u. a.
sind **compile-time-Trait/Tag-Typen** — ihre Concepts verlangen nur statische Properties (`cache_line_size()`,
`supports_compression()`, `name()` …), es gibt KEINE aufrufbare `store/load`- bzw. `serialize`-Methode.
Eine Variation dieser Achsen in `run_workload` waere daher aktuell HOHL (kein messbarer Verhaltensunterschied).
Die Erweiterung von R5.B auf eine 3. Achse erfordert deshalb ZUERST, die jeweilige Achse runtime-operativ
zu machen (pro Achse eine behavioral-tragende Laufzeit-API + Wrapper mit echtem Verhaltensunterschied) —
das ist substantielle Achsen-Neugestaltung (R5.B-Voll / R5.C), kein blosses Wiederholen des
allocator-Musters. Die zweidimensionale Messung IST mit search × allocator vollwertig belegt; weitere
Dimensionen sind ein klar abgegrenztes Folge-Arbeitspaket. (→ §3.3 setzt genau das fuer memory_layout um.)

### 3.3 Addendum (2026-05-29): DRITTE Achse memory_layout runtime-operativ + ehrliches Mess-Limit

Umsetzung der in §3.2 beschriebenen Achsen-Operativierung fuer `memory_layout` (axis_05):
1. **Echte Laufzeit-API** in allen 5 Layout-Wrappern: `scan_field_sum(buf, n, record_size)` summiert je
   Datensatz ein Feld im LAYOUT-CHARAKTERISTISCHEN Zugriffsmuster — AoS-strided (`i*record_size`,
   CacheLineAligned/AoSStrict), SoA-contiguous (`i*4`), AoSoA-blocked (Block-Breite 8), Packed-2-Byte.
   Damit ist die Achse nicht mehr trait-only. Korrektheit verifiziert (`R5B_Axis05_ScanFieldSum`,
   axis_05-Suite 16/16): jedes Layout liest exakt seine Soll-Positionen.
2. **run_workload Segment 3** ruft `composition_t::memory_layout::scan_field_sum` auf einem 1-MB-Puffer
   (via Komposition-Allocator alloziert). 3-Achsen-Pilot: search (12) × allocator (2) × memory_layout (2)
   = **48 Permutationen**, Manifest um die Layout-Spalte erweitert.

**Ehrliches Mess-Ergebnis (48 DLLs, ops=20000, batches=128, seed=42):** Die **allocator**-Dimension
bleibt auch in der 48-DLL-Messung sauber aufloesbar (z. B. array256/AoS: std 589µs vs pool 219µs = 2,7×).
Die **memory_layout**-Dimension dagegen ist mit Wall-Clock auf dieser Maschine **NICHT** zuverlaessig
aufloesbar: die SoA/AoS-Verhaeltnisse je (search,allocator)-Paar streuen von 0,07× bis 9,18× ohne
konsistentes Vorzeichen — der echte Cache-Layout-Effekt (~zweistellige µs) liegt UNTER der
Wall-Clock-Rausch-Schwelle der Maschine (hunderte µs bis ms, dieselben Ausreisser wie §3.1). Das ist
ein belastbares **Negativ-/Limit-Ergebnis, kein Layout-Effekt-Nachweis** — bewusst NICHT als solcher
ausgewiesen.

**Schlussfolgerung (motiviert R5.D):** Grob-granulare Achsen mit per-Operation-Overhead im µs-ms-Bereich
(allocator: malloc-Call vs Pool-Free-List) sind wall-clock-messbar; fein-granulare Cache-Layout-Effekte
(AoS vs SoA) erfordern **Hardware-Performance-Counter** (Cache-Miss/L2-MPKI via PMC) statt Wall-Clock.
Die Achsen-MASCHINERIE ist damit fuer 3 Achsen end-to-end operativ + verifiziert; die saubere
QUANTIFIZIERUNG fein-granularer Achsen ist das konkrete Ziel von R5.D (PMC-Integration).

---

## 4. Stand der Architektur-Anforderungen (V41.F.6.1)

ERFÜLLT + verifiziert: Plugin-Controller/Prüfling-Slot (E11-AbstractFactory) · 3-Stufen-Dreigliedrigkeit
(F.5) · R5.G Auto-Materialisierung + Skalierung · Such-Bibliothek (axis_03a: 17 Wrapper, 12 CE-native via F15 gemessen) · R7.3 Queuing+
Concurrency · R7.4 Allocator-Adapter · A4 AoSoA-Layout · G.1 Build-Hierarchie · komplette F15-Mess-
Auswertung + CLI + empirisches Resultat · **R5.B Mess-Dimensionen: search_algo (§3.1) × allocator (§3.2,
wall-clock-aufloesbar) × memory_layout (§3.3, Achse runtime-operativ + verifiziert)** — 3-Achsen-Maschinerie
end-to-end (48 DLLs).

VERBLEIBEND (Mehr-Session / gated / user-manuell): **R5.D Hardware-Counter (PMC)** — fuer fein-granulare
Achsen (memory_layout AoS/SoA) ist Wall-Clock NICHT ausreichend (§3.3-Limit-Befund: Effekt unter
Rausch-Schwelle); PMC/Cache-Miss-Zaehler sind dafuer noetig · R5.B-Erweiterung auf weitere Trait-Achsen
(serialization/… analog §3.3 erst runtime-operativ machen) · voller kartesischer Mehr-Achsen-Raum-Build ·
F.2/F.3 Namespace-Restrukturierung · E11-Master-Facade +
E10 per-Untermodul-STATIC/SHARED (gated auf E4.1-Submodul-Befüllung) · weitere Tree-STRUKTUR-Paper ·
D1/D2 Diplomarbeit-Volltext (Autor).
