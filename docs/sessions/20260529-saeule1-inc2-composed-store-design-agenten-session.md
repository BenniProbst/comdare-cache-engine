# Session — Säule-1 Increment 2 (ComposedStore<N,L,A>): Agenten-Design-Ergebnisse

**Stand:** 2026-05-29 · **Typ:** Understand→Design→Synthesize-Workflow (Doku 24 §6, Säule-1 Fortsetzung)
**Workflow:** `wcofj34mx` / Run `wf_95fdecc7-210` — 7 Agenten, ~662k Subagent-Tokens, 200 Tool-Uses.
**Zweck dieser Doku:** Die Agenten-Ergebnisse (Understand-Befunde + 3 Design-Linsen + Synthese-Blueprint)
elaborat für spätere Konsultation festhalten, BEVOR der Increment implementiert wird.
**Bezug:** [[reference_boost_mp11_offline_prerequisite]] (Build-Voraussetzung), Doku 24 §6 (RESUME-PLAN),
Doku 14 §1–§3/§11.3 (Organ-Metapher). Vorgänger: Increment 1 (node_type-Storage-Organ) = ce `176c60b`.

> **Frage des Increments:** Wie holen wir **layout (axis_05)** und **allocator (axis_06)** als ECHTE
> Storage-Achsen in das komponierbare Storage-Organ (`NodeTypeSlotStore<N>` → `ComposedStore<N,L,A>`),
> sodass es WEITER das `StorageOrgan`-Concept (uint64-Key) erfüllt und std::map-äquivalent bleibt —
> rein additiv und build-grün?

---

## 1. Understand-Phase (3 Reader, code-verifiziert)

### 1.1 axis_05_memory_layout — **TRAIT-ONLY, stateless**
- **Concept `MemoryLayoutStrategy`** (`axis_05_memory_layout_concept.hpp:11-16`) fordert **NUR** `cache_line_size() noexcept → size_t`. **Kein** `slot_index`, kein Slot-Permutations-API.
- **Goldstandard-Erweiterung `CacheEnginePermutationStrategy`**: axis_tag (HM1–HM4), family_id, name(), family_name(), flag_suffix(), enabled.
- **CRTP-Basis `MemoryLayoutStrategyBase`** erbt von `AxisBase` (cross-axis: get_compiler()/is_original_module()), 3-fach Concept-Check, stateless.
- **5 Layout-Wrapper:** CacheLineAligned (fid 1, 64B AoS, HM1) · AoSStrict (fid 2, packed, HM2) · SoA (fid 3, columnar, HM2) · PackedBitmap (fid 4, succinct, HM3) · **AoSoA** (fid 5, SIMD-tiled W=8, HM2, neu A4).
- **Operative API `scan_field_sum(buf, n, record_size)`** (static, NICHT im Concept): AoS-strided (record_size-Multiplikator) vs. SoA/PackedBitmap (contiguous, ignoriert record_size) vs. AoSoA (block-stride). Das ist ein 4-Byte-**Feld-Scan-Mess-Modell**, NICHT das key/value-Substrat.
- **Subaxes HM1–HM4:** alignment_strategy / data_organization / packing_density / stride_pattern (HM4 noch ohne Wrapper).
- **Konsequenz:** Layout kann **nicht** ohne Concept-Erweiterung die physische Slot-Anordnung tragen.

### 1.2 axis_06_allocator — **INSTANZ-basiert, mit STL/PMR-Adaptern**
- **Concept `AllocatorStrategy`** (`axis_06_allocator_concept.hpp:76-92`): `a.allocate(bytes, alignment) → void*`, `a.deallocate(p, bytes, alignment) noexcept` — **instanz-gebunden** (nicht statisch), + copy_constructible + is_nothrow_destructible + operator==.
- **`AllocatorStrategyBase::as_std_allocator<T>()`** (`axis_06_allocator_strategy_base.hpp:152-156, non-const`) liefert `StdAllocatorAdapter<T>`, der einen **`Derived*`** hält (`:127`) → **Lifetime-kritisch**.
- `as_pmr_resource()` (PMR-Adapter) ebenfalls vorhanden.
- **Pilot-Allocatoren:** `MimallocAllocator` (bei mimalloc-OFF transparenter `real=std`-Fallback → Build grün ohne Vendor-Lib) · `PmrResourceAllocator` (immer ab C++17 via `new_delete_resource`; `allocate` gibt bei OOM **nullptr** statt Wurf).

### 1.3 Ist-Storage-Organ (ce `176c60b`)
- **`StorageOrgan`-Concept = 8 Methoden** (slot_count/key_at/value_at/set_value_at/append_slot/insert_slot_at/erase_slot_at/clear) + 2 Typedefs (key_type/value_type == uint64). **KEIN noexcept** gefordert.
- `RawSlotStore` (vector) und `NodeTypeSlotStore<N>` (bounded std::array, `N::max_capacity()`) erfüllen es beide.
- `ComposedSearch` hält den Store **by value, default-konstruiert, nie kopiert/bewegt** (`composable_search.hpp:132`).
- Harness `verify_matches_std_map<Wrapper>(key_mod, query_max)`: 600 gemischte Ops über gestreute Keys + Lookup-Sweep + `occupied_count()==ref.size()`; nutzt `Wrapper w{}`.

---

## 2. Design-Phase (3 Linsen, Judge-Panel)

| Linse | Risk | Kern | Verdikt |
|-------|------|------|---------|
| **1 — Allocator-only-real** (vector-backed, `as_std_allocator<Slot>`; Layout+Node trait-only) | **low** | Speicher real aus A; L/N als Provenienz | **GEWÄHLT** |
| **2 — additive Adapter-Trait** (Node256+CacheLineAligned+Mimalloc; Layout-Stride via additivem Adapter-Trait) | medium | führt Layout-Stride über Zusatz-Trait ein | verworfen |
| **3 — voll-kompositional / slot_index** (alle 3 Achsen physisch wirksam, `L::slot_index`, AoS/SoA-Dispatch) | (als „voll" geframt) | echte Layout-Permutation | verworfen |

**Warum 2 & 3 verworfen (Synthese-Korrektur, code-verifiziert):** Beide erfinden `L::slot_index(logical,size,capacity)` bzw. layout-abhängige physische Slot-Anordnung. `MemoryLayoutStrategy` fordert aber **nur `cache_line_size()`** — ein echtes Slot-Permutations-Layout würde entweder das Concept erweitern (**bricht 5 Wrapper + deren static_asserts → nicht-additiv**) oder einen erfundenen Trait verlangen. Ansatz 2 nutzt zusätzlich fragilen `L::name()=="…soa"`-String-Vergleich im `if constexpr` (semantisch unbelegt). → **Echte Layout-Wirkung ist Folge-Increment.**

> **Wichtig:** Der U3-Understand-Reader skizzierte selbst ein `L::slot_index`-Design (7 von 8 Methoden ändern sich). Das ist genau der verworfene „volle" Pfad — als Referenz für den späteren Layout-Concept-Erweiterungs-Increment dokumentiert, NICHT für diesen.

---

## 3. Gewählter Blueprint (Ansatz 1 + Graft aus 3)

**`ComposedStore<N,L,A>`** im Namespace `comdare::cache_engine::nodes::axis_04_node_type` (neben `NodeTypeSlotStore<N>`).
- **Constraints:** `NodeTypeStrategy<N>` && `MemoryLayoutStrategy<L>` && `AllocatorStrategy<A>`.
- **Storage real aus A:** `using Slot = std::pair<uint64,uint64>; using SlotAlloc = A::StdAllocatorAdapter<Slot>; using Vec = std::vector<Slot, SlotAlloc>;` — **Member-Reihenfolge `A allocator_;` VOR `Vec slots_;`**, `slots_` ctor-init mit `allocator_.as_std_allocator<Slot>()` (Lifetime des `Derived*`).
- **8 StorageOrgan-Methoden** wie RawSlotStore (linear, **keine** Layout-Indirektion) → semantisch std::map-äquivalent, unbounded (kein length_error). Mutatoren **nicht** noexcept (vector wirft) — Concept-konform.
- **Layout L minimal-real (Graft aus 3, ohne Concept-Erweiterung):** `static constexpr cache_line_size = L::cache_line_size()` fließt in den Typ (static_assert-prüfbar); Alignment-Hinweis; optionale **Nicht-Vertrags**-Methode `organ_scan_field_sum(record_size)` ruft `L::scan_field_sum` über das Backing → Layout messbar (F15-Brücke), aber NICHT Teil von StorageOrgan.
- **Provenienz:** node_capacity()/organ_name()/layout_name()/allocator_name().
- **Rule-of-5:** Copy/Move **`= delete`** (Adapter hält `&allocator_`; korrekte Rematerialisierung = Folge-Increment; Test braucht es nachweislich nicht, da `ComposedSearch` by value default-konstruiert). Default-Dtor reicht.
- **Selbstbeweis** am Dateiende: `static_assert(StorageOrgan<PilotStore>)` + 2× `TraversalOrgan`.

**Pilot-Tripel:** `(Node4Layout, CacheLineAlignedMemoryLayout, MimallocAllocator)` + **PMR-Anker** `(…, PmrResourceAllocator)` als garantiert-grüner Fallback ohne externe Vendor-Lib.

### Kritische Korrekturen des Blueprints (gegen Prompt & Designs)
1. **8 StorageOrgan-Methoden, nicht 9** (key_type/value_type sind Typedefs).
2. **Kein noexcept** im Concept (`storage_organ_concept.hpp:50-52`).
3. **Layout trait-only** — `slot_index` existiert NICHT → Ansatz 2/3 verworfen.
4. **Allocator instanz-basiert** (`a.allocate`, nicht `A::allocate`) + copy/nothrow-dtor/operator==.
5. **Member-Reihenfolge + Copy/Move=delete** wegen `StdAllocatorAdapter` hält `Derived*`.
6. **`PmrResourceAllocator::allocate` → nullptr bei OOM** (kein Wurf) — dokumentiert, für Pilot irrelevant.

---

## 4. Implementierungs-Plan (jeder Schritt build-grün)

**Dateien:** NEU `libs/cache_engine/topics/nodes/axis_04_node_type/axis_04_node_type_composed_store.hpp` ·
ADDITIV `tests/unit/test_v41_topic_traversal.cpp` (Includes + 1 TEST). **Keine CMake-Änderung** (Test-Target hat
seit Inc1 `COMDARE_ALL_AXIS_GENERATED_DIRS`; nodes-Achse header-only).

- **A** — Header anlegen (8 Methoden + Provenienz + 3 static_asserts über Pilot-Tripel).
- **B** — Test additiv: Includes (composed_store + axis_05 cache_line_aligned + axis_06 mimalloc/pmr) + Aliase + `TEST(Saeule1_ComposedStore, AllocatorBackedStoreDrivesBothTraversalOrgansAsStdMap)` (key_mod=100000 → breiter uint64-Key, beide Traversal-Organe × {Mimalloc, PMR}).
- **C** — Build offline (vendored mp11), `build/msvc-release`, STATISTICS AUS.
- **D** — ctest: alle `verify_matches_std_map` PASS + `occupied_count()==ref.size()` + keine Regression (2012→2013).

---

## 5. Scope-Grenze (explizite Folge-Increments — NICHT in diesem Increment)

1. **Echte Layout-Wirkung** (`L::slot_index`/AoS-SoA-AoSoA-Interleaving) → braucht saubere Layout-Concept-Erweiterung (eigener Increment, gegen Goldstandard-Checkliste).
2. **Bounded `ComposedArrayStore<N,L,A>`** (N::max_capacity() als hartes Limit via Allocator) — Pilot nutzt unbounded vector.
3. **observe_all / statistics() / snapshot_t / ObservableAxis** (Doku 24 §2.2) — bleibt aus dem StorageOrgan-Vertrag; `COMDARE_CE_ENABLE_STATISTICS` AUS.
4. **Lebewesen-Wrapper-Umstufung** (Array256/BST/B-Baum → Reference-Compositions, Doku 14 §6, R7.2).
5. **Anatomie/abi_adapter/Composition-Anbindung** (art_reference.hpp, F15-Mess-Integration).
6. **Registry-Eintrag + PermutationEngine** (ComposedStore noch nicht vom CacheEngineBuilder konsumiert).
7. **TYPED_TEST-Vollmatrix** (4 NodeTypes × 5 Layouts × 27 Allokatoren) + echte externe Vendor-Allokatoren (jemalloc/tcmalloc/hoard).
8. **prt-art / optional_prt_art_impl-Slot** (orthogonal).

---

## 6. Nächster Schritt

Implementierung gemäß §4 (tag-gesichert, build-verifiziert vor Commit), dann Commit + Push (GitHub) +
da-Submodule-Pointer-Bump (Submodule-Sync). Danach RESUME-PLAN-Schritte 3–5 (Doku 24 §6).
