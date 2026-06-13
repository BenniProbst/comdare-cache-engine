# MASTERPLAN-SESSION 2026-06-13 — Architektur-Konsolidierung, Aufräumen, saubere Wiederaufnahme

> **Auslöser (User, 2026-06-13):** Diese Session hat an der Architektur gearbeitet, ohne ausreichend in ihr
> verankert zu sein (Flach-Tupel-Achsen statt B+-Baum-Organ-Tausch). User-Direktive:
> 1. **Wirklich ALLE Architektur-Docs lesen — ab dem ERSTEN (Doc 00), nicht erst ab Doc 9.**
> 2. **Den Stand in EIN neues, umfassendes, aktuelles Architektur-Dokument konsolidieren** (Single-Source-of-Truth),
>    damit sicher gearbeitet werden kann.
> 3. **Die ursprünglichen Aufgaben-Referenzen vom Session-Start** einbeziehen + weiter im Blick behalten
>    (CLAUDE.md-Direktive: „die letzten 15 Themen an Session-Dokumenten und alle Masterpläne aus Projekte laden").
> 4. **Alle Aufgaben dieser Session ab der ersten gelten als falsch umgesetzt** und müssen aufgeräumt werden.
> 5. **Erst nach all dem (= Phase a)** wird (b) bearbeitet: die ursprünglichen Aufgaben, EINE pro Session,
>    jede mit maximaler Präzision/Tiefe gegen das gesamte Audit gegengeprüft (1M-Token je Session).
>
> **Leitprinzip (gelernte Lehre):** Nie an der Architektur arbeiten ohne Verankerung in ihr. Achse = Organ;
> Permutation = Organ-Tausch; das lebt im B+-Experiment-Baum/Composition/Anatomie der cache-engine, NIE flach
> im Eval-Tool. ([[feedback_axis_exchange_belongs_in_bplus_tree]], [[feedback_always_use_trees_for_search]],
> [[feedback_never_guess_always_lookup_state_of_art_and_docs]])

---

## PHASE A — Vollständige Architektur-Grundlegung (PFLICHT vor allem anderen)

### A0 — Ursprüngliche Aufgaben-Referenzen der Session laden + festhalten
**Was:** Die CLAUDE.md-Session-Start-Direktive nachholen, die diese Session übersprungen hat:
- Die **letzten 15 Session-Dokumente** (`docs/sessions/`, chronologisch) laden + verstehen.
- **Alle Masterpläne** aus den Projekten laden (`OneDrive/Desktop/Projekte/**` Masterplan-Dateien + die Thesis-
  Masterpläne).
- Das **ursprüngliche Goal/Aufgaben-Set der letzten Session** verbatim sichern: das war die Bias-Bruch-Matrix-
  Mess-/Audit-/Appendix-Mission (`GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md` + die User-Detailanforderungen:
  Ausgabe = Testdaten-Konfig × Tier; 3D-Diagramme z=ns/op je Interface-Funktion; Achsen-Austauschbarkeits-Diffs;
  Appendix immer ALLE Werte; ZIH-Vorlage; relative Pfade; ein Experiment → PDF).
**✓ Kriterium:** Eine Referenz-Liste „Original-Aufgaben + ihre Quell-Dokumente" liegt vor; nichts vom
Session-Start ist verloren.

**A0-ERGEBNIS (2026-06-13 ✅ Enumeration):**
- **Original-Mission (letzte Session) = `GOAL-MESSUNG-AUDIT-APPENDIX-20260612.md`:** Bias-Bruch-Matrix
  Messung → Audit-Abarbeitung → interpretierbarer LaTeX-Appendix. Detail: Ausgabe = Testdaten-Konfig × Tier;
  je Interface-Funktion Verarbeitungsdauer (ns/op) auf z-Achse eines 3D-Diagramms; **Achsen-Austauschbarkeits-
  Belege** (Wechsel EINER Achse → Diff gegen alle anderen Tiere als Tabelle) — **PRÄZISIERUNG (B+-Baum, nicht
  flach!)**; Appendix IMMER alle Werte; TU-/ZIH-diplominf-Vorlage; NUR relative Pfade; EIN Experiment → fertige PDF.
- **Letzte 15 Session-Docs (cache-engine `docs/sessions/`):** 20260613-MASTERPLAN · GOAL-AUTONOM-20260613 ·
  20260613-ARCHITEKTUR-KORREKTUR-achsenaustausch-im-baum · 20260613-autonom-A2a-done · 20260612-uebergabe-m2/L ·
  **GOAL-MESSUNG-AUDIT-APPENDIX-20260612** · 20260611-audit-ergebnisse-synthese · 20260611/10-undolog-uebergaben ·
  20260605-observer-konsolidierung-START · 20260604-observer-konsolidierung + mess-architektur-19-achsen-timer ·
  20260603-queuing-migration-87-container-tier-unterklasse · 20260603-gattungs-kategorienfehler-3-ebenen-modell.
- **Thesis-Masterpläne:** `Diplomarbeit-Root/docs/MASTERPLAN_KONSOLIDIERUNG_TERMINE.md` (K-A…K-J, 10 Phasen,
  Memory `reference_konsolidierungs_masterplan`) + DIESER Masterplan. **(Cluster/Products/Research-Masterpläne
  in `OneDrive/Desktop/Projekte/**` = ANDERE Workstreams, NICHT thesis-relevant.)**
- **Offen in A0 (vor B):** die 15 Session-Docs + `MASTERPLAN_KONSOLIDIERUNG_TERMINE.md` inhaltlich lesen
  (zusammen mit A1; viele sind ohnehin Architektur-/Mess-Docs).

### A1 — ALLE Architektur-Dokumente lesen, ab Doc 00
**Thesis-Basis** `docs/architektur/` (Diplomarbeit-Root, jetzt per Junction in der cache-engine verlinkt):
- 00_INDEX · 01_REV_Historie · 02_aktueller_master_REV7_7 · 03_konzepte_saeule_a · 04_konzepte_saeule_b ·
  05_uml_klassen · 06_er_modell · 07_cross_reference · 08_drawio_export
- 09_taxonomien ✅ · 10_schichten_modell_M ✅ · 11_axes_vs_strategies_disambiguation ✅ ·
  11_konzept_achsen_extension_visitor_pattern ◐(½) · 12_queuing_topic_achsen_eigenschaften ⬜ ·
  13_paper_legacy_code_architektur ⬜ · 14_achsen_komposition_organ_metapher ◐(§0–§20 von §53)

**Cache-engine** `docs/architecture/` (15–33 + benannte):
- 15_isa · 16_imc · 17_paper_kartografie · 18_achsen_algorithmus_paper_code_map ·
  19_f6_prtart_migration · 20_plugin_controller · 21_plugin_slot_merge · 22_f15_messpipeline_und_such_bibliothek ·
  23_namespace_migration (+23a) · **24_messmodell_korrektur_zwei_dimensionen** · 25_static_shared / 25_submodule ·
  **26_permutations_bplus_baum_und_inverse_signatur** · **27_experiment_baum_registry_bindung_4_bruecken** ·
  **28_vollstaendigkeits-kartographie** · **29_experiment_baum_generik_und_composition_driver** ·
  **30_audit_achsen_delegation_pflichtachsen** (Autorität des 3-Ebenen-Modells) · **31_observer_interface_konsolidierung_i1** ·
  32_lastprofil_katalog · **33_undolog_memento_und_mess_resume** · **abhaengigkeitskette_lebewesen_pruefdock_abi_konvergenz** ·
  **messarchitektur_design_observer_handle_no_dynamic_cast** · messarchitektur_klarstellungen · messarchitektur_v5_design ·
  messarchitektur_v5_drei_profile · messarchitektur_v5_entscheidungen · messarchitektur_v5_i8
**(fett = kritisch für Achsen/Baum/Messung; zuerst, aber ALLE lesen.)**
**✓ Kriterium:** Jedes Dokument vollständig gelesen (kein Überfliegen); SUPERSEDED-Banner + Widersprüche notiert
(für B2). Lese-Fortschritt in dieser Liste abgehakt.

### A2 — Pflicht-Code-Pre-Read (die Architektur IM Code)
`builder/experiment_tree/experiment_tree.hpp` ✅ · `…/registry_to_axis_levels.hpp` ⬜ · `…/profile_to_tree.hpp` ⬜ ·
`…/composition_registry.hpp` ⬜ · `anatomy/composition_concept.hpp` ✅ · `anatomy/composition_factory.hpp` ◐ ·
`anatomy/search_algorithm_anatomy.hpp` ⬜ · `anatomy/abi_adapter.hpp` ◐ · `anatomy/observable_tier.hpp` ⬜ ·
`builder/experiment_tree/perm_runner.hpp` ◐ · `builder/experiment_tree/cache_engine_builder_iterator.hpp` ⬜ ·
`src/permutations/permutation_engine.hpp` ⬜ · die Gattungs-/Genus-Bindung (`genus_binding_traits.hpp`).
**✓ Kriterium:** Der IST-Code-Stand (Baum, Anatomie, Composition, Permutation, Mess-Pfad, ABI) ist verstanden
und gegen die Docs abgeglichen (Doku-↔-Code-Drift notiert).

### A3 — Die zwei teuren Audits als Architektur-Soll-Abgleich einbeziehen
Mess-Audit (57 Befunde) + Pattern-Audit (28 Befunde) = 85 bestätigt, K1–K10 + 8 Meta-Lehren
(`audit-sicherung-20260612/ERKENNTNISSE.md`, `20260611-audit-ergebnisse-synthese.md`, die JSON-Endergebnisse;
vollständiges Manifest in `GOAL-AUTONOM-ABARBEITUNG-20260613.md` §2.5.6).
**✓ Kriterium:** Jeder Audit-Kern-Defekt ist einem Architektur-Konzept (Achse/Organ/Baum/Mess-Modell/Observer)
zugeordnet — Grundlage der Konsolidierung B.

---

## PHASE B — Konsolidierung in EIN umfassendes aktuelles Architektur-Dokument

### B1 — Neues Master-Architektur-Dokument (Single-Source-of-Truth)
**Was:** Ein neues Dokument, das den IST-Stand über ALLE Docs (00–33) + Code-Realität (A2) + Audit (A3)
konsolidiert. Vorschlag Ort/Name: `docs/architektur/15_KONSOLIDIERTER_MASTER_IST_STAND.md` (Thesis-Basis,
durchgehende Nummer nach 14) — Ort beim Erstellen final mit User bestätigen.
**Pflicht-Inhalt (mind.):**
- **3-Ebenen-Modell** (Doc 30 §8.0 / Doc 14 §0): Gattung = Interface/Prüf-Dock · Tier-Unterklasse = fester
  Achsen-Satz (SearchAlgorithm/Set/Sequence/Adapter/View) · Achsen = Organe (keine optional, alle uniform getrieben).
- **Organ-Metapher + Permutation = Organ-Tausch** (Doc 14 §1/§9) + Reference-/Composition-Templates (§11/§13).
- **B+-Experiment-Baum** (Doc 26/27/29 + `experiment_tree.hpp`): Achsen = Baum-Ebenen, Pfad = binary_id = Tier,
  Mixed-Radix-Bijektion (`StaticBinaryView`), Achsen-Austausch = Ziffernwechsel; lazy, nie ∏-materialisiert.
- **4-Subsystem-Modell** (Doc 10): messung_driver / CacheEngineBuilder / CacheEngine / Prüfling (bidirektional).
- **3-Stufen-Prüfung** (Doc 11/14 §18): CE-only / Prüfling-einzeln (ERSETZT-mit-Fallback) / Full-Join.
  **Prüfling ≠ Prüflings-Binary.**
- **Verantwortlichkeits-Trennung** (Doc 14 §17): PermutationEngine / SearchAlgorithmAnatomy (+ABI-Observer) /
  CacheEngineBuilder.
- **Mess-Modell 2 Dimensionen** (Doc 24) + **Observer-Konsolidierung I1** (Doc 31) + **Memento/Resume** (Doc 33) +
  **no-dynamic-cast / ABI-Konvergenz** (abhaengigkeitskette, messarchitektur_design_observer).
- **Pflicht-Achsen-Satz** (Doc 30 §8.0: 19 Achsen inkl. queuing q1/q2) + Goldstandard-Achsen-Aufbau.
**✓ Kriterium:** Das Doc steht für sich allein als IST-SOLL-Referenz; ein neuer Bearbeiter könnte daraus korrekt
arbeiten, ohne alle 33 Quell-Docs einzeln zu lesen.

### B2 — Widersprüche / SUPERSEDED auflösen
Viele Docs tragen SUPERSEDED-/Korrektur-Banner (z.B. 09/10/11 „2026-05-31 überholt", 14 §0-Korrektur,
CLAUDE.md-Block teils historisch). B1 hält je strittigem Punkt fest, **was aktuell GILT** + Quell-Doc + Datum.
**✓ Kriterium:** Keine offene Doppeldeutigkeit (Gattung-Begriff, 110/120/130-Merge revidiert, opn-3/4-Status,
Adapter-Gattung, etc.) bleibt unaufgelöst.

### B3 — Original-Aufgaben (A0) in den konsolidierten Stand einordnen
Die Bias-Matrix-Mess-/Audit-/Appendix-Mission gegen das konsolidierte Modell re-formulieren: WO im
3-Ebenen-/Baum-/Mess-Modell jede Teilaufgabe korrekt verankert ist (z.B. Achsen-Austauschbarkeit = im Baum,
nicht im Eval-Tool).
**✓ Kriterium:** Jede Original-Teilaufgabe hat einen architektur-konformen Soll-Ort.

### B4 — Goal-Dokument grundlegend überarbeiten (auf den Masterplan + Steuerung umstellen) [User-Ergänzung]
Das am Session-Anfang erstellte `GOAL-AUTONOM-ABARBEITUNG-20260613.md` wird grundlegend re-grounded: die alte
M/L-Ausführungs-Reihenfolge (§2, L1–L8-Flach-Pipeline = Architektur-Fehler) wird verworfen; das Goal verweist als
**schlanke Steuerung auf DIESEN Masterplan** (Phasen A→F) + den konsolidierten Architektur-Stand (B1); der
Audit-Backlog §2.5 + das Rohdaten-Manifest §2.5.6 bleiben als Referenz (Phase D). So ist das /goal sauber setzbar
und der Masterplan autonom abarbeitbar.
*(Eine schlanke Sofort-Fassung dieses Verweises wurde bereits 2026-06-13 spät als Kopf-Banner ins Goal-Doc
eingetragen, damit das /goal SOFORT setzbar ist; B4 ist die gründliche Re-Grounding nach Existenz von B1 —
inkl. Entfernen/Archivieren der überholten §2-Inhalte.)*
**✓ Kriterium:** Goal-Doc trägt keine überholte L-Pipeline-Steuerung mehr; verweist sauber auf Masterplan + B1;
das /goal ist gesetzt und treibt die autonome Abarbeitung.

---

## PHASE C — Aufräumen ALLER falsch umgesetzten Session-Änderungen

> Grundsatz: NICHTS verteidigen. Jede dieser-Session-Änderung wird gegen den konsolidierten Stand (B) bewertet
> und revertet/revidiert/behalten. Aufräumen reversibel (Tag vor destruktiven Reverts).

### C1 — Inventar aller dieser-Session-Commits + Disposition
| Commit | Repo | Inhalt | Disposition (vorläufig, B entscheidet endgültig) |
|---|---|---|---|
| `1dd509b`,`c93536b`,`92a911f` | ce | Goal-Charter + Audit-Backlog + Rohdaten-Manifest | Re-eval: behalten als Historie ODER in B1 überführen |
| `4a64bc8` | ce | **A2a/K3** restore_statistics 17 Wrapper + Verif-TU | **Re-eval gegen B** (Organ-Observer/Memento-konform? ja-erwartet, aber prüfen) |
| `cbb7d6b` | ce | verifizierter Arbeitsplan (Mapping-Workflow) | Re-eval: enthält L-Flach-Annahmen → revidieren |
| `3c296df` | ce | Architektur-Korrektur-Doc (Achsen-Austausch im Baum) | behalten (korrekt) ODER in B1 überführen |
| `915297f` | ce | Basisarchitektur-Junction + gitignore | behalten (deine Anweisung) |
| `c610354` | **super** | **L1** csv_to_latex Flach-Achsen-Tupel | **REVERT** |
| `1ec5cfd` | **super** | **L2** Stufe-05 3D-Surfaces | **REVERT** (oder Tier-Ordnung auf Baum-Index, nach B) |
| Submodul-Bumps | super | Pointer | mit den ce-Reverts synchronisieren |
**✓ Kriterium:** Tabelle vollständig (auch ich-übersehene Commits via `git log --since` beider Repos); je Zeile
eine begründete Disposition.

### C2 — L1 + L2 zurücknehmen (git revert), Submodul-Pointer synchronisieren. ✓ Working Tree + Historie sauber.
### C3 — A2a/K3 re-evaluieren gegen B1: behalten (wenn Organ-Observer/Memento-konform) oder revidieren. ✓ Entscheid belegt.
### C4 — Goal-/Audit-/Korrektur-Docs re-evaluieren: in B1 überführen oder als Historie kennzeichnen. ✓ Keine widersprüchliche „autoritative" Doppelung.
### C5 — M2-Mess-Lauf-Disposition: läuft cowmem-v1 (un-gefixte Mess-Architektur). Entscheiden: pausieren bis nach
Audit-Wellen ODER als Interim weiterlaufen. **User-Entscheid.** ✓ Klarer Status.
### C6 — Memory bereinigen: die in dieser Session angelegten/aktualisierten Memories
(`project_biasmatrix_fullrun_and_nas`, `feedback_axis_exchange_belongs_in_bplus_tree`,
`feedback_lehrbuch_design_patterns_only_zero_cost_metaprog`) gegen B1 prüfen; falsche/überholte korrigieren.
✓ Memory ↔ konsolidierter Stand konsistent.

---

## PHASE D — Die zwei teuren Code-Audits vollständig durcharbeiten (PFLICHT, vor E)

> **User-Ergänzung 2026-06-13:** Die zwei sündhaft teuren Audits der letzten Session (Mess-Audit 57 +
> Pattern-Audit 28 = **85 bestätigt, >300 €**, K1–K10 + 8 Meta-Lehren) werden hier NICHT nur referenziert (A3),
> sondern **vollständig, als Pflicht, Befund für Befund durchgearbeitet** — denn das (1) **erweitert das
> konsolidierte Master-Architektur-Doc (B) umfassend** (jeder Befund = ein IST-Defekt + SOLL-Korrektur am
> Architektur-Modell) und (2) **bestimmt die Aufgaben-Auslegung von Phase E maßgeblich** (viele E-Aufgaben SIND
> Audit-Fix-Wellen). Quellen: `messaudit-endergebnis.json` / `patternaudit-ergebnis.json` / `ERKENNTNISSE.md` /
> `20260611-audit-ergebnisse-synthese.md` (Manifest in `GOAL-AUTONOM-ABARBEITUNG-20260613.md` §2.5.6).

### D1 — Jeden der 85 Befunde gegen den konsolidierten Stand (B) durcharbeiten
Pro Befund: claim ⇄ aktueller Code ⇄ konsolidiertes Architektur-Soll (B) → **Disposition** (real / durch
zwischenzeitliche Umbauten gegenstandslos / User-Entscheid) + **architektonische Implikation** (was ändert sich am
Soll-Modell?) + **Aufgaben-Implikation** (welche E-Aufgabe folgt?). Keine Abkürzung, jeder Befund einzeln.
**✓ Kriterium:** Tabelle aller 85 Befunde mit Disposition + Architektur-Bezug + E-Aufgaben-Bezug.

### D2 — Konsolidiertes Architektur-Doc (B) um die Audit-Erkenntnisse erweitern
Die K1–K10-Kern-Defekte + die 8 Meta-Lehren + die SOLL-Korrekturen als eigenen Abschnitt ins Master-Doc (B1)
einarbeiten — so trägt das Doc dauerhaft auch die teuer erkauften Defekt-/Korrektur-Einsichten, nicht nur den
IST-Stand.
**✓ Kriterium:** B-Doc um „Audit-IST-Defekte + SOLL-Korrekturen" erweitert; jede Meta-Lehre als bindende Regel.

### D3 — Aufgaben-Auslegung für Phase E aus den Audits ableiten
Aus den Befunden + Fix-Wellen (Synthese §4) die architektur-konforme E-Aufgaben-Liste mitformen: welche Fix-Welle
= welche E-Aufgabe, in welcher Reihenfolge, mit welchem Soll-Ort im 3-Ebenen-/Baum-Modell. Ohne D ist die
E-Auslegung unvollständig.
**✓ Kriterium:** Audit-fundierter E-Aufgaben-Entwurf liegt vor (Input für E1).

---

## PHASE E — (= deine (b)) Saubere Wiederaufnahme, EINE Aufgabe pro Session

### E1 — Original-Mission gegen B (inkl. D-Erweiterung) neu planen
Die Bias-Matrix-Mess-/Audit-/Appendix-Mission (A0/B3) + den audit-fundierten Entwurf (D3) in eine geordnete,
architektur-konforme Aufgaben-Liste gießen — jede Aufgabe explizit gegen das konsolidierte Modell + die 85
Audit-Befunde verankert.
**✓ Kriterium:** Aufgaben-Liste, jede mit Soll-Ort im 3-Ebenen-/Baum-Modell + Audit-Bezug.

### E2 — Abarbeitung: EINE Aufgabe pro 1M-Session, maximale Präzision/Tiefe
Pro Aufgabe: gegen das Voll-Audit gegenprüfen, gegen B1 (inkl. D) verankern, literal verifizieren, committen.
Keine Parallel-Hast, kein Flach-Shortcut. (Reihenfolge wird in E1 festgelegt.)
**✓ Kriterium:** Je Aufgabe ein nachvollziehbarer, architektur-konformer, verifizierter Abschluss.

---

## PHASE F — Persistenz / Disziplin (laufend)
- **F1:** Dieser Masterplan ist die Session-Steuerung; bei jedem Session-Start zuerst hierher + B1.
- **F2:** Memory-Pointer auf diesen Masterplan + B1.
- **F3:** 3-Repo-Disziplin (ce/super committen+pushen+bump) NUR für architektur-konforme, gegen B geprüfte Änderungen.

---

## Sofort-Status (Stand 2026-06-13, Erstellung dieses Masterplans)
- M2-cowmem-v1-Voll-Lauf läuft im Hintergrund (Disposition offen, C5).
- Gelesen: Docs 09/10/11(axes)/11(extension,½)/14(organ,§0–20) + `experiment_tree.hpp`/`composition_concept`.
- Offen: der Großteil von Phase A (Docs 00–08, 12, 13, Rest 11/14, alle cache-engine 15–33 + Code-Pre-Read).
- **Nächster Schritt nach User-Freigabe:** Phase A0 + A1 vollständig durchziehen (alle Docs ab 00), dann B1.
