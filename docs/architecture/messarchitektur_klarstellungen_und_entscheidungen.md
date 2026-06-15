# Mess-Architektur — Klarstellungen & Entscheidungen (Gesprächsrunde 2026-05-31)

> **Zweck:** Konsolidierte Aufzeichnung ALLER Architektur-Klarstellungen aus der Design-Gesprächsrunde zur
> einheitlichen Mess-Abstraktion (P0/I1). Ergänzt — nicht ersetzt — die Detail-Dokumente:
> - `abhaengigkeitskette_lebewesen_pruefdock_abi_konvergenz.md` (die Lebewesen-Binary-seitige Kette + ABI-Konvergenz)
> - `20260531-mess-abstraktion-cross-platform-architektur-plan.md` (der Gesamt-Plan, Sessions)
> - **In Arbeit (Workflow `wjr7lwpp3`):** der No-`dynamic_cast`-Handle-Mechanismus + `dynamic_cast`-Timing-Befund →
>   wird hier nachgetragen, sobald verifiziert.
>
> **Status:** Entscheidungs-/Klarstellungs-Log. Noch KEIN Code geändert.

---

## 0. Die zentrale Korrektur — ZWEI Seiten, nicht eine Kette

Frühere Diagramme stellten Prüf-Dock-Wahl und Lebewesen-Kette in *eine* Linie. **Richtig sind ZWEI Seiten**, die sich
nur an der ABI-Grenze (dem geladenen Handle) treffen:

```
   LEBEWESEN-BINARY-SEITE (in der .dll, Modul-Autor)        │ HANDLE │   CACHEENGINEBUILDER-SEITE (Host)
═════════════════════════════════════════════════════ │ ══════ │ ═══════════════════════════════════════════════
 LEBEWESEN  IExecutionEngine                           │        │
   └ GATTUNG  AnatomyGenus  ─────────────────── genus()┼────────┼──►  PRÜF-DOCK-WAHL (matcht GATTUNG)
       └ LEBEWESEN-ART  Composition (17 Organe) ─────────────┼────────┼──►  PruefDockRegistry::select_for→ SearchAlgorithmDock
           └ ANATOMIE  SearchAlgorithmAnatomy<C>       │        │        (ein Prüf-Dock je Gattung, host-seitig)
               └ [OPTIONAL, cmake-Flag] Observer       │        │
                   observe_all = Aggregat aller        │        │     IMeasurableWorkload-LASTPROFILE (matcht LEBEWESEN-ART/ANATOMIE)
                   Teil-Observer                        │        │        - YCSB-Daten + Zugriffsmuster, host-seitig
                                                        │        │        - MEHRERE Profile je geladener Binary
   tier_insert/lookup/erase  ◄──────── Operation+Param ┼────────┼──   gegen den Handle ins Lebewesen getrieben
   observe_all()  ──────────────────── Observer-Snap ──┼────────┼──►  korreliert + persistiert (nur im Messmodus)
```

**Merksätze:**
- **Prüf-Dock ↔ Gattung** (host-seitig; ein Dock je `AnatomyGenus`; matcht über `genus()` des Handles).
- **`IMeasurableWorkload`-Lastprofil ↔ Lebewesen-Art/Anatomie** (host-seitig; **mehrere** Profile je Binary möglich).
- **Observer ↔ cmake-Flag** (in der Binary; optional; wenn eingebaut, dann Vertrag = exportiert).

---

## 1. `IMeasurableWorkload` = host-seitiger BELASTUNGSPLAN (nicht Observer, nicht DLL-intern)

- `IMeasurableWorkload` beschreibt einen **Belastungsplan eines Lebewesens**: YCSB-Testdaten + Zugriffsmuster
  (Operationsfolge), **auf der CacheEngineBuilder-Seite**, gegen den Prüf-Dock-Handle **ins Lebewesen getrieben**.
- Es ist **vollkommen disjunkt vom Observer**: das Lastprofil sagt *was/in welcher Reihenfolge* getrieben wird;
  der Observer sagt *was dabei intern gemessen wird*.
- **Es kann MEHRERE `IMeasurableWorkload`-Lastprofile geben**, die gegen JEDE geladene Lebewesen-Binary untersucht
  werden (z. B. die OP-1..OP-6-Profile aus `op_type_filter` / YCSB A–F) → pro Binary eine *Menge* von Messläufen.
- Das Lastprofil muss zur **Lebewesen-Art/Anatomie** passen (z. B. Key/Value-Typ, unterstützte Operationen).

> **Offen (Design-Workflow):** Im IST-Code ist `IMeasurableWorkload::run_workload` ein **DLL-internes** Self-contained-
> Workload (Pfad A, `abi_adapter.hpp:146`). Die Vision „host-seitiger Belastungsplan, gegen den Handle getrieben"
> entspricht eher **Pfad B** (`tier_insert/lookup/erase` host-getrieben). Der Design-Workflow klärt, ob Pfad A
> (DLL-internes Workload) erhalten bleibt, durch host-getriebene Sequenzen ersetzt oder beide koexistieren.

---

## 2. Observer = optionales cmake-Flag, Vertrag-wenn-präsent

- Statistik/Observer sind **abschaltbar** (`#ifdef COMDARE_CE_ENABLE_STATISTICS`, s. `abi_adapter.hpp:254`):
  Kompilierung **ohne** → „reines Lebewesen" (nur funktionale API, kein Messmodus).
- **Ist der Observer einkompiliert (Messmodus), MUSS er exportiert werden → ein Vertrag existiert.**
- Eine Messmodus-Binary hat **EINEN einkompilierten Observer**, der **alle Teil-Observer enthält + abfragt**
  (`observe_all` = Aggregat über alle aktiven Achsen-Observer).
- Konzept-Skizze des Users: über den Handle eine Init-Methode `template Observer init(){ return observe_all }`,
  danach die Operation `Tier(constexpr AxesList <3,2,6,5,..>, API-call erase("suchwort"))`.

---

## 3. EINE Schnittstelle: Operation + Parameter + Observer zusammen, COMPILE-TIME, OHNE per-Op-`dynamic_cast`

- Es soll **EINE Schnittstelle** geben, die **Handle-/Prüf-Dock-Operation + deren Parameter + den Observer**
  zusammen überträgt — als **eine** Brücken-Operation, nicht als getrennte Probes.
- **Kein per-Operation-`dynamic_cast`**: `dynamic_cast` ist langsam und erzeugt Latenz, die bei latenzkritischen
  Messungen nicht tolerierbar ist. → Capability/Observer-Auflösung **einmalig** (beim Laden / über ein
  Versions-Capability-Bit / über eine Funktionszeiger-Tabelle in der Factory-Rückgabe), im Hot-Loop nur noch
  direkter Aufruf.
- **Template-Vision ABI-konform:** Templates lösen **IN der DLL** auf (jede Binary ist EINE fixe Composition mit
  fixer AxesList) — die ABI exponiert nur das **aufgelöste Ergebnis** (fixe Funktionszeiger / fixer POD). Die
  „constexpr AxesList"/„template Observer init" sind compile-time *in* der DLL; nach außen quert nur das Resultat.

> **✅ GEKLÄRT (Design-Workflow `wjr7lwpp3`, verifiziert 2026-05-31 — Detail: `messarchitektur_design_observer_handle_no_dynamic_cast.md`):**
> Der `dynamic_cast<IObservableTier*>` ist **KALT** — exakt **1× pro Modul** beim `measure()`-Eintritt
> (`search_algorithm_dock.hpp:42`), per Referenz an die **cast-freie** Hot-Loop gereicht
> (`tier_observe_trace_abi.hpp:82,89-135` enthält NULL Casts, nur Virtual-Calls). **Die Latenz-Sorge ist auf dem
> Mess-Weg bereits eingehalten** (1 Cast/Modul bei Σ(writes+~2000 lookups+~200 erases)/Checkpoint Operationen).
> Ebenso: Operation+Observer sind in `IObservableTier` **bereits in EINER Schnittstelle vereint**
> (`tier_insert`=Op+Param, `tier_observe`=Observer, gleiches Objekt). Beide meistgefürchteten Vision-Punkte sind
> also schon erfüllt. **Empfohlener Härtungs-Schritt:** Capability-Bit als additives 5. `extern "C"`-Symbol
> (`comdare_anatomy_capabilities()`) → Host weiß Observer-Präsenz ohne Cast (statt aus `nullptr` raten).

---

## 4. Anatomy-ABI und Observer-Tier-Schnittstelle sind DISJUNKT

| Konzept | Was | Pflicht? | Wo |
|---------|-----|----------|-----|
| **Anatomy-ABI** | funktionale Pflicht-Schnittstelle: `comdare_create/destroy_anatomy`, `genus()`, `tier_insert/lookup/erase/clear/size` + Funktional-Tests + Verbose-Trace | **PFLICHT** (jedes Lebewesen) | `anatomy_module_abi_v1_decl.hpp`, `anatomy_base.hpp`, `observable_tier.hpp` (tier-Ops) |
| **Observer-Tier-Schnittstelle** | Mess-Observer: `observe_all` → Snapshot-POD | **OPTIONAL** (cmake-Flag; Vertrag wenn präsent) | `observable_tier.hpp` (tier_observe), `observer_aggregate.hpp` |

- Ohne Messmodus: der Host fährt **nur generische Funktional-Tests** gegen die API der zu prüfenden Binary —
  mit **Verbose-Logs** über Binary / API / Methode + **Trace** (Interface-Funktion auf der Binary, Zeitpunkte +
  Reihenfolge). Keine Messung.
- Mit Messmodus: zusätzlich der einkompilierte `observe_all`-Observer, der zusammen mit der Operation über die
  EINE Schnittstelle quert.

---

## 5. Host-seitige Zuordnung (die Matching-Matrix)

| Host-Artefakt (CacheEngineBuilder) | matcht | Mechanismus |
|------------------------------------|--------|-------------|
| **Prüf-Dock** (z. B. `SearchAlgorithmDock`) | **GATTUNG** (`AnatomyGenus`) | `PruefDockRegistry::select_for(handle)` → `handle.genus()` → Dock je Gattung |
| **`IMeasurableWorkload`-Lastprofile** (1..n) | **LEBEWESEN-ART/ANATOMIE** | Profil-Eignung gegen Composition/Key-Value-Typ; **mehrere** Profile je Binary getrieben |
| **Observer-Erhebung** | **cmake-Messmodus** der Binary | Capability (einmalig aufgelöst), nur wenn einkompiliert |

→ Ein vollständiger Mess-Durchlauf je Binary = `genus()` → richtiges Prüf-Dock → **Schleife über alle passenden
Lastprofile** → je Profil: Operationsfolge gegen den Handle treiben + (im Messmodus) `observe_all` korreliert ziehen.

---

## 6. Konsequenz für I1 (vorläufig, bis Design-Workflow landet)

Die ABI-Konvergenz-Notation (`abhaengigkeitskette_…md`) bleibt gültig für die **Lebewesen-Binary-Seite** + den POD.
Diese Runde ergänzt **host-seitig**:
- Prüf-Dock-Wahl per `genus()` ist KORREKT host-seitig (war in der Notation bereits so verortet).
- **Neu:** mehrere `IMeasurableWorkload`-Lastprofile je Binary → der Mess-Treiber (`drive_tier_observe_trace_abi`
  / `measure_genus_sequential`) muss eine **Profil-Schleife** vorsehen, nicht ein einzelnes Workload.
- **Neu:** No-per-Op-`dynamic_cast` → die Observer-Auflösung wird aus dem Hot-Loop herausgezogen (Detail folgt).
- **Neu:** Messmodus vs. funktional-only als zwei Build-Profile (cmake-Flag) → die ABI/der Loader müssen
  „diese Binary ist im Messmodus" erkennen können (Capability-Bit statt Probe).

Revidierte, vollständige I1-Anfass-Liste folgt mit dem Design-Workflow-Ergebnis.

---

## 7. Offene Entscheidungen — Stand nach Design-Workflow

1. ✅ **GEKLÄRT:** `dynamic_cast` ist **kalt** (1×/Modul) → No-Cast-Umbau im Hot-Path **nicht nötig**; nur Capability-Bit-Härtung empfohlen.
2. ⬜ **Pfad A (DLL-internes `run_workload`) vs. Pfad B (host-getriebene Sequenz)** — Koexistenz oder Ablösung? (Vision „host-seitiger Belastungsplan" = Pfad B; aktuell beide vorhanden.) **Entscheidung offen.**
3. ✅ **EMPFOHLEN:** Capability-Bit als **additives 5. `extern "C"`-Symbol** `comdare_anatomy_capabilities()` (ABI-Major bleibt 1 für diesen Teil; `dynamic_cast`-Fallback für alte DLLs). Funktionszeiger-Tabelle (Mechanik 3) nur falls Cross-Compiler-RTTI unzuverlässig.
4. ⬜ **Profil-Schleife** (n Lastprofile je Binary) — Registrierung/Eignungs-Match gegen Lebewesen-Art noch zu entwerfen.
5. ✅ **GEKLÄRT:** Verbose-**Op**-Trace existiert NICHT (Trace ist checkpoint-aggregiert) → **NEU** als `ITierTraceSink` fürs funktional-only-Profil (C1), compile-time-gated, nie im Mess-Heißpfad.

**Entscheidungs-Kandidat für I1 (User):** B1 „Interface-Split `IObservableTier`→`IDrivableTier`+`IObservableTier` für *vollkommen disjunkt*" ist ein vtable-Eingriff → reitet auf dem ohnehin geplanten `COMDARE_ANATOMY_ABI_MAJOR 1→2` mit (eine Charge: POD-Unifikation + Split + Capability-Bit). Funktional erfüllt der Ist-Stand die Disjunktheit bereits; B1 = Reinheit. **Mitmachen (Vision wörtlich) oder funktional belassen?**
