# L-MEAS-THESIS — Echte Ende-zu-Ende-Messung der SearchAlgorithm-DLL-„Lebewesen" (2026-06-03)

**Auftrag (User 2026-06-03):** „einen echten Ende-zu-Ende Build mit Messung durchführen, welcher echte Werte für die
Diplomarbeit und echte dll Tiere generiert für das Thema/Gattung Suchalgorithmen." Dieses Dokument hält die **literalen
Messwerte** (perm_runner-Ausgabe, nicht autor-behauptet) + die **reproduzierbare E2E-Kette** fest.

> **Gattung im Fokus:** SearchAlgorithm (`AnatomyGenus::SearchAlgorithm`). Jedes „Lebewesen" = EINE perm-DLL = 17-Slot-
> AdHocComposition + 3 Build-Achsen, geladen + gemessen über die echte `.dll`-Grenze. Kein Mock, kein Stub.

---

## 1. Methode — die gate-freie lokale E2E-Kette

```
thesis_<fam>_<organ>.cpp           (1 ADHOC-BUILDVARIANT-Anatomie, 16 Achsen FIX, 1 Achse variiert)
  └─ cl /LD + COMDARE_MEASUREMENT_ON=1 + voller ADHOC-Include-Satz (45 Dirs aus build/.../generated + Boost.MP11)
       └─ thesis_<fam>_<organ>.dll  (echtes SearchAlgorithm-Lebewesen-Binary; IObservableTier scharf)
            └─ perm_runner <dll> <binary_id> <n_ops>
                 └─ run_observable_perm: n_ops × tier_insert(i, 7i+1) → n_ops × tier_lookup(i) → tier_observe
                      └─ result_ingest-Zeile: binary_id + 13 Observer-Felder  (→ ExperimentTree-NodeValue)
```

- **Quellen:** `tests/unit/thesis_tiere/thesis_sa_*.cpp` (5×) + `thesis_nt_*.cpp` (3×) — committet, repo-reproduzierbar.
- **Build+Mess-Skript:** `tests/unit/thesis_tiere/build_and_measure_thesis_tiere.ps1` (Auto-Discovery + RAM-Watchdog +
  inkrementelles Reuse). Voraussetzung: CMake EINMAL konfiguriert (`build/msvc-release/generated/` + `perm_runner.exe`).
- **KRITISCH:** `COMDARE_MEASUREMENT_ON=1` schaltet die `IObservableTier`-Vererbung im `SearchAlgorithmAbiAdapter` scharf
  (`abi_adapter.hpp:88 #if COMDARE_MEASUREMENT_ON`). Ohne diese Defines baut die DLL im „MESSUNG-AUS"-Modus (nur
  `IDriveableTier`) → `perm_runner` findet kein `IObservableTier` (exit 1). Define-Satz gespiegelt vom CMake-Target
  `perm_adhoc_buildvariant`.
- **Isolierter Achsen-Vergleich (F15):** in JEDER Familie sind 16 der 17 Anatomie-Achsen IDENTISCH; variiert wird genau
  EINE Achse → der gemessene Unterschied ist dieser Achse zurechenbar.

**Host/Lauf:** Windows 11, MSVC `cl /std:c++latest`, n_ops ∈ {1000, 2000, 4000}, je Tier eine Wiederholung (die 13
Observer-Felder sind STRUKTURELL/deterministisch — nicht Wall-Clock; Wiederholungs-Streuung betrifft nur Timing = PMC,
s. §6). 8/8 Lebewesen gebaut (exit 0, min. ~9 GB RAM frei), 24/24 Messungen exit 0.

---

## 2. result_ingest-Feldsemantik (13 Felder, `result_ingest.hpp`)

`binary_id ; search_lookup ; hit ; miss ; insert ; erase ; peak_occupancy | bytes_alloc ; bytes_in_use ; alloc_cnt ; dealloc_cnt ; fail | observable_axes ; tier_fill_level`

---

## 3. Familie A — Such-Organ variiert (axis_03a), Node256 + 15 weitere Achsen FIX

Gemessene perm_runner-Zeilen (literal). Allokation (`bytes_alloc/bytes_in_use/alloc_cnt`) ist über ALLE 5 Such-Organe
identisch (erwartet — die Speicher-/Layout-/Allokator-Achsen 04/05/06 sind fix). Der Such-Organ-Effekt zeigt sich in
**`peak_occupancy` + `tier_fill_level`**:

| Such-Organ (axis_03a) | peak/fill @ n=1000 | @ n=2000 | @ n=4000 | bytes_alloc @ n=2000 | alloc_cnt | Belegungs-Regime |
|---|---|---|---|---|---|---|
| **Array256SearchAlgo**       | 256  | 256  | 256  | 115200 | 20 | **gebunden-256** (256-Slot-Array) |
| **VectorU8U8SearchAlgo**     | 256  | 256  | 256  | 115200 | 20 | **gebunden-256** (u8-Schlüsselraum) |
| **LinearScanSearchAlgo**     | 1000 | 2000 | 4000 | 115200 | 20 | **n-linear** (hält alle Elemente) |
| **BinarySearchTreeSearchAlgo** | 1000 | 2000 | 4000 | 115200 | 20 | **n-linear** |
| **BTreeSearchAlgo**          | 1000 | 2000 | 4000 | 115200 | 20 | **n-linear** |

**Befund A:** Die Gattung SearchAlgorithm zerfällt unter diesem Insert/Lookup-Workload messbar in **zwei Belegungs-
Regime** — *kapazitätsgebundene* Organe (Array256, VectorU8U8: `peak`/`fill` sättigen bei 256, unabhängig von n) vs.
*elementlineare* Organe (LinearScan/BST/BTree: `peak`/`fill` = n). Das ist die reale, dem Such-Organ zurechenbare
Signatur (alle übrigen Observer gleich). search_hit = n, miss = 0 (alle eingefügten Keys werden gefunden — korrekt).

---

## 4. Familie B — Node-Organ variiert (axis_04), Array256 + 16 weitere Achsen FIX

| Node-Organ (axis_04) | peak/fill (alle n) | bytes_alloc @ n=2000 | alloc_cnt | Ergebnis |
|---|---|---|---|---|
| **Node256NodeType** (= Familie-A-Baseline) | 256 | 115200 | 20 | Referenz |
| **Node48NodeType**  | 256 | 115200 | 20 | **identisch** |
| **Node16NodeType**  | 256 | 115200 | 20 | **identisch** |
| **Node4NodeType**   | 256 | 115200 | 20 | **identisch** |

**Befund B:** Variation des Node-Organs bei festem **Array256**-Such-Organ ändert die Messung NICHT (alle 4 identisch).

> ⚠️ **KORREKTUR (2026-06-03, nach tiefem Architektur-Audit — User-Einwand bestätigt):** Die ursprüngliche Deutung
> „erwünschte Null-Kontrolle / reale Achsen-Orthogonalität" war **FALSCH**. Der wahre Grund ist ein **echter
> Architektur-Verstoß**: die axis_03a-Such-Organe (Array256/VectorU8U8/LinearScan/BST/BTree) sind **Monolithen mit
> eingebettetem Eigenspeicher** und **delegieren NICHT** an die Speicher-Achsen node_type/memory_layout/allocator. Im
> `SearchAlgorithmAbiAdapter` existieren zwei getrennte, unverbundene Speicher: `search_organ_` (Monolith → liefert
> peak/fill, ignoriert node_type; `abi_adapter.hpp:516`) und `container_` (separater `ComposedStore` mit **hart
> verdrahtetem** `SortedBinaryTraversal` → liefert nur alloc_*; `abi_adapter.hpp:507-517`). Zusätzlich trägt der V1-
> Mess-POD gar keine node_*-Felder. Variation von axis_04 ist deshalb **per Konstruktion** wirkungslos — eine
> Tautologie, kein empirischer Befund. **Konsequenz:** die Lebewesen durchlaufen NICHT uniform alle 17 Organ-Interfaces;
> das Sezierungs-Prinzip (Doku 14 „Achse = Organ, kein Such-Organ besitzt eigenen Speicher") ist im aktiven Mess-Pfad
> verletzt. **Auch Befund A** misst damit die *interne* Eigenspeicher-Struktur der Monolithen, nicht ein in
> node_type/layout/allocator zerlegtes Organ-System. Voll-Audit + Fix-Plan: `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md`.

---

## 5. Reproduktion (gate-frei, lokal)

```powershell
# (einmalig) CMake konfigurieren, damit build/msvc-release/generated/ + perm_runner.exe existieren.
pwsh "tests/unit/thesis_tiere/build_and_measure_thesis_tiere.ps1"
# → build/thesis_tiere/thesis_measurements.csv  (organ;n_ops;<13 Observer-Felder>)
```

Einzel-Tier:  `perm_runner build/thesis_tiere/thesis_sa_btree.dll thesis_sa_btree 2000`.

---

## 6. Grenzen + Cluster-Anbindung (GOAL-K78)

- **Was lokal NICHT messbar ist:** Wall-Clock-/Cache-/Branch-Counter pro Achse (PMC). Die 13 Observer-Felder sind
  STRUKTUR-Zähler (deterministisch), kein Timing. Echtes Per-Achsen-Timing braucht privilegierte Hardware-Counter →
  **GOAL-K78 CE-D3** (`docs/sessions/20260601-26-...`). Daher hier KEINE Zeit-/Durchsatz-Tabelle (wäre eine falsche
  Erfolgsmarke ohne echten Counter-Zugriff).
- **Cluster-Skalierung (CE-D2):** dieselben `thesis_*.cpp` bauen+messen auf ZIH via `apptainer build comdare-ce.sif` →
  `sbatch`-Array → `singularity exec ... perm_runner` → Webhook → `result_ingest` → Baum. Das result_ingest-FORMAT ist
  **MSVC↔gcc identisch** → diese lokalen Zahlen und die Cluster-Zahlen fließen in DENSELBEN Mess-Anhang (GATE-MAXIMAL,
  user-gebunden — NICHT autonom).
- **Allokator-Real-Link (#84, literaler IST-Stand 2026-06-03):** im `build/msvc-release`-Cache sind ALLE
  `COMDARE_HAVE_<vendor>` = **OFF** → alle axis_06-Wrapper laufen aktuell im C++23-Re-Impl-Fallback
  (`is_original_module()=false`); deshalb ist `alloc_cnt`/`bytes_alloc` über alle Tiere gleich (derselbe Fallback-
  Allokator). Mechanik (ENABLE/HAVE/USE) ist real bewiesen für **mimalloc/snmalloc/dlmalloc** (self-contained, lokal
  aktivierbar via `-DCOMDARE_BUILD_PERMUTATIONS=ON`); die **4 harten Vendor** (jemalloc/tcmalloc/hoard/scalloc) brauchen
  vcpkg/MSYS2/ZIH-Linux → **GOAL-K78 CE-D4**. Spec: `docs/sessions/20260601-19-vendor-allokatoren-beschaffungs-spec.md`.

---

## 7. Fazit

Der Mechanik-Teil (Build → Laden → perm_runner → result_ingest) ist **lokal, gate-frei, reproduzierbar** vollzogen: 8
DLL-Lebewesen gebaut, 24 echte `result_ingest`-Zeilen gemessen. **Die METHODIK ist jedoch durch einen Architektur-Verstoß
eingeschränkt** (s. KORREKTUR in §4 + Voll-Audit `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md`): die
Such-Organe sind Monolithen mit Eigenspeicher und delegieren NICHT an node_type/layout/allocator → die Lebewesen durchlaufen
nicht uniform alle 17 Organe. **Befund A** misst daher die *interne Eigenspeicher-Struktur* der Such-Monolithen
(real reproduzierbar, aber NICHT „zerlegtes Achsen-System"); **Befund B** ist der Verstoß-Abdruck, KEINE Orthogonalität.
Vor einer Verwendung als Diplomarbeit-Kernmesswert ist der Fix-Plan (Audit-Doc §4 Q2: echte Such-Organ-Delegation +
Adapter-auf-einen-Store + bounded ComposedStore + V2-POD) umzusetzen. Timing-/Allokator-Tiefe + Cluster-Skala bleiben
als GOAL-K78 CE-D2/D3/D4 abgegrenzt (hardware-/Linux-/user-gated).
