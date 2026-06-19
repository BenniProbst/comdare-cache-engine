# Erweiterungs-Leitfaden — COMDARE cache-engine (#169)

> **Zweck.** Wissenschaftliche Nutzbarkeit der profil-getriebenen Engine: (A) wie man HEUTE eine Messung
> startet (Profil schreiben + EIN Kommando) und (B) die file:line-präzisen ANPASSUNGS-STELLEN für drei
> Erweiterungs-Szenarien. Alle Stellen sind gegen den realen Code verifiziert (Commit `e823a2b`-Basis),
> nicht aus dem Gedächtnis zitiert.
>
> **Build-Modell (ehrlich, Pflicht-Lektüre vorab).** Es gibt KEINEN „inkrementellen Bau". `1 DLL = 1 TU =
> EINE vollständige 19-Achsen-Komposition` je `binary_id`. „Ohne alles andere neu zu bauen" gilt NUR für die
> QUELLE (man schreibt die 18 anderen Achsen-Organe nicht um) — NICHT für den BAU: jede betroffene
> Konfiguration wird als eigene volle 19-Achsen-DLL frisch gebaut. Einzige Inkrementalität = Resume (Skip
> identisch-versionierter DLLs/Zeilen). Vollständige Begründung + file:line:
> `docs/architecture/BUILD-MODELL-1DLL-1TU-KLARSTELLUNG.md`.

---

## Teil A — Nutzerfreundliche Steuerung: heute eine Messung starten

### Das Bedienmodell in EINEM Satz
**Messung = ein `comdare_thesis_profile`-XML schreiben/editieren + EIN PowerShell-Kommando ausführen.** Die
WHAT-Konfiguration (Lebewesen / variierte Achsen / Sweeps / SOTA-Reihen / Working-Set / Lauf-Optionen) kommt
VOLLSTÄNDIG aus dem Profil; das Kommando liefert nur Pfade/Toolchain/Output.

### Der klare Einstieg (Kommando)
```powershell
# vcvars64 muss aktiv sein (cl im PATH); CMake EINMAL konfiguriert (build/msvc-release/generated/ existiert)
pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 `
    -Profile "libs\cache_engine\algorithm_profiles\thesis_profiles\m3v2_study.profile.xml" `
    -MaxBinaries 150
# Per-Achsen-Sweep einer der 4 vertieften Achsen:
pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 -SweepAxis migration_policy
# Schneller Funktions-Smoke (4 Binaries):
pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 -RunTest
```
- Default-Profil, falls `-Profile` leer: `m3v2_study.profile.xml`
  (`build_and_measure_150_tiere.ps1:160-161`).
- Das Harness baut EINMAL den Host `run_lazy_150.exe`, setzt `COMDARE_PILOT_INCLUDES` + die Mess-Defines
  und ruft EINMAL die deklarative CEB-Eintritts-API auf.

### Die Kette dahinter (verifiziert)
1. **Harness** `build_and_measure_150_tiere.ps1` → baut Host, übergibt `profile:<pfad>[@<achse>]`
   (`:164`), `COMDARE_LOAD_PROFILE_DIR` (`:70`).
2. **Host** `run_lazy_150.cpp:main` → reine Argument-/Toolchain-Aufbereitung + EIN Aufruf
   `tlz::run_profile(RunProfileArgs)` (`run_lazy_150.cpp:143-172`).
3. **CEB-Eintritt** `run_profile` (`profile_run_entry.hpp:101`) → parst Profil EINMAL (`:105`), baut den
   Experiment-B+-Baum deklarativ (`build_profile_basis_levels`, `:118-120`), fährt Basis-Pass + SOTA-Reihen
   über EINE vereinigte SourceGenFn (`:139-142`) in EINE CSV (`:158-159`).
4. **Profil → Baum** `build_axis_levels` (`profile_to_tree.hpp:25`); **Profil-Parser**
   `parse_thesis_profile` (`xml_config_parser.cpp:209`).

### Ehrliche Lücken für Nutzerfreundlichkeit (benannt, nicht beschönigt)
- **Kein eigenständiges Schema-Validierungs-Kommando.** Ein fehlerhaftes Profil fällt erst zur Laufzeit auf
  (`parse_thesis_profile` → `nullopt` → `run_profile` Exit 5, `profile_run_entry.hpp:106-110`). Wünschenswert:
  ein `--validate`-Modus, der Achsen-`ref`/`value`-Namen gegen die Registry prüft, BEVOR gebaut wird.
- **Profil-Werte sind Magic-Strings.** Die `<value>`-Strings (z.B. `k_ary`, `node4`) müssen exakt die
  `W::name()` der Wrapper treffen; ein Tippfehler liefert eine leere Quelle (DLL „nicht baubar", ehrlich
  sichtbar, aber spät). Es gibt keine zentrale, generierte „gültige Werte je Achse"-Liste für den Autor.
- **Mehrere ENV-Hebel** (`COMDARE_RUN_SOTA`, `COMDARE_WORKLOAD_RECORDS`, `COMDARE_PLATFORM` …,
  `run_lazy_150.cpp:160-170`) übersteuern das Profil — mächtig, aber für Einsteiger intransparent. Sie sind
  im SCHEMA.md / hier dokumentiert, aber nicht im Kommando selbst sichtbar.

> Kleiner Usage-Doc-Vorschlag: ein `--validate`-Flag am Host (`run_lazy_150`) ergänzen, das das Profil parst
> und JEDEN `<value>` gegen `AxisRegistry` (`profile_to_tree.hpp:22`) prüft und die Treffer/Fehlschläge
> auflistet, ohne zu bauen. Reine Lese-Operation, kein DLL-Build — würde die häufigste Fehlerklasse
> (Magic-String-Tippfehler) vor dem teuren Build abfangen.

---

## Teil B — Die drei Erweiterungs-Szenarien (file:line-Anpassungs-Stellen)

### Szenario 1 — Neuer Algorithmus in eine bestehende Achse (neuer `search_algo`-Wrapper)

**Beispiel: ein 18. Such-Algorithmus `myalgo` (Vorlage: `axis_03a_search_algo_bst.hpp`).**

| # | Anpassungs-Stelle (file:line) | Was |
|---|---|---|
| 1 | NEU `libs/cache_engine/axes/lookup/axis_03a_search_algo_myalgo.hpp` | Wrapper-Header nach Goldstandard (Vorlage `axis_03a_search_algo_bst.hpp:39` Klasse, `:41` `enabled = flags::myalgo_enabled`, `:52` `name()=="myalgo"`, `:54` `flag_suffix()`, `:205-207` Concept-`static_assert`). |
| 2 | `libs/cache_engine/axes/lookup/axis_03a_search_algo_registry.hpp:8-35` (Include) + `:49-76` (`AllStrategies`-Liste) | `#include "..._myalgo.hpp"` + `MyAlgoSearchAlgo` in die `mp::mp_list` aufnehmen. `EnabledStrategies` (`:81`) filtert dann nach `enabled`. |
| 3 | `libs/cache_engine/axes/lookup/axis_03a_search_algo_flags.hpp.in:57-59` (neuer `#cmakedefine01`) + `:61-77` (neue `inline constexpr bool myalgo_enabled = …`) | Flag-Template-Eintrag. |
| 4 | `CMakeLists.txt:217` (neue `option(COMDARE_AXIS_03A_ENABLE_MYALGO … ON)`) + `:727` (`MYALGO` in die `foreach(_s03a …)`-Liste) | Build-Option + USE-Berechnung; `configure_file` (`:734-737`) erzeugt die `flags.hpp`. |

**Was NICHT angefasst werden muss (verifiziert):**
- `all_axes_umbrella.hpp:20` zieht die `axis_03a`-Registry, die ihrerseits alle Varianten inkludiert → der
  neue Wrapper ist im Emitter automatisch sichtbar. **Keine Umbrella-Änderung.**
- `topic_traversal_config_set.hpp:23` `StaticAxisVariants_03a = EnabledStrategies` → der neue Wrapper landet
  automatisch im Katalog. **Keine ConfigSet-Änderung.**
- `source_catalog.hpp:78` `CatalogAxes::L00 = mp_take_c<…StaticAxisVariants_03a, KSearch>` (KSearch=4) →
  Sichtbarkeit im Basis-320 NUR, wenn der neue Wrapper in den ersten `KSearch` der EnabledStrategies liegt;
  für einen reinen Per-Achsen-Sweep (`-SweepAxis search_algo`) zählt die volle Liste. **Keine
  Katalog-Code-Änderung nötig**, ggf. `KSearch` erhöhen, wenn der Algorithmus im 320er-Kreuz erscheinen soll.

**Profil-Stelle (Aktivierung in der Messung):** im `m3v2_study.profile.xml:40-42` `<axis ref="search_algo">`
einen `<value>myalgo</value>` ergänzen — der String MUSS exakt `MyAlgoSearchAlgo::name()` treffen
(`bst.hpp:52`-Konvention).

**DLL-Konsequenz (Build-Modell ehrlich):** Es entstehen so viele NEUE volle 19-Achsen-DLLs, wie es
Profil-Kombinationen mit `myalgo` gibt (je `binary_id` eine eigene DLL = eigene TU). KEINE bestehende DLL
wird „erweitert"; bestehende DLLs ohne `myalgo` bleiben unter Resume unverändert (Skip via `.version`-Sidecar,
`BUILD-MODELL…md:26-29`). Die QUELLE der 18 anderen Achsen wird wiederverwendet, nicht umgeschrieben.

---

### Szenario 2 — Neue Achse erstellen (eine 20. Achse)

Eine echte neue Achse ist die teuerste Erweiterung. Pflicht ist der **10-Komponenten-Goldstandard**
(`reference_axis_gold_standard_checklist.md`; Vorlage `topics/allocator/axis_06_allocator/`):

| # | Komponente (Goldstandard) | Vorlage-Pfad |
|---|---|---|
| 1 | Topic-Tag + Topic-Marker-Concept | `topics/allocator/axis_06_allocator/concepts/…concept.hpp` |
| 2 | TopicConfigSet (`StaticAxisVariants`) | `topics/allocator/topic_allocator_config_set.hpp` (analog `topic_traversal_config_set.hpp:21-33`) |
| 3 | Strategy-Concept (Pflicht-API) | `axis_06_allocator/concepts/axis_06_allocator_concept.hpp` |
| 4 | CEPS-Concept (`axis_tag`+`family_id`+`name`+`flag_suffix`) | `…/axis_06_allocator_cache_engine_permutation_concept.hpp` |
| 5 | Subaxes-Tags | `axis_06_allocator_subaxes_aa1_to_aa7.hpp` |
| 6 | CRTP-StrategyBase (3 `static_assert`) | `axis_06_allocator_strategy_base.hpp` |
| 7 | `flags.hpp.in` (`#cmakedefine01` + `inline constexpr bool`) | analog `axis_03a_search_algo_flags.hpp.in` |
| 8 | Registry (`AllVendors` + `is_enabled` + `EnabledVendors`) | `axis_06_allocator_registry.hpp` |
| 9 | N Vendor-Wrapper (Pflicht-API) | `axis_06_allocator_*.hpp` |
| 10 | CMakeLists (`option` + USE-`foreach` + `configure_file`) | `CMakeLists.txt:727-737`-Muster |

**Zusätzliche, achsen-zahl-erweiternde Anpassungs-Stellen (das ist der ABI-brechende Teil):**

| # | Anpassungs-Stelle (file:line) | Was / Warum ABI-brechend |
|---|---|---|
| a | `anatomy/composition_factory.hpp:49-72` (`AdHocComposition<T0..T18>` → `…T19`) + `:91-93` (`static_assert(sizeof...(Vs) == 19 → 20)`) | Die zentrale Komposition wächst von 19 auf 20 Slots. **ABI-brechend** (Tuple-Arität + Reihenfolge ändern). |
| b | `builder/codegen/adhoc_emitter.hpp:50-68` (`adhoc_macro_args<C>` — eine `add(type_name<typename C::neue_achse>())`-Zeile ergänzen) | Der Emitter muss den 20. FQ-Typ in den Modul-`.cpp`-Quelltext schreiben. Ohne diese Zeile fehlt die Achse in JEDER generierten DLL. |
| c | `builder/codegen/all_axes_umbrella.hpp:19-40` (Registry-Include) + Kommentar `:2`/`:10-13` (19→20) | Die neue Achsen-Registry muss über den Umbrella sichtbar sein, sonst „kein Member" beim perm-DLL-Build. |
| d | `source_catalog.hpp:76-103` (`CatalogAxes` — neuer `using L19 = mp_take_c<…,1>` + neuer `CatalogCfg<L19>` in der `PermutationEngine`-Liste) + `:117-129` (`catalog_static_levels` — neue `push_static_axis<L19>(lv,"neue_achse")`) | Der Katalog deckt sonst nicht den 20-Achsen-Raum ab; binary_id-Pfad würde nicht passen. |
| e | ABI/Observer-POD (`anatomy/observer_aggregate.hpp` + `cache_engine/abi/anatomy_module_abi_v1.hpp` + `abi_adapter.hpp`) | Wenn die neue Achse einen Observer beisteuert: neuen POD-Slot + ABI-Major-Bump (alte DLLs → `magic_mismatch`, bewusst). **ABI-brechend.** |
| f | Profil-Schema: `xml_config_parser.hpp:142-172` (`ThesisProfile`) — die Achse erscheint generisch über `permute_axes`; ein neuer `<axis ref="neue_achse">` braucht KEIN neues Struct-Feld, NUR einen Registry-Eintrag in der `AxisRegistry`-Map (`profile_to_tree.hpp:22`). |

**DLL-Konsequenz:** Eine neue Achse ändert die `AdHocComposition`-Arität → **ALLE** binary_ids/DLLs sind neu
(20-Achsen-Pfad ≠ 19-Achsen-Pfad), inkl. ABI-Major-Bump → alte DLLs werden vom Loader verworfen
(`magic_mismatch`). Das ist die maximal-invasive Erweiterung; sie baut den gesamten Lebewesen-Bestand neu.

---

### Szenario 3 — Entwurfsraum der Mess-Profile erweitern (neues `comdare_thesis_profile`-Feld)

**Präzedenzfall: Strang-A-Inc2** (`working_set_sweep` / `axis_sweeps` / `sota_series_set` / `run_options`).
Diese vier additiven Felder zeigen exakt die Kette für ein NEUES Profil-Feld:

| # | Anpassungs-Stelle (file:line) | Was |
|---|---|---|
| 1 | `xml_config_parser.hpp:142-172` (`struct ThesisProfile`) — neues Feld; Vorlage: `:168` `working_set_sweep`, `:169` `axis_sweeps`, `:170` `sota_series`, `:171` `run_options`. Ggf. ein Sub-Struct wie `ThesisAxisSweep` (`:117-120`) / `ThesisSotaSeries` (`:125-129`) / `ThesisRunOptions` (`:134-140`). | Datenmodell. |
| 2 | `xml_config_parser.cpp:273-305` (`parse_thesis_profile`) — neuer `if (auto const* x = root->child("…"))`-Block. Vorlagen: `working_set_sweep` `:276`, `axis_sweeps` `:278-285`, `sota_series_set` `:287-295`, `run_options` `:297-305`. | XML→Struct. |
| 3 | `profile_to_tree.hpp:25` (`build_axis_levels`) — falls das Feld den BAUM/die binary_id beeinflusst: neue `AxisLevel` ergänzen (statisch wie `:64` oder dynamisch wie `:75-84`). Felder, die NUR Selektion/Lauf steuern (wie Inc2), berühren `build_axis_levels` NICHT (sie ändern die binary_id nicht — SCHEMA.md §Rückwärts-Kompatibilität). | Baum-Wirkung (optional). |
| 4 | Treiber-Konsum in `profile_run_entry.hpp` (`run_profile`) — Vorlagen: `working_set_sweep`→`:148` `profile_working_set_sweep(tp)`; `axis_sweeps`→`:205-228` Sweep-Baum; `sota_series`→`:247-275` SOTA-Pässe; `run_options`→`:113` `profile_run_options(tp)`. | WHAT-Verarbeitung. |
| 5 | `algorithm_profiles/thesis_profiles/SCHEMA.md:28-31` (Element-Tabelle) — das neue Element dokumentieren (+ Rückwärts-Kompatibilitäts-Note `:33-38`). | Schema-Doku. |

**Beispiel-Profil** zum Editieren/Kopieren: `m3v2_study.profile.xml` (`<working_set_sweep>` `:75-80`-Umfeld,
`<axis_sweeps>` `:78-80`).

**DLL-Konsequenz:** Hängt vom Feldtyp ab. **Selektions-/Lauf-Felder** (wie die 4 Inc2-Felder) sind
`is_static=false`/rein deklarativ → sie ändern die `binary_id` NICHT (SCHEMA.md §RW-Kompatibilität;
`profile_to_tree.hpp:67-85` filtert dynamische Ebenen via `static_filter()` raus) → **keine** neuen DLLs,
nur andere Auswahl/Reihenfolge des Baus. Ein Feld, das hingegen eine STATISCHE Achsen-Ebene hinzufügt (in
`build_axis_levels` mit `is_static=true`), erzeugt neue binary_ids → neue DLLs für die betroffenen
Kombinationen (1 DLL = 1 TU).

---

## Anhang — Verifizierte Schlüsselstellen (Kurzindex)

- Build-Modell: `docs/architecture/BUILD-MODELL-1DLL-1TU-KLARSTELLUNG.md` (1 DLL = 1 TU, Resume = einzige Inkrementalität).
- Wrapper-Vorlage: `libs/cache_engine/axes/lookup/axis_03a_search_algo_bst.hpp`.
- Registry/Enabled-Filter: `axis_03a_search_algo_registry.hpp:49-81`.
- Flags: `axis_03a_search_algo_flags.hpp.in` + `CMakeLists.txt:217,727-737`.
- Umbrella (19 Registries): `builder/codegen/all_axes_umbrella.hpp:19-40`.
- Komposition (19 Slots): `anatomy/composition_factory.hpp:49-95`.
- Emitter (19 Args): `builder/codegen/adhoc_emitter.hpp:46-70`.
- Katalog/Sweep: `tests/unit/thesis_tiere/source_catalog.hpp:76-282`.
- Profil-Parser: `libs/common/serialization/xml_config_parser/xml_config_parser.{hpp:142-190,cpp:209-307}`.
- Profil→Baum: `libs/cache_engine/builder/experiment_tree/profile_to_tree.hpp:25-88`.
- CEB-Eintritt: `tests/unit/thesis_tiere/profile_run_entry.hpp:101-286`.
- Host/Einstieg: `tests/unit/thesis_tiere/run_lazy_150.cpp` + `build_and_measure_150_tiere.ps1`.
- Schema: `libs/cache_engine/algorithm_profiles/thesis_profiles/SCHEMA.md`.
- Goldstandard-Checkliste: `reference_axis_gold_standard_checklist.md` (10 Komponenten, Vorlage `topics/allocator/axis_06_allocator/`).
