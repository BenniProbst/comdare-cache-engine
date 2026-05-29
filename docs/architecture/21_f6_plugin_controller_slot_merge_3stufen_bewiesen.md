# Doku 21 — F.6 Plugin-Controller + Slot-Füllung + 3-Stufen-Permutation: bewiesene Architektur

**Stand:** 2026-05-29 · **Status:** Kern-Mechanismen end-to-end bewiesen (Compile-Time) ·
**Bezug:** Doku 14 §18-§19 (Kompositionale Joins), Doku 19 (F.6-Migration), Doku 20 (Plugin-Controller-Anforderung)

Dieses Dokument konsolidiert die **end-to-end verifizierte** Architektur-Kette der Diplomarbeit und
grenzt sie vom noch offenen schweren Build-Subsystem ab. Alle hier als „bewiesen" markierten Aussagen
sind durch `static_assert`- bzw. GTest-Tests im Repo abgesichert (Pfade in Klammern).

---

## 0. Kern-Beitrag der Arbeit (Motivation — warum das alles)

Die gesamte Architektur begründet sich aus EINEM Kern-Beitrag (User-Direktive 2026-05-29):

1. **Zerlegen + original-getreu rekonstruieren:** Existierende Algorithmen aus Papern werden in
   **Achsen-Algorithmen zerlegt**, sodass sie als **eindeutige Kompositions-Auswahl aller Achsen** per
   Metaprogrammierung **original-getreu wieder erstellbar** sind (Habich-Compliance, Doku 18 Provenienz).
2. **Modularer Austausch → Vergleichbarkeit:** Über die Achsen-Schichten lassen sich einzelne
   Achsen-Algorithmen **modular austauschen** — gleiches Gerüst, eine Achse variiert → starke Vergleichbarkeit.
3. **Transparente Anatomie:** Wir finden die **Anatomie** eines (Such-)Algorithmus / Containers und machen
   sie **transparent auswertbar — pro Achse UND für den gesamten Algorithmus** (zweidimensionale Messung, §4a).
4. **Das gelöste Problem:** Ein Forscher, der nur EINEN neuen Achsen-Algorithmus erforscht, müsste sonst
   einen ganzen Suchalgorithmus bauen + für ALLE übrigen Achsen je einen Algorithmus finden/implementieren,
   den er gar nicht braucht. Ineffizient.
5. **Die Lösung:** Ein **Library-Framework der Algorithmus-Achsen mit der cache engine** — der Forscher
   steckt seine eine neue Achse ein, das Framework liefert alle übrigen Achsen als Defaults → sofort ein
   vollständiger, vergleichbarer Algorithmus. Genau das realisiert das **Plugin-Controller-/Prüfling-Slot-
   Modell** (§2-§3): der Prüfling liefert nur seine spezifischen Achsen (`optional_prt_art_impl`), die
   cache-engine ergänzt die Defaults; die 3-Stufen-Permutation (§4) + der F15-Mess-Treiber (§4a) machen das
   Ergebnis messbar.

→ Jede Achsen-, Mess- und Framework-Arbeit dient diesem Zweck.

---

## 1. Die thesis-zentrale Kette (bewiesen)

```
cache-engine LÄDT prt-art als Plugin           [E11 Plugin-Controller]
   → prt-art FÜLLT CE-Achsen-Slots             [Phase B Slot-Füllung]
      → 3-Stufen MERGEN Slots → Permutations-Räume   [F.5-Kern: pruefling_merge → PermutationEngine]
         → count() = Binary-Set-Größe pro Stufe      [rein metaprogrammatisch, Compile-Time]
```

Jeder Pfeil ist im Plugin-Controller-Build (`cache-engine -DCOMDARE_CE_PRUEFLINGE=<prt-art>`) durch
`test_prt_art_pruefling_registration` (22 Tests) abgesichert.

---

## 2. E11 Plugin-Controller (bewiesen, 3/3)

Die cache-engine ist Framework **und** Plugin-Controller (Doku 20): sie lädt ein oder mehrere
Prüfling-Repos parallel auf Wurzel-Ebene, konfiguriert durch die Diplomarbeit via `COMDARE_CE_PRUEFLINGE`.

- **Mechanik:** Root-`CMakeLists.txt` Load-Loop inkludiert je Prüfling `<dir>/comdare_pruefling.cmake`.
  prt-art's Snippet registriert seinen Include-Pfad + Integration-Test (`comdare_pruefling.cmake`).
- **Registry:** `cache_engine/api/pruefling_registry.hpp` (`PrueflingRegistry : IPrueflingRegistry`,
  function-local-static Singleton). prt-art's `PrtArtPrueflingFactory : IPrueflingFactory`
  (`prt_art/identity/prt_art_pruefling_factory.hpp`) registriert sich via `register_prt_art_pruefling()`.
- **Richtung:** cache-engine → lädt → prt-art (NICHT prt-art → nested cache-engine; letzteres war Legacy,
  via **E6** entfernt — prt-art ist reines Plugin, Standalone-Build zeigt auf Geschwister-cache-engine).

---

## 3. Phase B Slot-Füllung (bewiesen, 4/4 Achsen, 21 Tests)

Der Prüfling liefert pro überschriebener Achse einen **CE-konformen Wrapper** + ein **Slot-Struct**
(`prt_art/slots/axis_*_slot.hpp`). Slot-Vertrag (`anatomy/pruefling_merge.hpp`):

```cpp
struct Slot {
    using PrueflingVariants = mp::mp_list<PrtArtWrapper>;   // CE-konforme Variante(n)
    static constexpr bool has_pruefling = true;
    static constexpr AnatomyGenus genus = AnatomyGenus::SearchAlgorithm;  // Gattungs-Constraint
};
```

| Achse | prt-art-Wrapper | CE-Concept | prt-art-Logik |
|-------|-----------------|------------|---------------|
| axis_07 prefetch | `PrtArtRedirectPrefetch` | PrefetchStrategy | RedirectNode-Fan-Out-Prefetch |
| axis_01 page-type | `PrtArtBPlusPageType` | PageTypeStrategy | density-getriebene internal-search-Wahl (25/50/75%) |
| axis_14 value-handle | `PrtArtChainRefHandle` | ValueHandleStrategy | Multi-Value-Linked-List |
| axis_11 telemetry | `PrtArtPerNodeCounter` | TelemetryStrategy | **F15-Anti-Pattern** (per-node counter) |

Das Pattern generalisiert über vier distinkte Achsen-Concepts. Die prt-art-Wrapper inkludieren
CE-Achsen-Header unter `libs/cache_engine/topics/`; der Plugin-Controller-Test-Build liefert diese
Roots (`tests/unit/CMakeLists.txt` Pruefling-Loop).

---

## 4. 3-Stufen-Permutation / Prüfungs-Dreigliedrigkeit (F.5-Kern bewiesen, 22/22)

`anatomy/pruefling_merge.hpp` ist ein **reines C++23-Compile-Time-Metaprogramm** (keine Runtime-Selektion,
konform „kein Runtime-Switch"):

| Stufe | Target | Merge | Semantik |
|-------|--------|-------|----------|
| 1 | `comdare_perms_ce` | `StufeOneAxis<Default>` = Default | CE-only, keine Prüfling-Beteiligung |
| 2 | `comdare_perms_pa` | `StufeTwoAxis` = `std::conditional_t<has_pruefling, PrueflingVariants, Default>` | ERSETZT-mit-Fallback pro Achse |
| 3 | `comdare_perms_full_join` | `StufeThreeAxis` = `mp_unique<mp_append<Default, PrueflingVariants...>>` | Union non-redundant |

Dispatch via `MergeAxis<MergeStrategy S, ...>` — `S` ist **Non-Type-Template-Parameter** (kein
Runtime-Switch). Genus-Constraint (`assert_pruefling_slot_genus<Slot>`) verhindert Cross-Genus-Joins
zur Compile-Time (`search_algorithm_permutation_engine.hpp`).

**Permutations-Raum (Binary-Set-Größe), bewiesen über die 4 echten Slots:**
- Stufe 1 = `n07·n01·n14·n11` (kartesisches Produkt der CE-Defaults)
- Stufe 2 = `1` (jede Achse prüfling-ersetzt → die prt-art-Identität, eine Komposition)
- Stufe 3 = `(n07+1)·(n01+1)·(n14+1)·(n11+1)` (CE + prt-art-Variante je Achse, distinkt)
- `Stufe3 > Stufe1 > Stufe2`

`PermutationEngine::count()` ist `constexpr` → die Binary-Set-Größe wird **zur Compile-Time** berechnet
(`test_prt_art_pruefling_registration.cpp::F5_DreigliedrigkeitPermutationSpace`). **Das ist der
„Binaries aus Rekombinationen zur Compile-Time"-Kern** — der Raum, den die Codegen-Targets
materialisieren würden, ist metaprogrammatisch bestimmt, bevor irgendein Binary gebaut wird.

---

## 4a. Das einheitliche `std::map<key,value>`-Interface + F15-Mess-Treiber (Kernziel)

**Konzept (Anatomie-Metapher, [[std-map-unified-interface]]):** Die Tier-Metapher dient dazu,
Algorithmen zu DEFINIEREN + ZERLEGEN, damit sie überhaupt VERGLEICHBAR werden. Jeder Such-Algorithmus
wird auf EIN einheitliches Interface zusammengeschnitten — bei (key,value) ist das **immer
`std::map<key,value>`** (der „Körperbau", der Vergleichbarkeit herstellt). Die 17 Achsen (Organe)
beschreiben, **WIE sich das Innere** dieser `std::map` verhält (dense-Array / sortierter Vektor / ART /
B+ / Patricia / …) — und das bestimmt die Performance.

**Daraus folgt das KERNZIEL (F15):** Zwei `std::map<key,value>` zu vergleichen ist **nicht hohl**,
sondern die Operationalisierung der Diplomarbeit-Frage. Identisches Interface, unterschiedliches
Innen-Verhalten durch Achsen-Wahl → messbar unterschiedliche Performance. „Bringt die CacheEngine
messbaren Wert?" ≡ „Bringt die Achsen-Konfiguration des `std::map`-Innenlebens messbaren Wert?".

**Die Messung ist ZWEIDIMENSIONAL (Join):**
- **Dimension 1 — Achsen-Ebene:** einzelne Achsen-Werte gegeneinander (z.B. zwei `search_algo`-Werte).
- **Dimension 2 — Algorithmus-Ebene:** ganze Algorithmen/Kompositionen gegeneinander.

Beides vergleicht Implementierungen DESSELBEN `std::map`-Interface mit unterschiedlichem Innenleben.

**F15-Mess-Treiber (Stufe A, `tests/unit/test_v41_anatomy_f15_measurement.cpp`, 4/4 grün):**
0. lädt die Codegen-DLLs (`load_all`, R5.I-Pattern) → Identität (`composition_name`) + Lifecycle.
1. **Dim 1:** `Array256SearchAlgo` (dense O(1)) vs `VectorU8U8SearchAlgo` (sortiert O(log n)).
2. **Dim 2:** `ArtComposition` vs `HotComposition` (volle Algorithmen, gemessen über ihr
   `C::search_algo` = dominante `std::map`-Lookup-Struktur).
Beide via echtem Welch-T-Test (`welch_t_test.hpp`).

**Empirisches Ergebnis (belegt F15, beide Dimensionen):**
- Dim 1: Array256 ≈ 44 µs vs VectorU8U8 ≈ 410 µs/Batch, t = −57, **p ≈ 2.8e-128**.
- Dim 2: ArtComposition ≈ 42 µs vs HotComposition ≈ 391 µs/Batch, t = −68, **p ≈ 1.3e-143**.

→ Sowohl die Achsen-Wahl ALS AUCH die Algorithmus-Wahl bringen einen **~9× messbaren,
hochsignifikanten** Performance-Unterschied. Die CacheEngine-Konfiguration bringt messbaren Wert.

> HINWEIS Dim 2: misst aktuell die dominante `search_algo`-Struktur der Komposition. Sobald weitere
> Achsen ins `std::map`-Innenleben routen (R5.B), erfasst dieselbe Mess-Stelle den vollen Algorithmus.

**Ehrliche Grenze:** Last läuft host-seitig über die Achsen-Implementierungen (die geladene DLL
exponiert via `IAnatomyBase` nur Metadaten + Lifecycle-State-Flips, keine CRUD-ABI); Latenz =
`steady_clock` (keine PMC). Last DURCH die DLL = additive ABI-Methode (Stufe B, R6).

---

## 5. Verbleibend: schweres Build-Subsystem (noch nicht implementiert)

Der Permutations-RAUM ist bewiesen UND als Binaries materialisiert:

- ✅ **F.5 Codegen-Build-Targets** `comdare_perms_ce/pa/full_join` (#15) — **DONE 2026-05-29**: alle 3
  Stufen materialisieren echte SHARED-DLLs end-to-end. Kette: prt-art-Slots → `PrtArtCompositionDemo`
  (codegen-fähig, `prt_art/slots/prt_art_composition_demo.hpp`) → `anatomy_codegen_tool`
  (`--external-composition`/`--no-known`) → `anatomy_codegen_runner` (`EXTERNAL_COMPOSITIONS`/`NO_KNOWN`)
  → `comdare_codegen_anatomy_module_list` → DLL. Verifiziert: Stufe-2 (1 DLL) + Stufe-3 (3 DLLs:
  CE art/hot + prt-art), Stufe-1 via R5.I-Pilot. Gated auf Plugin-Controller-Build + 2-Pass (Tool zuerst).
- **VERBLEIBEND F.5 / R5.G — präzise gescopt (Planrunde 2026-05-29):**
  - ✅ Die **Enumeration** ist BEREITS bewiesen: `test_v41_search_algorithm_permutation_engine.cpp`
    zeigt `for_each_composition_type` über den vollen 17-Achsen-kartesischen Raum (count = 3×2×1¹⁵ = 6),
    jede auto-enumerierte `AdHocComposition` ist `IsComposition` + via `for_each_abi_adapter` als
    `IAnatomyBase` (organ_count=17) instanziierbar.
  - ✅ **Materialisierungs-Kern GELÖST + bewiesen** (2026-05-29): Der Komma-Blocker (AdHoc-Typ-Ausdruck
    `AdHocComposition<A,B,…>` enthält Kommas → bräche das Makro) ist via neuem **variadischem Makro
    `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(T0,…,T16)`** gelöst (baut die AdHoc-Composition intern als Alias).
    Verifiziert end-to-end: `test_v41_anatomy_adhoc_codegen_macro` (1/1) — das Makro definiert die
    extern-C-Factory einer AdHoc-Permutation (= ArtCompositions 17 Achsen), `comdare_create_anatomy()`
    liefert IAnatomyBase (organ_count=17, composition_name="AdHocComposition", genus=SearchAlgorithm,
    run_workload/Stufe-B funktioniert). Auto-enumerierte Permutationen sind damit codegen-fähig.
  - ✅ **Auto-Emitter GELÖST + bewiesen** (2026-05-29): `type_name<T>()` (constexpr FQ-Typ-Name via
    `__FUNCSIG__`) + `all_axes_umbrella.hpp` (alle 17 Achsen in einem Include) + `emit_adhoc_modules<Engine>`
    (`adhoc_emitter.hpp`) schreiben pro enumerierter Permutation ein standalone-kompilierbares Modul-`.cpp`
    (Umbrella-Include + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC` mit den 17 FQ-Achsen-Typen). End-to-end
    verifiziert: `test_v41_anatomy_adhoc_dll_load` (1/1) — eine auto-emittierte AdHoc-Permutation →
    SHARED-DLL → via `AnatomyModuleLoader` geladen → `run_workload`/Stufe-B grün.
  - ✅ **R5.G SKALIERUNG GELÖST + bewiesen** (2026-05-29): `comdare-adhoc-emitter` (configure-time-CLI,
    `apps/adhoc_emitter/`) enumeriert den GANZEN Pilot-Raum + emittiert pro Permutation ein `.cpp`;
    `cmake/adhoc_emitter.cmake` (`comdare_run_adhoc_emitter` 2-Pass-Runner + `comdare_build_adhoc_modules`
    glob/`add_library`-Loop) baut JEDE emittierte Permutation automatisch zu einer eigenen SHARED-DLL.
    Verifiziert: Pass1 SKIPPED → Tool gebaut → Pass2 emittiert 3 `.cpp` → 3 SHARED-DLLs →
    `test_v41_anatomy_adhoc_autobuilt_load` (1/1) lädt ALLE 3 via `load_all` + misst jede (Stufe B).
    **Damit ist R5.G end-to-end UND skaliert geschlossen** (Raum → N Modul-`.cpp` → N DLLs → N Anatomien);
    offen bleibt nur die Ausweitung des Pilot-Raums auf den vollen kartesischen Produkt-Raum (Konfig-Frage,
    kein neuer Mechanismus).
  - **R5.B (separat) — Gap präzise analysiert (Planrunde 2026-05-29):** Heute misst
    `SearchAlgorithmAbiAdapter::run_workload` (abi_adapter.hpp §127) NUR `A::composition_t::search_algo`
    (instanziiert die Struktur direkt, insert/lookup); die 16 übrigen Achsen werden nicht durchlaufen.
    **Wurzelursache:** die Nicht-`search_algo`-Achsen sind derzeit **Compile-Time-Deskriptoren** (nur
    `name()`/`family_name()`/`is_*()`/`flag_suffix()` + Tags/Flags — z.B. `LeafOnlyCounter` hat KEINE
    operative `record`/`count`-Methode), während die operative insert/lookup-Logik in `search_algo`
    konzentriert ist. Die Deskriptoren machen Algorithmen **vergleichbar + selektierbar** (das ist ihre
    Anatomie-Rolle), tun aber zur Laufzeit (noch) nichts. **Folgerung:** „mehr Achsen ins `std::map`-
    Innenleben routen" ist KEIN bounded Increment — es ist gated auf (a) den **operativen Achsen-
    Vollausbau** (R7.2 Traversal / R7.3 Queuing+Concurrency / R7.4 Allocator-Body — #5/#6/#7: Achsen
    bekommen echte, an insert/lookup teilnehmende Implementierungen) UND (b) einen **Composition-Driver**,
    der `search_algo` an die operativen Achsen (allocator/node_type/memory_layout/prefetch/value_handle/
    telemetry) delegiert. Deskriptoren künstlich in `run_workload` zu „bolzen" wäre ein Quick-Fix
    (verboten — `[[no_quick_fixes]]`): die Achsen müssen erst operativ werden, dann misst Dim 2 / in-DLL
    automatisch den VOLLEN Algorithmus. R5.B ist somit ein Folge-Task NACH R7.x, kein eigenständiger.
- **R5.D/R5.E/R6** (#26): CacheEngineBuilder-CLI + extern-C-ABI + dlopen-Loader + Mess-Treiber
  (der eigentliche F15-Messlauf: bringt die CacheEngine messbaren Wert?).
- **F.2/F.3** (#12/#13): Achsen-zentrische Namespace-Restrukturierung + Legacy-Concept-Wiederverwendung.
- **Phase C Rest:** 8 `already_covered`-prt-art-Header (je 1 Konsument) — erst Konsumenten auf
  CE-Äquivalente migrieren, dann löschen (Doku 19 §2.1).
- **E4.1** (#22): 6 cache-engine-Submodule-Repos befüllen (Infrastruktur).

Jeder dieser Punkte ist eine fokussierte Mehr-Schritt-Integration; die in §2-§4 bewiesenen Mechanismen
sind ihre Grundlage.

---

## 6. Verifikations-Index (Repro)

```
cmake -S . -B build/msvc-release -DCOMDARE_CE_PRUEFLINGE="<pfad>/comdare-prt-art"
cmake --build build/msvc-release --config Release --target test_prt_art_pruefling_registration
./build/msvc-release/tests/unit/Release/test_prt_art_pruefling_registration.exe   # 22/22 PASSED
```

- 3 E11-Factory-Tests · 6 axis_07 · 4 axis_01 · 4 axis_14 · 4 axis_11 · 1 F.5-Permutations-Raum.
- Reiner CE-Standalone-Merge (Dummy-Typen): `test_v41_anatomy_pruefling_merge.cpp`.

### R5.G Auto-Emitter + Skalierung (CE-only, 2-Pass-Build)

```
cmake -S . -B build/msvc-release                                              # Pass 1 (R5.G SKIPPED)
cmake --build build/msvc-release --config Release --target comdare_adhoc_emitter_cli
cmake -S . -B build/msvc-release                                              # Pass 2 (Emitter laeuft, 3 DLLs)
cmake --build build/msvc-release --config Release --target test_v41_anatomy_adhoc_autobuilt_load
./build/msvc-release/tests/unit/Release/test_v41_anatomy_adhoc_autobuilt_load.exe   # 1/1 PASSED
```

- `test_v41_anatomy_adhoc_codegen_macro` (1/1, ABI_STATIC) · `test_v41_codegen_type_name` (2/2)
  · `test_v41_codegen_all_axes_umbrella` · `test_v41_anatomy_adhoc_dll_load` (1/1, eine auto-emittierte DLL)
  · `test_v41_anatomy_adhoc_autobuilt_load` (1/1, ALLE 3 auto-gebauten DLLs via `load_all` + Stufe-B-Messung).
- Helper: `libs/cache_engine/builder/codegen/{type_name,all_axes_umbrella,adhoc_emitter}.hpp` ·
  `apps/adhoc_emitter/` · `cmake/adhoc_emitter.cmake`.
