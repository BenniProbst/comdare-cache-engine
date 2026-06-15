# Session 2026-06-03 — Gattungs-Kategorienfehler, 3-Ebenen-Modell + Delegations-Fix (Stand-Übergabe)

**Zweck:** Elaborate Übergabe des Stands nach einer langen Session, die mit „README + Mess-Anleitung" begann und über
einen vom User aufgedeckten **Architektur-Kategorienfehler** zu einem präzisierten **3-Ebenen-Modell** + flächendeckender
Doku-Konsistenz führte. Der eigentliche **Code-Umbau** (queuing→SA-Achse, Container-Lebewesen-Unterklasse) steht als nächstes an.

---

## 1. Auftrags-Bogen dieser Session (chronologisch)

1. **README + Mess-Anleitung** (cache-engine + Diplomarbeit-Sicht) → erledigt (Abschnitt 5).
2. **Echter E2E-Build + Messung** (8 SearchAlgorithm-DLL-Lebewesen, echte Werte) → erledigt (Abschnitt 5).
3. **GOAL-K78-Erweiterung** (Cluster-Wünsche CE-D1…D5) → erledigt (Abschnitt 5).
4. **User-Einwand (KRITISCH):** „Wie kann ein Modulbaustein wie queuing eine Gattung sein?" → Kategorienfehler-Audit.
5. **5 Modell-Präzisierungen** des Users (s. Abschnitt 3) → 3-Ebenen-Modell verankert.
6. **Flächendeckende Doku-Konsistenz** über alle Architektur-Docs ≥10 → erledigt (Abschnitt 4).
7. **Nächster Schritt:** Code-Umbau (#87/#88/#89) → OFFEN (Abschnitt 6).

---

## 2. Der Kategorienfehler (vom User aufgedeckt, bestätigt)

**Befund (Audit `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md` §8):** `AnatomyGenus::Adapter` („Container"-
Gattung) war im IST-Zustand **eine Hülle um die queuing-Achse** (q1 `FIFOQueueBuffer` hat bereits die volle Container-API;
`ContainerAnatomy::put/get/size/clear` = 1:1-Pass-Through). Eine **Achse** (queuing) war fälschlich zur **Gattung** erhoben.

**Ursprung:** Doc 27 §0.1 (2026-06-02) re-interpretierte fälschlich „q1/q2 passen nicht in die 17" → „also eigene
Gattung" (unter Zweckentfremdung von §32 Cross-Genus). **Doku 14 §7 („q1/q2 = Organe") war KORREKT** → rehabilitiert.

**Mein erster Fix-Plan war ebenfalls falsch** („queuing als Pflicht-Slot jedes Suchalgorithmus + Adapter-Gattung löschen")
— verlagerte den Fehler nur. Nach User-Korrektur revidiert.

---

## 3. Das korrekte 3-Ebenen-Modell (vom User präzisiert, autoritativ in §8.0)

Verbatim verankert in **Doc 24 §8.8** („Prüf-Dock je Gattung — Search Algorithm / Container / Graphen") + **§8.6**
(„ABI-Interface der API der Gattung") + **Doku 14 §25** („durch Unterklassen spezifiziert"):

| Ebene | Begriff | Definition | Beispiele |
|-------|---------|------------|-----------|
| **1** | **Gattung = Außen-Interface** | was die Außenwelt sieht (= ein Prüf-Dock) | SearchAlgorithm / Container / Graph |
| **2** | **Lebewesen-Unterklasse** | unter dem Interface, **fester Achsen-Satz** | SearchAlgorithm-Lebewesen (std::map-ähnl.); Set/Sequence/Adapter/View = Container-Lebewesen-Unterklassen |
| **3** | **Achsen** | Organe der Lebewesen-Unterklasse; **alle Pflicht, in jedem Lebewesen-Binary uniform getrieben** | 17 (→19) Achsen; je Achse austauschbarer Algorithmus |

**Die 5 User-Präzisierungen (in Reihenfolge):**
1. queuing ist Achse, keine Gattung (kein doppeltes Topic).
2. Keine Achse thematisch / kein Topic konzeptionell doppelt.
3. **Achsen sind NIE optional** — Interfaces ALLER Achsen werden in JEDEM Lebewesen-Binary tatsächlich + uniform getrieben;
   ein nicht-pufferndes Lebewesen wählt den konkreten Durchreich-Algorithmus `NoBuffer`/`NoFlush` (analog NonePrefetch/NoMigration).
4. **Gattung = Interface**, Lebewesen-Unterklasse = fester Achsen-Satz darunter; **aktuell nur EINE Lebewesen-Unterklasse** gebaut
   (SearchAlgorithm), alle Achsen Pflicht.
5. Diese Gliederung war als Schärfung bereits verteilt definiert (Doc 24 §8.6/§8.8 + Doku 14 §25) → gefunden + verankert.

**Konsequenz queuing:** queuing q1/q2 = **Pflicht-Achsen der (aktuellen SearchAlgorithm-)Lebewesen-Unterklasse**. Der
„Adapter = queuing-Gattung"-Fehler war doppelt falsch: (i) queuing keine Gattung; (ii) std::queue/stack wäre eine
**Container-Lebewesen-Unterklasse** (axis_inner + ordering), nicht queuing.

---

## 4. Doku-Konsistenz hergestellt (2 Agenten-Sweeps, verifiziert)

- **Sweep A (queuing-Kategorienfehler, 20 Agenten, 1/Doc ≥10):** 19 clean, **Doc 24** korrigiert (+ vorab 27/28/29 manuell).
- **Sweep B (3-Ebenen-Konsistenz, 26 Agenten):** 12 Docs angeglichen (15/19/20/21/24/27/28/29 + abhängigkeitskette +
  mess_observer + Doku 12/14), Rest clean. „Gattung"(=Achsen-Satz)→Lebewesen-Unterklasse; „5 Gattungen"/„Gattung A–F"→
  Lebewesen-Unterklassen; „optionale Achse"→Durchreich-Algorithmus; „Gattung=Interface/Prüf-Dock" belassen.
- **Doku 14 (Autorität):** verbatim §25-Direktive INTAKT; nur 7 datierte Schärfungs-Notizen additiv (never-delete).
- **Verifiziert:** alle angeglichenen Docs tragen datierte Marker; keine freistehende „Gattung=fester-Achsen-Satz"-/
  „optionale Achse"-Inkonsistenz (Rest-Treffer nur in KORREKTUR-Bannern als zitierter Fehler).

---

## 5. Gültiger Code-/Liefer-Fortschritt dieser Session (vom Kategorienfehler UNABHÄNGIG, bleibt gültig)

- **Storage-Delegation (Punkt 1, committet bab6406):** `container_` im `SearchAlgorithmAbiAdapter` von unbounded
  `ComposedStore` auf node-wirksame **`NodeChunkedStore<N,L,A>`** umgestellt → `alloc_cnt = ceil(n/node_cap)` im echten
  perm_runner-Mess-Pfad node-abhängig (Node4=250 … Node256=4 @ n=1000; vorher konstant 18). Standalone-Beleg
  `test_node_delegation_proof` PROOF_OK. **Betrifft search↔node/layout/allocator, NICHT queuing/Gattung — bleibt gültig.**
- **Thesis-Mess-Anhang:** 8 SearchAlgorithm-DLL-Lebewesen gebaut + 24 echte result_ingest-Zeilen (`docs/sessions/
  20260603-l-meas-thesis-searchalgo-tiere.md`; Befund-B-Korrektur dort). Mess-Anleitung: `README.md` „Messwerte erzeugen" +
  Superprojekt `docs/anleitung_messwerte_erzeugen.md`.
- **GOAL-K78 (Cluster, Infra-Agent):** §8 CE-D1…D5 in `cluster_development` + `Projekte/Cluster` (beide synchron).

---

## 6. OFFEN — der Code-Umbau (nächster Schritt, User-bestätigt „Code umbauen")

Auf der jetzt konsistenten Modell-Grundlage:

- **#88 (zuerst):** queuing q1/q2 als **Pflicht-Achsen der SearchAlgorithm-Lebewesen-Unterklasse** — `AdHocComposition<17>→<19>`
  (+ `composition_concept`/`organ_count`/`ObserverAggregate`/`all_axes_umbrella`/`axis_path_serialization`/
  `genus_binding_traits`/`composition_registry`/`adhoc_emitter` + ALLE 16 Lebewesen-Quellen explizit `NoBuffer`/`NoFlush`).
  Atomare ~29-Datei-Migration. queuing-Interface in jedem Lebewesen uniform getrieben.
- **#87:** `ContainerAnatomy`/`ContainerComposition` als **echte Container-Lebewesen-Unterklasse** neu definieren (`axis_inner` +
  ordering FIFO/LIFO/Priority + delegierte Achsen); die queuing-Hülle verwerfen. Container nutzt queuing NICHT.
- **#89:** Verifikation (Mess-Pfad + cmake build grün; `test_br3_obs22` 22=19+3, `test_genus_binding`, Container-Tests) +
  Code-Kommentar-Korrekturen (`container_anatomy.hpp`/`genus_binding_traits.hpp`/`axis_observer_classification.hpp`).
- **Separat (groß):** codebase-weite Umbenennung `AnatomyGenus`→Lebewesen-Unterklassen-Naming (Enum + Code) — eigene Migration.
- **Rücksetzpunkt:** Git-Tag `pre-delegation-sweep-20260603`.

---

## 7. Commit-Trail (cache-engine main, diese Session)

`808a1f9`/`0628e21` (K78 §8 + Aufräumen) · `8fe7583` (Thesis-Mess-Tiere) · `bab6406` (NodeChunkedStore node-wirksam) ·
`d386409`/`97f86dc`/`6c6f514`/`bf930c4`/`3b2bd19` (Audit-30 §8 Kategorienfehler + 3-Ebenen-Modell, iterativ) ·
`a35c82c` (3-Ebenen-Konsistenz-Sweep 12 Docs). Superprojekt-Pointer jeweils gebumpt.

---

## 8. Lektionen

- **Banner-am-Kopf ≠ Revision** (User-Rüge): Falschaussagen müssen IM GANZEN Dokument korrigiert werden, nicht nur annotiert
  am Anfang. → 2 vollständige Agenten-Sweeps.
- **Konsistenz bei Modell-Änderung:** jede Modell-Präzisierung muss ERNEUT über ALLE Docs gezogen werden (User-Frage
  „alle 17 Docs konsistent?") — sonst driften korrigierte + neue Aussagen auseinander.
- **Code-Prämissen kritisch prüfen:** Ich übernahm „Adapter = queuing-Gattung" unkritisch aus Code + erstem Audit-Agenten.
  Der User-Einwand deckte den Kategorienfehler auf. Bei Architektur-Aussagen IMMER gegen die User-Definition (Doku 14 §25
  + Doc 24 §8.8) prüfen.
- **Achsen-Invariante:** ALLE Achsen-Interfaces in JEDEM Lebewesen-Binary uniform getrieben, keine optional — das war auch der
  Kern des Punkt-1-Fixes (Speicher-Achsen tatsächlich treiben statt Monolith-Bypass).
