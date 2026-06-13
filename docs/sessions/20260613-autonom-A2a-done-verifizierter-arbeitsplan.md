# 2026-06-13 — Autonome Abarbeitung: A2a (K3) ERLEDIGT + verifizierter Arbeitsplan (Mapping-Workflow)

> ## ⚠️ TEIL-SUPERSEDED (Masterplan-Phase C4, 2026-06-13 spät)
> **§1 (A2a/K3 CoW-Fix `4a64bc8`) GILT WEITER** — C3-verifiziert: behalten, Memento-konform (Doc 33). Die **Audit-Fix-
> Arbeitspakete §2 (A2b/A2c/A1a–d/A3a–d/A4)** bleiben gültige **E-Wellen-Vorlage** (jetzt geführt über Goal §2.5.5 + A3 +
> Doc 34 §12). **ÜBERHOLT ist allein die §2-Ausführungs-Reihenfolge „L1→L7 zuerst" (Appendix als Flach-Auswertung):** der
> Achsen-Austausch lebt im **B+-Experiment-Baum** (Doc 34 §3/§12 + `20260613-ARCHITEKTUR-KORREKTUR-achsenaustausch-im-baum.md`),
> NICHT in flachen L-Tools (L1/L2 in C2 revertet, L3-WIP gestasht). **Autoritativ jetzt:** Doc 34 (Single-Source) +
> GOAL-AUTONOM (B4-Fassung) + MASTERPLAN (Phasen A–F).
>
> Kontext: Aktives /goal `GOAL-AUTONOM-ABARBEITUNG-20260613.md`. M2-cowmem-v1-Voll-Lauf läuft im Hintergrund
> (63/320 gestempelt, Resume an). Eine Mapping-Workflow (`wf_03ac2361-0ca`, 20 Agenten, 1,64 Mio Tokens)
> hat die Phase-A-Wellen + Phase-L-Lücken gegen den AKTUELLEN Code verifiziert + adversarial geprüft.

## 1. A2a (Audit-K3: CoW real für die 320) — ERLEDIGT + VERIFIZIERT + COMMITTET

**Befund K3:** Die 320 FullPilot-Tiere tragen im `search_algo`-Slot die ROHEN Registry-Wrapper
(`comdare::cache_engine::lookup::{KAry,Interpolation,Eytzinger,LinearScan,…}SearchAlgo`, enumeriert in
`axis_03a_search_algo_registry.hpp`; `AdHocComposition::search_algo = T0` = ein Registry-Typ). Diese hatten
`statistics()/reset()`, aber **kein `restore_statistics`** → `abi_adapter::organ_cow_capable_v` false →
`cow_capable_` false → stiller if-constexpr-Rückfall auf eager Copy-Memento. Der 42/42-`test_cow_memento`
prüfte nur die Referenz-Kompositionen (Art/Hot/Masstree mit `Observable*Organ`-Hüllen) = Test-Pop ≠ Ziel-Pop.

**Fix (ce `4a64bc8` / super `d499065`, gepusht):** `restore_statistics(snapshot_t const&)` in **ALLE 17**
produktiven Roh-Wrapper `axes/lookup/axis_03a_search_algo_*.hpp` ergänzt (additiv, spiegelt
`ObservableComposedSearch:84`; nicht-kopierbare Wrapper bleiben via Copy-Requirement self-gating auf dem
Fallback). NEU `tests/unit/test_cow_capable_wrappers.cpp` + `build_test_cow_capable_wrappers.ps1`:
compile-time `static_assert(OrganCowCapable<lk::…>)` über alle 17 + `SearchAlgoFullyCowCapable<KArySearchAlgo>`
— **literal grün:** `==== #142 Welle 2 / K3: ALLE 17 OK ====`. Damit ist die Audit-Meta-Lehre #1/#2
(Capability über die ZIEL-Population, kein stilles Degradieren) erfüllt.

**Wirkung:** reine Header-Änderung → die laufenden M2-DLLs unberührt; CoW wird real beim M3-cowfix-v1-Neubau.
Messwerte bleiben gültig (Copy-Pfad war rb_exact); gefixt sind Kosten/Label/Test-Abdeckung.

**Lektion (Audit-Methodik bestätigt):** Der Mapping-Agent (mapA) identifizierte FÄLSCHLICH
`ComposedHotPatriciaSearch` (= internes Organ der Referenz-HOT-Komposition) als FullPilot-`search_algo`. Die
**adversariale Verify-A-Stufe korrigierte das** („KArySearchAlgo … korrekt, passt zu organ_cow_capable_v")
und ich verifizierte die Grundwahrheit direkt über Registry + Tiername + Audit-Cross-Check. → Subagenten-Karten
IMMER gegen die Grundwahrheit prüfen.

## 2. Verifizierter Arbeitsplan (Mapping-Workflow-Synthese) — autoritativ für den Rest

**Ausführungs-Reihenfolge (empfohlen):** `L1→L2→L3→L4→L5→L6→L7` (zuerst — Appendix-Pipeline, sicher neben M2,
bedient die Interpretierbarkeit aus dem M2-Teilstand) → `A2a✅ → A2b → A2c → A1a → A1b → A1c → A1d → A3a → A3b →
A3c → A3d → A4` (Audit-Fixes, batch_for_m3) → `M3` (cowfix-v1-Neubau) → `L8` (E2E) → `A5` (Diskussion).

**M2-Sicherheits-Split:** `safe_during_m2` = alle L-Pakete (L1-L7) + A2a-Test/static_assert + A5-Doku.
`batch_for_m3` = alle DLL-verhaltensändernden libs-Edits (A2b/A2c, A1a-d, A3a-d, A4) — werden erst beim EINEN
cowfix-v1-Neubau wirksam; Stamp-/CSV-/Harness-Matrix-Änderungen NIE während M2 einspielen.

**Schon erledigt durch M1 (raus aus der Liste):** K2 ns_per_op-Divisor · die 18 `op_<art>`-CSV-Spalten ·
Harness-env-Pinning+Abbruch · Stamp-`pf.good()`-Gate · **K5a is_new = bereits occupied_count-Delta**
(`abi_adapter.hpp:644-646/677/712-715`, KEIN Doppel-Lookup mehr → A2b nur noch verifizieren).

### Arbeitspakete (file:line + erster Schritt + Verifikation)

| ID | Titel | Dateien | m2 | Größe | User |
|----|-------|---------|----|-------|------|
| **A2a✅** | K3 CoW real (restore_statistics 17 Wrapper + static_assert Ziel-Pop) | axes/lookup/axis_03a_*.hpp + test_cow_capable_wrappers | safe | M | — |
| A2b | K5c/K6 Apparat: container_ `SortedBinaryTraversal`→`LinearScanTraversal` (O(n)-rebuild raus) + echter Policy-Allocator in NodeChunkedStore/LayoutAwareChunkedStore (statt fabrizierter Stats); K5a nur verifizieren | abi_adapter.hpp, observable_composed_container.hpp, axes/node/* | batch | L | — |
| A2c | K5b NotifyPolicy compile-time (NullNotify zero-cost) statt `std::function observer_.notify` (347 Treffer libs-weit), Pilot=array256 dann ausrollen | observable_composed_{search,container}.hpp + 03a-Wrapper | batch | M | — |
| A1a | K4/P2 `tier_scan` als GoF-Iterator-Organ-API (in-order/lower_bound+next), O(scan_len+log n); Hash ehrlich nicht-scanbar; `save_state` nur Memento | abi_adapter.hpp (ab 1262) | batch | L | — |
| A1b | K9 uint16→uint64-Key-Entscheid array256/array65535 (Trunkierung+Negativ-Query-Bruch >32767) | axis_03a_*array256/65535.hpp | batch | M | — |
| A1c | K8 Resume-Stempel: XML-INHALTS-Hash + effektiver Seed + env_limits; Gültigkeits-Gate (two_phase_valid=0 friert nicht ein); Resume-Check VOR b.ok() | cache_engine_builder_iterator.hpp (~292), run_lazy_150.cpp | batch | M | — |
| A1d | K8 XML-Validierung (op_mix-Pflicht, `<records>`), CSV-Stream-Fehlercheck, Separator/ID-Validierung, Doc-32↔21-XML (LP11-Sweep+LP02) | iterator.hpp, algorithm_profiles/load_profiles/ | batch | M | — |
| A3a | K7c Zipfian/Latest Key-Scrambling (Splitmix64) — Hot-Set entzerren | perm_runner.hpp | batch | M | — |
| A3b | K7b/M5 Load-/Insert-Key-Räume trennen (Insert-Profile = echtes Einfügen) | perm_runner.hpp | batch | M | — |
| A3c | K9 Konformitäts-Gate (import→GATE→messen) in Voll-Lauf-Pfad + Selektions-Guard (search_algo-balanciert statt erste-N) | iterator.hpp, run_lazy_150.cpp | batch | M | — |
| A3d | P1/K1 RC-Dimension Organ-Hooks ODER ehrlich entfernen | abi_adapter.hpp, resource_controllable_tier.hpp, algorithm_resource_control.hpp | batch | L | **JA** |
| A4 | K10 Pattern-Hygiene (Adapter/Memento/Visitor/Command/Observer-Etiketten, „B+-Baum", Release-Zero-Overhead) | abi_adapter.hpp, algorithm_visitor/, anatomy/, docs/ | batch | L | — |
| L1 | Ausgabe=Konfig×Tier: Stufe 04 trennt Konfig-Tupel von Tier (binary_id→19 Achsen), ns/op je Interface-Fn aus 18 op_-Spalten; ns_per_op aus total_ns/timed_ops neu | Code/04_csv_to_latex/ | safe | M | — |
| L2 | 3D-Surfaces je Interface-Fn (x=Workload, y=Tier, z=ns/op), relative Pfade; ycsb_e/lp_range_scan ausschließen | Code/05_diagram_generator/ | safe | M | — |
| L3 | Achsen-Austauschbarkeits-Diffs: Tier-Paare die sich NUR in Achse a unterscheiden, Δns/op longtable je Diagramm | Code/04+05 | safe | L | — |
| L4 | appendix_messwerte.tex (§A Konfig+ehrliche Limitierungen, §B 3D, §C Diff, §D Roh-Aggregate) | Code/06+04 | safe | M | — |
| L5 | Thesis-Integration (relative Pfade, diplominf, EN≡DE, Vorlage unangetastet) | thesis/diplomarbeit/ | safe | M | — |
| L6 | Generalprobe: M2-Teil-CSV → L1-L5 → Test-PDF | Code/04-06 + thesis | safe | S | — |
| L7 | Professor-Paket: frischer Snapshot → PDF + 3-Punkte-Austauschbarkeits-Interpretation | Code/06 + thesis | safe | S | — |
| M3 | cowfix-v1: 320 perm-DLLs NEU (nach ALLEN batch_for_m3) + Voll-Matrix (120.960 Zeilen, Scans>0) + NAS | build_and_measure_150_tiere.ps1 | batch | L | — |
| L8 | E2E `-GenerateThesis`: Lauf→Aggregat→Appendix→build.ps1 de+en in EINEM Aufruf über cowfix-v1 | Harness + Code/04-06 + thesis | batch | M | — |
| A5 | Second-Execution-Diskussion (Zwei-Phasen bleibt Pflicht; nur Optionen dokumentieren) | docs/architecture/33_*.md | safe | S | **JA** |

### User-Entscheide (nicht-blockierend, Default greift)
- **RC-DIM (A3d):** RC = write-only Null Object → ×18→×3 degeneriert. Opt A Organ-Hooks (viel Code, echte
  Dynamik) vs Opt B caps=0 + RC-Dims raus (×6 Messzeit gespart). **Default:** M2 belassen + Auswertung weist
  RC als nominale Pseudo-Replikate aus; für M3 RC-Dims entfernen falls keine Antwort.
- **SECOND-EXEC (A5):** Zwei-Phasen-Warmup nullt Cache/BPU-Signale die memory_layout/prefetch differenzieren.
  **Default:** Zwei-Phasen bleibt unverändert (Pflicht-Direktive §0.1); nur Diskussion.

## 3. Nächster Schritt
Gemäß recommended_order: **L1** (Stufe 04 erweitern) — sicher neben M2, höchster Professor-/Thesis-Wert.
Roh-Plan (Volltext je Paket inkl. Verifikations-Kriterium) lag in `tasks/waqp1vwzq.output` (flüchtig) →
dieser Doc ist die durable Fassung. Quelle: GOAL-AUTONOM §2.5 (Audit-Backlog) + diese Synthese.
