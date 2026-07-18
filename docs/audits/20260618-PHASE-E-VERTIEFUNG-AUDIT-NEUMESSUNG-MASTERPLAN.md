# Phase-E Vertiefung + Audit-Restwellen + Neumessung — durabler Masterplan

> **Persistente Planungssession (überlebt Session-Ende).** Stand **2026-06-18**. Single-Source der offenen Arbeit nach der
> G5-Audit-Korrektur. Bezug: aktives /goal (autonom), Matrix-Plan `20260618-phase-L-einstieg-G2-nas-und-bias-matrix-plan.md`,
> Doc 34 IST-Stand, Audit-Soll-Abgleich `20260613-A3-audit-soll-abgleich.md`, Auswertungs-Agent-Feedback
> `Messdaten-Backup/FEEDBACK_IMPL-AGENT_messdaten-probleme_2026-06-18.md`.
> **Rolle:** Implementierungs-Agent (cache-engine/Thesis). Infra/Linux/PMC/Cluster = delegiert an Infra-Agent.

---

## 0. Kernerkenntnis dieser Session (warum dieser Plan existiert)
Der finale **G5-Audit fing eine echte „declare-victory-by-reclassification"**: 6 Achsen (migration/io/filter/value_handle/
patricia/prefetch) waren als „terminale, vom User gebilligte Limitierungen" geführt — der User-Auftrag 2026-06-04 war aber
**„KEINE Simulation mehr … jetzt zu VERTIEFEN"**. Korrigiert: alle 6 real gemacht. Die **gründliche Audit-Vollverifikation** (alle
85 Befunde) fand **18 weitere offene Punkte** (A1 + K10). Der **Auswertungs-Agent** meldete **9 Messdaten-Probleme** (P-MD1…9),
die zeigen: der aktuelle Mess-Lauf beantwortet FF0–FF4 **noch nicht belastbar**. → Die teure Neumessung ist **bewusst angehalten**,
bis alles Code-seitige + die Mess-Probleme stehen (User-Entscheid: EINE Messung, Linux+PMC).

---

## 1. ✅ ERLEDIGT diese Session (Referenz, nicht erneut anfassen)
- **Phase L (G1/G3) KOMPLETT + auf Overleaf**: Bias-Matrix + 6 Surfaces + 4 Austauschbarkeits-longtables + Limitierungs-Tabelle;
  `build_all.ps1` → bilinguale PDF (132/128 S., 0 Fehler); **fresh-clone byte-identisch** (G3). Generator-Bugs gefixt.
- **G2**: M3-Matrix (120.960 Z., 100 % two_phase_valid, 320/320) + **NAS-Ablage verifiziert** + 3-fach-Backup.
- **#153**: WindowsPcmPmcSource build-sicher gescaffolded (Intel PCM, COMDARE_ENABLE_PMC-Guard).
- **6 Achsen real statt synthetisch** (je Workflow implement→adversarial-verify→Commit-Gate), additiv + memento-sicher:
  migration `a4210c4` · io `1abeabd` · filter `158ef6c` · value_handle `627c8b4` · patricia `9760b92`
  (**Verifier fing use-after-realloc-SIGSEGV → gefixt + Fuzz N=20000/O2**) · prefetch Pfad-B `1364d33` · prefetch Pfad-A `17ea916`.
- **Audit-Restwelle A1** (#157, 7 Mess-Validitäts-Befunde) committet 3-Repo + Overleaf (ce `adff9ea` / super `6fc8688` / thesis `a0d0a32`):
  op_mix-Pflicht, CSV-Wire `==176`+binary_id-Hygiene, LP11-Katalog, OOM→valid=false, +2 ehrliche Limitierungen (uint16/OOM).
- **#159 prefetch-Vollständigkeit**: Pfad-A T7 re-grounded (`seg_prefetch_ns` real), descent-Doku, modules-Spiegel tot bestätigt.

---

## 2. 🔄 / ⏭️ OFFENE TODOs — die elaborate Zusammenfassung

### 2a. Pre-Mess-Code (mein Bereich, vor der Messung)
| Task | Was | Status |
|------|-----|--------|
| **#158** | Audit-Restwelle **K10** — 11 Befunde: 10 terminal (Memento-Rev.1 entfernt; Etiketten ehrlich; PMAJOR-06/07 waren FALSCHE Audit-Claims → korrigiert); **PMAJOR-04 nur TEIL** | 🔄 committet `5586a60`, Rest = #166 |
| **#166** | **PMAJOR-04 VOLLSTÄNDIG (KEIN out-of-scope, User 2026-06-18):** Release-zero-overhead über die **GANZE Pflicht-`std::map`+`std::vector`-Prüfdock-Schnittstelle** — Observer-Kopplungen unter `#if COMDARE_MEASUREMENT_ON` auch in **tier_clear/tier_erase/tier_scan** (+ alle weiteren Interface-Methoden), nicht nur insert/lookup. Am Prüf-Dock werden ALLE std::map/std::vector-Interfaces verlangt ([[std_map_unified_interface]]). Doppel-Build-Modell vom User bestätigt: OFF-Build = **Wall-Clock-Optimierung OHNE Observer** (=Release), ON = Observer/Messung; Mess-Build IDENTISCH. Verifikation: dumpbin = 0 Kopplungs-Symbole über ALLE Interface-Methoden. **Erst damit ist PMAJOR-04 (#158) terminal.** | ⏭️ Pflicht |
| **#160** | **P-MD1 CLU-Instrumentierung (BLOCKER)** — zentrale Cache-Line-Metrik unbrauchbar (field_bytes layout-invariant=46509, cache_lines=Modell-Vielfaches). CLU aus realem Speicher-Image je Layout; muss differenzieren (>20 %, layout-abhängig) | ⏭️ nächster Blocker |
| **#161** | **P-MD3 seg-Timing attributiv** — `sum(seg_*_ns)` deckt nur 33,6 % von total_ns; misst Observer-Overhead statt Organ-Zeit. Coverage >90 %, Observer-Overhead separat | ⏭️ |
| **#165 (b)** | **P-MD8 Per-Zeilen-Quality-Flag** (`ns_per_op > k×` Gruppen-Median = system-gestört) — mein Code-Anteil | ⏭️ |
| **#155** | **CMake-Registrierung** der neuen Vertiefungs-Tests (migration/filter/value_handle/patricia/prefetch×2 + io) + Suite-Build | ⏭️ |

### 2b. Mess-Daten-Probleme (Auswertungs-Agent-Feedback, → Neumessung)
| Task | P-MD | Schwere | Verantwortung |
|------|------|---------|---------------|
| #160 | P-MD1 CLU kaputt | 🔴 BLOCKER | **mein Code** |
| #161 | P-MD3 seg-Timing | 🟠 | **mein Code** |
| #162 | P-MD6 **kein PRT-ART/SOTA** → FF3 fehlt komplett; +Messreihen A/B/C | 🟠 GROSS | Build/Lebewesen (Planungsrunde + User-Scope) |
| #163 | P-MD5 SIMD/ISA+Allokator+≥2 Plattformen | 🟠 | Build + Infra-Agent |
| #164 | P-MD7 Working-Set > LLC (sonst Layout/Prefetch ohne Hebel) | 🟠 | Mess-Design |
| #165 | P-MD2/8/9 quiesziertes Experiment-OS (AP-M1) + Quality-Flag + winsorisiert | 🟡 | Infra + Code |
| (—) | P-MD4 PMC fehlt | 🟠 | **schon abgedeckt** = Linux+PMC (#152/#156) |

### 2c. Die EINE Messung + Post-Mess
| Task | Was | Status |
|------|-----|--------|
| **#156** | **M3-Neumessung — EIN umfassender Lauf** (Linux+PMC): Cache-Misses real + prefetch-real + per-Achsen-Sweeps (9 Achsen) + Working-Set-Sweep + PRT-ART/SOTA + SIMD/Plattformen + Quality-Flag/winsorisiert | 🔒 HELD bis alle Blocker + Linux-PMC-Umgebung |
| (post) | Tabellen neu generieren (bias_matrix + ld_exchange (9 Achsen) + Surfaces + le_limitierung mit realen Cache-Misses, K9 raus) → bilinguale PDF → **Overleaf** | ⏭️ nach Messung |
| (post) | **Finaler G5-Re-Audit** (sollte jetzt sauber durchlaufen) | ⏭️ Abschluss |

### 2d. Legitim getrennt geführt (gated / needs_user — NICHT als erledigt zählen)
- **#19** Vendor-Allokatoren (jemalloc/tcmalloc) — toolchain-gated (Linux/Infra) · **#24** Cluster — GATE-MAXIMAL ·
  **#25** D1/D2 Diplomarbeit-Volltext — user-manuell · **#154 / L-i** Tabellen-Textüberlagerung — User-deferred ·
  **K1/A5** RC-Dimension/Second-Execution — needs_user · **#125 / P6** lazy-DLL-Versionierung — build-effizienz, defensibel deferred.

---

## 3. Pre-Mess-Sequenz (geordnet, autonom)
1. **K10-Welle** (#158, läuft) → verifizieren + committen.
2. **P-MD1 CLU-Blocker** (#160) — zentrale Metrik; ohne sie ist das Cache-Line-Hauptthema wertlos.
3. **P-MD3 seg-Timing** (#161) + **P-MD8 Quality-Flag** (#165b).
4. **#155 CMake-Registrierung** der Vertiefungs-Tests.
5. **Mess-Design-Erweiterung**: Working-Set-Sweep (#164) + per-Achsen-Sweeps + (mit User) PRT-ART/SOTA-Scope (#162) + SIMD/Plattformen (#163).
6. **Handover an Infra-Agent erweitern** (`2026-06-18-HANDOVER-an-infra-agent-linux-pmc-gitlab-ci.md`): Linux+PMC + quiesziertes
   AP-M1 + Plattformen + große Datasets + die Sweep-/SOTA-Matrix.
7. **EINE Messung** (#156) → Tabellen → PDF → Overleaf → **G5-Re-Audit**.

---

## 4. ⚠️ User-Entscheidungs-Punkte (wenn ich dort ankomme)
- **#162 P-MD6 (SOTA-Scope):** welche genau ≥8 Rang-1-SOTA-Lebewesen + PRT-ART, welche Messreihen A/B/C-Tiefe → eigene Planungsrunde.
- **#163 P-MD5 (Plattformen):** welche ≥2 realen Plattformen (Hybrid-CPU + Sapphire Rapids — Cluster/ZIH/Infra-Agent-Verfügbarkeit).

---

## 5. Task-ID-Mapping (durable Spiegelung der ephemeren Session-Task-Liste)
Erledigt: #122 io · #123 migration · #124 filter · (P5b value_handle) · (P5c patricia) · (P5d prefetch) · #157 A1 · #159 prefetch-Rest.
Offen Code: #158 K10 · #160 CLU · #161 seg-Timing · #165 Quality-Flag(+Infra) · #155 CMake.
Offen Mess: #156 Lauf · #162 SOTA · #163 SIMD/Plattform · #164 Working-Set · #152 PMC(=Linux).
Getrennt: #19 #24 #25 #154 #125 + K1/A5.

> **Nächster Schritt nach dem Lesen dieses Docs:** K10 (#158) abschließen → **CLU-Blocker (#160)** angehen.
