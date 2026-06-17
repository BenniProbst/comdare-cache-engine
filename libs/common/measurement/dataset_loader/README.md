# Dataset-Loader-Slot (AP-CE2)

Bindet **Nicht-YCSB-Datensaetze/Frameworks** (SOSD, TPC, SPEC, CloudSuite, gem5, Allokator-Suiten)
an den Mess-Apparat an, indem sie auf das gemeinsame **`comdare::workload_generator::Operation`**-Modell
(uint64) abgebildet werden. Ziel: ALLE Lastprofile gegen ALLE Lebewesen-Binaries fahren
(alle-gegen-alle-Matrix; Thesis Kap. 2.4.2 / 3.4.1).

**Stand 2026-06-17:** Slot + Registry + Komfort-Funktion + 1 Beispiel-Loader vorhanden (header-only).
Phase-5-Verdrahtung unten dokumentiert (bewusst noch nicht im Kern-Treiber angewandt — siehe Hinweis).

## Bestandteile
| Datei | Inhalt |
|---|---|
| `include/comdare/measurement/dataset_loader/dataset_loader.hpp` | `DatasetLoaderStrategy` (abstrakt), `DatasetLoaderRegistry` (Singleton), `load_or_generate_ycsb()` |
| `include/comdare/measurement/dataset_loader/loaders/example_uint64_keyfile_loader.hpp` | Beispiel-Loader `example_uint64_file` (Vorlage) |
| `CMakeLists.txt` | INTERFACE-Target `comdare_dataset_loader` (linkt `comdare_workload_generator`) |

## Eigenen Loader hinzufuegen
1. Neuer Header unter `include/.../loaders/<name>_loader.hpp`, Klasse erbt `DatasetLoaderStrategy`,
   implementiert `load(dataset_id, seed) -> std::optional<std::vector<wg::Operation>>`.
2. Selbst-Registrierung am Dateiende (wie im Beispiel):
   ```cpp
   inline const bool kReg = [] {
     DatasetLoaderRegistry::instance().register_loader("<loader_id>", std::make_unique<MyLoader>());
     return true; }();
   ```
3. `<loader_id>` == XML-Attribut `dataset_source`.

## Zu implementierende Framework-Slots (dokumentiert, noch leer)
| loader_id | Quelle | Mapping-Hinweis | Ziel-Lastprofil |
|---|---|---|---|
| `sosd` | SOSD-Index-Keys (binaer) | sortierte uint64-Keys → Read-Last (Punktsuche) | LP04/LP05, YCSB_C |
| `tpc` | TPC-C/TPC-DS (lineitem etc.) | Tabellen-Keys → Scan/RMW-Mix | LP11/LP14, YCSB_E/F |
| `spec` | SPEC CPU Traces | Instruktions-/Adress-Trace → uint64-Keys | YCSB_C |
| `cloudsuite` | CloudSuite (scale-out) | Request-Keys → zipfian Read/Update | YCSB_A |
| `gem5` | gem5-Traces | Speicher-Trace → uint64-Zugriffsfolge | YCSB_C |
| `allocator` | mimalloc-bench/Larson/threadtest/shbench | Alloc/Free-Folge → Insert/Erase | LP10/LP12 |

Determinismus (fester `seed`), Fairness (gleiche Eingaben je Kandidat), N/A statt verdeckter
Emulation bei fehlender Faehigkeit (Thesis Kap. 2.5).

## Verdrahtung in Phase 5 (`experiment_driver.cpp::phase5_run_workload`)
> **Hinweis:** Dieser Patch ist bewusst noch **nicht** angewandt, da hier kein C++-Compiler zur
> Build-Verifikation verfuegbar ist und `phase5_run_workload` Kern-Treiber-Code ist. Der Impl-Agent
> wendet ihn an und verifiziert im Vollbuild (Feldnamen `descriptor.test_data_set.attributes` gegen
> den realen `PermutationDescriptor` pruefen).

```cpp
#include <comdare/measurement/dataset_loader/dataset_loader.hpp>
// (mind. einen konkreten Loader-Header einbinden, damit er sich registriert)
#include <comdare/measurement/dataset_loader/loaders/example_uint64_keyfile_loader.hpp>

// in phase5_run_workload, bei der Workload-Erzeugung pro Modul/Datensatz:
namespace dl = comdare::measurement::dataset_loader;
std::string source = /* descriptor.test_data_set.attributes["dataset_source"] (leer = YCSB) */;
std::string ds_id  = /* descriptor.test_data_set.attributes["dataset_id"] bzw. YCSB-Buchstabe */;
auto ops = dl::load_or_generate_ycsb(source, ds_id, wopts.config, wopts.config.random_seed);
auto wl  = gen.to_abi_descriptor(ops);   // wie bisher -> module->run_workload(...)
```

## XML-Konvention (`test_data_sets.xml`)
```xml
<data_set id="sosd_books_1m">
  <attribute key="dataset_source">sosd</attribute>   <!-- loader_id -->
  <attribute key="dataset_id">books</attribute>      <!-- loader-spezifisch -->
</data_set>
```
Fehlt `dataset_source`, faellt `load_or_generate_ycsb` automatisch auf den YCSB-Generator zurueck
(kein Fatal-Error).

## Build-Kartierung (Achtung: Thesis-T ≠ Code-axis)
Bezug zur I/O-Dispatch-Achse (Thesis-T14, Code `axes/io_dispatch/`): der Dataset-Loader ist orthogonal
zur Achse — er erzeugt die *Eingabe-Last*, waehrend `io_dispatch` das *Storage-Backend* (mmap/direct/
in-memory) waehlt. Beide zusammen ergeben realistische Nicht-YCSB-Messungen. Vgl. Doc 35, Handout
`docs/sessions/20260617-HANDOUT-impl-agent-io-achse-tpie-mehlhorn.md`.
