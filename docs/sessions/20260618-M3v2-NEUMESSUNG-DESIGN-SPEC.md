# M3-v2 Neumessung — vollständiger Design-Spec (ausführbar für den Infra-Agenten)

> **Stand 2026-06-18.** Autoritativer Spec für den EINEN umfassenden Mess-Neulauf nach der Phase-E-Vertiefung + den
> Mess-Daten-Korrekturen. Steuert Task **#156**. Bezug: durabler Masterplan `20260618-PHASE-E-VERTIEFUNG-AUDIT-NEUMESSUNG-
> MASTERPLAN.md`, Goal §7, Auswertungs-Agent-Feedback (P-MD1…9), Handover `2026-06-18-HANDOVER-an-infra-agent-linux-pmc-gitlab-ci.md`.
> **Rollen:** Implementierungs-Agent = Code + dieser Spec + gate-freier Build-Prep. Infra-Agent = Linux+PMC-Umgebung + Plattformen +
> der eigentliche Lauf. **HELD bis** der Code-Build-Prep + die Linux+PMC-Umgebung stehen.

## 1. Warum v2 (gegenüber cowfix-v1)
cowfix-v1 (120.960 Z.) beantwortet FF0–FF4 nicht belastbar: CLU-Phantom (P-MD1), seg/total_ns-Nenner-Bug (P-MD3), nur 2 reale
Layout-Strides, kein PMC (P-MD4), kein PRT-ART/SOTA (P-MD6), Working-Set in-cache (P-MD7), Tail-Artefakte (P-MD2/8/9). **Alle
Code-Befunde sind gefixt** (s. Masterplan §1); v2 erhebt die Daten gegen den korrigierten Apparat.

## 2. Build (gate-frei, Implementierungs-Agent — vor dem Lauf)
- **BuildVersion `m3v2`.** Einheitlicher **Neu-Build ALLER Lebewesen-DLLs** (ABI-Bump Snapshot-Version 4→5 wg. #161 seg_framework_ns/
  seg_run_total_ns; +5 reale Layout-Reps #167; prefetch-real #159; +2 Wire-Felder, 178-Feld-Schema). Alte cowfix-v1-DLLs sind
  inkompatibel (Loader-Reject / seg_coverage=n/a) — kompletter Re-Build Pflicht.
- **#155 CMake-Registrierung** der ~10 neuen Tests beim Build mitziehen (Suite grün vor dem Lauf).
- Mess-Build = `COMDARE_MEASUREMENT_ON`; Release/Funktional-only-Build = OHNE Define (PMAJOR-04 #166: Wall-Clock-pur, 0 Observer-
  Kopplungen über die ganze std::map-Schnittstelle — der Doppel-Build belegt die Zero-Overhead-Variante).

## 3. Mess-Matrix (Soll)
### 3a. Basis-Achsen (voll-faktoriell, real)
`search_algo(4) × node_type(4) × memory_layout(5 jetzt PHYSISCH distinkt) × prefetch(4 jetzt _mm_prefetch real)` = 320 — wie
gehabt, aber jede Achse jetzt real (Layout 5 Reps, prefetch reale Adressen). seg_*_ns + seg_coverage neu (P-MD3).
### 3b. Per-Achsen-Sweeps (die 9 vertieften Achsen, Austauschbarkeits-Belege „mit verschiedenen Pfaden")
Gegen eine feste Baseline-Tier je ein Sweep über die Auspraegungen der vertieften Achse: migration_policy (none/HotCold/TierBased/
Adaptive) · filter (none/bloom/cuckoo/xor/surf) · value_handle (inline/pool/versioned/chain) · path_compression (none/patricia) ·
io_dispatch (Fixture-separat, nicht im Mess-Pfad). KEIN volles Kartesisch → echte Diff-Belege ohne Explosion.
### 3c. SOTA + Prüfling (FF3, User-Entscheid „Voll") — die 3 Kompositionalen Joins
PRT-ART + alle 6 SOTA (art/hot/masstree/surf/start/wormhole) unter: **Reihe A** = Stufe-1 ce-only (Pruefling vs SOTA isoliert) ·
**Reihe B** = Stufe-2 Pruefling-ersetzt-mit-Fallback (Entwurfs-Variation) · **Reihe C** = Stufe-3 full-join-non-redundant (Merge/
Regression). Messreihen-Tag A/B/C je Zeile.
### 3d. Working-Set-Sweep (P-MD7) — cache-sprengend
N ∈ {2^14≈16K (in-L2), 2^17≈131K (~cowfix-v1), 2^20≈1M (>L3), 2^23≈8M (>>LLC)} (vom Infra-Agenten an die reale LLC der Zielplattform
anpassen). Erst hier zeigen Layout/Prefetch echten Hebel (Cliff's δ ≥ small).
### 3e. Workloads × Wiederholungen
21 Lastprofile × ≥3 Wiederholungen (separat, nie interpoliert), Two-Phasen-Cache-Warmup PFLICHT.

## 4. Plattformen (FF0, User-Entscheid ≥2) — Infra-Agent
1. Diese Maschine (Hybrid-CPU-Klasse) — Linux-Boot des Experiment-OS AP-M1 ODER Win+msr.sys.
2. ZIH-Cluster **Barnard** (CPU, Sapphire-Rapids-Klasse) und/oder **Capella** — via Infra-Agent + SLURM + Singularity. Plattform-Tag
   je Zeile. (ZIH/Cluster-Manöver = User-Absprache-pflichtig, CLAUDE.md „Kritische Manöver".)

## 5. PMC / Cache-Misses (P-MD4, FF3-Kern) — Infra-Agent
Linux `perf_event_open`/PAPI → reale L1/L2/L3-Misses + dTLB + Branch-Misses + IPC/CPI je Zeile (LinuxPerfPmcSource = IPmcSource-Impl,
Handover CE-DL2). Windows-Fallback: WindowsPcmPmcSource (#153, msr.sys+Admin). → le_limitierung Zeile 1 (Cache-Misses=0) entfällt.

## 6. Quality (P-MD2/8/9) — Infra-Agent + Post-Analyse
- **Quiesziertes Experiment-OS:** CPU-Pinning/isolcpus, Governor=performance, OneDrive/Hintergrunddienste aus, Turbo/HT fix
  dokumentiert; Lauf NICHT im OneDrive-Pfad.
- **Post-Analyse:** Per-Zeilen-Quality-Flag (system-gestört, wenn ns_per_op > k× (binary,workload)-Gruppen-Median) + winsorisierte/
  Perzentil-Ausgabe; Auswertung Median + Statistik-Triade (MWU+Holm+Cliff's δ), KEINE Mittelwerte.

## 7. Nach dem Lauf (Implementierungs-Agent)
csv_to_latex/diagram_generator neu (header-getrieben, verträgt +Spalten) → bias_matrix + ld_exchange (jetzt 9 Achsen, mit echten
Diff-Pfaden) + Surfaces + le_limitierung (Cache-Misses real, K9-prefetch raus, CLU echt) + neue Tabellen (SOTA-Reihen A/B/C, CLU je
Layout, Working-Set-Sweep) → bilinguale PDF → Overleaf → **finaler G5-Re-Audit**.

## 8. Pre-Run-Checkliste
- [x] 6 Achsen real · A1 · K10+PMAJOR-04 · prefetch-Pfad-A · CLU/5-Reps · seg-Coverage — committet.
- [ ] #155 CMake-Reg + Suite grün (beim m3v2-Build).
- [ ] Sweep-/SOTA-/Working-Set-/Plattform-/Workload-Selektion als SelectMode/Profil im Harness (Implementierungs-Agent, gate-frei).
- [ ] Linux+PMC-Umgebung + Plattformen (Infra-Agent) — Handover erweitert.
- [ ] einheitlicher m3v2-Re-Build aller Lebewesen-DLLs.

> **Bottleneck v2:** (a) Sweep-/SOTA-/Working-Set-Harness-Selektion (Implementierungs-Agent, nächster gate-freier Code-Schritt) +
> (b) Linux+PMC + ZIH-Plattformen (Infra-Agent). Erst dann der EINE Lauf.
