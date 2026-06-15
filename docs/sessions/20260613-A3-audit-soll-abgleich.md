# A3 — 85 Audit-Befunde ⇄ Architektur-Soll-Abgleich (Masterplan-Phase A3)

> **Zweck (Masterplan A3):** Jeden Audit-Kern-Defekt einem **Architektur-Konzept** (3-Ebenen / Organ / B+-Baum / Mess-Modell /
> Observer / Memento) zuordnen + den **E-Aufgaben-Bezug** ableiten — Grundlage der Konsolidierung B + der E-Auslegung.
> **Quellen:** Goal §2.5 (K1–K10 + 36 Major + 17 Minor + 8 Meta-Lehren, in Kontext gelesen) · die zwei Audit-JSONs (Volltexte,
> Manifest Goal §2.5.6) · A1-Notizen (Architektur-IST) · A2-Notizen (Code-IST). **Done-Kriterium je Befund (Goal §2.5):**
> (a) gefixt mit literalem Beleg ODER (b) Daten-Limitierung im Appendix ODER (c) User-Entscheid pending. **Phase D arbeitet die
> 85 dann Befund-für-Befund gegen die JSONs durch; A3 = die Architektur-Verankerung + E-Auslegungs-Saat.**

## §1 K1–K10 Kern-Defekte → Architektur-Konzept → IST-Status → E-Aufgabe

| K | Defekt (Audit) | Architektur-Konzept (A1/A2-Verankerung) | IST-Status (Code, A2-verifiziert) | E-Aufgabe / Done-Weg |
|---|---|---|---|---|
| **K1** | RC = write-only Null-Object: `applied_rc_` nie gelesen, KF-5-§7-Organ-API nie gebaut → ×18-Dynamik degeneriert zu ×3 | **LASTENPROFIL (Doc 24 §2 / v5_drei_profile)** + `Algorithm_Resource_Control` (KF-4, dyn. Baum-Ebene Doc 26 §2) | RC-Dim wirkt nicht (Audit) | **E (needs_user):** RC-Organ-Hooks real (concurrency/prefetch messbar verschieden) ODER RC-Dim entfernen + caps=0; bis dahin RC **nominal** im Appendix (b). Stop-Bedingung Goal §3 |
| **K2** | ns_per_op systematisch HALBIERT (Divisor `2*n_ops`) | **Lebewesen-Wall-Clock (Doc 24 §2.1)** / perm_runner | ✅ **GEFIXT** (M1.1 timed_ops) | erledigt (a) |
| **K3** | Memento/CoW compile-time TOT für 320 (produktive search_algo-Wrapper ohne `restore_statistics` → stiller copymem); 42/42-Test prüfte nur Referenz-Komp. (Test-Pop≠Ziel-Pop) | **Memento Rev.2 CoW (Doc 33)** + **A2.1 abi_adapter** `restore_statistics`/Fallback-Kaskade; Meta-Lehre #2 | restore_statistics in 17 Wrappern (A2a/K3, ce 4a64bc8, in C re-eval); Adapter-CoW real (A2.1 :1158) | **E (Kern, cowfix-v1):** `restore_statistics` in die PRODUKTIVEN search_algo-Wrapper + **`static_assert` über ≥1 echte Pilot-AdHoc-Komposition** (nicht nur Referenz); `tier_memento_is_copy_on_write()==true` Pilot-Lebewesen |
| **K4** | `tier_scan` No-Op für 320 / Vollkopie+sort für Referenz → ycsb_e/lp_range_scan messen Aufruf-Latenz | **IScannableTier / scan-Achse** (Ledger §a.V5 #49-E/F, ABI-Minor); GoF-**Iterator**-Organ | IScannableTier additiv da (Ledger); tier_scan über save_state-Snapshot (A2.1) | **E:** GoF-Iterator als Organ-API; 1-Lebewesen-Smoke `scan>0`; Hash-Organe ehrlich nicht-scanbar |
| **K5** | Mess-Apparat dominiert WC: (a) verdeckter Doppel-Lookup (is_new), (b) `std::function`-notify ohne Subscriber, (c) `container_` O(n)-flatten/Op, (d) T1/T2-Buchführung | **Befund 2 (Doc 30 §6/§8)** = zwei-Speicher + Apparat-Reinheit (Meta-Lehre #6, zero-cost-Metaprog Hot-Pfad) | **(a) ✅ GEFIXT am Code** (A2.1 :644/:677 occupied_count-Delta); (b)/(c)/(d) ⏳ | **E:** (b) compile-time NotifyPolicy (NullNotify, zero-cost); (c) container_→LinearScan/Append; WC-Abfall belegen. **Koppelt an Q2-Schritt-4** (A2-Fazit) |
| **K6** | Phantom-Allocator: Stores constrainen Policy A, allozieren via std::allocator + FABRIZIEREN `allocator_statistics()` → T6 misst A nie | **Befund 2 / allocator-Achse (Doc 30)** + node_type/layout-Delegation | container_=NodeChunkedStore → node-abhängig (A2.1 :879 Q2-1-3); T6 aus Datenzustand DERIVIERT (Doc 33 §1) — aber Such weiter Monolith | **E:** Chunk-Speicher real über A (`A::StdAllocatorAdapter`) + `A::statistics()` durchgereicht; alloc-Bytes layout/policy-abhängig. **= Q2-Schritt-4-Komplex** |
| **K7** | Workload-Treue: (a) coco_neg50=zipfian, (b) Run-Phase nur-Upserts, (c) Zipfian/Latest ohne Key-Scrambling | **Lastprofil-Katalog/Paper-Bias (Doc 32)** + Reproduzierbarkeit (v5_drei_profile, xorshift64) | (a) ✅ M1.2; (b)/(c) ⏳ | **E:** (a) 5 Sweep-XMLs identische Verteilung ✅; (b) Load-/Insert-Key-Räume getrennt; (c) Splitmix64-Scrambling, Histogramm gestreut |
| **K8** | Resume-Härte: Stamp ohne XML-Inhalt (seed wirkungslos, env_limits fehlen); Stamp trotz Write-Fehler; gated≠gültig; Resume-Check nach b.ok() | **Mess-Resume Config-Stamp (Doc 33 §5)** + Meta-Lehre #7 (env-Volatilität=Daten-Integritätsrisiko) | **✅ teilw.** (Stamp-Gate/env-Guard/Harness-Pinning M1.3/1.4; A2.6 lazy_try_resume+result.csv.stamp präsent); ⏳ XML-Hash/env_limits/Resume-vor-b.ok (**batch_for_m3**) | **E (A1-Welle):** Stamp trägt XML-Inhalts-Hash + env_limits + effektiven Seed; pf.good()-Gate; Resume-Check vor b.ok(); Negativ-Proben literal |
| **K9** | Validität: Konformitäts-Gate nie gerufen; seg_ns n=1; prefetch misst Key-Werte als Pseudo-Adressen; uint16 trunkiert uint64; Index-Selektion konfundiert search_algo | **Konformitäts-Gate import→GATE→messen (v5_design §6)** + seg_ns Pfad-B (Doc 31, A2.1 :990 save_state-Key-Ernte n>1) | Gate-Code da (Ledger V5-I4 4249/4249); seg_ns n>1 via save_state (A2.1); Rest ⏳ | **E:** Gate im Voll-Lauf-Pfad; seg_ns n>1 ✅-Mechanik; prefetch ehrlich ausweisen; uint16→uint64-Entscheid; SelectMode=search_algo_grid für M3 |
| **K10** | Pattern-/Etiketten-Integrität (12 Major): Adapter-ohne-Adaptee · Memento-Etikett · „Visitor existiert nicht" · Command-tote-Parallelstruktur · MeasurableObserver≠Observer · „B+-Baum"≠B+-Baum · degenerierte Abstract Factory · Release-DLL-Mess-Kopplung | **Pattern-Direktive (Lehrbuch-Patterns + Benennung)** + B1-Split/compile-time-Removal (A2.2, Release-DLL ohne vtable-Slot) + Memento-Etikett→CoW-Rev.2-Rename (Doc 33: undo→cow) | Release-zero-overhead via #if COMDARE_MEASUREMENT_ON (A2.1 :121, A2.2) verifiziert; Memento-Etikett via cow-Rename adressiert; Rest ⏳ | **E (Welle 4, kein Mess-Einfluss):** je Etikett kanonisches Pattern ODER ehrlich umbenannt (grep-Beweis); Release-DLL zero-overhead belegt |

## §2 Major (36 = 24 Mess + 12 Pattern) + Minor (17) — Architektur-Bucket-Zuordnung

- **Mess-Major (24, in K2/K5/K6/K7/K8/K9 verdichtet):** Stamp-Robustheit/XML-Inhalt/Resume-Reihenfolge/CSV-Stream-Fehlercheck/
  Separator-ID-Validierung/Doc-32↔21-XML-Abgleich → **Mess-Resume + Pipeline-Schema (Doc 33 §5 / e2e-Abnahme G1)**. stat_*-enthält-
  Load → **Observer-Reset nach Load (Doc 24 §2.2)**. Second-Execution-Grundsatz → **Zwei-Phasen-PFLICHT (v5_design §4, A5 nur Diskussion)**.
  uint16-Keys/Index-Selektion-konfundiert → **Such-Organ/Key-Raum (Befund 2)**.
- **Pattern-Major (12, alle K10):** → **Pattern-Direktive + 3-Ebenen-Benennung (Doc 30 §8) + B1-Split (A2.2)**. „B+-Baum"-Benennung →
  **Doc 26/experiment_tree** (Baum-Begriff prüfen). Abstract-Factory (Node Static/Dynamic, Doc 26 §4) degeneriert→bool-Flag → echte Factory.
- **Minor (17):** env_limits-im-Stempel · Vollständigkeits-Gate-Zählung · rb_exact-Probe · 2× Kommentar-Drift (Meta-Lehre #5:
  Semantik in Tests nicht Kommentare) · clock::now-Overhead · OOM-Degradationspfade · 8 Pattern-Naming-Minor (Naming-Konvention
  `feedback_naming_convention_allocator_as_goldstandard`). → je Welle mit-erledigt ODER Appendix-Limitierung (b).

## §3 Die 8 Meta-Lehren = BINDENDE Architektur-Regeln (Goal §2.5.4) — auf B/E anzuwenden

1. **Capability-Detection nie still degradieren** — jede `if constexpr(capable)`-Pfadwahl braucht `static_assert` über die ZIEL-
   Population (die 320), nicht nur Referenz-Komp. (= K3-Lehre; gilt für CoW + alle requires-Kaskaden). **A2.1-relevant:** Befund-2-Fix
   muss über echte Pilot-Komposition static_assert'et werden.
2. **Test-Population ≠ Ziel-Population** — Verifikation instanziiert ≥1 echten Vertreter jeder Ziel-Klasse (320 Pilot-Wrapper).
3. **Differenz-Beweise strukturell blind** — Äquivalenz/Diff-Beweis braucht Nachweis VERSCHIEDENER Pfade (Diagnose-Flag im Output).
   **Direkt für die Mission:** die Achsen-Austauschbarkeits-Belege (E) brauchen den Beleg verschiedener Organ-Pfade (NICHT Apparat-Artefakt).
4. **Etiketten-Drift messbar gefährlich** = die Pattern-Direktive: Name/Pattern nur tragen, wenn kanonische Semantik erfüllt.
5. **Dokumentierte Semantik gehört in Tests, nicht Kommentare** (mehrere Kommentar-Realität-Drift-Majors).
6. **Mess-Apparat-Reinheit = eigene Disziplin** — zero-cost-Metaprog systematisch auf den Hot-Pfad (K5). **A2-Befund:** Release-DLL
   zero-overhead via #if COMDARE_MEASUREMENT_ON ist erfüllt; verbleibend container_-O(n) + NotifyPolicy.
7. **env-/Parameter-Volatilität = Daten-Integritätsrisiko** — Resume pinnt die GESAMTE Konfiguration oder bricht ab (K8).
8. **Adversariale Mehrfach-Linsen funktionieren** — Audit-Aufbau resume-fähig wiederverwendbar (genutzt für die A1-Verifikation 2026-06-13).

## §4 A3-FAZIT + E-Auslegungs-Saat (Input für B + E1)

**Architektur-Verankerung vollständig:** Jeder der 85 Befunde hängt an einem A1/A2-Konzept (Befund 2 / Mess-Modell 2-Dim / Observer-I1 /
Memento-CoW / Lastprofil-Bias / Konformitäts-Gate / Pattern-Direktive / 3-Ebenen-Benennung). **Zentrale Erkenntnis für E:** der
dominierende Mess-Echtheits-Defekt ist **Befund-2/Q2-Schritt-4** (search_organ_-Monolith beschattet node/layout) — K5+K6 hängen daran,
und Meta-Lehre #3 (Diff-Beweise) macht ihn mission-kritisch (Achsen-Austauschbarkeit darf nicht Apparat-Artefakt sein). **E-Wellen-Saat
(audit-fundiert, für E1):** Welle-A1 (Resume/Stamp-Härtung K8, Pipeline-Schema) · **Welle-A2 (Apparat-Reinheit: Q2-Schritt-4 + K5(b/c) +
K6 + CoW-für-320/K3 + seg_ns-n>1/K9 → cowfix-v1, DLL-Neubau — das Herzstück)** · Welle-A3 (Dimension/Validität: RC-Entscheid K1, Scrambling
K7c, Konformitäts-Gate K9, SelectMode) · Welle-A4 (Pattern-Hygiene K10). Reihenfolge + Soll-Ort exakt wie Goal §2.5.5; gegen B1-konsolidiertes
Modell verankert. **Done-Disziplin (Goal §2.5):** kein Befund verfällt still — (a) gefixt / (b) Appendix-Limitierung / (c) User-pending.
