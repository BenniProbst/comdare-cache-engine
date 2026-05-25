# cache-engine API — Master-Framework-Facade (V41.E11)

**Stand:** 2026-05-25 (Skelett, User-Direktive)

## Zweck

cache-engine ist **Master-Framework**. Konsumenten (z.B. Diplomarbeit) linken NUR gegen cache-engine — die 6 ausgelagerten Submodule (`comdare-{search-engine,cache-engine-core,measurement,isa-dispatch,build-tools,test-system}`) werden intern via **Facade-Pattern** vermittelt.

Zusaetzlich: das PermutationsModul nimmt Pruefling(e) (z.B. prt-art) via **Abstract-Factory-Pattern** auf. cache-engine HAT konfigurierte Pruefling(e) — gewaehlt via CMake-Option `COMDARE_CE_PRUEFLING=prt-art;...`.

## Headers

| Header | Zweck |
|--------|-------|
| `i_cache_engine.hpp` | Facade-Interface ICacheEngine + `get_cache_engine()` Singleton |
| `i_pruefling_factory.hpp` | Abstract Factory: IPruefling, IPrueflingFactory, IPrueflingRegistry |

## Status

- **Skelett:** Interface-Headers vorhanden (V41.E11 heute).
- **Implementation:** Phase 7+ in `src/facade/cache_engine_facade.cpp`.
- **Prueflings-Adapter:** Phase 7+ in `comdare-prt-art/src/prt_art_factory.cpp`.
- **Diplomarbeit-Migration:** Phase 7+ — Code/02_messung_driver/main.cpp ersetzt direkte Includes durch `<cache_engine/api/i_cache_engine.hpp>`.

## Konsument-Beispiel (Phase 7+, geplant)

```cpp
#include <cache_engine/api/i_cache_engine.hpp>
#include <cache_engine/api/i_pruefling_factory.hpp>

using namespace comdare::cache_engine::api;

int main() {
    auto& ce = get_cache_engine();
    auto& reg = get_pruefling_registry();

    // Iteriere alle registrierten Prueflinge (z.B. prt-art)
    for (auto* factory : reg.all_factories()) {
        for (auto axes : factory->available_axes_combinations()) {
            auto pruefling = factory->create(axes);
            double micros = 0.0;
            if (pruefling->run(1000, micros) == 0) {
                // ce.measurement().record(...)
            }
        }
    }
}
```

## Bezug zu anderen V41-TODOs

- **V41.E5:** OBSOLET — Diplomarbeit linkt nicht direkt 3 Modul-Subsets, sondern via Facade
- **V41.E10:** verbunden — Permutations-Binaries enthalten Facade + Achsen + Pruefling-Body STATIC
- **V41.E4.1:** Phase-7+ — die 6 Modul-Repos werden mit Inhalt gefuellt, der dann die Facade-Provider-Implementierungen liefert
