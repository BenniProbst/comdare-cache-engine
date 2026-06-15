# DESIGN — Operation + Parameter + Observer über EINEN Handle, ohne per-Op-`dynamic_cast`

> **Provenienz:** Workflow `wjr7lwpp3` (4 Explore-Recherche + adversarielle `dynamic_cast`-Timing-Verifikation +
> Synthese, 2026-05-31). Alle Datei:Zeile literal nachgelesen. Annahmen als **[ANNAHME]**. Ergänzt
> `messarchitektur_klarstellungen_und_entscheidungen.md` + `abhaengigkeitskette_lebewesen_pruefdock_abi_konvergenz.md`.

> ⚠️ **BEGRIFFS-BANNER (korr. 2026-06-03, s. Doc 30 §8.0; verbatim Doc 24 §8.8 + Doku 14 §25):** Dieses Dokument
> verwendet „**Gattung**" / „**Gattungs-Ops**" / „**Gattungs-Antrieb**" durchgängig für die **Außen-Interface-Ebene =
> die SearchAlgorithm-Gattung** — also für die ABI-stabile Treiber-API `tier_insert/lookup/erase/clear/size`, mit der
> die `CacheEngineBuilder` ein geladenes Lebewesen-Modul durchtreibt (Quelle: `observable_tier.hpp:7-8` „über das ABI-stabile
> **Interface der GATTUNG (SearchAlgorithm)** … testet die **Gattungs-API** durch (tier_insert/lookup/erase)"). **In
> diesem Sinn ist „Gattung" KORREKT und bleibt** (3-Ebenen-Modell: **GATTUNG = ein INTERFACE / Prüf-Dock für die
> Außenwelt = SearchAlgorithm / Container / Graph**). Zur Vermeidung der Kategorienfehler-Lesart, die andere Dokumente
> betraf, hier die Verortung der in diesem Doc vorkommenden Begriffe in den drei Ebenen:
> - **(1) GATTUNG = Außen-Interface / Prüf-Dock** (SearchAlgorithm / Container / Graph): die Treiber-Ops
>   `tier_insert/lookup/erase` + `genus()` (Dock-Diskriminator) gehören HIERHER → „Gattungs-Ops"/„Gattungs-Antrieb"
>   = korrekt.
> - **(2) LEBEWESEN-UNTERKLASSE** = liegt UNTER dem Gattungs-Interface und trägt den **FESTEN Achsen-Satz** (Doku 14 §25/§26).
>   Was dieses Doc „**fixe Composition**" / „AxesList `<3,2,6,5,…>`" / „einkompilierter Observer" / `AdHocComposition<17>`
>   nennt, sitzt auf DIESER Ebene (heute ist genau **EINE** Lebewesen-Unterklasse gebaut — die std::map-ähnliche
>   SearchAlgorithm-Lebewesen-Unterklasse), **NICHT** auf der Gattungs-Ebene.
> - **(3) ACHSEN = Organe der Lebewesen-Unterklasse, NIE optional.** „**Optionales Sub-Interface**" (IObservableTier /
>   IMeasurableWorkload) und „nur die zwei getriebenen Achsen" / `observable_axis_count==0` / „kollabiert auf Nullen"
>   beschreiben, WELCHE **Observer-Snapshots** über die ABI-Grenze queren (Mess-Profil-Frage), **NICHT** dass eine Achse
>   fehlt: jedes Lebewesen-Binary trägt alle 17 Organe uniform; eine nicht-puffernde Komposition wählt einen KONKRETEN
>   Durchreich-Algorithmus (NoBuffer/NoFlush/NonePrefetch/None/NoMigration), statt „eine Achse wegzulassen".
> - Die **Invariante** (feste Slot-Zahl pro Lebewesen-Unterklasse, `AdHocComposition<17>` als ABI-Identität) bleibt
>   unverändert gültig — sie ist eine **Lebewesen-Unterklassen**-Invariante. queuing q1/q2 (kommen in diesem Doc nicht vor)
>   wären — wo sie aufträten — **Pflicht-Achsen dieser Lebewesen-Unterklasse**, kein Interface, keine Gattung.

---

## 1. Wie ist es HEUTE gelöst?

### 1.1 Drei disjunkte vtables, eine ABI-stabile Wurzel
Die geladene Binary exportiert genau EINE `IAnatomyBase`-Instanz über eine `extern "C"`-Factory. Der Adapter erbt **dreifach** (`abi_adapter.hpp:76-77`):
```cpp
template <AnatomyConcept A>
class SearchAlgorithmAbiAdapter final : public IAnatomyBase, public IMeasurableWorkload, public IObservableTier {
```
- **`IAnatomyBase`** (`anatomy_base.hpp:99`, erbt `IExecutionEngine`) = PFLICHT-Wurzel-vtable: `genus/composition_name/paper_id/organ_count` + Lifecycle. Nie ändern (alte DLLs).
- **`IMeasurableWorkload`** (`measurable_workload.hpp:21`) = Pfad A (`run_workload`, Mess-Last IN der DLL).
- **`IObservableTier`** (`observable_tier.hpp:80`) = Pfad B: Gattungs-Ops `tier_insert/lookup/erase/clear/size` (uint64) **UND** `tier_observe(Snapshot*)` — **Operation und Observer bereits in einer Schnittstelle vereint**. (Zu „Gattungs-Ops" = Treiber-API des SearchAlgorithm-**Außen-Interfaces**, nicht der fixen Achsen-Komposition: s. Begriffs-Banner oben, korr. 2026-06-03, Doc 30 §8.0.)

Sub-Interfaces hängen **bewusst nicht** an `IAnatomyBase` (Kommentar `observable_tier.hpp:13-17`) → vtable-Stabilität; Host probt per `dynamic_cast`.

### 1.2 Observer quert als flacher POD
`observable_tier.hpp:40-67`: `ComdareTierObserverSnapshotV1` = nur uint64 (search\_* + alloc\_* + Meta), `static_assert standard_layout + trivially_copyable`. `observe_all` füllt search+alloc in EINEN POD (`abi_adapter.hpp:251-279`).

### 1.3 cmake-Gating real, zweistufig
`CMakeLists.txt:51-56`: `option(COMDARE_CE_ENABLE_STATISTICS … ON)`. Observer-Body `abi_adapter.hpp:254-275` doppelt gegated: (1) `#ifdef COMDARE_CE_ENABLE_STATISTICS` (Code raus), (2) `if constexpr (ObservableAxis<…>)`. Bei OFF bleiben `tier_insert/lookup/erase/clear/size` voll funktional (`:223-249`); nur `tier_observe`-Inneres kollabiert auf Nullen.

### 1.4 Der `dynamic_cast`: KALT, nicht heiß — Latenz-Sorge entschärft (VERIFIZIERT)

| Stelle | Datei:Zeile | Frequenz |
|---|---|---|
| Pfad-B-Cast `dynamic_cast<IObservableTier*>` | `search_algorithm_dock.hpp:42` | **1× pro `measure()`**, gecacht + per Referenz weitergereicht |
| `measure()`-Aufrufer | `pruef_dock_sequencer.hpp:57-67` | **1× pro Modul-Handle** (`for h : handles`), NICHT pro Op |
| Hot-Loop | `tier_observe_trace_abi.hpp:89-135` | nimmt `IObservableTier& tier` (Sig. :82); **NULL `dynamic_cast`**, nur Virtual-Calls |
| Pfad-A-Cast `dynamic_cast<IMeasurableWorkload*>` | `f15_compare/main.cpp:205` | **1× pro DLL**, gecacht |
| Ops selbst | `observable_tier.hpp:87-105` | reine `virtual … noexcept = 0` (vtable-Dispatch), kein verstecktes RTTI |

**Fazit:** pro Modul genau **1 Cast** (`O(1)/Modul`), bei Op-Zahl/Modul = Σ(writes + ~2000 lookups + ~200 erases) je Checkpoint. **Die Latenz-Sorge ist auf dem Mess-Weg bereits eingehalten.**

**Was NOCH fehlt ggü. der Vision:** (a) keine per-Op-Verbose-Trace-Zeile (Trace ist checkpoint-aggregiert, `tier_observe_trace_abi.hpp:143-188`); (b) kein nach außen exponierter AxesList-Deskriptor `<3,2,6,5,…>` (nur `composition_name/paper_id/organ_count`); (c) keine explizite `init()`-Methode (observe_all = `tier_observe`-Body); (d) **kein Capability-Bit** im Handshake (nur 4 Symbole version/magic/create/destroy; „hat Observer?" wird über `dynamic_cast==nullptr` bzw. `observable_axis_count==0` geraten).

---

## 2. Soll-Design: EINE Schnittstelle, kalt aufgelöst, heiß nur Virtual

### 2.1 Leitprinzip „Template in der DLL, fixe Funktionszeiger nach außen"
> **Die DLL IST eine fixe Composition.** Das Template-Permutations-Wissen (AxesList `<3,2,6,5,…>`, `if constexpr (ObservableAxis<…>)`, `observe_all`) löst **innerhalb der DLL zur Compile-Zeit** auf (genau das tut `SearchAlgorithmAbiAdapter<A>` heute). Nach außen exponiert die ABI nur das **aufgelöste Ergebnis** (stabile vtable + einkompilierter Observer). Der Host treibt generisch über `tier_insert/lookup/erase` (uint64-Common-Key) und zieht `tier_observe`.

→ „Eine Schnittstelle überträgt Op+Param+Observer zusammen" ist **bereits erfüllt** durch `IObservableTier`; der Template-Charakter lebt IN der DLL, die ABI ist die aufgelöste Projektion.

### 2.2 Capability-Resolution EINMAL beim Laden — drei Mechaniken
- **Mechanik 1 (Ist, beibehalten):** Cast-once + Referenz-Caching. `search_algorithm_dock.hpp:42` castet einmal, gibt `*tier` per Referenz an `drive_tier_observe_trace_abi(IObservableTier&, …)`. Der Treiber **kann** strukturell nicht re-casten. Tragend, bleibt.
- **Mechanik 2 (NEU, empfohlen): Capability-Bit im Versions-Handshake.** 5. `extern "C"`-Symbol, **additiv** (kein vtable-Bruch):
  ```cpp
  extern "C" COMDARE_ANATOMY_ABI_EXPORT std::uint64_t comdare_anatomy_capabilities() noexcept;
  // Bit 0 = IMeasurableWorkload, Bit 1 = IObservableTier(drivable), Bit 2 = Observer eingebaut (STATISTICS=ON)
  ```
  Loader (`anatomy_module_loader.cpp:122-135`) resolved es als 5. optionales Symbol, cached `has_observer` im `AnatomyModuleHandle`. Host weiß **vor** jedem Cast, ob er lohnt; Build-Profile explizit signalisiert statt aus `nullptr` geraten. **ABI-Major bleibt 1** (additiv; fehlt es → `caps=0`, `dynamic_cast`-Fallback). **[ANNAHME]** additives optionales Symbol = kein ABI-Bruch (konsistent mit `anatomy_module_abi_v1_decl.hpp:8-12`).
- **Mechanik 3 (Funktionszeiger-Tabelle, optional):** Factory liefert zusätzlich eine `ComdareObserverVtableV1*` (rohe `noexcept`-fnptrs) → RTTI **ganz** weg, cross-compiler-robust. **NICHT jetzt** (Hot-Path schon kalt, erhöht Boilerplate); nur falls Cross-Compiler-RTTI über die `.dll`-Grenze unzuverlässig wird (typeinfo-Identitäts-Falle bei Mehrfach-Vererbung).

### 2.3 Hot-Loop bleibt rein virtual
Nach der einmaligen Resolution treibt der Hot-Loop nur Virtual-Calls auf der gecachten Referenz (`tier_observe_trace_abi.hpp:97-132`). **Keine Hot-Path-Änderung nötig.**

---

## 3. Disjunktheit konkret

### 3.1 PFLICHT-Anatomy-ABI (immer exportiert, funktional, vtable-stabil)
4 `extern "C"`-Symbole (`anatomy_module_abi_v1_decl.hpp:58-76`) · `IAnatomyBase` genus/composition_name/paper_id/organ_count (`anatomy_base.hpp:117-126`) · Lifecycle (`abi_adapter.hpp:97-112`) · Gattungs-Ops `tier_insert/lookup/erase/clear/size` (`observable_tier.hpp:87-99`) · Funktional-Tests (verify_matches_std_map).

> **Entwurfs-Schärfung [Empfehlung]:** Die Gattungs-Ops stehen heute physisch in `IObservableTier` zusammen mit `tier_observe`. Für „**vollkommen disjunkt**" (User): `IObservableTier` splitten in
> - **`IDrivableTier`** (NEU, Pflicht): `tier_insert/lookup/erase/clear/size` — funktionaler Gattungs-Antrieb.
> - **`IObservableTier`** (Rest, optional): nur `tier_observe()`; erbt `IDrivableTier` ODER separat geprobt.
>
> So exportiert ein „reines Lebewesen" (STATISTICS=OFF) `IDrivableTier`, aber kein `tier_observe`. Heute wird das umgangen (Observer kollabiert bei OFF auf Nullen) — funktional ok, aber nicht „vollkommen disjunkt". Dieser Split ist die EINZIGE strukturelle Änderung, die die Vision wörtlich einlöst.

### 3.2 OPTIONALE Observer-Schnittstelle (cmake-gated, bei Anwesenheit Pflicht-Export)
`tier_observe(ComdareTierObserverSnapshotV1*)` (`observable_tier.hpp:105`) · Snapshot-POD (`:40-64`) · Observer-Body (`abi_adapter.hpp:251-279`, unter `#ifdef`) · `observable_axis_count` (`:276`).

**Vertrag „eingebaut ⇒ exportiert":** `STATISTICS=ON` + mind. eine `ObservableAxis` ⇒ Modul MUSS `tier_observe` exportieren UND `capabilities()` Bit 2 setzen. cmake-Flag entscheidet Anwesenheit, Capability-Bit signalisiert sie, `kTierObserverSnapshotVersion` versioniert das Format.

---

## 4. Messmodus vs. funktional-only — zwei Build-Profile

| | **MESS** (`STATISTICS=ON`) | **REIN** (`=OFF`) |
|---|---|---|
| Exportierte Symbole | 4 Pflicht + `comdare_anatomy_capabilities` | identisch (caps-Wert unterscheidet) |
| vtables | `IAnatomyBase` + `IDrivableTier` + `IObservableTier` (+ `IMeasurableWorkload`) | `IAnatomyBase` + `IDrivableTier` (kein `IObservableTier`) |
| `tier_observe` | implementiert (search+alloc) | **nicht vorhanden** (Code weg) |
| `capabilities()` Bit 1/2 | gesetzt | gelöscht |
| Host-Erkennung | `caps & OBSERVER_BIT` (Mechanik 2) → Fallback `dynamic_cast`/`observable_axis_count` | alle → „kein Observer" |
| Host-Verhalten | Pfad B: Füllstands-Trace + korrelierte Snapshots | nur Funktional-Tests + **Verbose-Op-Trace** (§5 C1, NEU) |

---

## 5. Konsequenz für I1 — revidierte Anfass-Liste

**A. KEINE Änderung nötig (Vision bereits erfüllt) — nur dokumentieren:** Hot-Loop-Disziplin (`tier_observe_trace_abi.hpp:82,89-135`, kein Cast) · Cast-once (`search_algorithm_dock.hpp:42`) · Op+Param+Observer-Vereinigung (`observable_tier.hpp:80-106`). **Es fällt JEDER Punkt weg, der „dynamic_cast aus dem Hot-Path entfernen" forderte — er ist nicht drin.**

**B. Strukturelle Änderungen zur wörtlichen Vision (reiten auf dem ohnehin geplanten ABI-Major-Bump mit):**
- **B1 (Disjunktheit):** `IObservableTier` → `IDrivableTier` (Ops, Pflicht) + `IObservableTier` (nur `tier_observe`). Dateien: NEU `anatomy/drivable_tier.hpp`; `observable_tier.hpp:80-106` (Ops raus, von `IDrivableTier` erben); `abi_adapter.hpp:76-77,223-249`; `search_algorithm_dock.hpp:42`; `tier_observe_trace_abi.hpp:82`; `anatomy_module_abi_v1_decl.hpp:18-19`.
- **B2 (Capability-Bit):** `comdare_anatomy_capabilities()` 5. Symbol. Dateien: `anatomy_module_abi_v1_decl.hpp:58-76`; `anatomy_module_abi_v1.hpp` (Makro/Default-Impl je `#ifdef`); `anatomy_module_loader.{cpp:64-67,122-135;hpp}` (Pfn-Typedef + Handle-Feld `uint64 capabilities`).
- **(I1-Kern, unverändert):** EIN autoritativer Mess-POD `…SnapshotV2` (volle Spalten + HW-Counter, feste uint64-Slots je Achse) + `kTierObserverSnapshotVersion`→2 + `COMDARE_ANATOMY_ABI_MAJOR`→2 → alle DLLs neu. B1+B2 reiten auf DIESEM einen Major-Bump mit.

**C. Funktionale Lücke (eigene User-Anforderung „ohne Messmodus"):**
- **C1 Verbose-Op-Trace:** per-Op-Senke (Binary/API/Methode/Reihenfolge/Zeitpunkt). NEU optionaler `ITierTraceSink`-Callback **[ANNAHME: host-seitig, Header-only]**, den `drive_*` pro Op aufruft — **nur** wenn Verbose-Flag gesetzt (compile-time `if constexpr`, nie im Mess-Heißpfad). Datei: NEU `builder/anatomy_commands/tier_functional_trace.hpp`; Aufrufer `apps/*/main.cpp`.

---

## 6. Offene Entscheidungen + Risiken (ehrlich)

1. **B1 (Interface-Split) ist ein vtable-Eingriff** an `IObservableTier` → bricht gebaute DLLs. **Mitigation:** auf dem ohnehin geplanten `COMDARE_ANATOMY_ABI_MAJOR 1→2` mitreiten (eine Major-Charge erledigt POD-Unifikation + Split + Capability-Bit). Pragmatisch erfüllt der Ist-Stand die Vision **funktional** schon; B1 ist „Reinheit", kein Bug-Fix → **Entscheidung: Split mit-machen (Vision wörtlich) oder funktional belassen?**
2. **`dynamic_cast` über `.dll` + Mehrfach-Vererbung** kann bei MSVC↔libstdc++ `nullptr` liefern obwohl Interface da. **Mechanik 2 (Capability-Bit) + Mechanik 3 (fnptr-Vtable) sind die RTTI-freien Antworten.** Relevanz steigt mit Cross-Compiler-Builds (Plattform-Fingerprint/ZIH).
3. **AxesList-Deskriptor nach außen?** Existiert nicht in der ABI. Vision „compile-time" ist IN der DLL erfüllt. **Empfehlung:** optionales Diagnose-Symbol `comdare_anatomy_axes_descriptor() → const uint8_t[17]` (rein deskriptiv, NICHT Hot-Path-Dispatch).
4. **`init()`-Methode:** keine solche Signatur; `observe_all` = `tier_observe`-Body, fix beim Composition-Build gewählt (`if constexpr`). **[ANNAHME]:** „EIN einkompilierter Observer" bindend → `init()` bereits durch Compile-Time-Aufbau erfüllt. Falls runtime-wählbare Observer-Teilmenge gewünscht → Rückfrage-Kandidat (widerspricht „ein einkompilierter Observer").
5. **Verbose-Op-Trace (C1)** ist `O(ops)` → nur im funktional-only-Profil aktiv (compile-time `if constexpr`), nie im Latenz-Mess-Profil.
6. **Snapshot-Erweiterung = neue Version, nicht Layout-Mutation.** Weitere Achsen ⇒ `V2`-POD, `kTierObserverSnapshotVersion`-Bump; Host führt Format-Version im Handshake mit.

**Kernaussage:** Die zwei meistgefürchteten Vision-Punkte — „kein per-Op-`dynamic_cast`" + „Op+Param+Observer in einer Schnittstelle" — sind im Ist-Code **bereits realisiert** (verifiziert). Offen bleiben nur Reinheits-/Robustheits-Verbesserungen: Interface-Split (B1, vtable-Risiko → auf Major-Bump mitreiten), Capability-Bit statt `nullptr`-Raten (B2, additiv-sicher), Verbose-Op-Trace fürs funktional-only-Profil (C1).
