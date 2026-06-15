# Umstufung-A (Task #41 Schritt B) — OriginalXxx-Sezier-Planrunde: Agenten-Workflow-Ergebnis

**Stand:** 2026-05-29
**Workflow:** `wjy6nix6j` / Run `wf_b2b0dedc-aff` (7 Agenten, 697k Subagent-Tokens, 112 Tool-Calls, ~448 s)
**Auftrag:** Planrunde — 5 OriginalXxx-Lebewesen (ART/HOT/START/Wormhole/SuRF, S04–S08) in Organe sezieren.
**Bezug:** Doku 14 §11.2/§13/§14.2, `[[feedback_no_whole_tier_axes_genus_configurator]]` (JEDES Lebewesen — auch
OriginalXxx — muss seziert werden), `[[feedback_web_research_per_algorithm_pflicht]]`.

---

## 1 KRITISCHER IST-STATE-BEFUND (Reader 1 + 2)

### 1.1 Die OriginalXxx-Wrapper-Bodies sind triviale Re-Impls — KEIN Paper-Code im Body

ALLE 5 Wrapper (`axis_03a_search_algo_original_{art,hot,start,wormhole,surf}.hpp`) haben einen eigenen
C++23-Re-Impl-Body; **keiner ruft den Paper-Original-Code auf**. `is_original=true` ist eine reine
**Compile-Time-Deklaration** (OriginalCodeMixin + SHA256-validierte `manifest.txt` des *physischen*
Paper-Codes in `legacy_code/paper_pXX/`), NICHT ein Beleg, dass Paper-Code im Body läuft (echtes
`extern "C"`-Linking ist erst „s4" geplant).

| Lebewesen | Strategy (Reg.) | key_type | s2-IST-Body (verifiziert) | is_original_module | Paper-Code (physisch) | Lizenz |
|---|---|---|---|---|---|---|
| ART | S04 (P01) | uint8 | `std::array<optional<u64>,256>` direct-addressed (== Array256) | **4/4 TRUE** | art.hpp 2108 Z. | Apache-2.0 |
| HOT | S05 (P02) | uint8 | `vector<k>+vector<v>+lower_bound` (== VectorU8U8) | 2/4 (ins+lkp) | HOTRowex.hpp 435 Z. | ISC |
| START | S06 (P05) | **uint16** | `vector+vector+lower_bound` (== VectorU16U16) | 2/4 | nur 112-Z.-sosd-Adapter | MIT |
| Wormhole | S07 (P07) | uint8 | `vector+vector+lower_bound` (KEIN Hash/Trie im Body!) | 3/4 (kein clear) | wh.c 3873 Z. | **GPL-3.0** |
| SuRF | S08 (P10) | uint8 | `vector+vector+lower_bound`, `erase` real (exaktes K→V) | 1/4 (nur lkp) | surf.hpp 408 Z. | Apache-2.0 |

**Konsequenz:** 4 der 5 Bodies sind byte-identisch derselbe `sorted-vector+lower_bound`-Code, ART ist
`array<optional,256>`. Die ECHTE Organ-Differenzierung (adaptive Node4/16/48/256, LOUDS-dense/sparse,
wormmeta-Hash-Anchor, Patricia-Split) existiert **ausschließlich im physischen Paper-Code**, NICHT im
Wrapper-Body.

### 1.2 Bestehende Gattungs-Konfiguratoren/Anatomie sind faktisch uniform

11 Compositions (6 CE-Re-Impl `*_reference.hpp` + 5 `*_paper_binding_reference.hpp`); jede setzt dieselben
17 Organ-Achsen-Aliase. **14 von 17 Achsen sind bei ALLEN identisch fixiert** (node_type=Node256Layout,
path_compression=PathCompressionNone, allocator=Mimalloc, …). Nur 3 variieren: search_algo,
cache_traversal {LinearFanout/HashLookup}, mapping {DirectPlacement/PoolRelative}. `reference` vs
`paper_binding` unterscheiden sich **nur im search_algo** (reference=CE-Re-Impl, paper_binding=OriginalXxx).
`SearchAlgorithmAnatomy<C>` (anatomy/) hat insert/lookup/erase/clear ENTFERNT (→ AnatomyExecutionContext);
`observe_all()` ist real nur für search_algo (Säule-2-Stand). Die §13-SOLL-Trie-Organe sind im Code
**noch nicht** umgesetzt — größte Lücke Doku-Soll vs Code-Ist.

### 1.3 Build-/Provenienz-Auffälligkeiten (zu dokumentieren, NICHT in #41 zu fixen)

- **SuRF:** generierter `paper_p10_surf_is_original.hpp` fehlte im gelesenen Snapshot (nur p01/p02/p05/p07).
  In MEINEM `build/msvc-release` kompiliert die Registry inkl. SuRF aber bereits (Test 7/7 grün) → vor Inc
  verifizieren; falls Header fehlt: regenerieren via configure. → R7.1.b (#27).
- **Wormhole:** GPL-3.0 → Doku 18 §2 führt `is_original=✗` (Linking-Blocker), aber Wrapper-Manifest
  erzwingt 3/4 → Widerspruch. → R7.6.c (#4).
- **Venue-Fehlzitate (Registry-Kommentare):** START „Mertens ICDE 2024" → korrekt Fent/Jungmair/Kipf/
  Neumann ICDEW 2020; Wormhole „ATC 2019" → korrekt EuroSys 2019; HOT „PVLDB/SIGMOD 2018" web-verifizieren.

---

## 2 GEWÄHLTER ANSATZ: Linse C (pragmatisch gestaffelt, Rekonstruktions-Beleg-first)

**Verworfen Linse B** (Voll-Pool-Familie je Lebewesen): da 4/5 Bodies identischer sorted-vector-Code sind, wäre
eine eigene `RadixNodePool`/`HotNodePool`/… nur eine umbenannte Kopie von `RawSlotStore+SortedBinary` →
**null neue Organ-Semantik, reine Duplikation** (`[[feedback_no_quick_fixes]]`, `[[cross-axis-defaults-no-bloat]]`)
+ würde die existierenden Achsen axis_04/axis_02 duplizieren und die Permutierbarkeit (Kern-Beitrag)
zerstören. **Linse A ≈ C** im Endzustand; C gewählt wegen Increment-Disziplin (Komplexitäts-Staffelung).

**#41 = Brücke (Rekonstruierbarkeit des IST-Wrappers belegen), NICHT Entfernung (= #42).** `is_original`/
Habich bleibt am Wrapper (Bodies unangetastet); das composable-Organ ist provenienz-frei und seziert nur
das IST-Such-Verhalten.

### 2.1 Lebewesen→Organ-Mapping (rein additiv, KEINE neuen Pool-Dateien)

```cpp
using OriginalArtOrgan      = LinearScanOrgan;    // S04 ART  (array<optional,256> == LinearScan), uint8 → 200/255
using OriginalHotOrgan      = SortedBinaryOrgan;  // S05 HOT  (sorted-vector),                     uint8 → 200/255
using OriginalStartOrgan    = SortedBinaryOrgan;  // S06 START (sorted-vector, key_type=uint16!),  → 1000/1000
using OriginalWormholeOrgan = SortedBinaryOrgan;  // S07 Wormhole (sorted-vector, KEIN Hash im Body), uint8 → 200/255
using OriginalSurfOrgan     = SortedBinaryOrgan;  // S08 SuRF (sorted-vector, exaktes K→V im Body), uint8 → 200/255
```
Beleg je Lebewesen: horizontal `verify_variants_equivalent<Organ, Tier>(key_mod, query_max)`; vertikal
(`Organ ≡ std::map`) ist über die identischen Organ-Typen (Array256/VectorU16U16-Zeilen) transitiv erfüllt.
**KRITISCH:** START ist das EINZIGE uint16-Lebewesen → `key_mod=1000` (sonst Key-Cast-Kollision); die 4 uint8-
Lebewesen → `200/255`.

**SuRF-Range-Filter-Semantik:** Der IST-Body ist KEIN Approximate-Filter (er hält parallele Vektoren +
liefert exaktes K→V), daher ist der Doppel-Beleg gültig. Die LOUDS-Bitmap-Range-Filter-Semantik (bool) des
Papers ist s4-Thema (axis_filter RangeSurfFilter + axis_05 PackedBitmap + axis_10 SuccinctSerialization).

---

## 3 DRAUSSEN (#41) — Pflicht-Folge-Tasks (damit „JEDES Lebewesen seziert" NICHT verloren geht)

- **#42 Umstufung-B:** OriginalXxx S04–S08 aus `AllStrategies`/`EnabledStrategies` entfernen + Composition-
  search_algo von `OriginalXxxSearchAlgo` auf Organ-Komposition umstellen.
- **s4 „echte Trie-Sezierung" (NEUE Pflicht-Charge, KRITISCH für die Vollständigkeit der erklärten Ordnung):**
  Die wahre Anatomie der 5 Original-Algorithmen — adaptive Node4/16/48/256 (ART/START), Patricia/k-constrained
  Path-Compression (HOT), LOUDS-dense/sparse + Succinct (SuRF), wormmeta-Hash-Anchor + B+-Leaf (Wormhole) —
  als **node_type × path_compression × Trie-Traversal Mehr-Achsen-Modell** original-getreu sezieren + echtes
  `extern "C"`-Paper-Code-Linking (s4). Erst damit sind die Original-Lebewesen VOLL in ihre charakteristischen
  Organe zerlegt (heute sind die Wrapper-Bodies nur flache Re-Impl-Platzhalter). Die §13-SOLL-Organe in den
  Compositions umsetzen (statt überall Node256/None). Lizenz-Blocker beachten: Wormhole GPL-3.0.

---

## 4 Verweise

- Workflow-Output (vollständig): `tasks/wjy6nix6j.output` (90k+ chars; understanding[3] + designs[3] + blueprint).
- #41-Schritt-A-Doku: `docs/sessions/20260529-umstufungA-hash-skiplist-btree-dissection-design-agenten-session.md`.
- Doku 18 (Paper-Code-Map, autoritativ): `docs/architecture/18_achsen_algorithmus_paper_code_map.md`.
- Organ-Vorlage: `composable/tier_to_organ_mapping.hpp` (LinearScanOrgan/SortedBinaryOrgan existieren bereits).
