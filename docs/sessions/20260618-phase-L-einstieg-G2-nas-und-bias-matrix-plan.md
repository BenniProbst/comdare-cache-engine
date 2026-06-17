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
