# `comdare_thesis_profile` — Schema (KF-2, 2026-06-02)

Diplomarbeit-ansteuerbares XML-Profil für den Cache-Line-Konfigurator. Ersetzt die hartcodierten
`codegen.cmake`-Profile (smoke/medium/full). **Additiv** — baut auf den bestehenden
`permutation_axes.xml` (11 Achsen) + `sota/*.profile.xml` (Paper-Originale) auf, ändert KEINE bestehende Datei.

Vollständiger Entwurf + Begründung: `docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md`.
Beispiel: `cacheline_study.profile.xml` (gleicher Ordner).

> **SUPERSEDED-Hinweis (2026-07-11, additiv):** Einzelne Zellen unten verweisen historisch auf
> `build_and_measure_150_tiere.ps1` (Z.30) bzw. den Host `run_lazy_150.cpp`/`run_lazy_150` (Z.31/34) als das, was
> ein Profil-Feld ersetzt. Diese `.ps1`/`run_lazy_150.*` wurden am 2026-07-11 bei der Behelfsweg-Bereinigung
> **entfernt**; der offizielle Mess-/Validier-Host ist heute **`Code/02_messung_driver`** (E4-XML). Die Verweise
> bleiben additiv als historischer Kontext erhalten.

## Wurzel
`<comdare_thesis_profile id="..." schema_version="1">`

## Elemente

| Element | Pflicht | Bedeutung |
|---|---|---|
| `<base_tiers>` / `<tier id profile_ref paper_ref/>` | ja | Paper-Originale = **statische Achsen-Tupel**. `profile_ref` → ein `sota/*.profile.xml`. Werden als EINE gekreuzte Dimension behandelt (inverse Auswertungslogik, ein Großexperiment statt 8 Separatläufe). |
| `<permute_axes>` / `<axis ref [per_organ]>` | ja | Die **dynamischen, cache-line-relevanten** Achsen (compile-time). Ohne Kind-`<value>` = volle Liste aus `permutation_axes.xml`; mit `<value>` = Teilmenge. **KANONISIERUNG (GO-4/GO-5-Nebenbefund, 2026-07-12):** `ref`-Namen + `<value>`-Werte sind die **kanonischen Registry-Namen** (`kCompositionAxisNames` + EnabledStrategies-`name()`, z.B. `memory_layout`/`node_type`/`cache_traversal`/`page_type`, `std_malloc`/`isa_amd64`) — `messung_driver --validate` prüft exakt dagegen. Die `permutation_axes.xml`-Aliasse der KF-2-Ära (`layout`/`node`/`traversal`/`page` + GROSS-Werte wie `X86_SSE42`/`SYSTEM_MALLOC`) sind **DEPRECATED** (die Datei bleibt als historische Referenz unverändert); ohne `<value>` expandiert heute die Achsen-Registry (EnabledStrategies) die volle **USE-enabled** Liste. |
| `<axis ref="cacheline" per_organ="…">` | ja (Kernthema) | **NEUE per-Organ Cache-Line-Unterachse** (KF-3). `per_organ` = Leerzeichen-Liste der Organe (page/node/traversal/allocator), die JEWEILS eine **eigene, unabhängige** Cache-Line-Einstellung tragen. Kinder: `<line_size>` (32/64/128/256; C1 2026-07-12: 32 additiv, KF-5 thesis-treu — 64 Standard x86-64/ARM64, 128 Azure/Power, 32 embedded, 256 z.B. s390x), `<alignment>` (none/cacheline_aligned/padded), `<sw_prefetch_hint>` (none/T0/T1/T2/NTA). Pro Organ also bis 4×3×5 = 60 (vor C1: 45) → bis **60ⁿ** für n Organe (beabsichtigt). |
| `<axis ref="node_width">` | nein (FF2-Studie) | **NEUE compile-time Unterachse „Knoten-Breite in Cache-Lines"** (C2/FF2, GO4/#8 F-C, 2026-07-12). Kind: `<width_in_lines>` (1/2/4/8/16). Bewusst eine **ANDERE Dimension** als die Line-**Größe** der `cacheline`-Unterachse: FF2 misst CSS/CSB⁺ (1 Cache-Line pro Knoten) gegen Hankins/Patel (16 Cache-Lines) nach. Sonderzweig wie `cacheline`: binary-id-relevant **NUR profil-aktiviert** (`profile_to_tree.hpp` → statische Sub-Ebene `node_width.width_in_lines`); ohne Aktivierung ändert sich KEINE binary_id. Konsum: Node-Organ deklariert die Breite (`NodeWidthAware`, `axes/cacheline/node_width_config.hpp`), der `LayoutAwareChunkedStore` macht das Chunk-/Knoten-Backing W Cache-Lines breit. Beispiel: `ff2_node_width_study.profile.xml`. |
| `<axis ref="alloc_hw">` | nein (F-B-Studie) | **NEUE compile-time Unterachse „NUMA/Page→allocator"** (F-B, GO4/#8, 2026-07-12). Kinder: `<numa_node>` (auto/0/1) und `<page>` (4k/2m). Verdrahtet die axis_12-Eigenschaften `numa_capable`/`memory_page_size`/`huge_page_capable` als **Compile-Time-Inputs an die Allocator-Kante** (axis_06): `gate_alloc_hw_for<HW>()` kompiliert Node-Pinning/2m-Hint weg, wenn die Plattform sie nicht trägt (`axes/alloc/alloc_hw_config.hpp`). Sonderzweig wie `cacheline`/`node_width`: binary-id-relevant **NUR profil-aktiviert** (`profile_to_tree.hpp` → statische Sub-Ebenen `alloc_hw.numa_node`/`alloc_hw.page`); ohne Aktivierung ändert sich KEINE binary_id. Konsum: `NUMAllocAllocatorBody` (Node-Bindung statt Hartcode −1) + `PoolResourceAllocatorBody` (Page-Hint → `pmr::pool_options`). HW-Gate-Grenze: der NUMA-**Effekt** braucht Multi-Socket-Hardware (~Sep); der Knopf existiert + ist dokumentiert. Beispiel: `fb_numa_page_study.profile.xml`. |
| `<compile_dims>` | ja | Compile-time Mess-Dimensionen (je Binary gebacken): `<workloads>` (YCSB A–F), `<telemetry mode silent>` (§8). |
| `<runtime_dynamic>` | nein | **OS-seitige dynamische Dimensionen**, vom CacheEngineBuilder am Prüf-Dock zur Laufzeit durchlaufen (Faustregel §7): `<thread_count>` (via `Algorithm_Resource_Control`, KF-4), `<hw_prefetcher>` (via MSR 0x1A4, KF-12). Additiv optional für Resource-Control: `<prefetch_distance>`, `<pool_budget_bytes>`, `<batch_size>`, `<inline_threshold_bytes>` als whitespace-getrennte uint64-Listen; `0` bedeutet Default/aus. |
| `<fixed_conditions .../>` | nein | Fixe reproduzierbare Bedingungen (Launcher, konstant): `turbo/smt/aslr/numa/governor`. |
| `<repetitions count interpolate overlay_in_chart/>` | ja | Wiederholungen (Default 3); `interpolate="false"` = **nie** gemittelt; alle Läufe separat (§9, KF-10). |
| `<modes>` / `<mode name merge active_axes [pruefling replaces_axes]>` | ja | Die 3 Permutationsmodi (`Stufe1_CeOnly`/`Stufe2_PrueflingReplace`/`Stufe3_FullJoin` aus `pruefling_merge.hpp`). `active_axes` = Achsen-Teilmenge, die in diesem Modus permutiert wird (deine 3-Modi-Achsen-Beschränkung). |
| `<static_axes from="base_tier"/>` | ja | Die nicht-permutierten Achsen nehmen die Werte des jeweiligen `base_tier` (Paper-Tupel). |
| `<constraints>` / `<require>` `<pin>` | nein | Cross-Constraints (z.B. `isa host_supported`, algorithmus-gebundene Pins) — kappen unsinnige Kombinationen vor der Enumeration. |
| `<key_value_signature>` | nein | `key_types`/`value_types` (wie `sota/*.profile.xml`). |
| `<working_set_sweep>{N-Liste}` | nein | **(S1-a, Increment 2)** Whitespace-getrennte Working-Set-Record-Zahlen der **äußeren Lauf-Iteration** (P-MD7). Ersetzt `COMDARE_WORKLOAD_RECORDS` + die PS-`foreach` in `build_and_measure_150_tiere.ps1`. Leer = einmaliger Lauf (rückwärtskompatibel). |
| `<axis_sweeps>` / `<axis_sweep axis baseline>` | nein | **(S1-b)** Per-Achsen-Sweep: variiert GENAU EINE Achse gegen eine **feste Baseline** (alle Ebenen Index 0, via `StaticBinaryView::flat_index`). Ersetzt `make_axis_sweep` + die `axis_to_level`-Map (`run_lazy_150.cpp`). `baseline="index0"` (aktuell einziger Marker). |
| `<sota_series_set>` / `<sota_series id lebewesen merge [fairness]>` | nein | **(S1-c)** SOTA-/PRT-ART-**Reihe** A/B/C: `merge` ∈ {`Stufe1_CeOnly`,`Stufe2_PrueflingReplace`,`Stufe3_FullJoin`} (die 3 Kompositionalen Joins, `pruefling_merge.hpp`). Ersetzt `sota_lebewesen_names`/`sota_series_ids`. Nutzt die bestehenden `merge`-Felder. **`fairness`** (GO-5 Fork 6, 2026-07-12, OPTIONAL): der Thesis-§sec:fairness-Modus der Reihe — `common_denominator` (gemeinsamer Minimalmodus: externe Werte-Handles, keine PRT-ART-Spezialpfade) ODER `native` (PRT-ART-Native: Inline-Umschaltung, Cache-Engine, Seitentyp-Scheduler); die Thesis fordert die TRENNUNG je Vergleich (beide Modi als eigene Reihen deklarierbar). Weggelassen = ungesetzt (heutiges Verhalten; CSV-Tag `-`). HEUTIGER Konsum: Durchreichung bis in die Runner-Spec (`SotaPass.fairness_mode`) + CSV-Endspalte `fairness_mode` + Resume-Stamp (`resume-v5`). Die Kompositions-Pinnung des common_denominator-Falls (`value_handle_external` + PRT-Spezialpfade aus) + die MESS-Abnahme sind **DATEN-gated** (#156/#162-Fenster) — bis dahin erzeugen fairness-Varianten desselben (lebewesen,merge)-Paars DIESELBE binary_id (das `--validate` warnt). |
| `<datasets>` / `<dataset id akte_ref loader/>` | nein | **(GO-5 Fork 1, Option A′, 2026-07-12)** Die Mess-INPUT-Dimension **D** (Dataset) ADDITIV im E4-Schema: je Eintrag referenziert `akte_ref` eine **test_data-AKTE** `Code/test_data_xml/<name>.test_data.xml` (super) — die Akten sind die **SINGLE-SOURCE** der Datensatz-Provenienz (GO-5 Fork 2/R2: der leere Legacy-Parser-Slot `test_data_sets.xml` ist DEPRECATED und darf NICHT nachträglich gefüllt werden — KEINE Doppelquelle). `loader` = `DatasetLoaderRegistry`-loader_id (`string_corpus` für die 6er-Kanon-String-Korpora; `sosd_uint64` für die binäre SOSD-Akte). Datasets sind Mess-INPUTS, keine Binary-Achsen → **binary_id-neutral** (golden-Roundtrip unberührt); fehlt `<datasets>` = heutiges Verhalten (synthetischer YCSB-Generator) byte-identisch. HEUTIGER Konsum: Lauf-Provenienz-Log + Resume-Stamp-Signatur (`profile_datasets_signature`; geänderte Deklaration invalidiert Resume konservativ). Der Loader-MESS-Konsum (`load_or_generate_ycsb(dataset_source=loader, dataset_id=akte)`) ist der **dokumentierte offene Folge-Schritt** (lauf-gated; Loader-Slot seit #184 hermetisch bewiesen). **Verortungs-Hinweis K (SUPERSEDED durch INC-3, 2026-07-14 — additiv, alter Wortlaut bewahrt):** ~~die dritte W/D/K-Dimension `<measurement_categories>` ist Phase-6-/DATEN-gated (#215) und wird bewusst NICHT gebaut~~ — die VERORTUNG (dieses E4-Profil-Schema, nicht der degradierte Messreihen-Pfad) war mit GO-5 Fork 1 fixiert; mit **INC-3 (Familie A vollenden)** ist `<measurement_categories>` nun **GEBAUT** (additiv; eigene Zeile unten): eine Spalten-**Projektion** über die 16 gemessenen System-Kategorien — **golden-neutral** (keine Kompositions-Achse → `binary_id` unverändert). |
| `<measurement_categories>` / `<category name/>` | nein | **(INC-3 Familie A, 2026-07-14)** Die dritte W/D/K-Dimension **K** (Mess-**Kategorie**) ADDITIV im E4-Schema — eine **Spalten-Projektion** über die **16 gemessenen System-Kategorien** (`cache_engine/measurement/measurement_category.hpp` / `MeasurementCategory`; `name` = exakt der Enum-Name, z.B. `CLU`, `CACHE_MISS_L1`, `LATENCY_P99`, `THROUGHPUT`). Es ist eine reine **Sicht-/Spalten-Auswahl über die gemessenen Kategorien, KEINE Binary-/Kompositions-Achse** → **binary_id-neutral** (golden-Roundtrip unberührt; die gepinnte Basis erzeugt dieselbe DLL/`binary_id`). Fehlt `<measurement_categories>` = heutiges Verhalten (**alle 16 Kategorien**) byte-identisch. **Single-Source der gültigen Namen** = `kMeasurementAxisRegistry` (`measurement_axis_registry.hpp`) — NICHT hartkodiert; `messung_driver --validate` lehnt einen getippten Namen (z.B. `LATENCY_P90`) als HARTEN Fehler ab (sonst stille leere/falsche Projektion), eine mehrfach genannte Kategorie ist eine WARNUNG (redundante Spalte). Feld: `ThesisProfile::measurement_categories` (Namen-Strings; die String→Enum-Auflösung lebt in der cache_engine-Schicht `validate_profile.hpp`, da `ThesisProfile` in der common-Schicht die Enum nicht referenzieren darf — Baseline-Layering, exakt wie `ThesisDatasetRef.loader`). |
| `<run_options cap platform build_version resume>` | nein | **(S1-d)** Lauf-Steuerungs-Defaults, die heute aus `argv`/`env` von `run_lazy_150` kommen (`cap`=max_binaries, `platform`/`build_version`=CSV-Tags, `resume`=Mess-Resume #139). Deklarativ; `argv`/`env` darf weiterhin übersteuern. **cap-Semantik (GO-4/GO-5-Nebenbefund, 2026-07-12, `profile_effective_cap`):** `cap="0"` **und** fehlendes `cap` = **KEIN Cap** → alle Basis-Zellen selektiert (gepinnt in `test_profile_roundtrip` (7)); `cap="N"` (N>0) = exakt N (auf die Basis-Zellen-Anzahl geklemmt). Vorher ergab `cap=0` eine **leere** Basis-Selektion — damit bekam `m3_golden_coverage` (`cap="0"` = dokumentiert „KEIN künstliches Cap") fälschlich keine Basis-320. |
| `<system_axes>` / `<compiler>` `<extension_hardware>` `<target_isa>` | nein | **(GN-3/§32-F4, 2026-07-19; A9-Drift-Nachtrag 2026-07-20 — bisher UNDOKUMENTIERT)** Die **System-Achsen-Familie** (BAU-/Host-Konfiguration, KEINE Kompositions-Achse) im Thesis-Kanal — geteilte `parse_system_axes`-Naht mit dem `comdare_experiment`-Kanal (EINE Lese-Funktion, GN-3). Kinder: `<compiler>` mit `<opt_level><option value="…"/>` (z.B. `O2`/`O3`; `Ofast` bewusst weggelassen — bricht IEEE-754-/Run-to-Run-Determinismus + den CRC64-golden-Anker) und optional `<atomic128><option value=>` (S2/A2); `<extension_hardware>` mit `<simd><option value="…"/>` (z.B. `no_extension`/`avx2`); optional `<target_isa><option value="…"/>` (z.B. `x86_64`). **binary_id-NEUTRAL** — die opt×simd-Wahl multipliziert NUR die BAU-Matrix/den Sidecar (`build_version +opt=/+ext=`), NIE die 17-Organ-Kartesik (`binary_id` unverändert); der Planer/CEB permutiert opt×simd real über den run_profile-Walk (ISA-gegated: `avx2` nur `x86_64`). Fehlt `<system_axes>` = heutiges Verhalten byte-identisch (die Felder `ThesisProfile::compiler`/`extension_hardware`/`target_isa` bleiben leer). `messung_driver --validate` prüft die opt_level-/simd-/atomic128-/target_isa-Werte gegen ihre Registries (Zähler `opt_levels`/`simd`/…). Beispiele: `all_axes_golden` (opt `O2`+`O3`, simd `no_extension`+`avx2`), `cacheline_study` (target_isa `x86_64`). |
| `<run_methodology>` / `<method value="…"/>` | nein | **(A9.1-Schema-Kern 2026-07-20 / A9.2-Füllung)** Die **Run-Methodik-Mess-UNTER-Achse** (Planer-delegiert): WELCHE Ablauf-Methode(n) der Mess-Vollzug fährt. `value` ∈ `kRunMethodologyRegistry` (`measurement/run_methodology_registry.hpp`: `debug` = parallel/schnell OHNE golden-Zahlen, `measure` = 1-Thread/deterministisch = die golden-Messung, `release` = Voll-Optimierung ohne Instrumentierung). Mehrere `<method>` = Sweep-Liste (Feld `ThesisProfile::run_methodology`). `--validate` prüft jeden Wert HART gegen die Registry (Single-Source im Code); leer = Skip (byte-identisch). **binary_id-NEUTRAL** (der Fan-out/Vollzug gehört S5). |
| `<measurement_framework name="…"/>` | nein | **(A9.1-Schema-Kern 2026-07-20 / A9.2-Füllung)** Die **Mess-Framework-Mess-UNTER-Achse**: WELCHES Last-/Mess-Framework den Workload treibt. `name` ∈ `kMeasurementFrameworkRegistry` (`measurement/measurement_framework_registry.hpp`: heute EIN ehrlicher Baustein `ycsb`, honest-1 — der reale YCSB-Generator). EINZELWERT (Feld `ThesisProfile::measurement_framework`). `--validate` prüft HART gegen die Registry; leer = Skip. **binary_id-NEUTRAL.** |
| `<writeback_methods>` / `<method value="…"/>` | nein | **(A9.1-Schema-Kern 2026-07-20 / A9.2-Füllung)** Die **Rückschrieb-Methoden-Mess-UNTER-Achse**: WIE die Mess-Ergebnisse persistiert werden — die ehrliche Formalisierung des `<output>`-Trios. `value` ∈ `kWritebackMethodRegistry` (`measurement/writeback_method_registry.hpp`: `csv` = Roh-Messzeilen/maschinen-lesbare Wahrheit, `latex_table` = formatierte Ergebnis-Tabelle, `comparison_metrics` = abgeleitete SOTA-Deltas). **KEIN `pdf`** — die PDF-Kompilation ist honest-0 KEIN Rückschrieb-Kanal der Mess-Maschine (das PDF entsteht Thesis-seitig). Mehrere `<method>` = Liste (Feld `ThesisProfile::writeback_methods`). `--validate` prüft HART gegen die Registry (`pdf` ⇒ HARTER Fehler); leer = Skip. **binary_id-NEUTRAL.** |
| `<measurement_tooling>` / `<combo tools="…"/>` | nein | **(A9.1-Schema-Kern 2026-07-20 / A9.2-Füllung)** Die **Mess-Tooling-HAUPT-Achse** (die AUFFÄCHERUNGS-Achse des CEB-Typs `[a,b,c]`): je `<combo>` eine Mess-INSTRUMENTIERUNGS-Konfig, `tools` = whitespace-getrennte Tooling-ids aus `kMeasurementToolingRegistry` (`measurement/measurement_tooling_registry.hpp`: `wallclock`/`macro`/`micro`). N Combos → N `ceb:build:[a,b,c]`-Strecken (§47/§55). **HEUTE PASSIV im Thesis-Kanal (SCOPE P-MESSTOOL):** das Feld `ThesisProfile::measurement_tooling` wird geparst, aber der Plan-Director ruft `measurement_combos_of(measurement_categories)` OHNE `tooling_configs` (Default `{}` ⇒ 1 Voll-Konfig `[all]`) — der `--dump-plan` bleibt **byte-identisch** (der N>1-Fan-out ist dormant bis S5/S6). `--validate` prüft die ids hier bewusst NICHT (deckungsgleich zum `comdare_experiment`-Kanal). **binary_id-NEUTRAL.** |

> **Rückwärts-Kompatibilität (Increment 2):** Alle vier Konstrukte sind **additiv** — fehlen sie (wie in
> `base_pilot`/`cacheline_study`), bleiben die `ThesisProfile`-Felder leer/Default; der binary_id-Round-Trip ist
> davon unberührt. Sie sind **Selektions-/Lauf-Konstrukte** (vom Treiber konsumiert), KEINE statischen
> Achsen-Ebenen → sie ändern die `binary_id` nicht. Die `runtime_dynamic`-Dimensionen (thread_count/hw_prefetcher)
> + die Wiederholungs-Achse emittiert seit Increment 2 **ausschließlich** `build_axis_levels` (Einzelquelle;
> die frühere Doppelquelle in `profile_runner.hpp` ist entfernt).

## Compile-time vs. Runtime (Faustregel)

- **Compile-time** (eigenes Binary): alle **Architektur-Achsen** inkl. per-Organ-`cacheline`, `workload`, `telemetry`-Modus.
- **Runtime** (CacheEngineBuilder am Prüf-Dock, in Env-/Ressourcengrenzen): OS-seitige, nicht-architektonische Größen —
  `thread_count`, `hw_prefetcher`, `prefetch_distance`, `pool_budget_bytes`, `batch_size`,
  `inline_threshold_bytes`, Affinität/NUMA. Plus Wiederholungen (Orchestrierung).

## Verarbeitung (Pipeline)

1. **Parser** (KF-1, tinyxml2) liest das Profil → erweitert `CacheEngineConfig` um `permute_axes`, `modes`,
   `runtime_dynamic`, `repetitions`, `cacheline`-Sub-Dim.
2. **Enumeration** (KF-9): kartesisches Produkt der `permute_axes` × `base_tiers`-Tupel, je `<mode>` per `active_axes`
   gefiltert; **Dedup** per FNV1a-Fingerprint (inverse Auswertung KF-15).
3. **CEB-Generator** (KF-8) erzeugt je Unique-`PermutationDescriptor` ein `perm_<id>.cpp` → SHARED-DLL.
4. **CacheEngineBuilder/Prüf-Dock** misst je DLL × `runtime_dynamic` × Wiederholungen (silent-mode Snapshot-Diff).
5. **Inverse Projektion** (KF-15) + **Thesis-Anbindung** (KF-14): Ergebnisse → bilingualer Mess-Anhang.

## Validierung VOR dem Bau (`--validate`, #169(A), 2026-06-19)

Ein **rein-lesendes** Validat prüft das Profil gegen die AxisRegistry/EnabledStrategies, **BEVOR** teuer gebaut/
gemessen wird — so fällt ein getippter `<value>` (z.B. `node_4` statt `node4`) oder eine unbekannte
`<axis ref="…">` SOFORT auf, statt erst nach langer Bau-/Mess-Wartezeit als „falsche Matrix"/„DLL nicht baubar".

> **SUPERSEDED (2026-07-11):** Die beiden gezeigten Aufrufe (`build_and_measure_150_tiere.ps1 -Validate`,
> `run_lazy_150 --validate`) sind entfernt (Behelfsweg-Bereinigung). Offizielles Validat heute:
> **`messung_driver --validate <profil>`** (`Code/02_messung_driver`, E4). Der folgende Block bleibt additiv als
> historischer Stand erhalten.

```powershell
# Harness-Schalter (kein DLL-Bau, keine Messung):
pwsh tests/unit/thesis_tiere/build_and_measure_150_tiere.ps1 -Validate -Profile <pfad>
# ODER direkt am Host:
run_lazy_150 --validate <pfad>
```

**Geprüft** (`validate_profile.hpp` / `tests/unit/thesis_tiere/test_validate_profile.cpp`):
1. jeder `<axis ref="X">` in `<permute_axes>` ist ein bekannter Achsen-Name (Registry-Key /
   `kCompositionAxisNames`; `cacheline` = KF-3-Sonderzweig, separat; `node_width` = C2/FF2-Sonderzweig, ebenso
   separat; `alloc_hw` = F-B-Sonderzweig, ebenso separat).
2. jeder `<value>Y</value>` ist ein **`name()` der EnabledStrategies dieser Achse** — die Fehlermeldung nennt die
   Achse + den ungültigen Wert + die gültigen Werte.
3. jeder `<axis_sweep axis=>` + jede `<sota_series lebewesen=>` referenziert eine deklarierte Achse / ein
   deklariertes `<base_tier>`.
4. **(GO-5 Fork 6, 2026-07-12)** `<sota_series fairness=>` trägt (falls gesetzt) nur die erlaubten Modi
   `common_denominator|native` (Thesis §sec:fairness); fairness-Varianten desselben (lebewesen,merge)-Paars
   ergeben eine WARNUNG (gleiche binary_id bis zur DATEN-gated Kompositions-Pinnung).
5. **(GO-5 Fork 1, 2026-07-12)** jeder `<dataset>` ist FORMAT-plausibel: `id` nicht leer + eindeutig,
   `akte_ref` endet auf `.test_data.xml` (die Akten = Single-Source, Fork 2/R2 — die DATEI-Existenz ist
   super-seitig und wird bewusst NICHT geprüft), `loader` nicht leer; ein loader außerhalb der Repo-Loader-ids
   (`string_corpus|sosd_uint64`) ist eine WARNUNG (Registry laufzeit-offen, aber Tippfehler fielen beim
   Mess-Konsum still auf den YCSB-Generator zurück).
6. **(M-CE-12, 2026-07-13)** jede `<compile_dims><workloads>`-id ist eine **REAL existente
   `load_profiles/`-id** (`ycsb_a..ycsb_f`, `lp_*`, `coco_*`, `ih`, `lh`) — die `<workloads>`-Auswahl ist die
   AUTORITATIVE Achse-2-Auswahl; eine getippte id (z.B. das alte `A` statt `ycsb_a`) matcht 0 Lastprofile und
   der Lauf bräche mit **exit 4** ab. Der Host reicht die via `discover_load_profiles` entdeckten ids herein;
   eine unbekannte id ist ein **HARTER Fehler**, leere `<workloads>` = „alle Lastprofile" (OK).
7. **(INC-3 Familie A, 2026-07-14)** jede `<measurement_categories><category name=>` ist ein **gültiger
   `MeasurementCategory`-Name** — die **Single-Source** der gültigen Namen ist `kMeasurementAxisRegistry`
   (`measurement_axis_registry.hpp`, die 16 System-Kategorien), NICHT hartkodiert. Ein getippter Name (z.B.
   `LATENCY_P90`) ist ein **HARTER Fehler** (er erzeugte sonst still eine leere/falsche Spalten-Projektion), eine
   mehrfach genannte Kategorie eine **WARNUNG** (redundante Spalte); fehlt das Element = „alle 16 Kategorien" (OK).
   Kategorien sind eine **Projektion**, keine Achse → **binary_id-neutral**.
8. **(A9.1-Schema-Kern / A9.2 S4-Delta B9, 2026-07-20)** die drei **PASSIVEN Mess-UNTER-Achsen** werden HART gegen
   ihre **constexpr-Registries** geprüft (**Single-Source im Code, KEIN hartkodiertes id-Literal**):
   `<run_methodology><method value=>` ∈ `{debug,measure,release}` (`run_methodology_registry.hpp`),
   `<measurement_framework name=>` ∈ `{ycsb}` (`measurement_framework_registry.hpp`, honest-1),
   `<writeback_methods><method value=>` ∈ `{csv,latex_table,comparison_metrics}` (`writeback_method_registry.hpp`;
   `pdf` ist ein **HARTER Fehler** = honest-0-Ausschluss der PDF-Kompilation als Rückschrieb-Kanal). Ein unbekannter
   Wert ist ein **HARTER Fehler**; leer = Skip (Zähler 0 = byte-identisch). Die Zusammenfassung zählt die geprüften
   Elemente (`… , N run_methodology, M measurement_framework, K writeback_methods`). Die **HAUPT-Achse**
   `<measurement_tooling>` reist hier PASSIV mit (nur Parse), wird aber im Thesis-Kanal bewusst **NICHT** validiert
   (SCOPE P-MESSTOOL, deckungsgleich zum `comdare_experiment`-Kanal). Alle vier sind **binary_id-NEUTRAL**
   (Planer-delegiert; der Fan-out/Vollzug gehört S5/S6).

**Quelle der gültigen Werte = der CODE, nicht hartkodiert:** die Registry kommt aus `build_all_axis_levels()`
(`registry_to_axis_levels.hpp`), die die realen `TopicConfigSet::StaticAxisVariants*`-Listen reflektiert
(`reflect_names`, `axis_reflect.hpp`). **Exit 0** + Zusammenfassung bei OK; **Exit != 0** + klare Meldung bei
Fehler. Garantiert rein-lesend: der `--validate`-Zweig kehrt VOR jeder `CompileFn`/`BuildOrchestrator`-Logik
zurück, es entsteht KEINE `perm_<id>.cpp/.dll`.
