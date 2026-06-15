# V5 — Die DREI streng getrennten Profile (Build ⊥ Lastenprofil ⊥ Compile-Release)

> User-Direktive 2026-05-31 (/goal V5, GRUNDMODELL): „DREI PROFILE streng getrennt: (1) BUILD-PROFIL
> statisch; (2) LASTENPROFIL host-seitig; (3) COMPILE-RELEASE-PROFIL (cmake, DEFAULT Messung eingebaut).
> Build-Profil ⊥ Lastenprofil = die zwei Haupt-Experiment-Achsen." Diese Notiz bildet jedes Profil auf
> seinen KONKRETEN, im Repo existierenden Mechanismus ab (keine Aspiration).

## Übersicht

| # | Profil | Wann fixiert | Was es bestimmt | Code-Mechanismus (IST) |
|---|--------|--------------|-----------------|------------------------|
| 1 | **BUILD-PROFIL** | Compile-Time (configure) | WELCHE Lebewesen-Binaries (Kompositionen) + Permutations-Gesetzmäßigkeiten gebaut werden | configure-Zeit-Codegen: `r5i_perm_list.cmake`, `comdare_perms_pa_list.cmake`, `f15_perm_list.cmake`, `anatomy_modules_multi` → je eine `.dll`/`.so` pro Komposition |
| 2 | **LASTENPROFIL** | Runtime (host-seitig) | Testdaten-Range + Operationsabläufe (Op-Mix) + Umfang + (Pausen/Zeiten je Gattung) | `WorkloadConfig` + `WorkloadGenerator` (xorshift64, reproduzierbar) + `MeasurementPlan`/`run_measurement_plan` (V5-I9, `workload_orchestrator.hpp`) |
| 3 | **COMPILE-RELEASE-PROFIL** | Compile-Time (cmake-Option) | OB die DLL Messung (observer_all + memento_all) einkompiliert hat oder funktional-only/auslieferbar ist | `COMDARE_MEASUREMENT_MODE` / `COMDARE_RELEASE_MODE` → `COMDARE_MEASUREMENT_ON` (V5-I1, `CMakeLists.txt`) |

## Orthogonalität (die zwei Haupt-Experiment-Achsen)

**Build-Profil ⊥ Lastenprofil.** Das Experiment ist ein kartesisches Kreuz:
```
                   Lastenprofil (host, V5-I9)
                   YCSB-A   YCSB-B   YCSB-C   YCSB-D   InsertHeavy   …
Build-   Komp.1    (1,A)    (1,B)    (1,C)    (1,D)    (1,IH)
Profil   Komp.2    (2,A)    (2,B)    (2,C)    (2,D)    (2,IH)
(DLLs)   …
```
- EINE gebaute Binary (Build-Profil-Zelle) wird gegen MEHRERE Lastprofile gefahren (`run_measurement_plan`,
  V5-I9) → eine Zeile des Kreuzes ohne Neukompilation.
- DASSELBE Lastprofil wird gegen ALLE Binaries identisch wiederholt (`WorkloadGenerator` mit gleichem Seed
  ⇒ bit-identische Op-Sequenz) → eine Spalte des Kreuzes, fair vergleichbar.
Die beiden Achsen sind unabhängig variierbar — das ist die Kern-Faktorisierung des F15-Experiments.

**Compile-Release-Profil ist NICHT eine dritte Experiment-Achse**, sondern ein Build-Schalter:
- `COMDARE_MEASUREMENT_MODE=ON` (Default) → DLL trägt `IObservableTier`+`IMeasurableWorkload`+`IRollbackableTier`
  (observer_all + memento_all einkompiliert) → mess-fähig (das Experiment läuft hierauf).
- `COMDARE_RELEASE_MODE=ON` → erzwingt `MEASUREMENT_MODE=OFF` → DLL trägt NUR `IDriveableTier` (funktionaler
  Gattungs-Antrieb) → kein Mess-Overhead, an die Forschung auslieferbar. Dieselbe Komposition (Build-Profil),
  dasselbe Lastprofil — nur OHNE Messung lauffähig (z.B. Produktiv-Einsatz des gewählten Algorithmus).
Die ABI unterscheidet die Varianten: Major 2 (immer-präsenter `IDriveableTier`-Kontrakt) · Minor 1
(MESSUNG-AN-Variante mit `IRollbackableTier`). Loader-`dynamic_cast` degradiert sauber, wenn ein Sub-Interface
in der geladenen Variante fehlt.

## Reproduzierbarkeit (Pflicht über alle drei Profile)

- Build-Profil: Codegen ist deterministisch (idempotente `*_list.cmake`); gleiche Permutations-Liste ⇒ gleiche DLLs.
- Lastenprofil: `WorkloadConfig` + Seed ⇒ identische Op-Sequenz je Binary (`WorkloadGenerator`, xorshift64).
  Persistiert via `serialize_workload_config` (V5-I9) ins Mess-Protokoll → jede Messung ist exakt rekonstruierbar.
- Compile-Release-Profil: in den cmake-Cache eingebrannt (`COMDARE_MEASUREMENT_MODE` etc.); die ABI-Version
  (`comdare_anatomy_abi_version()`) zeugt zur Laufzeit, welche Variante eine geladene DLL ist.

## Verweise

- V5-I1 cmake-Flags: `CMakeLists.txt` (`COMDARE_MEASUREMENT_MODE`/`COMDARE_RELEASE_MODE`)
- V5-I9 Lastenprofil-Orchestrator: `libs/cache_engine/builder/workload_driver/workload_orchestrator.hpp`
- Zwei-Phasen-Messung (Default bei MESSUNG-AN): `tier_observe_trace_abi.hpp` (V5-I7) + Dock (V5-I10)
- ABI-Varianten: `anatomy_module_abi_v1_decl.hpp` (Major 2 / Minor 1)
