# Compositions — Algorithm-as-Achsen-Tupel (V41.F.6.1.R2 Pilot)

**Stand:** 2026-05-26 nach Architektur-Krise (User-Direktive 2026-05-26 spaet)
**Doku-Master:** `docs/architektur/14_achsen_komposition_organ_metapher.md`
**Memory:** `[[achsen-komposition-organ-metapher]]`

## Konzept

Ein **Composition-Template** ist ein reines `using`-Tupel das einen konkreten
**Such-Algorithmus** als **Permutations-Konfiguration aller Achsen** definiert.

```cpp
struct ArtComposition {
    using search_algo     = traversal::axis_03a_search_algo::Array256;       // 3.A BYTEBYBYTE
    using cache_traversal = traversal::axis_03b_cache_traversal::LinearFanout;
    using mapping         = traversal::axis_03m_mapping::DirectPlacement;
    using allocator       = allocator::axis_06_allocator::MimallocAllocator;
    static constexpr std::string_view paper_id = "P01 Leis/Kemper/Neumann ICDE 2013";
};
```

## Tier-Organ-Metapher (User-Direktive 2026-05-26)

> "Achse = Organ, Algorithmus = Komposition aller Achsen-Ausprägungen.
> Permutation = genetisches Experiment am Tier."

Composition-Templates **sind die "Tiere"** — sie sagen welche konkrete
Achsen-Ausprägung pro Organ verwendet wird, um den Original-Algorithmus
zu reproduzieren. Sie sind **keine** Permutations-Elemente sondern
Permutations-Konfigurationen.

## Aktuelle Compositions (Pilot R2)

| Composition | Paper | Search-Algo | Status |
|---|---|---|---|
| `ArtComposition` | P01 ART (Leis 2013) | Array256 (BYTEBYBYTE-aequivalent) | Pilot R2 |
| `WormholeComposition` | P07 Wormhole (Wu 2019) | VectorU8U8 (HASH_ANCHOR-aequivalent) | Pilot R2 |
| `SurfComposition` | P10 SuRF (Zhang 2018) | VectorU16U16 (LOUDS-aequivalent) | Pilot R2 |

## Erweiterbarkeit

Wenn neue Achsen angelegt werden (axis_04 node_type, axis_05 memory_layout,
axis_07 prefetch, axis_08 concurrency), werden die Composition-Templates um
weitere `using`-Lines erweitert.

## Permutation-Engine-Integration (R4 future)

PermutationEngine iteriert NICHT ueber Composition-Templates (das waeren
ja "Tiere"), sondern ueber **Cartesian aller Achsen-Sub-Werte**:

```cpp
using AllPermutations = mp_product<mp_list,
    axis_03a_search_algo::EnabledStrategies,
    axis_03b_cache_traversal::EnabledStrategies,
    axis_03m_mapping::EnabledStrategies,
    axis_06_allocator::EnabledVendors,
    // ... weitere Achsen
>;
```

Composition-Templates erscheinen als **konkrete Punkte** in dieser Permutations-
Matrix. PermutationEngine kann via `matches<P, ArtComposition>` ueberpruefen
ob eine bestimmte Permutation einer Referenz-Composition entspricht.

## Pflicht-Disziplin

1. **KEIN eigener Body** — Compositions sind reine using-Tupel
2. **KEINE neue API** — Compositions definieren Konfiguration, kein Verhalten
3. **paper_id Pflicht** — jede Composition referenziert Original-Paper
4. **static_assert Pflicht** — alle Sub-Achsen-Concepts werden statisch geprueft
