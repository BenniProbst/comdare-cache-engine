# Meilenstein: Konfigurator-Fundament KF-2 → KF-1 → KF-4 (2026-06-02)

> `/goal`: Fundament bauen, dann autonom alle machbaren Tasks bis die gebauten DLLs Diplomarbeit-Messwerte
> erzeugen → Diplomarbeit-Kette → LaTeX-Auswertung. Nach jeder Teilaufgabe Planungsschritt mit Bestandsaufnahme,
> nach jedem Meilenstein elaborate Session-Doku. Design-Spec: `20260602-cacheline-konfigurator-design-und-hw-recherche.md`.

## Ausgangszustand (Sync)

Vor Baubeginn alle 3 Repos committet/gepusht/synchron: Superprojekt + cache-engine + prt-art CLEAN, 0/0 mit
Remote, Submodul-Pointer korrekt. Thesis-Build-Artefakte committet, Fremd-Ordner (IDE/Overleaf/ZIH-Template)
gitignored (Commit DA `89c46b4`).

## KF-2 — XML-Schema `comdare_thesis_profile` + Beispielprofil ✅

**Bestandsaufnahme:** `permutation_axes.xml` = 11 Achsen (`<axis id required><values><value>`), `sota/*.profile.xml`
= Paper-Originale (`<comdare_algorithm_profile><axes><page>…</axes>`), 39 Profile (sota + allocators).
`CacheEngineConfig`/`AlgorithmProfile` aus dem Parser.

**Gebaut:** `algorithm_profiles/thesis_profiles/cacheline_study.profile.xml` + `SCHEMA.md` — additiv, referenziert
`../sota/*.profile.xml` + `../permutation_axes.xml`. Enthält base_tiers (8 Paper-Originale), permute_axes inkl.
**per-Organ `cacheline`-Unterachse**, compile_dims, runtime_dynamic, fixed_conditions, repetitions(3, nie interpoliert),
3 modes mit active_axes-Maske, static_axes=base_tier, constraints. KEINE bestehende Datei geändert.

**Verifiziert:** XML wohlgeformt, 8 Lebewesen/8 Achsen/3 Modi, alle 8 `profile_ref` lösen auf, cacheline per_organ=4 /
line_sizes=3 / alignments=3 / sw_hints=5. (Commit ce `6de0b14`.)

## KF-1 — Self-contained XML-DOM-Reader + `parse_thesis_profile` ✅

**Bestandsaufnahme + Entscheidung:** `tinyxml2` ist NICHT vendored; Offline-Build-Restriktion (FortiGate blockt
GitHub, vgl. Boost.MP11-Vendoring) macht einen neuen Vendor teuer. Die bestehende Regex-Heuristik
(`<(\w+)>([^<]+)</\w+>` → Map) kann **wiederholte Kind-Elemente nicht** (`<value>`/`<line_size>` kollabieren).
Der Parser-Header erlaubt ausdrücklich „eigene Implementation" → **Entscheidung: kleiner self-contained XML-DOM**
statt tinyxml2.

**Gebaut:** `xml_reader.hpp` (header-only DOM: Elemente/Attribute/Kinder/Text/Kommentare/self-closing/Entities,
`text_tokens()`). `ThesisProfile`-Structs + `XmlConfigParser::parse_thesis_profile()` (DOM-basiert). Bestehende
`parse()`-Pfade UNVERÄNDERT (rein additiv; Consumer experiment_driver/test_codegen_from_profile unberührt).

**Verifiziert:** Standalone-Test (MSVC `cl /std:c++latest`) **33/33** gegen das echte Profil (DOM-Smoke +
alle Felder inkl. cacheline-per-Organ, 3 Modi, prtart-replaces, repetitions nie-interpoliert). (Commit ce `f993135`.)

## KF-4 — `Algorithm_Resource_Control` (Laufzeit-Steuerschnittstelle) ✅

**Bestandsaufnahme:** Additives ABI-Sub-Interface-Muster (`IObservableTier`/`IScannableTier`): eigenständig, via
`dynamic_cast` abgefragt, flacher uint64-POD mit `static_assert` standard_layout/trivially_copyable, NIE in-place
ändern (SEH-0xc0000005-Lektion). Diese sind an `COMDARE_MEASUREMENT_ON` gebunden. **Schlüssel-Unterschied (User):**
`Algorithm_Resource_Control` muss AUCH bei Messung-aus aktiv sein → **immer einkompiliert**, da Steuer- statt Mess-Ebene.

**Gebaut:** `anatomy/resource_controllable_tier.hpp` — `IResourceControllableTier` (query_caps + apply) + POD
`ComdareResourceControlV1` (per-Achsen-Felder thread_count/prefetch_distance/pool_budget/batch_size/inline_threshold).
`builder/algorithm_resource_control.hpp` — Host-Controller: klammert `desired` auf min(tier-caps, env-limits) und
wendet an (Faustregel §7: in Env-/Ressourcengrenzen). Organ-Verdrahtung = KF-5, Durchlauf = KF-7 (abgegrenzt).

**Verifiziert:** Standalone-Test (cl) **9/9** — Klammerung Env(4→2)/Cap(2000→1000), unsteuerbare Achse→0,
`nullptr`-Degradierung. (Commit ce `7775f28`.)

## Commit-Übersicht

| Schritt | cache-engine | Superprojekt-Bump |
|---|---|---|
| Sync | — | `89c46b4` |
| KF-2 | `6de0b14` | `617000e` |
| KF-1 | `f993135` | `584793d` |
| KF-4 | `7775f28` | `181ab17` |

## Nächster Planungsschritt → KF-3 (per-Organ Cache-Line-Unterachse)

**Bestandsaufnahme (zu tun):** Goldstandard-Achsen-Vorlage `axis_06_allocator` (Concept/CEPS/Subaxes/CRTP-Base/
Wrapper/flags.hpp.in/Registry) studieren; die 4 betroffenen Achsen (page/node/traversal/allocator) + ihre Organe;
wie `flags.hpp.in` per `configure_file` generiert wird. Dann die `cacheline`-Unterachse je Organ als CRTP-Achse anlegen.

**Verbleibende KF-Tasks:** KF-3 (cacheline-Unterachse), KF-5 (alle Organ-Algorithmen erweitern — groß), KF-6 (node
echt verdrahten), KF-8 (CEB-Generator, kein Python), KF-9 (dynamische Enumeration + 3-Modi-Maske + Fingerprint-Map),
KF-7 (Laufzeit-Durchlauf), KF-10 (Wiederholungen), KF-11 (Telemetrie silent-mode), KF-12 (Launcher), KF-13 (ZIH),
KF-14 (Thesis-Anhang), KF-15 (inverse Auswertung). Hauptauftrag: gebaute DLLs → Messwerte → Diplomarbeit-Kette → LaTeX.
