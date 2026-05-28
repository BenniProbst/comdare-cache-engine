# axis_12_general_hardware — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** E (Engineering / reine HW-Plattform-Profile mit Build-Time-Konstanten — keine Algorithmen, keine Paper, keine OSS-Codes; bewusste Vergleichs-Baselines).

## §1 Pflicht-Note
Diese Achse enthaelt KEINE Wrapper mit echtem is_original-Linking. Alle 3 Wrapper sind reine Hardware-Plattform-Profile (Build-Time-Konstanten) bzw. ein konservatives Fallback-Profil — keiner basiert auf einem Algorithmus-Paper oder einem permissiv lizenzierten OSS-Code. Es handelt sich durchgaengig um bewusste Engineering-Baselines (Vergleichs-Nullpunkte), nicht um Re-Implementierungen referenzierter Verfahren.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| X86_64HardwareProfile | x86-64 HW-Plattform-Profil (Build-Time-Konstanten) | — | — | — | nein | none | ✗ |
| Aarch64HardwareProfile | AArch64/ARM64 HW-Plattform-Profil (Build-Time-Konstanten) | — | — | — | nein | none | ✗ |
| GenericHardwareProfile | Generisches/konservatives HW-Profil (Fallback-Baseline) | — | — | — | nein | none | ✗ |

## §3 Compliance-Status
Alle 3 Wrapper sind als reine HW-Plattform-Profile / Fallback-Baselines gekennzeichnet (`paper_found = false`, `c_cpp_code_exists = no`, `is_original_eligible = false`, alle confidence = high). Map §5 fuehrt die HW-Profile (axis_09/09b/12) explizit unter den "bewussten Baselines/Lehrbuch-Konzepten — kein Klaerungsbedarf, korrekt als Vergleichs-Nullpunkte gekennzeichnet". Damit ist die Habich-Pflicht erfuellt: jeder Wrapper ist entweder Paper-referenziert ODER als Baseline ausgewiesen — hier durchgaengig Letzteres.

is_original-Kandidaten (Map §3): KEINE fuer diese Achse.
Lizenz-blockierte Codes: KEINE fuer diese Achse.
§4-Korrekturen: KEINE Eintraege betreffen axis_12_general_hardware.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md (§2 axis_12_general_hardware; §5 Baselines; §6 Hinweis: axis_12 ohne P-ID, reine CE-Baselines/Standards).
- Doku 17 §4.5 (Klassifikation): Klasse E (Engineering-Baseline).
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md): KEINE P-ID fuer axis_12 (Map §6 — alle ISA-/SIMD-/HW-Profile ohne P-ID).
