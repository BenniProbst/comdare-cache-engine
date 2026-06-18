# 26 — Permutations-B+-Baum & inverse Signatur-Auswertung (2026-06-02)

> **User-Direktive 2026-06-02 (kritische Korrektur).** Die zuvor skizzierte „inverse Auswertung über FNV1a-
> Fingerprints" (Doc `20260602-cacheline-konfigurator-design` §7-B) ist FALSCH und wird durch dieses Modell ersetzt.
> Ein opaker Hash kann die gemischt statisch/dynamische Struktur nicht tragen und erlaubt keine Wiedererkennung
> „welcher Paper-Algorithmus gehört zu dieser Messung". Maßgeblich ist der **Permutations-B+-Baum** mit
> **serialisierten Pfaden** + **statischer Signatur-Wiedererkennung**.
>
> Grundsatz (merken): **Für Suche werden IMMER Bäume verwendet** — lineare Baum-Traversierung, NICHT quadratische
> Scans. (Thematisch konsequent: die Diplomarbeit IST eine Studie über Such-/Baum-Algorithmen.)

## 0. Schicht-Trennung — Experiment-Manager vs. Lebewesen/Organe vs. Diplomarbeit (User 2026-06-02)

**Diese Ergänzung betrifft AUSSCHLIESSLICH die Organisation/Verarbeitung des Experiments (erhobene Permutationen +
Ergebnisse). Sie ersetzt nur Bestandteile des MESS-Systems IN der `CacheEngineBuilder` (die das Prüf-Dock fährt) —
die Lebewesen und Organe bleiben UNVERÄNDERT.** Lebewesen/Organ-Metapher (Doku 14): Organ = Achse/Sub-Aufgabe eines Algorithmus;
Lebewesen = volle Achsen-Komposition; ein Lebewesen liegt nur SEZIERT als Organ-Komposition vor. Der B+-Baum strukturiert + liest
die Messungen ÜBER die Lebewesen/Organe, **greift aber nicht in sie ein**. Annahme: die aktuelle Cache Engine funktioniert
einwandfrei + ist perfekt aufgebaut — diese Ergänzung ändert nur die Mess-Organisation, nicht die Engine-Interna.

**WER macht WAS:**
- **Cache-Engine-Bibliothek = das WIE.** Profilverwaltung + Permutation (Aufbau/Traversierung des Experiment-B+-Baums)
  + die Organe/Achsen + das Messen am Prüf-Dock sind IMMER Aufgabe der Bibliothek (`CacheEngineBuilder` als
  Experiment-Manager).
- **Diplomarbeit = das WAS.** Liefert nur, WAS untersucht werden soll (`comdare_thesis_profile`), und liest die
  Ergebnisse **read-only über das Traversal des Experiment-Baums** durch ein Interface aus der Cache Engine.

Konkret = ein **custom C++23-SLURM-Experiment-Launcher mit Baum-Traversierung**, der die zuvor FLACH verwalteten
Experimente baum-strukturiert. **Alles in C++23.**

> Abgrenzung zur cacheline-Achsen-Erweiterung (KF-3/KF-5): Das per-Organ-`cacheline` ist eine separate, vom User
> mandatierte **Bibliotheks-Achsen-Erweiterung** (erweitert Organ-Algorithmen, damit sie die Cache-Line-Einstellung
> tragen — die untersuchte Eigenschaft). Sie ist NICHT Teil des Experiment-Managers; der Manager (dieses Dokument)
> fasst Organe nicht an, er misst + organisiert nur.

## 1. Das Problem mit Fingerprints

Ein flacher Hash (FNV1a) über die volle Achsen-Konfiguration liefert pro voller Kombination EINE opake ID. Damit
lässt sich:
- NICHT ausdrücken „diese Messung gehört zu Paper P, weil ihre GEPINNTEN Achsen P entsprechen" — der Hash mischt
  gepinnte + permutierte Achsen in einen Wert, eine Teilmengen-Wiedererkennung ist unmöglich.
- NICHT die gemischt statisch/dynamische Hierarchie abbilden (Achsen-Algorithmus vs. dynamische Variablen darunter).
- NICHT linear über Teil-Experimente traversieren.

## 2. Der Permutations-B+-Baum (ein Gesamt-Experiment)

EIN großer B+-Baum repräsentiert das GESAMTE Experiment (alle Paper + alle Permutationen):

- **Jede Baumebene = Entscheidung über den Algorithmus EINER Achse** (welches Organ für diese Achse).
- **Voll-dynamisches Fanout je internem Knoten** = Anzahl der an dieser Stelle EXPLORIERTEN Optionen der Achse:
  - Achse **gepinnt** (statisch, Paper-Original) → Fanout **1** (genau ein Kind).
  - Achse **zur Permutation freigegeben** → Fanout **N** (fächert über die N Varianten auf).
- **Eingeschobene dynamische Variablenebene UNTER einer Achse:** jede Achse, die ZUSÄTZLICH dynamische Variablen
  durchläuft (z.B. die per-Organ-`cacheline`-Werte, oder thread_count unter concurrency), erzeugt unter ihrer
  Achsen-Ebene eine WEITERE dynamische Baumebene (mit eigenem Fanout über die dynamischen Werte).
- **Blatt = Pointer auf das erzeugte optionale Resultat** (das Lebewesen-Binary / der Mess-Eintrag; „optional", weil es
  bis zum Bau/zur Messung fehlen darf).
- **JEDE node hält ein Key-Value (nicht nur die Blätter):**
  - **key** = die **serialisierte Signatur der Kind-Permutationen**, die der Knoten durch seine bloße Existenz
    repräsentiert (der Pfad-Abschnitt Wurzel→Knoten);
  - **value** = die **Observer-Statistics + Mess-Auswertung**, die der Knoten **spezifisch für diese B+-Baum-Ebene
    persistiert** (aggregiert über den Teilbaum unter dem Knoten).
  → Der Baum ist damit ein auf JEDER Ebene lesbarer Ergebnis-Speicher: die Diplomarbeit liest Resultate auf beliebiger
  Granularität (z. B. „alle Messungen unter traversal=ART" am ART-Knoten aggregiert) per reiner Baum-Traversierung.

**Ausführungssemantik am Prüf-Dock (User 2026-06-02) — Kern der Trennung:**
- **STATISCHE Knoten = je distinkter Static-Pfad lädt EINE NEUE Lebewesen-Binary ins Prüf-Dock** (compile-time-Identität,
  inkl. der compile-time-Cache-Line-Sub-Properties line_size/alignment/sw_hint, die in die Binary gebacken sind).
- **DYNAMISCHE Knoten = eine FOR-SCHLEIFE auf EINER bereits geladenen Binary**, die nacheinander die Test-Einstellungen
  über die Variablen-Schnittstelle (`Algorithm_Resource_Control`, KF-4) durchprobiert (thread_count, hw_prefetcher, …)
  — erzeugt KEINE neue Binary.
- Folglich: ein **Blatt = EIN Mess-Lauf (Binary × Laufzeit-Einstellung)**; `binary_count` = Zahl distinkter Static-Pfade;
  Prüf-Dock-Modell = **je Binary EINMAL laden, dann Laufzeit-Schleife** über die dynamischen Einstellungen (KF-7).

**Materialisierung + Filterung (User 2026-06-02):** Der Gesamtbaum existiert formal ZUSAMMENHÄNGEND (statische +
dynamische Schichten), aber MATERIALISIERT werden nur die STATISCHEN Ebenen → Binary-Blätter; die dynamischen
Variablen sind VIRTUELL ineinander geschachtelte for-Schleifen (nicht physisch aufgefächert → der Baum bleibt bei der
BINARY-Zahl, nicht der dyn. Kartesik).

> **KORREKTUR/Präzisierung (User 2026-06-02, OOM-Befund):** „Den gesamten Baum zu materialisieren ist falsch — du
> materialisierst immer nur EINEN Pfad von der Wurzel zum Blatt und iterierst durch. Beim Build sind es nur so viele
> Pfade wie zulässige DLL-Build-Prozesse." Auch der STATISCHE Teilbaum darf NICHT eager voll-materialisiert werden:
> bei vollem Enabled-Inventar ist die Binary-Zahl ∏ mp_size(Enabled_i) astronomisch (≥ 1e15) — ein Knoten-je-Wert-
> je-Ebene-Aufbau zieht OOM (empirisch belegt: ein eager `tree.build` über alle 22 Achsen zog ~21 GB). Daher:
> - **`binary_count()` = ∏ der statischen Ebenen-Größen — REIN ARITHMETISCH**, ohne je einen Knoten zu materialisieren
>   (Doc 26 §5: der Baum ZÄHLT die Kardinalität, er baut sie nicht).
> - **Iteration = lazy mixed-radix Odometer:** es lebt zu jeder Zeit nur EIN Pfad Wurzel→Blatt (O(Tiefe) Speicher);
>   nach dem Besuch wird er verworfen, der nächste erzeugt.
> - **Build = nur K Pfade gleichzeitig** (K = zulässige parallele DLL-Build-Prozesse, KF-16b): die indizierte
>   `StaticBinaryView::operator[](i)` dekodiert genau EINEN Pfad on-demand (Index → mixed-radix), der Orchestrator
>   hält nie alle ∏ Specs. Implementiert in `experiment_tree.hpp` (lazy `StaticBinaryView` + `for_each_binary`-Odometer).
> - **per-node Observer-Statistics (§2) = SPARSE Map** (key=binary_id → NodeValue), NUR für tatsächlich GEMESSENE
>   Binaries — nie ein Eintrag je ∏-Blatt. Eine **Baum-Filterung nach statisch/dynamisch** (`static_filter()` /
`dynamic_filter()`) extrahiert aus dem zusammenhängenden Ganzen den statischen Teilbaum (Binaries) und den dynamischen
Teilbaum (Iterations-Schleifen je Binary). Das **BLATT (`ExperimentSetting`) akkumuliert die volle dynamische Belegung
als EXAKT EINE Experiment-Einstellung** (Binary × eine dyn. Kombination). Implementiert in
`libs/cache_engine/builder/experiment_tree/` (KF-9, verifiziert).
- **Pfad Wurzel→Blatt = die serialisierte, eindeutige Verifikation/Signatur** eines (gemischt statisch/dynamischen)
  Lebewesen-Binary-Experiments. Die Pfadabfolge ERSETZT den FNV1a-Fingerprint als eindeutige Binary-ID.

```
                (Wurzel)
                  │  Achse traversal  (gepinnt: ART → Fanout 1)
              [traversal=ART]
                  │  Achse node       (freigegeben → Fanout N)
        ┌─────────┼─────────┐
   [node=N4]  [node=N16] [node=N256]
        │          ...        │  dynamische Ebene UNTER node: cacheline (Fanout 3: 64/128/256)
   ┌────┼────┐               ...
 [cl=64][cl=128][cl=256]
   │
  (Blatt → optional* Resultat-Pointer; Pfad = "traversal=ART/node=N4/cacheline=64/...")
```

## 3. Paper-Wiedererkennung über die statische Signatur (NICHT Hash)

- **Ein Paper-Lebewesen = statische Rekombination aus B+-Baum-Ebenen** — ein bestimmter Pfad über die GEPINNTEN Achsen.
- Die **statische Signatur** = das **Array der gepinnten Achsen-Werte** (die Ebenen, die sich für dieses Paper NICHT
  ändern). Genau diese Signatur ist der **Wiedererkennungswert** für genau diesen Paper/Lebewesen-Suchalgorithmus.
- **Filterung der für ein Paper-Teilexperiment relevanten Mess-Einträge:** alle Blätter, deren Pfad auf den GEPINNTEN
  Ebenen die statische Signatur des Papers trägt (die freigegebenen/dynamischen Ebenen variieren frei).
- **Doppelerkennung via multimap:** mehrere Paper können dieselbe statische Signatur teilen → `multimap<Signatur,
  Paper>`. So mappt eine Signatur auf alle zugehörigen Paper.

Damit traversieren wir in EINEM gemeinsamen großen B+-Baum-Gesamtexperiment über MULTIPLE Paper-Teilexperimente:
die Paper sind überlappende Pfad-Mengen, die sich ihre gepinnten Ebenen teilen, wo ihre Signaturen koinzidieren.
Die „inverse Auswertung" = Projektion der real gemessenen Blätter auf die Paper-Sichten per Signatur-Filter
(lineare Traversierung) — exakte Zuordnung, keine Interpolation, keine Doppelmessung.

## 4. Statische vs. dynamische Knoten — zwei Produkt-Typen via Factory Method (KEIN degeneriertes bool-Flag)

> **Muster-Klarstellung (K10-PMAJOR-06, 2026-06-18, code-verifiziert):** Die realisierte Schnittstelle in
> `experiment_tree.hpp` ist die GoF-**Factory-Method**-Variante — **EIN** abstrakter Creator
> (`AbstractNodeFactory`) mit **zwei Factory-Methoden** (`make_static`/`make_dynamic`), die **zwei distinkte
> Produkt-Typen** (`StaticAxisNode`/`DynamicVariableNode`, beide `INodeDescription`) erzeugen. Die früher hier
> beschriebenen **getrennten Fabrik-Klassen** `StaticAxisNodeFactory`/`DynamicVariableNodeFactory` (klassische
> Zwei-Fabriken-Abstract-Factory) sind **NICHT** implementiert; der Anspruch ist auf das tatsächlich realisierte
> Factory-Method-Muster zurückgeführt. Die Produkt-Art trägt ein `NodeKind`-Enum als **legitime
> Selbstauskunft** (GoF-konformer Produkt-Diskriminator), **kein** „degeneriertes bool-Flag" — das `bool
> is_static` in `AxisLevel` ist eine flache Reflektions-Zeilen-Markierung der `StaticBinaryView`-Ebenen,
> nicht die Knoten-Fabrik.

GENAU ZWEI Knotenarten als **distinkte Produkt-Einzelklassen** (ein nacktes Enum-Flag OHNE eigene Typen genügt NICHT):

- `AbstractNodeFactory` (Schnittstelle) → erzeugt `INodeDescription`-Instanzen über `make_static`/`make_dynamic`.
- `StaticAxisNode` (via `make_static`) — bildet eine **statische Achseneigenschaft eines Organ-Algorithmus** ab
  (die fixe Algorithmus-Wahl der Achse; trägt die Paper-Signatur).
- `DynamicVariableNode` (via `make_dynamic`) — bildet eine **Organ-Algorithmus-Variable (dynamisch)** ab
  (eine einstellbare Variable unter einer Achse, z. B. cacheline-Wert, thread_count).

**Zusätzlich werden beide Arten je konkreter Achseneigenschaft als EIGENE Einzelklasse ausgeprägt** (nicht generisch-
flach), damit:
- für **jedes einstellbare Detail die korrekten Variablen** typsicher mitgeliefert werden, und
- **Observer-Statistics-Mappings für ALLE Kind-Serialisierungen** im Knoten aufgenommen werden können (das key→value
  je Knoten aus §2: key = serialisierte Signatur der Kind-Permutationen, value = Observer-Statistics + Mess-Auswertung
  dieser Ebene).

**Die Knotenart IST die compile-time/runtime-Unterscheidung** (kein separates Flag): `StaticAxisNode` = compile-time →
lädt eine Binary; `DynamicVariableNode` = Laufzeit-FOR-SCHLEIFE auf der geladenen Binary über die Variablen-Schnittstelle.
Compile-time-variierende Eigenschaften (auch cacheline-size/alignment, obwohl „Sub-Ebene unter einer Achse") sind daher
**StaticAxisNodes** (sie erzeugen Binaries); nur echt laufzeit-einstellbare Größen (thread_count, hw_prefetcher) sind
`DynamicVariableNodes`. Begründung gegen enum-Flag: die Arten + Achseneigenschaft-Spezialisierungen tragen
unterschiedliches Verhalten (Binary-Identität vs. Laufzeit-Schleife, Signatur-Beitrag, Variablen-Satz, Observer-Mapping,
Serialisierung) → getrennte Typen + Factory (Typsicherheit + Erweiterbarkeit).

## 5. Komplexität

- Aufbau + Traversierung des Baums = **linear** in der Zahl der Blätter (= Zahl der Lebewesen-Binaries).
- Paper-Projektion = lineare Traversierung + Signatur-Filter (multimap-Lookup O(log) je Signatur).
- KEINE quadratische All-gegen-all-Suche. **Für Suche immer Bäume.**

## 6. Bezug zur bestehenden Architektur

- **Ersetzt/verfeinert:** die flache compile-time `mp_product`-Enumeration der `PermutationEngine`
  (`anatomy/search_algorithm_permutation_engine.hpp`) wird als B+-Baum strukturiert (gepinnte vs. freigegebene
  Ebenen + dynamische Sub-Ebenen). Der **`fingerprint_of` (FNV1a)** in `02_messung_driver/measurement_writer.hpp`
  wird durch die **serialisierte Pfad-Signatur** ersetzt (FNV1a darf höchstens noch als kompakter Index ÜBER der
  Pfad-Zeichenkette dienen, nie als Wiedererkennungs-Schlüssel).
- **3-Stufen-Modi** (`pruefling_merge.hpp`): die `active_axes`-Maske je Modus = welche Ebenen freigegeben sind;
  Stufe-2-Prüfling-Replace = der Prüfling pinnt die ersetzten Ebenen.
- **Cache-Line per Organ** (Doc 26-Vorgänger / KF-3): jede betroffene Organ-Achse bekommt ihre dynamische Sub-Ebene.

## 7. Auswirkung auf die KF-Tasks

- **KF-9** (Enumeration): NICHT flaches kartesisches Nested-Loop, sondern **B+-Baum bauen** (aus dem
  `comdare_thesis_profile`: gepinnt vs. freigegeben je Achse + dynamische Sub-Ebenen) und **traversieren**; jedes
  Blatt = ein Lebewesen-Binary, sein serialisierter Pfad = seine eindeutige ID. Statische Signatur je Blatt mitführen.
- **KF-15** (inverse Auswertung): KEINE Hash-Dedup, sondern **`multimap<statische Signatur, Paper>`** +
  Projektion der gemessenen Blätter auf die Paper-Sichten per Signatur-Filter (linear).
- **KF-8** (CEB-Generator): ZWEI getrennte Pfade (D3/L-77, 2026-06-02): (1) `ceb_generator::generate_all` = STRING-
  getriebenes **Diagnose/Pfad-Manifest** (je Blatt ein `perm_<pfad>.cpp` mit Pfad-#defines + `perm_run`-Stub, KEINE
  reale Anatomie — der Pfad kennt nur Namen, kein C++-Typ → bewusst kein String→Typ-Dispatch); (2)
  `ceb_generator::generate_all_real<Engine>` → delegiert TYP-getrieben an `codegen::emit_adhoc_modules<Engine>` =
  **realer BR-4-Anatomie-Emitter** (`COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<17 FQ>)` + Umbrella; baubar/ladbar/observierbar).
- Knoten-Typen (KF-9): `StaticAxisNode` / `DynamicVariableNode` via Abstract Factory.

## 8. Entscheidungen (Stand 2026-06-02, vom User bestätigt)

- ✅ **Schicht-Trennung (§0):** B+-Baum/`CacheEngineBuilder` = Experiment-Manager (das WIE); Diplomarbeit = das WAS +
  read-only-Traversal. Lebewesen/Organe unverändert (cacheline-Achse KF-3/5 = separate Bibliotheks-Achsen-Erweiterung).
- ✅ **Zwei Knotenarten** (`StaticAxisNode` / `DynamicVariableNode`), als Einzelklassen je Achseneigenschaft;
  compile-time vs. runtime als Attribut am dynamischen Knoten (keine dritte Art).
- ✅ **Jede node hält key+value** (serialisierte Signatur → Observer-Statistics/Mess-Auswertung der Ebene).
- ✅ **Alles C++23**, inkl. des custom SLURM-Experiment-Launchers mit Baum-Traversierung (ersetzt die flache Verwaltung).
- offen (Implementierungsdetail, in KF-9 festzulegen): genaues Serialisierungsformat des Pfads/der Signatur +
  konkretes Observer-Mapping-Layout je Knoten + die Read-Interface-Signatur für die Diplomarbeit-Seite.
