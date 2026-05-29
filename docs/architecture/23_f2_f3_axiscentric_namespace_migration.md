# Doku 23 — F.2/F.3: Axen-zentrische Namespaces + Achsen-Concepts (Migrationsplan)

**Stand:** 2026-05-29 · **Tasks:** #12 V41.F.2 (axen-zentrische Namespaces) + #13 V41.F.3 (Achsen-Concepts)
**Quelle:** `Diplomarbeit/docs/sessions/20260524-V41-open-todos.md` §261–314 (User-Wortlaut).

## 1. Ziel-Struktur (User-Direktive)

```
comdare::cache_engine::                       // Tools/Werkzeuge (zentral, wie gehabt)
comdare::cache_engine::<algorithm_axis>::     // ALLE Achsen-Interfaces aller Implementierungen
comdare::cache_engine::<axis>::optional_prt_art_impl   // Pruefling-Spezialisierung pro Achse
comdare::prt_art::                            // nur prt-art-spezifische Hilfen
```
Jede Achse + Basisfunktionalitaet entspricht einem abstrakten C++23-`concept` (F.3).

## 2. Sichere Migrations-Strategie (root-cause, KEIN Big-Bang-Rename)

Ein direktes Umbenennen aller `comdare::cache_engine::<topic>::axis_NN_<name>`-Namespaces wuerde jeden
Build-Zwischenstand brechen (codebase-weit, alle 1965 Tests). Stattdessen drei Stufen:

1. **Alias-Fassade (ERLEDIGT, Inkrement 1):** `libs/cache_engine/axes/axis_centric_namespaces.hpp`
   fuehrt die axen-zentrischen Namen als ALIASE auf die bestehenden topic-/achsen-Namespaces ein.
   Ab sofort gueltig: `comdare::cache_engine::lookup::Array256SearchAlgo` etc. — alt UND neu funktionieren,
   kein Bruch. (verifiziert: `F2F3_AxisCentricFacade.AliasesAreSameTypeAndConceptsHold`, perm-engine 21/21.)
2. **Inkrementeller physischer Rename (GROSS, je Achse):** pro Achse Header verschieben
   (`cache_engine/axes/<axis>/…`) + Definition-Namespace umbenennen; Referenzen folgen; der Alias bleibt
   bis alle Referenzen migriert sind → jederzeit gruener Build.
3. **Alt-Aliase entfernen:** wenn eine Achse vollstaendig physisch migriert ist, entfaellt ihr Alias.

## 3. Achsen-Map (physischer Topic-Namespace → axen-zentrischer Name)

| Achse (axen-zentrisch) | physischer Namespace (Implementierung) |
|------------------------|----------------------------------------|
| `lookup`               | `traversal::axis_03a_search_algo` |
| `cache_traversal`      | `traversal::axis_03b_cache_traversal` |
| `mapping`              | `traversal::axis_03m_mapping` |
| `path_compression`     | `nodes::axis_02_path_compression` |
| `node`                 | `nodes::axis_04_node_type` |
| `layout`               | `memory_layout::axis_05_memory_layout` |
| `alloc`                | `allocator::axis_06_allocator` |
| `prefetch_axis`        | `prefetch::axis_07_prefetch` |
| `concurrency_axis`     | `concurrency::axis_08_concurrency` |
| `serialization_axis`   | `serialization::axis_10_serialization` |
| `telemetry_axis`       | `telemetry::axis_11_telemetry` |
| `value_handle_axis`    | `value_handle::axis_14_value_handle` |
| `simd`                 | `hardware::axis_09_isa` |
| `index_organization`   | `search_engine::axis_01_index_organization` |
| `io_dispatch`          | `io::axis_io` |
| `migration_policy`     | `migration::axis_migration` |
| `filter_axis`          | `filter::axis_filter` |

> Namens-Hinweis: `*_axis`/`*_dispatch`/`*_policy`-Suffixe vermeiden Kollisionen mit bereits unter
> `cache_engine::` existierenden Topic-Namespaces gleichen Namens (z. B. `filter`, `prefetch`).

## 4. F.3 — abstrakte Achsen-Concepts

Pro Achse wird das bestehende, getestete Achsen-Concept unter dem axen-zentrischen abstrakten Namen
re-exponiert (Delegation via `requires`). **Pilot (ERLEDIGT):**
`cache_engine::concepts::LookupAxis` (= `SearchAlgoVariant`), `AllocAxis` (= `AllocatorStrategy`),
`LayoutAxis` (= `MemoryLayoutStrategy`). Die uebrigen 14 folgen demselben Muster beim Achsen-Rename.

## 5. `optional_prt_art_impl`-Slot (F.2)

Pro Achse ist `comdare::cache_engine::<axis>::optional_prt_art_impl` der reservierte Slot fuer
Pruefling-Spezialisierungen (prt-art u. a.). Registrierung compile-time per CMake-Liste
`COMDARE_CE_PRUEFLINGE` (open-todos §278–281): cache-engine durchlaeuft je Pruefling alle Achsen und
inkludiert `<cache_engine/<axis>/<pruefling>_impl.hpp>` falls vorhanden (sonst Dummy). Wird je Achse
beim physischen Rename (Stufe 2) materialisiert.

## 6. Stand

- **ERLEDIGT (Inkrement 1):** Alias-Fassade (17 Achsen) + F.3-Pilot-Concepts (3) + Test (21/21), kein Regress.
- **VERBLEIBEND (GROSS, Mehr-Session):** physischer Rename je Achse (Stufe 2) + Rest-Concepts (14) +
  Slot-Materialisierung + Alt-Alias-Entfernung (Stufe 3). Verbunden mit E11 (Facade braucht klare
  Achsen-Interfaces) und der prt-art-Pruefling-Einbindung (#8/R8).
