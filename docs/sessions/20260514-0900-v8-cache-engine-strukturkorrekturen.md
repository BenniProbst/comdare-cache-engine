# V8 cache-engine Strukturkorrekturen (2026-05-14, 09:00)

**Master:** `Diplomarbeit/docs/sessions/20260514-0900-v8-implementations-anker-rev7-6.md`
**Schwester:** `comdare-prt-art/docs/sessions/20260514-0900-v8-prt-art-abi-inheritance.md`

**Anlass:** User-Vervollstaendigung von 3 Diplomarbeits-Dokumenten am
2026-05-13/14 mit 8 Kommentaren, davon 7 cache-engine-relevant.

---

## §1 Cache-Engine-spezifische Aufgaben (V8.2-V8.8)

### V8.2 — prt_art/legacy_reimpl/ Migrations-Plan (NICHT Loeschung)

User-Direktive: *"Derzeit gibt es faelschlicherweise einen PRT-ART Ordner
direkt in der CacheEngine, aber der PRT-ART sollte mit neuen Layered
Algorithmus-Baustein-Bestandteilen und einer separaten eigenen
Konfiguration im PRT-ART repo als Pruefling dargestellt werden."*

**Vorgehen:**
1. README.md in `prt_art/legacy_reimpl/` mit DEPRECATED-Warnung +
   Migrations-Verweis auf prt-art Repo.
2. CMakeLists.txt schaltet legacy_reimpl auf nicht-default (Option
   COMDARE_BUILD_LEGACY_REIMPL OFF).
3. In prt-art-Repo: gleiche Struktur unter `prt_art/legacy_reimpl/` anlegen.
4. Folge-Phase: Loeschung in cache-engine NACH ctest-Verifikation.

### V8.3 — algorithm_profiles/ Ordner

User-Direktive: *"Ich vermisse in der CacheEngine einen Ordner, in dem
alle Suchalgorithmen mit ihrer Gesamtkonfiguration als XML/json
persistiert sind, sodass die in der CacheEngine einzeln persistierten
Algorithmusbestandteile durch die Permutationsbeschreibung
wiederherstellbar sind."*

**Layout:**
```
cache_engine/algorithm_profiles/
├── README.md                      (Erklaert Profil-Format)
├── _schema.xsd                    (XSD-Validierung)
├── permutation_axes.xml           (11 Achsen + Wertebereiche)
└── sota/
    ├── art.profile.xml            (P01 Leis 2013)
    ├── hot.profile.xml            (P02 Binna 2018)
    ├── masstree.profile.xml       (P03 Mao 2012)
    ├── coco_trie.profile.xml      (P04 Boffa 2024)
    ├── start.profile.xml          (P05 Fent 2020)
    ├── b2tree.profile.xml         (P06 Schmeisser 2022)
    ├── wormhole.profile.xml       (P07 Wu 2019)
    ├── surf.profile.xml           (P10 Zhang 2018)
    ├── css_tree.profile.xml       (P11 Rao/Ross 1999)
    ├── csb_tree.profile.xml       (P12 Rao/Ross 2000)
    └── ...
```

Pro Profil ein vollstaendiger Konfig-Satz: Page + Node + Traversal +
Allocator + Concurrency + Telemetry + Prefetch + ISA + Layout +
Reclamation + Sync.

### V8.4 — COMDARE_EXPERIMENT_MODE CMake flag

```cmake
# cache-engine root CMakeLists.txt (NEW)
option(COMDARE_EXPERIMENT_MODE
    "Aktiviert ResultAggregator + Mess-Hooks in der ExecutionEngine"
    OFF)
if(COMDARE_EXPERIMENT_MODE)
    add_compile_definitions(COMDARE_EXPERIMENT_MODE_ON=1)
    message(STATUS "COMDARE_EXPERIMENT_MODE: ON (Mess-Pfad aktiv)")
else()
    message(STATUS "COMDARE_EXPERIMENT_MODE: OFF (Production-Pfad)")
endif()
```

User-Direktive: *"Schalten wir den Experiment Modus ab, den wir bei der
Kompilation aus dem CMake code der CacheEngine erben, dann kompilieren
wir einen Suchalgorithmus als SearchEngine ohne Messung. Dies sollte ein
statisch einstellbares CMake compile flag sein, welches default uebrigens
aus ist, nur der Aufruf des Messtreibers aktiviert und ueberschreibt die
Einstellung beim Laden des PRT-ART, welcher das flag dann seinerseits an
die CacheEngine weitergibt."*

### V8.5 — ResultAggregator in ExecutionEngine

```cpp
// cache_engine/include/cache_engine/abi/execution_engine.hpp
#ifdef COMDARE_EXPERIMENT_MODE_ON
#include <comdare/experiment/result_aggregator.hpp>
#endif

template <typename ProcessingStrategy>
class execution_engine : public CacheEngine {
public:
    #ifdef COMDARE_EXPERIMENT_MODE_ON
    [[nodiscard]] experiment::ResultAggregator& result_aggregator() noexcept {
        return aggregator_;
    }
    #endif

private:
    #ifdef COMDARE_EXPERIMENT_MODE_ON
    experiment::ResultAggregator aggregator_;
    #endif
};
```

User-Direktive: *"Dadurch gehoert der result Aggregator in die abstrakte
ExecutionEngine, wird dort mithilfe der SearchEngine als spezielles
Messinterface implementiert und ist, wenn wir im experiment Modus
kompilieren, immer fester Bestandteil der result binary."*

### V8.6 — defined/full mode in xml_config_parser

```xml
<!-- erweitertes Schema -->
<comdare>
  <messreihe id="A_PRT_ART_vs_SOTA">
    <mode>defined</mode>
    <sota_profiles>
      <profile>art</profile>
      <profile>hot</profile>
      <!-- ... -->
    </sota_profiles>
  </messreihe>
  <messreihe id="A_PRT_ART_vs_SOTA_full">
    <mode>full</mode>
    <!-- alle Achsen-Permutationen -->
  </messreihe>
</comdare>
```

xml_config_parser.{hpp,cpp} um:
- `enum class MessreihenMode { Defined, Full }`
- Filter-Logik: defined → nur referenzierte Profile, full → alle
  Permutationen aus permutation_axes.xml

### V8.7 — CacheEngineBuilder zwei-stufig

User-Direktive: *"Ursprünglich war vorgesehen, dass ein CacheEngineBuilder
als statisches Programm und Bestandteil der CacheEngine ABI stabile und
dynamisch vorkompiliert ladbare C++23 Module (je SearchEngine
Suchalgorithmus ein vorkompiliertes Modul) laed und ausmisst, das
bedeutet, dass es fuer den CacheEngineBuilder (den wir beibehalten wollen)
eine permutierte vorbereitete Liste an ExecutionEngine virtual
Implementierungen gibt, die ueber ein virtual Interface zuerst getestet
... und im naechsten Schritt ausgemessen werden. ... Ist eine Permutation
fuer die CacheEngineBuilder nicht vorkompiliert vorliegend, wird sie per
runtime auf einen CompilerAufruf aus der Metaprogrammierten Permuation
des Suchalgorithmus-Stacks generiert."*

**Stage 1 (Compile-Time):** Builder-Binary selbst wird zuerst gebaut.
**Stage 2 (Runtime):** Builder iteriert Permutationen:
- Wenn vorkompiliert (.dll/.so vorhanden) → laden + messen
- Sonst → cmake/cl auf source library aufrufen (hot compile) → laden + messen

Implementation in `experiment_driver/experiment_driver.hpp`:
```cpp
class ExperimentDriver {
public:
    // Stage 2: pro Permutation pruefen
    int phase3_compile_or_load_runtime(PermutationDescriptor const& p);
};
```

### V8.8 — algorithm_baustein als compile-time std::variant

User-Direktive: *"Formal ist jeder Algorithmus-Baustein eine Ansammlung
an compile time std::variants dieser bestimmten Algorithmus Komponente.
Formal handelt es sich also synchron auf jeder spezifischen Ebene des
Stacks ... um einen full join der angebotenen Algorithmen des oder der
multiplen Pruefling mit der Ebene bekannter Layer-Baustein-Algorithmen,
die bereits in der CacheEngine zum Stand der Technik gehoeren."*

```cpp
// cache_engine/include/cache_engine/abi/algorithm_baustein.hpp (NEU)
namespace comdare::cache_engine::baustein {

template <typename... Variants>
struct algorithm_axis {
    using variant_t = std::variant<Variants...>;
    static constexpr std::size_t size = sizeof...(Variants);
};

// Beispiel: Page-Achse mit allen SOTA + Pruefling-Konkretisierungen
using PageAxis = algorithm_axis<
    DenseByteArt256Page,        // ART
    HotMultiBytePage,           // HOT
    MasstreeINodePage,          // Masstree
    CoCoCompactPage,            // CoCo-Trie
    /* ... weitere SOTA */
    PrtArtRedirectPage          // PRT-ART (Pruefling)
>;

// Full-Join Pattern: Cartesian product zweier Achsen
template <typename Axis1, typename Axis2>
struct full_join {
    template <std::size_t I, std::size_t J>
    using pair_t = std::pair<
        std::variant_alternative_t<I, typename Axis1::variant_t>,
        std::variant_alternative_t<J, typename Axis2::variant_t>>;
};

}  // namespace
```

---

## §2 Reihenfolge der Umsetzung

V8.4 → V8.5 → V8.3 → V8.6 → V8.8 → V8.7 → V8.2 (Migrations-Plan)

Nach jedem Schritt: cmake + ctest verifizieren.

---

## §3 Cross-Repo-Konsequenzen

- prt-art (`docs/sessions/20260514-0900-v8-prt-art-abi-inheritance.md`):
  V8.9 baut auf V8.4-V8.5 auf.
- Diplomarbeit (`docs/sessions/20260514-0900-v8-implementations-anker-rev7-6.md`):
  V8.12 baut auf V8.4 auf.

---

## §4 Akzeptanzkriterien (cache-engine)

- [ ] `cmake -B build-msvc` ohne Flag baut + ctest gruen
- [ ] `cmake -B build-msvc-exp -DCOMDARE_EXPERIMENT_MODE=ON` baut + ctest gruen
- [ ] `cache_engine/algorithm_profiles/sota/` hat 8+ Profile
- [ ] `xml_config_parser` parst `<mode>defined</mode>` + `<mode>full</mode>`
- [ ] `prt_art/legacy_reimpl/README.md` hat DEPRECATED-Warnung
- [ ] `cache_engine/include/cache_engine/abi/algorithm_baustein.hpp` existiert
