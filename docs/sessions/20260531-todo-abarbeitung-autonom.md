# TODO-Abarbeitung autonom (2026-05-31) — /goal „arbeite die übrigen TODOs ab"

> Nach der V5-Re-Verifikation (Substanz-Blocker) hat der User das /goal „arbeite die übrigen TODOs autonom ab"
> gesetzt. Diese Notiz hält fest, was autonom abgearbeitet wurde + wo die ECHTEN externen/entscheidungs-Grenzen
> liegen (mit Evidenz, [[feedback_no_success_marks_without_literal_output]] / [[feedback_verify_ist_state_before_gross_tasks]]).

## Autonom abgearbeitet (verifiziert, committet)
| TODO | Inhalt | Commit | Test-Evidenz |
|------|--------|--------|--------------|
| #50 | Autoritativer 16+6-Mess-POD + Workload→PDF-Bridge | `f70dad7` | test 3/3 + PDF-E2E (echte V5-Daten) |
| #44 | echte per-Achsen `MementoAxis` (2 Organe) + Adapter-Wiring | `d467f69` | organ_memento 2/2 + Adapter-Regression grün |
| #48 | End-to-End 16+6→PDF mit echten Daten | (s. #50) | `pipeline_v5_real.pdf` |
| #49 | YCSB-TREU: Zipfian/Latest-Key-Verteilung + Vollzitat | `262539f` | ycsb_distribution 3/3 (Skew bewiesen) |
| #26 | PMC-Mess-Quellen-Abstraktion (IPmcSource, ehrliches `available`) | `e10c59a` | measurement_snapshot 5/5 |
| #46 | Disk-Memento-Referenz (Kontrakt „inkl. Disk-Persistenz" bewiesen) | `cc901e9` | disk_memento 3/3 |
| #27, #42 | waren bereits erledigt (Triage-verifiziert) | — | axis_08 8/8, Umstufung-B abgenommen |

## ECHTE Grenzen — brauchen User-Entscheidung oder externe Ressource (NICHT autonom lösbar)

- **#49-Rest** (YCSB-E/F): Range-Scan + Read-Modify-Write brauchen NEUE `WorkloadOpKind` + Tier-Scan/RMW-
  Methoden (ABI-Erweiterung). Die Verteilungs-Treue (Hauptmerkmal) IST geliefert. → **Op-Set-Erweiterung = eigener Punkt.**
- **#26-Rest** (reale PMC-Werte): Intel PCM / RDPMC / RAPL-MSR brauchen Vendor-Lib + Admin/MSR-Treiber auf der
  i7-1270P → **Beschaffung/Recht (extern)**. Software-Abstraktion + Drop-in-Punkt sind fertig.
- **#19** (jemalloc/tcmalloc/hoard/scalloc echt linken): vendored, aber `USE_*=0`. jemalloc-Header fehlen
  (autogen.sh), tcmalloc/hoard/scalloc haben keinen Windows-Vendor-Build-Pfad (nur `find_library`). →
  **Windows-Build-Infrastruktur / Lib-Beschaffung + Build-Strategie-Entscheidung.** (mimalloc-only-Aktivierung
  via `-DCOMDARE_BUILD_PERMUTATIONS=ON` ist möglich, aber schwer/riskant — zieht den Permutations-Build.)
- **#22** (6 Submodule-Repos befüllen): `.gitmodules` referenziert 6 GitHub-Repos `BenniProbst/comdare-*`
  (FortiGate-blockiert); der Echtcode liegt komplett in `libs/cache_engine/` (941 Dateien). WIDERSPRUCH zur
  User-Direktive 2026-05-25 „Module sind cache-engine-intern, kein Submodule". → **User-Klärung: ist #22 durch
  die interne-Module-Direktive ÜBERHOLT, oder sollen die Repos doch befüllt+gepusht werden?** (+ Push FortiGate-blockiert.)
- **#9-Naming**: Harmonisierung axis_12/04/03a/q1+q2/08. Kollision axis_12 (general_hardware vs telemetry). →
  **User-Schema-Entscheidung** (welcher Suffix-Standard, Reihenfolge); v42-future. Cross-Constraints+axis_08 (der
  Rest von #9) sind erledigt.
- **#4** (masstree p03 is_original): Original-Source `kohler/masstree-beta` GitHub-FortiGate-blockiert; zudem
  per Ledger als optional/v42-future ausserhalb der Abnahme-Gates. → **Source-Beschaffung (inside-Kanal).**
- **#24** Cluster, **#25** Diplomarbeit-Text: extern-blockiert bzw. User schreibt manuell.

## Fazit
Alle TODOs mit echtem autonomem Fortschritt wurden bis zur externen/Entscheidungs-Grenze vorangetrieben (8 Stück,
test-verifiziert). Die 6 verbleibenden brauchen jeweils einen Input, den ein autonomer Agent in dieser Umgebung
(FortiGate-Egress-Block, keine PMC-Hardware, keine Windows-Allokator-Build-Kette) nicht liefern kann — sie sind
mit konkreter Entscheidungs-/Beschaffungs-Frage dokumentiert.
