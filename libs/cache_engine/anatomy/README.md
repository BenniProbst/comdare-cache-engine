# anatomy/ — Saeugetier-Anatomie der Suchalgorithmen

**Phase:** V41.F.6.1.R3 (Stand 2026-05-26)
**Doku:** `docs/architektur/14_achsen_komposition_organ_metapher.md` §11+§12+§13

## Kern-Idee

Eine **einzige** generische `SearchAlgorithmAnatomy<Composition>`-Template-Klasse
liefert die Anatomie aller Suchalgorithmen. Konkrete Algorithmen sind reine
Template-Instantiationen mit konkreter Composition.

## Saeugetier-Anatomie-Metapher

> "Alle Saeugetiere haben im Kern hauptsaechlich dieselben Organe und Anatomie,
> aber alle Knochen, Organe, Bindegewebe haben unterschiedliche Auspraegungen."
> — User 2026-05-26

| Tier (Algorithmus) | Composition | Distinguishing Organ |
|---|---|---|
| ART | `ArtComposition` | Array256SearchAlgo-Skelett (dense) |
| HOT | `HotComposition` | VectorU8U8SearchAlgo + Patricia-Verdichtung |
| Wormhole | `WormholeComposition` | HashLookup (Verdauung) |
| SuRF | `SurfComposition` | PoolRelative (Nervensystem) + Filter = Funktion |
| Masstree | `MasstreeComposition` | VectorU16U16SearchAlgo (Layer-Slice 8-Byte) |
| START | `StartComposition` | VectorU16U16SearchAlgo (Multibyte-Span) |

## Dateien

- `composition_concept.hpp` — `IsComposition` Pflicht-Concept mit 17 using-Aliases
- `search_algorithm_anatomy.hpp` — `SearchAlgorithmAnatomy<C>` Template (R3 Pilot mit std::map Container)
- `known_algorithms.hpp` — 6 Template-Instantiationen (Art/Hot/Wormhole/SuRF/Masstree/Start)

## Phasen-Plan

| Phase | Was | Status |
|---|---|---|
| R3 | Skelett mit std::map Pilot-Container | aktiv |
| R3.2 | OriginalXxx-Wrappers deprecation | pending |
| R4 | PermutationEngine mp_product Cartesian | pending |
| R5 | CacheEngineBuilder baut pro Permutation .so/.dll | pending |
| R6 (V42) | Mess-Treiber + Welch-Test ueber tausende Permutationen | pending |
| R7 (V42) | F15-Auswertung schnellstes Tier identifizieren | pending |

## Forschungs-Ziel

> *Gibt es eine zentrale Anatomie-Implementation eines Suchalgorithmus, die
> durch Template-Parameter-Variation aller orthogonalen Achsen ALLE bekannten
> Such-Algorithmen als Spezialfaelle reproduziert UND eine systematische Suche
> im Permutations-Raum erlaubt um bisher unbekannte performante Algorithmen
> zu finden?*

Antwort wird durch R3-R7 aufgebaut.
