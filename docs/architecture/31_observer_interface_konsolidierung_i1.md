# 31 — Observer-Interface-Konsolidierung (I1): von V1/V2/V3/V4 zu GENAU EINER Schnittstelle

> **Status:** umgesetzt 2026-06-05 (ABI-Major 2→3). Dieses Dokument **persistiert die architektonische Rationale**, die zuvor als
> Inline-Kommentare in `observable_tier.hpp` / `abi_adapter.hpp` / `tier_observer_v2_bridge.hpp` lebte und im Zuge der
> Konsolidierung aus dem Code entfernt wurde. Die Detail-Begründung (warum es überhaupt vier parallele Sub-Interfaces gab,
> warum sie ABI-robust getrennt waren, wie ihre Felder subsumiert wurden) ist hier vollständig erhalten — **nicht im Code
> verloren**, sondern an die richtige Stelle (Architektur-Doku) verschoben.
>
> **User-Direktive 2026-06-04 (Auslöser):** „Es gibt GENAU EINE Observer-Schnittstelle, NICHT V1/V2/V3 (und nicht das neue V4)
> separat." → [[feedback_one_consistent_observer_interface_pruefdock]].

---

## §1 Worum es geht (Pfad B, Doku 24 §8.6)

Der host-seitige `CacheEngineBuilder` kommuniziert mit einer geladenen Tier-Binary (`.so`/`.dll`) AUSSCHLIESSLICH über das
ABI-stabile Gattungs-Interface. Im Mess-Modell (HYBRID, Doku 24 §8.1) ist das **Pfad B**: der Host treibt die Gattungs-API
(`tier_insert/lookup/erase/clear/size` über `uint64`) und zieht die **IM Tier eingebauten Observer** als flachen POD über die
Modul-Binary-Grenze (`tier_observe`). Der POD quert die Grenze als **komposition-UNABHÄNGIGER, memcpy-fähiger POD** (nur
`uint64`/`int64`, fixes Layout, keine STL/vtable). **Pfad A** (`IMeasurableWorkload::run_workload*`, isolierter Achsen-Bench
in der DLL selbst) ist davon getrennt und bleibt unverändert.

## §2 Warum es historisch VIER Observer-Sub-Interfaces gab (die ABI-Robustheits-Rationale)

Der zentrale ABI-Zwang (abhaengigkeitskette §5, messarchitektur_design_observer_handle_no_dynamic_cast):

> **Eine neue Mess-Fähigkeit darf NIEMALS als neue virtuelle Methode ans vtable-Ende von `IObservableTier` (oder gar
> `IAnatomyBase`) angehängt werden.** Das ist nur bei *synchronem* DLL-+-Host-Rebuild sicher; bei einer alten DLL gegen einen
> neuen Host springt der Aufruf über eine **fremde vtable** → Absturz (beobachtet als **SEH `0xc0000005`**, Befund 2026-06-03).
> Stattdessen: jede neue Fähigkeit kommt als **eigenständiges Sub-Interface**, das der genus-typisierte ABI-Adapter ZUSÄTZLICH
> erbt. Der Host fragt es via `dynamic_cast<IObservableTierVn*>(ianatomy_ptr)` ab (**1× kalt je Modul, NIE im Hot-Loop**) — eine
> alte DLL liefert dort `nullptr` → der Host **degradiert sauber**, weil der `dynamic_cast` *fehlschlägt* statt über eine
> fremde vtable zu springen.

So entstand die Staffel V1 → V2 → V3 → V4. Jede Stufe trug exakt eine zusätzliche Mess-Dimension:

| Stufe | POD / Methode | Was es trug | Felder |
|-------|---------------|-------------|--------|
| **V1** `ComdareTierObserverSnapshotV1` / `IObservableTier::tier_observe(V1*)` | die beiden ZUERST real getriebenen Achsen | `search_algo` (axis_03a): lookup/hit/miss/insert/erase/peak — `allocator` (axis_06): bytes_allocated/bytes_in_use/alloc_cnt/dealloc_cnt/fail — + Meta (observable_axis_count, tier_fill_level) | 13 |
| **V2** `ComdareTierObserverSnapshotV2` / `IObservableTierV2::tier_observe_v2` (V42 L-74c) | V1 + die 4 OperativeCapable-Achsen | + `telemetry` (axis_11) + `memory_layout` (axis_05) + `serialization` (axis_10) + `node_type` (axis_04). Die ersten 13 Felder layout-identisch zu V1 (Migrations-Erleichterung) | 26 |
| **V3** `ComdareTierObserverSnapshotV3` / `IObservableTierV3::tier_observe_v3` (Phase A 2026-06-04) | GENERISCHE, schema-stabile `axis_stats[19][8]`-Matrix ALLER 19 SearchAlgorithm-Achsen — statt je Achse eigene named Felder (der V1/V2-Weg, der bei jeder neuen Achse einen NEUEN POD-Typ erzwang). EXAKT das `seg_ns[19]`-Muster (measurable_workload.hpp), nur 2-dimensional | 19×8 + Meta |
| **V4** `IObservableTierV4::tier_observe_timed_v3(ComdareSegmentLatencyV2*)` (Plan v2 2026-06-04) | Pfad-B **Per-Achsen-TIMING**: `seg_ns[19]` über die EINE REALE, vom Host befüllte composite-Struktur (search_organ_ + container_.chunks_ + Instanz-Organe), KEIN synthetischer Mikrobenchmark-Puffer ([[feedback_always_use_trees_for_search]] / Doku 24 §8.1 Pfad B) | seg_ns[19] |

Der V3-Schritt war die konzeptionelle Wende: ein **generischer schema-stabiler POD** (`axis_stats[T][f]`, Schema =
`kV3AxisSchema`, single-source) macht die Per-Achsen-Erweiterung um weitere Achsen zu einem reinen Befüllen weiterer
`axis_stats`-Zeilen — OHNE neuen POD-Typ und OHNE ABI-Bruch. Damit war V3 bereits der „eine generische POD" in nuce; V4 ergänzte
nur die orthogonale Zeit-Dimension (`seg_ns`).

## §3 Feld-Subsumptions-Map (Preflight-Workflow `wkqt7a0il`, 0 POD-Orphans)

Der konsolidierte POD `axis_stats[19][8] + seg_ns[19] + Meta` subsumiert JEDES frühere V1- (13) und V2-Feld (26) **verlustfrei**:

```
V1 search_algo  → axis_stats[0][0..5]   (lookup, hit, miss, insert, erase, peak)
V1 allocator    → axis_stats[6][0..4]   (bytes_alloc, bytes_in_use, alloc_cnt, dealloc_cnt, fail)
V1 obs_axes/fill→ Meta (observable_axis_count, tier_fill_level)
V2 telemetry    → axis_stats[10][0..3]  (events, leaf_updates, node_updates, peak_tracked)
V2 memory_layout→ axis_stats[5][0..4]   (scan, records, field_bytes, cache_lines, checksum)
V2 serialization→ axis_stats[9][0..3]   (serialize, records, bytes, checksum)
V2 node_type    → axis_stats[4][0..3]   (find, keys_stored, queries, checksum)
Pfad-B-Timing   → seg_ns[0..18]         (je Achse aufsummierte ns, reale Komposition)
```

Das vollständige Schema (alle 19 Achsen × bis zu 8 Felder) steht autoritativ in `observable_tier.hpp::kV3AxisSchema` (Vertrag
Schreiber-DLL ↔ CSV-Spaltenname; **kV3*-Namen bewusst beibehalten**, kein Churn). **Honest-0:** Baseline-Strategien
(PathCompressionNone, InMemoryOnly, NoMigration) liefern echte 0-Teilfelder — der Zähler folgt der echten Op, KEIN Fehler.

## §4 Die tier_observer_v2_bridge (entfallen) — was sie tat

`tier_observer_v2_bridge.hpp` (`fill_tier_observer_v2(ObserverAggregate<Composition> const&, ComdareTierObserverSnapshotV2*)`)
war die **Cross-ABI-Brücke** des ersten V2-Schritts (Doc 29 §3 Schritt 4): der `ObserverAggregate<Composition>` ist
**composition-ABHÄNGIG** (sein Layout variiert je Composition), der V2-POD war flach + versioniert + memcpy-fähig — die Brücke
mappte die 5 voll observable+getriebenen Achsen (search_algo + die 4 OperativeCapable) aus dem Aggregate generisch in den
flachen POD, jede Achse `if-constexpr`-geschützt (nicht-observable Achsen → Felder bleiben 0). **Entfällt mit I1:** der
`SearchAlgorithmAbiAdapter` füllt den konsolidierten POD jetzt direkt aus den getriebenen Organen (`fill_observer_v3` schreibt
`out->axis_stats[t][f]`), die separate Aggregate→POD-Brücke ist redundant.

## §5 Warum I1 konsolidiert — und wie die Versionierung jetzt läuft

Die Sub-Interface-Staffel war ABI-korrekt, aber sie **vermehrte** die Schnittstellen (V2/V3/V4) + erzwang host-seitig einen
`dynamic_cast`-Degrade-Pfad je Version. Die User-Direktive verlangt **GENAU EINE** konsistente Observer-Schnittstelle. I1 löst
das, indem die Versionierung von der per-Version-Sub-Interface-Ebene auf die **ABI-Major-Ebene** wandert:

- **EINE** `IObservableTier::tier_observe(ComdareTierObserverSnapshot*)` (pure virtual `= 0`), **EIN** versionierter POD.
- Die früheren `IObservableTierV2/V3/V4` + `ComdareTierObserverSnapshotV1/V2/V3` + `tier_observe_v2/v3/timed_v3` + `fill_observer_v2`
  + die Bridge sind **entfernt**.
- Versionierung jetzt über **`COMDARE_ANATOMY_ABI_MAJOR` 2→3** (Minor→0, Magic `.A2.`→`.A3.`). Der Loader lehnt inkompatible
  Alt-DLLs per **Major-Mismatch** ab (`AnatomyAbiVersion::host_compatible_with`: `major==host.major && module.minor<=host.minor`;
  `status_abi_major_mismatch`). Das ersetzt den dynamic_cast-Degrade durch einen **sauberen Loader-Reject** — der korrekte Ort
  für einen *echten* Layout-/vtable-Bruch (im Gegensatz zu einer additiven Fähigkeit, die weiter ein Sub-Interface bekäme).

> **Trade-off bewusst:** Ein additiver Fähigkeits-Zuwachs (z.B. eine weitere orthogonale Mess-Dimension) wäre weiterhin als
> eigenständiges Sub-Interface ABI-kompatibel nachrüstbar (alte DLLs bleiben gültig). I1 ist KEIN solcher additiver Schritt,
> sondern eine **Layout-Konsolidierung** (ein POD ersetzt vier) → sie MUSS den Major-Bump reiten und ALLE Permutations-DLLs neu
> bauen. Genau dafür existiert der Major-Reject-Pfad.

## §6 Die Q1-Sequenz in der EINEN `tier_observe` (gegen Doppelzählung)

Weil der konsolidierte POD Observer-Stats UND Pfad-B-Timing in EINEM Aufruf trägt, ist die **Reihenfolge zwingend** (Preflight
Q1): **(1) `axis_stats`-READ** (vor dem Timing, `fill_observer_v3` liest die akkumulierten Real-Workload-Zähler) → **(2) `seg_ns`-Timing**
(`fill_segment_timing_v3` treibt die per-op-Organe) → **(3) per-op-Reset** (am Ende von `fill_segment_timing_v3`). Ohne diese
Ordnung würden die per-op-getriebenen Achsen (T0/T1/T2/T3/T7/T8/T10/T17/T18) **doppelt** gezählt (einmal im Workload, einmal im
Timing). Der Host zieht ohnehin keinen separaten Timing-Call mehr — die EINE `tier_observe` liefert alles korreliert.

## §7 Was getrennt bleibt (Pfad A)

`IMeasurableWorkload` / `IMeasurableWorkloadV2` / `IMeasurableWorkloadV3` (`run_workload`, `run_workload_segmented`,
`run_workload_segmented_v2`) + `ComdareSegmentLatencyV1/V2` bleiben **unangetastet**: das ist **Pfad A** (isolierter
Achsen-Bench in der DLL selbst, Doku 24 §8.1), orthogonal zu Pfad B und weiterhin Eingangspunkt für den isolierten
Achsen-Wall-Clock-Vergleich (Holm/MWU/Cliff's δ, Doku 22 §3). `seg_ns` im konsolidierten Observer-POD = **Pfad B** (reale
Komposition), NICHT Pfad A.

---

### Querverweise
- `docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md` §8.1/§8.6/§8.7 (HYBRID-Modell, Pfad A/B)
- `docs/architecture/abhaengigkeitskette_lebewesen_pruefdock_abi_konvergenz.md` §5 (der EINE Mess-POD + ABI-Major)
- `docs/architecture/messarchitektur_design_observer_handle_no_dynamic_cast.md` (1× kalt dynamic_cast je Modul)
- `docs/architecture/28_vollstaendigkeits-kartographie.md` §1 (die 19/22 Achsen + kV3AxisSchema-Verortung)
- `docs/architecture/30_audit_achsen_delegation_pflichtachsen.md` Befund 2 (Storage-Delegation, alle Achsen Pflicht)
- Session: `docs/sessions/20260604-observer-konsolidierung-und-mess-echtheit.md` + `20260605-UEBERGABE-START-HIER-observer-konsolidierung.md`
- `[[feedback_one_consistent_observer_interface_pruefdock]]` · `[[feedback_always_use_trees_for_search]]` · `[[feedback_no_success_marks_without_literal_output]]`
