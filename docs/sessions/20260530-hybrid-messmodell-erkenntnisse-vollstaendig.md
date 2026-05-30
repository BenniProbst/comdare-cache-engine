# Session 2026-05-30 — Hybrid-Messmodell: User-Erkenntnisse vollständig dokumentiert

**Auslöser:** User-Direktive „das Messmodell der Diplomarbeit ist akkurat dokumentiert, such es in den
Sessions (Messung) + Architektur, setze es nach Plan um" + fünf präzisierende Folge-Nachrichten.
**Autoritative Quelle:** **Doku 24** (`docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md`),
ergänzt um **§8 Hybrid-Modell**; Doku 22 (F15-Pipeline) mit Präzisierungs-Pointer.
**Zweck dieses Docs:** die Erkenntnisse konsolidiert festhalten (User: „Bitte dokumentiere diese
Erkenntnisse vollständig"). Volltext der Diplomarbeit schreibt der Autor (`[[user-manual-workflow]]`).

---

## 1. Das HYBRID-Mess-Modell (Kern-Erkenntnis)

Der **composite Tier** (ein ganzer Suchalgorithmus = Komposition über die 17 Achsen) wird vom
**CacheEngineBuilder + Submodulen** als **Modul-Binary** (.so/.dll) gebaut (adhoc_emitter →
`comdare_build_adhoc_modules` → `AnatomyModuleLoader`). Die **Mess-Konfiguration** wählt dann einen von
ZWEI Mess-Pfaden — es ist ein **Hybrid**:

### Pfad A — isolierte Achsen-Algorithmen gegeneinander (auf der DLL selbst)
- **Wann:** Vergleich von Achsen-Algorithmus-Varianten GEGENEINANDER (welche `search_algo`-Variante /
  welcher Allocator je Achse schneller).
- **Wo:** **auf der DLL selbst** — `IMeasurableWorkload::run_workload` fährt den Mess-Workload IN der je
  Permutation kompilierten DLL und liefert Batch-Latenzen.
- **Auswertung:** host-seitige Aggregation der DLL-Samples + robuste Statistik (Median-Ranking,
  Welch-t, Mann-Whitney-U, Cliff's δ) — die Doku-22-§3-Resultate (18,5×/85× Spannen).
- **NICHT verworfen:** run_workload ist korrekt + produktiv — es ist diese Hälfte des Hybrids.

### Pfad B — composite Tier (zentral über die CacheEngineBuilder)
- **Wann:** den GANZEN Tier (alle 17 Achsen komponiert) durchmessen.
- **Wo:** **zentral host-seitig über die CacheEngineBuilder**, durch **ABI-stabilen Zugriff auf die
  Observer** des Tier-Moduls (`observe_all()` → `ObserverAggregate`).
- **Was:** BEIDE Dimensionen gemeinsam + korreliert (siehe §2).

---

## 2. Pfad B im Detail: BEIDE Dimensionen, zeit-/zustands-KORRELIERT

Die CacheEngineBuilder erhebt im Composite-Pfad **SOWOHL** die allgemeinen Tier-Metriken (Wall-Clock,
später RAM/Disk) **ALS AUCH** die **vollständigen** Achsen-Observer-Statistics (`observe_all` über ALLE
17 Achsen) — in EINEM Lauf, nicht zwei getrennte.

**Observer-Update-Trigger (Mess-Konfiguration wählt):**
- **(a) Zeitschritt-Sync:** periodisch alle Δt einen `observe_all`-Snapshot + Wall-Clock-Stempel →
  Observer-Trajektorie über die ZEIT.
- **(b) Zustands-Manipulation:** nach zustandsändernder Operation (insert/erase) einen Snapshot +
  Stempel → Observer-Entwicklung über den ALGORITHMUS-ZUSTAND (z. B. Füllstand).

**Korrelation:** jeder Observer-Snapshot trägt einen Wall-Clock-Zeitstempel → korrelierte Zeitreihe
`[(t₀, ObserverAggregate₀), (t₁, ObserverAggregate₁), …]`. Damit ist jede Per-Achsen-Statistik-Änderung
einer Wall-Clock-Zeit/Latenz-Phase ZUORDENBAR — die zwei Composite-Dimensionen sind verschränkt
auswertbar.

**Daten-Sample (Pfad B):** `{ wall_clock_ns, op_type∈{read,write,delete}, fill_level, ObserverAggregate }`.

---

## 3. Abbildung auf die drei Dimensionen (Doku 24 §2) × die zwei Pfade

| Dimension | Pfad A (DLL-selbst, isolierte Achse) | Pfad B in-process (Composite) | Pfad B über Modul-Binary-ABI |
|-----------|--------------------------------------|-------------------------------|------------------------------|
| §2.1 Tier-Wall-Clock (Füllstand, r/w/d, RAM) | ✅ run_workload + f15_compare | ✅ `tier_observe_trace.hpp` | ✅ **R6 Ink.2a** `drive_tier_observe_trace_abi` (r/w/d) |
| §2.2 Achsen-`observe_all` | (n/a, isoliert) | ✅ `AnatomyExecutionContext::observe_all` (search_algo+allocator real, uint64) | ✅ **R6 Ink.1+2b** `IObservableTier::tier_observe` (search_algo **+ allocator**, 2-dimensional) |
| §2.3 Achsen-Vergleich (Interface vs std::map) | ✅ Welch+MWU+Cliff's δ | ✅ `verify_matches_std_map` (Compile-Time) | n/a |

**R6-Implementierungs-Stand (2026-05-30, erledigt + verifiziert):** Ink.1 `IObservableTier` ABI-Sub-Interface
+ POD `ComdareTierObserverSnapshotV1` (`5b72eae`) · Ink.2a host-seitiger Füllstand-Treiber
`drive_tier_observe_trace_abi` (`4b68b13`) · Ink.2b Wall-Clock-Stempel-Korrelation (`146e6b2`) +
CSV-Persistierung `serialize_abi_tier_trace_csv` (`7c6d670`) + JSON + p50/p99 (`c8f26e8`). **Pfad-B-Schleife
geschlossen:** bauen → laden → Gattungs-API durchtesten → Observer ziehen → Wall-Clock-korrelieren → persistieren.
**Loader-Entkopplung (`6140705`):** `anatomy_module_abi_v1_decl.hpp` (leichte ABI-Schnittstelle) → Loader
ohne `abi_adapter`. **Allocator-Achse über ABI (`db4de2e`):** ComposedStore im Adapter → Cross-ABI-POD
2-dimensional (search_algo + allocator). Verbleibend (Folge-Charge): echter .dll-Round-Trip (DLL-Rebuild +
Treiber über geladenes Modul).

---

## 4. Status-Auflösungen

- **§5.5-Key-Type-Blocker AUFGELÖST:** Umstufung-A/B (2026-05-30) → alle Such-Organe über gemeinsamem
  **uint64-Key**; der Builder treibt das echte Composition-Organ verlustfrei (Pfad B). Option (A) de facto umgesetzt.
- **Entscheidung FREIGEGEBEN:** Die früher „USER-eigene/gesperrte" Mess-Modell-Entscheidung (axis_03t,
  §5.5 A/B) ist mit „nach Plan umsetzen" freigegeben — `axis_03t` ist NICHT mehr gesperrt.

---

## 5. R6 — der präzise verbleibende Implementierungs-Schritt (Pfad B über die Modul-Binary-Grenze)

**Ziel:** `observe_all` über die .dll-Grenze, sodass der host-seitige CacheEngineBuilder die korrelierte
Pfad-B-Erhebung (§2) eines permutierten Modul-Binarys durchführt.

**Bausteine:**
1. **POD-Snapshot** `ComdareTierObserverSnapshotV1` — flache C-Struktur, NUR `uint64`-Felder (search_algo:
   insert/lookup/hit/miss/peak_occupancy …; allocator: allocation_count/bytes_in_use …) → ABI-stabil,
   keine STL/vtable über die Grenze.
2. **ABI-Sub-Interface** `IObservableTier` (Muster `IMeasurableWorkload`: der ABI-Adapter erbt es
   ZUSÄTZLICH, Host-Abfrage via `dynamic_cast`, KEIN vtable-Bruch von `IAnatomyBase`):
   `tier_insert/tier_lookup/tier_erase/tier_clear/tier_size` (treiben, uint64) +
   `tier_observe(ComdareTierObserverSnapshotV1*)` (Observer-POD auslesen).
3. **Adapter-Impl** (`SearchAlgorithmAbiAdapter`): hält das getriebene echte Composition-Organ (uint64,
   Säule-1-Reconciliation erledigt), implementiert die drive-Ops + `tier_observe` via `observe_all` →
   POD-Flatten.
4. **Host-Treiber:** `drive_tier_observe_trace` über `IObservableTier*` generalisiert — Host triggert
   Operation/Zeitschritt → Wall-Clock stempeln → Observer-POD lesen → korrelierte Zeitreihe.

**Nicht-Rückbau:** Pfad A (run_workload) bleibt unverändert.

---

## 6. Geänderte Dokumente (diese Session)

- `docs/architecture/24_messmodell_korrektur_zwei_dimensionen.md` — **§8 Hybrid-Modell** (§8.1–§8.7) NEU.
- `docs/architecture/22_f15_messpipeline_und_such_bibliothek.md` — Präzisierungs-Pointer auf Doku 24 §8.
- dieses Session-Doc.

**Ende — Hybrid-Messmodell vollständig dokumentiert (2026-05-30).**
