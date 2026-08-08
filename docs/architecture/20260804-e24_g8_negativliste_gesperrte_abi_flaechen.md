# E-24 G8 -- NEGATIV-LISTE: gesperrte ABI-/Fingerprint-Flaechen (bis nach der Abgabe)

Stand: 2026-08-04, E-24 C11 (b). Erhoben am ce-Branch `e24-b` nach C10 + C11 (a).
Bauplan: `docs/sessions/20260803-DOSSIER-e24-fenster-bauplan.md` Paragraf 3.2-C11 + 4.2/C11.
Dossier: `docs/architecture/20260803-e24_container_gattungs_abi_dossier.md`.
ASCII-only.

---

## 0. Was G8 ist -- und was es NICHT ist

E-24 war das LETZTE ABI-Fenster vor der Abgabe (GATE 4). Mit dem Abschluss dieses Fensters gilt:
**KEIN ABI-/POD-/Fingerprint-Touch mehr bis zur Abgabe.** Der Grund ist nicht Ordnungsliebe, sondern
Arithmetik: nach dem Voll-Bau-4 haengen ~1,57 Mio Tier-Binaries an genau einem Anker. Jede spaetere
Beruehrung einer der unten gelisteten Flaechen erzeugt ein ZWEITES Neuanker-Ereignis und macht die
Ein-Anker-Bilanz falsch (Bauplan Risiko R1; Owner-E3 schliesst einen Neubau aller Binaries aus).

G8 sperrt **Flaechen, nicht Dateien**. Gesperrt ist, was die Lade-Akzeptanz, das Wire-Layout, die
binary_id oder den SHA512-Fingerprint einer Tier-Binary bewegt. Ein Kommentar in derselben Datei ist
NICHT gesperrt -- er bewegt nichts (Nachweis: die Kadenz jedes Commits faehrt golden-320 +
Registry-Roundtrip; eine Kommentar-Aenderung laesst beide byte-identisch).

**Aufhebung:** nur durch Owner-Entscheid. Ein Verschiebe-/Erweiterungs-Wunsch ist IMMER Owner-Sache,
nie eine Ermessensentscheidung im Bau (E24-DOSSIER Auflage 2).

---

## 1. Die gesperrten Flaechen

### 1.1 ABI-Identitaet (Lade-Akzeptanz)

| Flaeche | Datei | Warum gesperrt |
|---|---|---|
| ABI-Major/Minor | `include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp` (`COMDARE_ANATOMY_ABI_MAJOR/MINOR`) | Jede Drehung lehnt ALLE eingelagerten Binaries ab (Loader-Schritt 5). |
| Magic | dieselbe Datei (`COMDARE_ANATOMY_ABI_MAGIC`) | Kodiert den Major; Schloss 1 der Lade-Wache (C10 (b)). |
| CEB-Contract-Minor | dieselbe Datei (`kCebContractCodegenMinor`) | Bewegt `+ceb=` und damit den Objekt-Store-Key-Namensraum. |
| Loader-Vertrag | `builder/anatomy_module_loader/anatomy_module_loader.{hpp,cpp}` | 7-Schritt-Validierung + destroy-vor-dlclose-Ordnung. |
| Modul-Makros | `abi/anatomy_module_abi_v1.hpp`, `abi/set_module_abi_v1.hpp`, `abi/sequence_module_abi_v1.hpp`, `abi/adapter_module_abi_v1.hpp`, `abi/view_module_abi_v1.hpp` | Die vier extern-"C"-Pflicht-Symbole jeder Permutations-DLL. |

### 1.2 Wire-Layout (PODs + Sub-Interfaces)

| Flaeche | Datei |
|---|---|
| SA-Observer-POD + Antriebs-Interface | `anatomy/observable_tier.hpp`, `anatomy/idriveable_tier.hpp` |
| Der grosse ABI-Adapter | `anatomy/abi_adapter.hpp` |
| Gattungs-Wire-Formen (C6) | `anatomy/genus_observer_aggregate.hpp` |
| Genus-Antriebs-Interfaces + V1-PODs | `anatomy/set_tier.hpp`, `anatomy/sequence_tier.hpp`, `anatomy/adapter_tier.hpp`, `anatomy/view_tier.hpp` |
| Genus-ABI-Adapter | `anatomy/set_abi_adapter.hpp`, `anatomy/sequence_abi_adapter.hpp`, `anatomy/adapter_abi_adapter.hpp`, `anatomy/view_abi_adapter.hpp` |
| SA-Zusatz-Sub-Interfaces | `anatomy/resource_controllable_tier.hpp`, `anatomy/allocator_proxy_tier.hpp`, `anatomy/rollbackable_tier.hpp`, `anatomy/scannable_tier.hpp` |
| Achsen-Aggregat | `anatomy/observer_aggregate.hpp` |

**Insbesondere gesperrt:** die VERERBUNGSREIHENFOLGE der Sub-Interfaces. Eine Erweiterung ist nur als
NEUES Sub-Interface + genau ein kalter `dynamic_cast` zulaessig, nie als vtable-Anhang (Auflage 5) --
und selbst das ist nach C11 ein Owner-Entscheid.

### 1.3 Enums + Ordnungen

| Flaeche | Datei | Regel |
|---|---|---|
| `AnatomyGattung` / `AnatomyGenus` | `anatomy/anatomy_base.hpp` | KEIN Append, KEIN Reorder. `Graph = 2` bleibt unangetastet (Q5, nach Abgabe). Die C7-1-Umbenennung SearchAlgorithm -> Map war der letzte zulaessige Eingriff und bewegte NUR den Namen, nicht den Wert. |
| System-Achsen-Ordnung | `abi/system_axis_order.hpp` | Byte-unberuehrt seit 19adba05 (verifiziert); die scheduling-Abgangs-Wache (`static_assert(!is_known_system_axis("scheduling"))`) bleibt stehen. |
| Suffix-Segment-Ordnung | `profile_facade/system_version_suffix.hpp` (`kSuffixSegmentOrder`) | `+ceb=` bleibt an Position 3. |
| Fehlerklassen-NUMMERN | `include/cache_engine/measurement/axis_error.hpp` | Etiketten UND Nummern reisen in Experiment-Logs (RF-3). Additive Erweiterung am ENDE war C5; die Nummern 5/6 (`GattungsBindungFehlt`/`GattungsSlotAritaet`) aendern sich NIE -- Manager-Entscheid LEDGER:3844: CSV-Etiketten BELASSEN, nur Kommentar-Heilung. |

### 1.4 Fingerprint / Stempel

| Flaeche | Datei | Regel |
|---|---|---|
| Preimage-Ordnung + Format | `abi/anatomy_fingerprint.hpp` | `fingerprint_format=2` ist TABU; 6 Glieder; System-Glied an Position 2. |
| Sub-Achsen-Werteset-Segment | `abi/subaxis_valueset_segment.hpp` | Preimage-Glied [4]. |
| System-Zellwerte | `abi/system_cell_values.hpp` | W10-Territorium. |
| SHA512-Primitive | `src/sha512/ctsha512.hpp` | Aenderung = jeder Stempel bewegt sich. |
| Organ-/System-Stempel-Zeilen | `abi/anatomy_version_stamp.hpp`, `abi/anatomy_stamp_entries.hpp`, `abi/meta_meta_stamp_suffix.hpp` | `kOrganAxisCount == 18`. |

### 1.5 golden / binary_id

| Flaeche | Regel |
|---|---|
| `tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt` (+ die `_abi4/5/6`-Freezes) | Byte-Wache. Nicht anfassen. |
| CRC64-Anker `kNewGolden131072Crc64 = 0x56F1B721C72DC10E` (`profile_facade/source_catalog.hpp:190`) | TABU-Wert. |
| `permutation_axes.xml`, `m3v2_study.profile.xml`, `system_axis_registry.xml` | byte-stabil. |
| `binary_id`-permutierende Komposition (18 Organ-Haupt-Achsen) | Keine neue Organ-Haupt-Achse. |

### 1.6 axes/-Header

Alle 340 `.hpp` unter `libs/cache_engine/axes/` sind in Tier-Binaries EINKOMPILIERT: eine Aenderung
dort ist binary-beruehrend, auch wenn kein ABI-Symbol wandert. Gesperrt ab C11 mit der Ausnahme der
A8-Scheiben unten (Abschnitt 3), die VOR dem Anker liegen muessen.

### 1.7 Dock-Vertrag (HY-D2-Freeze)

`builder/pruef_dock/pruef_dock.hpp`: die `IPruefDock::measure`-Signatur, die `dock_status_*`-Ints
(0..4) und der V5-Konformitaets-Gate-VERTRAG (import -> GATE -> messen) bleiben unveraendert.
Die dock_status-Ints sind ABI-nahe TRANSPORTFORM; die KLASSIFIZIERUNG geschieht CEB-seitig
(FK-7, `dock_error_classification.hpp`) und ist dort erweiterbar, ohne den Vertrag zu beruehren.

---

## 2. Ausnahme-Klasse: A8-S2 und A8-S6

**A8-S2 und A8-S6 sind von G8 AUSGENOMMEN.** Begruendung: sie sind kanten-frei -- host-/doku-only.
Sie beruehren keine der Flaechen aus Abschnitt 1, kompilieren in keine Tier-Binary ein und bewegen
weder Lade-Akzeptanz noch Fingerprint noch binary_id. Sie duerfen deshalb NACH dem Anker-Vollzug
laufen und bleiben trotzdem trigger-blockierend als A8-Kern (A8-DOSSIER:346-349).

Die Ausnahme gilt AUSSCHLIESSLICH fuer S2/S6 und AUSSCHLIESSLICH, solange sie host-/doku-only
bleiben. Faellt in S2/S6 ein Bedarf an einem Sub-Interface oder einem POD an, gilt STOPP + Manager
(K-b) -- nicht "das ist ja nur ein kleines Feld".

---

## 3. Vor-Anker-Pflichtliste (K-a/K-d) -- Commit-Hash-Nachweis

Der EINE Anker-Vollzug (TP1-Neu-Inventur + A2-SHA512-Eichung, GATE 5) darf erst laufen, wenn ALLE
binary-beruehrenden Scheiben gelandet sind. Sonst ist die Ein-Anker-Bilanz falsch.

### 3.1 GELANDET (Nachweis am Objekt, `git log` im ce-Repo)

| Scheibe | Commits | Merge |
|---|---|---|
| **A8-S1** (T17-Messwert-Verlust) | `8ad4ef19939f8e8b1fdc01237045b6efd9868c2a` (fix: die drei <17-Kopierschleifen auf kV3AxisCount)<br>`57237484cc9cd12c67a4d2579948f337ef0394a8` (refactor: hartkodierte Achsen-Zahlen der Wire-Naht auf die tragende Konstante)<br>`a26fba868a1bc26dd1f2285bfc576bd222dc90b7` (test: blinde <17-Schleifen der Mess-Tests auf kV3AxisCount) | `de7688b9d8f4040a3fb4c7f48d4665618457c3fa` |

Alle vier Hashes sind am Branch `e24-b` erreichbar (Vorfahren dieses Fensters -- A8-S1 lief VOR dem
b-Teil, wie die strikte Serialisierung S1 -> Fenster-b -> S3 es verlangt; beide fassen
`abi_adapter.hpp` an). Wirkungs-Beleg: `grep -n "i < 17\|t < 17"` ueber `anatomy/abi_adapter.hpp` und
`builder/experiment_tree/node_value_measurement.hpp` liefert am Ist **0 Treffer** -- die drei
Schleifen der Bauplan-Inventur (Paragraf 1.0) existieren nicht mehr.

| Scheibe | Stand |
|---|---|
| **W10** (System-Zellwerte in die system_stamp_line) | GELANDET vor dem b-Teil; hat u. a. den codegen-Minor 1 -> 2 gedreht (s. Abschnitt 5, ABWEICHUNG). |
| **E-24 a-Teil** (C0-C5) | GELANDET als Merge `44bcda99`. |
| **E-24 C6-V** | GELANDET als Merge `19adba05`. |
| **E-24 b-Teil** (C6-C11) | dieses Fenster. |

### 3.2 NOCH OFFEN -- und deshalb PFLICHT VOR DEM ANKER

| Scheibe | Warum vor dem Anker |
|---|---|
| **A8-S3** | Fasst `abi_adapter.hpp` an (SA-Member-Nachruestung + Einsammlungs-Erweiterung). Binary-beruehrend. Serialisiert HINTER diesem Fenster (Ein-Schreiber-Regel). |
| **A8-S4** | Binary-beruehrend, parallel zu S5 fahrbar. |
| **A8-S5** | Fasst `axes/`-Header an (Ist: 70 von 340 mit std-Container-/OS-Call-Treffern -- Datei-Liste IMMER aus grep, nie aus dieser Zahl). Binary-beruehrend, familienweise parallel. |

**Diese drei stehen hier EXPLIZIT als vor-Anker-Pflicht.** Laufen sie nach dem Anker, ist das ein
zweites Neuanker-Ereignis -- der Fehlerfall, den C11 verhindern soll.

Entsteht in S3/S4/S5 ein Sub-Interface-/POD-Bedarf: **STOPP + Manager** (K-b). Nach C11 greift G8
ausnahmslos; ein solcher Bedarf ist dann ein Owner-Entscheid, keine Bau-Entscheidung.

---

## 4. Was NICHT gesperrt ist

Damit die Sperre nicht zur Laehmung wird, ausdruecklich erlaubt:

- **Kommentare, Doku, Session-/Architektur-Dokumente.** Kommentar-Heilung ist die einzige Reaktion auf
  die K1-Prosa-Reste (Abschnitt 5.1) und bleibt bis zur Abgabe offen.
- **Tests, die keine ABI-Flaeche bewegen** -- neue Wachen, neue Proben, Fixture-Nachzuege.
- **Host-/CSV-Seite** (Klasse C des Qualitaets-Katalogs): Spalten-Appends am Snapshot-Ende,
  Legenden-Semantik, Tail-Perzentile, pmc-Spalte. Kein Wire-Ereignis.
- **FK-7-Klassifizierung CEB-seitig** (`dock_error_classification.hpp`) -- sie liegt neben dem
  Dock-Vertrag, nicht in ihm.
- **Auswertungs-/Phase-6-Arbeit**, Thesis-Text, Bestandslog-Werkzeuge ohne Fingerprint-Naht.

---

## 5. P11-Sweeps (C11-Auflage)

### 5.1 P11-A -- K1-Prosa-Rest "SearchAlgorithm-Gattung"

Nach der C7-1-Umbenennung (Ebene 1 heisst **Map**) ist "SearchAlgorithm-Gattung" ein Ebene-1-Etikett
mit dem Namen des Ebene-2-Genus -- also genau der Owner-benannte Terminologie-Fehler, nur in Prosa.
Erhebungs-Kommando (bindend ist das Kommando, nicht die Zahl):

```
grep -rn "SearchAlgorithm-Gattung" --include=*.hpp --include=*.cpp libs/ apps/ tools/ tests/
```

Ist 04.08. nach C11 (a): **29 Treffer** (vor C11 (a): 31). Geheilt wurden in C11 (a) die drei
DEFINITORISCH tragenden Stellen -- die Gattungs-Spalte der Tier-Metapher-Tabelle
(`anatomy/anatomy_base.hpp`, Saeugetier-Zeile), der Doku-Kommentar des `AnatomyGenus`-Enumerators
(dieselbe Datei) und der Doku-Kommentar ueber dem `ContainerType`-Concept
(`anatomy/container_framework.hpp`), der dem `static_assert`-Text derselben Datei widersprach.

**Klassifizierung: K1 (Prosa/Kommentar-only) -- compile-neutral, golden-neutral, ABI-neutral.**
Betroffen sind Doku-Kommentare, Include-Kommentare und zwei `static_assert`-DIAGNOSETEXTE
(`anatomy/abi_adapter.hpp:195`, `compositions/prt_art_merge_reference.hpp:212`). Keine dieser Stellen
steht unter G8.

**ENTSCHEID (deklariert, nicht still):** der Rest-Sweep laeuft NICHT im letzten Commit eines
ABI-Fensters. Ein Kommentar-Pass ueber ~20 Dateien unmittelbar vor dem Anker-Vollzug bringt null
Erkenntnis und maximale Diff-Flaeche -- und er faellt nicht unter G8, ist also NICHT durch das
Fenster-Ende befristet. Er geht als benannter Posten in den Abschluss-Aufraeumpass (Abschnitt 6), wo
er ohne Zeitdruck und mit dem Diff als Quelle der Datei-Liste erledigt wird. C11 (a) hat bewusst nur
die Stellen gezogen, die als DEFINITION gelesen werden und dem Enum/`static_assert` derselben Datei
widersprachen.

### 5.2 P11-B -- Graph-Sweep AUSSERHALB der Anatomy-Flaeche

Auflage aus Owner-KERN NACHTRAG 4 (LEDGER:3836): *"Die Gattung Graph hat noch keine Genus
implementiert und ist daher derzeit stub. Sofern doch etwas implementiert ist, muss das bezueglich
der beiden Ebenen eingeordnet werden."* Erhebungs-Kommandos:

```
grep -rniE "\bgraph" --include=*.xml --include=*.cmake --include=CMakeLists.txt .
grep -rniE "\bgraph" apps/anatomy_codegen_tool/ tools/
grep -rniE "\bgraph\b" --include=*.hpp --include=*.cpp libs/ apps/ tools/ tests/
```

**BEFUND (vollstaendig, mit Einordnung nach BEIDEN Ebenen):**

| Fund | Ebene 2 (Genus) | Ebene 1 (Gattung) | Einordnung |
|---|---|---|---|
| `AnatomyGattung::Graph = 2` (`anatomy/anatomy_base.hpp:43`) | -- | Graph | **STUB.** Kein Genus, keine Anatomie, keine Komposition. |
| `admit_gattung_build_path` (`builder/experiment_tree/genus_build_admission.hpp:15-18/:97-98/:189-195`) | -- | Graph | Korrekte Stub-WACHE: weist einen Graph-Bau-Wunsch `constexpr` ab. Kein Rest, sondern die Absicherung des Stubs. |
| `pruef_dock_version.hpp:64-66` | -- | Graph | Nur Doku ("AnatomyGenus hat FUENF und KEIN Graph"). Korrekt. |
| `genus_binding_traits.hpp:8` | -- | Graph | Erwaehnung als kuenftiger Erweiterungspunkt. Korrekt. |
| **`virus/graph_bfs.hpp` (`GraphBfs`)** + `execution_engine/virus_execution_engine.hpp` | **KEINS** | **KEINE** | **DER EINZIGE implementierte Graph-CODE -- und er liegt NICHT unter der Anatomie.** `GraphBfs` ist eine `IVirusExecutionEngine`: der Nicht-Lebewesen-Schwesterzweig an der `IExecutionEngine`-Wurzel, ohne Achsen-System, ohne Komposition, ohne Anatomie. Er ist damit weder Genus noch Gattungs-Instanz; die Zwei-Ebenen-Einordnung lautet: **ausserhalb beider Ebenen**. Der Enumerator `AnatomyGattung::Graph` bleibt davon unberuehrt und weiter Stub. |
| `test_d12_virus.cpp`, `test_genus_binding.cpp:3`, `test_striktheit_metaprog_guard.cpp:45`, `test_e24_c7_gattung_map_umbenennung.cpp:14/:62/:98-101` | -- | Graph | Wachen, die den Stub-Zustand FESTHALTEN (`genera_in(Graph) == 0`). Korrekt. |

**XML / CMake / Codegen: 0 Graph-Treffer.** Der einzige Treffer in einer `.xml` ist ein Paper-TITEL
("Space-efficient Static Trees and Graphs (LOUDS)", `algorithm_profiles/sota/louds.profile.xml:7`) --
kein Gattungs-Bezug. `anatomy_codegen_tool` und `buildsystem`-XML tragen nichts zu Graph.
`tools/p27_bundle_finder` fuehrt einen `CallGraph` -- ein Disassembly-Werkzeug, kein Gattungs-Bezug.

**VERDIKT P11-B:** der Anatomy-Ist ist deckungsgleich mit NACHTRAG 4. Graph ist Stub; das einzige
implementierte Graph-Stueck (`GraphBfs`) gehoert zum Virus-Zweig und ist damit korrekt eingeordnet.
Es besteht KEIN Handlungsbedarf im Fenster. Der offene Punkt 4 des Gattungs-Diskrepanz-Dossiers
(Abschnitt 7) ist damit ERLEDIGT.

---

## 6. Abschluss-Aufraeumpass -- Kandidaten aus diesem Fenster

> **ZEIGER (08.08.2026): die geltende, zusammengefuehrte Kandidatenliste steht im super-Repo unter
> `docs/plaene/20260808-KANDIDATENLISTE-75-abschluss-aufraeumpass.md`.** Dieser Abschnitt bleibt als
> Fenster-Beleg stehen, ist aber nicht mehr die Arbeitsgrundlage.
>
> **Zwei Richtigstellungen aus dem Zusammenfuehren:**
> 1. Der Satz unten *"Fortschreibung der Liste im E24-Dossier (die Nummern 33/34 stammen von dort)"*
>    trifft nicht zu: `docs/architecture/20260803-e24_container_gattungs_abi_dossier.md` enthaelt
>    **keine** nummerierte Kandidatenliste (nachgeprueft mit `/usr/bin/grep -nE '^\s*[0-9]+\.\s'` und
>    `-nE '^\| *[0-9]+ *\|'`; es referenziert die Liste nur bei `:132` und `:296`). Die Nummern
>    `33`-`38` stammen aus dem **super-Ledger** (`:3825` und `:3861`) und sind hier eine **Doppel-
>    fuehrung** derselben Posten, keine eigene Zaehlung.
> 2. Ist-Stand der sechs Posten am 08.08. (ce `4487a9c1`): `33` OFFEN (Doppel-Schicht am Objekt
>    belegt, `set_dock.hpp:55` + `:100`) | `34` **UNBELEGT** (Fundstelle nicht auffindbar) | `35`
>    **ERLEDIGT durch Design-Entscheid** (`pruef_dock_version.hpp:32/:66-74` erklaert das virtuelle
>    `dock_version()` ausdruecklich fuer unerwuenscht und stellt `pruef_dock_version_for(genus)` als
>    Ersatz) | `36` OFFEN, Umfang auf **54** Prosa-Treffer gewachsen (nicht 29) | `37` OFFEN,
>    **Dublette von Ledger-Posten (32)** | `38` gilt weiter: die `perm_*`-Module NICHT reparieren.

Fortschreibung der Liste im E24-Dossier (die Nummern 33/34 stammen von dort):

| # | Kandidat |
|---|---|
| 33 | `SetDock`/`SetPruefDock`-Doppel-Schicht je Dock-Datei -- Namens-Kollisions-Grund dokumentiert, Zusammenfuehrung offen. |
| 34 | Nummerierungs-Drift in CMake-Kommentaren (a/5 vs. a/2). |
| 35 | SA-Dock ohne `dock_version()`-Member (Asymmetrie zu den vier neuen Docks). |
| 36 | **P11-A-Rest: 29 K1-Prosa-Stellen "SearchAlgorithm-Gattung"** (Abschnitt 5.1). Datei-Liste aus dem grep ableiten, nie handgepflegt. |
| 37 | `kAdapterCompositionSlotCount == 13` (frozen legacy) vs. live `slot_count == 11` -- bewusst unangetastet, gehoert aufgeloest. |
| 38 | Die vier `perm_*`-Prueflings-Module aus C10 (`perm_set_defekt_d13`, `perm_alt_major7`, `perm_major7_neue_magic`) sind ABSICHTLICH defekt/veraltet -- beim Aufraeumen NICHT "reparieren". |

---

## 7. TABU-Belege dieses Fensters

Je Commit gefuehrt, hier zusammengefasst (`git diff --stat 19adba05..HEAD`):

- `golden_fullpilot_320*`, `permutation_axes.xml`, `m3v2_study.profile.xml`,
  `system_axis_registry.xml`: **0 Treffer** im diff-stat aller b-Teil-Commits.
- `abi/anatomy_fingerprint.hpp`, `abi/subaxis_valueset_segment.hpp`, `abi/system_cell_values.hpp`,
  `profile_facade/source_catalog.hpp`, `src/sha512/ctsha512.hpp`, `abi/system_axis_order.hpp`,
  `tests/unit/thesis_tiere/`: **byte-unberuehrt** (leerer diff).
- CRC64-Anker `0x56F1B721C72DC10E`: unveraendert (`source_catalog.hpp:190`).
- `fingerprint_format`: bleibt 2.
- Stempel-Preimage: fuer ein festes Referenz-Tripel VOR und NACH C8 byte-gleich (C10 (c), Digests
  dort literal).
- golden-320-Roundtrip: je Commit gruen.

---

ENDE. Diese Datei ist die verbindliche Negativ-Liste; sie wird NICHT geloescht, nur fortgeschrieben.
