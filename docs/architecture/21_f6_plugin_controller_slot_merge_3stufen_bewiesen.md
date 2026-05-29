# Doku 21 — F.6 Plugin-Controller + Slot-Füllung + 3-Stufen-Permutation: bewiesene Architektur

**Stand:** 2026-05-29 · **Status:** Kern-Mechanismen end-to-end bewiesen (Compile-Time) ·
**Bezug:** Doku 14 §18-§19 (Kompositionale Joins), Doku 19 (F.6-Migration), Doku 20 (Plugin-Controller-Anforderung)

Dieses Dokument konsolidiert die **end-to-end verifizierte** Architektur-Kette der Diplomarbeit und
grenzt sie vom noch offenen schweren Build-Subsystem ab. Alle hier als „bewiesen" markierten Aussagen
sind durch `static_assert`- bzw. GTest-Tests im Repo abgesichert (Pfade in Klammern).

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

## 5. Verbleibend: schweres Build-Subsystem (noch nicht implementiert)

Der Permutations-RAUM ist bewiesen UND als Binaries materialisiert:

- ✅ **F.5 Codegen-Build-Targets** `comdare_perms_ce/pa/full_join` (#15) — **DONE 2026-05-29**: alle 3
  Stufen materialisieren echte SHARED-DLLs end-to-end. Kette: prt-art-Slots → `PrtArtCompositionDemo`
  (codegen-fähig, `prt_art/slots/prt_art_composition_demo.hpp`) → `anatomy_codegen_tool`
  (`--external-composition`/`--no-known`) → `anatomy_codegen_runner` (`EXTERNAL_COMPOSITIONS`/`NO_KNOWN`)
  → `comdare_codegen_anatomy_module_list` → DLL. Verifiziert: Stufe-2 (1 DLL) + Stufe-3 (3 DLLs:
  CE art/hot + prt-art), Stufe-1 via R5.I-Pilot. Gated auf Plugin-Controller-Build + 2-Pass (Tool zuerst).
- **VERBLEIBEND F.5:** volle PermutationEngine-getriebene Auto-Enumeration (Codegen iteriert den GANZEN
  gemergten Raum via `for_each_composition_type` statt hand-Composition-Liste) — R5.G-Erweiterung.
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
