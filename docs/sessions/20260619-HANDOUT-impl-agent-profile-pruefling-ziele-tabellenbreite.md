# Handout an den Implementierungsagenten (2026-06-19)

> Absender: **Text-Agent** (Diplomarbeit). Empfänger: **Implementierungsagent** (cache-engine / prt-art Code).
> Anlass: Cross-Check der geschärften Front-Matter gegen den Code (Code = Primärquelle). Vier Arbeitspakete,
> die **code-/generator-seitig** zu erledigen sind und die der Text-Agent nicht selbst ausführt
> (Code ist für den Text-Agenten read-only).

---

## TODO-1 — SOTA-Profile auf Paper-Vollabdeckung ausbauen

**Ist-Stand (verifiziert):** `algorithm_profiles/sota/*.profile.xml` = **30** funktionierende Profile;
`compositions/*_reference.hpp` = 13 Referenz-Kompositionen (8 Rang-1). Die Diplomarbeit nennt im Abstract/Intro
jetzt **konsistent „dreißig"** (vorher DE-Abstract fälschlich „mehr als vierzig" — war eine Ziel-Schätzung des
Autors, korrigiert).

**Ziel (Autor-Direktive 2026-06-19):** Jedes in Kap. 3 analysierte Paper (**P01–P33**) liefert
**mindestens einen neuen Algorithmus für mindestens eine Achse**. Zwei Paper-Typen:
1. **Abstrakter Achsen-Satz** — das Paper liefert nur einzelne neue Achsen-Algorithmen (wie PRT-ART),
   keinen vollständigen Algorithmus → fließt als Achsen-Variante(n) in den Baustein-Katalog ein.
2. **Voll-Algorithmus** — das Paper erfüllt die Mindest-Achsenzahl für ein vollständig eigenes Verfahren
   → eigenes vollständiges SOTA-Profil.

**Auftrag:** Lücke 30 → ≥ Paperzahl schließen; je Paper prüfen, welcher Typ vorliegt, und die fehlenden
Profile/Achsen-Konfigurationen anlegen. Mapping-Grundlage = Doc 18 (Algorithmus↔Paper↔Code↔Lizenz) +
`docs/sessions/.../kap3-instanz-mapping-survey` (Thesis-Seite).

---

## TODO-2 — Prüflinge: abstrakte **und** vollständige Varianten + „Originalkonfiguration"

**Definition Prüfling** (zur Selbstvergewisserung nachschlagen: Doc 14 §18–§19 Prüfling-Slot-Pattern,
3 kompositionale Joins): der **Prüfling** ist das zu testende Kandidaten-Lebewesen (z. B. PRT-ART), das
Achsen-Slots gegen den CacheEngine-Standard ersetzt/füllt (Stufe 1 ce-only / Stufe 2 Prüfling-Replace mit
Fallback / Stufe 3 Full-Join).

**Auftrag:** Für jeden Prüfling sicherstellen, dass **beide** Ausprägungen existieren und gemessen werden:
- **Abstrakter Prüfling** — füllt nur eine Teilmenge der Achsen, restliche Achsen via ce-Fallback.
- **Vollständiger Prüfling** — die **„Originalkonfiguration"**: mindestens **einmal** wird das Verfahren
  **NUR** mit den **eigenen** Achsen-Algorithmen des Prüflings *self-contained* gemessen und bewertet
  (vgl. `docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md` §0/§1.2: „bekannte
  Paper-Suchalgorithmen als Basis-Lebewesen in Originalkonfiguration").

**Kennzeichnung (neu):** In der **Profil-Erzeugung** und in den **Messungen** muss je Messreihe der Typ
**`abstract` / `full`** explizit gesetzt und ausgegeben werden (eigenes Feld/Spalte), damit die Auswertung
Original-vs.-rekombinierte Konfiguration trennen kann.

---

## TODO-3 — Noch nicht umgesetzte Ziel-Versprechen implementieren

Diese drei Punkte sind in der Diplomarbeit jetzt **ehrlich als Zielsetzung/Ausblick** formuliert (Code gewinnt),
ihre **Implementierung** ist Aufgabe des Impl-Agenten:
1. **Automatische Auswahl + Versand der besten Binary** — aus den feingliedrigen Messergebnissen die beste
   Permutation post-hoc auswählen und als eigenständige, ABI-stabile Binary ausliefern. (Die Mess-Pipeline
   liefert die Rangbildung bereits; Auswahl + Versand fehlen.)
2. **Laufzeit-dynamische Cache-Line-Anpassung** — anhand eines ausgewerteten Profils zur Laufzeit die
   korrekten cache-line-aware Einstellungen je Last/Operation wählen (statische heuristische System-Richtlinie).
3. **XML-Heuristik-/Lastprofil-Export** — die „Extraktion" als wiederverwendbares XML-Lastprofil-Ergebnis je
   Architekturfokus ablegen, um Heuristiken automatisch zu erzeugen.

---

## TODO-4 — Tabellen-/Diagramm-Generator: Breite auf `\textwidth` zwingen (Layout-Bug)

**Befund (Thesis-Build 2026-06-19):** **184 Overfull-Boxen** (davon **134 > 10 pt**), DE und EN identisch,
**ausschließlich** aus `anhang/{de,en}/tabellen/` (auto-generiert), z. B.:
- `lc_surface_*` (TikZ-Heatmap, `width=0.95\textwidth` + `scale only axis` + `colorbar` + Y-Label → Gesamtbreite
  > `\textwidth`, ~77 pt).
- `cartesian_smoke43_diagram_body` (TikZ ybar, `width=0.85\textwidth`, 43 symbolische x-Koordinaten, `bar width=10pt`).
- `ld_exchange_*` (Alignment ~35 pt zu breit), `bias_matrix_table` (21-Spalten-WIDE-Tabelle).

**Ursache:** Bei `scale only axis` gilt `width=` nur für die **Achsenfläche**; Titel, Achsenbeschriftung,
Tick-Labels und **Colorbar** kommen **zusätzlich** dazu → die Gesamtbox überschreitet `\textwidth`.

**Auftrag (Generator `diagram_generator` REV 7.6 / `csv_to_latex`):**
- Plots mit Colorbar/Y-Label: `width` auf ~`0.72–0.78\textwidth` reduzieren **oder** das gesamte
  `tikzpicture` in `\resizebox{\textwidth}{!}{…}` setzen (Nicht-Float-Variante) bzw. die `figure` mit
  `\centering\makebox[\textwidth]{\resizebox{\textwidth}{!}{…}}` kapseln.
- Sehr breite WIDE-Tabellen (≥ ~15 Spalten): `\scriptsize` + `\setlength{\tabcolsep}{2pt}` + bei Bedarf
  `sidewaystable`/landscape **oder** Spaltensplit.
- Ziel: **0** echte (> 10 pt) Overfull-Boxen aus `tabellen/`.

**Hinweis:** Der Text-Agent patcht diese **auto-generierten** Dateien bewusst **nicht** (würden bei der
nächsten Generierung überschrieben). Es sind Demo-/Platzhalter (smoke43) — bei den echten Kap-7-Messläufen
ohnehin neu zu erzeugen, dann bitte mit der korrigierten Breiten-Logik.

---

## TODO-5 — Architektur-Klärung: `search_engine` (Klasse) ↔ Anatomie — KEIN blinder Umbau

**Anlass:** Cross-Check (Workflow `we4xlbb23`, 5 Trace + 3 adversariale Linsen, Konsens). Die Frage
„besitzt die Such-API-Klasse `search_engine` eine Anatomie (Kette Gattung→Lebewesen→Achsen)?" lautet
**`not_realized`**. Frühere Formulierung „orthogonal" war irreführend → korrekt: **getrennte
Parallel-Hierarchien mit gemeinsamer Wurzel**.

**Ist-Stand (code-belegt):**
- `search_engine<Collection, ConfigPermutation>` (`abi/search_engine.hpp:19-21`) erbt NUR von
  `execution_engine<…>`, trägt KEINE Anatomie/Composition/Achsen; einzige Collection-Links
  `key_t/value_t/binary_key_t` (Z.26-28); reine virtuelle Such-API (lookup/insert/erase/size/empty).
- Die 19 Achsen + die Kette leben im PARALLELEN Baum: `IExecutionEngine → IAnatomyBase →
  SearchAlgorithmAbiAdapter<A>` (`abi_adapter.hpp:118-138`), der die 19 Organe als private Member hält
  (`abi_adapter.hpp:1609-1685`); `A = SearchAlgorithmAnatomy<Composition>`
  (`search_algorithm_anatomy.hpp:31`), `Composition` = 19 `using`-Achsen (`composition_concept.hpp:20-58`).
- KEINE Verbindung zwischen beiden Bäumen außer der Wurzel `execution_engine`/`IExecutionEngine`;
  `search_engine` erscheint in `abi_adapter.hpp` nur im Metapher-Kommentar (Z.9/109). Zusammenführung
  erst zur Laufzeit über Builder/`AnatomyExecutionContext` + `AnatomyModuleLoader`.
- **Namens-Kollision (wichtig):** `search_engine` ist ZWEIERLEI — (a) die *dormant* ABI-Klasse
  `search_engine<>`, (b) ein *lebendiger Namespace* `comdare::cache_engine::search_engine::axis_01_…`
  (Achsen-Topics). Nicht verwechseln.

**Vorbedingung (zuerst klären, NICHT raten):** Ist die KLASSE `search_engine<>` lebend/geplant oder
überholt? Evidenz spricht für **überholt**: (1) keine Instanziierung/Ableitung der Klasse gefunden
(`grep -rn "search_engine<" libs/ tests/` liefert nur Namespace-Treffer); (2)
`identity/prt_art_search_engine_adapter.hpp` ist `deprecated`, „aktueller Adapter =
prt_art_execution_engine_adapter.hpp" (`docs/architecture/19_f6_prtart_migration_plan.md:191`); (3)
Thesis-Intro: „in früheren Entwürfen … ExecutionEngine→SearchEngine→SearchEngineType". → Definitiv via
Git-History/Sessions bestätigen.

**Option A (wahrscheinlich) — Klasse überholt → KEIN Umbau, nur bereinigen:** Falls keine produktiven
Instanzen: `abi/search_engine.hpp` deprecaten bzw. den „Stufe-3/hält-Anatomie"-Kommentar (Z.2-7) tilgen;
in Doku/Thesis die Beziehung als „getrennte Parallel-Hierarchien" führen. Sauberster Weg, kein ABI-Risiko.

**Option B — Klasse SOLL Such-Fassade der Anatomie werden → Kopplung OHNE ABI-Bruch:** NICHT
`search_engine` mit Achsen-Membern aufblähen (verletzt Mess-Pfad-A/B + Observer-Konsolidierung I1).
Stattdessen `SearchAlgorithmAbiAdapter<A>` ZUSÄTZLICH von `search_engine<Collection,ConfigPermutation>`
ableiten (neben `IAnatomyBase`), `Collection`/`ConfigPermutation` aus `A::composition_t` ableiten, die
virtuelle Such-API auf die bestehenden `tier_*`-Ops über `search_organ_`/`container_` mappen. Anatomie
dann von der Such-API via `dynamic_cast<IAnatomyBase*>` auf denselben Adapter erreichbar — kein neuer
Member, kein ABI-Major-Bump (`search_engine` header-only, überquert die `extern "C"`-Factory NICHT).
`static_assert`, dass abgeleitete `key_t/value_t` == `Collection::key_t/value_t`.

**Verbot:** Keine Achsen-Member in `search_engine` selbst (Organe leben genau EINMAL im Adapter — I1).
Kein Quick-Bypass.

**Rückmeldung erbeten:** Welche Option gilt (A/B), bestimmt auch die Thesis-Formulierung in Kap. 4 +
ob das Abstract-Framing „CacheEngine→ExecutionEngine→SearchEngine" angepasst wird.

---

### Quer-Referenzen
- Thesis-Tasks: AP-X1 (Text erledigt), AP-X2 (= dieses Handout), AP-X4 (Layout → TODO-4), AP-X5 (Architektur-Verifikation → TODO-5).
- Frühere Handouts: `20260617-HANDOUT-impl-agent-io-achse-tpie-mehlhorn.md`,
  `20260617-HANDOUT-impl-agent-CE1-funchops-CE2-dataloader.md`.
