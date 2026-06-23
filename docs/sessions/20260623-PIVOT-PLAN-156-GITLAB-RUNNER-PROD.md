# PIVOT-PLAN 2026-06-23 — #156-Messung über Production-GitLab-Runner (statt ZIH)

> **FRISCHE-SESSION-KICKOFF-DOK.** Geschrieben bei Kontext 89% als elaborate Planung; die nächste Session beginnt frisch und
> arbeitet diesen Plan ab. **Pflicht-Pre-Read der nächsten Session:** dieses Dok → Goal `GOAL-AUTONOM-ABARBEITUNG-20260613.md` §9
> (Offener-Stand-Register) → Doc 34 (konsolidierter IST). Dieses Dok **ersetzt** den §9.5-„Montag-Wiedereinstieg" (ZIH/Bare-Metal-
> warten) — die Hardware-Blockade für #156 ist über die prod-GitLab-Runner aufgelöst.

## §1 Provenienz + der Pivot (User 2026-06-23)
Die Production-Maschinen liefern jetzt **nutzbare GitLab-Runner** auf mehreren Plattformen: **Bare-Metal-Ubuntu, Talos OS, und
mehrere Docker-Container (Debian + Fedora)**. Damit wird **#156 (die EINE gültige Messung mit realen Cache-Misses + ≥2 Plattformen)
NICHT mehr von ZIH oder lokalem Bare-Metal blockiert** — sie läuft auf den prod-Runnern. User-Direktiven:
1. **GitLab UND GitHub beide als Remote syncen** (Dual-Remote) für alle Thesis-Repos.
2. **Dort anfangen zu testen** (CI auf den prod-Runnern).
3. **ZIH hinten anstellen** (deprioritisiert, nicht gestrichen).
4. Plattform-Lieferung der prod: Bare-Metal-Ubuntu · Talos OS · Docker Debian/Fedora.

Das ist die Realisierung der CLAUDE.md-„Buildsystem-Delegation" (200+ Projekte / externe Build-Cluster) — nur jetzt der **eigene
prod-Cluster** statt ZIH als Mess-Plattform.

## §2 Ausgangslage (verifiziert 2026-06-23, grep-belegt)
- **Remotes = NUR GitHub** (GitLab fehlt → Phase A): super=`BenniProbst/probst-Diplomarbeit-cache-engine` · cache-engine=
  `BenniProbst/comdare-cache-engine` · prt-art=`BenniProbst/comdare-prt-art` · thesis=`BenniProbst/20260931-Overleaf-Diplomarbeit`.
- **PMC-Code:** `libs/cache_engine/builder/`: `pmc_source.hpp` (IPmcSource) · `pmc_source_factory.hpp` (make_pmc_source) ·
  `windows_pcm_pmc_source.hpp`. Vorhanden: `IPmcSource` + `NullPmcSource` + `WindowsPcmPmcSource`. **KEINE LinuxPmcSource/PAPI** → Phase B.
- **P5-PMC-WIDE-Naht READY** (committet `b285002`/`5d46547`): `make_pmc_source()`/IPmcSource ist im WIDE-Mess-Pfad (`perm_runner`)
  verdrahtet + 7 additive `pmc_*`-cache_misses-Spalten in `lazy_csv_header`. → Eine LinuxPmcSource wird **Drop-in** real getragen.
- **`.gitlab-ci.yml` existiert bereits** in super + cache-engine + prt-art (→ Phase C reviewt+erweitert, erfindet nicht neu).
- **Domain `comdare.de`**; exakte prod-GitLab-URL/Projekt-Pfade/Runner-Tags = offene Frage (§4).
- **Gate-frei = 0** (Re-Audit `wvxmwhvlx`), 85 Audit-Befunde je 1 Done-Zustand, G1-G4 grün gegen cowfix-v1. #156 ist der einzige
  Rest zur vollen G5+§7.4 (§9.4).

## §3 Der Plan in Phasen (jede: offizieller Weg, adversarial verifiziert, Commit+Push+3-Repo-Sync)

### Phase A — Dual-Remote-Sync (GitLab + GitHub)
- prod-GitLab-URL + Projekt-Namespace bestätigen (User/Cluster-Doku; comdare.de, VLAN40). Pro Repo ein GitLab-Projekt anlegen/finden.
- Je Repo einen **zweiten Remote** `gitlab` ergänzen (`git remote add gitlab <url>`), `origin`=GitHub bleibt. ODER `origin` als
  Multi-Push-URL (beide pushen). Entscheidung: getrennte Remotes `github`/`gitlab` (sauberer als Multi-Push) — mit User klären.
- Initial-Push aller 4 Repos nach GitLab (inkl. LFS für die Mess-CSVs — prüfen, ob prod-GitLab LFS hat). **Datenerhaltung:** kein
  History-Rewrite, cowfix-v1/tier150 unverändert mitschieben.
- Submodul-URLs: das super-Repo referenziert die 2 Submodule per GitHub-URL — für GitLab-CI ggf. relative/GitLab-Submodul-URLs
  nötig (`.gitmodules` + CI-`GIT_SUBMODULE_STRATEGY`). Klären.

### Phase B — LinuxPmcSource implementieren (der fehlende Code für reale Cache-Misses)
- Neue `linux_perf_pmc_source.hpp` (Strategy hinter `IPmcSource`, analog `windows_pcm_pmc_source.hpp`) hinter `#if COMDARE_ENABLE_PMC
  && __linux__`. Backend-Wahl: **`perf_event_open(2)`** direkt (kein Extra-Dependency, kernel-nativ) ODER **PAPI** (portabler, aber
  Lib-Dependency → Vendoring analog Boost.MP11/Intel-PCM-Slot). Empfehlung: perf_event_open zuerst (L1/L2/L3-misses, dTLB, ggf.
  energy via RAPL-perf-events), PAPI als Fallback.
- `make_pmc_source()` (pmc_source_factory.hpp) um den Linux-Zweig erweitern (OS-Guard). Auf Bare-Metal-Ubuntu liefert sie real;
  in Containern abhängig von Privilegien (§4).
- **Verifikation am offiziellen Weg:** Build auf einem prod-Linux-Runner (CMake `-DCOMDARE_ENABLE_PMC=ON`); ein Smoke (analog
  `m3v2_pmc_smoke`, ctest-registriert) zeigt `pmc_available=1` + reale Counter ≠ 0. KEIN Scratch-/Behelfsweg.
- Hinweis perf-Permissions: `perf_event_paranoid` ≤ 2 bzw. `CAP_PERFMON`/privileged Container nötig — als CI-Voraussetzung dokumentieren.

### Phase C — GitLab-CI-Pipeline für den #156-Mess-Lauf
- Bestehende `.gitlab-ci.yml` (cache-engine) reviewen + um Mess-Stages erweitern: `build` (offizieller CMake-Weg, 2× configure,
  Boost.MP11-Vendoring) → `measure` (`COMDARE_ENABLE_PMC=ON`, das m3v2-Profil über `run_profile`/run_lazy → m3v2-WIDE-CSV mit realen
  `pmc_*`-Spalten) → `collect` (Artefakt + NAS-Ablage via `scripts/copy_results_to_nas.sh`, UNC/bash).
- **Runner-Tags je Plattform** (bare-metal-ubuntu / talos / docker-debian / docker-fedora) → Job-Matrix für die ≥2 Plattformen.
- M2-Sicherheitsregeln §1 in CI: Resume-Härtung, kein OneDrive (CI-Checkout ist sauber → der OneDrive-C1083-Behelfsweg ist auf den
  Runnern gar nicht nötig — offizieller Weg gewinnt von selbst). DLL-Bau (341) inkrementell + RAM-Admission.
- **Datenerhaltung:** Mess-CSV als neue `build_version` (z.B. `m3v2-prod-<plattform>-<date>`), cowfix-v1/tier150 NIE überschreiben;
  Artefakte additiv.

### Phase D — Plattform-Matrix (≥2 Plattformen, erfüllt P-MD5/#163)
- **Plattform 1 = Bare-Metal-Ubuntu** (echtes PMC via perf_event_open, vollwertig).
- **Plattform 2 =** eines von {Talos-OS-Node, Docker-Debian, Docker-Fedora}. **PMC-in-Container-Frage klären** (perf_event_open
  braucht `--privileged`/`CAP_PERFMON` + Host-perf-Zugang; Talos ist immutable/minimal → ggf. nur Wall-Clock dort). Realistisch:
  ≥2× echtes PMC nur dort, wo der Host-perf erreichbar ist; sonst die zweite Plattform als Wall-Clock-/Layout-Vergleich (ehrlich
  ausweisen). Mit User: welche 2 echten PMC-Plattformen (Hybrid-CPU vs Server-CPU der prod-Maschinen)?
- io_dispatch bleibt Fixture-separat (kein Mess-Pfad); die „9 vertieften Achsen" = 8 sweep-bar (§9.3-c2) — Scope mit User festziehen.

### Phase E — §7.2-Pre-Mess-Sequenz + Datenerhaltung + Resume/NAS
- Die §7.2-Sequenz (Doc `20260618-PHASE-E-VERTIEFUNG-…-MASTERPLAN.md`) auf CI übertragen: Gate-VOR-Messung (conformance), Two-Phasen-
  Warmup PFLICHT, Working-Set-Sweep > LLC, seg-Attribution, quality_flag/winsorize (P5/#165 bereits da).
- Resume-Härtung + NAS-Snapshot je Plattform-/N-Segment (Crash-Resilienz über Stunden-Lauf). NAS nur UNC + bash.

### Phase F — Auswertung → bilinguale PDF → finaler G5+§7.4-Re-Audit
- m3v2-WIDE-CSV → L8 `generate_wide_appendix.ps1` (m3v2-Schalter, die Generatoren `write_sota_series_table`/`…working_set_sweep`/
  `…sweep_axis`/`…seg_coverage` sind da) → bilinguale Abgabe-PDF mit realen Cache-Misses je Tier.
- **Finaler adversarialer G5+§7.4-Re-Audit** gegen literale Evidenz (ZERO actionable) — erst damit ist §9.4(b) erfüllt.

## §4 Offene Fragen / User-Entscheide (am frischen Session-Start einholen, §3-Stop-Bedingung — nicht raten)
1. **prod-GitLab:** exakte URL/Host (comdare.de? IP auf VLAN40?), Projekt-Namespace (`comdare/…`?), Auth (Token/SSH-Deploy-Key) —
   **Credentials NIE im Repo/Log exponieren**, in CI nur als geschützte CI-Variablen.
2. **Remote-Strategie:** getrennte `github`+`gitlab`-Remotes vs `origin`-Multi-Push?
3. **Plattform-Paar für echtes PMC** (P-MD5): welche 2 der prod-Maschinen/OSes liefern host-perf-Zugang (Bare-Metal-Ubuntu + ?)?
4. **PMC-Backend:** perf_event_open (empfohlen) vs PAPI(-Vendoring)?
5. **LFS auf prod-GitLab** vorhanden (für die 100MB+ Mess-CSVs)?
6. **Mess-Scope #156** final: welche ≥9 Achsen vertieft, welche SOTA-Reihen A/B/C, Working-Set-N-Stufen.

## §5 Carry-over-Leitplanken (bindend, aus Goal §0 + Session-Lehren)
- **IMMER der schwere offizielle Weg, NIE parallele Behelfswege** (CMake/ctest; Build-Probleme per Retry am offiziellen Pfad) —
  [[feedback_immer_schwerer_offizieller_weg_keine_behelfswege]]. (Auf CI-Runnern entfällt der OneDrive-C1083-Druck ohnehin.)
- **Messdaten NIE löschen/„ersetzen"** (cowfix-v1 + tier150 unveränderlich; CSV nur additiv/header-getrieben).
- Keine Erfolgsmarke ohne **literale Tool-Ausgabe** (Build-Log/ctest/CI-Job-Log). Je Einheit Commit+Push+**3-Repo-Submodul-Sync**
  (jetzt **auf beide Remotes** GitHub+GitLab).
- **ZIH/PMC/Cluster-Manöver erst nach ausdrücklicher User-Freigabe** (Straf-/Exmatrikulations-Risiko) — gilt für die prod-Maschinen
  abgeschwächt (eigener Cluster), aber prod-Eingriffe weiter mit User abstimmen.
- **Gate-frei-Invariante:** vor jeder „erledigt/leer"-Aussage adversarialer Voll-Audit (9. Meta-Lehre §9.1).
- ultracode (Workflow + adversarial verify); modules/* = tote Snapshots nie anfassen; Thesis = nur User-Ideen persistieren.

## §6 Frische-Session-Kickoff (Schritt 1 der nächsten Session)
1. Pre-Read: dieses Dok + Goal §9 + Doc 34 + (für CI) die 3 bestehenden `.gitlab-ci.yml`.
2. **§4-Fragen 1-6 dem User vorlegen** (kurz, gebündelt) — insb. prod-GitLab-URL/Auth + Plattform-Paar.
3. Dann **Phase A** (Dual-Remote-Sync) als erste ausführbare Einheit, danach Phase B (LinuxPmcSource).
4. Reihenfolge A→B→C→D→E→F; jede Phase = eigene verifizierte+committe Einheit. ZIH bleibt deprioritisiert/HELD.
