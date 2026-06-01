# Kartesische Kardinalität, CLI-Ladebalken & Voll-Mess-Scope (2026-06-01)

> Antwort auf User-Wunsch: „Ladebalken aller kartesischen Produkte auf der Kommandozeile +
> endlich das vollständige kartesische Produkt durchmessen, volles Programm für cache-engine UND prt-art."
> Quelle: Kartierungs-Workflow `wf_438b567b-d33` (6 Agenten) + verifizierter smoke-Lauf.

## 1. ZWEI Kardinalitäten — kritische Unterscheidung

| Größe | Wert | Status |
|-------|------|--------|
| **Registry-Theorie** (compile-time `mp_product` über 22 Achsen-Registries) | **1,41 · 10³²** | **NIE als Binaries materialisiert** — reine Obergrenze. Naiver Voll-Build = ~10²⁵ Jahre → physisch unmöglich. NICHT der Mess-Maßstab. |
| **Build-Pipeline-Realität** (was `codegen.cmake` tatsächlich enumeriert) | siehe unten | das ist der reale Mess-Maßstab |

Der Build-Pfad enumeriert **profilbasierte Achsen-Subsets** (nicht das volle 22-Achsen-Produkt):

| Profil | cache-engine | prt-art | Σ Binaries | Achsen (CE) |
|--------|-------------|---------|-----------|-------------|
| **smoke** (Default im Container) | 3×3×3 = **27** | 2×2×2×2 = **16** | **43** *(aktuell auf Disk verifiziert)* | simd(scalar,sse4,avx2) × layout(aos,soa,hybrid) × alloc(std,jemalloc,mimalloc) |
| **medium** | 4×3×4×2×2 = **192** | 3⁴ = **81** | **273** | +avx512, +snmalloc, +node(compact,wide), +concurrency(single,multi) |
| **full** | ~336 | ~108 | **~444** | Codegen auf **5 Achsen gedeckelt** (`codegen.cmake:104-105`: mehr braucht `cmake_language(EVAL CODE)`) |
| **Stufe-3-Join** (full-join union, `mp_unique<mp_append>`) | — | — | **~51 unique** | (n₀₇+1)(n₀₁+1)(n₁₄+1)(n₁₁+1) über die 4 echten Prüfling-Slots |

> Stale-Korrektur: CE-medium ist real **192** (codegen-Schleife), NICHT 108/144 wie in alten CMakeLists-Kommentaren.

## 2. 3-Stufen-Join (= „volles Programm für ce UND prt-art")

- **Stufe 1** `comdare_perms_ce` → `comdare_permutations_all`: CE-only (smoke 27 / medium 192).
- **Stufe 2** `comdare_perms_pa` → `comdare_prt_art_permutations_all`: Prüfling-Replace (prt-art ersetzt 4 Slots axis_07/01/14/11 mit Fallback; smoke 16 / medium 81).
- **Stufe 3** `comdare_perms_full_join`: non-redundante Union (~51 dedup Kompositionen).

## 3. Ladebalken — ERLEDIGT (Commit DA `32a7bb8`)

ASCII-Fortschrittsbalken in `Code/02_messung_driver/main.cpp`, eingehängt in beide schweren
Plugin-Schleifen (Mikrobench + Welch-Sampling) — deckt CE+prt-art GEMEINSAM ab (eine Schleife
über `load_all_perm_plugins`). Outer-Zähler `[Reihe k/3]`. Reines ASCII (NAS/bash + Windows).
Verifiziert auf smoke (43 Permutationen):

```
[Mikrobench] Kartesische Achsen-Permutationen (43 Permutationen):
[========================================] 43/43 (100%) pa_wide_none_linear_sampling
```

## 4. Smoke-Messlauf — Befund (40/43 valide)

- **40/43** Permutationen valide gemessen (Mikrobench, je n_runs=10), Output: `permutation_stats.csv`,
  `all_permutations.bin`, `welch_pairwise.csv`, `permutation_stats_per_axis.csv`.
- Real diskriminierend: `avx2_aos_*` ≈ 0,0027 µs/op vs. `avx2_hybrid_*` ≈ 0,079 µs/op (Layout ~30×).
- **3 Fehler:** `scalar_aos_{jemalloc,mimalloc,std}` liefern `run()!=0` (ok-samples=0). Pre-existing
  Laufzeitfehler der scalar+aos-Tier-Familie (NICHT durch den Ladebalken verursacht). → **vor einer
  medium-Voll-Coverage zu untersuchen**, sonst fehlen systematisch alle scalar_aos-Kombinationen.

## 5. Voll-Mess-Scope — Entscheidungsmatrix (`only-sampled` ist die ehrliche Lage)

Mess-Pfade: **Mikrobench** (aktiv, kRunOps=1000×kReps=10, Nanosekunden, sehr schnell) vs.
**Vollworkload** (ExperimentDriver, YCSB-Reihen A=5M/B=2,5M/C=5M ops — der teure, wissenschaftlich belastbare Pfad).

| Scope | Binaries | Build | Mikrobench | Vollworkload (3 Reihen) |
|-------|---------|-------|-----------|------------------------|
| smoke | 43 | ~2–3 Min | <1 s | ~1 h single-thread |
| **medium** (Empfehlung) | 273 (+51 Join = 324) | ~20–35 Min | Minuten | **~7–8 h single-thread / <1 h ZIH-Cluster** |
| full | ~444 | ~15–25 Min | Minuten | mehrere h; nur nach codegen-Umbau echtes >5-Achsen-Produkt |

**Empfehlung:** medium als wissenschaftliche Voll-Coverage (alle Achsen mehrfach variiert, statistisch
belastbar); smoke für lokale Iteration; full erst nach `cmake_language(EVAL CODE)`-Umbau (Memory B6
sperrt aktuell die volle Explosion — vorher mit User abstimmen).

## 6. Blocker / Caveats

1. **PMC P4-gated:** die 6 HW-Counter (L1/L2/L3/dTLB/coherence/energy) sind `pmc_available=0` /
   NullPmcSource ohne echte PmcSource → Voll-Messung liefert ohne PMC NUR Wall-Clock + Software-Counter
   (= die ehrlich-leeren 0-Spalten im CSV, vgl. Cache-Line-Validitäts-Frage A1/#26).
2. **scalar_aos_* run()-Fehler** (s. §4) — vor medium-Lauf root-causen.
3. **full-Codegen 5-Achsen-Deckel** — echtes 22-Achsen-Produkt im Build-Pfad nicht ohne EVAL-CODE-Umbau.
4. **ExperimentDriver-Vollworkload** noch nicht in einem Lauf gegen ALLE materialisierten DLLs verschaltet
   (aktuell nur Mikrobench produziert; R5.B/R6 = Task #26 in_progress).

## 7. Nächste Schritte (auf User-Scope-Wahl)

1. **smoke (sofort, lokal):** medium-Profil bauen ist nicht nötig — smoke-Mikrobench liefert bereits eine
   Anhang-taugliche measurements.csv (40/43). Einspeisen in den Thesis-Anhang via
   `generate_measurement_appendix.ps1` möglich.
2. **medium (Empfehlung, ~7–8 h lokal / <1 h Cluster):** `cmake -DCOMDARE_PERMUTATION_PROFILE=medium
   -DCOMDARE_PRT_ART_PERMUTATION_PROFILE=medium -DCOMDARE_PERMUTATION_MODE=on_rebuild` + Stufe-3-Join,
   dann Vollworkload mit `COMDARE_MEASUREMENT_ON` + `COMDARE_EXPERIMENT_MODE_ON`.
3. **scalar_aos_*-Fehler** vor medium fixen.
4. **ZIH-Cluster** (p_llm_compile, 20k core-h): Permutationen sind unabhängige DLLs → ideal für SLURM-Array.
