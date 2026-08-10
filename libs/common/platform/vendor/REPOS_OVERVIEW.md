# libs/common/platform/vendor — First-Party-Vendoring der foundation-Zellen (#265, Reuse P2, Default E-A)

Vendored Kopien der comdare-Matrix-Zellen (Modules/comdare-foundation-all/comdare-baseline_0-foundation),
Muster analog `ext/<topic>/REPOS_OVERVIEW.md` bzw. `libs/common/measurement/hdr_histogram_wrapper/vendor/`.
**Kein Submodul, kein FetchContent** (Ledger #265: „vendorn + comdare::-Alias, KEIN Live-FetchContent, keine Fassade").

| Zelle | Quell-Repo (Branch development) | Quell-SHA | Kopiert am | Lizenz | Pattern |
|---|---|---|---|---|---|
| comdare-platform | gitlab.comdare.de/comdare/modules/comdare-foundation/comdare-platform (GitHub-Spiegel BenniProbst/comdare-platform) | `511fca5759a695f6f068c46c3f36d360c4c9c591` | 2026-07-06 | LICENSE (mitkopiert) | vendored copy, byte-identisch (diff -r == 0) |
| comdare-simd | gitlab.comdare.de/comdare/modules/comdare-foundation/comdare-simd (GitHub-Spiegel BenniProbst/comdare-simd) | `7f0039f` (7f0039f…, dev-HEAD 2026-03-28) | 2026-07-06 | LICENSE (mitkopiert) | vendored copy, byte-identisch (diff -r == 0) |

**Mitkopiert je Zelle:** `CMakeLists.txt`, `include/`, `LICENSE`, `README.md`.
**Bewusst NICHT mitkopiert:** `tests/` (FetchContent googletest v1.14.0 via GIT_REPOSITORY = Netz-Egress + Kollision mit ce-GTest 1.15.2 — die `if(BUILD_TESTS AND EXISTS …/tests)`-Guard der Zellen-CMakeLists macht das Fehlen inert), `sessions/`, `configure.sh/.bat`, `buildsystem.xml`, `.gitmodules`/`cd-buildsystem-core` (nicht-initialisiertes Submodul der Quelle).

**Einbindung:** `libs/common/platform/CMakeLists.txt` — `add_subdirectory(vendor/comdare-platform)` ZWINGEND VOR
`add_subdirectory(vendor/comdare-simd)`: simd linkt `comdare::platform` nur `if(TARGET …)` (Silent-Skip-Falle bei
falscher Reihenfolge; Ledger-Regel „simd setzt platform voraus — immer mitvendorn"). Aliase `comdare::platform` /
`comdare::simd` + Target-Guards liefern die Zellen-CMakeLists selbst (idempotent).

**Abgrenzung (Kollisions-Doku):** Die vendored Namespaces `::comdare::platform` / `::comdare::simd` (Foundation-
Infrastruktur, Host-Wahrheit) sind NICHT `comdare::cache_engine::platform` (AP-3 i_platform_probe, Modell-Ebene /
subject-under-study) und NICHT `comdare::cache_engine::simd` (axis_09-Achse, per D2 AUTORITATIV). In gemeinsamen TUs
vollqualifiziert verwenden, keine `using namespace`-Mischung. Konsumenten-Verdrahtung (AP-3-Follow-ups ARM/sysfs,
7b-3, #270b 3-ISA-Matrix) = EIGENE Increments, nicht Teil dieses Vendoring-Commits.

**Update-Prozedur:** Quelle im Matrix-Klon auf origin/development ziehen → byte-treu neu kopieren (gleiche
Artefakt-Liste) → SHA-Spalte hier aktualisieren → diff -r-Beweis in den Commit-Text.

**Warn-Doktrin (10.08.2026, -Wall/-Wextra/-Wpedantic-Behebung):** Die Zellen-Header werden als
SYSTEM-Includes konsumiert (`INTERFACE_SYSTEM_INCLUDE_DIRECTORIES` in `libs/common/platform/CMakeLists.txt`)
— Warnungen aus dem Vendor-Baum erscheinen nicht in Consumer-TUs, weil Quell-Fixes per Byte-Identitaets-
Invariante ins Upstream-Repo gehoeren, nicht in die Kopie (analog `ext/`).

**Offener Upstream-Befund (10.08.2026, NICHT in der Kopie fixen):** `SIMDDetect.hpp:340/348` vergleicht
`int max_ext_id` gegen `0x80000004`/`0x80000001` (unsigned) — 2x `-Wsign-compare`, Ergebnis via
unsigned-Konversion zufaellig korrekt. Zeile 354 traegt dagegen `static_cast<int>(0x8000001D)`: bei einer
CPU OHNE Extended-Leaves (`max_ext_id == 0`) ist `0 >= -2147483619` WAHR und `detect_x86_cache_amd` liest
einen nicht vorhandenen CPUID-Leaf. Sauberer Upstream-Fix: `max_ext_id` als `unsigned` fuehren und alle
drei Vergleiche unsigned ausfuehren.
