# HANDOVER: Implementierungs-Agent → Infra-Agent (2026-06-18)

> **Absender:** Implementierungs-Agent (cache-engine / Thesis-Mess-Pipeline).
> **Empfänger:** Infra-Agent (COMDARE-Cluster / Linux-Test-Infrastruktur / GitLab).
> **Kanal-Hinweis:** Bitte als CE-D-Wünsche in `cluster_development/docs/sessions/K78-GOAL-CUTOVER-READY.md §8` verlinken
> (Memory `feedback_implementation_agent_delegates_infra_via_k78`). Diese Datei liegt im cache-engine-Repo (GitHub-synchron;
> nach GitLab-Anbindung auch dort).
> **User-Entscheidung 2026-06-18:** Option **(b)** freigegeben (Windows lokal Intel-PCM versuchen) · Option (a) ZIH-PAPI
> ERST nach stabiler Linux-Version · Linux = die uneingeschränkte Umgebung (du arbeitest daran) · GitLab-CI als zweite Remote.

## 1. Warum dieses Handover — die Kern-Lücke der Thesis

Die Diplomarbeit ist über eine **Cache-Engine** (CELM). Die **Cache-Misses (L1/L2/L3 + dTLB + coherence + energy) sind die
Kernmetrik** schlechthin. Aktueller Stand (Windows, lokal):
- **`NullPmcSource` aktiv** (`libs/cache_engine/builder/pmc_source.hpp:41-47`) → `available=false` → **alle Cache-Misses = 0**
  (ehrliches „nicht gemessen", kein Schein-0). Ursache: kein realer PMC (Intel PCM / RDPMC / RAPL-MSR + Admin/MSR-Treiber
  auf der i7-1270P).
- Die **120.960-Zeilen-M3-Hauptmatrix** (WIDE-Schema) enthält die PMC-Cache-Miss-Spalten **gar nicht** — Cache-Verhalten
  wird dort nur **indirekt** über Wall-Clock (`seg_memory_layout_ns`, `ns_per_op`) belegt. PMC lebt nur im separaten
  16-Spalten-Pfad (`ComdareMeasurementSnapshotV1`).

**Linux ist die Lösung:** dort liefern `perf_event_open` / **PAPI** die echten Cache-Miss-Counter ohne die Windows-MSR-Treiber-
Hürde (nur `perf_event_paranoid` ≤ 2 bzw. `CAP_PERFMON`). Deine Linux-Test-Infrastruktur ist damit der Ort, an dem die
**reale Cache-Miss-Messung** entsteht.

## 2. Die Architektur-Naht (was die Implementierung bereitstellt — du musst NICHTS am POD/Pipeline/PDF ändern)

`IPmcSource` ist eine pluggable Schnittstelle (`pmc_source.hpp:30-37`): `begin()` → Op-Lauf → `end()` liefert `PmcCounters`-
Delta + `available()`. Eine Linux-Implementierung ist ein **Drop-in** (eine weitere `IPmcSource`-Klasse) — KEINE Änderung an
`ComdareMeasurementSnapshotV1` / Pipeline-Stufen / PDF nötig (`pmc_source.hpp:6-9`). Ich (Implementierung) liefere die
Interface-Naht + den Mess-Harness; du lieferst die perf-fähige Umgebung + die Linux-Toolchain.

## 3. Konkrete Bitten an den Infra-Agenten (D-Wünsche, Linux)

| ID | Wunsch | Detail / Akzeptanz |
|----|--------|--------------------|
| **CE-DL1** | **Linux-Build** der cache-engine | CMake + gcc/clang (C++23). Test-Suite grün. Toolchain-Doku (welche Compiler-Version, Boost-Offline-Mechanismus existiert bereits). Akzeptanz: `ctest` grün auf Linux. |
| **CE-DL2** | **`LinuxPerfPmcSource`-Umgebung** | perf_event_open/PAPI verfügbar; `perf_event_paranoid` ≤ 2 ODER `CAP_PERFMON` auf dem Runner. Ich scaffolde die `IPmcSource`-Klasse (PAPI_L1_DCM/L2_TCM/L3_TCM/TLB_DM); du stellst die perf-Rechte + ggf. PAPI-Lib bereit. Akzeptanz: `available()==true` + Cache-Miss-Delta > 0 auf einem Smoke-Lauf. |
| **CE-DL3** | **320-DLL-Build + Mess-Lauf auf Linux** | Linux-Äquivalent zu `build_and_measure_150_tiere.ps1` (cmake-Build der 320 `.so` + `run_lazy_150`). Ergebnis: M3-Matrix MIT realen Cache-Miss-Spalten → das macht die Kernmetrik echt. Akzeptanz: eine CSV mit Cache-Miss-Spalten > 0. |
| **CE-DL4** | **GitLab zweite Remote + CI** | Projekt (3 Repos: cache-engine + Superprojekt + prt-art) auf den lokalen comdare-GitLab als 2. Remote laden. `.gitlab-ci.yml` (ich schreibe sie implementierungsseitig) mit Stages: build → test → (optional) measure auf Linux-Runner. Du: GitLab-URL + Runner + Zugang. |
| **CE-DL5** | **(SPÄTER) ZIH-PAPI** | Cluster-Skala über PAPI auf ZIH (Option a). ERST nach stabiler Linux-Version (User-Direktive). SLURM-Array + Singularity sind bereits vorbereitet (KF-12/13). |

## 4. GitLab-Anbindung (Koordination)

- Implementierungsseitig liefere ich die **`.gitlab-ci.yml`** (Linux build+test+measure). Bitte gib mir: GitLab-Server-URL,
  Projekt-Namespace, Runner-Tags, Zugangsweg (Token/Deploy-Key).
- Push-Strategie: GitHub bleibt primär (origin); GitLab als `gitlab`-Remote (`git remote add gitlab <url>`), CI läuft dort.
- Submodul-Konsistenz: die 3-Repo-Submodul-Disziplin (cache-engine ← Superprojekt-Pointer) muss auf GitLab gespiegelt werden.

## 5. Was ich (Implementierung) parallel auf Windows erledige — die Grenze

- **Phase L** (Bias-Matrix → bilinguale Abgabe-PDF): vollständig Windows-machbar (Wall-Clock-Proxy für die 4 variablen
  Achsen search_algo/node_type/memory_layout/prefetch). Läuft hier weiter.
- **Option (b) Windows-Intel-PCM-Versuch:** ich recherchiere die Windows-PMC-Beschränkungen (Intel PCM / WinRing0 / MSR-
  Treiber + Admin) und entweder (i) scaffolde eine `WindowsPcmPmcSource`, oder (ii) **guarde die OS-blockierten Teile sauber
  im Build** (CMake `if(WIN32)`/`if(UNIX)` + Feature-Flag), damit der Windows-Build grün bleibt und die Sperre ehrlich
  ausgewiesen wird. Das Ergebnis melde ich; falls Admin/Treiber nötig, kommt es als kritisches Manöver zurück an dich/User.
- **L-h-Limitierung:** bis reale PMC-Werte (Linux/Windows) vorliegen, weist die Thesis Cache-Misses ehrlich als
  hardware-gated aus + nutzt Wall-Clock als dokumentierten Proxy.

## 6. Offene Koordinations-Fragen an dich

1. GitLab-Server-URL + wie als Remote anbinden (Token/Deploy-Key/Runner-Tags)?
2. Linux-Runner: `perf_event_paranoid`-Wert / `CAP_PERFMON` setzbar? PAPI vorinstalliert oder selbst bauen?
3. Soll die PMC-Messung in die **M3-WIDE-Schema** integriert werden (dann landen Cache-Misses im Hauptdatensatz), oder
   bleibt der getrennte 16-Spalten-PMC-Pfad? (Architektur-Entscheid; `IPmcSource` ist für beides Drop-in.)
4. Status deiner Linux-Infra: was ist schon nutzbar, was fehlt für CE-DL1/DL3?

## 7. Referenzen

- PMC-Naht: `libs/cache_engine/builder/pmc_source.hpp` · POD: `libs/cache_engine/builder/measurement_snapshot.hpp`
- Mess-Harness (Windows): `tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1` + `run_lazy_150.cpp`
- M3-Matrix (3-fach gesichert): `build/thesis_tiere/tier150_measurements.csv` · OneDrive-Backup · NAS
  `//backup1.comdare.de/Cluster_NFS/experiment results/tier150_measurements_INDEX320_cowfix-v1_2026-06-18.csv`
- Phase-L-Plan + L-g/L-h-Befunde: `docs/sessions/20260618-phase-L-einstieg-G2-nas-und-bias-matrix-plan.md`
- Beschaffungs-Specs (Vendor/PMC, bereits vorhanden): `docs/sessions/20260601-26-pmc-counter-beschaffungs-spec.md`,
  `docs/sessions/20260601-19-vendor-allokatoren-beschaffungs-spec.md`
