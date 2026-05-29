# s4 (Task #43) — Deep-Trie-Sezierung der OriginalXxx: Planrunde + erster Increment (ByteWiseKeyPrefix)

**Stand:** 2026-05-29
**Planrunde-Workflow:** `w82140m0g` / Run `wf_322187fc-330` (8 Agenten, 1,08M Subagent-Tokens, 251 Tool-Calls, ~29 min)
**Auftrag:** echte Trie-Anatomie der 5 OriginalXxx (ART/HOT/START/Wormhole/SuRF) in `node_type × path_compression
× Trie-Traversal`-Organe sezieren — über die IST-Body-Rekonstruktion (#41) hinaus.
**Bezug:** `[[feedback_no_whole_tier_axes_genus_configurator]]` (JEDES Tier — auch Original — in seine ECHTEN
Organe), Doku 14 §13, `[[feedback_web_research_per_algorithm_pflicht]]`, `[[feedback_no_runtime_switch]]`.

---

## 1 KRITISCHER BEFUND: echte Trie-Organe existieren nur im Paper-Code; Achsen-Pendants sind Platzhalter

**ART-Anatomie (aus `ext/traversal/P01-ART/unodb/art_internal_impl.hpp`, 3868 Z., Apache-2.0):**
- **node_type:** adaptive Node4/Node16/Node48/Node256 mit echtem Speicher + SIMD-`find_child`
  (Node4: `union{byte[4]|uint32}` + SSE `_mm_cmpeq_epi8`; Node16: EIN `_mm_cmpeq_epi8` über `__m128i`,
  16 Bytes parallel — SIMD-Vorzeige-Organ; Node48: `uint8[256]` child_index → `idx[48]`, O(1); Node256:
  `idx[256]` direct subscript) + Growth/Shrink (N4→N16→N48→N256 / zurück).
- **path_compression:** `union key_prefix` (Z.877-1070) = gepacktes uint64 mit bis 7 Prefix-Bytes +
  Länge im High-Byte; `get_shared_length` = XOR + `countr_zero(diff|clamp)>>3`; `cut`/`prepend`.
- **Trie-Traversal:** 256-Fanout-Byte-Descent (`get_internal` Z.1229-1260, `shift_right`-Prefix-Skip).

**Realitäts-Check (verifiziert):** Die heutigen `axis_04` `Node4/16/48/256Layout` sind reine **Tag-Klassen**
(nur `max_capacity()`/`name()`, KEIN keys-Array, KEINE children, KEINE SIMD). `axis_02` `ByteWisePathCompression`
ist Tag-only (`compression_ratio()=0.5`, erfundener Platzhalter). `ComposedStore<N,L,A>` ist ein flacher
`vector<pair<u64,u64>>` — KEIN Byte-Trie. Das `StorageOrgan`-Concept (Index-Slot-Array über EINEM uint64-Key)
ist strukturell **unfähig**, einen byte-dispatchenden Trie mit Prefix-Kompression abzubilden. ⇒ Die echten
Organe müssen real implementiert werden; sie existieren bisher nur im Paper-Code.

**Lizenz-/Semantik-Sonderfälle:** Wormhole (P07) = **GPL-3.0** → extern-C-Linking VERBOTEN (kontaminiert die
Apache/MIT-Engine) → nur eigenständige Re-Impl; `ext/P07-Wormhole/` ist zudem leer. SuRF (P10) = approximativer
**Range-Filter** (bool, false positives, KEINE exakten Werte) → erfüllt das `std::map<u64,u64>`-Interface
**strukturell nicht** → gehört als Filter-/View-Gattungs-Organ (axis_filter, LOUDS-Bitmap), NICHT in axis_03a
search_algo; heutiges `SortedBinaryOrgan`-Mapping ist semantisch falsch (eigene Konzept-Charge MIT User-Abstimmung).

---

## 2 GEWÄHLTER ANSATZ: Linse C (gestaffelte Hybrid-Sezierung)

ART zuerst + allein als tractable Charge, als **is_original=false C++23-Re-Impl** mit ECHTER Trie-Anatomie
(kein MSVC-Intrinsic-Risiko im ersten Schritt; echtes Apache-Linking + SIMD später gestaffelt). Das verifizierte
#41-BTree-Muster (Concept + Pool-Store +static_assert + stateless TraversalOrgan +static_assert + 17-Zeilen-
Composed + Doppel-Beleg + adversarialer Stress) 1:1 auf einen **ART-Trie-Pool** anwenden. node_type/
path_compression bleiben orthogonale Achsen (Permutierbarkeit = Kern-Beitrag). Growth ist DATEN-getrieben
(Knoten-Typ als Daten-Feld), KEIN Algorithmus-Runtime-Switch (`[[no-runtime-switch]]` gewahrt).

- **Verworfen Linse A** (reines Multi-Achsen, high): 3 riskante Umbauten an static_assert-geschützten Achsen
  gleichzeitig — zu breit.
- **Verworfen Linse B** (per-Tier-Pool als Monolith, high): opfert die Achsen-Orthogonalität.

**Staffelung:** ART (P01, Apache, 4/4) zuerst → START (P05, recyclet ART-Pool, Span-Param + uint16)
opportunistisch → HOT (P02, Bit-Patricia + SparsePartialKey-AVX-512) eigene Charge → Wormhole (GPL-Re-Impl)
+ SuRF (Filter-Gattung) eigene Chargen. STARTs Cost-DP-Self-Tuning = neue Achse `axis_03t_node_tuning` (separat).

**Increment-Reihenfolge (ART):** (0) Baseline grün → **(1) echtes ByteWise-key_prefix-Organ [DIESER COMMIT]**
→ (2) `art_trie_node_pool_concept` → (3) `art_trie_node_pool_store` (4 Knoten skalar + Growth/Shrink) →
(4) `art_trie_traversal_organ` (Byte-Descent + Prefix-Skip) → (5) `composed_art_trie_search` + Äquivalenz +
Grow/Shrink-Stress + Mapping-Ummappung `OriginalArtOrgan` → (6) SIMD hinter axis_09b-ISA-Guard (Skalar-Fallback bleibt).

---

## 3 ERSTER INCREMENT GELIEFERT (dieser Commit): echtes ByteWiseKeyPrefix-Organ

`axis_02_path_compression_byte_wise.hpp` von Tag zu echtem Organ ausgebaut — **additiv** (Tag-Klasse
`ByteWisePathCompression` + ihre static_asserts unangetastet):
- **NEU `struct ByteWiseKeyPrefix`** — originalgetreue Re-Impl des gepackten `unodb::key_prefix`
  (art_internal_impl.hpp:877-1070, is_original=false): `packed_` uint64 (Byte i in Bits [i*8..], Länge im
  High-Byte); `from_bytes`/`length`/`operator[]`/`common_prefix_len` (= `shared_len` XOR+`countr_zero>>3`,
  verbatim)/`cut`/`prepend`/`length_to_word`. Technischer Identifier (Metapher nur im Kommentar).
- **NEU Concept `ByteSkipPathCompression`** (in `concepts/axis_02_path_compression_concept.hpp`, SEPARAT +
  additiv zu `PathCompressionStrategy` → bestehende Strategy-static_asserts bleiben grün): length/operator[]/
  common_prefix_len/cut.
- `static_assert(ByteSkipPathCompression<ByteWiseKeyPrefix>)`.
- **Test** (angehängt an `test_v41_axis_02_axis_04_nodes.cpp`, keine neue CMake-Registrierung): 5 TEST-Cases
  (ConceptsSatisfied · LengthAndIndexing · CommonPrefixLenMatchesManualExpectation (4 manuell verifizierte
  Fälle) · CutShortensFromFront · PrependMergesAndRoundtripsWithCut (cut∘prepend = Identität)). Korrektheit
  via constexpr-`static_assert` (Compile-Zeit) + Runtime-ASSERTs. **ctest: 18/18 grün** (13 Bestand + 5 neu).

**Warum zuerst:** kleinster Schritt, KEIN Trie nötig, isoliert testbar, rückwärtskompatibel; das Descent-Organ
(Increment 4) konsumiert genau dieses Organ; heilt die erfundene `compression_ratio()=0.5`-Platzhalter-Lücke
mit echter, gegen den Paper-Code verifizierbarer Semantik.

---

## 4 Scope-Grenze + Folge-Chargen

**DRIN #43 (diese Charge):** komplettes echtes ART-Organ-System (axis_02 ByteWise-Prefix ✓ + 4 adaptive Knoten
in ArtTrieNodePoolStore + Growth/Shrink + ArtTrieTraversalOrgan + ComposedArtTrieSearch + Doppel-Beleg +
Grow/Shrink-Stress + SIMD Node4/16) + START opportunistisch.

**DRAUSSEN (eigene Folge-Chargen):** HOT-Familie (Bit-Patricia + SparsePartialKey-AVX-512); Wormhole (GPL-Re-Impl
Hash-Anchor; ext leer); SuRF (LOUDS + **Filter-vs-View-Gattungs-Designentscheidung + std::map-Interface-Bruch,
MIT User-Abstimmung VOR Code**); neue Achse `axis_03t_node_tuning` (STARTs Cost-DP); Iterator/seek/range-Organ;
node_type-Achse formal von Ein-Typ zu Growth-Policy extrahieren; echtes is_original-Linking (R7.6.c/#4, MSVC-
Intrinsic-Risiko); **#42 Umstufung-B** (Entfernung aus EnabledStrategies) — diese Charge SEZIERT nur.

**Web-Recherche-Pflicht je Tier** (vor jeweiliger Implementierung, Doku 18 autoritativ): ART Leis ICDE 2013
Apache-2.0; START Fent et al. ICDEW 2020; HOT Binna SIGMOD 2018 MIT; Wormhole Wu et al. EuroSys 2019 **GPL-3
bestätigen**; SuRF Zhang SIGMOD 2018 Apache-2.0 (Range-Filter-Semantik).

---

## 5 Verweise
- Planrunde-Output (vollständig): `tasks/w82140m0g.output` (82k+ chars; understanding[4] + designs[3] + blueprint).
- ART-Paper-Organe (Re-Impl-Quelle): `ext/traversal/P01-ART/unodb/art_internal_impl.hpp` + `node_type.hpp`
  (NICHT `legacy_code/paper_p01_art/` — dort nur orchestrierende `art.hpp`).
- Proven-Muster: `composable/btree_node_pool_concept.hpp` + `_store` + `btree_traversal_organ.hpp` + `composed_btree_search.hpp`.
- Vor-Charge: `docs/sessions/20260529-umstufungA-originalxxx-dissection-planrunde-session.md` (IST-Body #41).
