# Offene Punkte — Mess-Pipeline, Custom-Permutations-Lebewesen & bilinguale Thesis-Anhang-Integration

> **Stand: 2026-06-01.** Elaborierte Bestandsaufnahme (User-Auftrag) für die drei Arbeitsfronten rund um die
> automatische, bilinguale Einbindung von Messergebnissen (Tabellen + Diagramme je Tier/Permutation aller Achsen)
> in die Diplomarbeit-Anhänge. Quelle: 4-dimensionaler Untersuchungs-Workflow (`wwnyh7jo3`, 5 Agenten) +
> manuelle Verifikation der Schlüssel-Evidenz. **Trennt strikt IMPLEMENTIERT (mit Beleg) von OFFEN/GATED.**

---

## 0. Kernfrage: Erhalten wir derzeit valide Latenz-/Performance-Messergebnisse der Cache-Line-Variation?

**Antwort: NEIN für die isolierte Cache-Line-/Layout-Aussage — TEILWEISE auf der grob-granularen Ebene.**
Der CODE ist valide, die WALL-CLOCK-MESSUNG für die isolierte Layout-Frage ist es nicht.

**Zwei sauber zu trennende Ebenen:**

1. **Layout-Implementierung = valide (JA).** Alle 5 Layout-Varianten implementieren in `scan_field_sum()` WIRKLICH
   unterschiedliche Speicher-Zugriffsmuster mit echten Strides — AoS-strided (`i*record_size`, CacheLineAligned/
   AoSStrict), SoA-contiguous (`i*4`), AoSoA-blocked (Block-Breite 8), Packed (`i*2`). Keine Stubs; Korrektheit
   per Unit-Test belegt (`test_v41_axis_05_memory_layout.cpp:196-231`, axis_05-Suite 16/16). Der Layout-Scan wird
   in F15 Pfad-A Segment 3 real ausgeführt (`abi_adapter.hpp:214-215`), Workload 16384×64 B = 1 MB (> L1/L2).

2. **Mess-Validität für die isolierte Layout-Aussage = NEIN.** Erhoben wird ausschließlich **Wall-Clock-Latenz des
   4-Segment-Komposit-Workloads** (search_algo + allocator + memory_layout + serialization gemeinsam). Der echte
   Cache-Layout-Effekt (~20–50 ns/Zugriff) ertrinkt im Rauschen der dominierenden Segmente (search_algo ~100–2000 µs).
   **Belegt (Doku 22 §3.3, verifiziert):** in der 48-DLL-Messung (12 search × 2 alloc × 2 layout) ist die
   **allocator**-Dimension sauber auflösbar (`array256/AoS: std 589 µs vs pool 219 µs = 2,7×`), die **memory_layout**-
   Dimension dagegen „mit Wall-Clock auf dieser Maschine NICHT zuverlässig auflösbar" (SoA/AoS-Verhältnisse streuen
   0,07×–9,18× ohne konsistentes Vorzeichen). Das ist dort **ehrlich als Limit-/Negativ-Ergebnis** ausgewiesen, NICHT
   als Layout-Effekt-Nachweis.

**Voraussetzung für valide Cache-Line-Messung** (eine von dreien):
(a) **Hardware-Performance-Counter** (L1/L2/L3-Miss, L2-MPKI via Intel PCM / PMC) — das ist Punkt **#26/A1**, extern-gated;
(b) **isoliertes Layout-only-Mikrobenchmark** (nur Segment 3, Puffer ≫ L3, viele Wiederholungen) — A3;
(c) **theoretische Cache-Line-Modellierung** (Cache-Line-Zählung statt Timing) — A3-Alternative.

Wall-Clock bleibt gültig für **grob-granulare** Achsen (allocator, search_algo) — dort sind die Faktoren sauber.

> Eine frische Wiederholungs-Messung würde dasselbe Negativ reproduzieren (Physik unverändert); auf Wunsch
> liefere ich aktuelle Zahlen, das Ergebnis bleibt aber „Layout isoliert nicht auflösbar ohne PMC".

---

## Gruppe A — Mess-Validität (Cache-Line/Layout) — HÖCHSTE PRIORITÄT

**A1 [P0] PMC-Integration (HW-Performance-Counter) nicht umgesetzt** — = TODO **#26**.
- *Fehlt:* Cache-Miss/L2-MPKI-Erhebung via Intel PCM. Wall-Clock kann den ~20–50-ns-Layout-Effekt physikalisch
  nicht von der µs-ms-Segment-Dominanz trennen.
- *Status:* Software-Abstraktion fertig (`IPmcSource`/`NullPmcSource`, `pmc_available`); Beschaffungs-Spec liegt vor
  (`docs/sessions/20260601-26-pmc-counter-beschaffungs-spec.md`). Reale Counter = HW/Treiber-gated (Intel-PCM bauen,
  MSR/WinRing0 + Admin, HVCI temporär aus) → bis Cluster/ZIH zurückgestellt (User-Entscheidung).
- *Nächster Schritt:* Beschaffungs-Spec-10-Schritte bei verfügbarer Umgebung; dann L2-MPKI je Layout erheben.

**A2 [P0] Pfad-B `observe_all()` ist Stub — keine layout-spezifische Achsen-Statistik.**
- *Fehlt:* Der host-seitige Observer-Pfad liefert für die nicht-getriebenen Achsen NULL-Statistik; memory_layout-KPIs
  kommen über Pfad B nicht durch (Blocker: protected CRTP-Ctor des Containers).
- *Warum wichtig:* Pfad B ist im Hybrid-Messmodell (Doku 24 §8) die zweite, korrelierte Dimension.
- *Nächster Schritt:* CRTP-Ctor-Blocker lösen + Container-Treiben im Builder verdrahten (Doku 24 §5.2–§5.4). Separat
  einplanen (nicht im erledigten R5.G-Scope).

**A3 [P1] Isoliertes Layout-Mikrobenchmark / Modellierung** (PMC-Alternative bzw. -Ergänzung).
- *Fehlt:* Segment 3 ist nur im 4-Segment-Komposit variierbar. Ein Standalone-Layout-Benchmark (Puffer ≫ L3, viele
  Wiederholungen, nur scan_field_sum) ODER eine theoretische Cache-Line-Zählung würde das SNR drastisch verbessern.
- *Nächster Schritt:* Standalone-`scan_field_sum`-Bench bauen ODER Cache-Line-Zähl-Modell je Layout dokumentieren.

**A4 [P2] Layout-Tuning-Sensitivität ungemessen** — AoSoA `kBlockWidth=8` fest; Sweep 4/16/32 + adaptive
Channel-Interleaving offen (R7.7, 32 SP). Erst sinnvoll NACH PMC-KPIs.

---

## Gruppe B — Custom-Permutations-Lebewesen (V5-Mess-Architektur)

> User-Bild: „zuerst vorkonfigurierte Tiere analysieren, dann CUSTOM Permutationstiere, die ALLE Achsen gegeneinander
> permutieren, Tier-Binaries kompilieren, am Prüf-Dock durchmessen". IST: vorkonfiguriert + Kompilations-/Prüf-Dock-
> Kette stehen; die VOLLpermutation aller Achsen ist eine Stichprobe.

**B1 [P0] adhoc_emitter ist SUPERSEDED — „Achse=Organ"-Klarstellung.** Der Ad-hoc-Pilot variiert MONOLITHISCHE Lebewesen
(Array256..BTree) mit Default-Achsen → verletzt „Achse=Organ, NIE ganze Lebewesen" (Doku 14 §3.1) und darf NICHT die
autoritative F15-Quelle sein. Im Emitter selbst bereits als superseded markiert (`apps/adhoc_emitter/main.cpp:13-21`).
- *Nächster Schritt:* Autoritative Quelle = die sezierten Observable-Organ-Compositions via
  `comdare_codegen_anatomy_module_list` + `f15_compare --pipeline-csv`; Emitter nur diagnostisch (R5.G) führen.

**B2 [P1] i7-1270P-Messreihe — git-Status (KORRIGIERT).** Der Workflow vermutete „nicht git-belegt"; **Verifikation:
`docs/sessions/20260531-f15-organ-messung-i7-1270p.txt` IST getrackt** (+ `20260531-organ-pipeline-csv-beleg.md`).
Das 48-DLL-Manifest liegt unter `build/generated/r5g_autobuilt_modules/manifest.txt` (build/ gitignored).
- *Offen:* Die reproduzierbare CSV der Organ-Messung dauerhaft als Referenz-Artefakt sichern (nicht nur build/).

**B3 [P1] Full-Coverage ist 1-wise-Stichprobe, NICHT Vollpermutation.** `fullcov::SampleEngine` deckt ~25 Permutationen
(„jede Variante jeder Achse ≥ 1×"), nicht den kartesischen Raum. Für nur 3 Achsen schon 17 search × 25 alloc × 5 layout
= 2125; über alle 17 Achsen nicht formal beziffert.
- *Nächster Schritt:* Sampling-vs-Vollpermutations-Entscheidung dokumentieren + formale kartesische Größen-Tabelle (alle
  17 Achsen) erstellen; bei Bedarf t-wise-Coverage statt 1-wise. **Dies ist der Kern von „alle Achsen gegeneinander permutieren".**

**B4 [P2] Conformance-Gate-Provenance je DLL.** Host-Gate ist im `--measurement-plan` integriert; alt-gebaute DLLs ohne
Gate-Support könnten `dock_status_conformance_failed` liefern → Build-Provenance der gemessenen DLLs erzwingen/dokumentieren.

**B5 [P2] Achsen-vs-std::map-Vergleichstests nicht flächendeckend belegt** (das std::map-Interface IST das F15-Kernziel) →
Test-Matrix pro Achse × Variante gegen std::map; Lücken auffüllen.

**B6 [P3] Leer-Achsen-Guard ohne dokumentierte Build-Konsequenz** + kartesische Größen-Berechnung dokumentieren.

---

## Gruppe C — Bilinguale Anhang-Auto-Anhängung (der zentrale User-Wunsch)

> User-Bild: „Messergebnisse automatisch beim Ausführen, wählbar EN/DE per compile-Schalter, hinten in die DA-Anhänge
> anfügen — ein Dokument mit allen Messtabellen + Diagrammen je Tier/Permutation." IST: die **Bausteine** sind fertig,
> die **Automatisierung/Verdrahtung** fehlt.
>
> **STATUS-UPDATE 2026-06-01 (Commit DA `ddae10b`): C1–C4 ERLEDIGT + verifiziert.** Der zentrale User-Wunsch — ein
> bilingualer, befüllter Mess-Anhang per `build.ps1 -Lang de|en` — ist real. Verifiziert: Thesis-Build EN+DE je 48 Seiten,
> 0 fatale LaTeX-Fehler, Tabelle A.1 (S.29) + Abbildung A.1 (S.30) in `.lot`/`.lof`, alle measurements-Refs aufgelöst;
> Regression test_04 5/5, test_05 6/6 grün. Offen bleibt nur C6 (USAGE-Doc, P3) und — orthogonal — A1 (valide
> Cache-Line-Zahlen am PMC/#26-Gate, s. Gruppe A) und Gruppe B (Custom-Permutations-Tiere).

**C1 [P0] ✅ ERLEDIGT — E2E-Build-Automatisierung (Mess-CSV → Pipeline → Anhang → PDF).** Statt eines CMake-Custom-Targets
gewählt: **PowerShell-Orchestrator** `thesis/diplomarbeit/generate_measurement_appendix.ps1` (passt zum Thesis-Build-Flow
`build.ps1`). Er nimmt EINE 16-Spalten-`measurements.csv`, ruft je Sprache `csv-to-latex --lang` + `diagram-generator --lang`
auf, legt Tabellen-+Diagramm-Fragmente nach `anhang/{de,en}/tabellen/` ab; `build.ps1 -Lang de|en` produziert den befüllten
Anhang. (Vollständige Verkettung mit `messung_driver` als vorgelagertem Schritt = optional, s. Gruppe B / B3.)

**C2 [P0] ✅ ERLEDIGT — bilingualer Pipeline-Schalter `--lang=de|en`.** An `csv_to_latex` (Spaltenkopf DE „Zyklen" vs EN
„cycles") + `diagram_generator` (TikZ-Titel/Achsen DE „Comdare-Messvergleich"/„Zyklen" vs EN) ergänzt. Default `en` =
rückwärtskompatibel. (Commits DA `f0dd4c7`, `73e8845`.)

**C3 [P1] ✅ ERLEDIGT — Anhang-Stubs + `tabellen/` befüllt.** `anhang/{de,en}/A_measurements.tex` hängen via
`\input{anhang/<lang>/tabellen/<spec_id>_table.tex}` + `_diagram.tex` ein (+ Intro-Text + Querverweise auf Tabelle/Abbildung);
`tabellen/` enthält die generierten Fragmente.

**C4 [P1] ✅ ERLEDIGT — LaTeX-Fragment-Standardisierung.** Tabellen-Fragment (booktabs, Caption/Label/Sprache parametrisiert,
csv_to_latex escaped selbst) + Diagramm: `diagram_generator --body-only` (`PageConstraints.body_only`) emittiert nur die
reine `tikzpicture`; der Orchestrator wrappt sie in eine `figure` mit spec-spezifischer Caption + referenzierbarem `\label`
(vermeidet verschachtelte `figure`). Caption-Sonderzeichen (SpecId-`_`) werden escaped (sonst Bruch in `.lof`).
*Korrektheits-Fix nebenbei:* Diagramm-Demo-Pfad plottete `op_count` trotz Achse „Zyklen/Cycles" → jetzt `total_cycles`.

**C5 [P2] ~ WEITGEHEND ABGEDECKT — Thesis-Integration statt nur Standalone-PDF.** Der Orchestrator liefert Fragmente direkt
in die Thesis-Anhänge; finales pdflatex über `build.ps1`. Das Standalone-`pipeline_demo.pdf` (Stufe 06) bleibt als separater
Demo-Pfad bestehen, ist aber für den Anhang nicht mehr nötig.

**C6 [P3] OFFEN — Verdrahtungs-Doku** (USAGE.md: „ein Befehl → beide PDFs mit befüllten Anhängen"). Kerninhalt steht im
Orchestrator-Header + hier; ein dediziertes USAGE.md fehlt noch.

---

## Bereits IMPLEMENTIERT + belegt (NICHT als offen führen)

**Layout-Achse (axis_05) — Zugriffsmuster REAL, keine Stubs:** 5 Varianten mit echten Strides in `scan_field_sum()`
(cache_line_aligned/aos_strict/soa/aosoa/packed_bitmap, je file:line belegt); Unit-Test 16/16; CRTP+3-Concept-Check;
HM1–HM4-Subaxes. **F15 Pfad-A** ruft den Layout-Scan real auf (`abi_adapter.hpp:214-215`), 1-MB-Workload korrekt dimensioniert.

**Mess-Lauf durchgeführt + ehrlich dokumentiert:** 48 Permutations-DLLs (12 search × 2 alloc × 2 layout) gebaut; allocator
auflösbar (2,7×), Layout-Negativ ehrlich ausgewiesen (Doku 22 §3.3); i7-1270P-Session git-getrackt.

**Permutations-/Lebewesen-Pipeline:** 11 vorkonfigurierte Reference-Compositions (`known_compositions_list.hpp`, static_assert
count==11); kartesischer mp_product-Permutations-Raum; Gattungs-Specialization-PermutationEngine; CMake-2-Pass-Lebewesen-Binary-
Kompilation (`cmake/adhoc_emitter.cmake`); **Prüf-Dock REAL** (IPruefDock gattungs-sicher + Conformance-Gate V5 +
`measure_genus_sequential`); Mess-Output `--pipeline-csv`/`--observe`/`--measurement-plan`.

**Bilinguale Thesis-Infrastruktur (Teil):** Thesis-Compile-Schalter VOLLSTÄNDIG (EN/DE bauen fehlerfrei, `build.ps1 -Lang`
+ `\thesislang`, sprachgetrennte `\include{kapitel|anhang/\thesislang/...}`); Pipeline-LaTeX-Erzeuger funktionsfähig
(csv_to_latex booktabs, diagram_generator TikZ+A4) — aber nur als Standalone-Demo; Anhang-Stubs strukturell angelegt.

**Klarstellung gegen Doppel-Erfassung:** Die Layout-**Implementierung** ist fertig — offen ist NUR die **Mess-Methodik**
(PMC/Pfad-B/Isolation). Die bilinguale **Thesis-Struktur** + **Pipeline-Erzeuger** sind fertig — offen ist NUR die
**Automatisierung/Verdrahtung** (E2E-Target, `--lang`-Flag, Fragment-Einhängung).

---

## Roter Faden / Empfohlene Reihenfolge

1. ~~**C1 + C2 (bilinguale E2E-Anhang-Automatisierung)**~~ ✅ **ERLEDIGT 2026-06-01 (Commit DA `ddae10b`)** — inkl. C3/C4.
   Der „ein-Befehl → beide PDFs mit befüllten Anhängen"-Flow steht (`generate_measurement_appendix.ps1` → `build.ps1 -Lang`).
2. **B3 (Vollpermutations-/Sampling-Entscheidung + kartesische Größen-Tabelle)** + B1-Klarstellung — definiert „alle Achsen
   gegeneinander permutieren" sauber. **Lokal machbar. ← NÄCHSTER FOKUS.**
3. **A1/#26 (PMC)** — erst dann werden die Cache-Line-Latenzen valide isoliert messbar. **Extern-gated (Cluster/ZIH).**
4. A2 (Pfad-B-Observer), A3 (Isolations-Bench), B4/B5 (Gate-Provenance, std::map-Test-Matrix) — mittelfristig.

> **Hinweis:** C1/C2/B3 sind ohne externe Ressourcen umsetzbar und liefern direkt den vom User beschriebenen
> bilingualen Mess-Anhang. A1 (valide Cache-Line-Zahlen) bleibt am PMC/#26-Gate.
