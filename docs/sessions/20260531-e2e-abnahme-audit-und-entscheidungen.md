# End-zu-Ende-Abnahme-Audit (3 Repos) + User-Entscheidungen

**Datum:** 2026-05-31 · **Audit-Workflow:** `w9iy2dhrc` (8 Leser exhaustiv über alle Architektur-Docs + Code-E2E-Verifikation + Synthese) · **Verdikt:** BEDINGTE ABNAHME (Modell korrekt, 2 Code-Blocker + 1 wissenschaftlicher Vorbehalt)

## 1. 3-Repo-Modell — CODE-VERIFIZIERT KORREKT

| Repo | Rolle (IST-belegt) |
|------|--------------------|
| **cache-engine** | Achsen-Bibliothek (15 Topics / 17 Achsen, `axes/`-Rename physisch) + Anatomie-Generator (`AdHocComposition<T0..T16>` → PermutationEngine → `adhoc_emitter` → je Permutation eine SHARED-DLL, 48 real gebaut) + EINHEITLICHES Prüf-Dock (`SearchAlgorithmDock`: `dynamic_cast<IObservableTier*>` über reale DLL-Grenze; Hybrid Pfad A `run_workload`+`f15_compare` / Pfad B `IObservableTier`-POD). Konsumiert prt-art als Prüfling via `COMDARE_CE_PRUEFLINGE`. |
| **prt-art** | PRÜFLING der Gattung SearchAlgorithm — reines Plugin (kein nested Submodul), `comdare_pruefling.cmake` + `register_prt_art_pruefling`; 4/17 Achsen-Slots gefüllt; 3-Stufen-Join compile-time (`pruefling_merge` static_assert). |
| **Diplomarbeit** | 6-Stufen-LaTeX-Pipeline `01 sample → 02 messung_driver → 03 binary→csv → 04 csv→latex → 05 diagram → 06 latex→pdf`; konsumiert CE korrekt (lädt Permutations-DLLs + nutzt CE-`welch_t_test`). |

**Richtungsregel** prt-art-konsumiert-CE im Code erfüllt. **Datenfluss strukturell vorhanden, aber NICHT geschlossen.**

## 2. Befunde (16 Gaps, code-verifiziert)

**BLOCKER (reiner Code-Fix, nicht extern-gated):**
- **G1 CSV-Schema-Bruch:** Stufe 03/04 = 15 Spalten (kein `workload_used`), Stufe 01/05/ResultAggregator = 16 Spalten (`workload_used` an Index 3). Stufe 04 wirft `std::stoull('YCSB_A')` auf 16-Spalten-Input. Kein CSV fliesst sauber durch 03→04→05.
- **G2 keine E2E-Orchestrierung:** kein Target verkettet die 6 CLIs; kein Cross-Stage-Test → der Schema-Bruch blieb unentdeckt.

**WISSENSCHAFTLICHER VORBEHALT:**
- **G3 Umstufung-B im Mess-Pfad nicht vollzogen:** `axis_03a::EnabledStrategies` + `adhoc_emitter` (SA0..SA11) fahren weiterhin 17 monolithische Tiere (Array256..BTree) als search_algo-Achsenwert, KEINE sezierten Organe — entgegen der Direktive „Achsen = NUR Organe". #42 ist „completed" markiert, im Mess-Pfad aber NICHT umgesetzt → F15-Headline-Resultate basieren auf dem Anti-Pattern.

**EXTERN/TERMIN/USER-GATED:**
- G4 reale HW-Messreihe (V21.2, nur Sample-Daten) · G5 prt-art `run()` = `std::unordered_map`-Platzhalter (TODO E6) · G6 PMU-Felder hart 0 / `total_cycles` approximiert · G7 2 real / 5 Vendor-Allokatoren std-Fallback · G8 zwei parallele Plugin-ABIs (alt `comdare_perm_descriptor` von Stufe 02 / neu `comdare_anatomy_perm_*` CE-intern) · G9 `observe_all` real ~2/17 Achsen · G10 `op_type_filter` fehlt · G11 WorkloadKind `Custom_BulkInsert` fehlt · G12 **massive Doku-Drift** (Docs 00–09, Y/Z, prt-art-5-Doks, REV7.7, Doku 23/23a vom Code überholt; nur /goal-V2-Ledger IST-treu) · G13 alte PrtArtSearchEngine koexistiert ungenutzt · G14 Slot-Abdeckung 4/~10 · G15 K-H/K-I-Doku · G16 F.6-Phase-C deprecated (termin-gated).

## 3. User-Entscheidungen (2026-05-31, AskUserQuestion)

| # | Frage | Entscheidung |
|---|-------|--------------|
| 1 | Pipeline-Blocker (CSV + Orchestrierung) | **16-Spalten kanonisch + Orchestrierung** — Stufe 03 + Binary-Record/Writer um `workload_used` erweitern, Stufe 04 auf 16; `add_custom_target` verkettet 6 CLIs + Cross-Stage-Test |
| 2 | Umstufung-B im Mess-Pfad | **Vollenden + F15 neu erheben** — `EnabledStrategies`+`adhoc_emitter` auf Organe, Monolithen deregistrieren, F15 neu messen |
| 3 | Mess-Daten | **i7-1270P lokal als Mindestmessung** — echte lokale Mess-Reihe vor Abnahme |
| 4 | Gated-Punkte | **Vendor-Allokatoren/PMC vorher beschaffen** — jemalloc/tcmalloc echt linken + PMC-Counter vor Abnahme (Beschaffung/Zugang nötig) |

**D1/D2-Volltext** bleibt strikt user-manuell (Thesis-Autorenschaft), sofern nicht anders angewiesen.

## 4. Abgeleitetes Arbeitsprogramm (Basis für neues /goal)

1. **P1 Pipeline-Schließung** (Diplomarbeit, Code): CSV 16-col kanonisch (Record+Writer+Stufe 03/04) + E2E-Orchestrierungs-Target + Cross-Stage-Test → geschlossener Lauf an EINEM Datensatz.
2. **P2 Mess-Pfad-Korrektur** (cache-engine, Code): Umstufung-B vollenden (Organe statt Monolithen in EnabledStrategies+adhoc_emitter) → F15 neu erheben.
3. **P3 Reale Messung** (i7-1270P): Mindest-Messreihe durchfahren → Pipeline → LaTeX/PDF mit echten Zahlen.
4. **P4 Vendor/PMC** (extern-beschaffung): jemalloc/tcmalloc echt linken + PMC-Counter — Beschaffungs-Anforderung an User.
5. **P5 Konsistenz** (Code+Doku): zwei-Plugin-ABI-Konsolidierung, prt-art `run()` (E6)+Slot-Abdeckung, `op_type_filter`+`Custom_BulkInsert`, Doku-Drift (SUPERSEDED-Banner, niemals löschen).

**Abgegrenzt (kein Agenten-Scope):** D1/D2-Volltext (User), F.6-Phase-C-Löschung (nach Habich-Termin~9), Cluster C1/C2 (extern/Termin).
