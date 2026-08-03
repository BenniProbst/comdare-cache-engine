# DESIGN-DOSSIER A8/F2 — Benchmarking-Schnitt (Micro = Achsen-Interface, 3 Wallclock-Ebenen) — NEUFASSUNG 03.08.2026

Datum: 03.08.2026 (vormittags) · Design-Agent: Fable 5 (Design-Nachtrag-Workflow, read-only) · Status: DESIGN, KEINE Code-Aenderung
Key: A8-F2-BENCHMARKING-SCHNITT-DESIGN-NEUFASSUNG-20260803 (ersetzt A8-F2-BENCHMARKING-SCHNITT-DESIGN-20260802)
Paket: Katalog A8 (KATALOG:24; Bauplan:43 "Pflicht vor Trigger"; ABGLEICH 03.08.:19 = OFFEN)

BASIS + ANLASS DER NEUFASSUNG:
- Erstfassung 02.08. existiert NICHT als Repo-Datei — nur im Backup
  `super docs/sessions/backups/20260802-welle3-parallel-workflows/00-welle3-result.json`
  (result.designs.designs[1], key A8-F2-BENCHMARKING-SCHNITT-DESIGN-20260802, 22044 Zeichen).
- Sammel-Review Welle 3: GESAMT-VERDIKT NACHBESSERN; A8 selbst = TRAGFAEHIG MIT AUFLAGE (befunde[4]),
  Auflage = beidseitige A8<->E-24-Sequenz-Kante (befunde[3], SCHWER) — in dieser Neufassung eingearbeitet (Abschn. 4.3, 5, 7 EV-5).
- Ist-Stand-Re-Verifikation am ce-Repo NACH Welle-2d-Landung: development `0d7a0d92` (Merge w2d-Transport-Bestandslog;
  HEAD beim Verifizieren `1cb1efbf` = 0d7a0d92 + reine cppcheck-Test-Suppression, kein Produktiv-Code).
  ALLE datei:zeile-Anker dieses Dossiers wurden am 03.08. LIVE am Working-Tree neu belegt (Zeilen-Drift durch 2d korrigiert).
- Owner-Entscheide 02./03.08. eingearbeitet (super docs/sessions/20260802-OWNER-entscheide-hybrid-tier-stempel-regression-os-unterachsen.md
  inkl. aller Nachtraege): E1 Hybrid-Tier-Stufe, E6 Fehlerklassen JETZT, R7 E-24-Pflicht, Q3-Flag-Grammatik, GO-Ketten 17:4x/18:4x,
  Parallelisierungs-Freigabe 03.08.

Basis-Pfade: SUPER = /home/comdare/Projekte/Research/probst-diplomarbeit-cache-engine · CE = SUPER/Code/external/comdare-cache-engine

---

## 0. Aenderungs-Protokoll der Neufassung (Delta zur Fassung 02.08.)

- N-1 (SCHWER, Review befunde[3]): NEUE Sequenz-Kante A8<->E-24 als BINDENDE Regel (Abschn. 4.3): alle binary-beruehrenden
  Scheiben S1/S3/S4/S5 VOR dem E-24-Anker-Vollzug (TP1-Neu-Inventur + A2-Eichung); additive Sub-Interfaces (L-74c-Muster, S3)
  nur VOR/IM E-24-Fenster — nach der E-24-Landung gilt E24-Auflage-10/G8 (KEIN ABI-/POD-Touch bis Abgabe). Die Erstfassung
  regelte nur den POD-Fall; der Sub-Interface-Zeitpunkt ist jetzt explizit geregelt.
- N-2 (Review befunde[4]): EV-5-Kollision als REAL bestaetigt uebernommen und ERWEITERT: zusaetzlich zur bauplan-internen
  Kollision (Bauplan:43 vs. :40) widerspricht auch die Wiederaufnahme-Reihenfolge 03.08.
  (PAUSE-lintrot:41: "... -> E-24 -> A3 -> A4 -> A5 -> A7-Rest -> A8/A9 -> ... -> Trigger") der Kante aus N-1 —
  A8 NACH E-24 ist mit der Ein-Neuanker-Bilanz von E-24-Option-C unvereinbar. Eskalation OF-A8-1 (Abschn. 10).
- N-3 (Ist-Delta post-2d): B-5-Bestand neu gezaehlt: **70** axes-Header mit std::vector/std::map/std::unordered_map
  (02.08. waren es 54; Zuwachs durch die zwischenzeitlichen Wellen; Gesamtbestand axes/ = 340 .hpp). Die Regel
  "Datei-Liste IMMER aus dem grep ableiten, nie handpflegen" hat sich damit selbst bewiesen; S5-Schaetzung auf 70er-Basis.
- N-4 (Ist-Delta post-2d, Zeilen-Drift): CSV-Seg-Spalten jetzt cache_engine_builder_iterator.hpp:375-377 (Header) /
  :565 (Zeile) / :571 (seg_sum) statt :557/:563; Planer-Lanes experiment_plan_director.hpp:97-106/:1741-1745/:1790-1794/
  :1882-1913; run_observable_perm perm_runner.hpp:181-225. B-6-Anker UNVERAENDERT: abi_adapter.hpp:861/:1687,
  node_value_measurement.hpp:72 — live neu belegt.
- N-5 (Befund-Schaerfung B-6): node_value_measurement.hpp:72-75 verliert nicht nur seg_ns[17], sondern auch die KOMPLETTE
  axis_stats[17][0..7]-Zeile der T17-Achse in der Knoten-Projektion; dazu NEUE stale Kommentare
  (":72 'Bau-INC-2d: 17 Achsen (isa raus)'", iterator:28 "kCompositionAxisNames[17]", iterator:375 "17 seg_..._ns-Spalten") als S6-Kandidaten.
- N-6 (Owner-E6 02.08.): Fehlerklassen-Auflage NEU (Auflage 11): alle in S1-S5 neu entstehenden/veraenderten Mess-Fehlerpfade
  laufen ueber das A15-Fehlerklassen-Framework (axis_error.hpp-Familie), abgestimmt mit A15 — analog zur E-24-Review-Auflage.
- N-7 (2d-Naht): NEUE Ist-Punkte im Mess-Transport, die S2/S3 respektieren muessen: mess_pfad_synchron_push
  (iterator:908 Definition, :1835 Aufruf), slice_window_bounds (bestandslog/planer_driven_build.hpp:88),
  per-Binary-CSV + Resume-Stempel NUR nach verifiziertem Write (iterator:1777ff). Der A8-Schnitt aendert MESS-INHALTE,
  nicht den Transport — die 2d-gehaertete Transport-/Bestandslog-Strecke ist TABU-Flaeche fuer A8-Worker.

Unveraendert aus der Erstfassung: F2-Gesetz (Abschn. 1), Soll-Schnitt inkl. T0..T17-Vertragstabelle (Abschn. 3),
Scheiben-Zuschnitt S1-S6 (Abschn. 5, jetzt mit Sequenz-Constraints), EV-1..EV-4/EV-6-Empfehlungen (Abschn. 7).

---

## 1. Das F2-Gesetz (Owner 01.08., bindend — SUPER/thesis/diplomarbeit/sessions/20260801-THESIS-FAKTEN-KANON.md:377-397)

1. Jede Achse hat ein ACHSEN-INTERFACE: Funktionen, die ALLE Algorithmen dieser Achse liefern muessen
   (Bsp. Allokator: Speicher bereitstellen). MICRO-BENCHMARK = die ACHSE ueber ihr Achsen-Interface durchmessen
   (achsen-spezifische Haupteigenschaften intern).
2. Die Achse ist NICHT eine Interface-Funktion des Suchalgorithmus. Die Gattungs-(Suchalgorithmus-)Interface-Funktionen
   VERWENDEN die Achsen-Interfaces; Achsen-Algorithmen duerfen andere Achsen-Interfaces verwenden
   ("Container werden ueberall gebraucht").
3. SCHNITT-REGEL (Architektur-Auflage): Alle Achsen-Algorithmen so schneiden, dass NUR NOCH Achsen-Interfaces statt
   generischer OS-Calls verwendet werden — inline einkompiliert per Metaprogrammierung; std::variant verboten.
4. Die CEB misst ZUSAETZLICH drei Wallclock-Ebenen: (E1) Wallclock der ACHSEN-Interfaces unter jedem Algorithmus
   (Gegenmessung zur internen Micro), (E2) Wallclock der GATTUNGS-Interfaces, (E3) Wallclock einer Test-Last aus den
   Last-Frameworks (Benutzungs-Konfiguration).
5. Ist-Implementierung ist FALSCH geschnitten (Owner-bestaetigt, Regression); der Impl-Nachzug folgt DIESEM Modell.
   Rahmen: LEDGER:3388-3391 (Schichten-Zuordnung: Wallclock = CEB-SEITE "sofern einkompiliert"; Macro+Micro = CEB UND
   Tier-Binary ueber das PRUEF-DOCK einkompiliert; Ausrichtungs-Mechanik Planer-Mess-Achse -> CEB -> Tier-Observer =
   EIN statischer Draht) + LED-64 Vereint-Default (Synthese 1.4 Nr. 33: alle 3 Pruef-Eigenschaften vereint in EINER CEB)
   + Synthese 1.4 Nr. 34 (F2 = Owner-Gesetz, Paket A8).

Owner-Rahmen-Entscheide mit A8-Wirkung (Owner-Doc 02.08. + Nachtraege):
- E1 Hybrid-Tier: std::variant-Ausnahme gilt AUSSCHLIESSLICH der Hybrid-Tier-Stufe HINTER der CEB (Dock-Array);
  in plain Tier-Binaries bleibt std::variant VERBOTEN — der gesamte A8-Scope (Mess-Treiber, Achsen-Header, Tier-DLLs)
  liegt im plain-Bereich, die Ausnahme ist fuer A8 IRRELEVANT.
- Q3-Flag-Grammatik (~17:0x): Versionen IMMER vX.Y.Z + Hardware-Flag (c/g/f/n) + optional e; CPU-only => alles endet c/ce.
  Die Bestands-Migration v1.0.0 -> v1.0.0c ist ins EINE A13-M2/M3-Neuanker-Fenster gebuendelt — A8 fasst KEINE
  Versions-Literale an (Auflage 4). Q10 (~19:5x): gerenderte Stempel-Form praefixfrei (@X.Y.Zc), Roh-Literale mit 'v'.
- E6 (Fehlerklassen JETZT mitbauen) -> Auflage 11 (N-6).
- R7: E-24 ist PFLICHT-Paket — die Sequenz-Kante aus Abschn. 4.3 ist damit nicht optional umgehbar.

---

## 2. Ist-Schnitt-Kartierung (wo misst "Micro" heute WAS) — re-verifiziert 03.08. am ce-Ist 0d7a0d92

### 2.1 Tooling-Haupt-Achse: Micro ist heute ein Etikett ohne Traeger
- CE/libs/cache_engine/include/cache_engine/measurement/measurement_tooling_registry.hpp:27-31:
  `enum class MeasurementTooling {WallClock, Macro, Micro}`; **:30 deklariert Micro als "feinkoernige PMC/Counter-
  Instrumentierung"** — widerspricht F2 (Micro = Achsen-Interface, nicht PMC). :34 kMeasurementToolingCount=3.
- Repo-weiter grep 03.08.: `MeasurementTooling::` hat AUSSERHALB der Registry **0 Consumer** in libs/apps/tools
  (live nachgezaehlt). Die Tooling-Wahl faechert nur Planer-Lanes auf (experiment_plan_director.hpp:97-106 Feld
  `tooling`/`legend`; :1741-1745 und :1790-1794 measurement_combos_of-Fan-out je <combo>; :1882-1913 Tooling-Haupt-
  Achsen-Fan-out inkl. D1-D3-Restpunkt-Kommentaren) und stempelt; sie veraendert NICHT, welche Instrumentierung
  kompiliert wird. Reale Schalter: COMDARE_MEASUREMENT_ON / COMDARE_CE_ENABLE_STATISTICS / COMDARE_ENABLE_PMC.

### 2.2 Die drei real existierenden Mess-Pfade

(A) Pfad A — in-DLL, SYNTHETISCH ("Micro" im heutigen Selbstverstaendnis):
- abi_adapter.hpp:380 run_workload (4-Segment-do_batch), :483 run_workload_segmented (V1-POD),
  :583 run_workload_segmented_v2 (18 Segmente).
- Getrieben werden mess-eigene STATISCHE Kerne auf einem synthetischen Roh-Puffer lbuf: scan_field_sum, serialize_scan,
  node_find_scan, index_org_scan, io_dispatch_scan, migration_decide_scan, filter_probe_scan,
  persistence_writeback_scan (T17: abi_adapter.hpp:845-848, acc[17] an :848). Diese *_scan-Funktionen sind
  Mess-Sonder-Eintrittspunkte der Achsen-Strategien, NICHT das Achsen-Interface, das die Gattungs-Funktionen verwenden.
- Live-Anbindung: drive_segment_latencies (perm_runner.hpp:351) hat KEINEN Live-Aufrufer (nur Doc-Kommentar :17-19
  + Unit-Tests tests/unit/test_all19_segment_timer.cpp u.a. — grep 03.08. bestaetigt). Pfad A ist im Mess-Betrieb
  faktisch tot; die CSV-seg-Spalten kommen aus Pfad B.

(B) Pfad B — in-DLL, ueber die REALE Komposition (heutige seg_ns-Quelle):
- abi_adapter.hpp:1444 fill_segment_timing_v3, ausgeloest durch tier_observe (:1721, Aufruf :1729;
  fixe Q1-Sequenz Observer-READ -> Pfad-B-Timing -> per-op-Organ-Reset, Kommentar :1090).
- Je Segment MISCHFORMEN dreier Vertragsarten:
  (i) echte Achsen-Interface-Ops der realen Komposition (T0 container_algorithm_.lookup; T1 ct_organ_.resolve;
      T2 map_organ_.resolve_offset; T3 pc_organ_.compress; T8 observe_critical_section; T15/T16 put/get bzw.
      should_flush; T17 pt_organ_.observe_writeback :1664-1670, acc[17] an :1670),
  (ii) Mess-Huellen-Scans UEBER das Store-Backing der Gattung (store_observe_node_type/layout/serialization/
      value_handle/index_org/io_dispatch/migration/filter — Umweg ueber den Container, nicht das nackte Achsen-Interface),
  (iii) reiner Stats-READ statt Achsen-Op: T6 allocator = store_allocator_statistics()-Lesen —
      allocate/deallocate wird in Ebene 1 NICHT gemessen.

(C) Host-Seite (CEB) — Wallclock + PMC:
- run_observable_perm: perm_runner.hpp:181-225 — EINE steady_clock-Klammer um die ganze insert+lookup-Last
  (t0 :198, t1 :204) -> total_ns (:106; heutige "Gesamt-Wallclock").
- run_workload_perm: perm_runner.hpp:234ff — Lastprofil ueber workload_orchestrator/run_workload_profile;
  JE Gattungs-Op einzeln getimt via two_phase_measure/abi_dur_ns (tier_observe_trace_abi.hpp)
  -> op_lat p50/p99 je {insert, lookup, erase, clear, scan, rmw}.
- PMC (#156): pmc_source.hpp + linux_perf_pmc_source.hpp — Delta-Klammer um die GANZE Run-Phase
  (perm_runner.hpp:121-124 EIN pmc je Treiber-Lauf; begin/end :197/:205 bzw. :305ff). PMC ist heute eine
  Ebene-3-Klammer, KEINE Micro-/Achsen-Messung.
- Weitere Host-Treiber: host_measure_loop.hpp (Settings x Wiederholungen, nur Observer),
  node_value_measurement.hpp (in-process, Observer-Projektion in den Experiment-Baum-Knoten).
- NEU seit 2d (N-7, TABU-Flaeche fuer A8): mess_pfad_synchron_push (iterator:908/:1835) schiebt Mess-Artefakte
  synchron in den Binaer-Cache; slice_window_bounds (planer_driven_build.hpp:88) bildet die Bestandslog-Fenster;
  per-Binary-CSV + Resume-Stempel nur nach verifiziertem Write (iterator:1777ff, fail-closed).

### 2.3 Falsch-Schnitt-Befunde (nummeriert, fuer Ledger-Nachtrag A11) — alle 03.08. live re-verifiziert

- B-1 Micro ohne Traeger: Tooling-id "micro" hat keinen kompilierten Effekt; Registry-Doku definiert Micro als PMC
  (measurement_tooling_registry.hpp:30) — F2-widrig. [unveraendert]
- B-2 Pfad A = der vom Owner verworfene Schnitt: Achsen als Interface-Funktionen des Suchalgorithmus-Messtreibers
  (synthetische *_scan-Parallelwelt auf lbuf), nicht ueber ihr Achsen-Interface gemessen. [unveraendert]
- B-3 Pfad B mischt Vertragsarten (2.2-B i/ii/iii): Ebene-1-Zeiten sind je Achse NICHT einheitlich
  "Achsen-Interface unter dem realen Algorithmus"; T6 misst gar keine Achsen-Op. [unveraendert]
- B-4 Gattung verwendet Achsen teils NUR als Mess-Nebenwirkung: tier_insert (:886) treibt konstitutiv nur
  container_algorithm_ (Kommentar :902-905: "EIN Speicher, konstitutiv"); ALLE Observer-feeding Auto-Kopplungen
  (T1/T2/T7/T8/T15/T16 ...) laufen NUR unter COMDARE_MEASUREMENT_ON (K10-PMAJOR-04-Bloecke :910-985 insert,
  :995-1032 lookup, :1055-1079 clear) — im Release-Build existieren diese Achsen-Ops im Hot-Path nicht.
  "Gattungs-Funktionen VERWENDEN Achsen-Interfaces" ist im Ist nur fuer die konstitutive Store-Kette erfuellt. [Anker aktualisiert]
- B-5 Schnitt-Regel verletzt — **AKTUALISIERT 03.08.: 70 Achsen-Header** (von 340 gesamt) unter CE/libs/cache_engine/axes/
  nutzen std::vector/std::map/std::unordered_map mit Default-Allokator (= generische OS-/libc-Allokation am
  Allokator-Achsen-Interface vorbei). Zaehlung: `grep -rlE "std::(vector|map|unordered_map)" axes/ --include=*.hpp | wc -l`
  = 70 (02.08.: 54 — der Bestand WAECHST mit jeder Welle; Liste IMMER frisch aus dem grep ableiten, nie handpflegen).
- B-6 DEFEKT (CONFIRMED, stiller Messwert-Verlust T17): drei `< 17`-Kopierschleifen bei kV3AxisCount=18
  (observable_tier.hpp:50):
  * abi_adapter.hpp:861 (Pfad A): kopiert nur acc[0..16] nach out->seg_ns; acc[17] (gemessen :848) faellt weg,
    fehlt in total (:867) und fliesst still in seg_framework_ns (:871).
  * abi_adapter.hpp:1687 (Pfad B): identisches Muster; acc[17] (gemessen :1670) fehlt in total_ns (:1692),
    Zeit fliesst still in seg_framework_ns (:1699).
  * node_value_measurement.hpp:72-75 (`t < 17`, stale Kommentar "Bau-INC-2d: 17 Achsen (isa raus)"):
    verliert in der Knoten-Projektion seg_ns[17] UND die komplette axis_stats[17][0..7]-Zeile der T17-Achse (N-5).
  Die CSV schreibt aber 18 seg-Spalten (iterator:375-377 Header und :565 Zeile iterieren
  kCompositionAxisNames.size()==18; axis_path_serialization.hpp:40-43 traegt persistence_target als 18. Namen)
  -> Spalte seg persistence_target ist immer 0 (nicht "n/a") = stiller Messwert-Verlust, Verstoss gegen die
  Ehrlichkeits-Doktrin. Review befunde[4]: am Code CONFIRMED. Laufzeit-Beleg (CSV literal) = Worker-Pflicht in S1.
  NEBENBEFUNDE (stale Kommentare, S6-Kandidaten): iterator:28 "kCompositionAxisNames[17]",
  iterator:375 "17 seg_<axis>_ns-Spalten", node_value_measurement.hpp:72 "17 Achsen"; dazu abi_adapter.hpp:47
  stale Zeilen-Verweise (fill_segment_timing_v3 "~:1136", real :1444).
- B-7 Ebenen unbenannt/unvollstaendig: E2 (op_lat) und E3 (total_ns, PMC-Klammer) existieren mechanisch, sind aber
  nicht als F2-Ebenen geschnitten/etikettiert; E1 als CEB-GEGENMESSUNG zur internen Micro fehlt konzeptionell
  (seg_ns ist DLL-intern und misst B-3-Mischvertraege). [unveraendert]

---

## 3. Soll-Schnitt nach F2 (Interface-Vertraege) — unveraendert zur Erstfassung, Anker aktualisiert

### 3.1 Achsen-Interface = Concept-Kette (bestehende Heimat, kein Neubau)
Das Achsen-Interface ist die Funktionsmenge des jeweiligen Achsen-Concepts (Drei-Stufen-Kette Topic-Concept ->
Achsen-Concept -> Permutations-Concept + CRTP-Basis; topics/axis.hpp, topics/organ_axis.hpp, topics/<topic>/concepts/).
SOLL: je Achse T0..T17 wird der VERTRAGSKERN als Single-Source-Tabelle festgeschrieben (Doku-Artefakt +
static_assert-Haertung im Achsen-Concept), abgeleitet aus dem Ist:

| Achse | Achsen-Interface-Kern (Ebene-1-Messvertrag) |
|---|---|
| T0 search_algo | insert/lookup/erase/clear (std::map-aequivalent) |
| T1 cache_traversal | register_entry/resolve |
| T2 mapping | register_slot/resolve_offset |
| T3 path_compression | compress (+ prefix-Ops) |
| T4 node_type | Slot-/Node-Find-Zugriff (node-find) |
| T5 memory_layout | Feld-/Stride-Zugriff (record-scan ueber das reale Backing) |
| T6 allocator | allocate/deallocate (REALE Ops, kein Stats-READ) |
| T7 prefetch | prefetch-Hint je Descent (observe_prefetch_descent-Vertragskern) |
| T8 concurrency | acquire/release |
| T9 serialization | encode/decode ueber reale Records |
| T10 value_handle | access/deref |
| T11 index_organization | Index-Ordnung-Scan/Lookup |
| T12 io_dispatch | dispatch |
| T13 migration_policy | decide (+ migrate_step via IMigratableTier) |
| T14 filter | probe (+ insert) |
| T15 queuing_q1 | put/get |
| T16 queuing_q2 | should_flush/on_flush_complete |
| T17 persistence_target | stage/writeback |

Regeln: (a) None-/0-Overhead-Strategien = deklarierte Vergleichs-Nullpunkte (ehrlich, kein n/a-Fake — heutiges Muster
bleibt, vgl. abi_adapter.hpp:1661-1663 T17-Ehrlichkeits-Kommentar); (b) je Slot AUSSCHLIESSLICH Achsen-Interface-
Aufrufe der REALEN Komposition (kein synthetischer Parallel-Puffer, kein Store-Umweg, kein Stats-READ als Zeit-Surrogat).

### 3.2 Schichtenmodell SOLL (konsistent mit LEDGER:3388-3391 Schichten-Zuordnung)
- MICRO (im Tier-Binary, Pruef-Dock einkompiliert): die Achse ueber ihr Achsen-Interface durchmessen; intern deren
  achsen-spezifische Haupteigenschaften (Observer-Zaehler axis_stats + achsen-eigene Kenngroessen). Traeger bleibt
  der EINE konsolidierte Snapshot (I1, ComdareTierObserverSnapshot, sizeof 1344).
- E1 CEB-Wallclock der ACHSEN-Interfaces unter jedem Algorithmus: Gegenmessung zur internen Micro. Realisierung:
  die CEB ORCHESTRIERT, die Zeitnahme ist im Tier einkompiliert (LEDGER:3389-3391: Wallclock CEB-Seite "sofern
  einkompiliert"; Macro/Micro beidseitig ueber das Pruef-Dock) und reist ueber die bestehenden seg_ns[18]-Slots —
  deren Semantik wird auf den 3.1-Vertrag UMGEWIDMET (Werte-, nicht Layout-Aenderung). [E1-Lesart = Owner-OK-pflichtig, OF-A8-2]
- E2 CEB-Wallclock der GATTUNGS-Interfaces: heutiges op_lat (je tier_insert/lookup/erase/clear/scan/rmw, two_phase)
  wird explizit Ebene 2.
- E3 CEB-Wallclock der Test-Last (Last-Frameworks/Benutzungs-Konfiguration): aeussere Klammer um den Profil-Lauf
  (heutiges total_ns-Analog); die PMC-Klammer bleibt hier verankert (EV-6).
- Konsistenz-Invariante (P-MD3-Prinzip fortgefuehrt, vgl. abi_adapter.hpp:1694-1696): Summe(E1-konstitutiv) <= E2 <= E3;
  jeder Rest EXPLIZIT benannt (framework/orchestration), Coverage gegen den je-Ebene kommensurablen Nenner.

### 3.3 Gattungs-Vertrag ("Suchalgorithmus-Interfaces VERWENDEN Achsen-Interfaces")
- Je (Gattungs-Funktion x Achse) eine deklarierte Rolle: KONSTITUTIV (die Funktion ruft das Achsen-Interface im
  Hot-Path — heute die Store-Kette search/node/layout/alloc) oder BEOBACHTEND (Mess-Organ, nur unter Messung-AN).
  SOLL: die Konstitutiv-Matrix wird hergestellt und dokumentiert; wo eine Achse im Hot-Path semantisch nichts
  beitraegt, bleibt sie ehrlich beobachtend mit eigenem E1-Treiber (Vergleichs-Nullpunkt) — F2 erlaubt das
  ("Achse ist NICHT eine Interface-Funktion des Suchalgorithmus"), verbietet aber, den Mess-Schnitt daran auszurichten.
- Achsen duerfen andere Achsen-Interfaces verwenden (Container ueberall) — Abhaengigkeitsrichtung dokumentieren
  (keine Zyklen; Allokator ist unterste Versorger-Achse).

### 3.4 Schnitt-Regel (Architektur-Auflage)
- In Achsen-Algorithmen keine generischen OS-Calls: Speicher NUR ueber das Allokator-Achsen-Interface
  (70-Dateien-Bestand B-5 abbauen, Stand 03.08.); Sync-Primitive nur in der Concurrency-Achse; I/O nur in
  io_dispatch/persistence_target. Inline einkompiliert per Metaprogrammierung (CT-Dispatch, CRTP/Concept-Muster);
  std::variant verboten (einzige Ausnahme = Hybrid-Tier-Stufe HINTER der CEB, Owner-E1 — von A8 unberuehrt, plain bleibt Tabu).
- ext/-vendored Paper-Originalcode bleibt faithful; die Uebersetzung leistet der Adapter (EV-4 / OF-A8-3, Owner bestaetigen).

### 3.5 Tooling-Neuzuordnung (byte-stabil)
WallClock = die 3 CEB-Ebenen · Macro = Gattungs-/Ende-zu-Ende (Observer + E2/E3) · Micro = Achsen-Interface intern.
Die ids/Stempel-Token "wallclock"/"macro"/"micro" bleiben BYTE-STABIL (Stempel/golden/Legenden unberuehrt); nur
Doku/Semantik-Kommentare ziehen nach (deprecaten, nie loeschen). Versions-Literale gehoeren der Q3-Flag-Grammatik-
Migration im A13-M2/M3-Fenster — A8 fasst sie NICHT an.

---

## 4. Byte-/golden-Wirkungs-Analyse + SEQUENZ-KANTEN

### 4.1 GOLDEN-NEUTRAL (kein Fenster noetig)
- Aenderungen an Mess-TREIBERN und Host-Code: abi_adapter-Messpfade, perm_runner, workload_orchestrator,
  host_measure_loop, PMC-Verdrahtung — beruehren weder binary_id/kCompositionAxisNames/serialize-Pfad noch
  Codegen-Bytes noch POD-Layouts. golden-320 (Byte-Wache) und CRC-Anker 0x56F1B721C72DC10E bleiben unberuehrt.
- seg_ns-SEMANTIK-Umwidmung (3.2): reine WERTE-Aenderung im selben Layout (sizeof 1344, static_assert
  observable_tier.hpp:160-162 unangetastet).
- Neue ABI-Flaechen NUR als additive Sub-Interfaces (L-74c-Muster wie IMeasurableWorkloadV2/V3, IMigratableTier:
  eigenstaendig, dynamic_cast, Alt-DLL-Degrade) — KEIN ABI-Major-Bump. ZEITFENSTER-Bindung s. 4.3.
- CSV: Wire-Format 169 Felder (perm_runner.hpp:82-88; Leser akzeptiert Alt-Laenge 160, Q-9) nur ADDITIV hinten
  erweitern; Alt-Mess-CSV = Archiv, nie loeschen. (Kein Widerspruch zum A9-173er-wide-Schema — zwei Formate.)

### 4.2 FENSTER/BEACHTUNG (kein golden-Bruch, aber koordinationspflichtig)
- (a) JEDE Snapshot-POD-Erweiterung (z.B. per-Funktions-E1-Zeiten) = sizeof-Bruch + ABI-Major -> NICHT in A8;
  Bedarf in den EINEN E-24-ABI-Schritt falten (nie eigenes A8-Fenster).
- (b) Mess-WERTE aendern sich flaechig (E1-Neuvertrag, T17-Fix, S5-Scrub) -> dokumentierter Archiv-Schnitt der
  Alt-CSV (Struktur-Modernisierungs-Doktrin), CSV-Spalten-NAMEN stabil.
- (c) Tooling-/Achsen-Versions-Literale NICHT anfassen; Flag-Grammatik-Migration v1.0.0 -> v1.0.0c laeuft separat
  im A13-M2/M3-Fenster (Owner-Q3; post-2d ist der Flag-Parser gelandet, die Bestands-Migration kommt mit M3).
- (d) Tests zementieren die alte Ordnung (3x real bei O-8): Fixture-Strings (tests/unit/test_striktheit_scan_kernel_purity.cpp,
  tests/unit/test_all19_segment_timer.cpp — beide existieren live) BEWUSST nachziehen, nie stumm gruen buegeln.
- (e) 2d-Transport-Naht (N-7): mess_pfad_synchron_push/slice_window_bounds/Resume-Stempel-fail-closed sind
  frisch gehaertete Flaechen — A8 aendert dort NICHTS; S2-CSV-Erweiterungen gehen NUR ueber die bestehenden
  format_csv_row/Header-Generatoren (iterator:375ff/:565ff), nie am Transport vorbei.

### 4.3 SEQUENZ-KANTE A8 <-> E-24 (NEU, bindend — Review befunde[3], SCHWER, beidseitig)

FAKTENLAGE: abi_adapter.hpp (2262 Zeilen) + die axes/-Header sind in JEDES Tier-Binary einkompiliert. Jede A8-Scheibe,
die sie beruehrt (S1, S3, S4, S5), dreht die SHA512-Quell-Fingerprints -> das F7-Skip-Gate baut neu. E-24 (Option C)
reklamiert "genau EIN Neuanker-Ereignis" und vollzieht mit TP1-Neu-Inventur + A2-Eichung den Anker; nach der
E-24-Landung gilt E24-Auflage-10/G8: KEIN ABI-/POD-Touch bis zur Abgabe.

BINDENDE REGELN (in dieser Reihenfolge durchzusetzen):
1. **Alle binary-beruehrenden A8-Scheiben (S1, S3, S4, S5) landen VOR dem E-24-Anker-Vollzug** (d.h. bevor
   TP1-Neu-Inventur + A2-Eichung den Fingerprint-Bestand einfrieren). Landet eine solche Scheibe danach, entsteht
   ein ZWEITES faktisches Neuanker-/Invalidierungs-Ereignis — die Option-C-Bilanz ("genau 1") ist dann falsch;
   nach dem Voll-Bau-4-Trigger hiesse es Neubau des kompletten 12 x 2^17-Bestands (~1,57 Mio Binaries) —
   exakt der von Owner-E3 ausgeschlossene Zustand.
2. **S3-Sub-Interfaces (additive L-74c-Flaechen) entstehen VOR oder IM E-24-Fenster, NIE danach** — nach der
   E-24-Landung verbietet Auflage-10/G8 jeden ABI-Touch. Entsteht in S3/S4 wider Erwarten POD-Bedarf:
   STOPP + in den EINEN E-24-ABI-Schritt falten (4.2-a).
3. **Host-only-/Doku-Scheiben (S2, S6) sind von der Kante FREI** (kein Tier-Binary-Beitrag, kein Fingerprint-Shift)
   und duerfen auch nach E-24 landen.
4. Die V3-Manager-Vorlage (E-24-Fenster-Bauplan) MUSS diese Kante ausweisen — ohne sie ist V3
   entscheidungs-unvollstaendig (Review-Wortlaut). Spiegel-Auflage an das E-24-Dossier: A8-Slot im Fenster-Bauplan.

KOLLISIONS-LAGE (Eskalation OF-A8-1, unveraendert OFFEN):
- Bauplan:43 "Pflicht vor Trigger: A8" vs. Bauplan:40 Zeitachse "Do 07.: Benchmarking-Schnitt" PARALLEL zum Trigger
  (bauplan-interner Widerspruch, Review-bestaetigt REAL).
- ZUSAETZLICH (N-2): PAUSE-lintrot:41 (Wiederaufnahme-Reihenfolge 03.08.) ordnet "... -> E-24 -> ... -> A8/A9 -> ...
  -> Trigger" — A8 NACH E-24 widerspricht Regel 1. Zeitbudget-/Scope-Entscheid Owner/Manager erforderlich.

---

## 5. Migrationsplan in landbaren Scheiben (mit Gates + Sequenz-Constraints)

Gate-Grundkadenz JEDER Scheibe: lokaler Voll-Bau + Test-Gate gruen (Dual-Weg CI UND lokal, offizielle CMake/ctest-
Targets; Gate-Zahl beim Einfrieren live ablesen — Stand 03.08.: 331) · golden-320-Byte-Wache gruen · J-Kette N=316
(NUR mit V32_ENABLE=ON) · kein ABI-Major, sizeof-Asserts unveraendert · literaler Output je Verifikation ·
Working-Tree-Freeze nach Paketmeldung (diff-stat literal).

- **S1 (SOFORT, klein, defekt-getrieben; binary-beruehrend -> Kante 4.3-1):** T17-Verlust-Fix — die drei
  `< 17`-Schleifen auf kV3AxisCount heben (abi_adapter.hpp:861, :1687; node_value_measurement.hpp:72-75 inkl.
  axis_stats-Zeile) + stale "17"-Kommentare an den drei Stellen mitziehen + Regressionstest "18-Slot-Vollzaehligkeit
  gegen kV3AxisCount, nie Literal" + Test, dass Sum(seg_ns)+framework==run_total die T17-Zeit enthaelt.
  VORHER Laufzeit-Beleg des Defekts literal sichern (CSV-Spalte seg persistence_target == 0 bei aktiver Messung).
  Werte-Aenderung dokumentieren (Alt-CSV-Archiv-Notiz). GATE: Grundkadenz.
- **S2 (Ebenen-Etikettierung, host-only; Kante-frei 4.3-3):** E2/E3 explizit machen — op_lat = E2, aeussere
  Lauf-Klammer = E3 (run_observable_perm/run_workload_perm), PMC als E3-Klammer deklarieren; CSV nur ADDITIV
  (Ebenen-Summen-Spalten hinten, Reader-Toleranz-Test 169/160-Muster), Doku-Kommentare an den Messstellen auf
  F2-Vokabular. Gegenpruefung LED-64-Vereint-Default + KERN-Mess-Schema Abschn. 59/E.0 (vereinter
  {wallclock,macro,micro}-Default EINER CEB muss den A8-Schnitt tragen). GATE: Grundkadenz + CSV-Feldzahl-Toleranz-Test.
- **S3 (E1/Micro-Neuschnitt in der DLL, Kern der Regression; binary-beruehrend -> Kante 4.3-1, Sub-Interfaces 4.3-2;
  Start erst nach Owner-OK der E1-Lesart, OF-A8-2):** fill_segment_timing_v3 (:1444) wird DER EINE E1-Treiber;
  je Segment NUR 3.1-Vertragsops der REALEN Komposition (T6: echter allocate/deallocate-Churn ueber den
  Kompositions-Allokator statt Stats-READ; store_observe_*-Umwege durch direkte Achsen-Interface-Ops ersetzen, wo sie
  Gattungs-Umwege sind); Vertrag je Segment als Kopf-Kommentar + Concept-static_asserts. Pfad A (run_workload/
  segmented/v2 + drive_segment_latencies) DEPRECATEN (Kandidatenliste Abschluss-Aufraeumpass; Entfernung erst am Ende,
  Doku bleibt). GATE: Grundkadenz + Strategie-Differenzierungs-Tests je Achse (distinkte Zeiten je Strategie-Wahl,
  Nullpunkte deklariert) + Coverage-Abnahme Sum(seg)+framework==run_total.
- **S4 (Gattungs-Vertrag; binary-beruehrend -> Kante 4.3-1):** Konstitutiv-Matrix (3.3) herstellen und pruefen —
  je Gattungs-Funktion dokumentieren, welche Achsen-Interfaces sie konstitutiv verwendet; Release-Pfad-Neutralitaet
  fuer die konstitutive Kette per Wallclock-Beweis (Abschn.-61-Release-Modus) belegen; beobachtende Achsen ehrlich
  deklarieren. GATE: Grundkadenz + Release-Verhaltens-Beweis.
- **S5 (GROSS, familienweise; binary-beruehrend -> Kante 4.3-1):** OS-Call-/std-Container-Scrub der **70**
  Achsen-Header (B-5, Stand 03.08.) — je Organ-Gruppen-Scheibe (01_read_path, 02_layout, 03_placement, 04_execution,
  05_write_path_io); Speicher ueber die Allokator-Achse, inline CT, kein std::variant; Datei-Liste je Scheibe AUS DEM
  GREP abgeleitet (Bestand waechst: 54 -> 70 binnen eines Tages). Aufwands-Schaetzung je Familie VOR
  Trigger-Terminierung (ultracode je Scheibe). GATE je Familie: Grundkadenz + Konformitaets-GTest der Familie +
  Perf-Sanity (keine 0-/Phantom-Zeiten).
- **S6 (Abschluss/Doku; Kante-frei 4.3-3):** Tooling-Semantik-Nachzug (measurement_tooling_registry-Kommentare :30,
  Director-Lane-Kommentare, plan_legend), stale-Kommentar-Nachzug (B-6-Nebenbefunde), Ledger-Nachtrag
  (Befunde B-1..B-7 + Vollzug), Thesis-Anschluss (Fakten-Kanon:311: die Thesis beschreibt das SOLL — dieses Dossier
  Abschn. 3 IST das Schnittbild; Ist-Vermerk aufloesen; Uebergabe als Vorlage an den Text-Strang, nie direkt).
  GATE: Doku-only, CI gruen.

Reihenfolge/Einordnung: S1 sofort (Defekt, zugleich fruehe Erfuellung der 4.3-Kante fuer die kleinste Scheibe);
S2-S4+S6 = A8-Kern, trigger-blockierend; S5 VOR dem Trigger PFLICHT (Neubau-Argument 4.3) UND vor dem
E-24-Anker-Vollzug (Kante 4.3-1), familienweise parallelisierbar (<=8 Bau-Worker, Arbeitsweise v2.1).
Fehlerklassen (Owner-E6): in JEDER Scheibe Pflichtteil (Auflage 11).

---

## 6. Auflagen fuer Worker (bindend)

1. KEIN ABI-Major-Bump, keine POD-Layout-Aenderung; sizeof-/static_assert-Wachen (1344, observable_tier.hpp:160-162)
   unangetastet. Neue ABI-Flaechen nur als additive Sub-Interfaces nach L-74c-Muster und NUR im Zeitfenster nach
   4.3-2. POD-Bedarf -> STOPP + Eskalation (E-24-Fenster).
2. golden-Tabu: binary_id, kCompositionAxisNames, serialize_composition_path, Codegen-Ausgaben, CRC-Anker NICHT
   anfassen; golden-320 + J-Kette (V32_ENABLE=ON) je Scheibe literal belegen.
3. CSV: Spalten nur ADDITIV hinten; Wire-Laengen-Toleranz im Reader (169/160-Muster); Alt-Mess-CSV nie loeschen
   (Archiv); Fehler-Zellen "failed"/"n/a", NIE stille 0 (B-6 ist das Negativ-Beispiel).
4. Tooling-/Versions-Literale byte-stabil ("wallclock"/"macro"/"micro"); Versions-Strings gehoeren der
   Q3-Flag-Grammatik (vX.Y.Z + HW-Flag, Migration NUR im A13-M2/M3-Fenster) — A8 fasst sie NIE an.
5. CT-Doktrin: kein Runtime-Switch, kein std::variant (Hybrid-Stufen-Ausnahme irrelevant fuer A8 — plain-Scope),
   CRTP+Concept, GoF-Muster; Konzept-Haertung per static_assert statt Laufzeit-Checks.
6. Ehrlichkeit: None-Strategien = deklarierte Nullpunkte; keine erfundenen Werte; kein Erfolgs-Haken ohne literalen
   Output; jede Messwert-Semantik-Aenderung im Scheiben-Log dokumentieren.
7. Tests: zementierende Fixtures BEWUSST nachziehen (Commit-Notiz je geaendertem Fixture); neue Wachen gegen
   kV3AxisCount/kCompositionAxisNames ableiten, NIE Zahlen-Literale.
8. Dual-Weg je Scheibe (CI und lokal, offizielle Targets, keine Behelfswege); Datei-Listen (70er-Bestand) aus grep
   ableiten, nie handgepflegt.
9. Prozess: vor jeder Scheibe Ledger + dieses Dossier lesen; je Paket ultracode-Planung; Working-Tree-Freeze nach
   Paketmeldung mit literalem diff-stat; ASCII-only-Kommentare + Selbstcheck; Doku nur deprecaten, nie loeschen;
   Ein-Agent-Regel je ce-Worktree.
10. Scope-Grenzen: KEIN Beruehren von Stempel-/Lager-Logik (A13/A2), KEINE Registry-XML-Aenderung, KEIN
    Overleaf/Thesis-Haupttext (S6 nur Impl-Doku + Ledger; Thesis-Anschluss als Vorlage an den Text-Strang);
    KEIN Beruehren der 2d-Transport-/Bestandslog-Naht (mess_pfad_synchron_push, slice_window_bounds,
    Resume-Stempel-fail-closed — N-7-TABU).
11. FEHLERKLASSEN (Owner-E6, NEU): jeder in S1-S5 neu entstehende oder veraenderte Mess-Fehlerpfad erhaelt
    Fehlerklassen-IDs aus dem A15-Framework (axis_error.hpp-Familie), abgestimmt mit dem A15-Strang; keine
    nackten bool-/0-Fehlerpfade in neuen Mess-Vertraegen.
12. SEQUENZ (NEU, 4.3): binary-beruehrende Scheiben (S1/S3/S4/S5) NUR solange der E-24-Anker NICHT vollzogen ist;
    vor Scheiben-Start den E-24-Stand pruefen (ABGLEICH-/Nachfolge-Doc); im Zweifel STOPP + Manager.

---

## 7. Entscheidungs-Vorlagen EV-1..EV-6 (Status Neufassung; E-21-verbose; Prozess-Stand: bisher UNVERSENDET — Task #22)

- **EV-1 (S1-GO):** T17-Kopierschleifen-Defekt (abi_adapter.hpp:861/:1687, node_value_measurement.hpp:72-75) sofort
  als eigene Mini-Scheibe fixen — golden-neutral, Werte-Aenderung dokumentiert, Laufzeit-Beleg vorher.
  EMPFEHLUNG: GO sofort, unabhaengig vom Rest. STATUS: als Default freigegeben durch Owner-GO-Kette
  (17:4x "volles Go fuer ALLE geplante offene Arbeit wie vorgegeben" + 18:4x pausenlos; Einspruch jederzeit).
- **EV-2 (E1-Granularitaet):** E1 je Achse AGGREGIERT ueber die bestehenden seg_ns[18]-Slots (kein POD-Bruch, kein
  ABI-Major) [EMPFOHLEN, Default] — versus je Achsen-Interface-FUNKTION (braeuchte POD/Sub-Interface, E-24-Kopplung;
  als Post-Abgabe-Erweiterung vormerkbar). STATUS: Default laeuft, Abweichung wuerde neu vorgelegt.
- **EV-3 (Pfad A):** synthetischen Pfad A (run_workload/run_workload_segmented/_v2 + drive_segment_latencies)
  DEPRECATEN und E1 ausschliesslich ueber die reale Komposition (Pfad B, neu geschnitten) messen [EMPFOHLEN, Default]
  — versus Pfad A ebenfalls umschneiden (doppelte Treiber-Pflege ohne F2-Mehrwert). STATUS: Default laeuft.
- **EV-4 (Schnitt-Regel-Scope, Owner-pflichtig):** ext/-vendored Paper-Originalcode bleibt faithful und ist vom
  OS-Call-Scrub AUSGENOMMEN; die Uebersetzung leistet der Adapter [EMPFOHLEN — deckt vendor->faithful->self-Doktrin].
  Owner-Bestaetigung noetig, da F2 "alle bekannten Achsen-Algorithmen" sagt. -> OF-A8-3.
- **EV-5 (Sequenzierung, ESKALATION — ERWEITERT):** A8-Scheiben, die abi_adapter/axes-Header beruehren
  (S1, S3, S4, S5), MUESSEN (a) VOR dem Voll-Bau-4-Trigger UND (b) VOR dem E-24-Anker-Vollzug landen;
  S3-Sub-Interfaces VOR/IM E-24-Fenster (Abschn. 4.3). Kollisionen: Bauplan:40 vs. :43 (bauplan-intern) +
  PAUSE-lintrot:41 (A8 nach E-24 einsortiert). Zeitbudget-/Scope-Entscheid Owner/Manager. -> OF-A8-1.
- **EV-6 (PMC-Platz):** PMC bleibt in A8 die Ebene-3-Klammer (Test-Last); per-Achsen-PMC = Post-Abgabe-Erweiterung,
  NICHT in A8 [EMPFOHLEN, Default]. Registry-Kommentar "Micro = PMC" (:30) wird in S6 per Deprecation-Vermerk
  korrigiert. STATUS: Default laeuft.

---

## 8. Offene Punkte (status-gefuehrt)

1. B-6-Laufzeit-Beleg durch den Worker VOR dem S1-Fix: CSV-Spalte seg persistence_target == 0 bei aktiver Messung,
   literaler Output (Design-Befund ist code-CONFIRMED, laufzeit-Beleg steht aus). [OFFEN, S1-Pflichtteil]
2. Owner-OK der E1-Lesart ("CEB misst" -> CEB orchestriert, Zeitnahme im Tier via Pruef-Dock; gedeckt durch
   LEDGER:3388-3391) VOR S3-Start. [OFFEN -> OF-A8-2; die GO-Ketten 17:4x/18:4x decken dokumentierte Defaults,
   die EV-Vorlagen wurden aber nie versendet — kein stilles Als-genehmigt-Behandeln der E1-Lesart]
3. EV-5-/Sequenz-Aufloesung (Bauplan-Korrektur + Wiederaufnahme-Pfad). [OFFEN -> OF-A8-1]
4. S5-Aufwandsschaetzung je Organ-Gruppen-Familie VOR Trigger-Terminierung — jetzt auf 70-Dateien-Basis
   (grep-Snapshot 03.08.; je Scheibe ultracode). [OFFEN]
5. Gegenpruefung KERN-Mess-Schema Abschn. 59 / Katalog E.0 + LED-64-Vereint-Default beim S2/S6-Schnitt
   (vereinter {wallclock,macro,micro}-Default EINER CEB). [OFFEN, S2/S6-Gate]
6. E-24-Kante: als bindende Regel FIXIERT (4.3) — RESTAUFGABE: Spiegel-Nachtrag im E-24-Dossier (A8-Slot im
   Fenster-Bauplan) + V3-Vorlage erweitern; Ueberwachung je Scheibe (Auflage 12). [TEIL-GEREGELT]
7. Thesis-Nachzug (Fakten-Kanon:311): Abschn. 3 = das SOLL-Schnittbild; Thesis-Strang (K4/C-Reihe) informieren,
   damit SOLL-Beschreibung + Ist-Vermerk konsistent nachgezogen werden (Text-Sessions gehoeren in die Thesis,
   nicht ins Impl-Repo). [OFFEN, S6-Vorlage]
8. Ledger-Nachtrag A11: Befunde B-1..B-7 (B-5=70, B-6 inkl. axis_stats-Zeilen-Verlust + stale Kommentare) + stale
   Registry-Doku "Micro = PMC/Counter" (measurement_tooling_registry.hpp:30) als Deprecation-Vermerk einreihen.
   [OFFEN, A11-Sammelpaket]
9. NEU: Dossier-Landung als Repo-Datei (Ziel-Ablage Abschn. 9) durch den Lead NACH der 2d-Landung committen
   (Design-Nachtrag-Workflow-Auftrag; bis dahin existiert A8 nur im Backup-JSON + diesem Scratchpad-Extrakt). [OFFEN]

---

## 9. Ziel-Ablage + Befund-Uebergabe

- ZIEL-PFAD im Repo: **CE/docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md**
  (Praezedenz: CE/docs/architecture/20260802-hybrid_tier_stufe_soll_design.md — SOLL-Design-Dossiers der
  cache-engine leben in ce docs/architecture/, nicht in super docs/sessions/; A8 ist ein ce-Implementierungs-Design).
  Commit durch den Lead (STRIKT read-only fuer diesen Workflow), fruehestens nach der 2d-Landungs-CI-Gruen-Wache.
- Befund-Uebergabe: B-1..B-7 (Abschn. 2.3) -> Ledger-Nachtrag A11; B-6 = CONFIRMED-Defekt mit stillem
  Messwert-Verlust (T17-Spalte immer 0 statt Messwert) und Vorrang (S1).
- Quellen-Kanon dieser Neufassung: KATALOG:24 · Bauplan:40+:43 · SYNTHESE-Lage-Dossier:66 (Nr. 34 F2) /:236 (GATE 9)
  /:259 (Abschn. 5 Nr. 10) · FAKTEN-KANON:311+:377-397 · LEDGER:3388-3391 · ABGLEICH 03.08.:19 · Owner-Doc 02.08.
  + Nachtraege · PAUSE-lintrot:41/:60-64 · Welle-3-Backup-JSON (Erstfassung + Review befunde[3]/[4]) ·
  ce-Ist 0d7a0d92 (Working-Tree 1cb1efbf, alle datei:zeile-Anker live 03.08.).

---

## 10. Owner-/Manager-Fragen (VERBOSE, E-21-Format)

### OF-A8-1 — Sequenz-/Zeitbudget-Entscheid A8 vs. E-24 vs. Trigger (ESKALATION, entscheidungs-kritisch)
KONTEXT: A8-Scheiben S1/S3/S4/S5 beruehren abi_adapter.hpp + 70 axes-Header — einkompiliert in JEDES Tier-Binary;
jede Landung dreht die SHA512-Fingerprints. E-24 (Option C) reklamiert genau EIN Neuanker-Ereignis und verbietet nach
seiner Landung jeden ABI-Touch (Auflage-10/G8). Der Bauplan fuehrt A8 als "Pflicht vor Trigger" (Bauplan:43), terminiert
den Benchmarking-Schnitt aber auf Do 07. PARALLEL zum Trigger (Bauplan:40); die Wiederaufnahme-Reihenfolge 03.08.
(PAUSE-lintrot:41) ordnet A8/A9 sogar NACH E-24 ein. Alle drei Ordnungen sind untereinander unvertraeglich.
OPTIONEN: (a) Bauplan-Korrektur: S1 sofort; S3/S4/S5 werden VOR dem E-24-Anker-Vollzug eingetaktet (E-24 rueckt
entsprechend nach hinten im Fenster; Ein-Anker-Bilanz bleibt wahr); S2/S6 frei. (b) Scope-Schnitt: vor Trigger nur
S1+S2+S6; S3/S4/S5 nach der Abgabe — der Voll-Bau-4-Bestand wird dann mit dem heutigen (Owner-bestaetigt falschen)
E1-Schnitt gemessen, nur T17-Fix und Ebenen-Etiketten sind drin. (c) Trigger-Verschiebung bis A8 komplett vor E-24
gelandet ist. EMPFEHLUNG: (a) — sie erhaelt F2-konforme Messwerte fuer den Voll-Bau-Bestand ohne zweites
Neuanker-Ereignis; (b) nur falls das Zeitbudget (a) nicht hergibt, dann als BENANNTE Abweichung mit Thesis-Vermerk
(Messwerte des Voll-Laufs = Alt-Schnitt). DEFAULT (wenn keine Antwort): (a), wobei der Manager die Eintaktung vor
E-24 verantwortet und (b) automatisch zieht, sobald die Restzeit fuer S5 nicht mehr reicht — der Uebergang wird
gemeldet, nie still vollzogen.

### OF-A8-2 — Owner-OK der E1-Lesart VOR S3-Start
KONTEXT: F2 sagt woertlich "die CEB misst" die drei Wallclock-Ebenen. Vorgeschlagene Realisierung fuer E1
(Achsen-Interface-Wallclock unter jedem Algorithmus): die CEB ORCHESTRIERT, die ZEITNAHME ist im Tier-Binary
einkompiliert (ueber das Pruef-Dock) und reist ueber die bestehenden seg_ns[18]-Slots — gedeckt durch die
Schichten-Zuordnung LEDGER:3388-3391 (Wallclock CEB-Seite "sofern einkompiliert"; Macro/Micro beidseitig via
Pruef-Dock). Alternative waere echte host-seitige Zeitnahme je Achsen-Interface-Aufruf — das braeuchte neue
ABI-Flaechen je Achsen-Op (E-24-Kopplung, Overhead im Hot-Path, Cross-DLL-Klammer-Verzerrung).
OPTIONEN: (a) Lesart bestaetigen (CEB orchestriert, Tier nimmt die Zeit; seg_ns[18]-Umwidmung ohne Layout-Bruch).
(b) Host-seitige Zeitnahme fordern (dann E-24-Fenster-Kopplung + POD/Sub-Interface-Bedarf, S3 verschiebt sich).
EMPFEHLUNG: (a) — ledger-gedeckt, golden-neutral, ohne ABI-Ereignis. DEFAULT: (a); S3 startet erst nach diesem OK
(explizite Owner-Antwort ODER Ablauf der dokumentierten Default-Frist im Vorlagen-Versand des Leads) — die
GO-Ketten vom 02.08. decken dokumentierte Defaults, aber diese Vorlage wurde noch nie versendet, darum wird sie
nicht still als genehmigt behandelt.

### OF-A8-3 — Schnitt-Regel-Scope: ext/-vendored Paper-Code ausgenommen?
KONTEXT: F2-Schnitt-Regel verlangt "alle bekannten Achsen-Algorithmen" auf Achsen-Interfaces statt generischer
OS-Calls. Die Vendoring-Doktrin (vendor -> faithful -> selfcontained) verlangt zugleich, dass ext/-Paper-Originalcode
FAITHFUL bleibt (Papertreue = Messaussage). Der S5-Scrub (70 Header, Stand 03.08.) betraefe sonst auch vendorten
Fremdcode und braeche die Papertreue.
OPTIONEN: (a) ext/-vendored Originalcode bleibt faithful und ist vom Scrub AUSGENOMMEN; die Uebersetzung auf
Achsen-Interfaces leistet der jeweilige Adapter (comdare-seitig, im Scrub-Scope). (b) Auch vendorter Code wird
umgeschnitten (Papertreue-Verlust, Doktrin-Bruch, hoher Aufwand). EMPFEHLUNG: (a). DEFAULT: (a) — deckungsgleich
mit der bestehenden ext/-Doktrin; ein Owner-Einspruch wuerde nur Option (b) aktivieren.
