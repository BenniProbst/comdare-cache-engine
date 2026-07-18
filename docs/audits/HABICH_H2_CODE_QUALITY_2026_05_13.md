# Habich-H2 Code-Qualitaets-Bewertung pro Bausteine-Quelle (2026-05-13)

**Anlass:** Habich-Direktive H2 (Habich-Sprechstunde, Aufgabe #101) —
Bewertung der Code-Qualitaet aller geklonten ext-Repos, damit klar wird,
welche Quellen direkt verwendbar sind und welche nur konzeptuell.

**Bewertungs-Skala (1-5 pro Achse):**

| Score | Bedeutung |
|---|---|
| 5 | Excellent — Production-grade, gut gepflegt, dokumentiert |
| 4 | Gut — Stabil, lesbar, kleine Luecken |
| 3 | Akzeptabel — Funktional, aber alt oder unvollstaendig |
| 2 | Eingeschraenkt — Research-Prototype, Stile-Probleme |
| 1 | Schlecht — Schwer wartbar, kaum dokumentiert |

**Bewertungs-Achsen:**

- **Style** — Code-Stil-Konsistenz (Naming, Formatierung)
- **Tests** — Test-Coverage + Test-Qualitaet
- **Docs** — Inline-Doku + externe README/Papers
- **Maint** — Maintainability (Modularitaet, Cyclomatic Complexity)
- **Status** — Pflege-Status (letzter Commit, offene Issues)
- **License** — Lizenz-Klarheit + Kompatibilitaet (Apache 2.0 als Comdare-Default)
- **Build** — Build-System-Modernitaet (CMake >= 3.x bevorzugt)

---

## Such-Algorithmen (P-Reihe)

### P01-ART (Leis/Kemper/Neumann 2013)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++14, klar strukturiert, einige TUM-Conventions |
| Tests       | 3 | Mikrobenchmarks vorhanden, kein Unit-Test-Framework |
| Docs        | 4 | Paper hervorragend, Code-Kommentare moderat |
| Maint       | 4 | Header-zentriert, leicht portierbar |
| Status      | 3 | Letzter Commit 2017, archived |
| License     | 5 | MIT — Apache 2.0 kompatibel |
| Build       | 3 | Makefile-basiert, kein CMake |
| **Gesamt**  | **3.7** | **Empfehlung: Vollstaendige C++23-Re-Implementation** |

### P02-HOT (Binna/Zangerle/Pichl/Specht/Leis 2018)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++17, Concept-orientiert |
| Tests       | 4 | Google-Test-basiert, gute Coverage |
| Docs        | 3 | Paper sehr gut, Code-Kommentare sparsam |
| Maint       | 4 | Modulare CMake-Struktur |
| Status      | 4 | Aktive Pflege bis 2022 |
| License     | 5 | Apache 2.0 |
| Build       | 4 | CMake 3.10 |
| **Gesamt**  | **4.0** | **Empfehlung: Direkt verwendbar als Referenz; Adapter sinnvoll** |

### P03-Masstree (Mao/Kohler/Morris 2012)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++11, MIT-Lab-Stil, viele Templates |
| Tests       | 3 | Custom-Framework, partiell |
| Docs        | 3 | Paper exzellent, Code-Kommentare gemischt |
| Maint       | 2 | Eng gekoppelt mit MIT-eigenem Infrastruktur-Code |
| Status      | 2 | Letzter Commit 2018 |
| License     | 4 | GPL/BSD mixed — Klaerung pro Datei noetig |
| Build       | 2 | Custom configure-script, kein CMake |
| **Gesamt**  | **2.7** | **Empfehlung: NUR konzeptuell verwenden, keine Code-Uebernahme** |

### P04-CoCo-Trie (Boffa/Ferragina/Tosoni/Vinciguerra 2024)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++20, modern |
| Tests       | 3 | Reproducibility-Scripts, keine systematischen Unit-Tests |
| Docs        | 4 | Paper aktuell, README ausfuehrlich |
| Maint       | 4 | Modulare Header-Hierarchie |
| Status      | 5 | Aktiv 2024 |
| License     | 5 | Apache 2.0 |
| Build       | 4 | CMake 3.16 |
| **Gesamt**  | **4.1** | **Empfehlung: Direkt verwendbar; Adapter mit Bedacht** |

### P05-START (Fent/Jungmair/Kipf/Neumann 2020)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++20, TUM-Stil (konsistent zu HOT) |
| Tests       | 3 | Begrenzt, mehr Demo-Code |
| Docs        | 3 | Paper gut, README ok |
| Maint       | 4 | Kompakte Struktur |
| Status      | 3 | Letzter Commit 2022 |
| License     | 5 | Apache 2.0 |
| Build       | 4 | CMake 3.10 |
| **Gesamt**  | **3.7** | **Empfehlung: Re-Implementation mit Paper als Konzept-Quelle** |

### P06-B²-Tree (Schmeisser/Schuele/Leis/Neumann/Kemper 2022)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++17 |
| Tests       | 2 | Sparse |
| Docs        | 3 | Paper gut |
| Maint       | 3 | OK |
| Status      | 2 | Letzter Commit 2023 |
| License     | 3 | Unklar — Email-Anfrage gesendet (P06 in WARTEN-Status) |
| Build       | 3 | CMake basic |
| **Gesamt**  | **2.7** | **Empfehlung: WARTEN auf Lizenz-Antwort + ggf. Re-Impl** |

### P07-Wormhole (Wu/Ni/Jiang 2019)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C99/C11 (NICHT C++) |
| Tests       | 3 | Microbenchmarks |
| Docs        | 4 | Paper sehr klar |
| Maint       | 3 | Single-File-zentriert |
| Status      | 3 | 2020-2022 sporadisch |
| License     | 5 | MIT |
| Build       | 2 | Makefile |
| **Gesamt**  | **3.3** | **Empfehlung: C++23-Portierung in eigener Re-Implementation** |

### P10-SuRF (Zhang/Lim/Leis/Andersen et al. 2018)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++14, RocksDB-Stil |
| Tests       | 4 | Google-Test, gute Coverage |
| Docs        | 4 | Paper + RocksDB-Integration |
| Maint       | 4 | Modular |
| Status      | 3 | Letzter Commit 2020 |
| License     | 5 | Apache 2.0 |
| Build       | 4 | CMake |
| **Gesamt**  | **4.0** | **Empfehlung: Direkte Adapter-Nutzung sinnvoll** |

### P20-B-Trees-Are-Back (Mueller/Benson/Leis 2025)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++20 |
| Tests       | 3 | Reproducibility-Suite |
| Docs        | 4 | Paper sehr aktuell |
| Maint       | 4 | Modern |
| Status      | 5 | 2025 |
| License     | 5 | Apache 2.0 |
| Build       | 5 | CMake 3.20+ |
| **Gesamt**  | **4.3** | **Empfehlung: Direkt verwendbar** |

### P25-Mahling (Mahling/Weisgut/Rabl 2025)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++17 |
| Tests       | 3 | OK |
| Docs        | 4 | Paper TUB |
| Maint       | 4 | Modular |
| Status      | 5 | 2025 |
| License     | 5 | Apache 2.0 |
| Build       | 4 | CMake |
| **Gesamt**  | **4.1** | **Empfehlung: Direkter Adapter** |

### P29-RCU (Userspace-RCU) (McKenney et al. 2001+)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C99, RedHat-Stil |
| Tests       | 5 | Umfassende ltp-style suite |
| Docs        | 5 | doxygen + manpages |
| Maint       | 5 | Industrie-grade |
| Status      | 5 | Aktiv 2025 |
| License     | 5 | LGPL 2.1 (Header) + GPL 2 (Library) — Apache 2.0 KOMPATIBEL via LGPL-Linking |
| Build       | 5 | autotools+CMake |
| **Gesamt**  | **4.9** | **VERWENDET, jetzt aber durch eigene C++23-RCU ersetzt (Aufgabe #104)** |

### P30-HazardPointers (Michael 2004)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++11-Referenz |
| Tests       | 2 | Sparse |
| Docs        | 4 | Paper sehr gut |
| Maint       | 3 | OK |
| Status      | 2 | 2018 |
| License     | 4 | MIT-aehnlich |
| Build       | 2 | Makefile |
| **Gesamt**  | **2.9** | **Empfehlung: Re-Implementation; folgt Aufgabe #104 (RCU) konzeptuell** |

---

## Allokator-Quellen (A-Reihe)

### A01-Hoard (Berger 2000)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++11 |
| Tests       | 2 | Wenige Unit-Tests |
| Docs        | 3 | OK |
| Maint       | 3 | Stabil |
| Status      | 3 | 2022 |
| License     | 4 | GPL2 (problematisch fuer Apache-2.0-Produkt!) |
| Build       | 3 | Makefile |
| **Gesamt**  | **3.0** | **Empfehlung: NUR konzeptuell; GPL2-Problem** |

### A03-MichaelLockfree (Michael 2004 expanded)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++14 |
| Tests       | 3 | OK |
| Docs        | 4 | Paper sehr gut |
| Maint       | 3 | OK |
| Status      | 2 | 2017 |
| License     | 4 | MIT |
| Build       | 3 | Makefile |
| **Gesamt**  | **3.1** | **Empfehlung: Adapter mit Vorsicht** |

### A04-mimalloc (Microsoft Research)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 5 | Sehr konsistent |
| Tests       | 5 | Umfangreich |
| Docs        | 5 | Paper + Wiki |
| Maint       | 5 | Industrie-grade |
| Status      | 5 | Aktiv 2025 |
| License     | 5 | MIT |
| Build       | 5 | CMake modern |
| **Gesamt**  | **5.0** | **Empfehlung: Direkter Adapter — Top-Rang** |

### A05-jemalloc (Facebook/Evans)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C, sehr konsistent |
| Tests       | 5 | Umfangreich |
| Docs        | 4 | gut |
| Maint       | 4 | aktiv |
| Status      | 4 | aktiv 2025 |
| License     | 5 | BSD 2-Clause |
| Build       | 4 | autotools+CMake |
| **Gesamt**  | **4.3** | **Empfehlung: Direkter Adapter** |

### A06-tcmalloc (Google)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 5 | sehr konsistent |
| Tests       | 5 | komplett |
| Docs        | 4 | gut |
| Maint       | 5 | aktiv |
| Status      | 5 | aktiv 2025 |
| License     | 5 | Apache 2.0 |
| Build       | 5 | Bazel + CMake |
| **Gesamt**  | **4.9** | **Empfehlung: Direkter Adapter — Top-Rang** |

### A07-snmalloc (Microsoft Research)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 4 | C++17 |
| Tests       | 4 | gut |
| Docs        | 4 | Paper |
| Maint       | 4 | aktiv |
| Status      | 4 | 2024 |
| License     | 5 | MIT |
| Build       | 4 | CMake |
| **Gesamt**  | **4.1** | **Empfehlung: Direkter Adapter** |

### A08-scalloc (Aigner/Haas et al.)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++11 |
| Tests       | 2 | Sparse |
| Docs        | 3 | Paper ok |
| Maint       | 2 | 2017 |
| Status      | 1 | Stale |
| License     | 4 | Apache 2.0 |
| Build       | 3 | Makefile |
| **Gesamt**  | **2.6** | **Empfehlung: Konzeptuell — Re-Impl wichtiger Achsen** |

### A10-rpmalloc (Mattias Jansson)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 5 | Single-File C, sehr lesbar |
| Tests       | 4 | gut |
| Docs        | 5 | excellent README |
| Maint       | 5 | aktiv |
| Status      | 5 | 2025 |
| License     | 5 | Public Domain / MIT |
| Build       | 4 | CMake + Makefile |
| **Gesamt**  | **4.7** | **Empfehlung: Direkter Adapter — sehr clean** |

### A11-lrmalloc (Leite/Rocha)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C++14 |
| Tests       | 3 | OK |
| Docs        | 3 | Paper ok |
| Maint       | 2 | 2020 |
| Status      | 2 | wenig aktiv |
| License     | 4 | MIT |
| Build       | 3 | Makefile |
| **Gesamt**  | **2.9** | **Empfehlung: Konzeptuell** |

### A20-dlmalloc (Doug Lea)

| Achse | Score | Anmerkung |
|---|---|---|
| Style       | 3 | C89, K&R-Stil |
| Tests       | 2 | Sparse |
| Docs        | 5 | Sehr ausfuehrlich (Doug-Lea-Notes) |
| Maint       | 4 | seit 1987 stabil |
| Status      | 3 | Maintenance-mode |
| License     | 5 | CC0 (Public Domain) |
| Build       | 1 | Single-File |
| **Gesamt**  | **3.3** | **Empfehlung: Referenz-Implementation, konzeptuell** |

---

## Konsolidierte Gesamt-Tabelle

| Rank | Quelle | Gesamt | Empfehlung |
|---|---|---|---|
| 1 | A04-mimalloc                 | 5.0 | Direkter Adapter — Top-Rang |
| 2 | P29-RCU (userspace-rcu)      | 4.9 | Ersetzt durch eigene C++23-RCU (#104) |
| 2 | A06-tcmalloc                 | 4.9 | Direkter Adapter — Top-Rang |
| 4 | A10-rpmalloc                 | 4.7 | Direkter Adapter |
| 5 | A05-jemalloc                 | 4.3 | Direkter Adapter |
| 5 | P20-B-Trees-Are-Back         | 4.3 | Direkter Adapter |
| 7 | P04-CoCo-Trie                | 4.1 | Adapter mit Bedacht |
| 7 | P25-Mahling                  | 4.1 | Direkter Adapter |
| 7 | A07-snmalloc                 | 4.1 | Direkter Adapter |
| 10 | P02-HOT                      | 4.0 | Direkt + Adapter |
| 10 | P10-SuRF                     | 4.0 | Direkt + Adapter |
| 12 | P01-ART                      | 3.7 | C++23-Re-Implementation |
| 12 | P05-START                    | 3.7 | C++23-Re-Implementation |
| 14 | P07-Wormhole                 | 3.3 | C++23-Portierung |
| 14 | A20-dlmalloc                 | 3.3 | Referenz, konzeptuell |
| 16 | A03-MichaelLockfree          | 3.1 | Vorsicht-Adapter |
| 17 | A01-Hoard                    | 3.0 | KONZEPTUELL (GPL2-Problem!) |
| 18 | A11-lrmalloc                 | 2.9 | Konzeptuell |
| 18 | P30-HazardPointers           | 2.9 | Re-Impl |
| 20 | P03-Masstree                 | 2.7 | NUR konzeptuell |
| 20 | P06-B²-Tree                  | 2.7 | WARTEN auf Lizenz |
| 22 | A08-scalloc                  | 2.6 | Konzeptuell |

---

## Konsequenzen fuer die Implementation

1. **Rang 1 (Score >= 4.5) — Direkter Adapter** (5 Quellen):
   - mimalloc, tcmalloc, rpmalloc, userspace-rcu (ersetzt), B-Trees-Are-Back
2. **Rang 2 (Score 4.0–4.4) — Direkter Adapter mit Bedacht** (7 Quellen):
   - jemalloc, CoCo-Trie, Mahling, snmalloc, HOT, SuRF
3. **Rang 3 (Score 3.0–3.9) — Eigene C++23-Re-Implementation** (5 Quellen):
   - ART, START, Wormhole, dlmalloc, MichaelLockfree
4. **Rang 4 (Score < 3.0) — Nur konzeptuell** (5 Quellen):
   - Hoard (GPL2), lrmalloc, HazardPointers, Masstree, B²-Tree (Lizenz),
     scalloc

**Konkrete Auswirkung auf das comdare-cache-engine-Projekt:**

- Die 23 Allokator-Familien-Adapter A01–A23 in
  `cache_engine/include/cache_engine/allocators/families/` wurden bereits
  als **abstrakte Mock-Adapter** angelegt (Phase 6.2.E).
- Pro Quelle gilt die Empfehlung oben: Rang-1/2 sollten einen ECHTEN
  Bind an die `ext/`-Sources haben; Rang-3/4 sollten weiterhin Mock bleiben,
  bis eine eigene Re-Implementation oder ein lizenz-sauberer Workaround
  vorliegt.
- **GPL2-Problem (A01-Hoard):** Comdare ist Apache 2.0 — GPL2-Linking
  ist Lizenz-inkompatibel. Hoard-Adapter MUSS Mock bleiben oder durch
  re-implementierte Aequivalent-Achse ersetzt werden.

## Querverweis

- Bausteine_Matrix.txt: `Diplomarbeit/20260508 Termin 7/Bausteine_Matrix.txt`
- 23 Allokator-Adapter: `comdare-cache-engine/cache_engine/include/cache_engine/allocators/families/`
- 14 LEGACY_REIMPL-Skelette: `comdare-cache-engine/prt_art/legacy_reimpl/P{11..27}/` (Rang-3-Re-Impl-Stubs)
- eigene RCU: `comdare-cache-engine/cache_engine/reclamation/rcu_reclaim/` (#104)
- Email-Anfragen (P06/P28/P31/P32/P33): siehe Task #74
