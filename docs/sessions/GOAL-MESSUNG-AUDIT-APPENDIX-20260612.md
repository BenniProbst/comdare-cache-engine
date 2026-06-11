# GOAL 2026-06-12 — Messung → Audit-Abarbeitung → interpretierbarer LaTeX-Appendix (PDF)

> **Auftrag (User, 2026-06-12, sinngemäß-verbatim):** „ZUERST die Messungen nochmal durchführen, dann das
> Audit autonom abarbeiten, und final die Messergebnisse für die LaTeX-Dokumente generieren, sodass sie
> interpretierbar werden. Ausgabe = Testdaten-Konfig × Tier (derzeit nicht grundsätzlich unterschieden).
> Die Messgeschwindigkeit jeder Interface-Funktion eines Tieres liegt als Verarbeitungsdauer je
> Testdatensatz-Operation auf der z-Achse eines 3D-Diagramms. Termin mit dem Professor MORGEN
> (2026-06-13): interpretierbare Belege für die AUSTAUSCHBARKEIT DER ACHSEN in der Suchalgorithmus-Hülle.
> Da jede Rekombination vorhanden ist, lassen sich beim Wechsel EINER Achse direkte Eigenschaftsänderungen
> als Diff gegen alle anderen Tiere dokumentieren — unter jedem Diagramm als Tabelle (egal wie lang).
> Thesis-Struktur existiert bereits unter `thesis/diplomarbeit` (bis auf den hier zu erstellenden
> Appendix); für git-Clones des Professors NUR RELATIVE PFADE. Der Appendix enthält je nach Konfiguration
> IMMER ALLE Messwerte (gewertete Länge der Arbeit hängt nur von handgeschriebenen Seiten ab). Ein
> Experiment generiert zum Schluss die fertige Diplomarbeit als PDF. TU-Dresden-/ZIH-Vorlage (diplominf)
> ist einzuhalten. Steuerung der Schritte erfolgt MANUELL durch den User."

## §0 Leitplanken (bindend für alle Phasen)

1. **Zwei-Phasen-Cache-Warmup bleibt PFLICHT** (Mess-Gültigkeit; der „Second-Execution"-Audit-Einwand wird
   NUR mit dem User diskutiert, nie stillschweigend umgesetzt).
2. **Pattern-Direktive:** neue Strukturen nur als benannte Lehrbuch-/erweiterte Patterns + Konvention;
   Metaprogrammierung nur zero-cost.
3. **Relative Pfade** in allen Thesis-/LaTeX-/Skript-Referenzen (git-clone-fest); **TU-Dresden-/ZIH-
   diplominf-Vorlage** unangetastet einhalten; **EN≡DE-Äquivalenz** der Thesis-Builds (`build.ps1 -Lang`).
4. **Keine Erfolgsmarke ohne literale Tool-Ausgabe**; jede Phase endet mit Commit+Push + Submodul-Sync
   (3 Repos). Roh-CSVs (zu groß für git) → lokale benannte Kopie + robuste NAS-Ablage
   (`scripts/copy_results_to_nas.sh`); in git nur Aggregate/Appendix/PDF.
5. **Lehren der Audits anwenden** (`docs/sessions/audit-sicherung-20260612/ERKENNTNISSE.md` §7):
   Capability-Pfade per static_assert über die ZIEL-Population absichern; Konfiguration pinnen statt
   stiller Defaults; Diff-Beweise nur mit nachweislich verschiedenen Pfaden.

## §1 Ausgangslage (verifiziert, 2026-06-12 früh)

- **320 cowmem-v1-DLLs gebaut** (laufen real auf dem Copy-Memento-Pfad — Audit K3: Label verbrannt, aber
  **MESSWERTE NICHT verfälscht**, rb_exact true). 9/320 Tiere tragen gültige 10k-Stamps; der gestoppte
  C-Lauf ist via Resume fortsetzbar.
- **Beide Audits komplett:** 85 bestätigte Befunde (32 blocker/36 major/17 minor), konsolidiert als
  Kern-Defekte K1-K10 + 4-Wellen-Fix-Plan (`20260611-audit-ergebnisse-synthese.md` §4, Task #142).
- **Bekannte Daten-Limitierungen eines SOFORT-Laufs** (ohne Welle 2): RC-Subdimension = 6 Pseudo-Replikate
  (für tier×workload-Auswertung nutzbar als zusätzliche Wiederholungen — ehrlich ausweisen); 2/21
  Scan-Profile (ycsb_e, lp_range_scan) invalide (No-Op) → aus der Auswertung ausschließen;
  Insert-Profile messen Upserts (ausweisen); stat_*-Spalten enthalten die Load-Phase (ausweisen);
  ns_per_op-Spalte vor M1-Fix halbiert.
- **Auswertungs-Bausteine vorhanden:** Stufe 04 `csv_to_latex` mit WIDE-Parser + `aggregate_tier_workload`
  + `write_bias_matrix_latex` (verifiziert an echter CSV); Stufen 05 (`05_diagram_generator`) und 06
  (`06_latex_to_pdf`) existieren als Pipeline-Plätze; Thesis-Gerüst `thesis/diplomarbeit` (diplominf,
  bilingual) steht bis auf den Appendix.

## §2 Phasen-Architektur (empfohlene Sequenz; User steuert manuell)

```
HEUTE (2026-06-12):  M1 Quick-Fixes (NUR Host/XML/Harness — DLLs unangetastet!) → M2 Lauf-START (Resume)
                     ║ parallel (CPU-leicht):  L1-L5 Auswertungs-/Appendix-Pipeline bauen
ABEND/NACHT:         Lauf misst; L6 Teilstand-Generalprobe: Teil-CSV → Appendix → Thesis-PDF
MORGEN (Termin):     L7 Professor-Paket aus dem Teilstand (so viele Tiere wie fertig)
NACH DEM TERMIN:     A1-A4 Audit-Wellen autonom (inkl. cowfix-v1-DLL-Neubau) → M3 finaler Voll-Lauf
FINAL:               L8 End-to-End: EIN Experiment-Aufruf → komplette Matrix → Appendix → fertige PDF
```
Begründung der Parallelität: Der Termin braucht die AUSWERTUNG dringender als die Voll-Matrix; der Lauf
liefert über Resume kontinuierlich per-Tier-Ergebnisse, die die Pipeline jederzeit einsammeln kann.

---

## §3 TODO-LISTE (elaborat; je Punkt mit Verifikations-Kriterium ✓)

### Phase M — Messung (zuerst; M1 vor M2 zwingend)

- [ ] **M1.1 ns_per_op-Divisor-Fix (K2, Host-only):** In `perm_runner.hpp`/`cache_engine_builder_iterator.hpp`
      die Ableitung `total_ns/(2*n_ops)` für den Workload-Pfad auf die ECHTE getimte Op-Zahl stellen
      (n_ops; Legacy-Pfad behält 2*n_ops). ✓ Eine Smoke-Zeile zeigt ns_per_op == total_ns/n_ops.
- [ ] **M1.2 coco_neg50-XML-Fix (K7a):** `coco_p04_neg50.xml` Verteilung zipfian→uniform (1 Zeile).
      ✓ Diff der 5 Sweep-XMLs zeigt identische Verteilung, nur neg% variiert.
- [ ] **M1.3 Harness-Härtung (K8, Re-Entry-Drift):** `build_and_measure_150_tiere.ps1` setzt
      COMDARE_LOAD_PROFILE_DIR + COMDARE_WORKLOAD_RECORDS SELBST (Parameter mit Voll-Lauf-Defaults), bricht
      bei fehlendem Profil-Verzeichnis ab; run_lazy_150 verweigert den Start ohne gültige Registry
      (env-Guard statt stillem Fallback). ✓ Aufruf ohne env läuft korrekt; Aufruf mit kaputtem Pfad bricht
      mit klarer Meldung ab.
- [ ] **M1.4 Stamp-Write-Gate (K8):** Stamp nur nach `pf.good()`-verifiziertem result.csv-Write; Gültigkeits-
      Gate (kein Stamp, wenn eine Zeile two_phase_valid=0 enthält — Audit „Stempel gated nicht Gültigkeit").
      ✓ Negativ-Probe: erzwungener Schreibfehler hinterlässt KEINEN Stamp.
- [ ] **M1.5 Host-Rebuild + Mini-Smoke** (1 Tier, n_ops=1000): ✓ Exit 0, ns_per_op-Stichprobe korrekt,
      Stamp entsteht.
- [ ] **M2.1 Voll-Lauf STARTEN** (Hintergrund, tokenfrei): 320 Tiere, **BuildVersion `cowmem-v1`
      UNVERÄNDERT** (kein DLL-Neubau — Copy-Pfad-Messwerte sind valide; Audit-Label-Problem ist
      dokumentiert und für die DATEN irrelevant), n_ops=10000, records=10000, n_repeats=3, 21 Profile,
      Resume an (9 fertige Tiere werden übersprungen — deren ns_per_op wird in der Auswertung neu aus
      total_ns abgeleitet, NICHT aus der CSV-Spalte: deckt M1.1 rückwirkend). ✓ Prozess läuft; Stamps
      wachsen.
- [ ] **M2.2 Fortschritts-Snapshot-Skript:** kleiner PowerShell-Einzeiler (Stamps zählen + Teil-CSV aus
      allen fertigen result.csv konkatenieren — Header einmal) für jederzeitige Auswertungs-Stände.
      ✓ Teil-CSV mit N fertigen Tieren entsteht reproduzierbar.
- [ ] **M3 (NACH Phase A) finaler Voll-Lauf** mit `cowfix-v1` (echtes CoW, Apparat-bereinigt, alle 21
      Profile inkl. Scans valide) → ersetzt die M2-Daten für die finale Abgabe. ✓ 120.960 Zeilen, 0
      Fehlmessungen, NAS-Ablage verifiziert.

### Phase L — Auswertung/Appendix/PDF (parallel zu M2 bauen; kritischer Pfad für MORGEN)

- [ ] **L1 Datenmodell „Ausgabe = Testdaten-Konfig × Tier":** Stufe 04 erweitern: aus `setting`+`workload`
      die VOLLE Testdaten-Konfig extrahieren (workload_id, n_ops, records, repetition, thread/prefetch-Label
      [als „nominal" ausgewiesen, K1]) und vom Tier (binary_id → 19 Achsen-Belegungen geparst) TRENNEN;
      je INTERFACE-FUNKTION (insert/lookup/erase/scan/rmw/clear) eigene Kennzahl: Median-ns je Op-Art aus
      den per-Op-Latenz-Spalten… **Achtung Datenlage:** die globale CSV trägt total_ns je Zeile, die
      per-Op-Art-Aufschlüsselung (insert_p50/lookup_p50/…) existiert im WorkloadRunResult — prüfen, ob sie
      in der WIDE-CSV ankommt; falls nein → M1-Nachtrag: per-Op-Art-Spalten in lazy_csv_header/format_csv_row
      ergänzen (Host-only, vor M2.1!). ✓ Parser liefert je Zeile (Konfig-Tupel, Tier-Tupel, {op_art → ns/op}).
- [ ] **L2 3D-Diagramme:** je Interface-Funktion EIN pgfplots-`\addplot3`-Surface: x = Testdaten-Konfig
      (Workload-Index, sortiert+legendiert), y = Tier (Index über die 320, sortiert nach Achsen-Lexikografie),
      **z = Verarbeitungsdauer je Testdatensatz-Operation (ns/op)**; zusätzlich je search_algo-Familie ein
      Detail-Surface. Generator in Stufe 05 (`05_diagram_generator`), Ausgabe = standalone .tex-Bausteine
      mit RELATIVEN Pfaden. ✓ Test-PDF zeigt ≥1 Surface mit echten Smoke-Daten.
- [ ] **L3 Achsen-Austauschbarkeits-Diffs (Kern-Beleg für den Professor):** für jede Achse a und jedes
      Werte-Paar (v→v′): alle Tier-Paare, die sich NUR in a unterscheiden (vollständige Rekombination ⇒
      systematische Paarbildung über die binary_id-Tupel); je Paar und Workload: Δns/op je Interface-
      Funktion + relatives Δ; Aggregat je (a, v→v′): Median/IQR über alle Paare×Workloads. **Ausgabe: unter
      jedem Diagramm eine longtable (Länge egal — Vollständigkeit zählt).** ✓ Für eine Stichproben-Achse
      (z.B. node_type) stimmt die Paar-Zahl mit der Kombinatorik überein; Tabelle baut in LaTeX.
- [ ] **L4 Appendix-Generator (IMMER ALLE Werte):** Stufe-04/05-Ausgaben zu EINEM `appendix_messwerte.tex`
      komponieren: §A Mess-Konfiguration (Profile, Seeds, Maschinen-Daten, Lauf-Metadaten, bekannte
      Limitierungen aus §1 EHRLICH tabelliert) · §B 3D-Diagramme je Interface-Funktion · §C Austauschbar-
      keits-Difftabellen je Achse · §D Roh-Aggregat-Tabellen (alle Konfig×Tier-Zellen). ✓ .tex kompiliert
      standalone.
- [ ] **L5 Thesis-Integration (relative Pfade, diplominf):** Appendix in `thesis/diplomarbeit` einhängen
      (\input mit RELATIVEM Pfad vom Thesis-Root; Anhang-Kapitel gemäß TU/ZIH-Vorlage; KEINE Vorlagen-
      Dateien verändern); EN- und DE-Build erhalten denselben Appendix (Mess-Daten sprachneutral,
      Beschriftungen via babel-Schalter oder EN≡DE-Paartexte). ✓ `build.ps1 -Lang de` UND `-Lang en`
      bauen die PDF mit Appendix; git clone in temp-Verzeichnis + Build = grün (Relativ-Pfad-Beweis).
- [ ] **L6 Generalprobe (HEUTE Abend):** M2.2-Teil-CSV → L1-L5 → Test-PDF. ✓ PDF zeigt echte Teil-Matrix-
      Diagramme + Diff-Tabellen.
- [ ] **L7 Professor-Paket (MORGEN früh):** frischer Teilstand-Snapshot → PDF + 3-Punkte-Interpretation
      der Austauschbarkeits-Belege (welche Achsen-Wechsel zeigen konsistente, lokalisierte Eigenschafts-
      Änderungen bei konstanten übrigen Achsen = der Austauschbarkeits-Nachweis). ✓ PDF liegt vor Termin vor.
- [ ] **L8 (FINAL, nach M3):** identische Pipeline über die cowfix-v1-Voll-Matrix; **EIN Experiment-
      Kommando** (Harness-Schalter, z.B. `-GenerateThesis`) führt: Lauf/Resume → Aggregat → Appendix →
      `build.ps1` beide Sprachen → fertige PDFs. ✓ Ein Aufruf, Ende-zu-Ende, literal belegt.

### Phase A — Audit-Abarbeitung (autonom, NACH dem Professor-Termin; Grundlage Task #142 + Synthese §4)

- [ ] **A1 Welle 1 — Rest** (was M1 nicht schon nahm): Resume-Stempel + XML-Inhalts-Hash + effektiver Seed
      + env_limits; Stempel-Gültigkeits-Gate; Resume-Check VOR b.ok()-Gate (fertige Binaries trotz
      Build-Fehler in die CSV); XML-Validierung (op_mix-Pflicht, `<records>`-Feld nutzen oder aus Schema
      entfernen); CSV-Stream-Fehlerprüfung; Separator-/ID-Validierung; Doc-32↔21-XML-Abgleich (LP11-Sweep
      ergänzen, LP02 nachziehen). ✓ je Punkt literale Probe.
- [ ] **A2 Welle 2 — Apparat-Reinheit (EINE neue BuildVersion `cowfix-v1`):** is_new via occupied_count-
      Delta in beiden Hüllen (K5a) · NotifyPolicy compile-time/pull-only (K5b) · container_ auf
      LinearScanTraversal (K5c) · echter Policy-Allocator in beiden Stores statt fabrizierter Stats (K6) ·
      `restore_statistics` in die 4 Pilot-Wrapper bzw. CRTP-SearchAlgoBase → **CoW real aktiv für die 320**
      + static_assert über mind. eine echte Pilot-AdHoc-Komposition (Lehre #1/#2) · Iterator-Scan als
      Organ-API (K4; Hash-Organe ehrlich nicht-scanbar) · uint16→uint64-Key-Entscheid der 03a-Organe ·
      Load-Phase aus stat_*-Spalten heraushalten (Observer-Reset nach Load) · fill_segment_timing-T0-Restore.
      ✓ test_cow_memento erweitert (Pilot-Typen!) grün; 1-Tier-Smoke zeigt CoW-Diagnose aktiv + Scan-Werte >0.
- [ ] **A3 Welle 3 — Dimension/Validität:** RC-Organ-Hooks gemäß §7-A ODER RC-Dims ehrlich entfernen
      (User-Entscheid einholen; bis dahin Auswertung markiert sie als nominal) · Zipfian/Latest-Key-
      Scrambling (z.B. Splitmix64-Hash) · Konformitäts-Gate in den Voll-Lauf-Pfad (import→GATE→messen) ·
      Load-/Insert-Key-Räume trennen (Insert-Profile messen echtes Einfügen) · Selektions-Guard
      (search_algo-balanciert). ✓ je Punkt literale Probe; Negativ-Sweep + Verteilungs-Histogramm geprüft.
- [ ] **A4 Welle 4 — Pattern-Hygiene (kein Mess-Einfluss; gemäß Pattern-Direktive):** Adapter-Etikett
      (Anatomie als echtes Adaptee instanziieren ODER ehrlich umbenennen) · Memento-Etikett + tote
      MementoAggregate-Artefakte · „Hybrider Visitor"-Anspruch aus Doku entfernen oder implementieren ·
      Command-Parallelstruktur auflösen · MeasurableObserver → benanntes Muster (Single-Slot-Callback ≠
      Observer) · „B+-Baum"-Benennung klären · Release-DLL-Zero-Overhead (Mess-Kopplungen unter
      COMDARE_MEASUREMENT_ON) · Pattern-Minor-Liste. ✓ Grep-Beweise + Doku-Sweep + Bestandstests grün.
- [ ] **A5 Grundsatz-Klärung mit User (NUR Diskussion):** „Second-Execution"-Einwand vs Zwei-Phasen-
      Pflicht-Direktive — Optionen dokumentieren (z.B. beide Phasen messen und getrennt ausweisen),
      Entscheidung dem User.

### Abschluss-Gates

- [ ] **G1:** M2-Teilstand hat den Professor-Termin bedient (L7 ✓).
- [ ] **G2:** Wellen 1-3 literal verifiziert; cowfix-v1-Matrix komplett (M3 ✓); NAS-Ablage ✓.
- [ ] **G3:** L8-E2E ✓ — ein Experiment-Aufruf erzeugt die fertige bilinguale Diplomarbeit-PDF mit
      vollständigem Mess-Appendix aus der finalen Matrix; frischer git-clone baut identisch (relative
      Pfade); TU/ZIH-Vorlage unverändert.
- [ ] **G4:** Session-Doku + Memory + 3-Repo-Sync nach jeder Phase.
