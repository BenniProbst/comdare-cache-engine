# Abschluss autonome Strecke 2026-06-20 — gate-freier Ledger-Bereich SATURIERT

> **ZUERST LESEN** (nach Doc 34 + `20260619-UEBERGABE-ELABORAT-ARCHITEKTUR-UND-PLANUNG.md`). Diese Datei = aktueller End-Stand der
> langen autonomen Strecke. **Alle gate-freien Punkte erledigt**; verbleibend NUR extern-HELD / needs_user / Thesis-Survey.

## Geliefert (jede adversarial verifiziert `refuted=False`, je 3-Repo committet)
| # | Punkt | Ergebnis (Commit ce) |
|---|---|---|
| Strang A | Profil-getriebene Selektion (G5-bestätigt) | `52e9428..1e381fd` |
| Build-Modell | `1 DLL=1 TU` korrigiert | `dd16eb3` |
| #168 | FF / 4 vertiefte Achsen sweep-fähig | `e823a2b` |
| #169 | Erweiterungs-Leitfaden + `--validate` | `f45da60`/`905d947` |
| **Datenerhaltung** | „cowfix-v1 wird ersetzt"=FALSCH getilgt; harte Direktive + Memory | `88c8c0b` |
| **#155** | CMake `gtest_discover_tests`-`[1]`-Bug → `add_test`; ctest -N grün | `5a8b37f` |
| **m3v2-Pilot 1** | Basis-Kette real (3 distinkte DLLs, 162-CSV, two_phase_valid-Regression gefixt) | `8e46729` |
| **m3v2-Pilot 2** | SOTA-Reihen A/B + axis_sweep real + C1083-Build-Härtung | `bc848d7` |
| **G1+G3** | Auswertungs-Pipeline m3v2 (SOTA/Working-Set/9-Achsen/seg_coverage) + **#173** Tabellenbreite | super `6cfc2d9` |
| **#171** | Prüfling `abstract`/`full` + additive Typ-Spalte | `733c25c` |
| **#170** | SOTA-Vollabdeckung: **keine Profil-Lücke**; P33→Text-Agent-Handover | `bf11ad1` |
| **#172.1** | `best_binary_selector` (Strategy/Repository/Builder) | `a0a374b` |
| **#174** | `CacheLinePolicySelector` (datengetriebene Heuristik→ComdareResourceControlV1) | `0c6e928` |
| **#175** | XML-Lastprofil-Export (`write_load_profile_xml` + Extraktor, Round-Trip) | `926c9b9` |
| **G5-Repro** | neue Werkzeuge/Tests in CMake (ctest -N 135→137) + 2 Clone-Breaker gefixt | `e2aece6` |

## Der zentrale Meilenstein: die profil-getriebene Voll-Mess-Kette ist BEWIESEN
`m3v2_study.profile.xml → run_lazy_150 → run_profile → B+-Baum → CEB::run_profile → reale distinkte perm.dll → EINE m3v2-CSV` —
**alle 3 Subsets** (Basis-320 · SOTA-Reihen A/B · axis_sweep) real gebaut+gemessen, kein binary_id-Drift (golden-320 stabil),
Datenerhaltung durchgängig (cowfix-v1 + tier150 nie berührt). Der Apparat ist fertig; der Voll-Lauf ist reines Skalieren + PMC.

## Verbleibend — NUR extern-HELD / needs_user / Thesis (kein gate-freier actionable Punkt mehr)
- **#156 Voll-Mess-Lauf (HELD):** 320 + 21 SOTA (inkl. Reihe C) × Two-Phasen × Working-Set-Sweep × ≥2 Plattformen. Braucht
  (a) **PMC/reale Cache-Misses** = Intel PCM (User lädt herunter — FortiGate blockt; Vendoring-Slot dann via Boost.MP11-Muster) **oder**
  Linux+PMC via Infra-Agent; (b) **quiesziertes OS** + ≥2 Plattformen (**ZIH = absprache-pflichtig, Straf-/Exmatrikulations-Risiko**);
  (c) für den 341-DLL-Bau: **OneDrive-Sync pausieren** (C1083-Retry ist drin, aber Pause ist sauberer). **Nie mid-Blocker starten.**
- **K1 (needs_user):** RC-Dimension — Organ-Hooks bauen ODER ehrlich entfernen.
- **A5 (needs_user):** Second-Execution vs. Zwei-Phasen-Pflicht.
- **P33 (Thesis-Survey, Text-Agent):** VAMPIR/NFP — Handover `20260620-HANDOUT-impl-an-text-agent-170-...P33.md`.
- **#154 (an die Messung gekoppelt):** Tabellenbreite — Generator gefixt (#173); 0-Overfull-Vollverifikation erst beim echten
  Kap-7-Messlauf (auto-generierte Tabellen werden dann neu erzeugt).
- **#125 (deferred):** P6 lazy-DLL Content-Hash-Versionierung.

## Nächster Schritt (nächste Session)
Da gate-frei saturiert: entweder (a) **User-Entscheidungen** K1/A5 einholen, oder (b) den **Voll-Lauf vorbereiten**, sobald der User
Intel PCM bereitstellt (Win-PMC-Vendoring) bzw. der Infra-Agent Linux+PMC stellt — **ZIH bleibt absprache-pflichtig**. Der finale
G5-Abgleich (alle 85 Befunde + die EINE gültige Messung + adversarialer Re-Audit) folgt NACH dem Voll-Lauf.
