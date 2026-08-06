# UML-Klassenatlas cache-engine — IST-Stand 2026-08-05 (Nachfolger von 05_uml_klassen.md)

**Stand:** ce `98eea2e7` (Gate 400; unmittelbar vor der S5-05q-Landung `b09ebb8a`/Gate 402 erhoben — die Q1/Q2-Organe sind im Atlas im Vor-Scrub-Stand abgebildet, die Struktur ist identisch)
**Seite:** [`05b_uml_klassenatlas_20260805.html`](05b_uml_klassenatlas_20260805.html) (self-contained, 1,5 MB)
**Gerenderte Fassung** (Mermaid-Diagramme live, Filter, Themes): https://claude.ai/code/artifact/61235adf-e3f9-4000-9e55-4bf4d85ee630 (privates Claude-Artefakt des Owners; Repo-HTML ist die kanonische Kopie)

---

## Korrektur-Vermerk (05.08.2026, Owner-Review — REV 2)

Der Owner-Review der Erstausgabe ergab drei berechtigte Befunde; die HTML-Fassung wurde in Revision 2 (gleiche Datei, git-Historie traegt die Erstausgabe) korrigiert:

1. **Gattung/Genus-Hierarchie war unterschlagen:** Die Erstausgabe praesentierte die CRTP-Basis `SearchAlgoBase` prominent, ohne die Ebene-1-Gattung. Korrekt (E-24 C7-1, `anatomy_base.hpp:49-58,86-97`): **Map** ist die abstrakte Gattung (K→V-Interface) mit den geforderten Interfaces; **SearchAlgorithm ist das Genus IN Map** und ERBT den Gattungs-Kern; Set/Sequence/Adapter/View sind Genera der Gattung Container (mit ITier-/ITierV2-ABI-Interfaces); Graph ist Stub. Das Hierarchie-Diagramm traegt jetzt diese Wurzel.
2. **SearchAlgoCores korrekt verortet:** Die `*SearchAlgoCore`-Klassen sind die **Organ-Implementierungen der Achse `search_algo`** — der ersten der **18** Organ-Haupt-Achsen (`kOrganAxisCount = 18`, `abi/anatomy_version_stamp.hpp:39`; die Erstausgabe sagte faelschlich 19 — `telemetry`/`isa` sind System-Achsen, keine Organ-Haupt-Achsen). `SearchAlgoBase` ist die CRTP-Basis der Organe **dieser einen Achse**, nicht die Systemwurzel.
3. **Layer-Gliederung Planer/CEB/Tier-Binary ergaenzt:** Navigation und Sektions-Reihenfolge folgen jetzt dem Schichten-Modell **L0 Planer** (`profile_facade/planner`, `experiment_tree`) → **L1 CEB** (`builder/`: Prüf-Docks je Genus, bestandslog, commands, transport; Hinweis: `CacheEngineBuilder` existiert nicht als benannte C++-Klasse, CEB ist die builder/-Schicht samt App) → **L2 Tier-Binary-ABI-Grenze** (`include/cache_engine/abi/`, `SearchAlgorithmAbiAdapter`, DLL-Naht) → **L3 Anatomie** (Gattung/Genus/Komposition) → **L4 Achsen-Organe** (18 Achsen je Organ-Kategorie = die 5 kOrganGruppen, Doppelwurzel axes/ + topics/ je Achse ausgewiesen) → **L5 Kern/Basis**. Neue 18-Achsen-Tabelle mit Organ-Kategorie, Wurzeln und Sektions-Links.

## Umfang

- **1918 Typ-Definitionen** (class/struct/enum/concept, inkl. nested Typen und Template-Spezialisierungen) aus **1211 Headern** unter `libs/cache_engine` — Tests ausgenommen.
- Je Definition: vollstaendiger `template<>`-Kopf inkl. requires, Basen mit CRTP-Kennung, Typaliase, **alle Methodensignaturen vollstaendig** (Sichtbarkeit +/-/#, static/constexpr/virtual, Rueckgabetyp, Parameter mit Typen und Namen, const/noexcept/override/final), Fundstelle `datei:zeile`, Zweck-Satz.
- **92 Mermaid-Beziehungsdiagramme** (Vererbung `<|--`, CRTP `<|..`) ueber **34 Subsystem-Sektionen**: Achsen (axes/), Topics-Doppelwurzel, include/-API, Builder/Experiment-Tree, Anatomy/ABI-Flaeche, Fassaden/Kern.
- Kopf-Sektion: A1–A3-Anatomie-Kern (AdHocComposition → SearchAlgorithmAnatomy → SearchAlgorithmAbiAdapter mit IAnatomyBase/IMeasurableWorkload/IObservableTier; GenusBindingTraits → PermutationEngine; AllocatorStrategyBase-CRTP-Kette), E4→E1-Maschinerie-Fluss, Stereotypen-Legende («CRTP», «template», «concept», «facade», «adapter», «POD»).

## Erhebungsmethode (Nachvollziehbarkeit)

1. 34 parallele Extraktions-Shards (Workflow, read-only am Stand `98eea2e7`), je Shard JSON-Inventar + HTML-Fragment.
2. Unabhaengiger grep-Census je Shard; Befunde: Shard „lookup Teil 1" per Regex-Abkuerzung unvollstaendig (58/114) → Volllektuere-Reparatur mit exakt aufgehendem Census (81+33; nested Node/Slot-Typen; CRTP-Kette `*SearchAlgoCore<Alloc,Self>` → `SearchAlgoBase<Self>` → `OrganAxis<Self>` am Text verifiziert); 19 verdichtete Methodenlisten („+N weitere") auf volle Signaturen expandiert; 6 vergessene Result-structs (Sektion bestandslog/pruef_dock) nachgezogen; Karten-Neubau deterministisch per jq aus dem JSON.
3. Struktur-Sanity: 1918 Karten == JSON-Inventar (abzueglich dokumentierter Pseudo-Eintraege), `<article>`-Balance 0, alle 92 Mermaid-Bloecke mit gueltigem Kopf.

## Bewusste Grenzen

- Funktionslokale `struct`-Definitionen (in Methodenkoerpern) und freie Funktionen sind keine Karten (im jeweiligen Klassen-Zweck vermerkt).
- `topics/traversal` Teil 1 besteht real nur aus Weiterleitungs-Headern (0 Typ-Definitionen — Census-bestaetigt, kein Extraktionsfehler).
- Mermaid-Diagramme zeigen Beziehungen (Vererbung/CRTP), keine Methoden — die vollstaendigen Methoden stehen in den Klassenkarten darunter.
- Im Repo-HTML werden die Mermaid-Bloecke als lesbarer `classDiagram`-Quelltext angezeigt (kein eingebettetes JS); die gerenderte Fassung liegt im o.g. Artefakt.

## Aktualisierung

Neuerhebung nach groesseren Struktur-Wellen: Extraktions-Workflow erneut ueber `libs/cache_engine` fahren (Shard-Plan und Karten-jq siehe Session 05.08.2026, Ledger-Nachtrag vormittag-2), neue HTML als `05c_…<datum>.html` daneben ablegen — dieses Dokument nie loeschen, nur Banner (Doku-Doktrin).

## REV-3-Vermerk (05.08.2026, Kette-Sektion nach Vertragsketten-Audit)

Neue Sektion **"Die Vertragskette: Planer -> CEB -> Tier-Binary (SOLL/IST nach Audit 05.08.)"** direkt nach der Architektur-Uebersicht: EIN Kette-Flussdiagramm (SOLL/IST-Doppelmarkierung an Planer [R-G1] und CEB-Emission [R-G3], DEPRECATED-Straenge gestrichelt), 9 Detail-Bloecke (Planer-CLI-Grammatik, Resolve-Doppelpfad, CEB-Emission, Dock-LIVE-Draht COMDARE_GOLDEN_N_*, Rueck-Kanal, CEB-intern 4096/ram_spool, Pruef-Dock, Rueckschrieb, Replay-Schluessel-Schichtung), SOLL/IST-Verdikts-Tabelle aller 5 Kettenglieder (Regressionen rot, bewusst-deferred grau) und die CEB==CacheEngineBuilder-Korrektur (Owner 05.08.; apps/cache_engine_builder existiert als Binary, ist aber der Legacy-REV-7.6-Orchestrator, nicht die Soll-CEB — heutige CEB-Rolle traegt der comdare-messung-driver). Quellen: Audit-Backup docs/sessions/backups/20260805-planer-ceb-kette-audit/ (4 Berichte); Ledger-Nachtrag vormittag-4.

## REV-4-Vermerk (05.08.2026, Klammer + Binary-Topologie + Konnektivitaet; NEUE Artefakt-URL)

REV 4 vereint drei Owner-Reviews: (1) **Binary-Topologie-Uebersicht** (Runde 3): Binaries als Container ihrer Layer (CEB-Bauzeit-Kapselung explizit), Heuristik als OPTIONALE EIGENE Binary am Pruef-Dock (Factory-Facade-Adapter, organ-frei), 6 kanonisch benannte Vertrags-Kanten (Experiment-Dock/Pruef-Dock/Arbeits-Dock/Hybrid-Pruef-Docks/Rueckschrieb-nicht-uebers-Dock). (2) **Achsen-Arten-Klammer-Sektion** (#klammer): Mess=DREISTUFIG dehnbar (Planer-RT -> CEB-CT inkl. Pruefdock-Konfig -> [Hybrid-CT] -> Tier-CT), System=ZWEISTUFIG dehnbar, Organ=ZWEISTUFIG hybrid-unberuehrt; Stufen-GESETZ + Hybrid-Vererbung (Ledger mittag-9/-10, Owner-abgenommen mittag-11); drei Klammer-Tabellen mit belegten Zahlen (Organ 18 Haupt/83 Bausteine/~75 Sub-Achsen-Header; System 3 Glieder/11 sub_axis; Mess 16 Kategorien/3 Kollektoren/6 DynDims) und ehrlichen Planungs-Stand-Marken. (3) **L4-Diagramm-Neugenerierung**: alle 21 Achsen-/Topic-Sektionen mit namespace-Gruppierung je Achse + Zugehoerigkeits-Ankern — 0 verbindungslose Knoten (vorher bis zu 117 je Sektion; Verifikation konnektivitaet.txt im Backup).

**Artefakt-URL-Wechsel:** Die fruehere Artefakt-URL (61235adf...) existiert nicht mehr (Artefakt geloescht — Galerie-Befund 05.08.); REV 4 ist unter NEUER URL publiziert: https://claude.ai/code/artifact/61359d31-8f7d-41b3-b5d6-8ff8a3fbe459 — dieses Repo-HTML bleibt die kanonische Kopie.

## REV-5-Vermerk (05.08.2026, Zukunfts-Phasen-SOLL + Rueckschreibe-Kette + Hinweis-Regeln-Entwurf)

Neue Sektion **#zukunft**: (1) Rueckschreibe-Kette Ende-zu-Ende (XML -> result.csv/stamp -> EIN-CSV-je-Zelle -> MeasurementSink/measure-drop -> messwert_registrierung -> Bestandslog-XML/doc_revision -> persist:measurements Git [Write-Token id=54] -> CSV->LaTeX) mit IST/PLAN-Kennung je Knoten; Format-Knoten als GoF-STRATEGY-WEICHE **CSV XOR xlsx, xlsx=DEFAULT** (Owner nachmittag-3; IST: nur Namens-Grammatik, xlsx-Serialisierer FEHLT -> Paket 46b/A9). (2) Zukunfts-Phasen-SOLL (PLAN-gestrichelt, IST-Anker solid): Messkurven-Synthese [IST: axis_spline/break_even/measurement_curve_loader komplett, Paragraf-75-geschuetzt] -> Kurven-Vergleich je Eingangslast -> beste Binary [IST: best_binary_selector] -> Release-Rekompilation OHNE Mess-Einrichtungen [IST: CMake-Schalter; Nachmess-LAUF ungedeckt] -> Wallclock-Nachmessung -> Multi-Last -> Last-Erkennungsheuristik [UNGEDECKT] -> Hybrid-Tier zur aktiven Runtime-Auswahl. (3) **5 UNGEDECKT-Badges** (Gegenpruefungs-Register nach Owner-Dauer-Regel nachmittag-4) + **Hinweis-Regeln-ENTWURF** (Posten 91, R1-R8 + Gegenseite N1-N6, Owner-Abnahme offen). Review FREIGABE nach 2 Zitat-Anker-Fixes.
