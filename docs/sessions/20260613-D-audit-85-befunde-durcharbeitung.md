# Phase D — 85 Audit-Befunde Befund-für-Befund (Masterplan D1/D2/D3)

> **Zweck (Masterplan D):** Die zwei teuren Audits (Mess 57 + Pattern 28 = **85 bestätigt**, >300 €) NICHT nur
> referenzieren (A3), sondern Befund für Befund gegen den konsolidierten Stand (Doc 34) + den IST-Code (A2)
> durcharbeiten → **D1** Disposition je Befund, **D2** Doc-34-§9-Erweiterung (SOLL-Korrekturen), **D3** audit-
> fundierte E-Aufgaben-Liste. **Quellen:** `audit-sicherung-20260612/ERKENNTNISSE.md` (§3 K1–K10, §4 Major, §5
> Minor, §6 Refuted, §7 Meta) + `20260611-audit-ergebnisse-synthese.md` (§1 Pattern P1–P8 + 12 Major, §2 Mess
> M1–M8, §4 4-Wellen-Fix-Plan) + JSON-Volltexte on-demand (`20260612-messaudit-endergebnis.json` /
> `20260611-patternaudit-ergebnis.json`). **Done-Kriterium je Befund (Goal §2.5):** (a) gefixt mit literalem
> Beleg / (b) Daten-Limitierung im Appendix / (c) User-Entscheid pending. **Re-gegroundet 2026-06-13:** Doc 34 +
> A1/A2/A3 + GOAL + MASTERPLAN genuin im Kontext gelesen.

## §0 Zähl-Abgleich + Disposition-Schema

- **85 bestätigt** = 57 Mess (24 blocker / 24 major / 9 minor) + 28 Pattern (8 blocker / 12 major / 8 minor).
  Die **32 Blocker-Nennungen verdichten sich auf 10 distinkte Kern-Defekte K1–K10** (Rest = konvergente
  Mehrfach-Funde derselben Defekte aus verschiedenen Linsen). + **17 Widerlegungen** (5 Mess / 12 Pattern).
- **Disposition-Codes:** **[FIX-DONE]** code-gefixt (A2-Beleg) · **[FIX-E]** offen, E-Welle (cowfix-v1) ·
  **[LIMIT]** Daten-Limitierung im Appendix (Goal §2.5-b) · **[USER]** User-Entscheid pending (nominal/nicht-block) ·
  **[GEGENSTANDSLOS]** durch zwischenzeitlichen Umbau erledigt · **[DOC]** reine Doku/Test/Etikett (kein Mess-Einfluss).
- **Architektur-Anker:** jeder Befund hängt an einem Doc-34-Konzept (§-Verweis). **Zentral:** der dominierende
  Mess-Echtheits-Defekt = **Befund-2/Q2-Schritt-4 (Doc 34 §9)** — K5+K6 hängen daran; Meta-Lehre #3 macht ihn
  mission-kritisch (Achsen-Austauschbarkeit darf nicht Apparat-Artefakt sein).

## §1 K1–K10 Kern-Defekte — volle Disposition

| K | Befund (verdichtet, Audit) | IST-Code-Status (A2-verifiziert) | Disposition | Arch-Anker (Doc 34) | E-Welle |
|---|---|---|---|---|---|
| **K1** | RC = write-only Null Object: `applied_rc_` nie gelesen; KF-5-§7-A-Organ-API nie gebaut; Caps hartkodiert 5 → thread×prefetch = 6 identische Settings → ×18 degeneriert zu ×3; RC-Spalten = Rauschen; ×6 Mess-Verschwendung | RC-Dim wirkt nicht (Audit, 6× konvergent) | **[USER]** + bis dahin **[LIMIT]** (RC nominal) | Lastenprofil §5 / Algorithm_Resource_Control (KF-4) | A3 (needs_user, Goal §3) |
| **K2** | ns_per_op systematisch HALBIERT (Divisor `2*n_ops` aus Legacy-Fix-Workload; Workload-Pfad liefert n_ops) | **GEFIXT** (M1.1 `timed_ops`, Smoke `ns_per_op==total_ns/timed_ops`) | **[FIX-DONE]** | Lebewesen-Wall-Clock §5 / perm_runner | — (A1✅) |
| **K3** | Memento/CoW compile-time TOT für die 320: produktive search_algo-Wrapper ohne `restore_statistics` → stiller copymem-Rückfall; 42/42-Test prüfte nur Referenz-Komp. (Test-Pop≠Ziel-Pop) | restore_statistics in 13 lookup-Wrappern + grüner static_assert über Ziel-Pop (ce `4a64bc8`, C3-behalten); Adapter-CoW real (A2.1 :1158) | **[FIX-E]** (Kern; cowfix-v1-Neubau aktiviert es real) | Memento Rev.2 CoW §7 / Meta-Lehre #1/#2 | A2 |
| **K4** | tier_scan No-Op für die 320 / Vollkopie+sort für Referenz → ycsb_e/lp_range_scan messen Aufruf-Latenz | IScannableTier additiv da; tier_scan über save_state-Snapshot (A2.1) | **[FIX-E]** (+ bis dahin **[LIMIT]** 2/21 Profile invalide) | GoF-Iterator-Organ / „Für Suche IMMER Bäume" | A2 |
| **K5** | Apparat dominiert Wall-Clock: (a) verdeckter Doppel-Lookup, (b) `std::function`-notify ohne Subscriber, (c) `container_` O(n)-flatten+rebuild/Op, (d) T1/T2-Buchführung | **(a) GEFIXT** (A2.1 :644/:677 occupied_count-Delta); (b)/(c)/(d) offen | **(a) [FIX-DONE]** · (b)(c)(d) **[FIX-E]** | **Befund 2 §9** + Apparat-Reinheit (Meta #6, zero-cost Hot-Pfad) | A2 |
| **K6** | Phantom-Allocator: Stores constrainen Policy A, allozieren via std::allocator + FABRIZIEREN `allocator_statistics()` → T6 misst A nie | container_=NodeChunkedStore node-abhängig (A2.1 :879, Q2-1-3); T6 aus Datenzustand DERIVIERT — Such weiter Monolith | **[FIX-E]** (= Q2-Schritt-4-Komplex) | **Befund 2 §9** / allocator-Achse | A2 |
| **K7** | Workload-Treue: (a) coco_neg50=zipfian, (b) Run-Phase nur-Upserts, (c) Zipfian/Latest ohne Key-Scrambling | (a) **GEFIXT** (M1.2 uniform); (b)/(c) offen | **(a) [FIX-DONE]** · (b)(c) **[FIX-E]** | Lastprofil/Paper-Bias §8 / Reproduzierbarkeit §5 | A1+A3 |
| **K8** | Resume-Härte: Stamp ohne XML-Inhalt (seed wirkungslos, env_limits fehlen); Stamp-trotz-Write-Fehler; gated≠gültig; Resume-Check nach b.ok(); Harness pinnt env/Params nicht | **teilw. GEFIXT** (Stamp-Gate/env-Guard/Harness-Pinning M1.3/1.4; lazy_try_resume+stamp A2.6); XML-Hash/env_limits/Resume-vor-b.ok offen | **[FIX-DONE]** (Teil) + **[FIX-E]** (batch_for_m3) | Mess-Resume Config-Stamp §7 / Meta #7 | A1 |
| **K9** | Validität: Gate nie gerufen; seg_ns n=1 (320); prefetch misst Key-Werte als Pseudo-Adressen; uint16 trunkiert uint64 (>32767 bricht Negativ-Garantie); Index-Selektion konfundiert | Gate-Code da (V5-I4); seg_ns-n>1-Mechanik via save_state (A2.1 :1007); Rest offen | **[FIX-E]** (+ prefetch/uint16 teils **[LIMIT]**) | Konformitäts-Gate §5 / seg_ns Pfad-B §6 | A2+A3 |
| **K10** | Pattern-Integrität (12 Major): Adapter-ohne-Adaptee · Memento-Etikett ×3 · „Visitor existiert nicht" · Command-tote-Struktur · MeasurableObserver≠Observer · „B+-Baum"≠B+-Baum · degen. Abstract Factory · Release-DLL-Mess-Kopplung | Release-zero-overhead via `#if COMDARE_MEASUREMENT_ON` (A2.1 :121/A2.2) **verifiziert**; Memento-Etikett via cow-Rename adressiert; Rest offen | **[FIX-E]/[DOC]** (kein Mess-Einfluss) | Pattern-Direktive / B1-Split §6 / Baum §3 | A4 |

## §2 Mess-Major (24) — Disposition (die nicht in K1–K10 verdichteten Einzel-Majors)

Die meisten Mess-Major sind Zweitnennungen der K-Cluster. Die eigenständigen Major + ihre Disposition:

| Major | IST/Disposition | E-Welle |
|---|---|---|
| Stamp-trotz-Write-Fehler (`if(pf)` nur open) | **[FIX-DONE]** (M7/Stamp-Gate `pf.good()`) | A1 |
| Stempel ohne XML-INHALT (id-Liste; seed wirkungslos; env_limits fehlen) | **[FIX-E]** (XML-Inhalts-Hash + env_limits in Stamp) | A1 batch_m3 |
| Resume-Check NACH `b.ok()`-Gate (Build-Fehler → stille CSV-Lücke bei Exit 0) | **[FIX-E]** (Resume-Check VOR b.ok()) | A1 batch_m3 |
| XML-`<records>` geparst-aber-verworfen | **[FIX-E]** (records ehren oder ehrlich dokumentieren) | A1 |
| XML ohne Validierung (fehlendes op_mix → stilles Default-Mix inkl. 1% Clear) | **[FIX-E]** (op_mix-Pflicht-Validierung) | A1 |
| stat_*-Spalten enthalten Load-Phase (×3 Linsen, doku-widrig) | **[FIX-E]** (Observer-Reset NACH Load) | A2 |
| fill_segment_timing hinterlässt aufgeblähte T0-Stats (Phantom-Lookups ab Checkpoint 2) | **[FIX-E]** (Q1-Reset-Sequenz härten) | A2 |
| Doc-33-§2-„exakt 2×" falsch für Schwellen-/Peak-Stats (q1 over/underflow, T7 max_queue_depth) | **[DOC]** (Kommentar/Doku-Korrektur, Meta #5) | A4-Doku |
| keine Separator-/ID-Validierung der Wire-/CSV-Kette (';' in IDs verschiebt Felder) | **[FIX-E]** (Separator/ID-Validierung) | A1 |
| globale CSV ohne Stream-Fehlerprüfung (Exit 0 trotz Totalverlust) | **[FIX-E]** (Stream-Fehlercheck) | A1 |
| Doc-32-Katalog ≠ 21 XMLs (LP11-read_ratio-Sweep fehlt, LP02 ohne Profil) | **[FIX-E]** (LP11-Sweep+LP02 ergänzen ODER Katalog korrigieren) | A1 |
| **Second-Execution-Grundsatzeinwand** (Zwei-Phasen nullt Cache/BPU-Signale für layout/prefetch) | **[USER]** (NUR Diskussion; Zwei-Phasen = PFLICHT-Direktive) | A5 |
| uint16-Keys (Zweitnennung) / Index-Selektion konfundiert | **[FIX-E]** (uint16→uint64-Entscheid; SelectMode=search_algo_grid) | A2/A3 |

## §3 Pattern P1–P8 (Blocker) + 12 Pattern-Major — Disposition

| P | Befund | IST/Disposition | Arch-Anker | E-Welle |
|---|---|---|---|---|
| P1 | RC Null Object (= K1/M1) | **[USER]/[LIMIT]** | Lastenprofil §5 | A3 |
| P2 | tier_scan = save_state O(n)+std::sort/Op (= K4/M4, Referenz-Klasse) | **[FIX-E]** GoF-Iterator-Organ | Baum/Iterator §3 | A2 |
| P3/P7 | verdeckter Doppel-Lookup je insert (is_new) | **[FIX-DONE]** (occupied_count-Delta A2.1 :644) | Befund 2 §9 | — (A2✅) |
| P4 | container_ O(n)-flatten+rebuild je Op (= M8) | **[FIX-E]** LinearScan/Append | Befund 2 §9 / K5(c) | A2 |
| P5/P8 | `std::function`-notify ohne Subscriber je Op | **[FIX-E]** compile-time NullNotify (zero-cost) | Apparat-Reinheit Meta #6 / K5(b) | A2 |
| P6 | Phantom-Allocator (= K6) | **[FIX-E]** A::StdAllocatorAdapter + A::statistics() | Befund 2 §9 / Q2-Schritt-4 | A2 |
| **12 Pattern-Major** | | | | |
| — | SearchAlgorithmAbiAdapter **kein Adaptee** (Anatomie nie instanziiert, parallel re-impl) | **[FIX-E]/[DOC]** — verknüpft mit „ZWEI Observe-Mechanismen" (A2.3): abi_adapter nutzt NICHT observe_all → in der Q2-Schritt-4-Konsolidierung vereinheitlichen ODER Etikett ehrlich | Befund 2 §9 / observe_all-vs-abi §6 | A2/A4 |
| — | Memento-Etikett ×3 (memento_all kapselt nicht „alle stateful"; MementoAggregate/save_axis/restore_axis tot) | **[DOC]** (Etikett via cow-Rename + tote Artefakte entfernen) | Memento §7 / Meta #4 | A4 |
| — | **„Hybrider Visitor" existiert nicht** (kein accept/visit/Double-Dispatch; `algorithm_visitor/` nur CMakeLists) ×3 | **[DOC]** (Doku-Etikett entfernen ODER GoF-Visitor real bauen) | Meta #4/#5 | A4 |
| — | Release-/funktional-only-DLL zahlt Mess-Kopplungen (container_-Doppel-Insert + ct/map/q1/q2 nicht compile-time entfernt) | **[FIX-E]** — TEILS schon erfüllt (A2.1 :121 `#if COMDARE_MEASUREMENT_ON` Vererbungs-Split verifiziert); verbleibt: container_-Kopplung unter Flag | Release-Zero-Overhead §6 (B1-Split) | A4 |
| — | „Experiment-B+-Baum" ist kein B+-Baum (Benennung/Anspruch) | **[DOC]** — Benennung gegen Doc 26/§3 klären (Mixed-Radix-Odometer ≠ klassischer B+-Baum; Etikett präzisieren) | Baum §3 | A4 |
| — | Abstract Factory degeneriert (1 ConcreteFactory + bool-Flag) | **[FIX-E]/[DOC]** — Static/DynamicAxisNode echte Factory (Doc 34 §3) | Baum §3 / Abstract Factory | A4 |
| — | Command-Anspruch tot (anatomy_commands Parallelstruktur, Mess-Pfad ruft tier_* direkt) | **[DOC]** (Etikett klären ODER Command real verdrahten) | Verantwortlichkeit §2 | A4 |
| — | MeasurableObserver ≠ GoF-Observer (Single-Callback Replace statt one-to-many) ×2 | **[DOC]** — koppelt an P5 (notify-Mechanik). Etikett präzisieren ODER pull-only umbenennen | Observer §6 / Meta #4 | A4 |

## §4 Minor (17) — Disposition (Sammelabarbeitung je Welle)

**Mess (9):** uint16-Aliasing-Phantom (Replay ersetzt → **[GEGENSTANDSLOS]** CoW Rev.2) · OOM-Degradationspfade unmarkiert **[FIX-E/LIMIT]** · env_limits fehlt im Stempel / Label requested-statt-applied **[FIX-E]** A1 · Vollständigkeits-Gate zählt besuchte statt Matrix-Größe **[FIX-E]** · rb_exact-Probe zertifiziert zu wenig (nur size==0 nach 1 Insert) **[FIX-E/DOC]** · 2× Kommentar-Drift (two_phase-Vertrag T0/T6; IRollbackableTier-Header) **[DOC]** Meta #5 · clock::now()-Overhead additiv in total_ns **[LIMIT]** (dokumentieren) · uint16-Negativ-Garantie (Drittnennung) **[FIX-E]** A2. **Pattern (8):** Naming-Konventions-Fälle (`feedback_naming_convention_allocator_as_goldstandard`) **[DOC]** A4.

## §5 Widerlegte Befunde (17) — als korrekt-NICHT-bestätigt verbucht

**Mess (5):** 2× BuildVersion-Drift (Harness-Parameter = dokumentierte Praxis) · tier_scan-sort-Variante (für 320 gilt No-Op K4; sort nur Referenz → dort Pattern-P2, beide Audits konsistent) · Doc-33-§3-Split-Äquivalenz + escalate-Fehlerpfad (beide durch **CoW Rev.2 [GEGENSTANDSLOS]**). **Pattern (12):** refuted-Liste im JSON. ⇒ **Beleg der Verifikations-Qualität** (Meta #8): das System hat korrekt nicht-bestätigt; die CoW-Rev.2-gegenstandslosen Befunde bestätigen, dass die Verifizierer den AKTUELLEN Code lasen.

## §6 (D2) SOLL-Korrekturen für Doc 34 §9 — Architektur-Erweiterung

Die teuer erkauften Defekt-Einsichten als bleibende SOLL-Regeln (werden in Doc 34 §9 eingearbeitet):

1. **Apparat-Reinheit (K5/K6/P3–P8) = Teil der Mess-Echtheit:** Der Hot-Pfad jeder getimten Op darf KEINE
   Apparat-Kosten tragen (kein Doppel-Lookup, kein `std::function`-notify, kein O(n)-Rebuild-Container, keine
   Auto-Kopplungs-Buchführung). zero-cost-Metaprog-Direktive systematisch auf den Hot-Pfad (Meta #6). Soll-Ort:
   Befund-2/Q2-Schritt-4-Komplex (§9) — `container_`→LinearScan/Append, NullNotify-Policy, Policy-Allocator real.
2. **Capability nie still degradieren (K3, Meta #1/#2):** jede `if constexpr(capable)`-Pfadwahl braucht
   `static_assert` über die ZIEL-Population (die 320), nicht nur Referenz-Komp. = bindende Regel für ALLE
   requires-Kaskaden (A2a/K3 ist der Präzedenzfall: `4a64bc8`).
3. **Differenz-Beweise brauchen verschiedene Pfade (Meta #3) — MISSION-KRITISCH:** ein Achsen-Austauschbarkeits-
   Beleg ist nur gültig, wenn die verglichenen Lebewesen nachweislich VERSCHIEDENE Organ-Pfade durchlaufen
   (Diagnose-Flag im Output). Solange `search_organ_`-Monolith node/layout beschattet (Q2-Schritt-4 offen),
   sind Achsen-Diffs teils Apparat-Artefakt → die finalen Belege blockieren auf E-Welle-A2.
4. **Etiketten-Drift = Pattern-Direktive (K10, Meta #4):** Name/Pattern nur tragen, wenn die kanonische Semantik
   erfüllt ist (Adapter braucht Adaptee; Observer = one-to-many; Visitor = Double-Dispatch; „B+-Baum"-Benennung
   präzise). Sonst ehrlich umbenennen (grep-Beweis).
5. **env-/Parameter-Volatilität = Daten-Integritätsrisiko (K8, Meta #7):** Resume pinnt die GESAMTE Konfiguration
   (XML-Inhalts-Hash + env_limits + effektiver Seed) oder bricht ab — nie still mit Defaults weiterlaufen.
6. **Dokumentierte Semantik gehört in Tests, nicht Kommentare (Meta #5):** Kommentar-Realität-Divergenzen
   (`exakt 2×`, „kodiert ALLES", „warmup-frei") sind Major — als Test ausdrücken.

## §7 (D3) Audit-fundierte E-Aufgaben-Liste (Input für E1)

Reihenfolge + Soll-Ort exakt Goal §2.5.5; jede E-Welle = eine 1M-Session, gegen Doc 34 + die 85 Befunde verankert,
Achsen-Austausch IM Baum:

- **E-Welle-A1 (Resume/Stamp/Pipeline-Härtung, batch_for_m3):** K8-Rest (XML-Inhalts-Hash + env_limits + Seed +
  Resume-vor-b.ok) · XML-Validierung (op_mix/`<records>`) · CSV-Stream-Fehlercheck · Separator/ID-Validierung ·
  Doc-32↔21-XML (LP11-Sweep+LP02) · K7b Load-/Insert-Key-Räume.
- **🫀 E-Welle-A2 (Apparat-Reinheit + cowfix-v1, DLL-Neubau — DAS HERZSTÜCK):** **Befund-2/Q2-Schritt-4** (search_organ_
  entfällt, Such-Strategie ALS Traversal über DENSELBEN container_-Store; perm_runner→V2-POD node_*-Felder) ·
  K5(b) NullNotify-Policy · K5(c)/P4 container_→LinearScan/Append · K6/P6 echter Policy-Allocator (A::StdAllocatorAdapter
  + A::statistics()) · K3 CoW real für die 320 (restore_statistics-Aktivierung im Neubau) · K9 seg_ns-n>1 (CoW-Key-Ernte) ·
  K4/P2 GoF-Iterator-Scan-Organ · stat_*-Load-Reset · uint16→uint64-Entscheid. **Ohne diese Welle keine gültigen
  Austauschbarkeits-Belege** (Meta #3).
- **E-Welle-A3 (Dimension/Validität):** K1 RC-Organ-Hooks ODER RC-Dim raus (needs_user) · K7c Splitmix64-Scrambling ·
  K9 Konformitäts-Gate in den Voll-Lauf-Pfad (import→GATE→messen) · K9 SelectMode=search_algo_grid (Index-Selektion entkonfundieren).
- **E-Welle-A4 (Pattern-Hygiene, kein Mess-Einfluss):** K10 komplett (Adapter/Memento/Visitor/Command/Observer-Etiketten +
  „B+-Baum"-Benennung + Abstract-Factory + Release-Zero-Overhead-Rest) — je Etikett kanonisches Pattern ODER ehrlich
  umbenannt (grep-Beweis).
- **A5 (NUR Diskussion, nicht-blockierend):** Second-Execution vs Zwei-Phasen-Pflicht — Optionen dem User.
- **Phase-L (Auswertung) NACH M3:** Achsen-Austausch-Diffs IM Baum (inverse Signatur-Projektion KF-15 über reale
  Compositions, Doc 34 §3/§12), NICHT flach. Appendix listet jeden ungefixten Befund (RC nominal, ycsb_e/lp_range_scan
  invalide bis K4, Insert=Upsert bis K7b, stat_* enthält Load bis A2, prefetch-Pseudo-Adressen, layout sub-noise→PMC,
  Second-Execution) als ehrliche Daten-Limitierung (Goal §2.5-b).

## §8 D-FAZIT

**Alle 85 Befunde dispositioniert** (§1–§5): **[FIX-DONE]** K2, K5a, K7a, P3/P7, Stamp-Gate/env-Guard/Harness-Pinning (K8-Teil) ·
**[FIX-E]** der Apparat-Reinheits-Komplex (K3/K4/K5bcd/K6/K9 + container_/Notify/Allocator/Iterator) = E-Welle-A2-Herzstück ·
**[USER]** K1 (RC) + Second-Execution (A5) · **[DOC]** K10-Etiketten + Kommentar-Drift (A4) · **[GEGENSTANDSLOS]** die
CoW-Rev.2-erledigten Rev.1-Befunde + uint16-Replay-Aliasing · **[LIMIT]** jeder bis M3 ungefixte Befund im Appendix.
**Kein Befund verfällt still** (Goal §2.5-Done-Kriterium erfüllt: jeder in genau einem Zustand). **Dominant + mission-
kritisch: Befund-2/Q2-Schritt-4** (Doc 34 §9) — Meta-Lehre #3 macht die echte Such-Delegation zur Voraussetzung
gültiger Achsen-Austauschbarkeits-Belege. **Doc 34 §9 wird um §6 (D2-SOLL-Korrekturen) erweitert; die E-Auslegung (§7)
ist der Input für Phase E1.** NÄCHSTE: Doc 34 §9-Erweiterung committen → Phase E1 (Mission-Aufgaben-Liste).
