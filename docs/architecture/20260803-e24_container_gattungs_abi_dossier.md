# A16-P1-DOSSIER — E-24 Container-Gattungs-ABI: LETZTER ABI-Schritt vor dem Voll-Bau-4-Trigger (NEUFASSUNG v2)

**Datum:** 03.08.2026 · **Agent:** Design-Nachtrag (Fable 5, Auftrag E24/A16-P1) · **Status:** VOLLSTAENDIGE NEUFASSUNG als Datei — ersetzt die nur im Backup-JSON existierende v1 (`super/docs/sessions/backups/20260802-welle3-parallel-workflows/00-welle3-result.json`, `result.designs.designs[0]`, key `A16-P1-E24-container-gattungs-abi-dossier`, 33.156 Zeichen, "read-only erstellt — keine Datei geschrieben").
**Paket:** A16-P1 (KATALOG A16 / Owner-R7 02.08.: "E-24, E-19, E-02, E-23, E-21 sind jetzt PFLICHT") · **Gate-Einordnung:** GATE 4 der Gate-Kette (00-SYNTHESE:226), LETZTER ABI-Schritt VOR dem Trigger (HY-D2-Merksatz bindend; ABGLEICH 03.08.:27 "E-24 = KRITISCHE ABI-KANTE VOR Trigger").
**ce-Ist verifiziert am:** development `0d7a0d92` (Merge w2d-Transport-Bestandslog) bzw. HEAD `1cb1efbf` (nur ein cppcheck-Suppress in tests/ darueber) — Stand NACH Welle-2d-Landung; alle datei:zeile-Anker dieser Fassung sind an diesem Stand live nachgeprueft (03.08.).
**Pfad-Konvention:** super = `/home/comdare/Projekte/Research/probst-diplomarbeit-cache-engine` · ce = `super/Code/external/comdare-cache-engine` · LEDGER = `super/docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md` · ANKER = `super/docs/sessions/20260726-SESSION-wahrheits-anker-voll-abgleich-982-E01-E26.md` · A8-DOSSIER = Welle-3-Backup-JSON, key `A8-F2-BENCHMARKING-SCHNITT-DESIGN-20260802` (Neufassung parallel in Arbeit). ASCII-Transliteration durchgehend.

## DELTA-LOG v1 -> v2 (Review-Verdikt NACHBESSERN, alle Befunde eingearbeitet)

| # | Befund (Welle-3-Review, `result.designs.review.befunde[]`) | Einarbeitung |
|---|---|---|
| R-1 | **SCHWER [E24+A8], befunde[3], Hauptgrund NACHBESSERN:** beidseitige A8<->E-24-Sequenz-/Fingerprint-Kante fehlte; Option C hatte KEINEN A8-Slot; A8-S3-Sub-Interfaces kollidieren mit Auflage 10/G8 | NEU: Abschnitt 2.7 (A8-Kante), Sequenz-Abschnitt 3 vollstaendig ueberarbeitet (A8-Slot, Risiko-Matrix-Zeile 9), Auflage 17, G8 erweitert, Vorlage V3 erweitert, OP-14 neu |
| R-2 | klein [E24], befunde[1]: "register_dock 0 produktive Aufrufer in libs/+tools/" zu eng (apps/ unterschlagen); "god-header ~1805 Z." stale | korrigiert in 1.4/1.5 (abi_adapter.hpp = **2262 Zeilen**, live `wc -l` 03.08.; produktiver Aufrufer `ce apps/f15_compare/main.cpp:220`), OP-4 als TEILBEANTWORTET fortgeschrieben |
| R-3 | [E24], befunde[2]: Fehlerklassen-Pflicht (Owner-E6/A15) fuer NEUE Container-Docks/Genus-Fehlerpfade nicht adressiert; G3 prueft nur dock_status 2/4 | NEU: Abschnitt 2.8 (FK-Abstimmung), Auflage 16, G3 um FK-Beweis erweitert |
| R-4 | Owner-Nachtraege 02.08. ~17:0x (Q3 Flag-Grammatik), ~17:4x (Q4-Q9-Defaults freigegeben, inkl. Q5), ~19:5x (Q10 gerenderte Form ohne 'v'), ~21:2x (Reihenfolge 2d -> M3 -> M4 -> OD-10-RT "wie empfohlen") + Nachtraege 03.08. (Neustart-Lage, Parallelisierungs-Freigabe) | eingearbeitet in 2.3/2.6, Auflage 6, Abschnitt 3 (M4-Position NEU bewertet — die v1-Empfehlung "M4 hinter E-24 verschieben" steht jetzt GEGEN eine owner-bestaetigte Reihenfolge und wird als explizite V3-Frage gefuehrt, nicht mehr als Default) |
| R-5 | Welle-2d-Landung (0d7a0d92/43a1b65a) veraendert Transport/Bestandslog/Flag-Grammatik | Ist-Abgleich 1.7 NEU: 2d-Flaeche ist DISJUNKT von der E-24-ABI-Flaeche (kein Anker verschoben); Flag-Grammatik-Wachen (`algo_semver.hpp`, `planner_version.hpp`, `test_reflect_versions_all_registered.cpp`) sind jetzt Pflicht-Naht fuer neue Genus-Versionseintraege (Auflage 6) |
| R-6 | OP-1 (#37-Scheduling-CT-Ist) war "live pruefen" | GEPRUEFT 03.08.: CT-Achse existiert (`scheduling_system_axis.hpp`), als CEB-UNTER-Achse des target_isa-Komplex-Wrappers (O-8 Schritt 6); DEPRECATED-vtable-Rest 0 Konsumenten — OP-1 aufgeloest, Details 1.8 |

Alle uebrigen v1-Inhalte (Ist-Kartierung, Fenster-Schnitt S12.1-S12.3, TABUs, Gates) sind unveraendert uebernommen und am 0d7a0d92-Ist erneut verifiziert.

---

## 0. Executive Summary

E-24 = Vervollstaendigung der Gattungen SearchAlgorithm + Container als **der LETZTE ABI-Schritt des Projekts** (Owner-Wortlaut ANKER:115-118: "diese Gattung als ABI-Schritt als LETZTEN Schritt mit eigener ultracode-Planung anhaengen und mit der Vervollstaendigung der Gattung SearchAlgorithm und Container erweitern ... bei weitem nicht erfuellt, gigantische Erweiterung ueber parallele Achsensysteme weiterer Gattungen").

Die **harte Kante aus HY-D2** (ce `docs/architecture/20260802-hybrid_tier_stufe_soll_design.md:280-283`, Risiko-Tabelle :413) verlegt das Fenster ZWINGEND VOR den Voll-Bau-4-Trigger: bumpt der ABI-Major erst NACH dem Voll-Bau, laedt die Hybrid-Stufe (und jeder neu gebaute Host) die eingelagerten 2^17-Binaries nicht mehr (Major-Check des Loaders, `anatomy_module_loader.hpp:149` "5. Major-Version match (Host vs Modul)") = Neubau ALLER Binaries — exakt der von Owner-E3 ausgeschlossene Zustand. Der Major ist dabei **Owner-fixiert**: F1b-Ebene-1-Promotion = "ABI/golden-GO GEWAEHRT (MAJOR)", "die Gattungen und deren Durchsetzung ist UNVERHANDELBAR" (LEDGER:1576-1581, User-Entscheid 16.07.).

Das Fenster ist **EIN S12-Fenster ohne stillen Split** (BAUPLAN:26, Review-MAJOR): Cross-Genus-Komposition-als-Sub-Organ via `OrganConcept` GEHOERT HINEIN (LEDGER:2771/:2828/:2858); "Rest" in Owner-R7 = die nicht-gelisteten E-Punkte, NICHT E-24-Teile. Graph-Gattung bleibt draussen (nach Abgabe, eigener Owner-Entscheid; Q5-Default per Owner-GO ~17:4x freigegeben, formale Quittung via V3b).

**NEU in v2 — die A8-Kante (Review-SCHWER):** Das E-24-Fenster endet mit dem EINEN Anker-Vollzug (TP1-Proben-Neu-Inventarisierung + A2-SHA512-Eichung). Die A8-Scheiben S1/S3/S4/S5 beruehren `abi_adapter.hpp` + 54 `axes/`-Header — einkompiliert in JEDES Tier-Binary, jede solche Scheibe dreht die SHA512-Fingerprints (A8-Dossier Abschn. 3-c). **Alle binary-beruehrenden A8-Scheiben MUESSEN daher VOR dem E-24-Anker-Vollzug liegen; A8-S3-Sub-Interfaces VOR/IM E-24-Fenster** (sonst Kollision mit der Nach-Fenster-ABI-Sperre Auflage 10/G8). Ohne diese Kopplung waere die Ein-Anker-Bilanz der Sequenz-Empfehlung falsch und V3 entscheidungs-unvollstaendig. Details Abschnitt 2.7/3.

**Sequenz-Empfehlung V3 (Abschnitt 3): Option C' (aktualisiert)** — E-24 als EIGENES, sauber reviewbares Bau-Paket in der owner-bestaetigten Kette `2d -> Q-M3-GATE -> A13-M3(+FK-5) -> M4 -> OD-10-RT -> ...` (Owner-Nachtrag ~21:2x + ABGLEICH 03.08.:49-51), mit A8-binary-beruehrenden Scheiben VOR dem Anker-Vollzug und EINEM Anker-Vollzug (TP1-Inventur + A2-Eichung) NACH der E-24-Landung. Die v1-Idee "M4 ans Fenster-Ende hinter E-24 verschieben" (ein +ceb=-Shift statt zwei) bleibt als OPTION in V3, ist aber nicht mehr Default, weil der Owner die Reihenfolge M3 -> M4 inzwischen bestaetigt hat.

---

## 1. Ist-Kartierung Gattungs-ABI (datei:zeile, live verifiziert 03.08. am Stand 0d7a0d92)

### 1.1 Zwei-Ebenen-Modell (Ebene 1 Gattung / Ebene 2 Genus)

| Anker | Datei:Zeile (ce) | Inhalt |
|---|---|---|
| Ebene-1-Enum | `libs/cache_engine/anatomy/anatomy_base.hpp:40-44` | `AnatomyGattung{SearchAlgorithm=0, Container=1, Graph=2}` — Aussen-Interface, Pruef-Dock je Gattung |
| Ebene-2-Enum | dieselbe Datei `:78-84` | `AnatomyGenus{SearchAlgorithm=0, Set=1, Sequence=2, Adapter=3, View=4}` — Tier-Unterklassen |
| Mapping | `:100-109` | `gattung_of()`: SA -> eigene Gattung; Set/Sequence/Adapter/View -> Container |
| F1b-Vermerk | `:76-77` | "eigenstaendige AnatomyGattung ist der SEPARATE, koordinierte ABI-Schritt F1b (NICHT hier)" — Promotion unvollzogen |
| stale (NEU v2) | `:79` | Genus-Kommentar "vollst. 17-Achsen-Anatomie, INC-2d" — stale ggue. ORG-18 (18 Achsen inkl. persistence_target); Nachzug-Kandidat OP-9 |
| Kanon | LEDGER Abschn. 54-T1 (00-SYNTHESE:32) | "Gattung" bezeichnet AUSSCHLIESSLICH das Tier-Binary-INTERFACE; KEINE Achsen-Gattungen |

### 1.2 Bau-Bindung (GenusBindingTraits) — 5/5 gebunden, Tiefe stark asymmetrisch

| Genus | `libs/cache_engine/builder/experiment_tree/genus_binding_traits.hpp` | Slots | Besonderheit |
|---|---|---|---|
| (primaer) | `:31-32` | — | Primaer-Template UNDEFINIERT; jede Gattung liefert Voll-Spezialisierung |
| SearchAlgorithm | `:41-59` | 18 (`:45`) | EINZIGE am zentralen `kCompositionAxisNames` (`:56-58`, "STRUKT-R ORG-18: 18"); T0..T17 inkl. persistence_target |
| Adapter | `:66-87` | 11 (`:70`) | `:69` traegt ZUSAETZLICH `gattung = AnatomyGattung::Container` (einziger Trait mit Ebene-1-Feld); eigene Namensliste |
| Set | `:92-114` | 13 | eigene Namensliste |
| Sequence | `:119-139` | 9 | eigene Namensliste |
| View | `:144-163` | 5 | eigene Namensliste |
| Concept | `:167-171` | — | `GenusBound<G>` — alle 5 erfuellt (Kommentar `:166`: "ALLE 5 Gattungen gebunden (D11) ... (5/5)") |
| TABU Q-8 | `:37-40` | — | persistence_target NICHT auf Container-Genera ausgedehnt — Owner-Entscheid Q-8 (26.07.): "Wer das aendert, braucht einen neuen Owner-Entscheid, nicht nur einen Patch" |

### 1.3 Container-Framework (Ebene-1-Sicht, heute Re-Export)

| Anker | `libs/cache_engine/anatomy/container_framework.hpp` | Inhalt |
|---|---|---|
| #29-ENTPARKT | `:20-25` | F1b per User-GO 16.07. FREIGEGEBEN; Umsetzung = "KOORDINIERTER ABI-Schritt (Version 4->5, gemeinsam mit #37 nach F12iii)" — historische Major-Zaehlung, heute 7->8 |
| ContainerType | `:46` | Concept: `gattung_of(G)==Container && GenusBound<G>` |
| Self-proving | `:85-95` | static_asserts: type_count==4; Adapter/Set/Sequence/View sind Container-Typen, SA NICHT; Slot-Zaehlungen 11/13/9 gepinnt |
| Genus->Typ | `:17-19` | "echte Genus->Typ-UMSTRUKTURIERUNG ... waere ABI/golden-beruehrend" — der S12.3-Fenster-Inhalt (Option A/B, LEDGER:2858) |

### 1.4 ABI-Flaeche (Decl, Modul-Makros, Adapter, Antriebs-Sub-Interfaces)

> **NACHTRAG 08.08.2026 (Paket HY-0):** die beiden ABI-Zeilen dieser Tabelle sind der IST-Stand **VOR**
> dem E-24-C8-Bump -- also genau der Zustand, den dieses Dossier zu bewegen plante (Abschnitt 2.6/4).
> Vollzogen wurde er am 04.08.2026 mit ce `4f569051`.
> Damals galt: **Major 8**, Magic **`.A8.`** = `0x434F4D444141382EULL` -- ABI-HISTORIE gegen SHA 0f08fab5.
> Lebend gilt seit NAHT-1 (09.08.2026, Mess-Naht am Genus-Interface): **Major 9**, Magic **`.A9.`** = `0x434F4D444141392EULL` -- Belege `anatomy_module_abi_v1_decl.hpp:108` und `anatomy_module_abi_v1_decl.hpp:112`.
> *(Nachsatz 08.08., Review-Mangel geschlossen: genau dieser Satz war eine vom Paket selbst gepflanzte
> Lebend-Behauptung in einer Formulierung, die Teil B der Wache nicht sah -- ein gewuerfelter
> Major-4-Koeder in derselben Formulierung blieb gruen. Teil B triggert jetzt auch auf "lebend
> gilt/seit/:", die Zeile steht in EINER Zeile mit voll ausgeschriebenen Ankern, und ihre Anker
> laufen durch die Teil-C-Anker-Pruefung: beim naechsten Bump wird sie ROT statt still falsch.)*
> Die Zeilen-Anker der Tabelle (`:62`, `:66`, `:59`, `:264`, `:270-283`) sind gegen SHA `6b8eee0f`
> verifizierte Historie und im heutigen Header GEWANDERT -- sie werden deshalb gekennzeichnet und
> nicht stillschweigend korrigiert. Erzwungen wird der Nachzug in der Hybrid-Doku von
> `scripts/ci_hy_label_gate.sh`; DIESES Dossier steht als GANZES weiter NICHT unter deren Teil A
> (Begruendung: es ist ein datierter Planungs-Schnappschuss, dessen Zahlen als Vorher-Stand richtig
> sind -- eine Wache darueber waere eine Dauerklage ueber korrekte Historie); seine ausdruecklichen
> Lebend-Behauptungen deckt der listenlose Teil B.

| Anker | Datei:Zeile (ce) | Inhalt |
|---|---|---|
| ABI-Major/Minor | `libs/cache_engine/include/cache_engine/abi/anatomy_module_abi_v1_decl.hpp:62-63` | `COMDARE_ANATOMY_ABI_MAJOR 7` / `MINOR 0` |
| ABI-Magic | `:66` | `0x434F4D444141372E` (".A7.") — Magic kodiert den Major (`:59`) |
| Host-Version | `:264` | `kHostAnatomyAbiVersion{MAJOR, MINOR}` |
| **+ceb=-Kopplung** | `:270-283` | `kCebContractCodegenMinor = 0` (`:277`) + `kCebContractVersion{COMDARE_ANATOMY_ABI_MAJOR, kCebContractCodegenMinor}` (`:283`) — **ein Major-Bump verschiebt AUTOMATISCH das `+ceb=`-Suffix aller build_version-Strings = Lager-Key-Shift** (Identitaets-Bilanz Abschnitt 2.6); "Jeder Bump laesst jede perm.dll.version mismatchen -> ALLE Binaries neu" |
| SA-Modul-Makro | `abi/anatomy_module_abi_v1.hpp:35` | `COMDARE_DEFINE_ANATOMY_MODULE` (4 Pflicht-extern-C-Symbole) |
| Set-Modul-Makro | `abi/set_module_abi_v1.hpp:10-11` | `COMDARE_DEFINE_SET_MODULE` — 4 extern-C-Symbole + ABI-Version/Magic IDENTISCH -> DERSELBE Loader |
| weitere Makros | `abi/adapter_module_abi_v1.hpp:4`, `abi/sequence_module_abi_v1.hpp`, `abi/view_module_abi_v1.hpp` | analoge Familie — Modul-Autor-Seite existiert fuer ALLE 5 Genera |
| SA-ABI-Adapter | `libs/cache_engine/anatomy/abi_adapter.hpp:39-52` | SearchAlgorithmAbiAdapter, god-header **2262 Zeilen** (live `wc -l` 03.08.; v1-Angabe "~1805 Z." war stale), VOLLE Sub-Interface-Matrix (IObservableTier/IRollbackable/IScannable/IMigratable/IResourceControllable/IAllocatorProxy/IMeasurableWorkload, Inhaltsverzeichnis `:39-52`) |
| Set-ABI-Adapter | `anatomy/set_abi_adapter.hpp:18-21` | `SetAbiAdapter final : IAnatomyBase, ISetTier` + `static_assert(genus()==Set)` ("Cross-Genus-Adapter sind type-system-mathematisch unmoeglich") |
| weitere Adapter | `anatomy/sequence_abi_adapter.hpp`, `anatomy/adapter_abi_adapter.hpp`, `anatomy/view_abi_adapter.hpp` | analog, je Genus |
| Antriebs-Sub-Interfaces | `anatomy/set_tier.hpp` (ISetTier + SetObserverSnapshotV1), `anatomy/sequence_tier.hpp`, `anatomy/adapter_tier.hpp`, `anatomy/view_tier.hpp`; SA: `anatomy/observable_tier.hpp:185/:193` | je Genus eigenes natives Sub-Interface vorhanden (F1a-Beleg LEDGER:516: Set = vollwertige native Tier-Unterklasse, kein Alias) |
| Loader | `builder/anatomy_module_loader/anatomy_module_loader.hpp:70-75` (destroy-vor-dlclose), `:147-151` (7-Schritt-Validierung, `:149` "5. Major-Version match (Host vs Modul)"), `:158-159` (platform_suffix) | EIN gattungs-agnostischer Loader; der Major-Check ist der Mechanismus der HY-D2-Kante |

### 1.5 Docks / Registry / Sequencer / Engines / Observer

| Anker | Datei:Zeile (ce) | Ist |
|---|---|---|
| Dock-Doktrin | `builder/pruef_dock/pruef_dock.hpp:10-14` | IPruefDock KEINE ABI-Grenze; Grenze = gattungs-eigenes Antriebs-Sub-Interface + flacher POD; "Neue Gattung = neues Dock + neues Sub-Interface + neuer flacher POD — NIE eine Mutation von IAnatomyBase oder dem bestehenden Snapshot" |
| dock_status-Familie | `:36-48` | `ok=0 / no_anatomy=1 / wrong_genus=2 / subinterface_missing=3 / conformance_failed=4` + `dock_status_name` — heutige Fehlerkennung der Dock-Ebene (FK-Naht, Abschnitt 2.8) |
| IPruefDock | `:57-79` | `:62` dock_genus, `:69` accepts ueber IM MODUL deklarierte Gattung, `:74-79` measure + **V5-Konformitaets-Gate-VERTRAG** ("Jede measure()-Implementierung MUSS das Modul VOR der Messung ... pruefen") — HY-D2-Freeze (NICHT-Kandidat) |
| SA-Dock | `builder/pruef_dock/search_algorithm_dock.hpp:26` | `SearchAlgorithmDock final : IPruefDock` — produktiv (DLL-Pfad, Drive `:65-86`) |
| Container-Docks | `set_dock.hpp:8/:24`, `adapter_dock.hpp:8`, `sequence_dock.hpp`, `view_dock.hpp` | existieren NUR als in-process-Templates ueber die Anatomie; wortlautlich: "Der DLL-Pfad ... ist ein Folgeschritt" — KEINE IPruefDock-Implementierungen, kein Konformitaets-Gate, keine Registry-Faehigkeit |
| Registry | `builder/pruef_dock/pruef_dock_registry.hpp:4-5` (stale: "initial nur SearchAlgorithmDock; Set/Sequence/Adapter/View kommen mit V42"), `:22-40` (`register_dock`/`select_for`/`dock_for_genus`) | EIN Dock je Gattung. **KORRIGIERT (R-2):** `register_dock` hat **genau EINEN produktiven Aufrufer**: `ce apps/f15_compare/main.cpp:220` (`reg.register_dock(std::make_unique<pd::SearchAlgorithmDock>())`, COMPARE-App, Observe-Pfad) — in libs/ + tools/ weiterhin 0 Treffer. OP-4 damit TEILBEANTWORTET: der Registrierungs-Ort existiert, registriert aber NUR das SA-Dock; die Container-Docks haben keinerlei Registrierungs-Pfad |
| Sequencer | `builder/pruef_dock/pruef_dock_sequencer.hpp:44-47` | `measure_genus_sequential` — bereits gattungs-uebergreifend gebaut (gruppiert Handles stabil nach Modul-Gattung) |
| Engines | `anatomy/search_algorithm_permutation_engine.hpp` (`for_each_abi_adapter`, einziger Treffer repo-weit, grep 03.08.); `set_/sequence_/view_permutation_engine.hpp` existieren OHNE for_each_abi_adapter; **`adapter_permutation_engine.hpp` existiert NICHT** (ls 03.08.: nur search_algorithm/sequence/set/view) | OP-3 |
| Observer | `anatomy/observer_aggregate.hpp:2/:79` (per-Achse-Aggregat, SA; Kommentar "17-Achsen"/"17 named Snapshot-Members" stale ggue. ORG-18) vs. flache Hand-PODs der Container-Genera (SetObserverSnapshot/V1 u.a.) | LEDGER:2771: SA treibt 9 reale Organe + per-Achse-ABI-POD; Set/Sequence/Adapter/View treiben je NUR 1 Organ, Observer = flacher Hand-POD |
| Cross-Genus | `anatomy/pruefling_merge.hpp:1-16` (PrueflingSlot, 3 kompositionale Joins — PRT-ART-Naht; `MergeStrategy` `:130-158`); **gemeinsames `OrganConcept` = 0 Treffer** (grep 03.08.: nur unverwandte `EytzingerTraversalOrganConcept` in `axes/lookup/composable/`); `organ_concept.hpp` existiert NICHT | Cross-Genus-JOIN type-unmoeglich (static_asserts in allen Adaptern); Komposition-als-Sub-Organ UNGEBAUT |
| Tests (Basis) | `tests/unit/test_genus_binding.cpp`, `tests/unit/test_abi_adapter_g3_batch2.cpp` | Regression-Basis des Fensters (Waisen-TU-Falle beachten) |

### 1.6 Fehlstellen-Bilanz = der E-24-Restumfang

1. `OrganConcept` (gemeinsame Op-Schnittstelle + ObservableAxis/statistics) fehlt vollstaendig.
2. Cross-Genus-Komposition-als-Sub-Organ ungebaut (`node_type` <- Sequence/Adapter, `index_organization` <- Set, `queuing` <- Adapter; LEDGER:2771).
3. Produktionstiefe der 4 Container-Genera: reale Organ-Member + observe-Verdrahtung + `XxxObserverAggregate<N>` (per-Achse) statt flacher Hand-PODs.
4. Docks Set/Sequence/Adapter/View: IPruefDock-Implementierung + DLL-Pfad + Konformitaets-Gate + Registry-Registrierung (Registrierungs-MUSTER existiert seit f15_compare:220, aber nur fuer SA).
5. `for_each_abi_adapter` in den Container-Permutation-Engines; Adapter-Engine-Frage klaeren (OP-3).
6. `XxxExecutionContext` je Genus.
7. F1b-Ebene-1-Promotion (Genus->Typ-ABI, S12.3, Owner-MAJOR) + comdare::container-Kopf-Framework (LEDGER:2858).
8. F2 native Set-ABI-V2 (extract/merge/set-ops, append-only; V1 existiert).
9. **NEU (R-3):** Fehlerklassen-Anbindung der NEUEN Container-Docks/Genus-Fehlerpfade an die A15-Familie (`axis_error.hpp`) — Abschnitt 2.8.
10. Gate-/Ledger-Verankerung: E-24 fehlte "in jeder Gate-Reihenfolge" (KATALOG G.1 = KATALOG:80) — seit 00-SYNTHESE:226 als GATE 4 verankert; LEDGER-A11-Zeile noch offen (OP-10).

### 1.7 NEU v2 — Welle-2d-Ist-Abgleich (0d7a0d92): E-24-Anker-Flaeche UNBERUEHRT, neue Pflicht-Naehte

Die 2d-Landung besteht aus zwei Merges (git show --stat, 03.08.):
- `43a1b65a` (CX-W5/W6 Flag-Grammatik): `axis_variant_version_table.hpp`, `registry_to_axis_levels.hpp`, `measurement/algo_semver.hpp`, `planner/planner_version.hpp`, + Tests `test_planner_version_flag_grammatik.cpp`, `test_reflect_versions_all_registered.cpp` ("Wachen ueber volle Organ-Population").
- `0d7a0d92` (CX-W1/W2/W3/W4/W8 + Z-01/GA-02 Transport/Bestandslog): `artifact_cache.hpp`, `bestandslog/{builder_registration,planer_driven_build,reservation_lifecycle}.hpp`, `build_orchestrator.hpp`, `cache_engine_builder_iterator.hpp` (+278, TP1-Planer-Filter-Iterator), + Tests.

**Befund:** KEINE der E-24-Anker-Dateien (anatomy/, abi/, pruef_dock/, genus_binding_traits.hpp, loader) wurde beruehrt — alle datei:zeile-Anker dieser Fassung gelten unveraendert. ABER zwei neue Pflicht-Naehte fuer das Fenster:
- (a) **Flag-Grammatik-Naht:** neue Genus-Versionseintraege des Fensters muessen die Q3-Grammatik (`vX.Y.Z` + HW-Flag `c`/`ce`, Kurzform verboten; gerenderte Stempel-Form praefixfrei `@X.Y.Zc` per Q10) erfuellen UND in den 2d-Wachen landen (`algo_semver.hpp`-Parser, `planner_version.hpp`, `test_reflect_versions_all_registered.cpp` prueft volle Organ-Population) — Auflage 6.
- (b) **TP1-/Transport-Naht:** die TP1-Proben-Neu-Inventarisierung nach dem Anker-Vollzug (Abschnitt 2.6/3) faehrt ueber den 2d-gehaerteten Bestandslog-/Transport-Pfad (Z-01/GA-02: kein trunc-Verlust ungesicherter Alt-Staende) — keine E-24-Codeaenderung, aber Ablauf-Referenz fuer G6.

### 1.8 NEU v2 — OP-1 aufgeloest: #37-Scheduling-CT-Ist (live gecheckt 03.08.)

- `ce libs/cache_engine/include/cache_engine/measurement/scheduling_system_axis.hpp:1-8`: "Scheduling als CEB-Konfig-System-Achse (Bau-INC-1c, #37) ... compile-time-CRTP-System-Achse ... kein Runtime-Switch, keine vtable"; Kopfblock O-8 Schritt 6: scheduling ist KEINE System-HAUPT-Achse mehr, sondern **UNTER-Achse des target_isa-Komplex-Wrappers**, mit Abgangs-Wache in `abi/system_axis_order.hpp`.
- `ce libs/cache_engine/include/cache_engine/concepts/scheduling_strategy.hpp:3-13`: die historische Runtime-vtable (`ISchedulingStrategy`) ist DEPRECATED, 0 Konsumenten; "ihr Ersatz ist der koordinierte Bau-INC-2".
- **Konsequenz:** der LEDGER:136/:1679-Fenster-Bestandteil "#37-Scheduling" ist ueber die O-8-/ORG-18-Strecke im CT-Sinn BEREITS GELANDET; im E-24-Fenster verbleibt NUR (i) die Bestaetigung im Fenster-Bauplan, dass kein ABI-sichtbarer Scheduling-Rest offen ist, und (ii) die Entfernung des DEPRECATED-vtable-Rests im Abschluss-Aufraeumpass (Kandidaten-Liste, NICHT im Fenster). Kein stiller Doppelbau.

---

## 2. Soll-Schnitt E-24 — EIN S12-Fenster, kein stiller Split

### 2.1 Autoritaets-Kette

- Owner-Wortlaut ANKER:115-118 (letzter ABI-Schritt, eigene ultracode-Planung, Vervollstaendigung SA+Container, "gigantische Erweiterung ueber parallele Achsensysteme").
- Owner-R7 02.08. (`super/docs/sessions/20260802-OWNER-entscheide-...md:19`): E-24 JETZT PFLICHT; "Der Rest nach der Abgabe auch PFLICHT" — "Rest" = nicht-gelistete E-Punkte (Review-Klaerung, BAUPLAN:26 bindend).
- Owner-GO ~17:4x (`OWNER-Doc:74-76`): Q4-Q9-Defaults freigegeben, darunter **Q5 = "E-24 im EINEN ABI-Fenster, Graph-Gattung nach Abgabe"**.
- LEDGER:2771 (Abschn.-52-B2 BAUPLAN: OrganConcept + Komposition + je-Genus-Bauliste), :2784 (S12.1 -> S12.2 -> S12.3), :2828 ("5 Genera ... = LETZTER ABI-Schritt (S12, user-gated ABI-Fenster)"), :2858 (offene-Punkte-Zeile inkl. Kopf-Framework + Option A/B), :1576-1581 (F1b = MAJOR, UNVERHANDELBAR), :136/:1679 (das EINE koordinierte ABI-Fenster: F1b+F2+#37).
- HY-D2 :280-283/:413: E-24 MUSS VOR dem Trigger liegen; die Hybrid-Stufe fordert KEINEN eigenen ABI-Schritt und erbt den finalen Major.
- 00-SYNTHESE:209/:226/:228: kritischer Pfad OS-U -> A13 -> W10 -> **E-24 (GATE 4)** -> A2 (GATE 5, einmalige Eichung NACH A13/E-24). ABGLEICH 03.08.:27/:49-51 fuehrt E-24 weiter OFFEN als kritische ABI-Kante VOR Trigger.

### 2.2 Fenster-Inhalt (verbindlich, EIN Fenster)

**S12.1 — OrganConcept** (neuer Header `ce libs/cache_engine/anatomy/organ_concept.hpp`): gemeinsame Op-Schnittstelle + ObservableAxis-/statistics-Forwarding (Memory-Kanon: Observable-Wrapper MUSS Concept-Member forwarden), CRTP+Concept-Muster, CT-statisch/zero-cost/dispatch-frei. SAs gehaltener Organ-Slot nimmt generisch ein beliebiges Genus-Organ auf.

**S12.2 — je Genus auf Produktionstiefe** (Set 13 / Sequence 9 / Adapter 11 / View 5, Schema-treu nach SearchAlgorithm; Haupt=Struktur/Identitaet gehalten, Unter=Verhalten/Ressourcen delegiert, LEDGER:2771):
(a) Anatomie haelt Haupt-Achsen als reale Organ-Member + observe-Verdrahtung; (b) `XxxObserverAggregate<N>` per-Achse ersetzt flachen Hand-POD (Wire-PODs append-only bzw. neue V2-Layouts NUR im Fenster); (c) `for_each_abi_adapter` in den Permutation-Engines (+ Adapter-Engine-Klaerung OP-3); (d) ABI-Adapter + Docks produktionsreif: IPruefDock-Implementierung, DLL-Pfad ueber denselben Loader, je Genus benanntes Konformitaets-Orakel (V5-Vertrag `pruef_dock.hpp:74-79`), Registry-Registrierung (Registrierungs-Muster nach f15_compare:220 ausweiten + stale Kommentar `pruef_dock_registry.hpp:4-5` nachziehen); (e) `XxxExecutionContext` je Genus; (f) **NEU (R-3): FK-Anbindung je neuem Dock/Genus-Fehlerpfad (Abschnitt 2.8)**.

**S12.3 — Genus->Typ-ABI (F1b-Promotion + Kopf-Framework):** Ebene-1-Gattung ABI-sichtbar, Ziel-Topologie als BENANNTER Fenster-Entscheid (Option A: Genera bleiben, Gattung wird ABI-Flaeche; Option B: Set/Sequence als Typen unter Container — `container_framework.hpp:17-19`, LEDGER:2858); comdare::container-Kopf-Framework (Kopf-Framework-Doktrin je Ebene-1-Gattung). Owner-fixiert: das ist der MAJOR (LEDGER:1576-1581).

**F2 — native Set-ABI-V2** append-only (extract/merge/set-ops) auf ISetTier/SetObserverSnapshotV1 (LEDGER:1580).

**#37-Scheduling-CT:** per Ist-Pruefung 03.08. BEREITS GELANDET (Abschnitt 1.8); im Fenster nur Rest-Bestaetigung, kein Neubau.

**Cross-Genus-Komposition-als-Sub-Organ (IM Fenster, Review-MAJOR):** `node_type` <- Sequence/Adapter, `index_organization` <- Set, `queuing` <- Adapter — via OrganConcept; Cross-Genus-JOIN bleibt type-unmoeglich (static_asserts bleiben bestehen). KEIN Verschieben "nach Abgabe" — das waere der verbotene stille Split (BAUPLAN:26).

### 2.3 NICHT im Fenster

- **Graph-Gattung:** nach Abgabe, eigener Owner-Entscheid (Q5-Default, per Owner-GO ~17:4x freigegeben; Einspruch jederzeit moeglich). Der `AnatomyGattung::Graph`-Enumerator (`anatomy_base.hpp:43`) bleibt UNANGETASTET (kein Append, kein Reorder). Aufloesung des E.10-Scheinwiderspruchs: "LETZTER ABI-Schritt" ist auf die Mess-Kampagne gescoped (vom Fenster bis zur Abgabe KEIN ABI-Touch); Graph nach Abgabe ist ein NEUES, eigenes user-gated Fenster ausserhalb dieses Scopes.
- **Stempel-Semantik:** E-24 fuehrt KEINEN zweiten Stempel-Bruch ein ("Input A13"-Default, BAUPLAN:26); merge-Zeilen-/Meta-Meta-/Klammer-Themen sind A13-Territorium (Owner-E2 + Q1/Q2-Nachtraege). E-24 liest die A13-Welt nur.
- **Hybrid-Bau:** Design festgeschrieben (HY-D2), Bau = Auswertungsphase; die Hybrid-Stufe erbt den neuen Major automatisch.
- **A8-Host-Scheiben (S2/S6):** host-/doku-seitig, beruehren keine Tier-Binary-einkompilierten Dateien — NICHT Teil der E-24-Kopplung (Abschnitt 2.7).
- **TABUs:** `kCompositionAxisNames`/18-Slot-SA-Pfad; persistence_target-Disjunktheit (Q-8, `genus_binding_traits.hpp:37-40`); golden_fullpilot_320-Byte-Wache; `permutation_axes.xml`; `m3v2_study.profile.xml`; CRC64-Anker `0x56F1B721C72DC10E`; Anatomie-Enum-Reihenfolge. GenusBindingTraits ist Wachen-Klasse — Beruehrung NUR im koordinierten Fenster mit GO (genau dieses Fenster IST das GO-Fenster, aber nur fuer die im Fenster-Bauplan gelisteten Aenderungen).

### 2.4 M0-Vorstufe (ABI-neutral, sofort baubar, VOR dem Fenster)

`organ_concept.hpp` + NUR beweisbare static_asserts (SA erfuellt OrganConcept; ContainerType-Genera erfuellen die statistics/value_type-Teilmenge). Luecken der 4 duennen Genera als Dossier-/Kommentar-Abschnitt, NICHT als brechender Assert (Review-MEDIUM-Korrektur v1). Byte-/golden-/ABI-Neutralitaet literal beweisen (Registry-sha identisch, ctest N unveraendert mit Namens-Diff, golden-320-Wache gruen).

### 2.5 Reihenfolge der Planung

Dossier VOR Code (Owner-Auflage "eigene ultracode-Planung", ANKER:115). Dieses P1-Dossier ist die Verankerung; der FENSTER-BAUPLAN (Detail-Schnitt je Genus, Commit-Zug, Orakel-Definitionen, FK-ID-Vergabe, Migrationsliste) ist eine eigene ultracode-Planungssession unmittelbar vor dem Bau, mit dem gelandeten A13-M3-Layout als fixem Input.

### 2.6 Identitaets-/Neuanker-Bilanz (zwei verschiedene Identitaets-Ebenen — nicht vermengen)

- **A13-M3(+W10)** verschiebt die STEMPEL-Ebene: POD 136->120 (v6, merge-Zeile faellt per Owner-E2), Fingerprint-Preimage, v1.0.0->v1.0.0c-Migration (Q3-Buendelung), Zellwerte. B3-Frozen-Neueinfrieren in EINEM Commit ist M3-Auflage.
- **A13-M4** bumpt den CEB-Contract-Minor 0->1 = ERSTER `+ceb=`-Shift (7.0 -> 7.1). Owner-bestaetigte Position: direkt nach M3 (Nachtrag ~21:2x "wie empfohlen ... 2d landen -> M3 -> M4 -> OD-10-RT"; ABGLEICH 03.08.:49-51).
- **E-24** verschiebt die ABI-/LAGER-KEY-Ebene: Major 7->8 (Magic .A7.->.A8.), und via `anatomy_module_abi_v1_decl.hpp:283` (`kCebContractVersion` koppelt an `COMDARE_ANATOMY_ABI_MAJOR`) automatisch das `+ceb=`-Suffix ALLER build_version-Strings = ZWEITER Shift (7.1 -> 8.1). Die SA-Stempel-Zeilen und das A13-Layout bleiben unberuehrt (Auflage 6).
- **A8 (binary-beruehrende Scheiben)** verschiebt die SHA512-FINGERPRINT-Ebene der Tier-Binaries (Abschnitt 2.7) — dritte Identitaets-Ebene, von v1 uebersehen (Review-SCHWER).
- **golden-neutral im Katalog-Sinn:** binary_id = reiner 18-Organ-Pfad der SA-Gattung — E-24 aendert weder Katalog noch CRC64-Anker `0x56F1B721C72DC10E` noch die golden-320-Byte-Wache. Neuanker betrifft NUR ABI-Freezes (additives `_abi7`-Einfrieren analog `_abi4/5/6`), Loader-Referenzen, `+ceb=`-Suffix-Erwartungen und die TP1-Lager-Proben-Inventur. Gemaess 00-SYNTHESE:252 (Nr. 3) ist die golden-Neutralitaets-/Bruch-Bilanz bei E-24 SAUBER AUSZUWEISEN (nicht nur behaupten).
- **Konsequenz fuer die Sequenz:** Der teure Anker-Vollzug (TP1-Proben-Neu-Inventarisierung + A2-SHA512-Eichung) darf nur EINMAL laufen — NACH der E-24-Landung UND nach allen binary-beruehrenden A8-Scheiben. Vorher dagegen sind `+ceb=`-Shifts billig: vor Voll-Bau-4 existiert kein schuetzenswerter Voll-Bestand (Analogie zur Q3-Migrations-Begruendung, OWNER-Doc:68); nur die TP1-Proben sind betroffen, und deren Inventur laeuft ohnehin erst am Fenster-Ende. Das entschaerft die v1-Sorge "zwei +ceb=-Shifts" wesentlich — Kern der V3-Aktualisierung (Abschnitt 3).

### 2.7 NEU v2 — Die A8<->E-24-Kante (Review-SCHWER, befunde[3]; beidseitig zu verankern)

**Faktenlage (A8-Dossier Abschn. 3-c + 4):** `abi_adapter.hpp` und die `axes/`-Header (54 Dateien mit std-Container-Default-Allokator-Bestand, A8-B-5) sind in JEDES Tier-Binary einkompiliert. Jede A8-Scheibe, die sie beruehrt — **S1** (T17-Verlust-Fix, `abi_adapter.hpp:861/:1687` u.a.), **S3** (E1/Micro-Neuschnitt in `fill_segment_timing_v3`), **S4** (Konstitutiv-Matrix), **S5** (OS-Call-/std-Container-Scrub der 54 axes-Header) — dreht die SHA512-Fingerprints; das F7-Skip-Gate baut neu. A8-S3 kann zudem additive Sub-Interfaces nach L-74c-Muster einfuehren (A8-Auflage 1: POD-Bedarf -> STOPP + Eskalation ins E-24-Fenster).

**Kollisionen ohne Kopplung:**
1. Landet eine binary-beruehrende A8-Scheibe NACH dem E-24-Anker-Vollzug (TP1-Inventur + A2-Eichung), entsteht ein weiteres faktisches Neuanker-/Invalidierungs-Ereignis — die Ein-Anker-Bilanz der Sequenz-Empfehlung waere falsch (genau der Review-Befund gegen v1-Option-C).
2. A8-S3-Sub-Interfaces nach dem Fenster kollidieren mit Auflage 10/G8 (nach E-24-Landung KEIN ABI-/POD-Touch bis zur Abgabe). A8 regelte bisher nur den POD-Fall (in E-24 falten), nicht den Sub-Interface-ZEITPUNKT.
3. Die ABGLEICH-03.08.-Reihung (:50) fuehrt "A8/A9" NACH der A2-Eichung — fuer die binary-beruehrenden A8-Scheiben ist das mit 1. unvereinbar und MUSS in V3 korrigiert werden.

**Bindende Kanten-Regel (beidseitig, ins A8-Dossier gespiegelt — dortige Neufassung traegt sie ebenfalls):**
- (K-a) ALLE binary-beruehrenden A8-Scheiben (S1, S3, S4, S5) liegen VOR dem E-24-Anker-Vollzug. S1 (Defekt-Fix) sofort; S3/S4/S5 vor oder parallel zum E-24-Fenster in eigenen, einzeln reviewten Scheiben (A8-Gate-Grundkadenz gilt je Scheibe).
- (K-b) A8-S3-Sub-Interface-Bedarf (L-74c-additiv) und JEDER A8-POD-Bedarf werden VOR/IM E-24-Fenster vollzogen — nach Fenster-Ende greift die G8-Sperre ausnahmslos.
- (K-c) A8-S2/S6 (host-/doku-seitig, keine Tier-Binary-Dateien) sind von der Kante frei und duerfen nach dem Anker-Vollzug laufen.
- (K-d) Der E-24-Fenster-Abschluss-Commit (G8-Negativ-Liste) fuehrt explizit auf, welche A8-Reste noch offen sind und dass sie unter (K-c) fallen — sonst FEHLER.
- (K-e) Die V3-Manager-Vorlage enthaelt den A8-Slot als eigenen Entscheid-Punkt (Anhang V3, Punkt 3) — ohne ihn ist V3 entscheidungs-unvollstaendig (Review-Wortlaut).

### 2.8 NEU v2 — Fehlerklassen-Abstimmung (Review befunde[2]; Owner-E6/A15 "Fehlerklassen bitte jetzt mit bauen")

**Ist:** `ce libs/cache_engine/include/cache_engine/measurement/axis_error.hpp` traegt die A15-Taxonomie (Kopf `:2-14`: "Fehlerklassen und Behandlung sind fuer ALLE Achsen -> Unterachsen -> Algorithmen Pflicht"; `CompilerCompilerErrorClass` `:39`, `SampleStatus` `:58`, `AdmissionStatus` `:79`, `BuildCellStatus` `:100`, `InfraErrorClass` `:174`, `HardwareProbeErrorClass` `:206`; stabile Etiketten via `error_class_label` `:109`). Die Dock-Ebene kennt heute NUR die rohen `dock_status_*`-Ints (`pruef_dock.hpp:36-48`), ohne Anbindung an diese Familie. ABGLEICH 03.08.:26: FK-0/1/2 FERTIG, FK-5 faehrt im M3-Fenster mit.

**Auflage (bindend, = Auflage 16):** Das E-24-Fenster definiert im Fenster-Bauplan je NEUEM Dock/Genus die Fehlerklassen-IDs und bindet sie an die `axis_error.hpp`-Familie an:
- (FK-a) je Container-Dock werden die Fehlerpfade (`wrong_genus`, `subinterface_missing`, `conformance_failed`, Lade-/Treiber-Fehler) auf die D2-Seite abgebildet (CSV-Zelle "failed" + Log, NIE stille 0/null — Kanon) und mit stabilen Etiketten der Familie versehen; die `dock_status_*`-Ints bleiben als ABI-nahe Transportform bestehen (kein Vertrags-Bruch am V5-Freeze), die KLASSIFIZIERUNG erfolgt CEB-seitig.
- (FK-b) neue Genus-Baupfade (Permutation-Engines, ExecutionContexts) melden Bau-Fehler in die D1-Seite (`CompilerCompilerErrorClass`/`BuildCellStatus`) — deklariert im Log, Experiment misst weiter.
- (FK-c) Erweiterungen der Enums sind ADDITIV am Ende (kein Enum-Bruch, OD-5-Muster), Count-Wachen (`kCompilerCompilerErrorClassCount` u.a.) ziehen im selben Commit nach.
- (FK-d) G3 wird um den FK-Beweis erweitert (Abschnitt 5).
- Abstimmungspflicht mit dem A15-Strang (FK-5 laeuft im M3-Fenster): FK-ID-Vergabe im E-24-Fenster-Bauplan GEGEN den dann gelandeten FK-Stand, kein Parallel-Erfinden.

---

## 3. SEQUENZ-EMPFEHLUNG V3 (AKTUALISIERT v2) — E-24 relativ zu A13-M3/M4 und A8

**Entscheid-Typ:** Manager-Entscheid (GATE 4, 00-SYNTHESE:226: "Sequenz relativ zu M3 als Manager-Entscheid, HY-D2-Merksatz bindend"), owner-sichtbar per V7.6-Verbose-Vorlage (Anhang V3). **Prozess-Stand:** ZWISCHENSTAND 20260802:15/:140-141 — Design-Nachtraege waren undone; Task #22 = Nachtraege einarbeiten, Dossier als Lead committen, DANN V3/V3b versenden. Diese Neufassung ist der Einarbeitungs-Schritt.

### 3.1 Veraenderte Entscheidungslage seit v1 (ehrlich ausgewiesen)

1. **M4-Position ist inzwischen owner-beruehrt:** v1-Option-C wollte M4 (CEB-Contract-Minor 0->1) ANS ENDE hinter E-24 verschieben (ein +ceb=-Shift statt zwei). Seitdem hat der Owner die empfohlene Reihenfolge `2d landen -> M3 -> M4 -> OD-10-RT` bestaetigt (OWNER-Doc Nachtrag ~21:2x "Bitte wie empfohlen weiter"), und der ABGLEICH 03.08.:49-51 fuehrt sie als bindend. Die M4-Verschiebung ist damit KEINE freie Manager-Option mehr, sondern eine explizite Abweichungs-Frage in V3 (OP-6 umgewidmet).
2. **Der +ceb=-Doppel-Shift ist billiger als in v1 bewertet:** vor Voll-Bau-4 existiert kein schuetzenswerter Voll-Bestand (Q3-Migrations-Praezedenz, OWNER-Doc:68); betroffen sind nur TP1-Proben, deren Inventur ohnehin ERST NACH E-24 laeuft. Der teure Ein-Anker-Kern (TP1-Inventur + A2-Eichung EINMAL, zuletzt) ist in der owner-bestaetigten Kette bereits gewahrt (A2 "EINMALIG ZULETZT", ABGLEICH:50).
3. **Die A8-Kante ist neu und hart (Abschnitt 2.7):** binary-beruehrende A8-Scheiben MUESSEN vor den Anker-Vollzug; die ABGLEICH-Reihung "A8 nach A2" ist insoweit zu korrigieren.

### 3.2 Optionen

- **Option A — Voll-Fusion:** E-24 in das A13-M3-Fenster falten (ein Commit-Zug M3+W10+E-24+M4, ein Review-Durchgang, ein Anker-Vollzug). Unveraendert abgelehnt: Mammut-Diff ueber zwei Identitaets-Ebenen, Bisektion praktisch unmoeglich, M3-Auflagen-Set vermischt; M3 ist L (Codex-Klasse), E-24 ehrlich L/XL.
- **Option B — zwei getrennte Fenster mit JE eigenem Anker-Vollzug:** M3(+W10)+M4 inkl. TP1-Inventur+Eichung, danach E-24 mit ZWEITEM Voll-Anker. Unveraendert abgelehnt: zwei teure Neuanker-Ereignisse, verletzt die Ein-Fenster-Doktrin (00-SYNTHESE:256).
- **Option C (v1) — gekoppelte Serie, M4 hinter E-24:** zwei getrennte Bau-Pakete, EIN Anker-Vollzug, EIN +ceb=-Shift (7.0 -> 8.1). Vorteil: formal sauberste Shift-Bilanz. Nachteil NEU: weicht von der owner-bestaetigten M3->M4-Reihenfolge ab (benannte Abweichung, braucht explizites GO) und verzoegert die M4-Voll-Verifikation, auf der OD-10-RT aufsetzt.
- **Option C' (EMPFEHLUNG v2) — gekoppelte Serie IN der owner-bestaetigten Kette, EIN Anker-Vollzug, A8-Slot:**
  `2d gelandet (IST) -> Q-M3-GATE -> A13-M3(+W10, +FK-5) -> M4 (+ceb= 7.0->7.1) -> OD-10-RT -> A8-S1 (sofortiger Defekt-Fix, jederzeit frueher moeglich) -> E-24-M0 + Fenster-Bauplan (ultracode) -> E-24-FENSTER (S12.1-S12.3 + F2 + FK-Anbindung; Major 7->8, +ceb= 7.1->8.1) -> A8-S3/S4/S5 (binary-beruehrend; soweit nicht schon parallel VOR dem Fenster gelandet) -> EIN ANKER-VOLLZUG: TP1-Proben-Neu-Inventarisierung + A2-SHA512-Eichung (GATE 5) -> danach NUR noch fingerprint-neutrale Arbeit (A8-S2/S6, A9, Beweise, 12-Perm, J-Kette) -> Trigger.`
  Zwischen E-24-Landung und Anker-Vollzug: KEIN Trigger, KEIN Lager-Voll-Einlagerungslauf, keine schweren Parallel-Jobs. B3-Frozen-Neueinfrieren bleibt M3-Auflage (Stempel-Ebene, von E-24 unberuehrt).

### 3.3 Risiko-Matrix (aktualisiert)

| # | Risiko / Kriterium | A: Voll-Fusion | B: zwei Voll-Anker | C (v1): M4 hinter E-24 | C' (Empf.): owner-Kette + A8-Slot |
|---|---|---|---|---|---|
| 1 | Zahl TEURER Neuanker-Ereignisse (TP1-Inventur + A2-Eichung) | 1 | 2 (verletzt Doktrin) | 1 | **1** (zuletzt, nach E-24 UND A8-binary-Scheiben) |
| 2 | Zahl `+ceb=`-Shifts | 1 | 2 | 1 | 2 — akzeptiert, da vor Voll-Bau-4 kein schuetzenswerter Bestand (3.1-2) |
| 3 | Review-/Bisektions-Qualitaet | SCHLECHT (Mammut-Diff) | gut | gut | gut (getrennte Pakete, je Dual-Review) |
| 4 | Schichten-Reinheit (M3=Stempel / E-24=ABI / A8=Fingerprint, je eigene Wachen-Klasse) | verletzt | erfuellt | erfuellt | erfuellt |
| 5 | Owner-Konformitaet der Reihenfolge (Nachtrag ~21:2x M3->M4) | n/a | erfuellt | **Abweichung, braucht GO** | **erfuellt** |
| 6 | Trigger-Termin-Kopplung | HOCH | mittel | mittel | mittel-niedrig (M3/M4 landen unabhaengig frueh; E-24+A8 = kritischer Pfad) |
| 7 | Zwischenzustand (Baum "zwischen den Ankern") | keins | keins | begrenzt | begrenzt — mitigiert durch stehende Verbote (3.2-C') |
| 8 | Arbeitsteilung v2.1 (M3 UND E-24 je Codex-Klasse) | ein Riesen-Auftrag | zwei | zwei | zwei, pipelinebar (E-24-Fenster-Bauplan parallel zur M3-Landung) |
| 9 | **NEU: A8-Fingerprint-Kante** (binary-beruehrende Scheiben nach Anker = verstecktes 2. Neuanker-Ereignis) | ungeregelt | ungeregelt | ungeregelt (Review-Befund gegen v1) | **geregelt** (K-a..K-e, Abschnitt 2.7) |

### 3.4 Empfehlung und Begruendung

**Option C'.** Sie erhaelt alle Qualitaets-Argumente der v1-Option-C (getrennte, je einzeln dual-reviewte Pakete; getrennte Wachen-Klassen; Bisektion; geringste Termin-Kopplung), respektiert die inzwischen owner-bestaetigte M3->M4-Reihenfolge (keine Abweichungs-Buerokratie), und schliesst als einzige die A8-Fingerprint-Luecke (Risiko-Zeile 9 = der Review-SCHWER-Befund). Der Mehrpreis gegenueber C (v1) ist genau EIN zusaetzlicher billiger `+ceb=`-Shift vor dem Voll-Bau — verifizierbar folgenlos, weil die TP1-Inventur erst nach dem letzten Shift laeuft. Wer dennoch die Ein-Shift-Bilanz will, waehlt in V3 die Beibehaltungs-Alternative C (v1) MIT explizitem Owner-GO fuer die M4-Verschiebung.

**Zeitachsen-Wirkung (ehrlich):** Der BAUPLAN terminierte nur das A16-P1-DOSSIER (Mi 06., BAUPLAN:39) — ein Bau-Slot fuer das E-24-FENSTER fehlt weiterhin (OP-7), und die ABGLEICH-Reihung stellt E-24 hinter OD-10-RT/A1-Rest. E-24 (L/XL) + A8-S3/S4/S5 VOR dem Anker-Vollzug ist substanzieller Umfang; der Trigger-Termin Do 07.08. steht nur mit aggressiver Parallelisierung disjunkter Pakete (Parallelisierungs-Freigabe Owner 03.08.: bis zu 8 Aufgaben) — sonst Reserve Fr 08.08. Per Owner-R9 ("ALLES genannte wird voll gebaut") ist Scope-Reduktion KEINE Option; nur die Reserve ist der Puffer. Der Fenster-Bauplan (ultracode) liefert die belastbare Slot-Schaetzung.

**Default:** Option C' gilt, wenn kein Einspruch erfolgt (Vorlage V3 unten). Umkehrbarkeit: bis M3-Landung frei; nach M4-Landung nur noch C' vs. B; nach E-24-Landung gegenstandslos.

---

## 4. Auflagen-Liste fuer den Bau-Worker (bindend)

1. **Dossier vor Code:** Fenster-Bauplan als eigene ultracode-Planungssession (Owner-Auflage ANKER:115); dieses Dossier + Ledger + alle Plandocs sind Pflicht-Input; A13-M3-Ist-Layout als fixer Input ("Input A13"-Default).
2. **EIN S12-Fenster:** Cross-Genus-Komposition-als-Sub-Organ und ALLE S12.1-S12.3-Teile bleiben im Fenster; jeder Verschiebe-Wunsch = Owner-Entscheid, NIE stiller Split (Review-MAJOR).
3. **Graph-TABU:** `AnatomyGattung::Graph` und alles Graph-bezogene unangetastet (Q5-Default: nach Abgabe, eigener Entscheid). Kein AnatomyGenus-Append, kein Enum-Reorder.
4. **Owner-fixierter MAJOR:** F1b = MAJOR (LEDGER:1576-1581, UNVERHANDELBAR) — Major 7->8, Magic .A7.->.A8., alle bestehenden Freezes ADDITIV erhalten (`_abi7`-Einfrieren analog `_abi4/5/6`); Loader-Major-Check-Beweis literal.
5. **ABI-Handwerks-Doktrin:** neue Faehigkeiten NIE als vtable-Anhang an IAnatomyBase oder bestehende Snapshots — neues Sub-Interface + 1x kalter dynamic_cast je Modul; Wire-PODs append-only, wo Bestands-POD weiterlebt (Set V1->V2); flacher POD je neuem Dock (`pruef_dock.hpp:13-14`); Vererbungsreihenfolge bestehender Adapter EINGEFROREN.
6. **Kein zweiter Stempel-Bruch + Flag-Grammatik-Naht (v2 erweitert):** SA-Stempel-Layout (A13-v6), Klammer-Grammatik (Q1), Flag-Grammatik unberuehrt; neue Genus-Versionseintraege folgen der Q3-Grammatik (`vX.Y.Z`+`c`/`ce`, Kurzform verboten; Code-Literal mit `v`-Praefix, gerenderte Stempel-Form praefixfrei `@X.Y.Zc` per Q10) und registrieren sich in den 2d-Wachen (`algo_semver.hpp`, `planner_version.hpp`, `axis_variant_version_table.hpp`; `test_reflect_versions_all_registered.cpp` MUSS die neuen Eintraege mitpruefen); merge-Zeile existiert nicht (Owner-E2) — E-24 liest die A13-Welt nur.
7. **TABUs (Wiederholung, hart):** `kCompositionAxisNames`/18-Slot-SA-Pfad; persistence_target-Disjunktheit Q-8 (`genus_binding_traits.hpp:37-40`); golden-320-Fixtures + Byte-Wache; `permutation_axes.xml`; `m3v2_study.profile.xml`; CRC64-Anker `0x56F1B721C72DC10E`; conformance-Oracle-Bestand nur ERWEITERN.
8. **Docks:** je Container-Genus IPruefDock-Implementierung mit accepts() ueber Modul-Gattung, DLL-Pfad ueber DENSELBEN Loader, Konformitaets-Gate-VERTRAG (`pruef_dock.hpp:74-79`) erfuellt — je Genus ein BENANNTES Referenz-Orakel definieren (std::map-Huelle ist SA-spezifisch; Kandidaten: std::set/std::deque bzw. std::stack/std::queue/std::span-Klassen — Fenster-Bauplan-Entscheid, OP-2); Registry produktiv verdrahten (Muster f15_compare:220 ausweiten: ALLE 5 Docks registrieren, Mischlauf ueber `measure_genus_sequential`) + stale Kommentar `pruef_dock_registry.hpp:4-5` nachziehen.
9. **Hybrid-Freezes:** `IPruefDock::measure`-Vertrag stabil (HY-D2); `heuristik/axis_spline.hpp`, `break_even.hpp`, `measurement_curve_loader.hpp` nicht umbenennen/entfernen; die Hybrid-Stufe erbt den neuen Major automatisch — kein Hybrid-Sonderbau in diesem Fenster; Hybrid-K1 bleibt E-24-neutral (OP-11).
10. **Nach-Fenster-Sperre:** nach E-24-Landung + A8-binary-Scheiben KEIN Paket mehr mit ABI-/POD-/Enum-/Tier-Binary-Fingerprint-Touch bis nach der Abgabe; Negativ-Liste in den Fenster-Abschluss-Commit (Gate G8) — sie fuehrt die erlaubten A8-Reste (S2/S6, host-/doku-only) EXPLIZIT als Ausnahme-Klasse auf (K-d).
11. **Kadenz (voll):** Ist-Verifikation beide Remotes -> fresh-context-Kartierung datei:zeile -> Umsetzung -> Voll-Review -> Mojibake-Grep==0 -> cf22 dry-run --Werror==0 -> Doppellauf ctest MIT N (nie nackte Prozent, N mit Namens-Diff) -> golden-Roundtrip -> granulare Commits -> gitleaks -> beide Remotes -> CI STRIKT GRUEN beide Hosts -> super-Sub-Build Pflicht; ASCII-Selbstcheck aus dem Diff abgeleitet; Codex-Zweitreview VOR dem Fenster-Merge (analog Q-M3-GATE; Fallback = Fable-xhigh-Adversarial MIT Ledger-Vermerk, Owner-Gate-Option b); Dual-Review Fable+Codex je Landung.
12. **Arbeitsteilung v2.1:** Fenster-Bauplan/Review = Fable xhigh; Geruest nach Design = Opus 5; OrganConcept-Verdrahtung, Observer-Aggregat-Umbau, Dock-DLL-Pfade = Codex (schwere Logik; bei defektem Codex-Env dokumentierter Opus-Fallback wie 2d); Doku-/Ledger-Nachzug = Opus 4.8; Manager = Abnahmen, gitleaks, Serialitaet, Owner-Kommunikation. Working-Tree-Freeze nach Paketmeldung; genau EIN Schreiber im ce-Tree.
13. **Tests:** neue TUs IMMER in CMakeLists registrieren (Waisen-TU-Lehre, 8 bekannte Waisen als Warnbeispiel); Fixture-Strings gegen die Gruene-Tests-zementieren-Falle pruefen; `test_genus_binding.cpp` + `test_abi_adapter_g3_batch2.cpp` fortschreiben statt umgehen.
14. **Doku/Ledger:** LEDGER-A11-Nachtrag (E-24-Fenster-Zeile + GATE-4-Verankerung + S12-Verweis + G8-Negativ-Liste, OP-10); stale Kommentare im Fenster nachziehen (`pruef_dock_registry.hpp:4-5` "V42", `observer_aggregate.hpp:2/:79` "17-Achsen", NEU `anatomy_base.hpp:79` "17-Achsen-Anatomie"); Thesis-Nachzug (Gattungs-Docks PL-19/PL-21 werden real) als C-Strecken-Punkt melden, nicht selbst texten (OP-13).
15. **Messdaten/Neuanker:** keine Messdaten loeschen; TP1-Proben-Neu-Inventarisierung als EINMALIGER Vollzug nach Fenster-Ende UND nach den A8-binary-Scheiben (Option C'), ueber den 2d-gehaerteten Bestandslog-Pfad; golden-/Neutralitaets-Bilanz sauber ausweisen (00-SYNTHESE:252).
16. **NEU (R-3) Fehlerklassen-Pflicht:** FK-Anbindung gemaess Abschnitt 2.8 (FK-a..FK-d) — Fehlerklassen-IDs je neuem Dock/Genus im Fenster-Bauplan festlegen, gegen den gelandeten A15-/FK-5-Stand abgestimmt; CSV-Sichtbarkeit "failed" statt stiller Nullen; Enum-Erweiterungen additiv mit Count-Wachen-Nachzug.
17. **NEU (R-1) A8-Kante:** Kanten-Regel K-a..K-e (Abschnitt 2.7) einhalten — keine binary-beruehrende A8-Scheibe nach dem Anker-Vollzug; A8-Sub-Interface-/POD-Bedarf NUR vor/im Fenster; Fenster-Bauplan prueft VOR Baustart den A8-Scheiben-Stand (welche von S1/S3/S4/S5 sind gelandet) und weist den Rest der Vor-Anker-Strecke zu.

---

## 5. Verifikations-Gates (je Gate literale Ausgabe, kein Haken ohne Beleg)

- **G1 (M0-Neutralitaet):** ctest-N vorher==nachher (Namens-Diff leer), golden-320-Byte-Wache gruen, Registry-sha identisch, 0 Byte-Ereignis ausserhalb neuer Header/Tests.
- **G2 (OrganConcept):** static_assert-Matrix 5 Genera x Concept-Teilmengen kompiliert; ObservableAxis-/statistics-Forwarding-Beweis (Wrapper forwardet Concept-Member); Negativ-Probe (Nicht-Organ-Typ scheitert am Concept, literal als Compile-Fehler-Fixture).
- **G3 (Docks/DLL/FK, v2 erweitert):** je Genus ein DLL-Roundtrip-Test: COMDARE_DEFINE_<G>_MODULE-Fixture bauen -> laden (7-Schritt-Validierung) -> messen -> CSV; `wrong_genus`-Negativ-Probe (dock_status 2) und Konformitaets-Gate-Negativ-Probe (dock_status 4) je Genus; `PruefDockRegistry::select_for` ueber alle 5 Gattungen + `measure_genus_sequential`-Mischlauf-Test; **NEU: je Fehlerpfad der FK-Beweis — die klassifizierte Fehlerklasse erscheint mit stabilem Etikett im Log UND als "failed"-Zelle in der CSV (nie 0/null), literal belegt (FK-a/FK-b, Abschnitt 2.8)**.
- **G4 (Cross-Genus):** Kompositions-Beweise `node_type<-Sequence/Adapter`, `index_organization<-Set`, `queuing<-Adapter` als Unit-Tests (SA-Anatomie traegt Genus-Organ via OrganConcept, Observer-Werte real getrieben); JOIN-Unmoeglichkeits-Asserts bestehen unveraendert.
- **G5 (ABI-Fenster):** Major-7-Altmodul wird vom Major-8-Host ABGELEHNT (Loader-Fehler literal); Major-8-Roundtrip gruen; Magic-Wechsel belegt; `_abi7`-Freezes additiv eingefroren; kHost/kCebContract-Werte im `--version`-Ausdruck belegt.
- **G6 (Identitaets-Bilanz):** `+ceb=`-Suffix-Diff dokumentiert (vorher/nachher literal, beide Shifts der C'-Kette); Stempel-Preimage-UNVERAENDERT-Beweis (OverlayHash identisch fuer ein Referenz-SA-Binary ohne Code-Aenderung); CRC64-Anker unveraendert; TP1-Proben-Inventur EINMAL nach Fenster-Ende + A8-binary-Scheiben (2d-gehaerteter Bestandslog-Pfad), Ergebnis-Zaehlung literal; A2-Eichung erst danach (GATE 5).
- **G7 (Voll-Kadenz):** Doppellauf 2x ctest mit N beziffert (Namens-Diff); super-Sub-Build gruen; CI beide Hosts gruen mit Job-Zahl-Referenz; Dual-Review-Protokolle (Fable+Codex bzw. dokumentierter Fallback) abgelegt; gitleaks 0; ASCII-Diff-Selbstcheck-Zeile.
- **G8 (Sequenz-Wache, v2 erweitert):** Fenster-Abschluss-Commit enthaelt die Negativ-Liste "ABI-/Fingerprint-beruehrende Flaechen — bis nach Abgabe gesperrt" (Enums, PODs, Modul-Makros, Loader-Vertrag, Dock-Vertrag, `abi_adapter.hpp`, `axes/`-Header) **PLUS die explizite Ausnahme-Klasse der erlaubten A8-Reste (S2/S6, host-/doku-only) und den Nachweis, dass ALLE binary-beruehrenden A8-Scheiben (S1/S3/S4/S5) VOR dem Anker-Vollzug gelandet sind (Commit-Hashes literal)**; Ledger-Zeile "E-24 vollzogen, letzter ABI-Schritt vor Trigger" gesetzt; danach darf NUR noch A2/Lager/Beweise/12-Perm/J-Kette (und A8-S2/S6, A9) bis zum Trigger laufen.

---

## 6. Offene Punkte (fortgeschrieben v2)

- **OP-1 — AUFGELOEST (03.08.):** #37-Scheduling-CT ist ueber die O-8-/ORG-18-Strecke gelandet (`scheduling_system_axis.hpp` = CT-CRTP-Unter-Achse des target_isa-Komplexes; DEPRECATED-vtable 0 Konsumenten, `scheduling_strategy.hpp:3-13`). Fenster-Bauplan bestaetigt nur den Rest (kein ABI-sichtbarer Scheduling-Punkt offen); vtable-Entfernung = Abschluss-Aufraeumpass-Kandidat. Details 1.8.
- **OP-2:** Konformitaets-Orakel je Container-Genus fehlt — der V5-Vertrag (`pruef_dock.hpp:74-79`) verlangt je Dock eine Orakel-Pruefung vor Messung; das std::map-Gate ist SA-spezifisch. Je Genus ein Referenz-Orakel benennen (Kandidaten: std::set/std::deque/std::stack/std::queue/span-Klasse) — Entscheid im Fenster-Bauplan.
- **OP-3:** Adapter-Permutation-Engine existiert nicht (ls 03.08.: nur search_algorithm/sequence/set/view unter `anatomy/`). Klaeren, ob Adapter ueber `anatomy_permutation_driver.hpp` faehrt oder eine eigene Engine braucht (for_each_abi_adapter-Traeger).
- **OP-4 — TEILBEANTWORTET (R-2):** produktiver `register_dock`-Aufrufer existiert: `apps/f15_compare/main.cpp:220` (nur SA-Dock, Observe-Pfad der COMPARE-App); in libs/+tools/ weiterhin 0. Im Fenster: Registrierungs-Muster auf alle 5 Docks ausweiten + klaeren, ob der Haupt-Messtreiber (jenseits f15_compare) die Registry oder Direkt-Aufrufe nutzt; stale Kommentar `pruef_dock_registry.hpp:4-5` nachziehen.
- **OP-5:** W10-Position bestaetigen — Empfehlung unveraendert: im M3-Fenster mitfahren (ein Stempel-Preimage-Shift).
- **OP-6 — UMGEWIDMET (v2):** M4-Verschiebung hinter E-24 ist nach dem Owner-Nachtrag ~21:2x (M3 -> M4 bestaetigt) KEIN Default mehr, sondern die explizite Alternativ-Frage in V3 (Option C v1 vs. C'). Siehe 3.1-1.
- **OP-7:** Zeitachsen-Slot fuer den E-24-BAU fehlt weiterhin im Bauplan (nur das Dossier war Mi terminiert, BAUPLAN:39); ABGLEICH-Reihung stellt E-24 hinter OD-10-RT/A1-Rest — Slot + belastbare Schaetzung liefert der Fenster-Bauplan; Trigger-Tag-Risiko Richtung Reserve Fr 08.08. benannt (3.4).
- **OP-8:** Q5-Graph-Quittung — Default (nach Abgabe, eigener Owner-Entscheid) per Owner-GO ~17:4x freigegeben; formale NUR-KENNTNIS-Quittung im gesammelten Owner-Vorlagen-Block ausstehend (Vorlage V3b liegt bei).
- **OP-9 (v2 erweitert):** Stale Kommentare im Fenster nachziehen: `pruef_dock_registry.hpp:4-5` ("V42"), `observer_aggregate.hpp:2/:79` ("17-Achsen"), NEU `anatomy_base.hpp:79` ("vollst. 17-Achsen-Anatomie, INC-2d" vs. ORG-18).
- **OP-10:** LEDGER-A11-Nachtrag offen: E-24-GATE-4-Verankerung + S12-Fenster-Zeile + Nach-Fenster-ABI-Sperre (G8-Negativ-Liste inkl. A8-Ausnahme-Klasse) — Lead-Vollzug, Teil der P1-Landung (ZWISCHENSTAND Task #22).
- **OP-11:** Hybrid-K1 (Lager-Identitaet der Hybrid-.so) bleibt offen und ist E-24-NEUTRAL — solange K1 offen ist, darf kein Lager-Schema Hybrid-Knoten annehmen; keine Kopplung ins E-24-Fenster ziehen.
- **OP-12:** Test-Basis fortschreiben: `test_genus_binding.cpp` + `test_abi_adapter_g3_batch2.cpp` im Fenster erweitern statt umgehen; neue TUs registrieren (Waisen-TU-Lehre).
- **OP-13:** Thesis-Nachzug melden (C-Strecke, nicht selbst texten): Gattungs-Docks PL-19/PL-21 werden mit dem Fenster real; Thesis fuehrt sie bisher als nachgelagerte Arbeitspakete (LEDGER-Bookkeeping um :2734).
- **OP-14 — NEU (R-1):** A8-Scheiben-Disposition VOR Fenster-Baustart erheben (welche von S1/S3/S4/S5 sind gelandet; S1 = sofortiger Defekt-Fix mit Vorrang) und die ABGLEICH-Reihung "A8/A9 nach A2" fuer die binary-beruehrenden A8-Teile korrigieren (V3-Punkt 3); die parallele A8-Dossier-Neufassung traegt die Kante spiegelbildlich (K-a..K-e).

---

## 7. Anker-Verzeichnis (Kurzform, alle live 03.08. am Stand 0d7a0d92)

ce-Ist: `anatomy_base.hpp:40-44/:76-77/:78-84/:79/:100-109` · `genus_binding_traits.hpp:31-32/:37-45/:56-58/:66-70/:166-171` · `container_framework.hpp:17-25/:46/:85-95` · `abi/anatomy_module_abi_v1_decl.hpp:59/:62-63/:66/:264/:270-283` · `abi/{anatomy,set,adapter,sequence,view}_module_abi_v1.hpp` · `anatomy/{abi_adapter.hpp:39-52 (2262 Z.), set_abi_adapter.hpp:18-21, sequence_/adapter_/view_abi_adapter.hpp}` · `anatomy/{set,sequence,adapter,view}_tier.hpp` · `anatomy/observable_tier.hpp:185/:193` · `anatomy/observer_aggregate.hpp:2/:79` · `anatomy/pruefling_merge.hpp:1-16/:130-158` · `builder/pruef_dock/pruef_dock.hpp:10-14/:36-48/:57-79` · `search_algorithm_dock.hpp:26/:65-86` · `{set,adapter,sequence,view}_dock.hpp` (in-process) · `pruef_dock_registry.hpp:4-5/:22-40` · `pruef_dock_sequencer.hpp:44-47` · `search_algorithm_permutation_engine.hpp` (for_each_abi_adapter) · `anatomy_module_loader.hpp:70-75/:149/:158-159` · `apps/f15_compare/main.cpp:220` · `measurement/axis_error.hpp:2-14/:39/:53/:58/:79/:100/:109/:174/:206` · `measurement/scheduling_system_axis.hpp:1-8` · `concepts/scheduling_strategy.hpp:3-13` · 2d: `measurement/algo_semver.hpp`, `planner/planner_version.hpp`, `experiment_tree/axis_variant_version_table.hpp`, `tests/unit/test_reflect_versions_all_registered.cpp` · `tests/unit/test_genus_binding.cpp` · `tests/unit/test_abi_adapter_g3_batch2.cpp`.
Plan/Owner: ANKER:115-118 · OWNER-Doc:19/:52-58 (Q1/Q2)/:60-69 (Q3)/:74-76 (~17:4x Q4-Q9)/:88-92 (Q10)/:134-138 (~21:2x M3->M4)/:190-199 (03.08. Parallelisierung) · LEDGER:136/:516/:1576-1581/:1679/:2769-2771/:2784/:2828/:2858 · KATALOG:32/:80/:111 · BAUPLAN:26/:39/:51 · 00-SYNTHESE:143/:146/:209/:226/:228/:252/:256 · HY-D2:280-283/:413 · ABGLEICH 03.08.:26-27/:49-51 · ZWISCHENSTAND 20260802:15/:140-141 · Welle-3-Backup-JSON (`result.designs.review.befunde[1..3]`, A8-Dossier-Key) · A8-DOSSIER Abschn. 3-c/4/5.

---

## 8. Ziel-Ort im Repo (Lead-Vollzug)

**Empfehlung:** `ce docs/architecture/20260803-e24_container_gattungs_abi_dossier.md` — Praezedenz HY-D2 (`ce docs/architecture/20260802-hybrid_tier_stufe_soll_design.md`): ABI-/Architektur-Design lebt im ce-Repo neben dem Code, den es bindet; die Sequenz-/Prozess-Anteile (Abschnitt 3, V3) verweisen auf super-Plandocs, ersetzen sie nicht. Lead committet als Teil der P1-Landung (ZWISCHENSTAND Task #22: "Nachtraege einarbeiten, Dossier als Lead committen, DANN Owner-Vorlage V3/V3b versenden"); LEDGER-A11-Zeile (OP-10) im selben Zug; danach V3/V3b-Versand.

---

## ANHANG: entscheidungs_vorlagen (v2)

### VORLAGE V3 (v2) — E-24-Fenster-Sequenz relativ zu A13-M3/M4 + A8-Kopplung (Manager-Entscheid mit Owner-Sichtbarkeit; Default gilt ohne Einspruch)

KONTEXT: E-24 (Container-Gattungs-Vollausbau) ist der LETZTE ABI-Schritt und MUSS vor dem Voll-Bau-4-Trigger liegen (HY-D2:280-283/:413 — ein Major-Bump NACH dem Voll-Bau wuerde alle eingelagerten 2^17-Binaries fuer die Hybrid-Stufe entwerten, Major-Check des Loaders `anatomy_module_loader.hpp:149`). Der Major selbst ist Owner-fixiert (F1b = MAJOR, UNVERHANDELBAR, LEDGER:1576-1581). Du hast die Reihenfolge `2d -> M3 -> M4 -> OD-10-RT` am 02.08. (~21:2x, "wie empfohlen") bestaetigt. Offen sind DREI Sequenz-Punkte:

PUNKT 1 — E-24-FENSTER-POSITION: E-24 als EIGENES Bau-Paket nach OD-10-RT, mit EINEM teuren Anker-Vollzug (TP1-Proben-Neu-Inventarisierung + A2-SHA512-Eichung) NACH der E-24-Landung (Option C', Empfehlung). Alternativen: Voll-Fusion mit M3 (A, abgelehnt: Mammut-Diff) oder zweiter Voll-Anker (B, abgelehnt: doppelte Inventur/Eichung).

PUNKT 2 — M4-POSITION: Deine bestaetigte Kette laesst M4 direkt nach M3 (= zwei billige `+ceb=`-Shifts: 7.0->7.1 bei M4, 7.1->8.1 bei E-24 — vor Voll-Bau-4 existiert kein schuetzenswerter Voll-Bestand, nur TP1-Proben, deren Inventur ohnehin erst nach E-24 laeuft). Die Alternativ-Idee aus der v1-Planung (M4 ans Fenster-ENDE hinter E-24 = genau EIN Shift 7.0->8.1) waere eine BENANNTE Abweichung von deiner ~21:2x-Bestaetigung. EMPFEHLUNG: M4 nach M3 belassen (C'), Doppel-Shift akzeptieren.

PUNKT 3 — A8-KOPPLUNG (NEU, Review-SCHWER): Die A8-Scheiben S1/S3/S4/S5 (Benchmarking-Schnitt) beruehren `abi_adapter.hpp` + 54 `axes/`-Header — einkompiliert in JEDES Tier-Binary, jede solche Scheibe dreht die SHA512-Fingerprints. Landet eine davon NACH dem Anker-Vollzug, ist die A2-Eichung entwertet (verstecktes zweites Neuanker-Ereignis); A8-S3-Sub-Interfaces nach dem Fenster kollidieren mit der Nach-Fenster-ABI-Sperre (G8). REGEL (bindend, beidseitig in beiden Dossiers verankert): alle binary-beruehrenden A8-Scheiben VOR dem Anker-Vollzug (S1 sofort; S3/S4/S5 vor/parallel zum E-24-Fenster); A8-Sub-Interface-/POD-Bedarf NUR vor/im Fenster; host-/doku-seitige A8-Reste (S2/S6) bleiben frei. Die bisherige Abgleich-Reihung "A8/A9 nach A2-Eichung" wird fuer die binary-beruehrenden A8-Teile entsprechend korrigiert.

ZEITWIRKUNG (ehrlich): Ein Bau-Slot fuer das E-24-FENSTER fehlt bisher in jeder Zeitachse (nur das Dossier war terminiert). E-24 (L/XL) + A8-S3/S4/S5 vor dem Anker-Vollzug ist substanzieller Umfang; Do 07.08. haelt nur mit aggressiver Parallelisierung disjunkter Pakete (deine 8er-Freigabe vom 03.08.), sonst Reserve Fr 08.08. Scope-Reduktion ist per R9 keine Option.

EMPFEHLUNG: Option C' (alle drei Punkte wie oben). DEFAULT: C' gilt ab sofort, wenn kein Einspruch erfolgt. UMKEHRBARKEIT: Punkt 2 bis zur M4-Landung frei; Punkt 1/3 bis zum Anker-Vollzug frei; danach gegenstandslos.

### VORLAGE V3b — NUR ZUR KENNTNIS (kein Arbeitsauftrag, Defaults gelten; Einspruch jederzeit moeglich)

(1) GRAPH-GATTUNG (Q5-Rest): Das E-24-Fenster laesst die Graph-Gattung vollstaendig unberuehrt (`AnatomyGattung::Graph` bleibt reserviert, kein Enum-Touch). Graph wird nach der Abgabe als EIGENES, user-gated ABI-Fenster mit eigenem Owner-Entscheid geplant — gemaess Q5-Default, den du mit "volles Go fuer ALLE geplante offene Arbeit wie vorgegeben" (02.08. ~17:4x) freigegeben hast. Der scheinbare Widerspruch zu "letzter ABI-Schritt" ist aufgeloest: "letzter" ist auf die Mess-Kampagne gescoped (vom Fenster bis zur Abgabe kein ABI-Touch); ein Nach-Abgabe-Fenster liegt ausserhalb dieses Scopes.

(2) MAJOR-VOLLZUG: Das E-24-Fenster vollzieht den von dir am 16.07. fixierten MAJOR (F1b "UNVERHANDELBAR", LEDGER:1576-1581) als Anatomy-ABI 7->8 (Magic .A7.->.A8.), mit additivem Einfrieren aller Alt-Freezes (`_abi7` analog `_abi4/5/6`) und literalem Loader-Ablehnungs-Beweis fuer Alt-Major-Module. Die SA-Stempel-Welt (A13) wird dabei NICHT erneut gebrochen; golden-Katalog und CRC64-Anker bleiben unveraendert; verschoben wird ausschliesslich die ABI-/Lager-Key-Ebene (`+ceb=`-Suffix). Die golden-/Neutralitaets-Bilanz wird im Fenster sauber ausgewiesen (00-SYNTHESE:252).

(3) CROSS-GENUS-KOMPOSITION: bleibt vollstaendig IM Fenster (`node_type`<-Sequence/Adapter, `index_organization`<-Set, `queuing`<-Adapter via OrganConcept) — kein Teil wird "nach Abgabe" verschoben; "Rest" in deinem R7 wird als "nicht-gelistete E-Punkte" gelesen (Review-Klaerung 02.08.), nicht als E-24-Teile.

(4) FEHLERKLASSEN (Owner-E6-Vollzug im Fenster): Die neuen Container-Docks/Genus-Fehlerpfade werden an dein Fehlerklassen-Framework (#29/A15, `axis_error.hpp`) angebunden — Fehler erscheinen klassifiziert im Log und als "failed"-Zelle in der CSV, nie als stille Nullen. Enum-Erweiterungen additiv, Count-Wachen ziehen nach.

*Ende der Neufassung v2. Read-only erstellt (Scratchpad); Vollzug (Commit unter `ce docs/architecture/`, LEDGER-A11-Zeile, V3/V3b-Versand, Fenster-Bauplan-Session) liegt beim Lead/Manager.*
