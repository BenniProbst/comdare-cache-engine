# ÜBERGABE an Implementierungs-Agent (2026-06-25): Code-Rückstand ggü. festgezogenem Kapitel 4

> **Anlass:** Der Text-Agent hat Kapitel 4 der Diplomarbeit („Konzept und Architektur") mit ultracode-Workflows
> gegen **alle drei Codebasen** (cache-engine, prt-art, 02_messung_driver) UND gegen die festgezogenen
> Kapitel 1–3 + Anhang D geerdet.
>
> **LEITPRINZIP (User-Direktive 2026-06-25):** Kapitel 1–3 der Diplomarbeit sind **FESTGEZOGEN = der gewünschte
> Soll-Zustand** (inkl. der Metapher-Verfeinerung: Lebewesen ≡ SearchAlgorithm, Anatomie = Verdrahtung *zwischen*
> den Organen, EINE Architektur). Wo ch4 (auf ch1–3 gegründet) vom Code abweicht, ist **der Code im Rückstand**
> und wird nachgezogen — **die Thesis wird NICHT verwässert.** Doc 36 (`docs/architecture/36_eine_architektur_lebewesen_ist_searchalgorithm.md`)
> ist die Architektur-Wahrheit und markiert offene Defekte (z. B. I1) ausdrücklich als „zu beheben, NICHT in der Thesis zu kanonisieren".
>
> Die ch4-seitig **falschen** Aussagen (Thesis-interne Widersprüche) hat der Text-Agent bereits in der Thesis korrigiert
> (Commit `6f8f292`): Free-List→Allocator, Distance-Estimator→Path-Oriented, Messreihen-XML-Schärfung, „tausende"→Potenzial.
> Dieses Handover betrifft **nur die `code_lag`-Punkte** — der Code muss an die festgezogene Thesis nachgezogen werden.

---

## Verifikations-Caveat
Die Zeilennummern stammen aus den Grounding-Agenten; der Text-Agent hat die **kritischen** Punkte per Direkt-Grep
bestätigt (`resolve_baustein.hpp` existiert, `comdare_create_anatomy` existiert, `sota_catalog.hpp:18-24`-Mapping
verifiziert). **Vor jedem Edit die Zeilennummern gegen den aktuellen Submodul-Stand re-greppen.**

---

## CODE-RÜCKSTAND #1 — I1: EINE Architektur statt zwei Template-Bäume (höchste Priorität)
**Bezug:** Doc 36 §4 / bestehende Übergabe `docs/sessions/20260620-…-EINE-ARCHITEKTUR-vereinheitlichung.md` (TODO-6).

- **Code-Ort:**
  - `libs/cache_engine/include/cache_engine/abi/search_engine.hpp:19-21` — `search_engine<Collection,ConfigPermutation> : public execution_engine<…>` trägt `lookup/insert/erase`, **KEINE 19 Achsen, KEINE Observer-Statistik**.
  - `libs/cache_engine/anatomy/abi_adapter.hpp:119` — `SearchAlgorithmAbiAdapter<A> final : public IAnatomyBase` = die echte Lebewesen-Repräsentation (19 Organe + `observe_all`).
  - `libs/cache_engine/anatomy/search_algorithm_anatomy.hpp:32` — Organe als Member.
- **Ist:** Zwei **unverbundene** Template-Bäume (verifiziert 2026-06-25, Doc 36 §4): `search_engine<>` erbt template-seitig von `execution_engine<>`; `SearchAlgorithmAnatomy/AbiAdapter` erbt runtime-virtuell von `IAnatomyBase→IExecutionEngine`. Kein gemeinsamer Code.
- **Soll (ch1 Z.114–117, ch4 `sec:three-layer`, Doc 36 §0/§2):** GENAU EINE geschlossene Hierarchie über `IExecutionEngine`
  als gemeinsamer Wurzel; die ABI-Such-Sicht läuft ausschließlich über `SearchAlgorithmAbiAdapter<A> : IAnatomyBase → IExecutionEngine`; 19 Achsen-Organe + `observe_all` NICHT nur auf der Anatomie-Seite. KEINE Parallel-Bäume.
- **Auftrag:** `search_engine<>` auf die Lebewesen-Hierarchie überführen bzw. eliminieren, sodass die ABI-Such-Sicht über
  `SearchAlgorithmAbiAdapter` über der Anatomie aufgeht statt als eigener `execution_engine<>`-Ast. Duplizierte Achsen-/Slot-Verwaltung
  und die drei nebeneinanderliegenden Container-Instanzen (Doc 36 §4) entfernen.

## CODE-RÜCKSTAND #2 — ABI-Such-Sicht-Identifier `search_engine` → variadisches `SearchEngine` (Teil von #1)
- **Code-Ort:** `libs/cache_engine/include/cache_engine/abi/search_engine.hpp:19-21` (`template<typename Collection, typename ConfigPermutation> class search_engine`, lowercase, 2 fixe Params). Bereits konform: `prt-art/.../prt_art_search_engine.hpp` (`PrtArtSearchEngine<Ts…>`, CamelCase, variadisch).
- **Ist:** `search_engine` (lowercase snake_case, 2 fixe Typ-Parameter). Doc 36 §2.5 nennt das den „historischen Identifier desselben Dings".
- **Soll:** Code-Name = Thesis-Name: variadisches `SearchEngine` (CamelCase, `template<class… Ts>`) als abstrakte ABI-Laufzeit-Sicht über `SearchAlgorithmAbiAdapter`, passend zur variadischen Hybrid-API (1 Param⇒vector, 2⇒map, N>2⇒map<K,tuple>; ch4 `sec:abi`, ch1 Z.129–132).
- **Auftrag:** Im Zuge von #1 den lowercase-Identifier zur variadischen ABI-Sicht (CamelCase) konsolidieren/umbenennen, sodass Code-Name und Thesis-Name (`SearchEngine`, `class… Ts`) zusammenfallen. `PrtArtSearchEngine<Ts…>` bleibt. **ch4 `fig:abi` (Z.244) NICHT auf den realen lowercase-Namen herabsetzen — der Soll-Name bleibt das variadische `SearchEngine`.**

## CODE-RÜCKSTAND #3 — Stufe→Reihe-Mapping in `sota_catalog.hpp` ans ch1-FF3-Mapping
- **Code-Ort:** `tests/unit/thesis_tiere/sota_catalog.hpp:18-24`; abzugleichen mit `02_messung_driver/main.cpp:108-118` (Enum `A_PrtArtVsSota / B_CacheEnginePerm / C_MergeAltNeu`).
- **Ist:** `sota_catalog.hpp:18-24` mappt flach 1:1: *Reihe A = Stufe1_CeOnly (Lebewesen ISOLIERT / die 6 SOTA selbst)*, *Reihe B = Stufe2_PrueflingReplace*, *Reihe C = Stufe3_FullJoin*. Das macht A zur Isolations-/SOTA-Solo-Reihe (Gegenteil von Prüfling-vs-SOTA), bindet B an Stufe2 statt Stufe3 — und **widerspricht sogar dem eigenen Treiber-Enum** `main.cpp:108-118`.
- **Soll (ch1 FF3 festgezogen + ch4 `tab:stage-series`, Commit `6f8f292`):** **A** = Prüfling vs. Stand der Technik, gespeist aus **Stufe1 UND Stufe2 gemeinsam**; **B** = **Stufe3_FullJoin** (systematische Permutation); **C** = **build-übergreifend** (Merge/Regression alt-vs-neu, an keine einzelne Stufe gebunden).
- **Auftrag:** `sota_catalog.hpp:18-24` so umstellen, dass A=Stufe1+Stufe2, B=Stufe3, C=build-übergreifend — konsistent mit `main.cpp:108-118`. **ch4 NICHT ändern** (ist bereits Soll).
- ⚠️ **VORBEDINGUNG (blockiert die finalen B/C-Code-Labels):** Es gibt eine **Thesis-interne B/C-Drift** zwischen
  **ch1 FF3** (B = systematische Variation, C = Merge/Regression alt-neu) und **ch6 `sec:series`** (B = Cache-Engine-Permutationen/SOTA-Basis ohne Prüfling, C = Merge-Punkte). A ist überall gleich. **Diese Drift muss der Text-Agent/User ZUERST auflösen** (welche Definition ist kanonisch — ch1 FF3, da festgezogen, vs. ch6). Erst dann die B/C-Labels im Code final fixieren.

---

## Was die Thesis (ch4) jetzt sagt — Soll-Referenz für den Impl-Agenten
Kanon (deckungsgleich ch1/ch2/ch3 + Doc 36): EINE Hierarchie `IExecutionEngine → IAnatomyBase (Lebewesen) →
SearchAlgorithm-Unterklasse (≡ Lebewesen) → SearchAlgorithmAnatomy<C> (Körper, 19 Organe) + SearchAlgorithmAbiAdapter<A>
(ABI-Sicht „SearchEngine")`. 3 Gattungen (SearchAlgorithm/Container/Graph), Container-Unterklassen Set/Sequence/Adapter/View.
PRT-ART überschreibt die **Allocator**-Achse (4+2-Pool in Free-List-Sub-Achse AA1) + **Path-Oriented-Prefetch** (eigen);
Distance-Estimator = übernommener ART-Standard. Drei-Stufen-Prüfung → Messreihen A/B/C per obigem Mapping.

**Code-bestätigt (kein Handlungsbedarf):** `comdare_create_anatomy()→IAnatomyBase*` (`anatomy_module_abi_v1_decl.hpp:81`),
`resolve_baustein.hpp` (`libs/cache_engine/include/cache_engine/abi/`), 3 Gattungen (`anatomy_base.hpp:39-51`),
19-Organ-Erzwingung (`composition_factory` static_assert), `IVirusExecutionEngine`-Geschwister (Test `test_v41_execution_engine.cpp:189-190`).
