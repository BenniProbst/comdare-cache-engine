# Audit-Ergebnisse 2026-06-11/12 — Synthese beider Voll-Audits (PERSISTENT)

> **✅ END-VERDIKT MESS-AUDIT (2026-06-12 früh, Workflow KOMPLETT, 71 Agenten):** **62 Befunde → 62
> adversarial verifiziert → 57 BESTÄTIGT (24 blocker / 24 major / 9 minor), 5 widerlegt.** Vollständige
> konsolidierte Texte (je Befund claim/evidence/consequence/fix + Verifizierer-reasoning):
> **`20260612-messaudit-endergebnis.json`** (314 KB — ersetzt die Rohdaten-Datei als autoritative Quelle;
> die Rohdaten bleiben als Zwischenstand-Beleg). Die 24 Blocker verdichten sich auf die §2-Cluster M1-M8;
> gegenüber der Vorab-Synthese HOCHGESTUFT auf blocker: Zipfian-ohne-Scrambling, Konformitäts-Gate-nie-
> gerufen, seg_ns-n=1-Degeneration (320), prefetch-Pseudo-Adressen-Achse, **Harness-Re-Entry-Drift**
> (build_and_measure_150_tiere.ps1 pinnt COMDARE_LOAD_PROFILE_DIR/WORKLOAD_RECORDS nicht und hat
> Voll-Lauf-fremde Defaults NOps=2000/BuildVersion=tier150-v1 → ein argloser Resume-Relaunch nach Reboot
> überschreibt fertige Ergebnisse mit falscher Matrix). **Die 5 Widerlegten:** 2× BuildVersion-Drift-
> Varianten, tier_scan-sort-Variante (für die 320 gilt stattdessen der No-Op-Blocker; sort betrifft nur
> Referenz-Kompositionen — dort vom Pattern-Audit als P2 bestätigt), Doc-33-§3-Split-Äquivalenz +
> escalate-Fehlerpfad (beide durch CoW Rev. 2 gegenstandslos). **Wichtige Ehrlichkeits-Notiz des
> Verifizierers zu M3 (CoW tot für die 320):** Die MESSWERTE des copymem-Pfads sind NICHT verfälscht
> (verifizierter Status quo, rb_exact true) — gebrochen sind Kosten-Annahme, Doku-/Test-Behauptung und
> die als undolog-v1/cowmem-v1 gestempelten Label (auf Copy-Pfad-DLLs „verbrannt" → nächste BuildVersion
> nach dem echten Fix MUSS neu sein, z.B. `cowfix-v1`); mein 0-Zellen-Determinismus-Diff war strukturell
> blind (beide Läufe = identischer Copy-Pfad). Der Fix-Plan §4 bleibt gültig und ist jetzt vollständig
> durch konsolidierte Verdicts unterlegt.

> **Provenienz/Kosten:** Zwei Multi-Agent-Audit-Workflows (User-Auftrag „besseres Modell → Design-Fehler
> suchen" + Pattern-Direktive), zusammen ~70 Agenten / >3 Mio Subagent-Tokens (>300 € — diese Doku + die
> zwei JSON-Rohdaten-Dateien sind die dauerhafte Sicherung; die Workflow-/Temp-Verzeichnisse sind flüchtig).
> **Rohdaten (vollständig, je Befund mit claim/evidence/fix/verdict-reasoning):**
> - `20260611-patternaudit-ergebnis.json` (FERTIGER Workflow `wf_86936298-e41`: 41 Befunde → 41 adversarial
>   verifiziert → **28 bestätigt** [8 blocker / 12 major / 8 minor], 12 widerlegt; 45 Agenten)
> - `20260611-messaudit-rohdaten.json` (PAUSIERTER Workflow `wf_a013b73f-aea` nach 2. Resume: **alle 9
>   Finder fertig = 62 Roh-Befunde** + 73 Einzel-Verdicts; Befund↔Verdict-Zuordnung über die
>   reasoning-Texte; Workflow-Synthese/restliche Verifies offen — Resume:
>   `Workflow({scriptPath: "C:\\Users\\benja\\.claude\\projects\\C--WINDOWS-system32\\78cf67f8-571e-4fcd-a907-1556dbc5be72\\workflows\\scripts\\mess-architektur-voll-audit-wf_a013b73f-aea.js", resumeFromRunId: "wf_a013b73f-aea"})`)
>
> **⛔ KONSEQUENZ SOFORT VOLLZOGEN:** Der (C)-Voll-Lauf (`bb8fz587k`, cowmem-v1) wurde bei 9/320 Tieren
> GESTOPPT — mehrere bestätigte Blocker hätten Teile der 120.960 Messungen invalidiert/verzerrt. Die 320
> cowmem-v1-DLLs + 9 Stamps bleiben liegen (Resume macht den Neustart nach den Fixes billig; nach
> DLL-relevanten Fixes ohnehin neue BuildVersion).

## §1 PATTERN-AUDIT (fertig) — 8 bestätigte BLOCKER

| # | Art | Befund (Kurzform) | Kern-Fix (Details im JSON) |
|---|---|---|---|
| P1 | pattern_violation | **Resource-Control ist ein unausgewiesenes Null Object:** `tier_apply_resource_control` klammert nur und schreibt ins repo-weit WRITE-ONLY `applied_rc_`; kein Organ-Hook existiert (KF-5-§7-A-API nie gebaut); Caps melden hartkodiert 5. **⇒ thread_count{1,2,4}×prefetch_distance{0,8} = 6 physisch identische Settings je Tier — die ×18-Dynamik degeneriert zu ×3 (nur repetition), RC-Spalten = Rauschen, ×6 Messzeit-Verschwendung.** | Organ-Hooks gemäß Design-Doc 20260602-§7-A (set_thread_count/set_prefetch_distance/…, CRTP-No-op-Defaults) ODER ehrlich: 0/caps=0 melden + RC-Dims aus dem Lauf nehmen |
| P2 | runtime_cost | **tier_scan = `save_state()` (O(n)-Vollkopie ALLER Records) + `std::sort` (O(n·log n)) PRO gemessener Scan-Op** (×2 durch two_phase) — misst memcpy+sort statt Scan-Fähigkeit; verletzt „Für Suche IMMER Bäume" | GoF-Iterator als Organ-API (in-order/lower_bound+next), tier_scan O(scan_len + log n); save_state nur Memento/Diagnose |
| P3 | runtime_cost | **Verdeckter Doppel-Lookup pro insert in BEIDEN Observable-Hüllen** (`is_new`-Rekonstruktion via lookup; Ergebnis vom Adapter verworfen) — volle zweite Traversierung in jeder gemessenen Insert-Op | is_new via O(1)-`occupied_count()`-Delta (exakt das abi_adapter-eigene Muster) |
| P4 | runtime_cost | **container_ (Allocator-Messpfad) = SortedBinaryTraversal über LayoutAwareChunkedStore → `insert_slot_at`/`erase_slot_at` = flatten+rebuild O(n) MIT Re-Allokationen in JEDEM gemessenen tier_insert/tier_erase** — der Mess-Apparat dominiert die Wall-Clock (deckungsgleich mit Mess-Audit M8) | LinearScanTraversal für container_ (Append-Pfad; Allocator-/Layout-Treibung identisch) oder Append-basierte Slot-Ops |
| P5 | runtime_cost | **`std::function`-basiertes `observer_.notify()` in JEDER gemessenen Op** (Hüllen-insert/lookup/erase + Allocator) — Push-Mechanik ohne je gesetzten Subscriber (über extern-C nicht setzbar), opaker Runtime-Check im Hot-Pfad | notify entfernen (Transport ist pull via statistics()) oder compile-time NotifyPolicy (NullNotify, zero-cost) |
| P6 | pattern_violation | **Phantom-Allocator-Policy:** NodeChunkedStore/LayoutAwareChunkedStore constrainen Allocator-Policy A, allozieren aber via Default-`std::allocator` und **FABRIZIEREN** `allocator_statistics()` aus Zählern — **die T6-Achse misst den Policy-Allokator NIE** (ComposedStore macht es korrekt vor) | Chunk-Speicher real über A beziehen (A::StdAllocatorAdapter) + A::statistics() durchreichen |
| P7 | runtime_cost | (= P3-Zwilling aus zweiter Linse, eigenständig verifiziert) Doppel-Traversierung je insert | s. P3 |
| P8 | runtime_cost | (= P5-Zwilling aus zweiter Linse) std::function-notify in Hot-Ops | s. P5 |

**12 bestätigte MAJOR (Pattern/Etiketten — volle Texte im JSON):** SearchAlgorithmAbiAdapter hat **kein
Adaptee** (Anatomie wird nie instanziiert, sondern parallel re-implementiert — GoF-Adapter-Bruch);
Memento-Etikett dreifach gebrochen (memento_all kapselt NICHT „alle stateful Achsen", MementoAggregate/
save_axis/restore_axis = tote Artefakte; das Rev.-1-„Undo-Log" wäre kanonisch Command-Undo gewesen);
**„Hybrider Visitor" existiert nicht im Code** (kein accept/visit/Double-Dispatch; `builder/algorithm_visitor/`
enthält nur eine CMakeLists.txt) — 3× unabhängig gefunden; **Release-/funktional-only-DLL zahlt die
Mess-Kopplungen trotzdem** (container_-Doppel-Insert + ct/map/q1/q2 NICHT compile-time entfernt — bricht die
dokumentierte Zero-Overhead-Zusage); **„Experiment-B+-Baum" ist kein B+-Baum** (weder Modell noch
Implementierung — Benennungs-/Anspruchs-Frage); Abstract-Factory degeneriert (1 ConcreteFactory + verbotenes
bool-Flag); Command-Anspruch nicht umgesetzt (anatomy_commands = tote Parallelstruktur, Mess-Pfad ruft tier_*
direkt); **MeasurableObserver ist kein GoF-Observer** (Single-Callback-Slot mit Replace-Semantik statt
one-to-many) — 2× gefunden. **8 MINOR + 12 refuted:** im JSON.

## §2 MESS-ARCHITEKTUR-AUDIT (alle 9 Finder fertig; 62 Roh-Befunde) — Blocker-Cluster

Mehrfach unabhängig gefundene, teils bereits einzel-verifizierte Kern-Befunde (Volltexte + Verdicts im JSON):

| # | Cluster | Kern-Aussage | Konsequenz |
|---|---|---|---|
| M1 | RC-Placebo | = P1 (von 3 Mess-Linsen ebenfalls gefunden) | ×18→×3 degeneriert |
| M2 | **ns_per_op-Faktor 2** | Divisor `2*n_ops` stammt aus dem Legacy-Fix-Workload (insert+lookup); der Workload-Pfad liefert aber GENAU n_ops getimte Ops → **ns_per_op aller Lastprofil-Zeilen systematisch HALBIERT** (7× gefunden; total_ns korrekt) | CSV-Spalte falsch; trivialer Fix |
| M3 | **CoW/Undo tot für die 320** | Die 4 produktiven FullPilot-search_algo-Wrapper haben **kein `restore_statistics`** → `cow_capable_`(wie zuvor `undo_log_capable_`) = false → **stiller Rückfall auf eager copymem** für ALLE 320 Voll-Lauf-Tiere (NUR die Referenz-Kompositionen Art/Hot/Masstree haben die Hüllen — der 42/42-Test prüfte genau diese, der Voll-Lauf nutzt andere!) | erklärt den cowmem-Smoke ≈ copymem; Fix: restore_statistics in die 4 Wrapper (Goldstandard-API) ODER Hüllen auch im FullPilot |
| M4 | **tier_scan No-Op für die 320** | Dieselben 4 Wrapper sind kein MementoAxis → `if constexpr` macht tier_scan zum **leeren Aufruf** → ycsb_e/lp_range_scan messen Funktionsaufruf-Latenz als „Scan" (für Referenz-Kompositionen gilt stattdessen P2: sort-dominiert) | 2/21 Profile invalide; Fix = P2-Iterator |
| M5 | **Run-Phase = nur Upserts** | Load-Phase füllt exakt [1, records] VOLL → jede „Insert"-Op der Run-Phase trifft einen existierenden Key = **Upsert**; Insert-lastige Profile messen nie echtes Einfügen/Wachstum | Profile-Semantik; Fix: Key-Räume Load/Insert trennen |
| M6 | **CoCo-neg50 konfundiert** | `coco_p04_neg50.xml` nutzt **zipfian**, die 4 Sweep-Geschwister uniform → der Negativ-Sweep ist an der 50%-Stufe konfundiert (XML-Tippfehler) | 1-Zeilen-XML-Fix |
| M7 | **Stamp trotz Write-Fehler** | `result.csv.stamp` wird auch geschrieben, wenn der result.csv-Write fehlschlug (`if (pf)` prüft nur open) → stale Zeilen dauerhaft als aktuell zertifizierbar | kleiner Fix (pf.good() nach Schreiben + rows-Gate koppeln) |
| M8 | **Apparat dominiert Wall-Clock** | In JEDER getimten Op: container_-O(n)-Rebuild (=P4) + T1/T2-Auto-Kopplungs-Linear-Scans → die gemessene „Tier-Latenz" misst überwiegend den Mess-Apparat, nicht das Such-Organ | = P3/P4/P5-Fix-Paket |

**Wichtige MAJOR-Cluster (Auswahl; alle im JSON):** Resume-Stempel deckt **XML-Profil-INHALT** nicht
(nur id-Liste; gestempelter seed ≠ effektiver XML-Seed; env_limits fehlen) — Profil-Edit mischt still alte/
neue Semantik (5× gefunden); Workload-Achse hängt am unvalidierten `COMDARE_LOAD_PROFILE_DIR`-env (fehlend →
stiller Lauf OHNE Achse 2, der fertige Ergebnisse überschreiben kann); **Zipfian/Latest ohne Key-Scrambling**
(Hot-Set = zusammenhängender Key-Block — widerspricht YCSB-Treue, reproduziert Doc-32-Bias); Konformitäts-
Gate (v5 §6 „import→GATE→messen") wird im Voll-Lauf-Pfad nie aufgerufen; `fill_segment_timing` hinterlässt
aufgeblähte T0-Stats (Multi-Checkpoint-Traces); **Zwei-Phasen-„Second-Execution"-Effekt**: der Warmup nullt
genau die Cache-/BPU-Signale, die memory_layout/prefetch differenzieren sollen (Grundsatz-Diskussion!);
seg_ns degeneriert für die 320 zu n=1-Schleifen (Key-Ernte braucht MementoAxis); statische prefetch-Achse
misst Tracker-Buchhaltung über Key-WERTE als Pseudo-Adressen; **uint16-Keys der 03a-Pilot-Organe** → stille
uint64-Trunkierung; records>32767 bricht Negativ-Query-Garantie + Inverse-Eindeutigkeit; Doc-33-§3-Äquivalenz
falsch für Splits (Rev.-1-bezogen, durch CoW Rev. 2 teilweise gegenstandslos); Index-Selektion erste-N =
konfundierte search_algo-Stichprobe. **Diverse MINOR** (Load-Phase in stat_*-Spalten dokumentations-widrig;
XML-`<records>` geparst-aber-verworfen; CSV-Schreiben ohne Fehlerprüfung; clock-Overhead; rb_exact-Probe
prüft zu wenig; …): im JSON.

## §3 Bereits-gegenstandslos-Prüfung (CoW Rev. 2 von heute Nachmittag)

Befunde, die NUR das Rev.-1-Undo-Log betrafen (Inverse-Replay-Exaktheit, escalate-Fehlerpfad-Inkonsistenz,
Doc-33-§3-Split-Äquivalenz, Phantom-Einträge-bei-uint16-Aliasing im Replay), sind durch den CoW-Umbau
GEGENSTANDSLOS — **ABER M3 bleibt voll gültig** (die Capability-Detection scheitert für die 320 identisch
am fehlenden `restore_statistics`, der Rückfall ist jetzt eben copymem-eager statt CoW — Korrektheit ok,
Kosten-/Anspruchs-Problem bleibt) und die uint16-Key-Befunde gelten UNABHÄNGIG vom Memento.

## §4 Fix-Plan (Triage-Vorschlag, Reihenfolge — Umsetzung NACH User-Freigabe/morgen)

**Welle 1 — billig + mess-kritisch (vor jedem neuen Voll-Lauf):** M6 (XML 1 Zeile) · M2 (ns_per_op-Divisor)
· M7 (Stamp-Gate) · Resume-Stempel um XML-Inhalts-Hash + env_limits erweitern · env-Guard für
COMDARE_LOAD_PROFILE_DIR (Abbruch statt stiller Fallback) · M5 (Load-/Insert-Key-Räume trennen).
**Welle 2 — Mess-Apparat-Reinheit (DLL-Neubau, eine BuildVersion):** P3/P7 (is_new via occupied_count) ·
P5/P8 (NotifyPolicy/pull-only) · P4 (container_-LinearScan) · P6 (echter Policy-Allocator) · M3
(restore_statistics in die 4 Pilot-Wrapper → CoW real aktiv) · P2/M4 (Iterator-Scan als Organ-API) ·
uint16→uint64-Key-Frage der Pilot-Organe.
**Welle 3 — Dimension/Validität:** P1 (RC-Organ-Hooks ODER RC-Dims ehrlich raus) · Zipfian-Scrambling ·
Konformitäts-Gate in den Voll-Lauf-Pfad · Index-Selektion → search_algo-balanciert.
**Welle 4 — Pattern-Hygiene (kein Mess-Einfluss):** Adapter-/Memento-/Visitor-/Command-/Observer-Etiketten
bereinigen (umsetzen oder ehrlich umbenennen, gemäß Pattern-Direktive) · „B+-Baum"-Benennung klären ·
Release-DLL-Zero-Overhead einlösen (Mess-Kopplungen unter COMDARE_MEASUREMENT_ON).
**Grundsatz-Klärung mit User (Mess-Semantik):** der „Second-Execution"-Einwand gegen den Zwei-Phasen-Warmup
(er ist dokumentierte PFLICHT-Direktive — der Einwand gehört diskutiert, nicht stillschweigend umgesetzt).

## §5 Status-Festschreibung

- (C)-Voll-Lauf GESTOPPT bei 9/320 (cowmem-v1-DLLs 320/320 gebaut, bleiben nutzbar bis Welle-2-Fixes).
- Pattern-Audit-Workflow KOMPLETT (Task #141 → Auswertung in dieser Doku; 1 Verify-Agent crashte
  API-seitig: „SearchAlgorithmAbiAdapter: Adapter-A" — sein Zwilling wurde als major bestätigt).
- Mess-Audit-Workflow PAUSIERT mit 9/9 Findern + 73/~62-Verifies (Restliste + Workflow-Synthese offen;
  Resume-Kommando oben). Tasks #140/#141 tracken die Rest-Triage.
