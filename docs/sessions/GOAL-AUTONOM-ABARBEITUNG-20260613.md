# GOAL 2026-06-13 — Autonome Abarbeitung: gesteuert durch den MASTERPLAN

> ## ⭐ NEU-FASSUNG (2026-06-13 spät, User-Direktive) — dieses /goal = den MASTERPLAN autonom abarbeiten
>
> **Dieses /goal steuert ab jetzt AUSSCHLIESSLICH über den Masterplan**
> `docs/sessions/20260613-MASTERPLAN-architektur-konsolidierung-und-aufraeumen.md` (Tasks #143–#150).
>
> **⭐ B4-UPDATE (2026-06-13 spät) — Phase A+B (B1–B4) ERLEDIGT:** Der konsolidierte IST-Architektur-Stand (Masterplan B1) liegt jetzt als
> **Single-Source `docs/architecture/34_KONSOLIDIERTER_MASTER_IST_STAND.md`** vor (§1–§12: 3-Ebenen/Organ/B+-Baum/4-Subsystem/3-Stufen/
> Mess-Modell/Observer-I1/Memento-CoW/Pflicht-Achsen-19/F15+Bias-Matrix/Befund-2-Mess-Echtheit/SUPERSEDED-Auflösung/Mission-Verankerung).
> **Jede künftige K/L/E-Aufgabe liest Doc 34 (statt der 48 Quell-Docs) + prüft gegen die 85 Audit-Befunde (`20260613-A3-audit-soll-abgleich.md`).**
> Die A0–A3-Lese-/Code-/Audit-Destillate (`20260613-A1-lesenotizen…` / `…-A2-code-pre-read…` / `…-A3-audit-soll-abgleich`) sind die
> agent-verifizierten Belege darunter. **VERBLEIBEND:** Masterplan C (Aufräumen) → D (85 Befunde Befund-für-Befund) → E (Mission, 1 Aufgabe/Session).
> Das /goal zu setzen bedeutet: die Masterplan-Phasen **A→B→B4→C→D→E→F in STRIKTER Reihenfolge** autonom
> abarbeiten —
> **A** (ALLE Architektur-Docs ab Doc 00 lesen + Pflicht-Code-Pre-Read + Audit-Überblick) →
> **B** (EIN konsolidiertes Master-Architektur-Doc, Single-Source) →
> **B4** (dieses Goal-Doc gründlich re-grounden) →
> **C** (falsch umgesetzte Session-Änderungen aufräumen: L1/L2 revert, A2a/K3 re-eval) →
> **D** (die 85 teuren Audit-Befunde vollständig durcharbeiten) →
> **E** (= das ursprüngliche (b): Original-Mission, EINE Aufgabe pro 1M-Session) →
> **F** (Persistenz/Disziplin). **NICHT mit Code/(b)=E beginnen vor A+B+C+D.** Phase A startet in frischer
> Session. **M2-Mess-Lauf ist PAUSIERT** (cowmem-v1, un-gefixte Mess-Architektur; wird ohnehin von cowfix-v1 abgelöst).
>
> **Was unten noch GILT (Referenz):** die Autonomie-Leitplanken **§0** (Zwei-Phasen-Pflicht, Pattern-Direktive,
> relative Pfade, 3-Repo-Disziplin, destruktive Ops in den 3 Repos mit Tag+Commit+Push) UND der **Audit-Backlog
> §2.5 + das Rohdaten-Manifest §2.5.6** (Phase D arbeitet sie Befund für Befund durch). **ÜBERHOLT/FALSCH:** die
> alte Ausführungs-Reihenfolge **§2** (M/L-Pipeline, L1–L8) — die L-Flach-Tupel-Auswertung war ein
> Architektur-Fehler; der korrekte Ort des Achsen-Austauschs ist der **B+-Experiment-Baum** (Masterplan +
> `20260613-ARCHITEKTUR-KORREKTUR-achsenaustausch-im-baum.md`).

> **Auftrag (User, 2026-06-13, verbatim-sinngemäß):** „Vielleicht lassen wir den Voll-Lauf auch
> einfach laufen, aber machen gleich mit den übrigen Aufgaben autonom weiter. Bitte formuliere ein
> goal, um alle in der letzten Session definierten Aufgaben und Audits autonom abzuarbeiten."
>
> **Kippschalter gegenüber dem Vorgänger-Goal:** `GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md` sagte
> „Steuerung erfolgt MANUELL durch den User". **Dieses Goal hebt das auf: AUTONOME Ausführung** —
> nicht fragen, fortfahren, je Welle committen+pushen+Submodul-Bump. Die TODO-Substanz (M1-M3, L1-L8,
> A1-A5, Gates G1-G4) bleibt im Vorgänger-Goal autoritativ; dieses Dokument ist der **Ausführungs-
> Charter** darüber (Reihenfolge, Autonomie-Grenzen, M2-Sicherheits-Regeln, Stop-Bedingungen).

## §0 Autonomie-Mandat & Leitplanken (bindend)

1. **Autonom fortfahren ohne Rückfrage** für alles, was aus dem Auftrag folgt und reversibel ist.
   Destruktive Ops NUR in den 3 Thesis-Repos (Diplomarbeit / cache-engine / prt-art) erlaubt, sofern
   **Tag + Commit + Push** (Memory `feedback_destructive_autonomy_3repos_with_tag`). Infra/andere Repos
   bleiben bestätigungspflichtig.
2. **Zwei-Phasen-Cache-Warmup bleibt PFLICHT** (Mess-Gültigkeit). Der „Second-Execution"-Audit-Einwand
   wird NUR dokumentiert/dem User vorgelegt (A5), NIE stillschweigend umgesetzt.
3. **Pattern-Direktive** (`feedback_lehrbuch_design_patterns_only_zero_cost_metaprog`): neue Strukturen
   nur als benannte Lehrbuch-/erweiterte Patterns + Benennungskonvention (web-verifiziert); Metaprog nur
   zero-cost. Gilt für JEDEN Code dieses Goals (v.a. Welle 4 + Phase-L-Tools).
4. **Relative Pfade** in allen Thesis-/LaTeX-/Skript-Referenzen (git-clone-fest); **TU-Dresden-/ZIH-
   diplominf-Vorlage** (`zihpub.cls`) unangetastet; **EN≡DE-Äquivalenz** der Thesis-Builds.
5. **Keine Erfolgsmarke ohne literale Tool-Ausgabe** (`feedback_no_success_marks_without_literal_output`);
   je Welle Commit+Push + 3-Repo-Submodul-Sync. Roh-CSVs → lokale benannte Kopie + robuste NAS-Ablage;
   in git nur Aggregate/Appendix/PDF.
6. **Audit-Lehren anwenden** (`audit-sicherung-20260612/ERKENNTNISSE.md` §7): Capability-Pfade per
   static_assert über die ZIEL-Population (die 320) absichern, nicht nur Referenz-Kompositionen;
   Konfiguration pinnen statt stiller Defaults; Diff-Beweise nur mit nachweislich verschiedenen Pfaden.

## §1 M2-Sicherheits-Regeln (während der Voll-Lauf läuft)

Der M2-Resume-Lauf (`cowmem-v1`-DLLs, Hintergrund) misst über die **bereits kompilierten** 320 .dll auf
Disk. Daraus folgt für die parallele autonome Arbeit:

- **ERLAUBT während M2** (berührt M2-Binärdateien NICHT): Quell-Edits an `libs/cache_engine/**`
  (Header, gegen die die DLLs einst kompilierten — die fertigen .dll ändern sich dadurch nicht) +
  Unit-Test-Bau/-Lauf in **separaten** Build-Bäumen (`build/msvc-release`, nicht `build/thesis_tiere`);
  die komplette Phase-L-Tool-/Thesis-Arbeit (`Code/04..06`, `thesis/diplomarbeit/**`).
- **VERBOTEN während M2** (würde den laufenden Lauf/Resume korrumpieren): Stamp-Format-/CSV-Schema-/
  Harness-Matrix-Änderungen (invalidieren die 63 fertigen Stamps → Re-Measure-Sturm); ein zweiter
  Mess-Lauf in `build/thesis_tiere`; Neubau der 320 perm-DLLs.
- **Konsequenz:** Welle-1 *Harness/Stamp/CSV*-Punkte werden **gebatcht für den M3-Neubau** (cowfix-v1),
  NICHT mid-M2 eingespielt. Welle-2 *libs*-Edits laufen sicher parallel (verifiziert über Unit-Tests).
- M2 ist **Entwicklungs-Substrat + Sicherheitsnetz**, nicht die Abgabe-Daten. Stirbt M2 (Reboot o.ä.),
  genügt derselbe Harness-Befehl (Resume) — kein Datenverlust (per-Binary-Stamps). **Die Abgabe-Daten
  kommen aus M3 (cowfix-v1).**

## §2 Autonome Ausführungs-Reihenfolge (mit Begründung)

```
[läuft] M2-Resume (cowmem-v1)         → Interim-Matrix + L-Entwicklungsdaten + Netz   (tokenfrei, Hintergrund)
  ║ parallel, token-gebunden, autonom:
  ├─ Phase A (Audit-Wellen → cowfix-v1) = GATE ZU GÜLTIGEN DATEN     [Schwerpunkt: libs + Tests]
  │    A1 Welle1-Rest (mess-kritisch; Harness/Stamp-Teile gebatcht für M3)
  │    A2 Welle2 Apparat-Reinheit + CoW-real-für-die-320 (restore_statistics → 4 Pilot-Wrapper +
  │       static_assert über echte Pilot-Komposition) + echter Policy-Allocator + Iterator-Scan
  │    A3 Welle3 Dimension/Validität (RC nominal bis User-Entscheid; Scrambling; Konformitäts-Gate)
  │    A4 Welle4 Pattern-Hygiene (Etiketten/Naming gemäß Pattern-Direktive; kein Mess-Einfluss)
  └─ Phase L (Auswertungs-Pipeline) = baut Tools gegen Interim-Schema, läuft final gegen M3
       L1✓(Spalten da) · L2 3D-Surfaces je Interface-Fn · L3 Achsen-Austauschbarkeits-longtables
       L4 Appendix (ALLE Werte + ehrliche Limitierungs-Tabelle) · L5 thesis-Integration (relativ, EN≡DE)
       L6 Generalprobe (Teil-CSV → Test-PDF)
↓ nach Phase A komplett + M2 fertig/abgelöst:
M3 finaler Voll-Lauf (cowfix-v1, alle 21 Profile valide) → ersetzt M2-Daten
↓
L7/L8 Finale: EIN Experiment-Kommando → Aggregat → Appendix → build.ps1 beide Sprachen → fertige PDF
              + NAS-Ablage der Roh-Matrix
```

**Warum Phase A VOR der finalen Auswertung:** Die Achsen-Austauschbarkeits-Belege (Professor-Kern)
brauchen Daten, in denen Achsen-Unterschiede ECHT sind, nicht Apparat-Artefakt. cowmem-v1 trägt K1
(RC ×18→×3 degeneriert), K3 (CoW-Capability tot → Memento-Pfad für alle 320 identisch), K5 (Apparat-
dominierte Wall-Clock). Darum: Tools gegen das Interim-Schema bauen (richtige Form), aber die
**interpretierbaren Endbelege aus cowfix-v1 (M3)** ziehen. Das Interim dient L-Entwicklung + Generalprobe.

## §2.5 AUDIT-ZIELE — verbindlicher Pflicht-Backlog der zwei teuren Audits

> **Provenienz (User-Direktive 2026-06-13: „nimm die Ziele des Design-Audits und Voll-Audits mit ins Goal"):**
> Zwei Multi-Agent-Audit-Workflows, ~116 Agenten, >4,3 Mio Subagent-Tokens, **>300 € Token-Kosten** —
> beide zweistufig (unabhängige Prüf-Linsen → adversariale Widerlegungs-Verifikation je Befund):
> - **Mess-Architektur-/Voll-Audit** (`wf_a013b73f-aea`, 71 Agenten, 9 Linsen): **57 bestätigt** (24 blocker /
>   24 major / 9 minor), 5 widerlegt. Volltexte: `docs/sessions/20260612-messaudit-endergebnis.json`.
> - **Design-/Pattern-Audit** (`wf_86936298-e41`, 45 Agenten, 4 Linsen, web-verifizierte Pattern-Definitionen):
>   **28 bestätigt** (8 blocker / 12 major / 8 minor), 12 widerlegt. Volltexte:
>   `docs/sessions/20260611-patternaudit-ergebnis.json`.
> - **Summe: 85 bestätigt** (32 blocker / 36 major / 17 minor) + 17 saubere Widerlegungen.
>   Konsolidierung: `audit-sicherung-20260612/ERKENNTNISSE.md`; Fix-Plan: `20260611-audit-ergebnisse-synthese.md` §4.
>
> **Done-Kriterium für JEDEN der 85 Befunde (eines von drei):** (a) GEFIXT mit literaler Tool-Ausgabe, ODER
> (b) bewusst als **Daten-Limitierung im Appendix** ehrlich ausgewiesen (z.B. Second-Execution-Effekt), ODER
> (c) **User-Entscheid pending**, markiert + nicht-blockierend mit Default-Verhalten. Kein Befund verfällt still.

### §2.5.1 Die 10 Kern-Defekte (K1-K10) → Welle → Done-Kriterium

| K | Defekt (Audit-Kurzform) | Welle | Done-Kriterium (literal) | Status |
|---|---|---|---|---|
| **K1** | RC = write-only Null Object: `applied_rc_` nie gelesen, KF-5-§7-A-Organ-API nie gebaut → ×18-Dynamik degeneriert zu ×3 (nur repetition), RC-Spalten = Rauschen | A3 | RC-Organ-Hooks wirken real (concurrency/prefetch ergeben messbar verschiedene Werte) ODER RC-Dims entfernt + caps=0; bis dahin RC **nominal** ausgewiesen | ⏳ needs_user |
| **K2** | ns_per_op systematisch HALBIERT (Divisor `2*n_ops` aus Legacy-Workload; Workload-Pfad liefert n_ops Ops) | A1 | Smoke: `ns_per_op == total_ns/timed_ops` | ✅ M1.1 (timed_ops) |
| **K3** | Memento/CoW-Capability **compile-time TOT für alle 320**: die produktiven `search_algo`-Wrapper haben kein `restore_statistics` → stiller copymem-Rückfall; 42/42-Test prüfte nur Referenz-Kompositionen (Test-Pop ≠ Ziel-Pop) | A2 | `restore_statistics` in die produktiven search_algo-Wrapper + **`static_assert` über ≥1 echte Pilot-AdHoc-Komposition**; `tier_memento_is_copy_on_write()==true` für ein Pilot-Tier; Build = `cowfix-v1` | ⏳ offen (Kern) |
| **K4** | `tier_scan` No-Op für die 320 (kein MementoAxis) / Vollkopie+sort für Referenz → ycsb_e+lp_range_scan messen Aufruf-Latenz | A2 | GoF-**Iterator** als Organ-API; 1-Tier-Smoke `scan>0`; Hash-Organe ehrlich nicht-scanbar | ⏳ offen |
| **K5** | Mess-Apparat dominiert Wall-Clock: (a) verdeckter Doppel-Lookup (is_new), (b) `std::function`-notify ohne Subscriber, (c) `container_` O(n)-flatten+rebuild je Op, (d) T1/T2-Buchführung | A2 | (a) is_new via `occupied_count()`-Delta; (b) compile-time NotifyPolicy (NullNotify, zero-cost); (c) `container_`→LinearScan/Append; Wall-Clock-Abfall belegt | ⏳ offen |
| **K6** | Phantom-Allocator: Stores constrainen Policy A, allozieren via `std::allocator` + FABRIZIEREN `allocator_statistics()` → T6-Achse misst A nie | A2 | Chunk-Speicher real über A (`A::StdAllocatorAdapter`) + `A::statistics()` durchgereicht; alloc-Bytes layout-/policy-abhängig | ⏳ offen |
| **K7** | Workload-Treue: (a) coco_neg50=zipfian (Sweep konfundiert), (b) Run-Phase=nur-Upserts, (c) Zipfian/Latest ohne Key-Scrambling | A1+A3 | (a) 5 Sweep-XMLs identische Verteilung; (b) Load-/Insert-Key-Räume getrennt; (c) Splitmix64-Scrambling, Histogramm gestreut | ✅(a) M1.2 · ⏳(b,c) |
| **K8** | Resume-/Re-Entry-Härte: Stamp ohne XML-INHALT (nur id-Liste; seed wirkungslos; env_limits fehlen); Stamp trotz Write-Fehler; gated Vollständigkeit≠Gültigkeit; Resume-Check nach b.ok(); Harness pinnt env/Params nicht | A1 | Stamp trägt XML-Inhalts-Hash + env_limits + effektiven Seed; pf.good()-Gate + two_phase-Gate; Resume-Check vor b.ok(); Harness-Pinning. Negativ-Proben literal | ✅ Stamp-Gate/env-Guard/Harness-Pinning (M1.3/1.4) · ⏳ XML-Hash/env_limits/Resume-vor-b.ok (**batch_for_m3**) |
| **K9** | Validitäts-Pfade: Konformitäts-Gate nie gerufen; seg_ns n=1 für 320; prefetch misst Key-Werte als Pseudo-Adressen; uint16-Keys trunkieren uint64 (records>32767 bricht Negativ-Garantie); Index-Selektion konfundiert search_algo | A2+A3 | Gate im Voll-Lauf-Pfad (import→GATE→messen); seg_ns n>1 via CoW-Key-Ernte; prefetch ehrlich ausgewiesen; uint16→uint64-Entscheid; SelectMode=search_algo_grid für M3 | ⏳ offen |
| **K10** | Pattern-/Etiketten-Integrität (12 Major): Adapter ohne Adaptee; Memento-Etikett gebrochen + tote MementoAggregate; „Hybrider Visitor" existiert nicht; Command tote Parallelstruktur; MeasurableObserver≠Observer; „B+-Baum"≠B+-Baum; degenerierte Abstract Factory; Release-DLL zahlt Mess-Kopplungen | A4 | Je Etikett: kanonisches Pattern umgesetzt ODER ehrlich umbenannt/entfernt (grep-Beweis); Release-DLL zero-overhead (Mess-Kopplungen unter `COMDARE_MEASUREMENT_ON`) | ⏳ offen |

### §2.5.2 Vollständige Major-Liste (36 = 24 Mess + 12 Pattern) → Welle

**Mess (24, in K1-K9 verdichtet + einzeln):** Stamp-trotz-Write-Fehler (A1✅) · Stempel-ohne-XML-Inhalt
(A1, batch_m3) · Resume-Check-nach-b.ok() (A1) · XML-`<records>`-geparst-aber-verworfen (A1) · XML-ohne-
Validierung (op_mix-Pflicht; A1) · stat_*-Spalten enthalten Load-Phase (A2: Observer-Reset nach Load) ·
fill_segment_timing-T0-Stats aufgebläht (A2) · Doc-33-§2-„exakt-2×"-Zusicherung falsch für Schwellen-Stats
(A4-Doku) · keine Separator-/ID-Validierung der Wire-/CSV-Kette (A1) · globale CSV ohne Stream-Fehlerprüfung
(A1) · Doc-32-Katalog ≠ 21 XMLs (LP11-Sweep fehlt, LP02; A1) · **Second-Execution-Grundsatzeinwand** (A5 —
NUR Diskussion, Zwei-Phasen ist Pflicht) · uint16-Keys (A2) · Index-Selektion konfundiert (A3).
**Pattern (12, alle A4):** Adapter-ohne-Adaptee · Memento-Etikett ×3 (memento_all/MementoAggregate/save_axis)
· Visitor-existiert-nicht ×3-Fund · Release-DLL-Mess-Kopplung · „B+-Baum"-Benennung · degenerierte Abstract
Factory (bool-Flag) · Command-tote-Parallelstruktur · MeasurableObserver≠Observer ×2.

### §2.5.3 Minor (17) — Sammelabarbeitung in der jeweiligen Welle

env_limits-im-Stempel/requested-vs-applied · Vollständigkeits-Gate zählt besuchte statt Matrix-Größe ·
rb_exact-Probe zertifiziert zu wenig · 2× Kommentar-Drift (two_phase-Vertrag / IRollbackableTier-Header) ·
clock::now()-Overhead additiv · Load-Phase in stat_*-Spalten · uint16-Aliasing-Reste (Replay ersetzt) · OOM-
Degradationspfade unmarkiert · 8 Pattern-Minor (Naming-Konventions-Fälle, im Pattern-JSON). Done: je Welle
mit-erledigt ODER als Limitierung ausgewiesen.

### §2.5.4 Die 8 Meta-Lehren des Audits (DAUERHAFT anwenden — die teuersten Erkenntnisse)

1. **Capability-Detection darf nie still degradieren** — jede `if constexpr(capable)`-Pfadwahl braucht
   `static_assert` über die ZIEL-Population (K3-Lehre; gilt für A2a + alle künftigen requires-Kaskaden).
2. **Test-Population ≠ Ziel-Population** — Verifikation muss ≥1 echten Vertreter jeder Ziel-Klasse (die 320
   Pilot-Wrapper) instanziieren, nicht nur Referenz-Kompositionen.
3. **Differenz-Beweise können strukturell blind sein** — ein Äquivalenz-/Diff-Beweis braucht den Nachweis
   VERSCHIEDENER Pfade (Diagnose-Flag im Output). Gilt direkt für die Phase-L-Austauschbarkeits-Belege!
4. **Etiketten-Drift ist messbar gefährlich** = die User-Pattern-Direktive: Name/Pattern nur tragen, wenn die
   kanonische Semantik erfüllt ist (`applied_axis_count>0` wurde als Erfolg gefeiert, ohne dass etwas wirkte).
5. **Dokumentierte Semantik gehört in Tests, nicht in Kommentare** (mehrere Majors = Kommentar-Realität-Drift).
6. **Mess-Apparat-Reinheit ist eine eigene Disziplin** — zero-cost-Metaprog-Direktive systematisch auf den
   Hot-Pfad (K5).
7. **env-/Parameter-Volatilität ist ein Daten-Integritätsrisiko** — Resume pinnt die GESAMTE Konfiguration
   oder bricht ab (K8).
8. **Adversariale Mehrfach-Linsen funktionieren** — der Audit-Aufbau (Skripte resume-fähig) ist
   wiederverwendbar; bei Unsicherheit erneut anwenden statt raten.

### §2.5.5 Welle→Befund-Abarbeitungs-Matrix (= Phase A, autonom)

- **Welle 1 (A1, billig + mess-kritisch):** K2✅ · K6/coco✅(K7a) · Stamp-Gate✅ · Harness-Pinning✅ · env-Guard✅
  | OFFEN (batch_for_m3): Stamp-XML-Inhalts-Hash+env_limits+Seed · Resume-Check-vor-b.ok() · XML-Validierung
  (op_mix/`<records>`) · CSV-Stream-Fehlercheck · Separator-/ID-Validierung · Doc-32↔21-XML-Abgleich · K5/M5
  Load-/Insert-Key-Räume.
- **Welle 2 (A2, Apparat-Reinheit + cowfix-v1, DLL-Neubau):** K3 (restore_statistics→Pilot-Wrapper +
  static_assert) · K4 (Iterator-Scan) · K5 (is_new/NotifyPolicy/container_-LinearScan) · K6 (echter
  Policy-Allocator) · stat_*-Load-Reset · seg_ns-n>1 · uint16→uint64.
- **Welle 3 (A3, Dimension/Validität):** K1 (RC-Hooks-oder-raus, needs_user) · K7c (Scrambling) · K9
  (Konformitäts-Gate, SelectMode=search_algo_grid) · K7b (Insert-Key-Räume).
- **Welle 4 (A4, Pattern-Hygiene, kein Mess-Einfluss):** K10 komplett (Adapter/Memento/Visitor/Command/
  Observer-Etiketten + „B+-Baum" + Abstract-Factory + Release-Zero-Overhead).
- **A5 (NUR Diskussion, nicht-blockierend):** Second-Execution vs Zwei-Phasen-Pflicht — Optionen dem User.

**Verknüpfung Phase L:** Die Limitierungs-Tabelle des Appendix (L4) listet JEDEN noch nicht gefixten Befund
ehrlich als Daten-Vorbehalt (RC nominal, ycsb_e/lp_range_scan invalide, Insert=Upsert, stat_* enthält Load,
cowmem=Copy-Pfad, Second-Execution) — so erfüllt selbst ein Interim-Stand das §2.5-Done-Kriterium (b).

### §2.5.6 Gesicherte Audit-Rohdaten — vollständiges Manifest (alles git-getrackt, in jedem Clone da)

> Die >300-€-Audit-Investition ist **dauerhaft im Repo gesichert** (die `.claude`-Workflow-/Temp-Verzeichnisse
> sind flüchtig — diese Dateien sind die einzige belastbare Quelle). Pfade repo-relativ ab cache-engine-Root.

| Datei | Größe | Inhalt | Audit |
|---|---|---|---|
| `docs/sessions/audit-sicherung-20260612/messaudit-workflow-output.json` | 316 KB | **ROHESTE** Quelle: kompletter Mess-Audit-Workflow-Output (Summary + Logs + konsolidiertes Result, alle 57+5 Volltexte) | Mess `wf_a013b73f-aea` |
| `docs/sessions/audit-sicherung-20260612/patternaudit-workflow-output.json` | 195 KB | **ROHESTE** Quelle: kompletter Pattern-Audit-Workflow-Output (28+12 Volltexte + Web-Referenzen) | Pattern `wf_86936298-e41` |
| `docs/sessions/audit-sicherung-20260612/ERKENNTNISSE.md` | 15 KB | Konsolidierte Erkenntnis-Doku (K1-K10, Major/Minor-Listen, 8 Meta-Lehren, Manifest) | beide |
| `docs/sessions/20260612-messaudit-endergebnis.json` | 314 KB | Konsolidiertes Mess-Endergebnis (57 Befunde, je claim/evidence/consequence/fix + Verifizierer-reasoning) | Mess |
| `docs/sessions/20260611-patternaudit-ergebnis.json` | 194 KB | Konsolidiertes Pattern-Ergebnis (28 Befunde + 12 Widerlegungen + Web-Belege) | Pattern |
| `docs/sessions/20260611-messaudit-rohdaten.json` | 348 KB | Zwischenstand-Beleg (9 Finder = 62 Roh-Befunde + 73 Einzel-Verdicts vor End-Konsolidierung) | Mess |
| `docs/sessions/20260611-audit-ergebnisse-synthese.md` | 14 KB | Synthese beider Audits + 4-Wellen-Fix-Plan §4 + End-Verdikt-Kopf (Triage-Einstieg) | beide |

**Lese-Reihenfolge zum Wiederfinden:** Einstieg = `ERKENNTNISSE.md` (Überblick) → `20260611-audit-ergebnisse-synthese.md`
(Fix-Plan) → für Volltext eines Einzelbefunds die JSON-Endergebnisse (Mess: `20260612-messaudit-endergebnis.json`,
Pattern: `20260611-patternaudit-ergebnis.json`); die `…-workflow-output.json` + `…-rohdaten.json` sind die
roheste Beweis-Ebene (Logs/Zwischenstände). Gesamtumfang Rohdaten ≈ 1,37 MB, vollständig versioniert.

## §3 Stop-Bedingungen (NUR hier den User einbeziehen — sonst autonom)

- **A3-RC-Entscheid** (RC-Organ-Hooks bauen ODER RC-Dimension ehrlich entfernen): bis zur User-Antwort
  wird RC in der Auswertung als **nominal** ausgewiesen und fortgefahren — **kein Blockieren**.
- **A5 Zwei-Phasen-„Second-Execution"-Grundsatzfrage**: NUR Optionen dokumentieren, Entscheidung User —
  **kein Blockieren** (Zwei-Phasen bleibt unverändert PFLICHT bis User anders entscheidet).
- **Echte Unklarheit / fehlende Stand-Technik-Doku** (`feedback_never_guess_always_lookup…`): Lookup +
  bei Unsicherheit teure Planungssession gegen Architektur/Doku/Ist VOR dem Bauen — dann autonom weiter.
- Alles andere: **fortfahren, nicht fragen.**

## §4 Definition of Done (Abschluss-Gates, aus Vorgänger-Goal)

- **G1:** Phase-L-Pipeline produziert aus einer Matrix-CSV reproduzierbar Appendix + bilinguale PDF
  (gegen Interim verifiziert = Generalprobe ✓).
- **G2:** Audit-Wellen 1-3 literal verifiziert; **cowfix-v1**-DLLs gebaut; M3-Matrix komplett
  (~120.960 Zeilen, 0 Fehlmessungen); NAS-Ablage ✓.
- **G3:** L8-E2E ✓ — EIN Experiment-Aufruf → fertige bilinguale Diplomarbeit-PDF mit vollständigem
  Mess-Appendix aus der finalen Matrix; frischer git-clone baut identisch (relative Pfade); ZIH-Vorlage
  unverändert; Achsen-Austauschbarkeits-Belege interpretierbar (3D-Surfaces + Diff-longtables).
- **G4:** Welle 4 Pattern-Hygiene abgeschlossen; Session-Doku + Memory + 3-Repo-Sync nach jeder Phase.
- **G5 (Audit-Backlog, §2.5):** ALLE 85 Audit-Befunde (K1-K10 + 36 Major + 17 Minor) stehen in genau einem
  Zustand: (a) gefixt mit literalem Beleg, (b) als Daten-Limitierung im Appendix ausgewiesen, oder (c)
  User-Entscheid pending (markiert). Ein abschließender Audit-Abgleich (Befund-Liste ⇄ Code-Ist) bestätigt:
  kein Befund still offen. Die 8 Meta-Lehren (§2.5.4) sind auf jeden neuen Fix angewandt.

## §5 Referenz-Dokumente (autoritativ)

| Zweck | Pfad (repo-relativ ab cache-engine) |
|-------|-------------------------------------|
| TODO-Substanz M/L/A + Gates | `docs/sessions/GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md` |
| Audit-Wissen (K1-K10, 4 Wellen, Lehren) | `docs/sessions/audit-sicherung-20260612/ERKENNTNISSE.md` |
| Audit-Synthese + Fix-Plan §4 | `docs/sessions/20260611-audit-ergebnisse-synthese.md` |
| Mess-Audit Volltexte (57 Befunde) | `docs/sessions/20260612-messaudit-endergebnis.json` |
| Pattern-Audit Volltexte (28 Befunde) | `docs/sessions/20260611-patternaudit-ergebnis.json` |
| **Gesicherte Audit-Rohdaten (vollständiges Manifest, 6 Dateien, ~1,37 MB)** | **§2.5.6 dieses Dokuments** + Verzeichnis `docs/sessions/audit-sicherung-20260612/` |
| Memento Rev.2 (CoW) + Resume | `docs/architecture/33_undolog_memento_und_mess_resume.md` |
| Letzte Übergabe (Einstieg) | `docs/sessions/20260612-session-uebergabe-m2-laeuft-phase-l.md` |
| Bestehender Appendix-Orchestrator | `thesis/diplomarbeit/generate_measurement_appendix.ps1` |
| Mess-Resume / NAS | `scripts/collect_partial_results.ps1` · `scripts/copy_results_to_nas.sh` |

## §6 Sitzungs-Wiedereinstieg (jede neue Session)

1. M2-Status prüfen: `pwsh scripts/collect_partial_results.ps1` (Stamp-Zahl); falls Lauf tot →
   Resume-Befehl erneut (siehe Harness-Aufruf, `-Resume $true`, gleiche Parameter).
2. Offene Welle/L-Stufe aus diesem Goal + Tasks (#142-Cluster) fortsetzen.
3. Je abgeschlossener Einheit: literal verifizieren → committen+pushen → Submodul-Bump → Memory/Doku.
