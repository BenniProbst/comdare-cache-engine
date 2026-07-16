# algorithm_profiles/ — Persistierte Suchalgorithmus-Konfigurationen (REV 7.6 V8.3)

**Anlass:** User-Direktive 2026-05-13/14 (`STRUCTURAL_CORRECTION_diplomarbeit.md` §2.2):
> *"Ich vermisse in der CacheEngine einen Ordner, in dem alle Suchalgorithmen
> mit ihrer Gesamtkonfiguration als XML/json persistiert sind, sodass die in
> der CacheEngine einzeln persistierten Algorithmusbestandteile durch die
> Permutationsbeschreibung wiederherstellbar sind."*

---

## Layout (IST-Stand, F34-Nachzieh 2026-07-16)

> KORREKTUR F34 (WP-4, Voll-Audit 2026-07-16): der darunter stehende Original-Baum
> von REV 7.6 V8.3 ist ein LEGACY-STAND (bewusst nicht geloescht, Doku-Doktrin). Er
> weicht in drei Punkten vom Ist ab: (1) `_schema.xsd` wurde nie materialisiert — die
> XSD-Wahrheit des comdare_experiment-Pfads liegt im SUPER-Repo unter
> `Code/test_data_xml/experiment_schema.xsd` (INC-C 2026-07-14; die Validierung selbst
> ist der C++-Validator `profile_facade/validate_profile.hpp`, kein xmllint-Gate);
> (2) `permutation_axes.xml` ist der historische 11-Achsen-Katalog (page/node/
> traversal/…) und TABU-read-only — der KANONISCHE Achsen-Stand sind die 19
> Kompositions-Achsen T00–T18, maschinenlesbar in der GENERIERTEN
> `cache_engine_axis_registry.xml` (tools/axis_registry_gen, INC-A; Round-Trip-Gate
> test_axis_registry_roundtrip, F29); (3) der Ordner traegt heute weit mehr als sota/.

```
algorithm_profiles/
├── README.md                        (dieses Dokument)
├── permutation_axes.xml             (LEGACY: historischer 11-Achsen-Katalog; TABU-read-only,
│                                     NICHT informations-redundant zur Registry-XML)
├── cache_engine_axis_registry.xml   (GENERIERT, INC-A: 19 Achsen T00-T18 / 90 Bausteine der
│                                     Enabled*-Reflektion; NICHT von Hand editieren;
│                                     Drift-Gate: ctest test_axis_registry_roundtrip)
├── sota/                            (34 SOTA-/Paper-Profile, P01-P30-Welle)
├── thesis_profiles/                 (E4-Studien-Profile inkl. m3v2_study.profile.xml
│                                     [golden-320-Quelle] + SCHEMA.md)
├── load_profiles/                   (Achse-2-Lastprofile: ycsb_a..f, lp_*, coco_p04_*, ih/lh
│                                     + SCHEMA.md)
└── allocators/                      (23 Allokator-Profile + README.md)
```

Das Schema der prt-art-Registry (`prt_art_axis_registry.xml`) ist identisch; sie lebt
im prt-art-Repo (`prt_art/algorithm_profiles/`, Generator `prt_art/registry_gen`).

---

## Layout (LEGACY-Stand REV 7.6 V8.3 — historisch, s. Korrektur oben)

```
algorithm_profiles/
├── README.md                        (dieses Dokument)
├── _schema.xsd                      (XSD-Validierung, optional)
├── permutation_axes.xml             (definiert die 11 Achsen + Wertebereiche)
└── sota/
    ├── art.profile.xml              (P01 Leis 2013)
    ├── hot.profile.xml              (P02 Binna 2018)
    ├── masstree.profile.xml         (P03 Mao 2012)
    ├── coco_trie.profile.xml        (P04 Boffa 2024)
    ├── start.profile.xml            (P05 Fent 2020)
    ├── b2tree.profile.xml           (P06 Schmeisser 2022)
    ├── wormhole.profile.xml         (P07 Wu 2019)
    └── surf.profile.xml             (P10 Zhang 2018)
```

Diese Liste wird waehrend der V8-Phase iterativ um weitere Profile erweitert
(P11-P30 Tier-2/Tier-3-Algorithmen).

---

## Profil-Format (XML)

Jedes Profil beschreibt einen **vollstaendigen** SOTA-Algorithmus mit allen
11 Achsen und der zugehoerigen Allokator/Concurrency/Telemetry-Konfiguration:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<comdare_algorithm_profile id="art" paper_ref="P01">
  <metadata>
    <name>Adaptive Radix Tree</name>
    <authors>Leis, Kemper, Neumann</authors>
    <year>2013</year>
    <venue>ICDE</venue>
  </metadata>

  <axes>
    <!-- Axis 1: Page-Type -->
    <page>DENSEBYTE_ART256</page>
    <!-- Axis 2: Node-Type -->
    <node>SPARSE_NODE4_ART</node>
    <!-- Axis 3: Traversal -->
    <traversal>STANDARD</traversal>
    <!-- Axis 4: Value-Handle -->
    <value_handle>EXTERNAL</value_handle>
    <!-- Axis 5: Concurrency -->
    <concurrency>OPTIMISTIC_LOCK_COUPLING</concurrency>
    <!-- Axis 6: Allocator -->
    <allocator>MIMALLOC</allocator>
    <!-- Axis 7: Prefetch -->
    <prefetch>NONE</prefetch>
    <!-- Axis 8: Telemetry -->
    <telemetry>OFF</telemetry>
    <!-- Axis 9: ISA -->
    <isa>X86_AVX2</isa>
    <!-- Axis 10: Layout -->
    <layout>STANDARD_HEAP</layout>
    <!-- Axis 11: Reclamation -->
    <reclamation>NONE</reclamation>
  </axes>

  <key_value_signature>
    <key_types>std::uint64_t, std::array&lt;char, 8&gt;</key_types>
    <value_types>std::string</value_types>
  </key_value_signature>
</comdare_algorithm_profile>
```

---

## Verwendung im CacheEngineBuilder

Der `xml_config_parser` liest ein Profil-Verzeichnis (z.B. `sota/`) und
generiert pro Profil eine Permutation. Im `defined`-Mode der Messreihe A
werden nur **referenzierte** Profile aktiviert; im `full`-Mode werden alle
Achsen-Permutationen aus `permutation_axes.xml` generiert (Habich-Direktive:
*"Messung muss immer so vollstaendig wie moeglich sein"*).

---

## Pruefling-Profile (separat in prt-art Repo)

PRT-ART als Beitrittspruefling hat sein eigenes algorithm_profiles/ unter
`comdare-prt-art/prt_art/algorithm_profiles/prtart_pruefling.profile.xml`.
Bei Beitritt zu SOTA wandert das Profil nach `cache-engine/algorithm_profiles/sota/`.

---

## Querverweis

- `STRUCTURAL_CORRECTION_diplomarbeit.md` §2.2 (User-Direktive)
- `Diplomarbeit/20260508 Termin 7/Phase5_UML_Detail/24_architektur_skizze_REV7_2026_05_13.md` §5.2 (XML-Format)
- `xml_config_parser/xml_config_parser.{hpp,cpp}` (Parser-Implementation)
- `docs/sessions/20260514-0900-v8-cache-engine-strukturkorrekturen.md` V8.3
