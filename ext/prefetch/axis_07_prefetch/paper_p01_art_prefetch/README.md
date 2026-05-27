# paper_p01_art_prefetch — ART Distance-Estimator-Prefetch (Leis ICDE 2013)

**Paper-ID:** P01
**Topic:** axis_07_prefetch
**Wrapper:** DistanceEstimatorPrefetch

## Paper-Referenz

- **Titel:** The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases
- **Autoren:** Viktor Leis, Alfons Kemper, Thomas Neumann
- **Venue:** Proceedings of the 29th IEEE International Conference on Data
  Engineering (ICDE 2013), pp. 38-49
- **Jahr:** 2013
- **DOI:** 10.1109/ICDE.2013.6544812
- **URL:** https://db.in.tum.de/~leis/papers/ART.pdf

## Original-Code

- **Repository:** https://github.com/laurynas-biveinis/unodb (kuratierte ART-Variante)
- **License:** Apache-2.0
- **Kuratiert in Forschungsarbeiten:** `Forschungsarbeiten/code/P01-ART/unodb/`
- **Kopiert hier:** `src/art_internal.cpp`, `src/art_internal.hpp` (vor weiterer Kuratur)

## Distance-Estimator-Heuristik

Leis 2013 §4.2 beschreibt Distance-Estimator: pre-load next ART-Node wenn
estimated cache-distance > threshold. Implementiert via __builtin_prefetch.

Relevante Funktionen:
- `art_internal::prefetch_child` — Distance-Check + Conditional Prefetch
- `internal_node_4_or_16::find_child_for_key` — Pre-loadet Child-Pointer

## Habich-Compliance

- **Status:** Original-Code kopiert (Kopie-vor-Kuratur, User-Direktive 2026-05-27)
- **Compiler:** GCC 9+ (modern C++17+)
- **Mixin-Inheritance:** PrefetchOriginalCodeMixin (TBD — siehe Task R7.6.b axis_07)
- **is_original_module:** wird zur Build-Time gepruefft via apps/is_original_validator
