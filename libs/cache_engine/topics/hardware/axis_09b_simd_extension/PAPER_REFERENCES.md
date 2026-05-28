# axis_09b_simd_extension — Paper-References
**Stand:** 2026-05-28
**Quelle:** docs/architecture/18_achsen_algorithmus_paper_code_map.md (autoritative Map, web-verifiziert) — DIESE Datei ist ein Achsen-Auszug.
**Klasse (Doku 17 §4.5):** E (Engineering / Vendor-Spec + Standards-Spec) — kein Wrapper ist is_original-faehig; alle Wrapper bilden ISA-/SIMD-Vendor-Erweiterungen bzw. eine Scalar-Baseline ab. Keine OSS-Algorithmus-Codes, keine pseudocode-/license-blockierten Algorithmen.

## §1 Pflicht-Note
Kein Wrapper dieser Achse hat echtes is_original-Linking (alle is_original = ✗, `c_cpp_code_exists = no`). Die SIMD-Extensions sind Hardware-/ISA-Erweiterungs-Profile, die gegen autoritative Vendor- bzw. Standards-Spezifikationen (Intel SDM/ISA-Ext-Ref, ARM ARM, RVV-Spec, NVIDIA Whitepaper) referenziert werden — kein wiederverwendeter Original-Paper-Code, sondern reine Hardware-Capability-Abbildungen bzw. (NoSimdExtension) eine Scalar-Baseline.

## §2 Wrapper → Paper → Code

| Wrapper | Algorithmus | Paper (Titel) | Venue/Jahr | DOI/URL | C/C++-Code | Lizenz | is_original |
|---------|-------------|---------------|------------|---------|------------|--------|-------------|
| NoSimdExtension | No SIMD / scalar baseline | — | — | — | nein | none | ✗ |
| Sse2SimdExtension | Intel SSE2 (128-bit, x86_64 ABI-Baseline) | Intel 64/IA-32 SDM / ISA Ext Programming Reference (319433) | Intel Spec, 2000/2003 | cdrdv2-public.intel.com/.../319433-060…pdf | nein | none | ✗ |
| Avx2SimdExtension | Intel AVX2 (256-bit, Haswell+ 2013) | Intel ISA Ext Programming Reference (319433); SDM | Intel Spec, 2013 | cdrdv2-public.intel.com/.../319433-060…pdf | nein | none | ✗ |
| Avx512SimdExtension | Intel AVX-512 (512-bit + 15 Sub-Flags) | Intel ISA Ext Programming Reference (319433-060) | Intel Spec, 2016/2017 | cdrdv2-public.intel.com/.../319433-060…pdf | nein | none | ✗ |
| NeonSimdExtension | ARM NEON / Advanced SIMD (AArch64 baseline, 128-bit) | ARM Architecture Reference Manual (NEON); Cortex-A8 TRM | ARM Spec, 2005 | developer.arm.com/.../intrinsics/ | nein | none | ✗ |
| Sve2SimdExtension | ARM SVE2 (ARMv9 mandatory, scalable 128-2048 bit) | The ARM Scalable Vector Extension (SVE-Fundamentalpaper) | IEEE Micro 37(2) 2017 | 10.1109/MM.2017.35 | nein | none | ✗ |
| RvvSimdExtension | RISC-V Vector Extension (RVV v1.0, scalable) | Vitruvius+ (akad. RVV-Impl); RVV Spec v1.0 (Standard) | ACM TACO 2023; RVV v1.0 2021 | 10.1145/3575861 ; github.com/riscvarchive/riscv-v-spec | nein | CC-BY-4.0 (Spec) | ✗ |
| CudaGh200SimdExtension | NVIDIA CUDA auf Hopper GH200 (massiv-parallel GPU) | NVIDIA Tesla: A Unified Graphics and Computing Architecture; GH200 Whitepaper | IEEE Micro 28(2) 2008; NVIDIA 2023 | 10.1109/MM.2008.31 ; resources.nvidia.com/.../grace-hopper | nein | none (CUDA proprietär) | ✗ |

## §3 Compliance-Status
Alle 8 Wrapper haben eine Paper-/Spec-Referenz ODER sind als Baseline gekennzeichnet → Habich-Pflicht erfuellt:
- **Baseline (kein Paper noetig):** NoSimdExtension (Scalar-Nullpunkt, bewusst paper_found=false).
- **Vendor-/Standards-Spec referenziert:** Sse2/Avx2/Avx512 (Intel SDM / ISA Ext Programming Reference 319433-060), NeonSimdExtension (ARM ARM / Cortex-A8 TRM), Sve2SimdExtension (IEEE Micro 2017 SVE-Fundamentalpaper), RvvSimdExtension (Vitruvius+ TACO 2023 + RVV v1.0 Spec, Spec CC-BY-4.0), CudaGh200SimdExtension (IEEE Micro 2008 Tesla + GH200 Whitepaper).
- **is_original-Kandidaten (Map §3):** keine — diese Achse hat keinen Wrapper mit `is_original_eligible = true`.
- **Lizenz-blockierte Linking-Faelle:** keine echten OSS-Algorithmus-Codes; CUDA ist proprietär (none), RVV-Spec ist CC-BY-4.0 (nur Spezifikation, kein Algorithmus-Code). Alle `c_cpp_code_exists = no`, confidence durchweg high.
- **Korrekturen (Map §4):** keine Eintraege fuer axis_09b_simd_extension.
- **Offene Punkte (Map §5):** keine Eintraege fuer axis_09b_simd_extension; alle SIMD-/HW-Profile sind als bewusste Vendor-Spec-/Baseline-Vergleichspunkte gekennzeichnet (siehe Map §5 Schlussabsatz).

## §4 Cross-Refs
- Autoritative Map: docs/architecture/18_achsen_algorithmus_paper_code_map.md
- Doku 17 §4.5 (Klassifikation)
- Lokaler Katalog Forschungsarbeiten/code/ (REPO_INVENTAR_FINAL.md): keine P-ID fuer axis_09b — laut Map §6 referenzieren alle ISA-/SIMD-/HW-Profile (axis_09/09b/12) externe Vendor-/Standards-Spezifikationen, keine P01–P33-Katalogeintraege.
