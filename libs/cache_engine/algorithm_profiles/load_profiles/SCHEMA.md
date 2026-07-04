# Lastprofil-XML-Schema (`comdare_load_profile`)

**Zweck (User-Direktive 2026-06-08):** Jedes Paper-Lastprofil (die Benchmark-Methodik, mit der ein Paper seinen
Algorithmus gegen andere vergleicht) wird als **XML-Lastprofil** festgeschrieben und zur **Laufzeit interpretiert**
(`workload_driver::parse_load_profile` → `WorkloadConfig`). Die Lastprofile sind die **Werte der dynamischen
Workload-Achse (Achse 2)** im Experiment-B+-Baum: jedes Tier wird unter **JEDEM** Lastprofil gemessen
(Bias-Bruch — kein Algorithmus nur unter seinem Heim-Workload). Siehe Memory
`feedback_all_papers_loadprofiles_xml_all_axes`.

## Verzeichnis
`libs/cache_engine/algorithm_profiles/load_profiles/*.xml` — eine Datei je Lastprofil; die `id` ist der
Workload-Achsen-Wert (erscheint in der CSV-Spalte `workload` + im `setting_label` als `workload.workload_id=<id>`).

## Katalog-ID (`lp_id`)
`lp_id` ist ein optionales, additives Root-Attribut fuer die Rueckverfolgbarkeit zur Katalog-Nummer `LP01`-`LP14`
aus `docs/architecture/32_lastprofil_katalog_und_paper_bias.md`. Es ist getrennt von `id`: `id` bleibt der
Workload-Achsen-Wert in CSV/`setting_label`, `lp_id` ist nur der Katalog-Tag und aendert keine Messsemantik.

## Bewusst absente Katalog-LP-IDs
| LP-ID | Grund |
|-------|-------|
| LP02 | `bulk-insert-sparse-prng` ist unter dem uint64-Op-Modell in LP01 gefaltet; keine eigene Profildatei. |
| LP03 | `insert-value-length-sweep` ist als Value-Length-Dimension deferred; keine eigene Profildatei. |
| LP07 | `static-read-real-string-corpus` ist wegen real-corpus/string-Dataset ausserhalb des aktuellen uint64-Key-Scopes. |
| LP13 | `concurrent-read-scaling` ist als Thread-Sweep in die bestehende `concurrency.thread_count`-Dimension gefaltet. |

Siehe Doc 32 `docs/architecture/32_lastprofil_katalog_und_paper_bias.md`: LP06 ist der `coco_p04_neg*`-Sweep,
LP07 ist out-of-scope, LP03 value_len deferred, LP12/LP13 threads -> concurrency-Dimension.

## Schema
```xml
<comdare_load_profile id="<eindeutig>" paper_ref="P04" lp_id="LP06" schema_version="1">
  <metadata>
    <name>Lesbarer Name</name>
    <source>Voll-Zitat (Autoren, Titel, Venue, Jahr, DOI)</source>
    <methodology>Wie das Paper misst (Op-Typen, Sweep), kurz</methodology>
  </metadata>
  <workload>
    <seed>42</seed>                  <!-- PRNG-Seed; bit-identische Op-Sequenz über alle Binaries -->
    <records>10000</records>          <!-- Load-Phase: # vorab eingefügte Sätze (= statischer Build) -->
    <num_operations>10000</num_operations>  <!-- gemessene Run-Phase-Ops -->
    <op_mix insert="0.0" lookup="1.0" erase="0.0" clear="0.0" scan="0.0" rmw="0.0"/>  <!-- Summe wird normalisiert -->
    <key_distribution>zipfian</key_distribution>   <!-- uniform | zipfian | latest -->
    <zipfian_theta>0.99</zipfian_theta>
    <negative_query_pct>0</negative_query_pct>      <!-- CoCo-Trie QUERY_NOT_IN_SET_%: Anteil Miss-Queries [0..100] -->
    <scan_length_max>100</scan_length_max>          <!-- YCSB-E Range-Scan-Länge [1..max] -->
  </workload>
</comdare_load_profile>
```

## Mapping → `WorkloadConfig` (workload_config.hpp)
| XML | WorkloadConfig-Feld |
|-----|---------------------|
| `seed` | `seed` |
| `records` | (Host: Load-Phase `workload_records`; Key-Range `[1, records]`) |
| `num_operations` | `num_operations` |
| `op_mix @insert/@lookup/@erase/@clear/@scan/@rmw` | `pct_insert/pct_lookup/pct_erase/pct_clear/pct_scan/pct_rmw` |
| `key_distribution` | `key_distribution` (Uniform/Zipfian/Latest) |
| `zipfian_theta` | `zipfian_theta` |
| `negative_query_pct` | `negative_query_pct` (Lookup/Scan auf `[records+1, 2·records]` = Miss) |
| `scan_length_max` | `scan_length_max` |
| `id` | `name` (Workload-Achsen-Wert) |

## Mess-Metriken je Lastprofil (CoCo-Trie-vergleichbar)
- **Bauzeit** = Load-Phase (statischer Build, ungemessen in der Run-Phase, separat erfassbar)
- **Query-Latenz** = `total_ns` / per-Op-ns (Run-Phase, Zwei-Phasen-Cache-Warmup)
- **Space** = Allocator-Observer (`stat_allocator_bytes_in_use`)
- **Observer** = alle 19 Achsen-Zähler (sauber durch Copy-Memento-Rollback)

## Archetypen (aus der 33-Paper-Analyse)
- `static-read-only-negsweep` (CoCo-Trie, SuRF, LOUDS): build + lookup, Negativ-Query-Sweep {0,25,50,75,100}
- `point-lookup-zipfian` (ART, HOT, Masstree): YCSB-C-artig
- `mixed-ycsb` (allgemein): A-F
- `range-scan-heavy` (B+-Bäume, CSB/CSS): scan-dominiert
- `write-heavy-bulk` (Bulk-Load-Paper): insert-dominiert
- `concurrent-rmw` (RCU/Hazard/ARTSync): rmw + Nebenläufigkeit
