# HANDOUT → Implementierungsagent: I/O-Achse-Stärkung (ESA-2002-Befund)

**Von:** Text-/Thesis-Agent  ·  **Datum:** 2026-06-17  ·  **Status:** offen (Code-seitig)
**Anlass:** Workflow-Sichtung des ESA-2002-Tagungsbands (LNCS 2461). Zwei I/O-/External-Memory-Fundamente
sollen die bisher dünn besetzte **I/O-Dispatch-Achse (Thesis-T14)** als *Erweiterung* stärken.
**Thesis-Gegenstück:** Task AP-B3 (zurückgestellt, zieht nach sobald Code steht).

---

## 0. Auftrag in einem Satz
Integriere die beiden Paper als **Achsenalgorithmen der I/O-Dispatch-Achse** —
**bestenfalls Originalcode + Originalcompiler** (`experiment_compiler()`, `is_original_module()=true`),
**sonst exakte C++23-Nachbildung** (`is_original_module()=false`, dokumentiert) — nach dem etablierten
Paper-Original-Code-/`legacy_reimpl`-Muster. (Genau diese „Original-oder-exakte-Nachbildung"-Wahl ist die
ausdrückliche User-Direktive.)

---

## 1. Die zwei Quellen (DOIs an SpringerLink verifiziert)

| Paper | Autoren | Venue | DOI | Transferierbares Organ |
|---|---|---|---|---|
| Implementing I/O-Efficient Data Structures Using **TPIE** | Arge, Procopiuc, Vitter | ESA 2002, LNCS 2461, S. 88–100 | `10.1007/3-540-45749-6_12` | Block-basierte Storage-Schicht; **mmap vs. read/write vs. direct-I/O** als austauschbares Storage-Organ; Stream-/AMI-Abstraktion; PDM-Modell (M/B/D) |
| **External-Memory BFS** with Sublinear I/O | Mehlhorn, Meyer | ESA 2002, LNCS 2461, S. 723–735 | `10.1007/3-540-45749-6_63` | I/O-Modell (M/B/D + `sort()`); Technik „**zufällige → sequentielle** Zugriffe via Clustering + Hot-Pool" |

---

## 2. Pro Quelle: Original vs. Nachbildung

### 2.1 TPIE (Arge et al.) — **Originalcode bevorzugt**
- Original-C++-Bibliothek existiert (Duke/Aarhus TPIE, Open Source) → Kandidat für **Originalcode-Adapter**
  hinter dem Engine-ABI: `COMDARE_HAVE_TPIE`-`#if defined`-Block + sicherer Fallback;
  `experiment_compiler()` liefert den Build-Identifier; `is_original_module()=true`.
- **LIZENZ PRÜFEN** (TPIE ist vermutlich LGPL — bitte verifizieren und in `NOTICE` dokumentieren;
  bei Lizenz-Inkompatibilität mit der cache-engine → stattdessen Nachbildung wie 2.2).
- Organ-Scope: NUR die **I/O-Schicht** (Block-Stream + mmap/read-write/direct-Dispatch) übernehmen —
  NICHT die TPIE-Geometrie-Datenstrukturen (Bkd-Tree etc.).

### 2.2 Mehlhorn/Meyer EM-BFS — **exakte Nachbildung**
- Ist ein **Graph-Algorithmus** (BFS), KEIN Such-Organ → den BFS NICHT übernehmen. Übertragbar ist die
  **Technik**: (a) das Standard-I/O-Modell (M/B/D, `sort()`) als Kosten-/Mess-Referenz; (b) das Umschreiben
  **zufälliger Pointer-Zugriffe in sequentielle Block-Scans** via Clustering + Hot-Pool — als
  I/O-Dispatch-/Prefetch-Sub-Strategie.
- → **Exakte C++23-Re-Impl**, `is_original_module()=false` (Technik-/Pseudocode-Fallback, Doku-Pflicht);
  consteval-SHA256 nur falls echte Original-Function-Bodies vorliegen (hier: nein → hart `false`).

---

## 3. Pflicht-Konventionen (Habich-Compliance)
- Ablage: `legacy_reimpl/<Pxx>-<name>/` (prt-art) ODER externer Adapter in cache-engine `ext/`
  (wie P15/P20). Paper-Original-Code-Pattern: `legacy_code/paper_<id>_<name>/`, **statisches Linken statt
  Body-Copy**, on-demand Compiler-Cache.
- Pflicht-API jeder Wrapper-Klasse: `experiment_compiler()`, `is_original_module()`, `get_compiler()`
  + AxisBase-Defaults (Wurzel `topics/axis_base.hpp`).
- Adapter: `COMDARE_HAVE_<X>`-Guard + typisierter Fallback (Build bleibt eigenständig lauffähig).
- SHA256-Validierung (consteval) pro Original-Function-Body → `is_original_<fn>()`; modulweit via `mp_all_of`.
- Compile-Time-only: keine Runtime-Property-Auswertung im Hot-Path.

---

## 4. Achsen-Mapping-Caveat (WICHTIG — sonst Fehlzuordnung)
- **Thesis-T-Nummern ≠ Code-`axis_NN`!** Thesis „T14 io_dispatch" ist **NICHT** Code-`axis_14`
  (= `value_handle`). Bitte die korrekte Code-Achse für I/O-Dispatch **identifizieren bzw. anlegen** und
  das Mapping in **Doc 18** (`docs/architecture/18_achsen_algorithmus_paper_code_map.md`) eintragen.
- Diese Paper gehören **nicht** zum P01–P33-Katalog (= die 33 Such-Paper). Vergib neue IDs
  (z. B. P34 = TPIE, P35 = EM-BFS) ODER eine I/O-Erweiterungs-Namespace-Konvention — deine Entscheidung;
  konsistent in Doc 18 + `NOTICE` + ggf. Profil-XML halten.

---

## 5. Workload-Affinität
- I/O-/Bereichs-Scan-lastig → Lastprofil **LP08 (range-scan-heavy) / YCSB_E**.
  Falls ein Profil-XML angelegt wird: `<expected_workload>` entsprechend taggen.

---

## 6. Querverweise
- ESA-2002-Gesamtübersicht (Text-Agent): `…/Forschungsarbeiten/ESA-2002-LNCS2461-Uebersicht.md`
- Bereits in die Thesis eingebundene ESA-2002-**Theorie**-Zitate (Commit `7e979d9`):
  `bender2002treelayout`, `bender2002scanning`, `bender2002orderlist`, `manzini2002suffixarray`.
- Muster/Doku: `prt_art/legacy_reimpl/README.md`, Doc 14 (Achsen-Goldstandard), Doc 18 (Paper-Code-Map).

---

## 7. Abnahme-Kriterien (Definition of Done, Code-seitig)
1. I/O-Dispatch-Achse im Code identifiziert/angelegt + Mapping in Doc 18.
2. TPIE: Originalcode-Adapter ODER (bei Lizenz-/Build-Hindernis) exakte Nachbildung — mit korrektem
   `is_original_module()` + Lizenz in `NOTICE`.
3. EM-BFS-Technik (random→sequential) als Nachbildung, `is_original_module()=false`, dokumentiert.
4. Beide hinter dem Engine-ABI, mit sicherem Fallback, Build eigenständig lauffähig.
5. Rückmeldung an Text-Agent (für AP-B3-Nachzug: finale Code-Achsen-IDs + ob original/nachgebildet).
