# Offene-TODOs-Ledger — Stand 2026-06-18 (Kontext-Ende, vollständig)

> Durable Vollliste ALLER offenen Punkte (das Task-System #1–#168 ist session-flüchtig). Ergänzt Übergabe §2.
> Reihenfolge = Priorität. „Done-Zustand" je Punkt: gefixt / Appendix-Limitierung / HELD-extern / needs_user / deferred.
> Single-Source-Kette: Doc 34 → Übergabe `20260618-SESSION-UEBERGABE-ELABORAT.md` → Masterplan `20260618-PHASE-E-...` →
> Goal §7 → m3v2-Spec `20260618-M3v2-...` → Workflow-Ledger `20260618-WORKFLOW-LEDGER.md` → dieses Ledger.

## 🔴 KRITISCH — Strang A: AbstractFactory-of-Anatomy-Rückbau (HÖCHSTE PRIO, User 2026-06-18)
| # | Punkt | Status | nächster Schritt |
|---|---|---|---|
| **A** | Mess-Selektion (PilotAxes/m3v2_select_profile) als **Abstract Factory der Anatomie** ausprägen statt Parallel-Struktur | **PLAN LÄUFT** (Workflow `wuz2dbsnu`/runId `wf_f582122b-64d`) | Plan-Result abholen → persistieren → User vorlegen → umsetzen |
| **#168** | 4 vertiefte Achsen + 6 SOTA/PRT-ART real baubar | **subsumiert von A** (über die Factory, NICHT DeepPilotAxes — revertiert) | folgt aus dem A-Plan |

## 🟡 GATE-FREI CODE (mein Bereich — autonom, nach Strang A)
| # | Punkt | Status | Notiz |
|---|---|---|---|
| **#162** | P-MD6: PRT-ART + 6 SOTA-Reihen A/B/C real **in den Lauf** | in_progress | Tag-Apparat fertig (`87088cd`); reale DLLs via Factory (Strang A). User-Entscheid: **voll** |
| **#155** | CMake-Reg der ~12 neuen Phase-E-Tests + Suite grün | pending | beim m3v2-Re-Build; macht alle scratch-verifizierten Tests reproduzierbar |
| **#156-Prep** | Auswertungs-Pipeline (csv_to_latex/diagram_generator) auf m3v2-Schema (header-getrieben, +Spalten seg_coverage/CLU/series/working_set_n + SOTA-/CLU-/Working-Set-Tabellen) | pending | gegen Klein-Pilot generalproben |
| **#149** | MP-E: „eine Aufgabe pro Session gegen Audit" | in_progress (Dauerauftrag) | der laufende Mechanismus, kein Einzel-TODO |

## 🟠 MESS-DESIGN / HELD (Strang B — gatet auf Strang A + Infra)
| # | Punkt | Status | gated auf |
|---|---|---|---|
| **#156** | Der EINE umfassende m3v2-Mess-Lauf | **HELD** | Strang A + Linux+PMC + Plattformen |
| **#164** | Working-Set-Sweep > LLC | ✅ **DONE** (`87088cd`, real verifiziert) | — |

## 🔵 INFRA-AGENT (extern delegiert — getrennt geführt, NICHT als erledigt gezählt)
| # | Punkt | Status | Notiz |
|---|---|---|---|
| **#152** | Cache-Misses real (PMC nicht angebunden) | pending | Linux `perf_event_open`/PAPI (LinuxPerfPmcSource); Win-Fallback #153 DONE (`d66b2cf`) |
| **#163** | P-MD5: SIMD/ISA + Allokator variiert + **≥2 Plattformen** (diese + ZIH Barnard/Capella) | pending | User-Entscheid ≥2; ZIH=Absprache-pflichtig |
| **#165** | P-MD2/8/9: quiesziertes Experiment-OS (AP-M1) + Per-Zeilen-Quality-Flag + winsorisiert | pending | quiesziert=Infra; Quality-Flag/winsorisiert=Post-Analyse (mein Bereich) |

## ⚪ needs_user (nominal weiterführen, bis User entscheidet)
| # | Punkt | Frage |
|---|---|---|
| **K1** | RC-Dimension: Organ-Hooks bauen ODER Dimension ehrlich entfernen | User-Entscheid |
| **A5** | Second-Execution vs. Zwei-Phasen-Pflicht | User-Entscheid |

## ⚫ DEFERRED / niedrige Prio (gated/extern/manuell)
| # | Punkt | Status |
|---|---|---|
| **#10** | V42 + Infrastruktur (#648-#653, #613, #619, #621, #622) | niedrige Prio |
| **#19** | Vendor-Allokatoren echt linken (jemalloc/tcmalloc/hoard/scalloc) Voll-Präzision | deferred bis Cluster (Beschaffungs-Specs) |
| **#24** | V41.C1/C2 Cluster-Tasks | BLOCKIERT bis Termin/extern |
| **#25** | V41.D1/D2 Diplomarbeit-Text + Bausteine-Matrix-Doku | **User schreibt manuell** |
| **#125** | P6 lazy DLL-Bibliothek (Content-Hash-Codegen per-Tier-Versionierung) | defensibel deferred |
| **#154** | L-i Tabellen-Textüberlagerung (unleserlich) | **DEFERRED (User 2026-06-18)** |

## ✅ Diese Session abgeschlossen (Referenz — NICHT erneut anfassen)
Phase L+Overleaf (G1–G4) · 6 Achsen real (#122-124, P5b/c/d, #159) · A1 (#157) · K10+PMAJOR-04 (#158/#166) ·
P-MD1→Phase-2 5 Layouts (#167) · P-MD3 seg-Coverage (#161) · Working-Set (#164) · m3v2-Selektions-Gerüst (#164/#162-Tag) ·
durable Doku (Masterplan/Goal§7/m3v2-Spec/Übergabe/Workflow-Ledger/dieses Ledger).

> **Der alte Re-Grounding-Plan** (Schritt 1-3: layout-honorierender Store + Pfad-B-Timing + I1-Observer-Konsolidierung) ist
> **VOLLSTÄNDIG ERLEDIGT** (Store `e018871`/#167, seg-Timing `d7c2595`/#161, I1 committet+grün). Nicht wieder aufgreifen.
