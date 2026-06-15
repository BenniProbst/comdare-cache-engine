# Voll-Audit 2026-06-11/12 — Vollständige Erkenntnis-Dokumentation (Sicherung)

> **Zweck dieses Verzeichnisses:** Dauerhafte, committete Sicherung der vollständigen Ergebnisse der zwei
> Multi-Agent-Audit-Workflows (User-Investition >300 € Token-Kosten; ~116 Agenten, >4,3 Mio Subagent-Tokens,
> ~930 Tool-Aufrufe). Die Workflow-Laufzeit-Verzeichnisse und Temp-Dateien sind flüchtig — ALLES Tragende
> liegt jetzt hier bzw. in `docs/sessions/` (siehe Manifest §8).

## §1 Methodik (warum diese Ergebnisse belastbar sind)

Zwei getrennte Workflows, beide nach demselben zweistufigen Schema:
1. **Finden:** unabhängige Prüf-Linsen (Mess-Audit: 9 — undolog, resume, workload, twophase_observer,
   csv_schema, konformitaet, baum_bindung, statistik, design_fresh; Pattern-Audit: 4 — anatomy, builder,
   axes_compositions, metaprog_zero_cost), jede mit eigenem Datei-Korpus, Architektur-Doku-Referenzen und
   dem Auftrag, NUR code-belegte Befunde (file:line + Zitat + Konsequenz + Fix) zu liefern. Das
   Pattern-Audit zusätzlich mit Pflicht-Web-Verifikation der kanonischen Pattern-Definitionen (GoF,
   Alexandrescu, Coplien, …).
2. **Adversarial verifizieren:** JEDER Befund ging an einen eigenen Skeptiker mit explizitem
   WIDERLEGUNGS-Auftrag und eigener Code-Lektüre; `real=true` nur bei literaler Bestätigung, Severity
   wurde unabhängig nachkorrigiert. Mehrere Linsen fanden dieselben Kern-Defekte unabhängig (bis zu 6×) —
   konvergente Evidenz.

**Qualitätsbeleg:** Die 17 Widerlegungen (5 Mess + 12 Pattern) sind präzise begründet — darunter exakt die
Befunde, die durch den zwischenzeitlichen CoW-Rev.-2-Umbau gegenstandslos geworden waren. Das System hat
also nicht nur gefunden, sondern auch korrekt NICHT-bestätigt.

## §2 Gesamtbild in Zahlen

| Workflow | Roh | Verifiziert | Bestätigt | blocker | major | minor | widerlegt |
|---|---|---|---|---|---|---|---|
| Mess-Architektur (`wf_a013b73f-aea`, 71 Agenten) | 62 | 62 | **57** | 24 | 24 | 9 | 5 |
| Pattern/Zero-Cost (`wf_86936298-e41`, 45 Agenten) | 41 | 41 | **28** | 8 | 12 | 8 | 12 |
| **Summe** | 103 | 103 | **85** | 32 | 36 | 17 | 17 |

Die 32 Blocker-Nennungen verdichten sich auf **10 distinkte Kern-Defekte** (§3) — der Rest sind unabhängige
Mehrfach-Funde derselben Defekte aus verschiedenen Linsen (Konvergenz).

## §3 Die 10 Kern-Defekte (konsolidiert, beide Audits)

**K1 — Resource-Control ist ein write-only Null Object (6× gefunden, härtest-verifiziert).**
`tier_apply_resource_control` klammert die Werte und schreibt sie in `applied_rc_` — das repo-weit NIE
gelesen wird; die in Design-Doc §7-A spezifizierte Organ-API (set_thread_count/set_prefetch_distance/…)
wurde nie gebaut; die Caps melden hartkodiert 5 „steuerbare" Achsen. **Folge:** thread_count{1,2,4} ×
prefetch_distance{0,8} sind 6 physisch identische Settings — die ×18-Dynamik des Voll-Laufs degeneriert zu
×3 (nur repetition), die RC-Spalten der Thesis-Daten wären als Effekt interpretiertes Rauschen, ×6
Messzeit-Verschwendung. Der Ledger feierte `applied_axis_count>0` als Verifikation (Etiketten-Falle).

**K2 — ns_per_op systematisch HALBIERT für alle Lastprofil-Zeilen (6× gefunden).**
Der Divisor `2*n_ops` stammt aus dem Legacy-Fix-Workload (n_ops Inserts + n_ops Lookups); der
Workload-Pfad (Achse 2) liefert aber GENAU n_ops getimte Ops. `total_ns` ist korrekt — nur die
abgeleitete Spalte ist falsch. Trivialer Fix, aber jede bisherige ns_per_op-Interpretation der
Profil-Zeilen war 2× zu optimistisch.

**K3 — Memento-Capability für ALLE 320 Voll-Lauf-Lebewesen compile-time tot (3× gefunden).**
`organ_*_capable_v` verlangt `restore_statistics` — das existiert NUR in den zwei Observable-Hüllen. Die
320 FullPilot-DLLs tragen im search_algo-Slot die ROHEN Registry-Wrapper (KAry/Interpolation/Eytzinger/
LinearScanSearchAlgo: statistics()/reset() ja, restore_statistics nein) → Capability false → stiller
if-constexpr-Rückfall auf das eager Copy-Memento. **Das galt für Rev. 1 (Undo-Log) UND gilt identisch für
Rev. 2 (CoW).** Der 42/42-Semantik-Test prüfte nur die Referenz-Kompositionen Art/Hot/Masstree (deren Slot
die Hülle ist) — Test-Population ≠ Ziel-Population. Verschärfung des Verifizierers: die als undolog-v1/
cowmem-v1 gestempelten DLLs kompilierten nachweislich den Copy-Pfad (Label = reiner Harness-Parameter,
„verbrannt"); der 0-Zellen-Determinismus-Diff war strukturell blind (beide Seiten = identischer Code-Pfad).
Messwerte des Copy-Pfads sind dabei NICHT verfälscht (verifizierter Status quo, rb_exact true) — gebrochen
sind Kosten-Annahme, Doku-/Test-Behauptung und Label-Semantik.

**K4 — tier_scan: No-Op für die 320 / Vollkopie+sort für die Referenz-Klasse (4× gefunden).**
Die 320er-Wrapper sind kein MementoAxis → `if constexpr` macht tier_scan zum leeren Aufruf → die
Scan-Profile (ycsb_e, lp_range_scan) messen Funktionsaufruf-Latenz als „Scan". Für MementoAxis-Organe
(Referenz-Kompositionen) gilt stattdessen: `save_state()` (O(n)-Vollkopie ALLER Records) + `std::sort`
(O(n·log n)) PRO gemessener Scan-Op — Memento-Mechanik als Iterator-Ersatz, misst memcpy+sort statt
Scan-Fähigkeit, verletzt „Für Suche IMMER Bäume". Fix: GoF-Iterator als Organ-API.

**K5 — Der Mess-Apparat dominiert die gemessene Wall-Clock (4 Einzel-Defekte, Pattern-Audit).**
In JEDER getimten Op stecken: (a) verdeckter Doppel-Lookup (is_new-Rekonstruktion in beiden Hüllen,
Ergebnis verworfen), (b) `std::function`-basiertes `observer_.notify()` ohne je setzbaren Subscriber,
(c) der Schatten-Container `container_` = SortedBinaryTraversal über LayoutAwareChunkedStore →
`insert_slot_at`/`erase_slot_at` = flatten+rebuild **O(n) mit Re-Allokationen** je tier_insert/tier_erase,
(d) T1/T2-Auto-Kopplungs-Buchführung. Die „Lebewesen-Latenz" misst damit überwiegend Apparat, nicht Such-Organ.

**K6 — Phantom-Allocator: die T6-Achse misst den Policy-Allokator NIE (Pattern-Audit).**
NodeChunkedStore/LayoutAwareChunkedStore constrainen die Allocator-Policy A, allozieren aber über den
Default-`std::allocator` und FABRIZIEREN `allocator_statistics()` aus eigenen Zählern (ComposedStore macht
es korrekt vor). Die Allocator-Achsen-Werte der Matrix beschreiben also nie das Verhalten von A.

**K7 — Workload-Treue-Brüche (workload-Linse).**
(a) `coco_p04_neg50.xml` nutzt **zipfian**, die 4 Sweep-Geschwister uniform → der CoCo-Negativ-Sweep ist an
der 50%-Stufe konfundiert (1-Zeilen-XML-Bug). (b) Die Load-Phase füllt exakt [1, records] VOLL → jede
„Insert"-Op der Run-Phase ist ein **Upsert**; Insert-Profile messen nie echtes Einfügen/Wachstum.
(c) Zipfian/Latest ohne Key-Scrambling → das Hot-Set ist ein zusammenhängender Key-Block (widerspricht
YCSB-Treue und reproduziert genau den Doc-32-Bias, den der Cross-Run brechen soll).

**K8 — Resume-/Re-Entry-Härte unvollständig (5× gefunden + Harness-Blocker).**
Der Config-Stempel deckt die XML-Profil-INHALTE nicht (nur die id-Liste; der gestempelte seed ist im
XML-Pfad wirkungslos; env_limits fehlen); der Stamp wird auch bei fehlgeschlagenem result.csv-Write
geschrieben; er gated Vollständigkeit, nicht Gültigkeit (two_phase_valid=0 wird eingefroren); der
Resume-Check liegt NACH dem b.ok()-Gate (Build-Fehler → fertige Binaries fehlen still in der globalen
CSV); und der **Harness selbst pinnt weder COMDARE_LOAD_PROFILE_DIR/WORKLOAD_RECORDS noch die
Voll-Lauf-Parameter** (Defaults NOps=2000/BuildVersion=tier150-v1) — ein argloser Relaunch nach Reboot
misst eine falsche Matrix und ÜBERSCHREIBT fertige Ergebnisse. Generell: Die Workload-Achse hängt an
unvalidiertem Shell-Env (fehlend → stiller Lauf OHNE Achse 2).

**K9 — Validitäts-Pfade fehlen/degenerieren.**
Das bindende Konformitäts-Gate (v5 §6 „import → GATE → messen") wird im Voll-Lauf-Pfad nie aufgerufen;
seg_ns (Per-Achsen-Timing) degeneriert für die 320 zu n=1-Op-Schleifen (Key-Ernte erfordert MementoAxis);
die statische prefetch-Achse hat im Pfad-B keinen Prefetch-Mechanismus (misst Tracker-Buchhaltung über
Key-WERTE als Pseudo-Adressen); die uint16-Keys der 03a-Pilot-Organe trunkieren uint64 still und brechen
ab records>32767 die Negativ-Query-Garantie; die Index-Selektion (erste N) erzeugt eine konfundierte
search_algo-Stichprobe.

**K10 — Pattern-/Etiketten-Integrität (Pattern-Audit, 12 Major).**
SearchAlgorithmAbiAdapter hat **kein Adaptee** (die Anatomie wird nie instanziiert, sondern parallel
re-implementiert); das Memento-Etikett ist mehrfach gebrochen (memento_all kapselt nicht „alle stateful
Achsen"; MementoAggregate/save_axis/restore_axis = tote Artefakte); der dokumentierte **„Hybride Visitor"
existiert nicht im Code** (kein accept/visit/Double-Dispatch; algorithm_visitor/ enthält nur eine
CMakeLists.txt — 3× unabhängig gefunden); der Command-Anspruch ist nicht umgesetzt (anatomy_commands =
tote Parallelstruktur); MeasurableObserver ist kein GoF-Observer (Single-Callback-Slot mit
Replace-Semantik); der „Experiment-B+-Baum" ist strukturell kein B+-Baum; die Abstract Factory ist
degeneriert (1 ConcreteFactory + bool-Flag); und die Release-/funktional-only-DLL zahlt die
Mess-Organ-Kopplungen entgegen der dokumentierten Zero-Overhead-Zusage.

## §4 Vollständige Major-Liste Mess-Audit (24 — Volltexte im Endergebnis-JSON)

Stamp-trotz-Write-Fehler · Stempel ohne XML-INHALT (id-Liste; seed wirkungslos) ×5-Linsen ·
Resume-Check nach b.ok()-Gate (Build-Fehler → CSV-Lücke bei Exit 0) · XML-`<records>` geparst-aber-
verworfen · XML ohne Validierung (fehlendes op_mix → stilles Default-Mix inkl. 1% Clear) ·
stat_*-Spalten enthalten die Load-Phase (×3-Linsen, widerspricht dokumentierter Semantik) ·
fill_segment_timing hinterlässt aufgeblähte T0-Stats (Phantom-Lookups ab Checkpoint 2) ·
Doc-33-§2-„exakt 2×"-Zusicherung falsch für Schwellen-/Peak-Statistiken (q1 over/underflow, T7
max_queue_depth — nicht-lineare Zähler) · keine Separator-/Eindeutigkeits-Validierung der Wire-/CSV-Kette
(';' in IDs verschiebt Felder still) · globale CSV ohne Stream-Fehlerprüfung (Exit 0 trotz möglichem
Totalverlust) · Doc-32-Katalog ≠ 21 XMLs (LP11-read_ratio-Sweep fehlt, LP02 ohne Profil) ·
Memento-Capability-tot (Zweitnennungen aus statistik/design_fresh) · Stempel gated nicht Gültigkeit ·
**Second-Execution-Grundsatzeinwand** (der Zwei-Phasen-Warmup nullt genau die Cache-/BPU-Signale, die
memory_layout/prefetch differenzieren sollen — NUR zur Diskussion: Zwei-Phasen ist User-Pflicht-Direktive) ·
uint16-Keys (Zweitnennung) · Index-Selektion konfundiert.

## §5 Vollständige Minor-Listen

**Mess (9):** uint16-Aliasing-Phantom-Einträge im (inzwischen ersetzten) Replay · OOM-Degradationspfade
halb-zurückgerollt+unmarkiert · env_limits fehlt im Stempel / Label trägt requested statt applied ·
Vollständigkeits-Gate zählt besuchte Settings statt Matrix-Größe · rb_exact-Probe zertifiziert weniger als
behauptet (nur size==0 nach 1 Insert) · 2× Kommentar-Drift (two_phase-Vertrag gilt nur T0/T6;
IRollbackableTier-Header verspricht „alle stateful Achsen") · clock::now()-Overhead additiv in total_ns ·
uint16-Negativ-Garantie (Drittnennung). **Pattern (8):** im Pattern-JSON (u. a. Naming-Konventions-Fälle).

## §6 Widerlegte Befunde (17 — Beleg der Verifikations-Qualität)

**Mess (5):** 2× BuildVersion-Drift-Varianten (Harness-Parameter ist dokumentierte Praxis) · tier_scan-
sort-Variante (für die 320 gilt der No-Op-Fall; sort nur Referenz-Klasse → dort vom Pattern-Audit als P2
bestätigt — beide Audits konsistent) · Doc-33-§3-Split-Äquivalenz + escalate-Fehlerpfad (beide durch CoW
Rev. 2 gegenstandslos — die Verifizierer lasen den aktuellen Code). **Pattern (12):** im Pattern-JSON
(refuted-Liste mit Begründungen).

## §7 Meta-Erkenntnisse (die teuersten Lehren, generalisiert)

1. **Capability-Detection darf nie still degradieren:** `if constexpr (capable)` mit Fallback ist elegant,
   aber ohne `static_assert` über die ZIEL-Population ist der Fallback unsichtbar (K3). Lehre: Jede
   requires-gestützte Pfadwahl braucht einen Test, der die produktiven Typen instanziiert.
2. **Test-Population ≠ Ziel-Population:** 42/42 grün auf Referenz-Kompositionen bewies nichts über die
   320 Pilot-Wrapper. Lehre: Verifikation muss mindestens einen echten Vertreter jeder Ziel-Klasse decken.
3. **Differenz-Beweise können strukturell blind sein:** Der 0-Zellen-Diff verglich zweimal denselben
   Code-Pfad. Lehre: Ein Äquivalenz-Beweis braucht den Nachweis, dass die VERGLICHENEN Pfade verschieden
   sind (z. B. Diagnose-Flag im Output).
4. **Etiketten-Drift ist messbar gefährlich:** „applied_axis_count>0" wurde als Erfolg gefeiert, obwohl
   nichts angewandt wurde; BuildVersion-Label versprachen Code-Stände, die nie kompiliert waren. Lehre =
   genau die User-Pattern-Direktive: Namen/Pattern nur tragen, wenn die kanonische Semantik erfüllt ist.
5. **Dokumentierte Semantik gehört in Tests, nicht in Kommentare:** Mehrere Majors sind reine
   Kommentar-Realität-Divergenzen („kodiert ALLES", „exakt 2×", „warmup-frei").
6. **Mess-Apparat-Reinheit ist eine eigene Disziplin:** Auto-Kopplungen, Schatten-Container und
   Komfort-Lookups summieren sich in der getimten Op zu dominanten Kosten (K5) — zero-cost-Metaprog-
   Direktive systematisch auf den Hot-Pfad anwenden.
7. **env-/Parameter-Volatilität ist ein Daten-Integritätsrisiko:** Resume-Systeme müssen die GESAMTE
   Konfiguration pinnen oder abbrechen — niemals still mit Defaults weiterlaufen (K8).
8. **Adversariale Mehrfach-Linsen funktionieren:** 6 unabhängige Funde desselben Defekts + 17 saubere
   Widerlegungen zeigen, dass Finden UND Nicht-Bestätigen funktionieren — der Audit-Aufbau ist
   wiederverwendbar (Skripte liegen im workflows/scripts-Verzeichnis der Session, Resume-fähig).

## §8 Manifest dieses Sicherungsverzeichnisses + verwandte Dateien

| Datei (hier) | Inhalt |
|---|---|
| `messaudit-workflow-output.json` (316 KB) | ROHESTE Quelle Mess-Audit: kompletter Workflow-Output (Summary, Logs, konsolidiertes Result mit allen 57+5 Volltexten) |
| `patternaudit-workflow-output.json` (195 KB) | ROHESTE Quelle Pattern-Audit: dito (28+12 Volltexte + Web-Referenzen) |
| `ERKENNTNISSE.md` | DIESE Doku |

Hinweis: Die Workflow-`journal.jsonl` wurden vom Runner nach Workflow-Abschluss automatisch entfernt — ihr
Informationsgehalt ist in den Output-JSONs vollständig enthalten (konsolidierte Results); der
Zwischenstand vor Abschluss ist zusätzlich in `../20260611-messaudit-rohdaten.json` konserviert.

| Verwandt (in `docs/sessions/`) | Inhalt |
|---|---|
| `20260611-audit-ergebnisse-synthese.md` | Synthese + 4-Wellen-Fix-Plan + End-Verdikt-Kopf (Triage-Einstieg) |
| `20260612-messaudit-endergebnis.json` (314 KB) | Konsolidiertes Mess-Endergebnis (= result-Extrakt aus dem Output hier) |
| `20260611-patternaudit-ergebnis.json` (194 KB) | Konsolidiertes Pattern-Ergebnis (= result-Extrakt) |
| `20260611-messaudit-rohdaten.json` (348 KB) | Zwischenstand-Beleg (9 Finder + 73 Verdicts vor der End-Konsolidierung) |

**Status nach den Audits:** (C)-Voll-Lauf gestoppt bei 9/320; 320 cowmem-v1-DLLs auf Disk (Copy-Pfad,
Label verbrannt — verfallen mit Welle 2); Fix-Umsetzung = Task #142 (Wellen 1-4, Synthese §4); neue
BuildVersion nach Welle 2: `cowfix-v1`; Grundsatzfrage „Second-Execution vs Zwei-Phasen-Pflicht" wartet
auf User-Entscheid.
