# axis_09_isa — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** E (Engineering / Standards-Spec) — alle 4 Wrapper sind Hardware-ISA-Profile, die auf Vendor-/Standards-Spezifikationen verweisen, keine Algorithmus-Paper. Kein is_original-faehiger OSS-Code, daher kein Original-Code-Linking moeglich.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking: alle 4 ISA-Profile (Aarch64Isa, Amd64Isa, PowerPcIsa, RiscVIsa) referenzieren ausschliesslich Vendor-/Standards-Spezifikationen ohne zugehoerigen permissiv lizenzierten OSS-Algorithmus-Code (`C/C++-Code = nein`, `Lizenz = none`). Sie sind Build-Time-Charakterisierungen der Ziel-Instruktionssatz-Architektur, keine Algorithmus-Implementierungen — entsprechend bewusste Standards-/Baseline-Eintraege.

## §2 Wrapper → Paper → Code
| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| Aarch64Isa | AArch64 / ARMv8-A 64-bit ISA (A64) | ARM Architecture Reference Manual (DDI 0487) — Vendor-Spec | ARM Ltd., ab 2011/2013 | developer.arm.com/documentation/ddi0487/latest | nein | none | ✗ |
| Amd64Isa | x86-64 / AMD64 / Intel 64 ISA | AMD64 Programmer's Manual + Intel 64/IA-32 SDM — Vendor-Spec | AMD 2000 / Intel fortlaufend | docs.amd.com … ; intel.com/.../intel-sdm | nein | none | ✗ |
| PowerPcIsa | Power ISA (POWER9/10, ppc64le) | Power ISA Specification (v3.0/3.1) — Vendor/Standards-Spec | IBM/OpenPOWER; v3.0 2017, v3.1 2020 | openpowerfoundation.org/specifications/isa/ | nein | none | ✗ |
| RiscVIsa | RISC-V RV64GC Open-Source ISA | The RISC-V Instruction Set Manual — Standards-Spec (UCB TR Ursprung) | RISC-V Intl.; UCB TR ab 2011 | github.com/riscv/riscv-isa-manual | nein | none | ✗ |

## §3 Compliance-Status
Alle 4 Wrapper haben eine Spec-Referenz (Vendor- bzw. Standards-Spezifikation) und sind als ISA-Profile/Standards gekennzeichnet → Habich-Pflicht erfuellt. Die Map (§5) klassifiziert alle ISA-Profile explizit als bewusste **Baselines/Standards** (Vergleichs-/Charakterisierungs-Nullpunkte) ohne Klaerungsbedarf, confidence durchgehend `high`.

- is_original-Kandidaten (Map §3): **keine** — kein Wrapper dieser Achse ist `is_original_eligible = true`.
- Lizenz-blockiert: **keine** (kein OSS-Code referenziert; alle `Lizenz = none`).
- Korrekturen (Map §4): **keine** — fuer axis_09_isa sind in §4 keine Wrapper-Header-Korrekturen gelistet.

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md): keine P-ID fuer axis_09_isa — alle ISA-/SIMD-/HW-Profile (axis_09/09b/12) sind ohne P-ID (Map §6).
