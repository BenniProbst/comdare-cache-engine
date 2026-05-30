# Session 2026-05-30 — #42 Umstufung-B KOMPLETT · SuRF S2 · START-Verifikation · Masstree KOMPLETT · Cross-Constraints · Säule-2 · R7.4

> **STATUS-UPDATE (Teil 2, nach diesem Blueprint geschrieben):** Der in §3 als „nächster Schritt" geplante
> Masstree-Blueprint ist VOLLSTÄNDIG AUSGEFÜHRT + adversarial verifiziert (0 Bugs). Danach folgten 7 weitere
> Charges (Cross-Constraints ISA×SIMD + ISA×Plattform, Säule-2-Mess-Pfad real, ART-Node-Shrink, queuing-Doku,
> R7.4 resource_ownership). Siehe **§5 Teil 2** am Ende + die erweiterte Commit-Tabelle. §3/§4 sind als
> historischer Plan markiert; der Ist-Stand steht in §5.

**Agent:** Thesis-Implementierungs-Agent (cache-engine), autonom unter aktivem `/goal` (Ultracode-Loop).
**Methodik durchgängig:** Design-Planrunde (echte Paper-Code-Lektüre) → Implementierung → adversariale Verifikation.
**4 reale Bugs** gefunden+regressions-gesichert. Submodul-Synchronität nach jedem Push.

## Commit-/Tag-Übersicht (cache-engine)

| Tag | Commit | Inhalt |
|-----|--------|--------|
| `v41-s4-inc7b-start-verifiziert` | `5c1698d` | START adversarial verifiziert (0 Bugs) + Coverage-Härtung (span-3, hohe Bytes, add_child-Assert) |
| `v41-42-phase1-rewiring` | `e31541a` | #42 Phase 1: alle 11 Konfiguratoren auf sezierte Organe (ObservableComposedContainer + paper_source) |
| `v41-surf-s2-inc1-louds` / `…-inc1b-oob-fix` | `05f12cb`/`ccdedf6` | SuRF S2 Inkrement 1: echtes LOUDS-Sparse-Filter-Organ (+ 2 Bug-Fixes) |
| `v41-42-phase2-deregistrierung` | `8bcbd7e` | #42 Phase 2: 13 Tiere via Flag-Gating deregistriert (EnabledStrategies 17→4) |
| *(Doc-Commit)* | `c82d151` | dieser Session-Doc (Blueprint-Stand) — danach folgten die untenstehenden Charges |
| — | `f9de770` | Masstree-Organ Fundament: Concept + Store + verifizierter kpermuter (S1/S2/S9) |
| — | `539e64f` | **Masstree-Organ KOMPLETT** — letzter Platzhalter-Konfigurator seziert (S3–S8) |
| — | `1a2b12a` | Masstree adversariale Verifikation `w2tra534f` — 0 Bugs, Churn-Coverage-Härtung |
| `v41-r5c3-isa-simd` | `f9b6303` | Cross-Constraint ISA×SIMD (#704): Permutationsraum-Korrektheit (mp_remove_if) |
| — | `4f499cd` | Säule-2: search_algo-Achse misst das ECHTE Composition-Organ (statt generisch SortedBinary) |
| — | `ce262d7` | Säule-2: Per-Achsen-observe_all-Statistik-Trace im Mess-Treiber (2. Mess-Dimension) |
| `v41-…-art-shrink` | `65f3102` | ART adaptiver Node-Type-Shrink beim Erase (N256→N48→N16→N4, std::map-treu) |
| `v41-isa-platform-cross-constraint-d29fdef` | `d29fdef` | **Cross-Constraint ISA×Plattform (#704-Erweiterung)** — 2. latenter Defekt geschlossen |
| — | `fdbb2bc` | queuing: CartesianQ1xQ2 bewusst ungefiltert begründet (kein #704-Defekt, orthogonal) |
| `v41-r74-allocator-resource-ownership` | `185f038` | **R7.4: resource_ownership() grenzt POOL gegen PMR typsicher ab** |

da-Pointer zuletzt: `15ca2cc` (→ ce `185f038`). Baseline-Tag `pre-42-umstufung-b`.

## 1. #42 Umstufung-B — VOLLSTÄNDIG (kritische Direktive erfüllt)

**Direktive (Doku 14 §3.1):** Achsen enthalten NUR Organe (Sub-Tasks), NIEMALS ganze Tiere.

- **Phase 1 (Composition-Rewiring):** Alle 11 Gattungs-Konfiguratoren (`compositions/*_reference.hpp` + `*_paper_binding_reference.hpp`) führen im `search_algo`-Slot das SEZIERTE Organ statt eines Tier-Wrappers.
  - NEU `composable/observable_composed_container.hpp`: tier-agnostische ObservableAxis-Hülle um die Container-Organe (verhindert stille Säule-2-Regression auf EmptyAxisSnapshot — die rohen `ComposedXxxSearch` sind nicht observable).
  - Paper-Bindings: `search_algo`→Organ + neuer `paper_source`-Slot (trägt is_original/SHA256/Habich, KEIN Achsen-Wert).
  - Test-Migration: `test_v41_anatomy` (ArtVsArtPaperBinding: jetzt gleiches Organ, Unterscheidung via paper_source) + `test_v41_compositions` (SearchAlgoVariant→IsDissectedSearchOrgan; „Shares"-Theoreme invertiert: jedes Tier hat sein EIGENES distinktes Organ).
- **Phase 2 (Deregistrierung):** CMakeLists option()-Defaults der 13 Tier-Flags → OFF. `EnabledStrategies = mp_filter<is_enabled, AllStrategies>` schrumpft 17→4. ENABLED bleiben NUR echte elementare Such-METHODEN: K-ary(S10), Interpolation(S11), Eytzinger(S12), LinearScan(S15).
  - **User-Entscheidung 2026-05-30:** die 4 flachen Tier-Platzhalter (Array256/VectorU8U8/VectorU16U16/Array65535) AUCH deregistrieren.
  - **Non-breaking + reversibel:** AllStrategies (mp_list) + Wrapper-Header + Direkt-Referenzen physisch erhalten. kSearch==17 / SimdSubset==6 / DenseSubset==2 (über AllStrategies) bleiben grün; count()/Cartesian (über EnabledStrategies) tautologisch grün. Reaktivierung via `-DCOMDARE_AXIS_03A_ENABLE_<X>=ON`.
  - **Stub-Subtilität:** deregistrierte OriginalXxx-Wrapper sind per Design No-op-Stubs (`if constexpr (enabled)` im Body). Runtime-Tests skippen sie (`if (!TypeParam::enabled) GTEST_SKIP()` in topic_traversal; `if constexpr (Tier::enabled)` in equivalence). Compile-time-Conformance läuft weiter über ALLE 17.

**Verifiziert grün:** topic_traversal 264 (+39 skip), permutation_engine 21, anatomy 13, compositions 25, observer 13, f15 26, axis_03a-equivalence 22, surf-filter S1 4 + S2 7, module_abi 10, execution_engine 11, paper_legacy_code 145, codegen 2+1.

## 2. SuRF S2 Inkrement 1 — echtes succinct LOUDS-Sparse-Filter-Organ

Seziert aus `ext/traversal/P10-SuRF` (Apache-2.0), portabel C++23 (std::popcount/countl_zero/byteswap; kein __int128/SSE). 4 neue Header in `axis_filter/composable/`: `surf_louds_bitvector.hpp` (SurfBitVector/SurfRank-512-LUT/SurfSelect-sample64), `surf_suffix_bits.hpp` (FP-Tuning-Organ, einseitiges check_equality), `louds_sparse_filter_store.hpp` (Single-Scan-Builder + level-major Suffix-Flatten), `louds_sparse_filter_organ.hpp` (lookupKey-Port + konservativer no-FN-Range). LOUDS-Sparse-only (Dense = Inkrement 2). SliceBytes-Pendant: Suffix-Typ/Länge als Compile-Time-Parameter.

**2 reale Bugs gefunden+gefixt:**
1. `distance_to_next_set_bit`-Überschuss am letzten LOUDS-Knoten (finaler `return distance` ins Padding statt `num_bits-pos`) → node_size zu groß → Binärsuche scannt Padding → bogus Leaf. Via Debug-Trace lokalisiert (suffixPos 839 > 800 Keys).
2. heap-buffer-overflow WRITE in `SurfBitVector::concatenate` (One-Past-End bei 64-Wort-Grenz-Carry). Via adversariale Verifikation (ASan) gefunden. Fix: +1 Carry-Wort.

Beweis (7/7 grün): no-FN HART, S2.contains≥S1.contains-Kreuzbeleg über 0..100000, FP-Monotonie + bits_per_key streng steigend über RealLen{0,4,8,16}, Range-no-FN vs std::set, Adversarial (Präfix-Ketten/Singleton/leer/Wort-Grenze).

## 3. Masstree-Organ (letzter Platzhalter): BUILD-REIFER BLUEPRINT — ✅ AUSGEFÜHRT

> **✅ ERLEDIGT (commits `f9de770` → `539e64f` → `1a2b12a`):** Der folgende S0–S9-Plan wurde
> VOLLSTÄNDIG umgesetzt. Das echte Masstree-Organ (`MasstreeLayerTraversalOrgan<SliceBytes=2>` +
> `MasstreeLayerNodePoolStore` mit nested `Kperm15` + `ComposedMasstreeSearch`) ersetzt den
> `ObservableSortedBinaryOrgan`-Platzhalter; `tier_to_organ_mapping.hpp`/`masstree_reference.hpp`
> routen das echte Organ. Adversarial verifiziert (Planrunde `w2tra534f`, **0 Bugs**): Churn-Stress
> (füllen/⅓-löschen/reinsert/Multi-Layer/komplett-leeren ≡ std::map), Layer-Boundary-Namensanspruch-Beleg,
> Variants≡ArtTrieOrgan, `<8>`-Degenerationsanker, Kperm15-Unit-Test. Der Plan unten ist HISTORISCH.

Planrunde `wxciy2wjk` (paper-verifiziert, `blocked: false`). `masstree_reference.hpp` nutzt noch `ObservableSortedBinaryOrgan` (Platzhalter). Ziel: echtes Masstree (Mao/Kohler/Morris EuroSys 2012) als 4-Datei-composable-Organ. is_original=false (masstree.hh ist template-only → Re-Impl). Single-Thread (OLC/RCU weg). C++23 portabel.

**KERN-FRAGE GELÖST (paper-verifiziert):** uint64-Key = genau 1 Masstree-Slice (`masstree_key.hh:62-65`, has_suffix nie true) → bei SliceBytes=8 ist die Layer-Maschinerie TOTER Code (nur „B+Baum mit kpermuter", Namensanspruch verletzt). **Lösung (START-Präzedenz `StartTrieTraversalOrgan<PolicySpan>`):** `MasstreeLayerTraversalOrgan<unsigned SliceBytes=2>` — SliceBytes<8 zerlegt den 8-Byte-Big-Endian-Key in `MaxLayers=8/SliceBytes` echte Slices; zwei uint64 mit gleichem führenden 2-Byte-Block + divergentem Rest ERZWINGEN `make_new_layer` (bis 4 echte Layer). `<8>` = paper-treuer Single-Layer-Degenerations-Anker (static_assert + Test). Big-Endian-Slicing (`std::byteswap`) garantiert: lexikografische Slice-Ordnung == numerische uint64-Ordnung == std::map-Ordnung.

**Masstree-Distinktion (3 strukturelle, paper-verifizierte Merkmale, sonst nur umbenannter B+Baum):**
1. **kpermuter-Knoten** (`kpermuter.hh:78-195`): Slots physisch fest, 64-Bit-Permutationswort trägt sortierte Reihenfolge. Insert = `leaf_alloc_slot`(=perm.back) + `leaf_perm_insert`(=insert_from_back, schiebt nur Nibbles) — KEIN memmove. Distinktion ggü. BTreeNodePool (physischer Array-Shift).
2. **B+tree-of-tries-Layering** (`insert.hh:62-126` make_new_layer; `get.hh` ksuf_matches=-8): voll-kollidierender Slice → neuer Sub-Layer-B+Baum (keylenx=128, lv_layer hält Sub-Wurzel). Bei SliceBytes=2 REAL erreichbar.
3. **ksearch-Binärsuche über Permutation** (`ksearch.hh:64-80`): In-Knoten-Suche über `perm[m]`.

**kpermuter (W=15, sized_kpermuter_info<2>, value_type=uint64) — GELESEN, exakte Arithmetik:**
- `make_empty()` = `0x0123456789ABCDE0 & ~15`
- `make_sorted(n)` = `(make_empty() << (n<<2)) | (full_value(0xEDCBA98765432100) & mask) | n`, mask = `(n==15 ? 0 : 16<<(n<<2)) - 1`
- `size()` = `x & 15`; `[i]` = `(x >> ((i<<2)+4)) & 15`; `back()` = `[14]`
- `insert_from_back(i)`: value=back(); `x = ((x+1) & ((16<<(i<<2))-1)) | (value << ((i<<2)+4)) | ((x<<4) & ~((256<<(i<<2))-1))`; return value
- `remove(i)`: if `(x&15)==i+1` then `--x`; else rot_amount=`((x&15)-i-1)<<2`, rot_mask=`((16<<rot_amount)-1) << ((i+1)<<2)`, `x = ((x-1)&~rot_mask) | (((x&rot_mask)>>4)&rot_mask) | (((x&rot_mask)<<rot_amount)&rot_mask)`

**Schritt-Plan (S0–S9):**
- **S0** PRE-READ Rest-Bodies: `masstree_split.hh` (leaf::split_into Z.50-121: mid=width/2+1, gleiche-ikey0-zusammenhalten, make_sorted(width+1-mid), split_ikey=nr->ikey0_[0]; internode::split_into Z.123 Median hoch), `masstree_remove.hh` (finish_remove=perm.remove), `string_slice.hh` (make_comparable=net_to_host Big-Endian), `masstree_struct.hh` (ksuf_matches Z.411: 1=Treffer<64, -8=Layer==128; layer_keylenx=128; leaf/internode_width=15).
- **S1** `masstree_layer_pool_concept.hpp` (Muster art_trie_node_pool_concept): `MasstreeLayerNodePool` — root/size/set_root/inc/dec/clear/free_node/is_leaf + Leaf-kpermuter-API (new_leaf, leaf_permutation/set, leaf_alloc_slot=back, leaf_perm_insert=insert_from_back, leaf_perm_remove, leaf_size aus Permutation, leaf_slice_at/set, leaf_keylenx_at(8|128)/set, leaf_value_at/set, leaf_layer_at→NodeRef/set, leaf_next/prev B-link) + Internode (new_internode, inode_n/set, inode_slice_at/set, inode_child_at/set).
- **S2** `masstree_layer_pool_store.hpp` (Muster art_trie_node_pool_store): NodeRef=size_t Kind in Bits 56-63; kWidth=15, kLayerKeylenx=128, kInlineKeylenx=8. NESTED `Kperm15` (uint64, VERBATIM-Arithmetik oben). `struct Leaf{ uint64 perm; array<uint64,15> slice; array<uint8,15> keylenx; array<uint64,15> lv_value; array<size_t,15> lv_layer; size_t next=kNil,prev=kNil; }`. `struct Internode{ int n=0; array<uint64,15> slice; array<size_t,16> child; }`. vectors + fl_leaf_/fl_inode_ Free-Listen. `static_assert(MasstreeLayerNodePool<…>)`.
- **S3** `masstree_layer_traversal_organ.hpp` (Muster btree_traversal_organ + start_trie_traversal_organ): `template<unsigned SliceBytes=2> struct MasstreeLayerTraversalOrgan` — `static_assert(SliceBytes>=1 && <=8 && 8%SliceBytes==0)`; MaxLayers=8/SliceBytes; `be_key=std::byteswap`; `slice_at(be,layer)=(be>>((MaxLayers-1-layer)*SliceBytes*8))&mask` (MSB-Block zuerst). Statisch: descend_to_leaf (Internode-Binärsuche), ksearch_in_leaf (lower_bound über kperm[m]→{logical,phys}), insert_into (Treffer-keylenx8→update; Treffer-keylenx128→Layer-Abstieg+nächster Slice+retry; gleicher-Slice-divergenter-Rest→make_new_layer-Twig; freier Slot→alloc+perm_insert; voll→split mid=width/2+1 Median-hoch ggf. neue Wurzel), lookup_in (descend→ksearch→keylenx8/128-shift-retry), erase_from (descend→ksearch→perm_remove, Kollaps weggelassen=I4). static_assert für `<2>` UND `<8>`.
- **S4** `composed_masstree_search.hpp` (VERBATIM aus composed_start_trie_search): 17-Zeilen-Shell, occupied_count→pool_.size().
- **S5** `tier_to_organ_mapping.hpp`: #include + `MasstreeOrgan = ComposedMasstreeSearch<MasstreeLayerTraversalOrgan<2>, MasstreeLayerNodePoolStore>` + `ObservableMasstreeOrgan = ObservableComposedContainer<MasstreeOrgan>`. Platzhalter `ObservableSortedBinaryOrgan` (Z.93) + TODO-Block ENTFERNEN (nur 3 Verwender). static_assert(ObservableAxis<ObservableMasstreeOrgan>).
- **S6** `masstree_reference.hpp` Z.39: search_algo → ObservableMasstreeOrgan. Platzhalter-Kommentare aktualisieren.
- **S7** Equivalenz-Tests: Uint64MasstreeMatchesStdMap (verify_matches_std_map 60000/60000, treibt Leaf/Internode-Split), MasstreeMultiLayerStress (5 Phasen, hohe Bytes erzwingen make_new_layer), MasstreeLayerBoundary (identische 2-Byte-Slices + divergenter Rest = Namensanspruch-Beleg), MasstreeVariantsEquivalent (vs ArtTrieOrgan), Degenerationsanker (`<8>` besteht denselben Test).
- **S8** `test_v41_compositions.cpp` Z.196 Platzhalter-Kommentar entfernen.
- **S9** Kperm15-Unit-Test ZUERST (fragilste Bit-Arithmetik, remove-Sonderfall x&15==i+1), DANN Organ-Test. Build via ./configure.sh (build/msvc-release; header-only über tier_to_organ_mapping sichtbar). Registry Z.69 (Masstree DEFERRED) + known_compositions_list BLEIBEN (is_original-Linking deferred).

**open_question (Planrunde-Empfehlung getroffen, reversibel):** SliceBytes=2 als Composition-Default (echte Layer + Namensanspruch) vs SliceBytes=8 (paper-treu, Layer tot für uint64). Empfehlung+Default = 2, mit 8 als Anker. 1-Zeilen-Umkehrung falls User strikte Paper-Slice-Breite höher gewichtet.

## 4. Verbleibende Folge-Chargen (optional/Refinement) — Stand nach Teil 2
- ~~Masstree-Organ~~ ✅ ERLEDIGT (§3, commits `f9de770`/`539e64f`/`1a2b12a`).
- ~~ART-Node-Shrink beim Erase~~ ✅ ERLEDIGT (`65f3102`, N256→N48→N16→N4).
- SuRF S2 Inkrement 2 (LoudsDense, reine Space-Opt) — **offen, grounded** (LOUDS-DENSE ist real spezifiziert; medium, session-tail-riskant da invasiv an der Sparse-Query). Sparse-only ist bereits korrekt+vollständig.
- `axis_03t_node_tuning` (START Cost-DP) — **GESPERRT**: Mess-Modell ist USER-eigene Entscheidung (Säule-2-Selbstmessung vs CEB-Säule-1), NICHT autonom zu starten.
- HOT/Wormhole-Refinements (SIMD find_child = Portabilitätskonflikt; Multi-Bit-Höhe; Node-Collapse) — Refinement, nicht Korrektheit.
- extern-C-Linking lizenzierte Paper-Codes (#4 / R7.6.c) — überwiegend lizenz-/template-blockiert (Wormhole GPL-3, Masstree template-only).
- Vendor-Detection-CMake für 24 disabled Allocatoren (R7.4-Rest) — extern-abhängig (jemalloc/tcmalloc-libs nicht im Repo), session-tail-riskant.
- axis_03m MP03/MP04 + axis_03b CT04/CT05 — **NICHT grounded**: nur vage Roadmap-Platzhalternamen ohne Paper-/Spec-Basis; Implementierung wäre spekulative Scope-Creep (verstößt gegen Algorithmus-Korrektheits-Disziplin). Bewusst NICHT umgesetzt.

## 5. Teil 2 — Folge-Charges NACH dem Blueprint (autonome /goal-Sequenz, Ultracode)

Methodik je Charge: (Planrunde bei Unklarheit →) Implementierung → adversariale Verifikation → Commit+Tag+Push+da-Bump.

1. **Masstree-Organ KOMPLETT** (`f9de770`/`539e64f`/`1a2b12a`) — §3-Blueprint ausgeführt; letzter Tier-Platzhalter seziert. Damit ist die Umstufung-B-Direktive (Achsen enthalten NUR Organe) restlos erfüllt: KEIN monolithisches Tier mehr in `axis_03a`.
2. **Cross-Constraint ISA×SIMD (#704)** (`f9b6303`) — physisch unmögliche SIMD/ISA-Paare (Neon auf x86_64 etc.) aus dem Permutationsraum gefiltert (`mp_remove_if`); Roh-Produkt diagnostisch erhalten.
3. **Säule-2 Mess-Pfad real** (`4f499cd`/`ce262d7`) — der F15-Mess-Treiber misst das ECHTE Composition-Organ pro Achse (`observe_all`-Statistik-Trace), statt generisch SortedBinary. Zweite Mess-Dimension operativ.
4. **ART-Node-Shrink beim Erase** (`65f3102`) — adaptiver Knoten schrumpft N256→N48→N16→N4 (Leis ICDE 2013, beide Richtungen), std::map-treu via Equivalence-Test.
5. **Cross-Constraint ISA×Plattform (#704-Erweiterung)** (`d29fdef`) — 2. latenter Defekt: eine Mikroarchitektur-ISA läuft nur auf Plattform DERSELBEN CPU-Familie (Amd64 nicht auf Aarch64-Plattform). `isa_platform_compatible<Isa,Platform>()` + `FilteredIsa09xPlatform12` + 3-Wege-Verkettung (schließt auch das SIMD-kompatible-aber-plattform-unmögliche Tupel). Deterministischer Voll-Produkt-Beweis: 12 Paare → genau 6 möglich.
6. **queuing Q1×Q2 begründet ungefiltert** (`fdbb2bc`) — Gegenprobe zu #704: Buffer×Flush sind orthogonal (kein physisch-unmögliches Paar; NoBuffer+Watermark nur redundant). Filter widerspräche dem F15-Vollraum-Mess-Ziel. Asymmetrie als Absicht dokumentiert.
7. **R7.4: resource_ownership() POOL vs PMR** (`185f038`) — die PMR-Familie (A22) typsicher getrennt: `enum ResourceOwnership {None,Owned,Borrowed}`, Pflicht-Property im Concept, Default `None` einmal in der CRTP-Base (kein Bloat), nur POOL=Owned/PMR=Borrowed überschreiben. Orthogonal zu `supports_pmr()`.

**Verifikations-Stand Teil 2:** volle Regression `ctest` **2078/2078 grün** (0 Defekte), nach der letzten Charge gemessen. Allocator-Topic 473/473. Cross-Constraint-Untersuchung über ALLE Topics abgeschlossen (nur hardware+queuing haben kartesische Produkte).

**Cluster-Architektur-Status:** Die Cross-Constraint-Methodik (physisch-unmögliche Permutationen compile-time ausfiltern) ist auf den Permutationsraum der cache-engine beschränkt — keine Infrastruktur-Berührung.
