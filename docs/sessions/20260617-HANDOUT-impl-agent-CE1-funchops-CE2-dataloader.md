# HANDOUT → Implementierungsagent: AP-CE1 (function-handle-hops) + AP-CE2 (Dataset-Loader-Slot)

**Von:** Text-/Thesis-Agent  ·  **Datum:** 2026-06-17  ·  **Status:** offen (Code-seitig)
**Anlass:** Verbleibende Code-Punkte, nachdem Thesis-Kap. 1–3 abgeschlossen sind (Kap. 4+ macht der User
später manuell). Beide Punkte sind im Thesis-Text **bereits als Erwartung verankert** und brauchen nur
noch die Code-Realisierung. Rückmeldung an Text-Agent erbeten (für etwaigen Kap.-5/6-Nachzug).

---

## AP-CE1 — Zentrales Entwickler-Dokument „function-handle-hops" (Anforderung C02-5)

**Thesis-Verankerung (wörtlich):**
- Kap. 2.3 (Ebenen der Dynamik): „Erst dadurch wird je Interface-Funktion erklärbar, wie sie intern
  verdrahtet ist --- ein Aspekt, den ein eigenes Entwickler-Dokument der Cache-Engine vertieft."
- Kap. 2.3.1 (`std::map`-Interface): die Doku „muss auch für die Weiterentwicklung der cache-engine als
  zentrales Dokument verfügbar gemacht werden, um anderen Entwicklern den Einstieg und die exakten
  cache-engine **function-handle-hops** zu erklären."

**Auftrag:** Erstelle ein zentrales Entwickler-Dokument (in `comdare-cache-engine/docs/`), das **je
`std::map`- und `std::vector`-Interface-Funktion** den exakten internen Hop-Pfad dokumentiert: vom
äußeren ABI-Call über die Achsen-Handles bis zum Ergebnis.

**Muss enthalten:**
1. Pro Interface-Funktion (`find`/`insert`/`insert_or_assign`/`emplace`/`erase`/`at`/`operator[]`/
   `lower_bound`/`upper_bound`/Range-Iteration … sowie `push_back`/`emplace_back`/`resize`/`data`/
   `size`/`empty`/`clear`): die **Kette der Achsen-Handle-Aufrufe** (z. B. search_algo → mapping →
   node_type → memory_layout → allocator → prefetch → telemetry → value_handle …) in Aufruf-Reihenfolge.
2. Die **Compile-Time-Auflösung** je Achse (`resolve_baustein.hpp` / `std::conditional_t`-Tag-Dispatch
   Prüfling-vs-Standard) und wo **statische vs. dynamische Sub-Achsen** eingreifen.
3. Abgrenzung **Compile-Time-Hops** (statische Achsen = eigene Binary) vs. **Runtime-Hops**
   (dynamische Sub-Achsen = Schleifen über derselben Binary) — vgl. Kap. 2.3.
4. Querverweis auf **Anhang F der Thesis** (vollständige Interface-Funktions-Tabelle) als Spiegel —
   die Doku muss ALLE dort gelisteten Standard-Funktionen abdecken (Konsistenz-Pflicht).

**DoD:** Doc existiert; deckt alle Standard-Interface-Funktionen ab (Konsistenz mit Anhang F); je
Funktion der Handle-Hop-Pfad + Compile/Runtime-Markierung; als Onboarding-Einstieg lesbar.

---

## AP-CE2 — Dataset-Loader-Slot für Nicht-YCSB-Frameworks

**Thesis-Verankerung (wörtlich):**
- Kap. 2.4.2: „die Anbindung der nicht-YCSB-Frameworks erfolgt über einen Datensatz-Lader."
- Kap. 3.4.1: „die noch nicht im eigenen Mess-Apparat realisierten Frameworks (TPC, SOSD, SPEC,
  CloudSuite, gem5, Allokator-Suiten) werden über einen Datensatz-Lader angebunden."

**Auftrag:** Implementiere einen austauschbaren **Dataset-Loader-Slot** (Strategy/Adapter), der
Nicht-YCSB-Lasten in das gemeinsame **uint64-basierte Operations-Modell (LP01–LP14)** überführt, sodass
die **alle-gegen-alle-Matrix** (alle Lastprofile × alle Lebewesen-Binaries + Container-Hüllen) fahrbar wird.

**Muss:**
1. Loader-Interface für mindestens: **SOSD** (Index-Keys), **TPC-C/TPC-DS** (OLTP/analytisch),
   **SPEC CPU**, **CloudSuite**, **gem5**-Traces, **reale String-Korpora**, **Allokator-Suiten**
   (mimalloc-bench/Larson/threadtest/shbench).
2. **Mapping** auf das gemeinsame Lastprofil-Modell (Op-Mix + Schlüssel-Verteilung), konsistent mit
   dem LP-Katalog (Thesis Tab. 3.x) und den `<expected_workload>`-Tags der Profil-XML.
3. **Determinismus** (feste Seeds), **Fairness** (identische Eingaben je Kandidat), **N/A**-Ausweisung
   bei fehlenden Fähigkeiten statt verdeckter Emulation (Kap. 2.5-Gütekriterien).

**DoD:** Loader-Slot vorhanden; ≥1 Nicht-YCSB-Framework exemplarisch angebunden + im `messung_driver`
routbar; übrige als dokumentierte, leere Slots; Determinismus/Fairness-Regeln eingehalten.

---

## Allgemeine Konventionen (beide Punkte)
- AxisBase-Defaults, `experiment_compiler()`, Adapter hinter dem Engine-ABI, **Compile-Time-only** im Hot-Path.
- Konsistenz mit Doc 14 (Achsen-Goldstandard) + Doc 18 (Paper-Code-Map) + Anhang F (Thesis).
- **Querverweis:** I/O-Achsen-Handout `20260617-HANDOUT-impl-agent-io-achse-tpie-mehlhorn.md` (verwandte
  Achsen-Erweiterung); Thesis-Tasks AP-CE1 (#83) / AP-CE2 (#84).
