# 26 — Permutations-B+-Baum & inverse Signatur-Auswertung (2026-06-02)

> **User-Direktive 2026-06-02 (kritische Korrektur).** Die zuvor skizzierte „inverse Auswertung über FNV1a-
> Fingerprints" (Doc `20260602-cacheline-konfigurator-design` §7-B) ist FALSCH und wird durch dieses Modell ersetzt.
> Ein opaker Hash kann die gemischt statisch/dynamische Struktur nicht tragen und erlaubt keine Wiedererkennung
> „welcher Paper-Algorithmus gehört zu dieser Messung". Maßgeblich ist der **Permutations-B+-Baum** mit
> **serialisierten Pfaden** + **statischer Signatur-Wiedererkennung**.
>
> Grundsatz (merken): **Für Suche werden IMMER Bäume verwendet** — lineare Baum-Traversierung, NICHT quadratische
> Scans. (Thematisch konsequent: die Diplomarbeit IST eine Studie über Such-/Baum-Algorithmen.)

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
- **Blatt = Pointer auf das erzeugte optionale Resultat** (das Tier-Binary / der Mess-Eintrag; „optional", weil es
  bis zum Bau/zur Messung fehlen darf).
- **Pfad Wurzel→Blatt = die serialisierte, eindeutige Verifikation/Signatur** eines (gemischt statisch/dynamischen)
  Tier-Binary-Experiments. Die Pfadabfolge ERSETZT den FNV1a-Fingerprint als eindeutige Binary-ID.

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

- **Ein Paper-Tier = statische Rekombination aus B+-Baum-Ebenen** — ein bestimmter Pfad über die GEPINNTEN Achsen.
- Die **statische Signatur** = das **Array der gepinnten Achsen-Werte** (die Ebenen, die sich für dieses Paper NICHT
  ändern). Genau diese Signatur ist der **Wiedererkennungswert** für genau diesen Paper/Tier-Suchalgorithmus.
- **Filterung der für ein Paper-Teilexperiment relevanten Mess-Einträge:** alle Blätter, deren Pfad auf den GEPINNTEN
  Ebenen die statische Signatur des Papers trägt (die freigegebenen/dynamischen Ebenen variieren frei).
- **Doppelerkennung via multimap:** mehrere Paper können dieselbe statische Signatur teilen → `multimap<Signatur,
  Paper>`. So mappt eine Signatur auf alle zugehörigen Paper.

Damit traversieren wir in EINEM gemeinsamen großen B+-Baum-Gesamtexperiment über MULTIPLE Paper-Teilexperimente:
die Paper sind überlappende Pfad-Mengen, die sich ihre gepinnten Ebenen teilen, wo ihre Signaturen koinzidieren.
Die „inverse Auswertung" = Projektion der real gemessenen Blätter auf die Paper-Sichten per Signatur-Filter
(lineare Traversierung) — exakte Zuordnung, keine Interpolation, keine Doppelmessung.

## 4. Statische vs. dynamische Knoten — Abstract Factory (KEIN enum-Flag)

Die Baumknoten müssen formal in **statische** und **dynamische Beschreibungsknoten** unterscheidbar sein —
**per Abstract-Factory-Muster**, ein `struct`-`enum`-Flag genügt NICHT:

- `AbstractNodeFactory` (Schnittstelle) → erzeugt `INodeDescription`-Instanzen.
- `StaticAxisNodeFactory` → `StaticAxisNode` (beschreibt eine fixe Achsen-Algorithmus-Wahl; definiert Paper-Signatur).
- `DynamicVariableNodeFactory` → `DynamicVariableNode` (beschreibt eine dynamische Variablen-Iteration unter einer Achse).

Begründung: die zwei Knoten-Arten tragen UNTERSCHIEDLICHES Verhalten (Signatur-Beitrag, Serialisierung, ob sie zur
statischen Wiedererkennung zählen, ob compile-time gebacken oder Laufzeit-durchlaufen). Das gehört in getrennte Typen
mit polymorphem Verhalten (Factory), nicht in ein Discriminator-Flag — Typsicherheit + Erweiterbarkeit + saubere
Trennung der Serialisierungs-/Signatur-Logik.

## 5. Komplexität

- Aufbau + Traversierung des Baums = **linear** in der Zahl der Blätter (= Zahl der Tier-Binaries).
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
  Blatt = ein Tier-Binary, sein serialisierter Pfad = seine eindeutige ID. Statische Signatur je Blatt mitführen.
- **KF-15** (inverse Auswertung): KEINE Hash-Dedup, sondern **`multimap<statische Signatur, Paper>`** +
  Projektion der gemessenen Blätter auf die Paper-Sichten per Signatur-Filter (linear).
- **KF-8** (CEB-Generator): erzeugt je Blatt das `perm_<pfad>.cpp` (der Pfad determiniert die Achsen-Wahl je Ebene).
- Knoten-Typen (KF-9): `StaticAxisNode` / `DynamicVariableNode` via Abstract Factory.

## 8. Offen / zu bestätigen

- Genaue B+-Knoten-API (Fanout-Repräsentation, Serialisierungsformat des Pfads, Signatur-Encoding).
- Ob der Baum compile-time (Typ-Ebene) ODER als Laufzeit-Struktur im CEB-Generator gebaut wird (wahrscheinlich
  Laufzeit im Generator-Tool KF-8, da es das Profil zur Laufzeit liest und `perm_<id>.cpp` schreibt — das passt zur
  Faustregel: der Generator ist Host-Werkzeug, die erzeugten Binaries bleiben compile-time-statisch).
