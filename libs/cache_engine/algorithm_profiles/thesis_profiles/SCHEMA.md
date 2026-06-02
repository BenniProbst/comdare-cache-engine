# `comdare_thesis_profile` — Schema (KF-2, 2026-06-02)

Diplomarbeit-ansteuerbares XML-Profil für den Cache-Line-Konfigurator. Ersetzt die hartcodierten
`codegen.cmake`-Profile (smoke/medium/full). **Additiv** — baut auf den bestehenden
`permutation_axes.xml` (11 Achsen) + `sota/*.profile.xml` (Paper-Originale) auf, ändert KEINE bestehende Datei.

Vollständiger Entwurf + Begründung: `docs/sessions/20260602-cacheline-konfigurator-design-und-hw-recherche.md`.
Beispiel: `cacheline_study.profile.xml` (gleicher Ordner).

## Wurzel
`<comdare_thesis_profile id="..." schema_version="1">`

## Elemente

| Element | Pflicht | Bedeutung |
|---|---|---|
| `<base_tiers>` / `<tier id profile_ref paper_ref/>` | ja | Paper-Originale = **statische Achsen-Tupel**. `profile_ref` → ein `sota/*.profile.xml`. Werden als EINE gekreuzte Dimension behandelt (inverse Auswertungslogik, ein Großexperiment statt 8 Separatläufe). |
| `<permute_axes>` / `<axis ref [per_organ]>` | ja | Die **dynamischen, cache-line-relevanten** Achsen (compile-time). Ohne Kind-`<value>` = volle Liste aus `permutation_axes.xml`; mit `<value>` = Teilmenge. |
| `<axis ref="cacheline" per_organ="…">` | ja (Kernthema) | **NEUE per-Organ Cache-Line-Unterachse** (KF-3). `per_organ` = Leerzeichen-Liste der Organe (page/node/traversal/allocator), die JEWEILS eine **eigene, unabhängige** Cache-Line-Einstellung tragen. Kinder: `<line_size>` (64/128/256), `<alignment>` (none/cacheline_aligned/padded), `<sw_prefetch_hint>` (none/T0/T1/T2/NTA). Pro Organ also bis 3×3×5 = 45 → bis **45ⁿ** für n Organe (beabsichtigt). |
| `<compile_dims>` | ja | Compile-time Mess-Dimensionen (je Binary gebacken): `<workloads>` (YCSB A–F), `<telemetry mode silent>` (§8). |
| `<runtime_dynamic>` | nein | **OS-seitige dynamische Dimensionen**, vom CacheEngineBuilder am Prüf-Dock zur Laufzeit durchlaufen (Faustregel §7): `<thread_count>` (via `Algorithm_Resource_Control`, KF-4), `<hw_prefetcher>` (via MSR 0x1A4, KF-12). |
| `<fixed_conditions .../>` | nein | Fixe reproduzierbare Bedingungen (Launcher, konstant): `turbo/smt/aslr/numa/governor`. |
| `<repetitions count interpolate overlay_in_chart/>` | ja | Wiederholungen (Default 3); `interpolate="false"` = **nie** gemittelt; alle Läufe separat (§9, KF-10). |
| `<modes>` / `<mode name merge active_axes [pruefling replaces_axes]>` | ja | Die 3 Permutationsmodi (`Stufe1_CeOnly`/`Stufe2_PrueflingReplace`/`Stufe3_FullJoin` aus `pruefling_merge.hpp`). `active_axes` = Achsen-Teilmenge, die in diesem Modus permutiert wird (deine 3-Modi-Achsen-Beschränkung). |
| `<static_axes from="base_tier"/>` | ja | Die nicht-permutierten Achsen nehmen die Werte des jeweiligen `base_tier` (Paper-Tupel). |
| `<constraints>` / `<require>` `<pin>` | nein | Cross-Constraints (z.B. `isa host_supported`, algorithmus-gebundene Pins) — kappen unsinnige Kombinationen vor der Enumeration. |
| `<key_value_signature>` | nein | `key_types`/`value_types` (wie `sota/*.profile.xml`). |

## Compile-time vs. Runtime (Faustregel)

- **Compile-time** (eigenes Binary): alle **Architektur-Achsen** inkl. per-Organ-`cacheline`, `workload`, `telemetry`-Modus.
- **Runtime** (CacheEngineBuilder am Prüf-Dock, in Env-/Ressourcengrenzen): OS-seitige, nicht-architektonische Größen —
  `thread_count`, `hw_prefetcher`, Affinität/NUMA. Plus Wiederholungen (Orchestrierung).

## Verarbeitung (Pipeline)

1. **Parser** (KF-1, tinyxml2) liest das Profil → erweitert `CacheEngineConfig` um `permute_axes`, `modes`,
   `runtime_dynamic`, `repetitions`, `cacheline`-Sub-Dim.
2. **Enumeration** (KF-9): kartesisches Produkt der `permute_axes` × `base_tiers`-Tupel, je `<mode>` per `active_axes`
   gefiltert; **Dedup** per FNV1a-Fingerprint (inverse Auswertung KF-15).
3. **CEB-Generator** (KF-8) erzeugt je Unique-`PermutationDescriptor` ein `perm_<id>.cpp` → SHARED-DLL.
4. **CacheEngineBuilder/Prüf-Dock** misst je DLL × `runtime_dynamic` × Wiederholungen (silent-mode Snapshot-Diff).
5. **Inverse Projektion** (KF-15) + **Thesis-Anbindung** (KF-14): Ergebnisse → bilingualer Mess-Anhang.
