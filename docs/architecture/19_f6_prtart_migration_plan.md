# Doku 19 — F.6 prt-art→cache-engine Migrations-Plan (Reaudit 2026-05-28)

**Stand:** 2026-05-28
**Quelle:** F.6.0-Reaudit (14 Achsen gegen aktuellen cache-engine-Stand, 21 Achsen). Ersetzt den veralteten 2025-05-25-Audit.
**prt-art HEAD:** d1b6140 | **cache-engine HEAD:** 30074f6

> Hinweis zur Zählung: Der Reaudit listet 14 prt-art-Achsen. Die Achse `nodes` aggregiert dabei
> Header, die physisch bereits unter `identity/`, `default_lookup/` und `traversal/` liegen
> (re-listing). §1/§2 zählen jede **physische Datei genau einmal** (44 unique Header). §3
> reproduziert die Achsen-Sicht treu aus der JSON, inkl. der Aggregations-Re-Listings der
> `nodes`-Achse — dort kann dieselbe Datei mit der achsenlokalen Klassifikation erneut auftauchen.

---

## §1 Zusammenfassung

| Klassifikation | Anzahl | Bedeutung / Aktion |
|----------------|-------:|--------------------|
| `already_covered` | 10 | cache-engine deckt das Konzept bereits ab (Goldstandard). prt-art erbt/ersetzt oder migriert minimal ein; kein Mehrwert im Behalten. |
| `basis_missing` | 14 | Echtes Basis-Konzept fehlt in cache-engine → **additiv migrieren** (Phase A). |
| `spezifisch` | 11 | prt-art-Prüfling-spezifisch → **bleibt** als `optional_prt_art_impl` (Phase B: auf Vererbung/Adapter umstellen). |
| `deprecated` | 9 | Leere Indikator-Marker / V32-Stubs → **löschen** (Phase C, gekoppelt an F.2-Ersatzmechanismus). |
| **Gesamt** | **44** | |

---

## §2 Aktionslisten (die eigentliche Arbeitsliste)

### §2.1 LÖSCHEN (deprecated + already_covered ohne Mehrwert)

Die 9 `deprecated`-Marker sind allesamt leere Namespace-/Indikator-Header (Auto-Permutations-
Signal) bzw. der zugehörige Index. Die 10 `already_covered`-Header haben keinen eigenen Mehrwert
mehr — sie werden durch cache-engine ersetzt (ggf. nach minimaler Migration ihrer Datei) und in
prt-art entfernt.

**deprecated (9 — reines Löschen, kein Code):**

| prt-art-Datei | Grund | CE deckt ab |
|---------------|-------|-------------|
| `default_lookup/prt_art_11_telemetry_default.hpp` | Leerer Namespace-Marker | `ITelemetryStrategy` + 4 Kuehn-Varianten (LeafOnlyCounter, LeafOnlySampledCounter, RetroactiveAggregator, PerNodeCounter), `cache-engine/concepts/telemetry/` |
| `default_lookup/prt_art_12_hardware_default.hpp` | Leerer Namespace-Marker | `IHardwareStrategy` + SimdFamily/CacheLevelTarget/NumaStrategy/PrefetchHwInstruction/AtomicFamily Enums (`hardware_strategy.hpp`, V32.EE.5) |
| `default_lookup/prt_art_13_scheduling_default.hpp` | Leerer Namespace-Marker | `ISchedulingStrategy` + WorkerPoolLayout/HeteroCoreDispatch/CoRoutineStrategy/BatchGranularity (`scheduling_strategy.hpp`, V32.EE.5) |
| `default_lookup/prt_art_3b_cache_traversal_default.hpp` | Leerer Namespace-Marker (Auto-Permutation) | `ArrayDiscipline` (`disciplines/array_discipline.hpp`) + weitere Disziplinen |
| `default_lookup/prt_art_62_reclamation_default.hpp` | Leerer Namespace-Marker | `ComdareRcuMechanic` (P29 McKenney) + RCU/Hazard-Pointers/QSBR/Mark-Sweep (`reclamation/rcu_reclaim/`) |
| `default_lookup/prt_art_63_numa_default.hpp` | Leerer Namespace-Marker | `INumaAffinity` (Local/Interleave/Preferred/Bind, V32.EE.5) |
| `default_lookup/prt_art_64_huge_page_default.hpp` | Leerer Namespace-Marker | `portable_aligned_alloc()` (THP/explicit/none, `allocators/portable_aligned_alloc.hpp`) |
| `default_lookup/prt_art_82_locking_default.hpp` | Leerer Namespace-Marker | `ILockingMode` + LockingMode-Enum (ReadOnly/ReadWrite/Upgradeable/OptimisticValidation), V32.EE.5 |
| `default_lookup/prt_art_9_isa_default.hpp` | Leerer Namespace-Marker | `IPlatformProbe` (x86_64/ARM64/RISC-V + ISA-Detection, `platform/i_platform_probe.hpp`) |

> Anmerkung: Die JSON enthält die `default_lookup/*`-Marker zusätzlich re-gelistet unter der
> `nodes`-Achse (V32.FF.1, „no code") — selbe physischen Dateien, identische Lösch-Aktion.
> Dort taucht ferner `default_lookup/prt_art_default_lookup_registry.hpp` und
> `identity/prt_art_search_engine_adapter.hpp` als `deprecated` auf (siehe §3 nodes).

**already_covered (10 — durch cache-engine ersetzt; ggf. minimal-migrieren, dann in prt-art löschen):**

| prt-art-Datei | Grund | CE deckt ab |
|---------------|-------|-------------|
| `identity/prt_art_search_engine.hpp` | Architektur cache-engine-konform (Hybrid-Klasse, 3 Spezialisierungen) | `comdare::search_engine<Collection, ConfigPermutation>` (abi/search_engine) — erbt davon |
| `internal_search/array_256.hpp` | prt-art-Variante minimal (kein Statistics/Observer) | `Array256SearchAlgo` (SearchAlgoVariant/DensityClassifiedStrategy/SimdCapableStrategy/Statistics) |
| `internal_search/vector_u8_u8.hpp` | prt-art-Variante minimal | `VectorU8U8SearchAlgo` (+ iterable_aspect_t mit Dichte-Schwellen 10/20/30/50/70 pct) |
| `internal_search/vector_u16_u16.hpp` | prt-art-Variante minimal | `VectorU16U16SearchAlgo` (Balanced, u16-Discriminator, 65536 max) |
| `measurement/density_tracker.hpp` | prt-art konkret vs. CE Strategy-Klasse | `axis_11_telemetry::DensityTracker` (CRTP-Strategy, V41.F.6.1.R7.5.b) — erbt davon |
| `memory_layout/cache_line_aligned_layout.hpp` | reine Utility; Primitiven nach CE-common extrahieren | `axis_05_memory_layout_cache_line_aligned.hpp` (+ Alignment-Primitiven nach `common/alignment_utils`) |
| `traversal/search_algo_traversal.hpp` | nur V32-Skelett ohne echte Impl. | `axis_03a_search_algo` (Array256/VectorU8U8/VectorU16U16/Array65535 konkret, S01–S04) → **LÖSCHEN, keine Migration** |
| `traversal/traversal_mapping.hpp` | nur V32-Skelett ohne echte Impl. | `axis_03m_mapping` (DirectPlacement MP01, PoolRelative MP02) → **LÖSCHEN, keine Migration** |
| `value_handle/inline_handle.hpp` | prt-art = Basis-Impl., CE = Concept+CRTP | `axis_14_value_handle_inline.hpp` |
| `value_handle/external_handle.hpp` | prt-art simpler (nur offset+size) | `axis_14_value_handle_external_pool.hpp` |

### §2.2 MIGRIEREN nach cache-engine (basis_missing)

| prt-art-Datei | Zweck | Ziel-CE-Pfad (aus migration_action) | Hinweise |
|---------------|-------|-------------------------------------|----------|
| `concurrency/olc_with_reserved_blocks.hpp` | OLC mit reservierten Value-Blocks gegen Cache-Coherence-Storms (REV 6 §5.17) | `libs/cache_engine/topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp` (Implementierungsdetail / Specialization-Variante) | OLC-Basis existiert als `OlcOptimisticConcurrency` (family_id=4). Empf.: migrieren + per Template-Parameter `cacheline_aligned_variant` aktivierbar machen; CE erzwingt die Variante nicht (bleibt optional). `reserve_value_block()` ist prt-art-spezifisch. |
| `identity/prt_art_identity.hpp` | PermutationFlags-Bit-Kombination zur Identifikation der PRT-ART-Binary (11 Achsen-Banks) | `cache_engine/anatomy/identity.hpp` (o.ä.) als Golden-Standard-Permutation für die SearchAlgorithm-Lebewesen-Unterklasse (korr. 2026-06-03, s. Doc 30 §8.0: „Gattung" hier ersetzt — die 17/19-Achsen-Komposition/Permutation ist die Lebewesen-Unterklassen-Ebene UNTER dem SearchAlgorithm-Interface, NICHT die Gattung; Gattung = das Außen-Interface SearchAlgorithm/Container/Graph) | Echtes Basis-Konzept: ausgereiftes Permutation-Flags-System; `prt_art_permutation_flags()` + `prt_art_identifier()` als generisches Pattern verfügbar machen. R5/R7-Goldstandard-Anatomien als Vorlage. |
| `identity/status.hpp` | errno-style Status-Codes (status_ok=0 … status_concurrent_modification=9) + Helper | `cache_engine/concepts/status_code.hpp` oder `cache_engine/abi/status_codes.hpp` als Basis-Konzept aller ABI-Operationen | Goldstandard errno-Pattern (REV 7, 2026-05-13). 10 Codes + 2 Helper. `insert/erase/clear` könnten `status_t` statt void/bool returnen (optionale Verbesserung). |
| `internal_search/array_65535.hpp` | Direct-addressed Array (65535 Slots) für mittlere Dichte via uint16-Discriminator | `axis_03a_search_algo` — **NEUE** Klasse `Array65535SearchAlgo` (S-Familie-Erweiterung) | Echte Lücke: CE hat nur Array256 (Dense), VectorU8U8 (Sparse), VectorU16U16 (Balanced). Soll `DensityClassifiedStrategy` mit kDensityMinPercent=25, kDensityMaxPercent=50 implementieren. |
| `memory_layout/virtual_offset_address.hpp` | TLB-inspirierte Positionsberechnung aus k-Byte-Prefix (k_count ≤ 8) | *(kein CE-Pfad; migration_action = BLEIBT-als-optional_prt_art_impl, infrastructure-Subsystem)* | JSON-Klassifikation `basis_missing`, aber `migration_action` sagt **BLEIBT in prt-art** (kein cache-engine-Äquivalent, keine Redundanz). `compute()`+`capacity_for()`+`decompose()` = prt-art Slot-Position-Hash. |
| `memory_layout/byte_path.hpp` | Cursor-basierter Key-Prefix-Walker (peek/advance/consume) für TLB-Offset-Stack | *(kein CE-Pfad; migration_action = BLEIBT-als-optional_prt_art_impl, utility-Subsystem)* | JSON-Klassifikation `basis_missing`, aber **BLEIBT in prt-art** (CE nutzt Compile-Time-Strategy-Selection, kein Runtime-Prefix-Scanning). Nicht redundant. |
| `nodes/bplus_node.hpp` | Cache-line-aligned B+-tree-artiger Branching-Node mit Density-getriebener internal-search-Wahl | `topics/nodes/axis_04_node_type/axis_04_node_type_bplus_page.hpp` (ergänzend zu Node4/16/48/256) | CE hat nur ART-Knoten, keine B+-Page. Komplement zu `axis_02` path_compression. Density-Tracking + internal-search-Abstraktion = CE-Feature-Erweiterung. |
| `nodes/redirect_node.hpp` | Path-collapsed Suffix-Node für sparse Key-Verteilungen (CoCo-Trie-inspiriert) | `topics/nodes/axis_04_node_type/axis_04_node_type_redirect_node.hpp` (neuer Node-Typ, family_id=256 oder 512) | Knoten-Typ-Variante unter `axis_04`. Strukturell komplementär zu `ByteWisePathCompression` (axis_02 patricia: single-bit-Split vs. prt-art suffix-collapse). |
| `nodes/traversal/search_algo_traversal.hpp` | Achse-3A-Aggregator: Density-getriebene Auswahl des internen Such-Algorithmus | `topics/nodes/axis_02_path_compression/concepts/` als traversal-concept / Sub-Achsen-Framework zur Density-Klassifizierung | (Re-Listing unter `nodes`; in der `traversal`-Achse als `already_covered`/LÖSCHEN klassifiziert — siehe §3.) CE hat keine Density-Dispatch-Logik in axis_02. |
| `prefetch/distance_estimator.hpp` | Density-basierte Prefetch-Distanz-Schätzung mit Latency-Faktor | `libs/cache_engine/topics/prefetch/axis_07_prefetch/axis_07_prefetch_distance_estimator_impl.hpp` (Implementierungsdetail) | CE hat nur Wrapper `DistanceEstimatorPrefetch`; `estimate()`+Density-Heuristik+Latency-Faktor als native Logik übernehmen (als StrategyImpl verankern). |
| `prefetch/path_oriented_prefetch.hpp` | Pfad-orientierte Prefetch-Heuristik (Path-Tracking, Extrapolation, Hot-Path-Hints) | `libs/cache_engine/topics/prefetch/axis_07_prefetch/axis_07_prefetch_path_oriented_impl.hpp` | CE hat Wrapper `PathOrientedPrefetch`; Algorithmus (`suggest_next()`, Hot-Path-Hints V11.1) ist Diplomarbeit-Kern → native Integration als StrategyImpl + HotPathMixin. |
| `serialization/signaling_bits.hpp` | Varlen-Kodierung mit Signalisierungs-Bits: VarLenEncoder (LEB128) + SignalingStream | `libs/cache_engine/topics/serialization/axis_10_serialization/axis_10_serialization_primitives.hpp` (neue Low-Level-Primitiven) | CE hat nur Wrapper-Strategien (VarLenSerialization etc.), nicht die Low-Level Encoder/Decoder. Namespace: `comdare::prt_art::serialization` → `comdare::cache_engine::serialization::axis_10_serialization::primitives`. |
| `telemetry/leaf_only_counter.hpp` | LeafOnlyCounter (Haupt-Strategie) + PerNodeCounter (Anti-Pattern-Vergleich), Cache-Line-Padding | **TEILEN:** LeafOnlyCounter → bereits via `axis_11_telemetry_leaf_only.hpp`; PerNodeCounter + NodeAccessCount → BLEIBT als optional_prt_art_impl | CE hat Strategy-Skelett ohne Datenstruktur-Impl. `NodeAccessCount` (alignas(64)) + `PerNodeCounter` = prt-art F15-Validierungsinstrument (Mail Kuehn 2026-05-08). Empf.: CE auf ConcreteImpl erweitern ODER prt-art als Referenz behalten. |
| `value_buffer/linear_value_buffer.hpp` | Append-only Linear-Buffer mit Lazy-Deletion (Tombstone) + periodische GC mit Slot-Remapping | **HYBRID:** neue Strategie `AppendOnlyTombstoneBuffer` als Q-Hybrid (family Q10+) in `topics/queuing/axis_q1_queuing` ODER als prt-art-spezialisierter Adapter behalten | CE Q02 (AppendOnly) + Q09 (Tombstone) decken Teile ab, NICHT die Kombination mit `explicitCompact(→mapping)`. Domain-spezifisches Optimierungs-Pattern (ART-Blatt mit Lazy-GC). Bei R8-Pruefling-Integration: spezifisch+adapter behalten. |
| `value_handle/chain_ref_handle.hpp` | Handle für verkettete Multi-Value-Einträge (Linked-List-Heads, chain_length) | `axis_14_value_handle` (neue Subklasse / VH-Subaxis, „VH4") | Nicht in CE vorhanden. `chain_head_offset` + `chain_length` → MUSS als VH4-Strategie/Subaxis hinzugefügt werden (Linked-List-Verwaltung). |

> Migrations-Hinweis zu `virtual_offset_address.hpp` / `byte_path.hpp`: Die JSON klassifiziert diese
> als `basis_missing`, die zugehörige `migration_action` ist jedoch **BLEIBT-als-optional_prt_art_impl**
> (kein CE-Ziel, keine Redundanz). Sie sind daher faktisch Phase-B-Kandidaten (prt-art-spezifisch),
> nicht Phase-A-Migrationen — beim Start von Phase A überspringen.

### §2.3 BLEIBT als prt-art-spezifisch / optional_prt_art_impl (spezifisch)

| prt-art-Datei | Zweck | Warum prt-art-spezifisch |
|---------------|-------|--------------------------|
| `allocator/pool_descriptor.hpp` | PoolKind-Enum (A/B/C/D/R/V-static/V-dynamic) + PoolStatistics + PoolDescriptor | REV 6 §5.17 interne Abstraktion; CE `axis_06_allocator` hat keine Pool-Routing-Abstraktion, nur Vendor-Wrapper. |
| `allocator/pool_router.hpp` | Routing PageEncodingTag/ValueHandleTag → PoolKind (constexpr) | prt-art-spezifische Routing-Policy; CE-Vendor-Abstraktionen bilden interne Pool-Semantik nicht ab. Hängt an pool_descriptor. |
| `allocator/pool_set.hpp` | Verwaltet 7 Pools als std::array<PoolDescriptor,7> + Tracking + Aggregation | prt-art Runtime-Management; CE bietet Vendor-Registry + Strategy-Base, KEINE Pool-Verwaltung. Hängt an pool_descriptor + pool_router. |
| `default_lookup/default_lookup_registry.hpp` | Registry der 9 default-lookup-Achsen mit Metadaten + CE-Pfaden | Pruefling-spezifisches Auto-Permutations-Registry; kein CE-Äquivalent (prt-art-Adapter-Header für CEB-AutoPermutator). |
| `identity/prt_art_search_engine_adapter.hpp` | ABI-Adapter: hybride PrtArtSearchEngine → comdare::search_engine via Komposition (Map/Vector/Tuple) | Generischer Adapter ist als `SearchAlgorithmAbiAdapter` schon da; dies ist prt-art-SPEZIALFALL. Such-Heuristik-Hooks (notify_density_threshold/hot_path/workload_change) prt-art-spezifisch. |
| `identity/prt_art_execution_engine_adapter.hpp` | Macht PrtArt zur ExecutionEngine-B (EE-B); EngineCallable/DI-Brücke, PrtArtHashBackend | CE hat EE-A (Standard); EE-B = prt-art-Surrogat für Baseline-Vergleiche. EngineCallable-Pattern als Optional-Pattern dokumentierbar, bleibt aber prt-art. |
| `measurement/hypothesis_metrics.hpp` | Thesis-Verifikations-Metriken H1 (page-type cost), H2 (code quality), H3 (inline-vs-external) | Spezifisch für PRT-ART-Validierung (Habich-Anforderung, REV 6 §5.17). Keine Redundanz zu CE `execution_result.hpp` (gleiche H-Namen, andere Semantik). |
| `memory_layout/multi_level_layout.hpp` | Cache-Ebenen-bewusste Slot-Stratifizierung (L1/L2/L3/Memory) via TierBudget + VirtualOffsetAddress | prt-art-Optimierungsheuristik; CE `axis_05` bietet Layouts (AoS/SoA), kein Ebene-Budgeting / Zugriffs-Lokalität. |
| `prefetch/redirect_prefetch.hpp` | Prefetch-Heuristik für RedirectNode-Subtree (Fan-Out-Slots) | RedirectNode ist prt-art-spezifische Struktur; CE `axis_07` modelliert sie nicht. Ggf. später als `axis_07_prefetch_redirect_subaxis.hpp` (PF4). Heute: prt-art-only. |
| `value_handle/value_handle.hpp` | Variant-Wrapper über Inline/External/ChainRef (Visitor-Pattern) | CE behandelt Strategien separiert (axis_14_*); das Variant-Konzept = Pruefling-Pattern für Runtime-Dispatch, in CE nicht repräsentiert. |
| `value_handle/cost_model.hpp` | WorkloadProfile-basiertes Kostenmodell (Inline vs. External vs. ChainRef, Laufzeit) | prt-art-Heuristik (H3-Hypothese, REV 6). CE nutzt Permutations-Compile-Time-Auswahl, keine Runtime-Kosten-Abschätzung. |

---

## §3 Map pro prt-art-Achse

Die folgenden 14 Abschnitte reproduzieren die Achsen-Sicht der JSON unverändert. Die Achse `nodes`
listet aggregierend Header re-, die physisch unter `identity/`, `default_lookup/` und `traversal/`
liegen — dort kann eine Datei mit der achsenlokalen Klassifikation erneut erscheinen.

### 3.1 `allocator` → CE: `axis_06_allocator`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `pool_descriptor.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | REV 6 §5.17; 7 Pools mit Tracking; CE hat keine Pool-Routing-Abstraktion |
| `pool_router.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | constexpr Routing-Policy; hängt an pool_descriptor |
| `pool_set.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | std::array<PoolDescriptor,7>; hängt an pool_descriptor + pool_router |

### 3.2 `concurrency` → CE: `axis_08_concurrency`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `olc_with_reserved_blocks.hpp` | basis_missing | MIGRIEREN → `axis_08_concurrency_olc.hpp` (Specialization-Variante) | OLC-Basis (OlcOptimisticConcurrency, family_id=4) da; als optionale cacheline_aligned_variant aktivierbar |

### 3.3 `default_lookup` → CE: `concepts + allocators + reclamation + platform`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `default_lookup_registry.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | Pruefling-Registry für Auto-Permutation; kein CE-Äquivalent |
| `prt_art_11_telemetry_default.hpp` | deprecated | LÖSCHEN | ITelemetryStrategy + 4 Kuehn-Varianten in CE |
| `prt_art_12_hardware_default.hpp` | deprecated | LÖSCHEN | IHardwareStrategy + Enums (hardware_strategy.hpp, V32.EE.5) |
| `prt_art_13_scheduling_default.hpp` | deprecated | LÖSCHEN | ISchedulingStrategy (scheduling_strategy.hpp, V32.EE.5) |
| `prt_art_3b_cache_traversal_default.hpp` | deprecated | LÖSCHEN | ArrayDiscipline + weitere Disziplinen in CE |
| `prt_art_62_reclamation_default.hpp` | deprecated | LÖSCHEN | ComdareRcuMechanic + RCU/Hazard/QSBR/Mark-Sweep |
| `prt_art_63_numa_default.hpp` | deprecated | LÖSCHEN | INumaAffinity (Local/Interleave/Preferred/Bind) |
| `prt_art_64_huge_page_default.hpp` | deprecated | LÖSCHEN | portable_aligned_alloc() (THP/explicit/none) |
| `prt_art_82_locking_default.hpp` | deprecated | LÖSCHEN | ILockingMode + LockingMode-Enum |
| `prt_art_9_isa_default.hpp` | deprecated | LÖSCHEN | IPlatformProbe (x86_64/ARM64/RISC-V) |

### 3.4 `identity` → CE: `anatomy (SearchAlgorithmAbiAdapter + IAnatomyBase) + abi/search_engine + abi/execution_engine`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `prt_art_identity.hpp` | ~~basis_missing~~ **korrigiert: spezifisch** | **BLEIBT-als-prt-art (Prüfling-Identität)** | Fehlklassifikation (Validierung 2026-05-29): generisches PermutationFlags-System (10 Banks + to_identifier) liegt BEREITS in `cache_engine/concepts/permutation_flags.hpp`. `prt_art_identity.hpp` *konsumiert* es nur + deklariert die PRT-ART-Bit-Kombination → Prüfling-Belang, vom Plugin-Controller gelesen. KEINE `anatomy/identity.hpp` nötig. |
| `prt_art_search_engine.hpp` | already_covered | ERBT-von `comdare::search_engine<Collection, ConfigPermutation>` | Hybrid-Klasse (3 Spezialisierungen), cache-engine-konform; status_t eigen |
| `prt_art_search_engine_adapter.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | Spezialfall des SearchAlgorithmAbiAdapter; Such-Heuristik-Hooks prt-art-spezifisch (V8.9) |
| `prt_art_execution_engine_adapter.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | EE-B (PrtArt-Surrogat); PrtArtHashBackend Dummy; EngineCallable-DI (V32.FF.2 + V34.A.2) |
| `status.hpp` | basis_missing | MIGRIEREN → `cache_engine/concepts/status_code.hpp` o. `abi/status_codes.hpp` | errno-Goldstandard (REV 7); 10 Codes + 2 Helper |

### 3.5 `internal_search` → CE: `topics/traversal/axis_03a_search_algo (Goldstandard V41.F.6.1)`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `array_256.hpp` | already_covered | MIGRIEREN → `axis_03a_search_algo_array256.hpp` (Array256SearchAlgo) | prt-art minimal (kein Statistics/Observer); semantisch identisch (O(1), 256 Slots) |
| `array_65535.hpp` | basis_missing | MIGRIEREN → `axis_03a_search_algo` — **NEU** Array65535SearchAlgo | echte Lücke (Fanout 256–65536); DensityClassifiedStrategy 25–50 % |
| `vector_u8_u8.hpp` | already_covered | MIGRIEREN → `axis_03a_search_algo_vector_u8u8.hpp` (VectorU8U8SearchAlgo) | CE-Pendant hat iterable_aspect_t (Dichte 10/20/30/50/70); semantisch identisch (sorted lower_bound) |
| `vector_u16_u16.hpp` | already_covered | MIGRIEREN → `axis_03a_search_algo_vector_u16u16.hpp` (VectorU16U16SearchAlgo) | Balanced, u16, 65536 max; CE ohne SimdCapableStrategy (Cost-DP nicht vektorisierbar) |

### 3.6 `measurement` → CE: `axis_11_telemetry bzw. execution_result.hpp (builder/commands)`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `density_tracker.hpp` | already_covered | ERBT-von `axis_11_telemetry::DensityTracker` (CRTP-Strategy, V41.F.6.1.R7.5.b) | prt-art konkret vs. CE Strategy-Klasse; konzeptuell äquivalent |
| `hypothesis_metrics.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | H1/H2/H3 Thesis-Verifikation (Habich, REV 6 §5.17); CE execution_result.hpp H1/H2/H3 andere Semantik |

### 3.7 `memory_layout` → CE: `topics/memory_layout/axis_05_memory_layout`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `cache_line_aligned_layout.hpp` | already_covered | MIGRIEREN → `common/alignment_utils` (Primitiven: aligned_offset/padding_to_next_line/lines_needed/is_aligned) | CE wrappt Alignment in Strategy; slot_view() prt-art-spezifisch (optional Helper) |
| `virtual_offset_address.hpp` | basis_missing | BLEIBT-als-optional_prt_art_impl (infrastructure-Subsystem) | TLB-style Offset-Addressing; kein CE-Äquivalent; nicht redundant |
| `byte_path.hpp` | basis_missing | BLEIBT-als-optional_prt_art_impl (utility-Subsystem) | inkrementelles Prefix-Parsing; CE = Compile-Time-Strategy, kein Runtime-Scan |
| `multi_level_layout.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl (policy/heuristics-Subsystem) | Ebene-Budgeting (L1/L2/L3/Memory); CE axis_05 = Daten-Org, nicht Zugriffs-Lokalität |

### 3.8 `nodes` → CE: `axis_02_path_compression + axis_04_node_type`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `bplus_node.hpp` | basis_missing | MIGRIEREN → `axis_04_node_type_bplus_page.hpp` | Komplement zu axis_02; CE hat nur ART-Knoten, keine B+-Page |
| `redirect_node.hpp` | basis_missing | MIGRIEREN → `axis_04_node_type_redirect_node.hpp` (family_id=256/512) | CoCo-Trie-inspiriert; komplementär zu ByteWisePathCompression |
| `identity/prt_art_search_engine.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl in `prt_art/identity/` | hybrider polymorpher Container; CE ohne Äquivalent |
| `identity/prt_art_execution_engine_adapter.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl in `prt_art/identity/` | EE-B-Adapter zu CEB DI; CE hat EE-A |
| `identity/prt_art_search_engine_adapter.hpp` | deprecated | LÖSCHEN oder fusionieren mit prt_art_execution_engine_adapter.hpp | v34-Stub, redundant; aktueller Adapter = prt_art_execution_engine_adapter.hpp |
| `identity/prt_art_identity.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl in `prt_art/identity/` | Pruefling-identity (permutation_flags/identifier); CE-Registry für eigene Achsen |
| `default_lookup/prt_art_9_isa_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | ISA via platform_probe; nur Doku |
| `default_lookup/prt_art_62_reclamation_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Reclamation via CE RCU; nur Doku |
| `default_lookup/prt_art_63_numa_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | NUMA via CE platform-aware allocators; nur Doku |
| `default_lookup/prt_art_64_huge_page_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Huge-Page via CE allocator; nur Doku |
| `default_lookup/prt_art_3b_cache_traversal_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Traversal-Prefetch via CE PathOrientedPrefetch; nur Doku |
| `default_lookup/prt_art_11_telemetry_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Telemetry via CE metrics; nur Doku |
| `default_lookup/prt_art_12_hardware_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Hardware via CE platform probe; nur Doku |
| `default_lookup/prt_art_13_scheduling_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Scheduling via CE scheduler (V32 NEU); nur Doku |
| `default_lookup/prt_art_82_locking_default.hpp` | deprecated | LÖSCHEN (Indikator-Marker V32.FF.1, no code) | Locking via CE concurrency; nur Doku |
| `default_lookup/prt_art_default_lookup_registry.hpp` | deprecated | LÖSCHEN (organisatorischer Index, keine Runtime-Relevanz) | nach Löschen aller Marker obsolet |
| `traversal/search_algo_traversal.hpp` | basis_missing | MIGRIEREN → `axis_02_path_compression/concepts/` (traversal-concept / Density-Dispatch) | komplement zu axis_04; CE hat keine Density-Dispatch-Logik |
| `traversal/traversal_mapping.hpp` | deprecated | LÖSCHEN oder KLÄREN (V31-Stub; Voraudit §4: traversal/* DEPRECATED V32-Stubs) | keine Analyse ohne Fileinhalt; per Voraudit DEPRECATED |

### 3.9 `prefetch (prt-art)` → CE: `axis_07_prefetch (Goldstandard, V41.F.6.1.R7.5.a)`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `distance_estimator.hpp` | basis_missing | MIGRIEREN → `axis_07_prefetch_distance_estimator_impl.hpp` | CE hat nur Wrapper; estimate()/Density-Heuristik/Latency-Faktor als StrategyImpl |
| `redirect_prefetch.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl (ggf. `axis_07_prefetch_redirect_subaxis.hpp`, PF4) | RedirectNode prt-art-spezifisch; CE axis_07 modelliert es nicht |
| `path_oriented_prefetch.hpp` | basis_missing | MIGRIEREN → `axis_07_prefetch_path_oriented_impl.hpp` | CE hat Wrapper; suggest_next()/Hot-Path-Hints (V11.1) = Diplomarbeit-Kern; StrategyImpl+HotPathMixin |

### 3.10 `serialization` → CE: `axis_10_serialization (topics/serialization)`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `signaling_bits.hpp` | basis_missing | MIGRIEREN → `axis_10_serialization_primitives.hpp` (Low-Level-Primitiven) | CE hat nur Wrapper-Strategien; VarLenEncoder + SignalingStream = Grund-Infrastruktur; Namespace-Mapping prt_art→cache_engine |

### 3.11 `telemetry` → CE: `axis_11_telemetry (Goldstandard, 4 Sub-Strategien)`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `leaf_only_counter.hpp` | basis_missing | TEILEN: LeafOnlyCounter → bereits covered (axis_11_telemetry_leaf_only.hpp); PerNodeCounter + NodeAccessCount → BLEIBT als optional_prt_art_impl | CE Strategy-Skelett ohne Datenstruktur; PerNodeCounter = F15-Anti-Pattern-Validierung (Mail Kuehn 2026-05-08) |

### 3.12 `traversal` → CE: `axis_03a_search_algo / axis_03b_cache_traversal / axis_03m_mapping`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `search_algo_traversal.hpp` | already_covered | LÖSCHEN | nur V32-Skelett; CE axis_03a = Goldstandard (S01–S04); keine Migration nötig |
| `traversal_mapping.hpp` | already_covered | LÖSCHEN | nur V32-Skelett; CE axis_03m (MP01 DirectPlacement, MP02 PoolRelative); VirtualOffsetCalculator vollständig erfasst |

### 3.13 `value_buffer` → CE: `topics/queuing/axis_q1_queuing (Q1) bzw. topics/value_handle/axis_14_value_handle (VH1–VH3)`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `linear_value_buffer.hpp` | basis_missing | HYBRID: neue Strategie AppendOnlyTombstoneBuffer (Q10+) ODER prt-art-Adapter behalten | CE Q02 (AppendOnly) + Q09 (Tombstone), aber nicht Kombination mit explicitCompact(→mapping). Bei R8: spezifisch+adapter |

### 3.14 `value_handle` → CE: `axis_14_value_handle`

| Datei | Klassifikation | Aktion | Notiz |
|-------|----------------|--------|-------|
| `value_handle.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | Variant-Wrapper (Inline/External/ChainRef, Visitor); CE behandelt Strategien separiert; Runtime-Dispatch-Pattern |
| `inline_handle.hpp` | already_covered | MIGRIEREN → `axis_14_value_handle_inline.hpp` (oder inline-enabled Flag) | CE hat Concept-Basis + CRTP; prt-art = Basis-Impl. (ersetzbar / optionaler Fallback) |
| `external_handle.hpp` | already_covered | MIGRIEREN → `axis_14_value_handle_external_pool.hpp` (oder Builder-Hilfklasse) | CE-Header vorhanden; prt-art simpler (nur offset+size) |
| `chain_ref_handle.hpp` | basis_missing | MIGRIEREN → `axis_14_value_handle` (neue Subklasse / VH4-Subaxis) | nicht in CE; chain_head_offset + chain_length; Linked-List-Verwaltung |
| `cost_model.hpp` | spezifisch | BLEIBT-als-optional_prt_art_impl | H3-Hypothese (REV 6); CE = Compile-Time-Auswahl, keine Runtime-Kosten |

---

## §4 Empfohlene Migrations-Reihenfolge (kleinschrittig, risiko-sortiert)

### Phase A — `basis_missing` additiv nach cache-engine migrieren (niedriges Risiko)

Diese Migrationen **ergänzen** cache-engine, ohne prt-art zu brechen: prt-art kann während der
gesamten Phase A unverändert weiterbauen, da nur neue Header/Klassen in cache-engine entstehen.

> **Phase-A-Status (Stand 2026-05-29): KOMPLETT.** Die genuin-additiven Basis-Migrationen sind
> **abgeschlossen**: status_code, signaling_bits, array_65535 (S09), distance_estimator + path_oriented
> (axis_07 StrategyImpl), OLC-reserved-blocks. Die ursprünglich als „Phase-A" gelisteten Telemetrie-/
> Node-/Handle-Punkte sind nach Validierung **deskriptor-seitig bereits covered** (axis_11/axis_01/axis_14) —
> ihre prt-art-spezifischen Datenstrukturen gehören als `optional_prt_art_impl` in **Phase B**.
>
> **`identity/prt_art_identity.hpp` ist KEINE Migration (Validierung 2026-05-29, korrigiert §3.2):** Das
> generische **PermutationFlags-System (10 Banks + to_identifier)** liegt bereits in
> `cache_engine/include/cache_engine/concepts/permutation_flags.hpp`. `prt_art_identity.hpp` *konsumiert*
> dieses nur und deklariert die **PRT-ART-spezifische Bit-Kombination** — ein **Prüfling-Belang, bleibt
> prt-art** (vom Plugin-Controller gelesen, um zu wissen welche Achsen-Slots PRT-ART füllt). Eine generische
> `cache_engine/anatomy/identity.hpp` ist **nicht nötig** (Redundanz vermieden).
>
> → **Es gibt keinen verbleibenden Phase-A-Basis-Port.** F.6 verschiebt sich auf **Phase B**
> (`optional_prt_art_impl`-Slotfüllung via Plugin-Controller, läuft end-to-end) + **Phase C** (destruktive
> prt-art-Header-Löschungen, autorisiert via Restore-Tag).

Reihenfolge innerhalb Phase A (von „reines Hinzufügen, keine Konsumenten" zu „Konsumenten anpassen"):

1. **Eigenständige Basis-Konzepte ohne Abhängigkeiten** (reines Hinzufügen):
   - ✅ `identity/status.hpp` → `cache_engine/concepts/status_code.hpp` (errno-Goldstandard) — **DONE** (7405524)
   - ✅ `serialization/signaling_bits.hpp` → `axis_10_serialization_primitives.hpp` — **DONE** (fe2508c)
   - ✅ `internal_search/array_65535.hpp` → neue `Array65535SearchAlgo` (S09, SA4 direct_multibyte,
     DensityClass::Balanced) in `axis_03a_search_algo` — **DONE 2026-05-29**. Korrektur ggü. Original:
     `kCapacity=65536` statt 65535 (Original-OOB bei discriminator==65535 behoben); Presence-Vektor
     statt Sentinel (jeder uint64-Wert gueltig). Typed-Test deckt automatisch ab + 5 spezifische Tests.
2. **Algorithmus-Logik in vorhandene CE-Wrapper einziehen** (StrategyImpl-Pattern):
   - ✅ `prefetch/distance_estimator.hpp` → `axis_07_prefetch_distance_estimator_impl.hpp` — **DONE 2026-05-29**
     (DistanceEstimatorImpl: estimate(density,latency)+clamp, constexpr; DistanceEstimatorPrefetch delegiert).
   - ✅ `prefetch/path_oriented_prefetch.hpp` → `axis_07_prefetch_path_oriented_impl.hpp` — **DONE 2026-05-29**
     (PathOrientedImpl: enqueue/suggest_next/reset + V11.1 note_hot_path_bytes; PathOrientedPrefetch stateful).
     Test 14/14 grün (6 neue F.6-Tests).
   - ✅ `concurrency/olc_with_reserved_blocks.hpp` → `axis_08_concurrency_olc.hpp` (optionale Variante) — **DONE** (fc038b3)
   - ⏭️ `telemetry/leaf_only_counter.hpp` — **KEINE Phase-A-Wrapper-Migration** (Validierung 2026-05-29,
     reconciled mit §3.11): Alle axis_11-Wrapper sind metadaten-only Deskriptoren; das Zählen erfolgt in
     der Mess-Schicht (CacheEngineBuilder misst, [[anatomie-nur-achsen-und-observer]]), NICHT im Wrapper.
     LeafOnlyCounter-Deskriptor ist **bereits covered** (axis_11_telemetry_leaf_only.hpp). Die Datenstruktur
     (NodeAccessCount + LeafOnlyCounter-Impl + PerNodeCounter = F15-Anti-Pattern-Validierung) **BLEIBT als
     optional_prt_art_impl in prt-art → Phase B** (vom Mess-Treiber konsumiert). Einbetten einer move-only
     atomic-Map in den Wrapper würde Muster + Kopierbarkeit brechen.
3. **Neue Node-/Handle-Typen** — **Deskriptor-Aspekt covered, Datenstruktur → Phase B**
   (Validierung 2026-05-29 + Session-Planrunde: B+/Redirect sind PAGE-TYPES (axis_01), NICHT axis_04
   Kapazitätsklasse). Die CE-Achsen liefern die **Deskriptoren/Defaults**; die prt-art-spezifischen
   **Datenstrukturen** werden als `optional_prt_art_impl` via Plugin-Controller in die Slots gefüllt:
   - ✅ `nodes/bplus_node.hpp` — Page-Type-Deskriptor covered: `axis_01_page_type_bplus.hpp` (BPlusPageType).
     B+-Page-Layout-Datenstruktur → Phase B optional_prt_art_impl.
   - ✅ `nodes/redirect_node.hpp` — Deskriptor covered: `axis_01_page_type_redirect.hpp` (RedirectPageType).
     Fan-Out-Slot-Datenstruktur → Phase B optional_prt_art_impl.
   - ✅ `value_handle/chain_ref_handle.hpp` — covered: `axis_14_value_handle_chain_ref.hpp` (ChainRef VH5, 91468cc).
     Linked-List-Verwaltung (chain_head_offset/chain_length) → Phase B optional_prt_art_impl.
   - ⏭️ `nodes/traversal/search_algo_traversal.hpp` — nur V32-Skelett (§3.12); CE axis_03a Goldstandard
     deckt S01–S09 ab → **keine Migration**, Phase-C-Löschkandidat in prt-art.
4. **Hybrid-/Entscheidungs-Migrationen** (Architektur-Entscheidung erforderlich):
   - `value_buffer/linear_value_buffer.hpp` → AppendOnlyTombstoneBuffer (Q10+) **oder** prt-art-Adapter
   - `identity/prt_art_identity.hpp` → `cache_engine/anatomy/identity.hpp` (Golden-Standard-Permutation)

> **NICHT in Phase A:** `memory_layout/virtual_offset_address.hpp` und `memory_layout/byte_path.hpp`
> sind JSON-seitig `basis_missing`, ihre `migration_action` ist jedoch **BLEIBT-als-optional_prt_art_impl**
> (kein CE-Ziel, keine Redundanz) → in Phase B behandeln.

#### Phase-A-Startliste (Datei → Ziel)

```
identity/status.hpp                         → cache_engine/concepts/status_code.hpp
serialization/signaling_bits.hpp            → topics/serialization/axis_10_serialization/axis_10_serialization_primitives.hpp
internal_search/array_65535.hpp             → topics/traversal/axis_03a_search_algo/  (NEU: Array65535SearchAlgo)
prefetch/distance_estimator.hpp             → topics/prefetch/axis_07_prefetch/axis_07_prefetch_distance_estimator_impl.hpp
prefetch/path_oriented_prefetch.hpp         → topics/prefetch/axis_07_prefetch/axis_07_prefetch_path_oriented_impl.hpp
concurrency/olc_with_reserved_blocks.hpp    → topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp  (optionale Variante)
telemetry/leaf_only_counter.hpp             → axis_11_telemetry_leaf_only.hpp  (nur LeafOnlyCounter ConcreteImpl)
nodes/bplus_node.hpp                        → topics/nodes/axis_04_node_type/axis_04_node_type_bplus_page.hpp
nodes/redirect_node.hpp                     → topics/nodes/axis_04_node_type/axis_04_node_type_redirect_node.hpp
value_handle/chain_ref_handle.hpp           → axis_14_value_handle  (NEU: VH4-Subaxis)
nodes/traversal/search_algo_traversal.hpp   → topics/nodes/axis_02_path_compression/concepts/  (Density-Dispatch-Concept)
identity/prt_art_identity.hpp               → cache_engine/anatomy/identity.hpp
value_buffer/linear_value_buffer.hpp        → topics/queuing/axis_q1_queuing  (AppendOnlyTombstoneBuffer Q10+)  [Entscheidung]
```

### Phase B — prt-art-spezifische Header auf Vererbung / `optional_prt_art_impl` umstellen

Erst nach Phase A (CE-Basis steht). Die `spezifisch`-Header bleiben in prt-art, werden aber wo
möglich auf die nun vorhandenen CE-Basen umgestellt (Vererbung statt Eigenimplementierung) bzw.
explizit als `optional_prt_art_impl` markiert. Ebenso die `already_covered`-Header, die durch CE
ersetzt werden (Vererbung/Delegation, dann Eigenimpl. entfernen):

- **Auf Vererbung umstellen (`already_covered`):** `identity/prt_art_search_engine.hpp` (erbt `comdare::search_engine`),
  `measurement/density_tracker.hpp` (erbt `axis_11_telemetry::DensityTracker`),
  `internal_search/{array_256,vector_u8_u8,vector_u16_u16}.hpp` (delegieren an axis_03a-Klassen),
  `value_handle/{inline_handle,external_handle}.hpp`, `memory_layout/cache_line_aligned_layout.hpp`
  (Primitiven aus `common/alignment_utils` nutzen).
- **Als `optional_prt_art_impl` markieren (`spezifisch`):** allocator-Trio (pool_descriptor/router/set),
  `default_lookup_registry.hpp`, identity-Adapter (search_engine_adapter, execution_engine_adapter),
  `hypothesis_metrics.hpp`, `multi_level_layout.hpp`, `redirect_prefetch.hpp`, `value_handle.hpp`,
  `cost_model.hpp`, sowie telemetry `PerNodeCounter/NodeAccessCount`.
- **Hier (nicht Phase A) behandeln:** `virtual_offset_address.hpp`, `byte_path.hpp` (BLEIBT-Aktion).

### Phase C — `deprecated`-Marker löschen — GEKOPPELT AN F.2

Die 9 (bzw. in der nodes-Aggregation re-gelisteten) `deprecated`-Header sind leere
Indikator-/Namespace-Marker, deren **bloße Existenz** dem CEB-AutoPermutator signalisiert, dass die
betreffende Achse auto-permutiert wird (Auto-Permutation). Sie dürfen daher **erst gelöscht werden,
wenn der Ersatz-Mechanismus aktiv ist**:

- Ersatz = `COMDARE_CE_PRUEFLINGE` / Registry-basierte Auto-Permutation (statt Datei-Existenz-Signal).
- **Phase C ist gekoppelt an F.2** (Pruefling-/Registry-Integration) und darf NICHT vor F.2
  durchgeführt werden, sonst geht das Auto-Permutations-Signal verloren.
- Betroffen: alle `default_lookup/prt_art_*_default.hpp` (9 Marker) + `default_lookup_registry`
  (als Index, sobald die Marker weg sind) + die transitionalen v34-Stubs
  `identity/prt_art_search_engine_adapter.hpp` (fusionieren) und `traversal/traversal_mapping.hpp`
  (V31-Stub klären/löschen).

---

*Ende Doku 19. Faithful synthesis aus F.6.0-Reaudit-JSON (44 unique Header, 14 Achsen). Keine eigene Analyse — Klassifikationen, Aktionen und Notizen stammen 1:1 aus dem Audit.*
