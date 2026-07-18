# Stand & Wartbarkeit — 2026-06-01 (elaborierte Übergabe)

> **Zweck:** Vollständiger, IST-verifizierter Stand nach dem autonomen TODO-Sweep (2026-06-01), als Basis für
> die vom User gewünschte „Ordnung & Wartbarkeit"-Phase. Single-Source-of-Truth bleibt das
> `architektur-ziele-offene-punkte-ledger.md`; dieses Dokument ist die narrative Übersicht + Wartbarkeits-Agenda.
> **[KLARSTELLUNG 2026-07-16, Voll-Audit F67]:** historisch — das ce-Ledger ist selbst SUPERSEDED; autoritativ = super-Ledger `docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md` (Diplomarbeit-Root).

---

## 0. Executive Summary

Diese Session hat (a) den **Push-Blocker aufgelöst** (Firewall=inside → `git push` funktioniert), (b) den **Build
wiederhergestellt** (3 latente `topics/`-Flags-Include-Bugs), und (c) **alle 6 verbliebenen Hook-TODOs** auf ihren
maximalen Stand gebracht: 4 voll erledigt (#49-E/F, #4, #9, #22-Befüllung), 2 (#19/#26) mit Abstraktion +
Beschaffungs-Specs (reale Umsetzung extern-gated). **Alle Repos synchron auf GitHub, CI grün.**

---

## 1. Diese Session erledigt (verifiziert + gepusht)

| Thema | Ergebnis | Schlüssel-Commits / Evidenz |
|------|----------|------------------------------|
| **Push-Blocker** | Firewall=inside → 57-Commit-Backlog aufgelöst, 3 Repos synchron | `cf38193..` (CE), Submodule-Sync |
| **Build-Recovery** | stale `HAVE_MIMALLOC` + **3× latente `topics/`-Flags-Include-Bugs** (vendor_includes, axis_02, axis_04) | `292aef9` u.a.; dock 3/3, V5-Suiten grün |
| **#49-E/F** | YCSB Scan(E)+RMW(F) treu — **additives `IScannableTier`** (ABI-Minor 1→2, `dynamic_cast`, alte DLLs→null→Skip) | `982ca95`; Unit `test_v5_ycsb_op_set` 5/5 + **Echt-DLL-E2E** scan_n=473/rmw_n=228 |
| **#4 masstree** | is_original 4/4 ALL ORIGINAL (Manifest-Def-Orte-Fix get/insert/remove.hh; p03-Codegen registriert) | `d3abd44`; axis_03a = **6/6** Original-Codes belegt |
| **#9 Naming** | axis_04 `Node{N}Layout`→`Node{N}NodeType` (Kollision mit axis_05 memory_layout beseitigt) | `e4a32bc`; 9 Test-Suiten grün (nodes/traversal/perm_engine/…) |
| **#19/#26 Specs** | Beschaffungs-/Setup-Specs geliefert (Vendor-Allok + reale PMC) | `88e0e7e`; Workflow `wtgq51wnz` (web+code-verifiziert) |
| **#22 Befüllung** | 6 Submodule-Repos mit kuratiertem Public-Header-Set (120 Header) befüllt | `ce32e84`; Workflow `wvwn8ntln`; Smoke 5/5 |

**Lektion #49 (wichtig):** NIE `IObservableTier` in-place erweitern (brach vtable geladener DLLs → SEH 0xc0000005);
neue Lebewesen-Fähigkeiten als **separates optionales Sub-Interface** + `dynamic_cast` (wie IRollbackableTier/IScannableTier).

---

## 2. Repo-/Git-Stand (alle gepusht)

| Repo | HEAD | Remote |
|------|------|--------|
| Diplomarbeit-Superprojekt | `da60baf` (ahead 0) | probst-Diplomarbeit-cache-engine |
| comdare-cache-engine | `2e24997` (ahead 0) | comdare-cache-engine |
| comdare-prt-art | in-sync (ahead 0) | comdare-prt-art |
| modules/comdare-cache-engine-core | `f0a2878` | comdare-cache-engine-core |
| modules/comdare-search-engine | `b6caff5` | comdare-search-engine |
| modules/comdare-measurement | `fa27bf4` | comdare-measurement |
| modules/comdare-isa-dispatch | `f7541de` | comdare-isa-dispatch |
| modules/comdare-build-tools | `4fc1df4` | comdare-build-tools |
| modules/comdare-test-system | `7bb9e08` | comdare-test-system |

**CI:** GitHub-Actions `comdare-cache-engine CI (REV 7.6 V11.7)` auf main = `completed success` (letzte Pushes).

---

## 3. Offene Folgephasen (gated — NICHT vergessen)

| Punkt | Stand | Gate / nächste Stufe |
|------|-------|----------------------|
| **#22 Option B** | Submodule befüllt (Spiegel); Monolith baut noch aus `libs/` | Echte Konsumptions-Migration (Monolith baut FROM Submodulen via **DependencyManager**) + nested-cleanup → gated auf GitLab-/DependencyManager-Reife (Doku 25) |
| **#19 reale Vendor-Libs** | Abstraktion + Spec fertig, Compile-Flag OFF (Stubs) | vcpkg/WSL-gcc/Pre-Built + FortiGate-Egress → `docs/sessions/20260601-19-vendor-allokatoren-beschaffungs-spec.md` |
| **#26 reale PMC** | Abstraktion + Spec fertig, `NullPmcSource` aktiv | Intel-PCM/WinRing0+Admin (Win) bzw. PAPI/perf `-C hwperf` (ZIH) → `docs/sessions/20260601-26-pmc-counter-beschaffungs-spec.md` |
| **#24 Cluster, #25 D1/D2** | extern / User-manuell | Cluster-Termin bzw. Autor-Volltext |

Cluster-Kontext (User 2026-06-01): dev läuft, prod fast bereit, GitLab läuft, ZIH-Ressourcen bald → dann #19/#26-Umsetzung + #22-Option-B.

---

## 4. Wartbarkeits-Agenda (für die „Ordnung herstellen"-Phase)

> **✅ UMGESETZT 2026-06-01 (User-Freigabe):** #1 (CWD-Stray-Fallback der Stufen-Tests 03/04/05 gehärtet via
> CMake-Source-Dir-Compile-Def — kein Stray mehr, test_03 5/5 grün), #2 (Flags-Include-Lint
> `scripts/lint_flags_includes.sh` + CI GitHub/GitLab; **fand sofort 12 weitere latente Instanzen derselben
> Bug-Klasse** in tests/unit/test_v41_axis_* → alle gefixt; Generalisierungs-Check ergab: `*_flags.hpp` ist die
> EINZIGE configure_file-generierte Header-Klasse → vollständig abgedeckt), #3 (`.gitattributes` in allen 4
> Wurzeln gegen LF/CRLF). Commits CE `f93f3a0`, PA `de8ab30`, DA `e134f14`; CI grün. Verbleibend §4-intern:
> nur #4 (MEMORY.md, Cluster-Domäne) + #5 (Spiegel-Drift, gated auf Option B). Stray-`fixtures/` (Punkt 1 unten)
> + die 2 thesis-`.zip` (nun in `.gitignore`, User 2026-06-01) erledigt.

> **✅ FRESH-BUILD-VERIFIKATION 2026-06-01 (definitiv):** Ein komplett frischer Build-Dir (`build/freshverify`,
> 2× Configure → ALLE `generated/`-Header neu erzeugt) gebaut: **39 Targets, FRESH-BUILD EXIT=0, 0 Fehler** +
> **25/25 Test-Suiten grün** (die 15 flags-betroffenen Tests + V5-Suite + dock/adhoc/abi/multi_codegen).
> Damit ist der wiederkehrende Stale-Artefakt-maskierte-Bug-Befund dieser Session (vendor_includes + 15 Flags-
> Includes) **abschließend geschlossen**: die committed Source baut + testet aus dem Nichts sauber. (Temp-Dir
> `build/freshverify` danach entfernt; gitignored.)

> **✅ ALTLASTEN-CLEANUP UMGESETZT 2026-06-01 (User-/goal-Freigabe, Habich-Gate aufgehoben):**
> (a) **`libs/deprecated/prt_art_legacy/` entfernt** (46 Dateien; `git rm` + Tag `pre-cleanup-deprecated-removal-20260601`
> reversibel; `add_subdirectory`+`COMDARE_BUILD_DEPRECATED` raus; CE `6b3ed0d`). (b) **4 tote Registry-Pfade
> repointet** (`axis_library_registry.hpp`: CSS_Node/CSB_NodeGroup/BPlusBinarySearch/FractalChen → reale
> `algorithm_profiles/sota/{css_tree,csb_tree,chen_fractal}.profile.xml`). (c) **`$job.pdf`-Build-Glitch entfernt**
> (+ Disk-Geschwister; `diplomarbeit/.gitignore` `$job.*`; DA `5c7e187`; build.ps1 selbst korrekt, Ursache =
> Nicht-PowerShell-Shell-Ausführung). Verifiziert: reconfigure sauber + `test_commands` 20/20.

Konkrete, in dieser Session aufgedeckte Wartbarkeits-Punkte (Empfehlung, keine Blocker):

1. **🧹 ERLEDIGT — Stray `fixtures/` auf DA-Wurzel entfernt.** Ursache: die Stufen-Tests 03/04/05
   (`test_0X_..._cached_fixtures.cpp`) haben einen CWD-relativen Fallback `fs::current_path() / "fixtures" / "cached"`.
   Wird eine Test-`.exe` manuell aus `Diplomarbeit - Datenbanken/` (statt via ctest, das `COMDARE_FIXTURES_DIR_0X`
   setzt) gestartet, entsteht dort ein Stray-`fixtures/`. **Härtungs-Empfehlung:** den CWD-Fallback durch einen
   Fehler/Skip ersetzen ODER auf die Source-Dir-relative Auflösung beschränken, damit kein Stray außerhalb des
   Stufen-Dirs entstehen kann. Kanonisch bleiben die per-Stufe `Code/<stufe>/tests/fixtures/cached/`.
2. **Latente `topics/`-vs-`axes/`-Flags-Include-Klasse.** 3 Fälle gefunden+gefixt (vendor_includes ×23, axis_02,
   axis_04): Konsumenten inkludierten generierte Flags über einen `topics/`-Pfad, der NICHT generiert wird (nur
   `axes/<…>/` wird via `configure_file` erzeugt; `topics/` sind Forwarder NUR für Source-Header, nicht für generierte).
   **Empfehlung:** ein Lint/Grep-Check „inkludiert irgendwer `topics/.../*_flags.hpp`?" in CI, da ein frischer
   Build-Dir sonst daran scheitert.
3. **LF→CRLF-Warnungen** bei jedem Commit (OneDrive/Windows). Kosmetisch; optional `.gitattributes` mit
   `* text=auto eol=lf` je Repo, um die Warnungs-Flut zu beenden.
4. **MEMORY.md > Größen-Limit** (Cluster-Workstream-Memory). Indexeinträge kürzen / Detail in Topic-Dateien
   auslagern (Cluster-Agent-Domäne, hier nur als Hinweis).
5. **#22-Submodule = Spiegel, kein Konsum.** Solange Option B nicht umgesetzt ist, sind die 6 modules/-Repos
   Kopien der `libs/`-Header → Drift-Risiko. Bis zur DependencyManager-Migration NICHT in den Modul-Repos
   editieren, sondern in `libs/cache_engine/` (die Module sind read-only-Spiegel). In Doku 25 vermerkt.

---

## 5. Architektur-Referenzen (für die Wartbarkeits-Phase relevant)

- **Build-Mechanik Allocator/Vendor:** `docs/sessions/20260601-19-vendor-allokatoren-beschaffungs-spec.md` §1 (ENABLE/HAVE/USE, `comdare::vendor_<v>`, PERMUTATIONS-Gate) — die autoritative Beschreibung des aktuellen Flag-Datenflusses.
- **Modul-Aufteilung:** `docs/architecture/25_modul_aufteilung_submodule_vs_dependencymanager.md` (Option A umgesetzt, B verbleibt).
- **Offene-Punkte-Ledger:** `docs/ledger-sections/architektur-ziele-offene-punkte-ledger.md` (Single-Source-of-Truth, TODO-Sweep-Block 2026-06-01).
- **Mess-Architektur V5:** `docs/architecture/messarchitektur_v5_design.md` + die V5-I1–I10-Ledger-Einträge.

---

## 6. Kurz-Fazit

Code-Substanz, Tests und Doku sind auf einem konsistenten, vollständig gepushten Stand; CI grün; alle lokal-machbaren
TODOs erledigt; die extern-gateten sauber spezifiziert + zurückgestellt. Die Wartbarkeits-Phase kann auf einem
sauberen Fundament starten — die fünf Punkte in §4 sind die konkreten, niedrig-riskanten Aufräum-/Härtungs-Kandidaten.
