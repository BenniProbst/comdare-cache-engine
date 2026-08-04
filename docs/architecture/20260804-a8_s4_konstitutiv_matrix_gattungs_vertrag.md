# A8-S4 -- KONSTITUTIV-MATRIX (Gattungs-Vertrag): je (Gattungs-Funktion x Achse) eine deklarierte Rolle

Stand: 2026-08-04, Scheibe A8-S4 (Vor-Anker-Pflicht 2/3, binary-beruehrend -> A8-Dossier Kante 4.3-1).
Basis: ce `65a61fcf` (development-Spitze mit A8-S3), Branch `a8-s4`.
Dossier: `docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md` Abschn. 3.3 + 5-S4 + Auflagen 1-12.
Negativ-Liste: `docs/architecture/20260804-e24_g8_negativliste_gesperrte_abi_flaechen.md` (G8 Par. 3.2 fuehrt A8-S4 als vor-Anker-Pflicht).
ASCII-only.

**ERHEBUNGSWEISE (bindend): AM OBJEKT.** Jede Zelle dieser Matrix wurde durch Lesen/grep in
`libs/cache_engine/anatomy/abi_adapter.hpp` und der Kette darunter belegt, nicht aus Doku uebernommen.
Alle `datei:zeile`-Anker gelten am Endstand dieser Scheibe (nach den Commits `9d53ed4e` und `d7d4d6c7`);
Zeilen-Drift ist moeglich -- bindend ist der TOKEN, nicht die Zahl.

---

## 1. Die Rollen -- Definition und Abgrenzung

Das F2-Gesetz (Owner 01.08.) sagt: *"Die Gattungs-(Suchalgorithmus-)Interface-Funktionen VERWENDEN die
Achsen-Interfaces"*, und zugleich: *"Die Achse ist NICHT eine Interface-Funktion des Suchalgorithmus"*.
Das Dossier 3.3 leitet daraus die zwei Rollen ab. Diese Matrix fuehrt sie mit einer PRAEZISIERUNG:

| Rolle | Bedeutung | Im Release-Hot-Path? |
|---|---|---|
| **KONSTITUTIV-RT** | Die Gattungs-Funktion RUFT das Achsen-Interface zur Laufzeit im Hot-Path. | JA, als Aufruf |
| **KONSTITUTIV-CT** | Die Achse PARAMETRISIERT die Hot-Path-Kette (Template-Argument); sie bestimmt den Code, der laeuft, ohne eigenen Aufruf (zero-cost, `if constexpr`/constexpr-Geometrie). | JA, als Code |
| **BEOBACHTEND** | Mess-Organ. Der Aufruf liegt AUSSCHLIESSLICH in einem `#if COMDARE_MEASUREMENT_ON`- oder `#ifdef COMDARE_CE_ENABLE_STATISTICS`-Block. | NEIN |
| **NICHT BETEILIGT** | Die Achse wird von dieser Gattungs-Funktion ueberhaupt nicht beruehrt -- auch nicht beobachtend. | NEIN |

KONSTITUTIV-CT ist KEINE dritte Rolle im Sinne des Dossiers, sondern die ehrliche Aufspaltung von
"KONSTITUTIV" nach Wirkungs-Mechanismus. Beide Faelle liegen im Release-Hot-Path; wer sie zusammenwirft,
verliert genau die Aussage, die die Metaprogrammierungs-Doktrin traegt (CT-Dispatch statt Runtime-Switch).

Zusaetzlich ausgewiesen, weil sie sonst still unter den Tisch fiele:

| Sonder-Rolle | Bedeutung |
|---|---|
| **STEUERND** | `IResourceControllableTier` (`tier_apply_resource_control`, abi_adapter.hpp:291-342). Ungegated, also auch im Release vorhanden -- aber EINMAL JE EINSTELLUNG, nicht je Op, und ausserhalb des Gattungs-Kerns. Genau drei Organ-Setter (Par. 5). |

---

## 2. DIE MATRIX -- Gattungs-Funktion x Achse

Gattungs-Funktionen = der `IDriveableTier`-Kern der Map-Gattung (idriveable_tier.hpp:46-58):
`tier_insert` (abi_adapter.hpp:954) - `tier_lookup` (:1065) - `tier_erase` (:1118) - `tier_clear` (:1133) -
`tier_size` (:1176).

Legende: **K-RT** = KONSTITUTIV-RT - **K-CT** = KONSTITUTIV-CT - **B** = BEOBACHTEND - **--** = NICHT BETEILIGT.
Alle Zeilen-Anker ohne Datei-Praefix meinen `libs/cache_engine/anatomy/abi_adapter.hpp`.

| Achse | tier_insert | tier_lookup | tier_erase | tier_clear | tier_size |
|---|---|---|---|---|---|
| **T0 search_algo** | K-RT :971 | K-RT :1067 | K-RT :1123 | K-RT :1139 + :1142 | K-RT :1178 |
| **T1 cache_traversal** | B :992 | B :1075 | -- | B :1163 | -- |
| **T2 mapping** | B :1006 | B :1083 | -- | B :1164 | -- |
| **T3 path_compression** | B :1038, :1052 | B :1107 | -- | B :1160 | -- |
| **T4 node_type** | K-CT :2228 | K-CT :2228 | K-CT :2228 | K-CT :2228 | K-CT :2228 |
| **T5 memory_layout** | K-CT :2228 | K-CT :2228 | K-CT :2228 | K-CT :2228 | K-CT :2228 |
| **T6 allocator** | K-RT :2228 (Store) | K-CT :2228 | K-RT :2228 (Store) | K-RT :2228 (Store) | K-CT :2228 |
| **T7 prefetch** | B :1033 | B :1096, :1101 | -- | -- | -- |
| **T8 concurrency** | B :1025 | B :1088 | -- | -- | -- |
| **T9 serialization** | -- | -- | -- | B :1167 (Stats-Reset) | -- |
| **T10 value_handle** | B :1047 | -- | -- | B :1156 | -- |
| **T11 index_organization** | -- | -- | -- | B :1167 (Stats-Reset) | -- |
| **T12 io_dispatch** | -- | -- | -- | B :1167 (Stats-Reset) | -- |
| **T13 migration_policy** | -- | -- | -- | B :1167 (Stats-Reset) | -- |
| **T14 filter** | B :1042 | -- | -- | B :1152 | -- |
| **T15 queuing_q1** | B :1011 | B :1086 | -- | B :1165 | -- |
| **T16 queuing_q2** | B :1017, :1019 | -- | -- | B :1167 (Stats-Reset) | -- |
| **T17 persistence_target** | -- | -- | -- | B :1167 (Stats-Reset) | -- |

`:2228` ist die Zeile, auf der `Composition::node_type`, `Composition::memory_layout` und
`Composition::allocator` GEMEINSAM als Template-Argumente des Stores stehen (die vollstaendige
Typ-Naht ist :2224-2228) -- deshalb tragen T4/T5/T6 denselben Anker.

Die Achsen-Nummerierung T0..T17 folgt `kCompositionAxisNames`
(`builder/experiment_tree/axis_path_serialization.hpp:40-43`) -- der einzigen Ordnungs-Wahrheit; die Wache
`test_a8s4_konstitutiv_kette` A1/A2 pinnt die Bindung an die NAMEN (nie an rohe Indizes).

**KERN-AUSSAGE (deckt Dossier-Befund B-4):** Die konstitutive Menge des gesamten Gattungs-Kerns ist GENAU
`{T0 search_algo, T4 node_type, T5 memory_layout, T6 allocator}` -- die Store-Kette. Die uebrigen **14**
Achsen sind ausnahmslos BEOBACHTEND. Der Kommentar `:969-970` ("EIN Speicher, konstitutiv") sagt das seit
`#188-4c-iii` fuer T0; diese Scheibe erhebt es zur vollstaendigen, geprueften Deklaration ueber alle
5 x 18 Zellen.

**Zwei Einzelbefunde der Matrix, die vorher nirgends standen:**

- `tier_erase` (:1118-1124) traegt als EINZIGE Gattungs-Funktion **NULL** Observer-Kopplungen. Der einzige
  Mess-Block darin ist die CoW-Memento-Materialisierung (:1120, kein Achsen-Interface). Sie ist damit der
  reinste Release-Pfad-Beleg der Matrix.
- `tier_size` (:1176-1179) ruft ueber T0 nur einen Store-ZAEHLER ab (`occupied_count` -> `slot_count`,
  axis_04_node_type_layout_aware_store.hpp:197). T4/T5/T6 sind hier reine Typ-Glieder; es laeuft kein
  Achsen-Interface unter T0. Ehrlich so deklariert statt als "konstitutiv" pauschalisiert.

---

## 3. Die konstitutive Kette im Detail (T0/T4/T5/T6)

Der Adapter baut fuer store-traversierbare Kompositionen (abi_adapter.hpp:2224-2228):

```
flat_container_algorithm_t
  = ObservableComposedSearch< Traversal(T0),
        LayoutAwareChunkedStore< Composition::node_type(T4),
                                 Composition::memory_layout(T5),
                                 Composition::allocator(T6) > >
```

**T0 search_algo -- KONSTITUTIV-RT.** Aufruf-Kette am Objekt:

| Glied | Beleg |
|---|---|
| Gattungs-Funktion | abi_adapter.hpp:971 / :1067 / :1123 / :1139 / :1178 |
| Observable-Huelle | axes/lookup/composable/observable_composed_search.hpp:38 insert, :57 lookup, :71 erase, :80 clear, :81 occupied_count |
| Kompositions-Organ | axes/lookup/composable/composable_search.hpp:191 `Traversal::insert_into`, :195 `lookup_in`, :197 `erase_from`, :207 `occupied_count`, :208 `clear` |
| Achsen-Vertrag | axes/lookup/composable/composable_search.hpp:57-64 `concept TraversalOrgan` (insert_into/lookup_in/erase_from) |
| Store-Vertrag | axes/lookup/composable/storage_organ_concept.hpp:29-48 `concept StorageOrgan` (8 Methoden) |
| CT-Nebenwirkung | abi_adapter.hpp:955-959 `capacity_constraint_of<SearchAlgo>` (ehrliche Kapazitaets-Ablehnung statt stillem Slot-Wrap, #217-2a) |

**T4 node_type -- KONSTITUTIV-CT.** Die Achse bestimmt die Geometrie des Hot-Path-Substrats, ohne dass
ein Achsen-Interface gerufen wird:

| Wirkung | Beleg |
|---|---|
| Vertrags-Klammer | axes/node/axis_04_node_type_layout_aware_store.hpp:54-56 `requires NodeTypeStrategy<N>` |
| Chunk-Kapazitaet | :97 `nat_cap_ = N::max_capacity()`, :103-107 `cap_` |
| Knoten-Breite (Unter-Achse C2/FF2) | :82-87 `N::node_width_in_lines()` |
| Wirkungs-Beleg literal | `test_a8s4_konstitutiv_kette` A6: `node_capacity` Node256=**256** vs. Node4=**4** |

**T5 memory_layout -- KONSTITUTIV-CT.** Die Achse bestimmt die physische Repraesentation jedes Records
(compile-time-Dispatch ueber `RepresentationKind`, `if constexpr` -- kein Runtime-Switch):

| Wirkung | Beleg |
|---|---|
| Vertrags-Klammer | axes/node/axis_04_node_type_layout_aware_store.hpp:55 `MemoryLayoutStrategy<L>` |
| Repraesentations-Wahl | :60 `kRep = L::representation_kind()`, :76-78 `record_bytes_`, :125 `cache_line_size = L::cache_line_size()` |
| Lese-/Schreib-Pfad | :198 `key_at` -> `load_key_`, :199 `value_at` -> `load_value_`, :229 `store_record_` |
| Wirkungs-Beleg literal | `test_a8s4_konstitutiv_kette` A7: `record_phys_bytes` cache_line_aligned=**64** vs. aos_strict=**16** |

**T6 allocator -- KONSTITUTIV-RT (Schreib-/Leer-Pfad) bzw. KONSTITUTIV-CT (Lese-Pfad).** Der einzige
Punkt der ganzen Matrix, an dem eine NICHT-T0-Achse ihr Interface im Release wirklich aufruft:

| Op | Beleg |
|---|---|
| `allocate` beim Chunk-Wachstum | axes/node/axis_04_node_type_layout_aware_store.hpp:222 (`append_slot`, aus `tier_insert`) |
| `allocate` bei der Memento-Kopie | :541 (`copy_from_`) |
| `deallocate` beim Leeren/Zerstoeren | :532 (`free_chunks_`), gerufen aus :244 `clear()` (aus `tier_clear`) und :153 (Destruktor) |
| Achsen-Vertrag | axes/alloc/concepts/axis_06_allocator_concept.hpp:76-86 `AllocatorStrategy` (allocate/deallocate/Identitaet) |
| Wirkungs-Beleg literal | `test_a8s4_konstitutiv_kette` B2/B3: `chunk_allocs (T6) = 16` nach 4096 Inserts; nach `clear()` = 0 |

Im reinen Lese-Pfad (`tier_lookup`, `tier_size`) wird der Allokator NICHT gerufen -- die Matrix fuehrt ihn
dort ehrlich als K-CT (Typ-Glied), nicht als K-RT.

---

## 4. Die beobachtenden Achsen -- mit ihrem E1-Treiber als Vergleichs-Nullpunkt

Das Dossier 3.3 verlangt: *"wo eine Achse im Hot-Path semantisch nichts beitraegt, bleibt sie ehrlich
beobachtend mit eigenem E1-Treiber (Vergleichs-Nullpunkt)"*. Der EINE E1-Treiber ist
`fill_segment_timing_v3` (abi_adapter.hpp:1584); er belegt je Achse den Slot `seg_ns[T]`
(`acc[T]`, :1632-:1810). Genau dieser Slot IST der Vergleichs-Nullpunkt: er misst die Achsen-Op unter der
REALEN Komposition, auch wenn sie im Gattungs-Kern nichts beitraegt.

| Achse | Kopplung im Gattungs-Kern | E1-Treiber (Vergleichs-Nullpunkt) | Ehrlichkeits-Vermerk |
|---|---|---|---|
| T1 cache_traversal | insert/lookup/clear (per-op) | :1634-1643 `ct_organ_.resolve` -> `acc[1]` | echte Achsen-Op der realen Komposition |
| T2 mapping | insert/lookup/clear (per-op) | :1645-1656 `map_organ_.resolve_offset` -> `acc[2]` | echte Achsen-Op |
| T3 path_compression | insert/lookup/clear (per-op) | :1658-1664 `pc_organ_.compress` -> `acc[3]` | echte Achsen-Op |
| T4 node_type (Mess-Organ) | -- (die Achse selbst ist K-CT) | :1666-1673 `store_observe_node_type` -> `acc[4]` | Mess-Huellen-Scan UEBER den Store, nicht das nackte Achsen-Interface (Dossier B-3 ii) |
| T5 memory_layout (Mess-Organ) | -- (die Achse selbst ist K-CT) | :1675-1682 `store_observe_layout` -> `acc[5]` | Mess-Huellen-Scan (B-3 ii) |
| T6 allocator (Mess-Route) | -- (die Achse selbst ist K-RT) | :1684-1694 `store_allocator_statistics` -> `acc[6]` | **reiner Stats-READ statt Achsen-Op** (Dossier B-3 iii) -- der bekannte Schnitt-Fehler, S3-Scope |
| T7 prefetch | insert/lookup (per-op) | :1698-1710 `observe_prefetch_descent` -> `acc[7]` | `NonePrefetch`/`Hardware` = deklarierter 0-Nullpunkt, nicht "nichts" |
| T8 concurrency | insert/lookup (per-op) | :1712-1720 `observe_critical_section` -> `acc[8]` | Mess-Kanon EIN Thread: contention 0 BY DESIGN |
| T9 serialization | nur Stats-Reset im clear | :1722-1729 `store_observe_serialization` -> `acc[9]` | Mess-Huellen-Scan (B-3 ii) |
| T10 value_handle | insert/clear (Build-Phase) | :1732-1738 `store_observe_value_handle` -> `acc[10]` | Inline-Strategien tragen `store_value` NICHT -> honest-0 |
| T11 index_organization | nur Stats-Reset im clear | :1742-1748 `store_observe_index_org` -> `acc[11]` | Mess-Huellen-Scan (B-3 ii) |
| T12 io_dispatch | nur Stats-Reset im clear | :1750-1756 `store_observe_io_dispatch` -> `acc[12]` | In-Memory-Simulation, keine Platte |
| T13 migration_policy | nur Stats-Reset im clear | :1758-1764 `store_observe_migration` -> `acc[13]` | decide-only; der echte Move liegt in `tier_migrate_step` (:2127, MESSUNG-AN) |
| T14 filter | insert/clear (Build-Phase) | :1766-1772 -> `acc[14]` | None-artige ohne `insert_key` -> honest-0 |
| T15 queuing_q1 | insert/lookup/clear (per-op) | :1774-1786 `put`/`get` -> `acc[15]` | echte Achsen-Op |
| T16 queuing_q2 | insert (per-op) | :1788-1800 `should_flush`/`on_flush_complete` -> `acc[16]` | echte Achsen-Op |
| T17 persistence_target | nur Stats-Reset im clear | :1804-1810 `pt_organ_.observe_writeback` -> `acc[17]` | `MemoryOnlyTarget` = deklarierter Nullpunkt; `DiskWritebackTarget` misst STAGING, nicht die Platte |

Diese Spalte "Ehrlichkeits-Vermerk" ist zugleich die Uebergabe an A8-S3/S5: die drei mit (B-3 ii/iii)
markierten Zeilen sind die im Dossier benannten Misch-Vertragsarten und werden dort geheilt, NICHT hier.

---

## 5. Zwei Sonder-Zeilen, die die Matrix ehrlich machen

### 5.1 STEUER-Naht (ungegated, aber kein Mess-Pfad)

`tier_apply_resource_control` (abi_adapter.hpp:291-342, `IResourceControllableTier`) ist IMMER
einkompiliert und beruehrt drei Mess-Organe -- die einzigen ungegateten `*_organ_.`-Beruehrungen der
ganzen Datei:

| Setter | Achse | Beleg |
|---|---|---|
| `cc_organ_.set_runtime_thread_count` | T8 concurrency | :317-318 |
| `pf_organ_.set_runtime_distance` | T7 prefetch | :322-323 |
| `vh_organ_.set_runtime_inline_threshold` | T10 value_handle | :334-335 |
| (`container_algorithm_.set_runtime_pool_budget`) | T6 allocator, ueber den Store -- keine `*_organ_.`-Form | :328-329 |

Sie laufen EINMAL JE EINSTELLUNG (Host-Loop `RuntimeVariableLoop`), nicht je Op, und gehoeren damit weder
in die konstitutive noch in die beobachtende Spalte. `test_a8s4_release_pfad_neutralitaet` T1(b) pinnt,
dass es AUSSER diesen dreien keine ungegatete Organ-Beruehrung gibt -- die Wache faellt, sobald jemand eine
Mess-Kopplung aus ihrem Gate schiebt.

### 5.2 Organ-gehuellte Familien (kein flacher Store)

Fuer Pool-Familien und Reference-/PaperBinding-Huellen (`pool_family_` :2234, `organ_hull_` :2236) ist
`container_algorithm_t` KEIN `ObservableComposedSearch` mit flachem Store (:2241-2246), also
`container_algorithm_is_store_backed_ == false` (:2261). Fuer sie gilt:

- **T0 bleibt K-RT** (das Organ traegt insert/lookup/erase/clear/occupied_count -- genau das pinnt der
  Header-Assert `A8S4GattungsKernAntrieb`, :2268).
- **T4/T5/T6 sind NICHT als flache Store-Kette sichtbar**; sie leben im Organ selbst. Die Matrix-Zellen
  K-CT/K-RT gelten dort sinngemaess, aber ohne die Anker aus Par. 3. Der Header-Assert
  `a8s4_store_kette_kompositions_gebunden_v` (:2271) ist fuer diese Familien ehrlich neutral (`true`,
  else-Zweig :175-177) -- **nicht** falsch-gruen: er behauptet nichts, wo er nichts pruefen kann.
- Die store-slot-gescannten Observer sind fuer sie honest-0 (bestehender, dokumentierter
  DESIGN-MANDATED-HONEST-0-Befund, tests/unit/CMakeLists.txt:1976-1984).

---

## 6. Abhaengigkeitsrichtung Achse -> Achse (Dossier 3.3, zweiter Satz)

Verlangt: *"Achsen duerfen andere Achsen-Interfaces verwenden (Container ueberall) -- Abhaengigkeitsrichtung
dokumentieren (keine Zyklen; Allokator ist unterste Versorger-Achse)."* Erhebung am Objekt
(`grep -rh "^#include" axes/<familie>/*.hpp | grep -oE "(axes|topics)/[a-z_]+" | sort | uniq -c`):

| Achsen-Familie | zeigt auf | zeigt NICHT auf |
|---|---|---|
| `axes/lookup` (T0) | `topics/nodes` (14), `axes/alloc` (6) | layout, keine Rueckkante |
| `axes/node` (T4) | `topics/traversal` (8), `topics/memory_layout` (7), `topics/allocator` (6), `axes/cacheline` (3) | -- |
| `axes/layout` (T5) | nur `axes/layout` + `topics/memory_layout` + `axes/cacheline` | node, alloc, lookup |
| `axes/alloc` (T6) | nur `topics/allocator` + `axes/alloc` + `axes/cacheline` | node, layout, lookup |
| `axes/cacheline` | **0 Achsen-Includes** (Blatt) | alles |

**BEFUND: gerichteter azyklischer Graph, Allokator ist die unterste Versorger-Achse -- belegt, nicht
behauptet.** Ordnung: `lookup(T0) -> node(T4) -> {layout(T5), allocator(T6)} -> cacheline(Blatt)`.
Es existiert KEINE Rueckkante von `alloc` oder `layout` nach oben. Der Struktur-Audit vom 04.08.
(`super docs/sessions/20260804-DOSSIER-struktur-audit-einteilung.md` Par. 5) hat unabhaengig 0 Datei-Zyklen
ueber 255 Knoten/541 Kanten bewiesen; diese Erhebung ist der achsen-lokale Ausschnitt davon.

**EHRLICHE EINSCHRAENKUNG zu "Container ueberall" (Uebergabe an A8-S5):** Die Substitutions-Regel ist am Ist
nur fuer die NUTZLAST erfuellt. Die konstitutive Kette selbst verwaltet ihren Chunk-INDEX mit einem
`std::vector<Chunk>` am Default-Allokator (axis_04_node_type_layout_aware_store.hpp:559) -- die
Record-Bytes laufen ueber die T6-Achse (:222/:532), das Chunk-Verzeichnis nicht. Der Bestand ist repo-weit
**70 von 340** `axes/`-Headern mit `std::vector|map|unordered_map` (live nachgezaehlt 04.08., identisch zur
Dossier-Zahl vom 03.08.). Das ist Dossier-Befund **B-5** und ausdruecklich **S5-Scope**, nicht S4:
S4 deklariert die Rolle, S5 zieht den Schnitt. Datei-Liste dafuer IMMER aus dem grep, nie aus dieser Zahl.

---

## 7. CT-Haertung (Auflage 5: kein Runtime-Switch, Haertung per static_assert)

Zwei compile-time-Wachen im Header, die die konstitutive Kette pinnen. Sie erzeugen **kein Byte**: reine
Praedikate, kein Member, kein vtable-Slot, kein POD-Feld -- die G8-Wire-Flaeche bleibt unberuehrt.

| Wache | Ort | Was sie verhindert |
|---|---|---|
| `concept A8S4GattungsKernAntrieb` | abi_adapter.hpp:140-154, geprueft :2268 | Eine Gattungs-Funktion routet still an `container_algorithm_` vorbei / die T0-Kette reisst. |
| `a8s4_store_kette_kompositions_gebunden_v` | abi_adapter.hpp:156-178, geprueft :2271 | Der Store traegt einen FREMDEN node_type/memory_layout/allocator -- T4/T5/T6 waeren aus der Messung herausgedreht, ohne dass ein Test rot wuerde. |

Beide haengen an `container_algorithm_t` und gelten damit fuer **jede** Adapter-Instanziierung (alle
Kompositionen aller Tier-Binaries). Ohne neuen Include: `std::is_convertible_v`/`std::is_same_v` aus dem
bereits vorhandenen `<type_traits>`.

Dazu die Rollen-Deklaration als Kopf-Kommentar an allen fuenf Gattungs-Funktionen (:943, :1057, :1112,
:1126, :1171) -- Kommentare, keine Logik-Aenderung am Hot-Path.

---

## 8. RELEASE-PFAD-NEUTRALITAETS-BEWEIS (Gate-Pflicht der Scheibe)

Zu beweisen war: die BEOBACHTENDEN Kopplungen liegen im Release-Build (`COMDARE_MEASUREMENT_ON` aus)
NICHT im Hot-Path. Der Beweis liegt in `tests/unit/test_a8s4_release_pfad_neutralitaet.cpp` und wird
DREIFACH gefuehrt. Die TU ist als einzige des Repos release-uebersetzt (drei `#undef` in der ersten
Quellzeile = exakt `COMDARE_RELEASE_MODE=ON`; zwei `#error`-Wachen machen den Zustand fail-closed;
verlinkt nur `Boost::mp11`, damit kein messungs-AN uebersetztes Objekt einwandert).

**Teil 1 -- Praeprozessor-Pfad-Wache** (Gate = `#if COMDARE_MEASUREMENT_ON` ODER
`#ifdef COMDARE_CE_ENABLE_STATISTICS`; beide fallen im Release, weil die Wurzel-CMakeLists:143-144
STATISTICS zwingend abschaltet, sobald MEASUREMENT_MODE aus ist):

```
  [OK]  T1 abi_adapter.hpp lesbar (Praeprozessor-Pfad-Wache)
    Quelle: 2433 Zeilen, davon 1766 im Release-Gate (MEASUREMENT_ON / CE_ENABLE_STATISTICS)
  [OK]  T1(0) Selbstbeweis: bekannt-UNGEGATED wird als ungegated erkannt
  [OK]  T1(0) Selbstbeweis: bekannt-GEGATED wird als gegated erkannt
  [OK]  T1(a) alle 24 beobachtenden Kopplungen liegen AUSSCHLIESSLICH im Release-Gate (24 ok / 0 Leck)
  [OK]  T1(b) ausserhalb der Gates GENAU die 3 deklarierten Steuer-Setter, sonst keine Organ-Beruehrung
  [OK]  T1(c) alle 5 konstitutiven container_algorithm_-Aufrufe liegen ungegated (Release-Hot-Path)
  [OK]  T1(c) alle 5 Gattungs-Funktions-Koepfe sind ungegated (Antrieb IMMER einkompiliert)
```

Der Selbstbeweis T1(0) steht bewusst VOR den Aussagen: ohne ihn waere jedes gruene Ergebnis darunter
wertlos (eine Wache, die nichts unterscheidet, ist immer gruen).

**Teil 2 -- ABI-Objekt-Beweis** am echten Adapter:

```
  [OK]  T2 IDriveableTier VORHANDEN (die Gattung faehrt ohne jede Messung)
  [OK]  T2 IObservableTier ABWESEND (kein Observer-vtable-Slot im Release)
  [OK]  T2 IMeasurableWorkload ABWESEND (kein Pfad-A im Release)
  [OK]  T2 IRollbackableTier ABWESEND (kein memento_all im Release)
  [OK]  T2 IScannableTier ABWESEND (kein Range-Scan im Release)
  [OK]  T2 IMigratableTier ABWESEND (kein 2-Ebenen-Move im Release)
```

**Teil 3 -- Wallclock-Kontrast** ueber identische Last (4096 `tier_insert` + 4096 `tier_lookup`, gleiche
Key-Folge, gleiche Achsen-Wahl, dieselbe Maschine, derselbe Build). Weil ein EINZELNER Lauf hier zu
verrauscht ist (der Release-Lauf streut deutlich, Allokator-/Cache-Warmlauf), steht hier die
**Median-of-7** mit der VOLLEN sortierten Stichprobe -- keine gerundete Behauptung:

| Build | Stichprobe (7 Laeufe, sortiert, ns) | Median | Quelle |
|---|---|---|---|
| MESSUNG-AN | 16299572 16354812 16361212 **16487604** 16533004 16614175 17487223 | **16487604** | `test_a8s4_konstitutiv_kette` |
| RELEASE | 10496697 10529428 10532948 **10634279** 10683569 10700110 13319593 | **10634279** | `test_a8s4_release_pfad_neutralitaet` |

Median-Verhaeltnis **1,55**. Die Stichproben ueberlappen NICHT (Maximum Release ohne den einen Ausreisser
= 10700110 < Minimum MESSUNG-AN = 16299572), die Richtung ist also eindeutig. Die Zahlen stammen aus einem
unoptimierten Debug-Build und sind daher KEINE Messaussage im Sinne der E-Ebenen -- sie belegen, DASS der
Mess-Overhead existiert und im Release verschwindet, nicht WIE GROSS er unter Optimierung waere.
Die belastbare Aussage liefern Teil 1 und Teil 2; der Kontrast ist die dritte, unabhaengige Bestaetigung.

Hinweis zur Nachvollziehbarkeit: die Commit-Nachricht von `d7d4d6c7` nennt EINZEL-Laufwerte
(10056145 / 16349075 ns, Faktor ~1,63) -- sie waren literal, aber Stichprobe von n=1. Massgeblich ist die
Median-of-7 in dieser Tabelle.

---

## 9. Abgrenzung, Auflagen-Bilanz, offene Uebergaben

**Was diese Scheibe NICHT tut** (bewusst, Dossier-Scheiben-Schnitt):
- Sie heilt die Misch-Vertragsarten B-3 (i/ii/iii) NICHT -- das ist S3 (E1-Neuschnitt), insbesondere der
  T6-Stats-READ statt echter `allocate`/`deallocate`-Messung in Ebene 1.
- Sie fasst die 70 `axes/`-Header mit std-Containern NICHT an -- das ist S5.
- Sie aendert keine CSV-Spalte, kein Wire-Byte, keinen Stempel, keine Registry-XML.

**Auflagen-Bilanz (Dossier Abschn. 6):**

| Auflage | Erfuellung |
|---|---|
| 1 kein ABI-Major/POD | `abi_adapter.hpp` +102 Zeilen, 0 Deletions; `observable_tier.hpp` unberuehrt; sizeof-1344-Pin unbewegt |
| 2 golden-Tabu | 0 Treffer der Tabu-Dateien im Diff; golden-320 je Commit 3/3 gruen |
| 3 CSV additiv | keine CSV-Aenderung (S4-Ideal) |
| 4 Versions-/Tooling-Literale | nicht angefasst |
| 5 CT-Doktrin | Haertung ausschliesslich per `concept`/`static_assert`; kein Runtime-Switch, kein `std::variant` |
| 6 Ehrlichkeit | jede Aussage mit literalem Output belegt; K-CT/K-RT statt Pauschal-"konstitutiv"; Par. 6 nennt die eigene Einschraenkung |
| 7 Fixtures | keine Bestands-Fixture musste nachgezogen werden (rein additive Aenderung) |
| 8 Dual-Weg | lokal offizielle CMake/ctest-Targets; Datei-/Zahlen-Angaben aus grep abgeleitet |
| 9 Prozess | Ledger + Dossier + G8 vor der Scheibe gelesen; ASCII-only; Doku additiv |
| 10 Scope-Grenzen | keine Stempel-/Lager-/Registry-XML-/Transport-Beruehrung |
| 11 Fehlerklassen | KEIN neuer/veraenderter Mess-Fehlerpfad entstanden -- Auflage greift nicht |
| 12 Sequenz | S4 liegt VOR dem E-24-Anker-Vollzug (G8 Par. 3.2 fuehrt sie als vor-Anker-Pflicht) |

**Uebergaben:**
1. An **A8-S3**: T6 wird in Ebene 1 als Stats-READ gemessen, nicht als Achsen-Op (Par. 4, Zeile T6) --
   der Neuschnitt muss `allocate`/`deallocate` echt treiben.
2. An **A8-S5**: der Chunk-Index der KONSTITUTIVEN Kette selbst laeuft am Allokator-Achsen-Interface vorbei
   (Par. 6). Das ist B-5 im Herz der Kette, nicht in der Peripherie -- Prioritaets-Hinweis fuer den Scrub.
3. An **A8-S6**: die Matrix ist die Vorlage fuer die Ledger-/Thesis-Formulierung des Gattungs-Vertrags;
   Text-Sessions gehoeren in die Thesis, nicht ins Impl-Repo.
4. Fuer den **Abschluss-Aufraeumpass**: die Datei-Navigation im Kopf von `abi_adapter.hpp` (:37-52) traegt
   weiterhin stale Zeilen-Anker ("~:907", "~:1136", "~:1631", "~:1680"); dieser Kopf wurde von S4 bewusst
   NICHT nachgezogen (Diff-Flaeche ohne Erkenntnis, kein G8-Bezug) und gehoert in denselben Pass wie
   Posten 36 der G8-Liste.

---

ENDE.
