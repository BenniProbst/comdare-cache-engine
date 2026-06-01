# Architektur-Entscheidung: Modul-Aufteilung — Git-Submodule vs. DependencyManager

> **Status: ENTSCHIEDEN + UMGESETZT (User „doch befüllen", 2026-06-01).** Der User hat nach dieser Vorlage
> **Option A (Submodule befüllen)** als Sofort-Schritt gewählt. UMGESETZT: alle 6 modules/-Repos sind mit einem
> **kuratierten Public-Header-Set** (120 Header) + Scope-README + INTERFACE-CMakeLists (`comdare::<modul>`)
> befüllt + gepusht (core `f0a2878` · search-engine `b6caff5` · measurement `fa27bf4` · isa-dispatch `f7541de` ·
> build-tools `4fc1df4` · test-system `7bb9e08`); Parent-Pointer gebumpt (CE `ce32e84`). **Non-destruktiv:** der
> Monolith (`libs/cache_engine/`) ist UNVERÄNDERT, `modules/` ist NICHT im CMake-Build-Graph (kein
> `add_subdirectory(modules)`, kein Wurzel-`**`-GLOB) → Monolith-Build grün (Smoke `test_v5_ycsb_op_set` 5/5).
> Die Module sind damit eigenständige, dokumentierte Modul-Spiegel. **VERBLEIBT als Folgephase (Option B):** die
> tatsächliche Konsumptions-Migration (Monolith baut FROM den Submodulen via DependencyManager) + `nested cleanup` —
> das ist die nächste, größere Stufe, weiter gemäß der untenstehenden Analyse.

## 1. Das Problem

Task #22 verlangt, 6 GitHub-Submodule-Repos mit echtem, aus dem Monolithen `libs/cache_engine/` extrahiertem
Code zu **befüllen**:

| Submodul (`.gitmodules`) | Vermutete Verantwortlichkeit | Ist-Zustand |
|---|---|---|
| `modules/comdare-cache-engine-core` | Kern: ExecutionEngine, AnatomyBase, ABI, Achsen-Fundament | Scaffold (3 Dateien, `heads/main`) |
| `modules/comdare-search-engine` | Such-Achsen (axis_03a/03b/04/05 …), Kompositionen | Scaffold |
| `modules/comdare-measurement` | V5-Mess-Pipeline (snapshot/workload/pmc/orchestrator) | Scaffold |
| `modules/comdare-isa-dispatch` | Hardware/SIMD (axis_09/09b/12), ISA-Dispatch | Scaffold |
| `modules/comdare-build-tools` | Codegen, is_original_validator, CMake-Helfer, Emitter | Scaffold |
| `modules/comdare-test-system` | Test-Infrastruktur, gtest-Setup, Fixtures | Scaffold |

**Die Repos existieren + sind initialisiert** (Scaffold: README/CMake/.gitignore), aber **leer an Produktivcode**.
Die E11-Master-Facade-IMPL (`get_cache_engine()`) ist sequenz-gated auf diese Befüllung.

## 2. Der dokumentierte Widerspruch

Zwei bindende Memory-Direktiven stehen der naiven Submodul-Befüllung entgegen:

- **`feedback_no_git_submodules`** — „Keine Git-Submodules generell. **Ausnahme: comdare-cache-engine als Aggregator** (S2686b)."
- **`feedback_curl_bootstrap_architecture`** — „Consumer-Projekte ohne Binaries; **DependencyManager liefert, kein apt-get**."
- **`feedback_buildsystem_not_cmake`** / **`feedback_buildsystem_gitlab_cicd`** — Build über `./configure.sh` + BuildSystem, GitLab-CI/CD.

D.h.: Die ursprüngliche `.gitmodules`-basierte 6-Submodul-Aufteilung (E4.1, alt) könnte **dem späteren
DependencyManager-/curl-Bootstrap-Kurs widersprechen**. Die 6 Repos jetzt mit Submodul-Mechanik zu befüllen,
würde einen möglicherweise **überholten Integrations-Ansatz einzementieren**.

## 3. Die zwei Architektur-Optionen

### Option A — Git-Submodule befüllen (wörtliche #22-Lesart)
- Echten Code je Modul in das jeweilige Submodul-Repo extrahieren; Parent `cache-engine` aggregiert via Pointer.
- **Pro:** entspricht dem vorhandenen `.gitmodules`-Scaffold; vertraute git-Mechanik; E11-Facade direkt bedienbar.
- **Contra:** verletzt `feedback_no_git_submodules`; Submodul-Pointer-Pflege (3-Repo-Sync-Problem ×6); Konsumenten
  müssen `--recursive` klonen; widerspricht dem curl-Bootstrap-Ziel; FortiGate/VPN-Klon-Reibung pro Submodul.

### Option B — DependencyManager / curl-Bootstrap (Memory-konforme Lesart)
- Module als **eigenständige Pakete** (eigene Repos, ABER NICHT als git-Submodule eingebunden); der
  `DependencyManager` zieht sie zur Build-Zeit (Source oder vorgebaut) via curl/GitLab-Artefakt.
- **Pro:** konform zu `feedback_no_git_submodules` + `feedback_curl_bootstrap_architecture`; entkoppelte
  Versionierung; GitLab-CI/CD-Artefakte; kein rekursives Klonen; Konsumenten ohne Binaries.
- **Contra:** `DependencyManager` muss (weiter) implementiert werden; `.gitmodules`-Scaffold wäre zu entfernen
  (→ „nested cache-engine cleanup", der zweite Teil von E4.1+E6); mehr Infrastruktur vor erstem Nutzen.

### Option C — Hybrid (Aggregator-Ausnahme nutzen)
- `cache-engine` bleibt **EIN** Repo (die dokumentierte Aggregator-Ausnahme); die 6 „Module" sind interne
  `libs/<modul>/`-Verzeichnisse mit sauberen Schnittstellen, NICHT separate Repos. Externe Wiederverwendung
  später via DependencyManager-Paketierung der Aggregator-Teilbäume.
- **Pro:** kein Submodul-Bruch, keine Sync-Last, sofort baubar; Modulgrenzen rein über Verzeichnis+Namespace+CMake.
- **Contra:** die 6 GitHub-Repos blieben (vorerst) ungenutzt/Scaffold; „befüllen" wäre auf interne Modularisierung
  umgedeutet.

## 4. Empfehlung (zur Entscheidung)

**Option C (Hybrid) als Zwischenschritt, Option B als Ziel.** Begründung: Die Aggregator-Ausnahme in
`feedback_no_git_submodules` ist genau für `cache-engine` formuliert. Eine saubere INTERNE Modulgrenzen-Ziehung
(libs/<modul>/ + Schnittstellen-Header + per-Modul-CMake, vgl. E10 per-Untermodul-STATIC/SHARED = bereits done)
liefert den Architektur-Nutzen (Entkopplung, getrennte Build-Einheiten) OHNE Submodul-Schulden, und ist später
verlustfrei in DependencyManager-Pakete (Option B) überführbar. Die 6 GitHub-Repos werden erst befüllt, wenn der
DependencyManager + GitLab-CI/CD-Artefaktweg steht (Cluster-prod + GitLab — derzeit „fast bereit").

## 5. Cluster-Kontext (User 2026-06-01)

GitLab-Server läuft; Cluster-dev läuft, Cluster-prod fast bereit; ZIH-externe-Ressourcen bald verfügbar. Der
GitLab-CI/CD-Artefaktweg (Voraussetzung für Option B) wird damit bald real. → #22-Befüllung sinnvoll **erst nach
Cluster-/GitLab-Reife** terminieren.

## 6. Was als nächstes zu entscheiden ist (User)

1. Option A, B oder C?
2. Falls B/C: Wird das `.gitmodules`-6-Submodul-Scaffold entfernt (nested cleanup) oder als Platzhalter belassen?
3. Zeitpunkt der Befüllung relativ zur GitLab-/DependencyManager-Reife.

Bis dahin: **#22 bleibt pausiert** (kein einseitiges Befüllen). E11-Facade-IMPL bleibt sequenz-gated.
