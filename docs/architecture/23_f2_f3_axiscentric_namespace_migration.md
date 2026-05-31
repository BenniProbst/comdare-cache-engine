# Doku 23 — F.2/F.3: Axen-zentrische Namespaces + Achsen-Concepts (Migrationsplan)

> ✅ **STATUS-UPDATE / SUPERSEDED als aktiver Plan (2026-05-31):** Dieser MIGRATIONSPLAN ist UMGESETZT — F.2 (17/17
> physische `axes/<axis>/`, volle Regression 2112/2112) + F.3 (Concept-Layer alle 17, `F2F3_AxisCentricFacade` +
> perm_engine 21/21) sind **done-verified**. Als *aktiver* Plan damit **überholt** (historische Referenz); IST-treue
> Single-Source-of-Truth: `docs/sessions/architektur-ziele-offene-punkte-ledger.md` (§e). Etwaige „pending"-
> Formulierungen unten sind historischer Plan, NICHT offener Stand. Niemals löschen — nur Banner.

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
re-exponiert (Delegation via `requires`). **ALLE 17 Achsen ERLEDIGT** (`cache_engine::concepts::*Axis`):
traversal-Achsen via `*Variant` (LookupAxis=SearchAlgoVariant, CacheTraversalAxis, MappingAxis),
alloc/layout via `*Strategy` (AllocAxis, LayoutAxis), die uebrigen 12 via dem uniformen
`CacheEnginePermutationStrategy` (PathCompression/Node/Prefetch/Concurrency/Serialization/Telemetry/
ValueHandle/Simd/IndexOrganization/IoDispatch/MigrationPolicy/Filter). Damit entspricht JEDE Achse
konkret einem abstrakten Concept (verifiziert: `F2F3_AxisCentricFacade` static_assert je Achse ueber
deren Default-Typ).

## 5. `optional_prt_art_impl`-Slot (F.2)

Pro Achse ist `comdare::cache_engine::<axis>::optional_prt_art_impl` der reservierte Slot fuer
Pruefling-Spezialisierungen (prt-art u. a.; muss das jeweilige `cache_engine::concepts::*Axis`-Concept
erfuellen). Registrierung compile-time per CMake-Liste `COMDARE_CE_PRUEFLINGE` (open-todos §278–281):
cache-engine durchlaeuft je Pruefling alle Achsen und inkludiert `<cache_engine/<axis>/<pruefling>_impl.hpp>`
falls vorhanden (sonst Dummy). **ERLEDIGT (Code-Artefakt):** die 17 `optional_prt_art_impl`-Slot-Namespaces
sind in `axis_centric_namespaces.hpp` deklariert und ueber die Achsen-Aliase adressierbar
(`cache_engine::lookup::optional_prt_art_impl` etc.; verifiziert per Block-Scope-Alias im Facade-Test).
Offen bleibt die CMake-`COMDARE_CE_PRUEFLINGE`-Auto-Discovery + echte prt-art-Slot-Fuellung (#8/R8).

## 6. Stand

- **ERLEDIGT (Inkrement 1–3) — die gesamte STRUKTUR-Schicht von F.2/F.3:**
  1. Alias-Fassade (17 axen-zentrische Namespaces, rueckwaerts-kompatibel).
  2. F.3-Concepts fuer ALLE 17 Achsen (Concept-Layer vollstaendig, je Achse static_assert).
  3. `optional_prt_art_impl`-Slot-Namespaces fuer alle 17 Achsen (Pruefling-Erweiterungspunkt als
     Code-Artefakt, ueber Achsen-Aliase adressierbar). perm-engine 21/21, kein Regress.
- **VERBLEIBEND (GROSS, Mehr-Session):** der PHYSISCHE Rename je Achse (Stufe 2 — Definition-Namespace
  `<topic>::axis_NN` → `<axis>` umziehen + Header verschieben + Rueckwaerts-Alias; codebase-weit,
  ~10 Datei-Edits + ~30 Referenz-Aufloesungen je Achse) + Alt-Alias-Entfernung (Stufe 3) + CMake-
  `COMDARE_CE_PRUEFLINGE`-Auto-Discovery + echte prt-art-Slot-Fuellung (#8/R8). Die Struktur-Schicht
  steht; der Rename ist mechanische (aber risikobehaftete) Wiederholung je Achse, durch die Aliase
  jederzeit gruen-haltbar.
