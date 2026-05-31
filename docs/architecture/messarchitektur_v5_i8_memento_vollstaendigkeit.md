# V5-I8 — memento_all-Vollständigkeitsanalyse (Disk-Achsen) · IST-verifiziert 2026-05-31

> **Disziplin:** [[feedback_verify_ist_state_before_gross_tasks]] + [[feedback_no_success_marks_without_literal_output]]
> + [[feedback_no_quick_fixes]]. Diese Notiz hält fest, was der reale Code TUT — NICHT was eine künftige
> Disk-Persistenz tun würde. I8 wird durch **Ist-Stand-Feststellung** aufgelöst, NICHT durch Erfinden von
> Fake-Disk-Checkpoint-Code für Achsen, die nichts persistieren.

## Frage (I8 / Risiko R1)

Das V5-Design (§3) nennt drei „Disk-residente" stateful Achsen — `io_dispatch`, `serialization` (axis_10),
`migration_policy` — für die „ein einfacher Snapshot NICHT reicht" und ein expliziter Disk-Checkpoint im
`save_state`/`restore_state` nötig sei. I8 sollte diesen Checkpoint implementieren.

## Befund (literal verifiziert)

**1. Kein echtes Disk-I/O im gesamten Achsen-Code.** Grep nach `fstream|ofstream|ifstream|fopen|fwrite|
std::filesystem|::write(|::read(|FILE*|mmap|munmap|CreateFile|MapViewOfFile|VirtualAlloc|pwrite` über
`libs/cache_engine/topics/**/{io,serialization,migration}/**` und `libs/cache_engine/axes/**/
{io_dispatch,serialization_axis,migration_policy}/**`:
- Einziger Treffer = `axes/io_dispatch/axis_io_mmap.hpp` — und dort matcht nur das **Wort „mmap" in
  `family_name()`** (`"MmapIo (mmap file-backed, Persistent Memory, read-heavy)"`) bzw. der Include-Pfad.
  KEIN echter `mmap()`/`CreateFileMapping`-Aufruf, KEIN File-Handle.

**2. Die „Disk"-Achsen sind reine Compile-Time-Strategie-Deskriptoren.** `MmapIo` (io_dispatch) besteht
ausschließlich aus `static constexpr` (`enabled`, `name()`, `family_name()`, `flag_suffix()`,
`is_in_memory_only()`) + Typ-Aliassen (`topic_tag`/`axis_tag`/`family_id`). **Null nicht-statische Member.**
Gleiches für `serialization_axis/*` (z.B. `RawBinarySerialization`, `…_compressed`, `…_succinct`,
`…_var_len`) und `migration_policy/*` (`HotColdMigration`, `TierBased`, `Adaptive`, `None`): alle erben
`SerializationStrategyBase`/`MigrationStrategyBase` → `topics::AxisBase` (selbst stateless: nur static
`get_compiler()`/`is_original_module()`-Defaults). Grep nach nicht-statischen Datenmembern fand in den
Strategie-Headern KEINE.

**3. Der ABI-Adapter instanziiert diese Achsen gar nicht als Member.** `SearchAlgorithmAbiAdapter` hält als
einzigen Laufzeit-Zustand `search_organ_` (das Such-Organ) + `container_` (`ObservableComposedSearch` über
`ComposedStore<node_type, memory_layout, allocator>`). `serialization`/`io_dispatch`/`migration_policy`/
`concurrency`/`value_handle`/`filter` werden NUR compile-time benutzt (z.B. `Serializer::serialize_scan(...)`
als statischer Aufruf in `run_workload`), nie als zustandsbehaftete Member gehalten.

## Schlussfolgerung

Für die **aktuelle** Implementierung ist `memento_all` **vollständig und exakt** — es gibt keinen
Disk-Zustand und keinen weiteren instanziierten Achsen-Zustand zum Zurückrollen:

| Achse (Design-„stateful") | Laufzeit-Zustand im Adapter? | Memento-Behandlung (korrekt) |
|---|---|---|
| search_algo | ja — `search_organ_` | I6 Tiefkopie (exakt) |
| node_type / memory_layout / allocator | ja — in `container_` (`ComposedStore`) | I6 Tiefkopie (exakt) |
| serialization / io_dispatch / migration_policy | **nein** — stateless Compile-Time-Deskriptor | `EmptyMemento` (no-op = korrekt) |
| concurrency / value_handle / filter | **nein** — nicht als Member instanziiert | `EmptyMemento` (no-op = korrekt) |

⇒ **I8 ist im Ist-Stand gegenstandslos**: es existiert kein Disk-State, der gecheckpointet werden müsste.
Einen Fake-Disk-Checkpoint zu schreiben wäre erfundener Zustand ([[feedback_no_quick_fixes]] /
[[feedback_no_success_marks_without_literal_output]]).

## Vorwärts-Kontrakt (für künftige echte Disk-Persistenz)

Sobald eine Achse **echten** Laufzeit-/Disk-Zustand erhält (z.B. ein `MmapIo` mit echtem File-Mapping, oder
eine `migration_policy` mit persistenter Hot/Cold-Tabelle), ist der Erweiterungspunkt bereits gebaut:
- Die Achse implementiert `MementoAxis` (`memento_t` + `save_state()`/`restore_state()`, V5-I5) — `memento_t`
  trägt dann den Disk-Checkpoint (Datei-Offset/-Kopie/Journal), nicht nur einen flachen Snapshot.
- `MementoAggregate` (V5-I5) nimmt sie automatisch über `memento_of_t` auf.
- Der Adapter hält dann eine reale Instanz der Achse und ruft `save_axis`/`restore_axis`; `tier_save_all`/
  `tier_rollback_all` (V5-I6) + der Zwei-Phasen-Treiber (V5-I7) routen unverändert.
Diese echte Disk-Persistenz hängt an der breiteren „echtes mmap/Disk-I/O"-Arbeit (V42/Zukunft) und ist
KEIN aktuell-actionable V5-Punkt.

## Konsequenz für I6

Die frühere Formulierung „I6 In-Memory-Mechanismus, Disk-Achsen folgen in I8" wird präzisiert: `memento_all`
ist für die **aktuellen** Achsen-Implementierungen bereits **vollständig** (nicht nur In-Memory-Teilmenge).
Der „I6-Rest" (per-Achsen-Granularität + in-process `memento_all()` auf `SearchAlgorithmAnatomy`) bleibt ein
reines Vorwärts-Refinement ohne aktuelle Abdeckungslücke.
