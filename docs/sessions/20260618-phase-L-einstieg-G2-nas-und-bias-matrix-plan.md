# Phase-L-Einstieg: G2-NAS-Abschluss + fundierter Bias-Matrix-/Abgabe-PDF-Plan (2026-06-18)

> **Kontext:** /goal neu gesetzt (Stop-Hook, 2026-06-18) nach kritischer User-Prüfung „ist es wirklich erledigt?".
> Ergebnis: der Masterplan-**Audit-Fix-Umfang** (A–D, K1–K10, M8) + die **M3-Voll-Matrix** sind fertig, aber das
> **Gesamt-/goal (G1–G5 in `GOAL-AUTONOM-ABARBEITUNG-20260613.md §4`)** NICHT. Diese Session: **Ziel 1 (G2-NAS) erledigt**
> + **Ziel 2 (Phase L → Abgabe-PDF) fundiert vorbereitet** (Ist-Stand literal erhoben).

## 1. Ziel 1 — G2-Rest (NAS-Ablage) ✅ ERLEDIGT + literal verifiziert

- `scripts/copy_results_to_nas.sh` (NAS-UNC hartkodiert `//backup1.comdare.de/Cluster_NFS/experiment results`,
  User-Direktive 2026-06-08; robuste Retry-Schleife + Größen-Verifikation, lokale Quelle bleibt) legte die M3-Matrix ab:
  **`OK: …/tier150_measurements_INDEX320_cowfix-v1_2026-06-18.csv groessen-verifiziert (166987652 Bytes, Versuch 1/3)`**.
  NAS war erreichbar = Inside-Netz aktiv.
- **M3-Matrix jetzt 3-fach gesichert:**
  1. Original (temporär): `build/thesis_tiere/tier150_measurements.csv` (159,25 MB, 120.960 Zeilen)
  2. OneDrive-Backup (dauerhaft): `<Diplomarbeit-Root>/Messdaten-Backup/tier150_measurements_INDEX320_cowfix-v1_2026-06-18.csv`
  3. NAS (off-site): `//backup1.comdare.de/Cluster_NFS/experiment results/tier150_measurements_INDEX320_cowfix-v1_2026-06-18.csv`
- **G2 KOMPLETT:** Audit-Wellen 1–3 ✅ · cowfix-v1-DLLs ✅ · M3-Matrix (120.960 Zeilen, 100 % `two_phase_valid`, 320/320) ✅ · NAS ✅.

## 2. Ziel 2 — Phase L → Abgabe-PDF (G1/G3): präziser IST-STAND (literal erhoben)

**Pipeline-Stufen existieren** (`Code/03_binary_to_csv` → `04_csv_to_latex` → `05_diagram_generator`, mehrfach gebaut:
`build/msvc-g1`, `build/matrix-msvc-*`). **P1–P5 done** (Ledger), `pipeline_demo.pdf` real erzeugt (16-Spalten-Pfad).

**Schlüssel-Befund — die WIDE-Bias-Matrix-Library ist BEREITS implementiert** (`Code/04_csv_to_latex/csv_to_latex.cpp:158–264`,
Header `:46–85`, Stand 2026-06-11):
- `parse_wide_csv` — **header-getrieben** (Spalten per Name, Reihenfolge-/Breite-agnostisch → robust gegen die 154 M3-Spalten);
  Pflichtspalten `binary_id/repetition/n_ops/total_ns/ns_per_op/workload/two_phase_valid`; extrahiert `search_algo` aus `binary_id`.
- `aggregate_tier_workload` — Median ns/op je `(search_algo × workload)`, **NUR `two_phase_valid`-Zeilen** (ungültige fließen nie ein),
  `std::map` → deterministisch.
- `write_bias_matrix_latex` — Bias-Bruch-Matrix booktabs (Zeilen=search_algo, Spalten=21 Lastprofile rotiert, `resizebox`,
  leere Zelle `--`) = **der Achsen-Austauschbarkeits-Beleg (G3-Kern)**, bilingual (de/en Eckkopf).

**Die Lücke (G1/G3) ist damit präzise:**
- (L-a) ✅ **GRÖSSTENTEILS ERLEDIGT** (verifiziert 2026-06-18 per grep): Das CLI-main `Code/04_csv_to_latex/main_cli.cpp:33–37`
  ruft BEREITS den WIDE-Pfad auf (`parse_wide_csv → aggregate_tier_workload → write_bias_matrix_latex`); Test
  `tests/test_04_csv_to_latex_cached_fixtures.cpp:133–167` deckt ihn ab. VERBLEIBT nur: das CLI gegen die reale
  159-MB-M3-Matrix laufen lassen (Korrektheit/Performance bei 120.960 Zeilen) + Orchestrator (L-b) darauf richten.
- (L-b) Der **Orchestrator** `thesis/diplomarbeit/generate_measurement_appendix.ps1` ist Stand **C1 (16-Spalten,
  `ComdareMeasurementSnapshotV1`, Default-Csv `…/pipeline_real_organ/measurements.csv`)** → auf die M3-WIDE-Matrix + Bias-Matrix-Aufruf umstellen.
- (L-c) **3D-Surfaces je Interface-Funktion** (`diagram_generator` erweitern: Achse = search_algo × workload × ns/op; ggf. seg_*-Achsen-Profile).
- (L-d) **Achsen-Austauschbarkeits-longtables** mit **Diff-Beweisen** (verschiedene Pfade nachweislich verschieden — Audit-Meta-Lehre #3;
  die seg_*/stat_*-Achsen-Spalten der M3-Matrix nutzen, nicht nur ns/op).
- (L-e) **Ehrliche Limitierungs-Tabelle** im Appendix: jeder noch nicht gefixte Audit-Befund als Daten-Vorbehalt
  (K1 RC nominal, K9-c prefetch-Pseudo-Adressen, K9-d uint16 für Weg-B, A5 Second-Execution) — erfüllt §2.5-Done-Kriterium (b).
- (L-f) **Bilinguale PDF** (`thesis/diplomarbeit/build.ps1 -Lang de|en`, EN≡DE) mit eingebundenem Mess-Appendix; frischer
  git-clone baut identisch (relative Pfade); ZIH `zihpub.cls` unangetastet.

## 2b. ⚠️ NEUER VALIDITÄTS-BEFUND (User 2026-06-18) — TODO L-g: Spalten-Realitäts-Audit der M3-Matrix

**User-Beobachtung (PFLICHT-PRÜFUNG vor der finalen Abgabe-PDF):** Beim Durchgehen ALLER Spalten fällt auf, dass
**einige Messergebnisse unmöglich real gemessen worden sein können** — mehrere Testdurchläufe (Repetitionen × Workloads)
liefern **exakt glatte UND identische Werte**. Echte Messungen (v. a. Zeit-/Timing- und last-abhängige Zähler) müssten
über Wiederholungen rauschen bzw. über verschiedene Lastprofile divergieren; bit-identische, runde Werte deuten auf
**nicht-real-gemessene (fabrizierte / deterministisch-konstante / Phantom-) Spalten** hin.

**Stichproben-Bestätigung (b3i763gc1-Output, k_ary-Lebewesen über coco_p04_neg0/25/50/75/100):** zahlreiche `stat_*`- und
Allokator-Spalten sind über die 5 verschiedenen Workloads BIT-IDENTISCH (`bytes_alloc=3201280000`, `bytes_in_use=640000`,
`alloc_cnt=12505000`, `peak`, diverse runde `stat_*=10000/20000/30000/210000`) — bei unterschiedlichen Lastprofilen verdächtig.

**TODO L-g (hochpriorisiert, VOR L-e/L-f — speist die Limitierungs-Tabelle):**
1. **Systematischer Spalten-Realitäts-Audit** ALLER 154 Spalten: je Spalte über (Repetition × Workload × dyn-Setting)
   prüfen, ob die Werte variieren, wo sie variieren MÜSSTEN. Metriken: distinct-Werte/Spalte über N Durchläufe ·
   Null-Varianz über Repetitionen · Glattheit (Vielfache von 10^k).
2. **Je Spalte klassifizieren:** (a) legitim deterministisch (z. B. `bytes_alloc` bei fixer Datenmenge/fixem Seed = real,
   aber invariant), (b) **fabriziert/Phantom** (Validitäts-Defekt → fixen ODER aus den Thesis-Tabellen ausschließen +
   ehrlich in der Limitierungs-Tabelle L-e ausweisen). Gegenprüfung gegen Audit-Befunde **K6 (Phantom-Allocator:
   `allocator_statistics()` fabriziert)** + **K9 (Validitäts-Pfade)** — evtl. besteht ein K-Befund trotz Fix-Welle fort.
3. **Nur real-variable, gültige Spalten** dürfen in die Achsen-Austauschbarkeits-Belege (L-c/L-d). `two_phase_valid=1`
   sichert NUR den Cache-Warmup, NICHT die Spalten-Realität — dieser Audit ist orthogonal + zusätzlich nötig.
4. **Verifikation:** literaler Spalten-Varianz-Report (Spalte → distinct-Werte über N Durchläufe → Verdikt real/Phantom).
   Erst danach ist die Daten-Grundlage für die Abgabe-PDF freigegeben.

> Eingereiht nach **User-Direktive 2026-06-18** („als weiteres TODO einreihen und nach Abschluss der aktuellen Aufgabe
> prüfen"). Reihenfolge: **nach** dem laufenden Phase-L-Einstieg, aber **vor** der finalen Appendix-/PDF-Generierung —
> die Spalten-Realität bestimmt die zulässigen Tabellen-Spalten und die ehrliche Limitierungs-Tabelle.

## 2c. L-g — ERSTE BEFUNDE (2026-06-18, literal über die 120.960-Zeilen-M3-Matrix)

**Test 1 — Repetitions-Identität (schärfster Phantom-Test): TIMINGS SIND REAL.** Über die 40.320 rep-normalisierten
Gruppen (alle mit genau 3 echten Repetitionen) sind die Wall-Clock-Werte praktisch nie bit-identisch: `total_ns` **0 %**,
`seg_cache_traversal_ns` 0 %, `seg_search_algo_ns` 0,8 %, `seg_allocator_ns` 3,5 %. `total_ns`/`ns_per_op` haben
**117.934/120.960 = 97,5 % distinct**. → Die kern-entscheidende Bias-Matrix-Größe **`ns_per_op` ist echte Messung,
KEIN Phantom**. Phantom-Verdacht für die Timing-/Performance-Spalten **widerlegt**.

**Test 2 — Spalten-distinct über alle 154 Spalten:** 27 voll-variabel (>1000 distinct, real) · 90 mittel (4–1000) ·
**37 stark konstant (distinct ≤ 3)**. Klassifikation der 37:
- **~10 strukturell-legitim:** `n_ops=1` (fixe Workload-Größe), `repetition=3`, `two_phase_valid=1`, `obs_axes`/
  `applied_axes`/`v3_filled_axes` (Achsen-Meta), `fail=1` (0 Allok-Fehler), `op_clear_*` (clear nicht im Workload).
- **~24 ehrliche Null-Aktivität** (inaktive Achse der Komposition → Counter legitim 0): `concurrency=none`→
  contention/validation/pattern · `migration=none`→migrations/hot/cold/tier_moves · `io=in_memory`→align_adjusts ·
  `value_handle=inline`→indirect_deref/version_strips · q1/q2-Sub-Counter. **Kein Mess-Defekt** — korrekte 0-Darstellung.
- **3 prüfbedürftig — JETZT GEKLÄRT (2026-06-18, datengetrieben):**
  - (i) `stat_path_compression_checksum=1`: die Index-Auswahl enthält NUR `path_compression_none` (1/1 Ausprägung) →
    Konstanz **legitim**, aber **Coverage-Lücke** (die Index-Matrix liefert KEINE path_compression-Austauschbarkeitsdaten
    → ehrlich in Limitierungs-Tabelle L-e).
  - (ii) **`stat_node_type_find=1`: SINGLE-PROBE-OBSERVER-ZÄHLER (kein fabrizierter Phantom)** — Code-geprüft
    `axis_04_node_type_observable.hpp:57–68`: `observe_node_find` inkrementiert REAL `++find_count`, wird aber **genau
    einmal pro Messung als Observer-Probe** getrieben → konstant `1` *by design*; es ist KEINE Workload-Operationszahl.
    Die node_type-Achse IST real observiert — über **`seg_node_type_ns` (Wall-Clock, variiert real)** + **`node_type_checksum`
    (38 distinct, echtes format-divergentes `node_find_scan`-Ergebnis; Header §12: „der Format-Latenz-Unterschied bleibt
    Wall-Clock, Pfad B")**. **SELBSTKORREKTUR** meiner voreiligen „ECHTER PHANTOM"-Marke (datengetriebener Verdacht →
    Code-Prüfung verfeinert ihn). **VERALLGEMEINERUNG (wichtig für L-d):** Observer-Proben-Zähler (`*_find`/`*_count`)
    sind by-design Proben-Zähler (≈1), NICHT als variierende Mess-Größe geeignet — die Achsen-Austauschbarkeits-Belege
    fußen auf **`seg_*_ns` + `ns_per_op` (real)**; die Proben-Zähler-Semantik gehört in die Limitierungs-Tabelle (L-e).
  - (iii) `stat_node_type_checksum`: **38 distinct → REAL** (variiert mit node_type + Daten). ✓

**VERBLEIBENDE L-g-Detailarbeit (nächste Session):** (1) **Workload-Varianz-Test** der ~90 mittel-Spalten: variieren die
last-abhängigen `stat_*`/`op_*`-Counter über die 21 Workloads, wo sie es müssten? (2) Einzelklassifikation der 3
checksum/find-Kandidaten gegen den Achsen-Code (K6 Phantom-Allocator / K9). (3) **Finale Spalten-Whitelist** —
nur real-variable + legitim-deterministische Spalten dürfen in die Thesis-Tabellen; alle übrigen in die Limitierungs-Tabelle (L-e).

**Fazit Zwischenstand:** Der User-Befund ist **differenziert bestätigt** — es gibt konstante Spalten, ABER die für die
Achsen-Austauschbarkeits-Belege entscheidende `ns_per_op`-Messung ist real. Die Konstanz betrifft fast nur strukturelle
Meta-Felder + ehrliche Null-Aktivität inaktiver Achsen; nur 2–3 Counter sind echt aufklärungsbedürftig. **Keine Blockade
der Bias-Matrix**, aber die Limitierungs-Tabelle (L-e) muss die konstanten/inaktiven Spalten ehrlich ausweisen.

## 2d. L-g — EMPIRISCHER SCHLUSSSTEIN + WHITELIST (2026-06-18, literal über alle 154 Spalten)

**Workload-Bias EMPIRISCH BELEGT (= F15-Kernaussage):** für `search_algo=k_ary` spannt `ns_per_op` über die 21 Lastprofile
von **26,1 (lp_range_scan)** bis **363.925,6 ns/op (lp_bulk_insert)** = **Faktor ~13.954×**. Verschiedene Lasten → drastisch
verschiedene Performance, real gemessen → die Achsen-Austauschbarkeits-/Bias-Matrix ist substanziell aussagekräftig.

**Vollständiger Distinct-Report (gekappt @4000):** 33 Spalten distinct=1 · 1× (2) · 2× (3) · … · **19 Spalten ≥4000**.

**WHITELIST für die Thesis-Tabellen (real-variabel, code-konsistent):**
- **Performance/F15 (Primär):** `ns_per_op` (≥4000, 13.954×-Spanne) · `total_ns` (≥4000) · **19× `seg_<achse>_ns`** (per-Achsen-
  Wall-Clock; 14 davon >1000 distinct; `seg_allocator_ns`=172 = dokumentierte O(1)-Stats-Read-Baseline; `seg_migration_policy`=764) ·
  `op_<op>_p50/p99_ns` Latenz-Quantile (insert/lookup/erase/scan/rmw).
- **Korrektheit/Daten-Treue (variabel, NICHT Performance):** `*_checksum` (node_type=38, memory_layout=87, serialization=26,
  isa=28, index_org=26, io=26, filter=10) · `search_algo` hit/miss/insert/erase/lookup (381–389, last-abhängig).
- **LIMITIERUNG (L-e — NICHT als variierende Mess-Größe):** 33 Spalten distinct=1 = (a) strukturell (`n_ops`,
  `two_phase_valid`, `obs_axes`, `fail`, `op_clear_*`), (b) **Observer-Proben-Zähler** (`*_find`/`*_probe`/`*_get` ≈1 by design,
  s. §2c-Selbstkorrektur), (c) **inaktive-Achsen-0** (`concurrency=none`/`migration=none`/`io=in_memory`/`value_handle=inline`).
- **Pairing-Befund:** die V1-Spalten (idx 43–55: search_lookup/hit/…/alloc_cnt/bytes_alloc) sind distinct-identische
  Duplikate ihrer `stat_*`-Gegenstücke (z. B. `bytes_alloc`=`stat_allocator_bytes_alloc`=105) → in den Tabellen nur EINE Quelle.

**Code-Verifikation der Klassifikation** (dass kein „inaktive-Achse-0" in Wahrheit ein Phantom ist, das variieren MÜSSTE) +
**Phase-L-Erdung** laufen als Workflow (Per-Achsen-Observer-Lektüre + Pipeline-/Spec-Lektüre, adversarial verifiziert).

## 2e. ⚠️⚠️ PRÜFPUNKT L-h: CACHE-MISSES (KERNMETRIK) = 0 / FEHLEN IN M3 (User-Befund 2026-06-18)

**User-Beobachtung (KRITISCH, höchste Priorität):** bei der Messung der **Cache-Misses regelmäßig Nullen** gesehen — „aber das
sind ja genau die interessanten Werte". Für eine **Cache-Engine** (CELM) sind die Cache-Misses die **Kernmetrik** schlechthin;
0-Werte hier wiegen schwerer als jeder konstante Observer-Zähler (§2c/§2d).

**Code-verifizierte Ursache (`libs/cache_engine/builder/pmc_source.hpp:19-47`):** Die 6 HW-Counter
(`cache_misses_l1/l2/l3`, `dtlb_misses`, `coherence_invalidations`, `energy_micro_joules`) werden von einer **`IPmcSource`**
gespeist. Aktiv ist **`NullPmcSource`** → `end()` liefert `PmcCounters{}` mit **`available=false`** → **alle Cache-Misses = 0**.
Das ist BEWUSST ehrlich (kein Schein-0, sondern „nicht real gemessen"-Flag, behebt Re-Audit-Blocker 2). Reale Werte brauchen
**Intel PCM / RDPMC / RAPL-MSR + Admin/MSR-Treiber auf der i7-1270P** = **#26 / P4**, extern/HW-gated (Memory
[[project_thesis_19_26_22_deferred_until_cluster]]).

**Zweiter, schwererwiegender Befund (literal):** Die **M3-WIDE-Matrix (154 Spalten) enthält die PMC-Cache-Miss-Spalten GAR
NICHT** — der WIDE-Schema-Header (run_lazy_150 / lazy_csv_header) führt nur `seg_*_ns` (Wall-Clock) + `stat_*` (Observer) +
op-Quantile. Die Cache-Miss-Felder leben ausschließlich im **separaten 16-Spalten-PMC-Pfad** (`ComdareMeasurementSnapshotV1`,
`measurement_snapshot.hpp`; genutzt von `f15_compare --pipeline-csv` → `pipeline_demo.pdf`), wo sie via `NullPmcSource` 0 sind.
→ Die **120.960-Zeilen-Hauptmatrix kann Cache-Verhalten aktuell NUR INDIREKT** über Wall-Clock zeigen (`seg_memory_layout_ns`,
`ns_per_op` — real, aber Proxy; Cache-Misses kosten Zeit, daher korreliert, aber nicht der direkte Zähler).

**TODO L-h (PFLICHT, vor der finalen Abgabe-PDF — die zentrale Daten-Limitierung der Cache-Engine-Thesis):**
1. **Ehrliche Ausweisung (L-e, Spitzenplatz):** Cache-Misses = 0 / nicht erhoben ist die **wichtigste** Limitierung; die
   direkten Cache-Metriken fehlen, die Cache-Effekte werden über Wall-Clock-Proxy (seg_*_ns) belegt. `available`-Flag zitieren.
2. **Optionen dem User vorlegen (needs_user / kritisches Manöver):** (a) **ZIH/Cluster** reale PMC via **PAPI** (perf
   paranoid level / `-C hwperf`) — Cluster-gated; (b) **lokal Intel PCM / WinRing0 + Admin** auf der i7-1270P — kritisches
   Manöver (Treiber/Recht), User-Freigabe nötig; (c) als Limitierung belassen + Wall-Clock-Proxy dokumentieren.
3. **Mess-Pfad-Kohärenz prüfen:** soll der M3-WIDE-Lauf die PMC-Felder überhaupt mit-erfassen (Schema-Erweiterung), damit sie
   — sobald HW verfügbar — im Hauptdatensatz landen, statt im getrennten 16-Spalten-Pfad? (Architektur-Entscheid; IPmcSource
   ist Drop-in laut `pmc_source.hpp:6-9`, KEINE POD/Pipeline/PDF-Änderung nötig.)
4. **Verifikation:** literal belegen, welche Cache-/HW-Spalten in welchem Pfad existieren und welchen `available`-Status sie tragen.

> Eingereiht nach User-Direktive 2026-06-18. Reihenfolge: **vor** der finalen Appendix-/PDF-Generierung — bestimmt die
> ehrliche Kern-Limitierung + eine mögliche Hardware-/needs_user-Entscheidung. Verwandt: K1/A5 (needs_user), #26/P4 (HW-gated).

### L-h ENTSCHEIDUNG (User 2026-06-18): Option (b) Windows-Intel-PCM + Linux-Delegation

**User wählt (b)** — Windows lokal versuchen; **(a) ZIH-PAPI erst nach stabiler Linux-Version**; **Linux = uneingeschränkte
Umgebung** (Infra-Agent baut sie); zusätzlich **lokaler comdare-GitLab als 2. Remote + CI**. Handover geschrieben:
`docs/sessions/2026-06-18-HANDOVER-an-infra-agent-linux-pmc-gitlab-ci.md` (CE-DL1…DL5).

**Web-recherchierter Windows-PMC-Pfad (Option b, machbar):** Intel PCM = **BSD-3-Lizenz** (vendorbar wie mimalloc/Boost.MP11/
googletest), C++-API **`getL2CacheMisses()`/`getL3CacheMisses()`/`getL2CacheHits()`/`getL3CacheHits()`**. Windows-Hürde
(anders als Linux mit MSR-Kernelmodul): der **`msr.sys`-Treiber** muss **signiert** (Win7+: self-signed Cert via PowerShell)
nach `c:\windows\system32` + die Messung **als Administrator** laufen — sonst lädt Windows den Treiber nicht. = das
freigegebene kritische Manöver (einmaliger System-Schritt, Admin).
> Quellen: github.com/intel/pcm (doc/WINDOWS_HOWTO.md) · Intel-PCM-API (classPCM) · PCM-LICENSE (BSD-3).

**Umsetzungs-Pfad Windows (was ICH implementierungsseitig erledige):** (1) Intel PCM offline vendoren; (2)
`WindowsPcmPmcSource : IPmcSource` (begin/end → `getL2/L3CacheMisses`-Delta, `available()` = Treiber geladen?); (3) CMake-
Flag `COMDARE_ENABLE_PMC` + OS-Guard (`if(WIN32 AND COMDARE_ENABLE_PMC)`) → der PMC-Backend ist **optional**, Fallback
`NullPmcSource` → **Build bleibt grün** ohne Treiber/PCM (= das „auskommentieren beim Build" sauber als Feature-/OS-Guard);
(4) System-Schritt (Admin): `msr.sys` signieren+installieren → dann `available()==true` + reale Cache-Misses. **Verbleibt
System/Admin-gebunden; Code-Pfad mache ich Windows-seitig fertig.**

**Fokus jetzt (User: „alle Elemente, die wir hier auf Windows schon erledigen können"):** Phase L (Bias-Matrix → bilinguale
PDF) ist voll Windows-machbar und unblockiert → wird priorisiert. Der WindowsPcmPmcSource ist eigene Task; reale Cache-Miss-
Zahlen entstehen primär auf Linux (Infra-Agent, CE-DL2/DL3), Windows-PCM als zweiter Beleg.

## L-b ✅ ERLEDIGT (2026-06-18, literal verifiziert) — reale Bias-Bruch-Matrix aus M3

- **L-b.0** Rebuild `csv-to-latex.exe` (Jun-1-Binary kannte `--schema=wide` nicht) → grün, Usage zeigt `--schema=wide`.
- **L-b.1** Smoke (Head-2000) → `2000 rows -> 84 Zellen`, EXIT 0, booktabs-Matrix korrekt.
- **L-b.2/3** Voll-Lauf gegen `tier150_measurements.csv` (120.960 Zeilen) → **`120960 rows -> 84 Zellen`** (4 search_algo × 21
  Lastprofile), Median ns/op nur `two_phase_valid`, **5–6 s** (RAM/Performance-Risiko widerlegt), bilingual de+en →
  `thesis/diplomarbeit/anhang/{de,en}/tabellen/bias_matrix_table.tex` (Kopf `Suchverfahren`/`Search method`, EN≡DE-symmetrisch).
- **Bias-Bruch sichtbar:** z. B. `linear_scan` auf `ih` (insert-heavy) **schneller** als die Bäume (36676 vs eytzinger 218259 ns/op).
- **⚠️ Overleaf-Push HÄLT (User-Entscheid nötig):** das `thesis/diplomarbeit`-Repo ist Overleaf-synchron (`20260931-Overleaf-
  Diplomarbeit`); die generierten `.tex` liegen lokal, werden für den PDF-Build genutzt, aber NICHT autonom nach Overleaf gepusht.
- **Befund Orchestrator:** der alte `generate_measurement_appendix.ps1` (16-Spalten-C1) wird durch den direkten `csv-to-latex
  --schema=wide`-Aufruf ersetzt — ein neuer Mini-Orchestrator (de+en in einem Lauf) folgt in L-f.

## L-c ✅ ERLEDIGT (2026-06-18, Impl-Agent + selbst-verifiziert) — Surfaces je Interface-Funktion

- **Code (`Code/05_diagram_generator/`):** `WideMeasurementRow` + `parse_wide_csv` (header-getrieben ';', portiert aus
  `csv_to_latex.cpp:158-201`) + `write_surface_search_algo_x_workload` (2D-Heatmap, ruft vorhandenes `write_heatmap`) +
  `write_surface3d_...` (echte 3D-Surface `view={45}{30}` + `\addplot3[surf]` + `zmode=log`, Log-Floor 1e-3) + CLI-Modus
  `--surface=<z_field> [--3d] [--body-only]`. Median = nearest-rank, NUR `two_phase_valid`; scan-Surface schließt `ycsb_e`+
  `lp_range_scan` aus (19 statt 21 Spalten verifiziert).
- **Build EXIT 0** · **Tests 8/8** (6 alt + 2 neu WIDE) · Voll-CSV 9,26 s · keine absoluten Pfade.
- **Artefakte (de+en):** `thesis/diplomarbeit/anhang/{de,en}/tabellen/lc_surface_{ns_per_op,op_insert_p50_ns,op_lookup_p50_ns,
  op_erase_p50_ns,op_scan_p50_ns,op_rmw_p50_ns}.tex` — alle `\addplot3`+`viridis`. (Overleaf-Push hält, wie L-b.)
- **Offen (User-Entscheid):** 2D-Heatmap (Default, druckfertig) vs. echte 3D-`surf` für die Thesis — beide verfügbar.

## 3. Pflicht-Lese-Reihenfolge für die Phase-L-Umsetzungs-Session (frischer Kontext)

1. `docs/architecture/34_KONSOLIDIERTER_MASTER_IST_STAND.md` — §F15-Pipeline + Mess-Modell + Bias-Matrix.
2. `docs/sessions/GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md` — die L1–L8-Spec (Substanz der Phase L).
3. `docs/sessions/20260613-A3-audit-soll-abgleich.md` — die 85 Befunde, v.a. die Limitierungs-Tabelle (L-e).
4. CLI-main von `csv_to_latex` lokalisieren (`Code/04_csv_to_latex/` app/main) + `diagram_generator.{hpp,cpp}` + `build.ps1`.

## 4. Konkreter erster Umsetzungsschritt (nächste Session)

L-a + L-b als kleinste E2E-Einheit: CLI-WIDE-Subkommando → `generate_measurement_appendix.ps1` auf die M3-Matrix richten →
**EINE reale Bias-Bruch-Matrix-`.tex`** aus den 120.960 Zeilen erzeugen (literal: nicht-leere Matrix, Zellen = Median ns/op,
nur two_phase_valid) — der erste interpretierbare Achsen-Austauschbarkeits-Beleg. Dann L-c…L-f.

## 5. Disziplin-Hinweise (Goal §0)

Keine Erfolgsmarke ohne literale Tool-Ausgabe · je Einheit Commit+Push+3-Repo-Submodul-Sync · bilingual EN≡DE · relative Pfade ·
ZIH-Vorlage unangetastet · Diff-Beweise nur mit nachweislich verschiedenen Pfaden (Audit-Meta-Lehre #3) · bei Unklarheit Planrunde.
