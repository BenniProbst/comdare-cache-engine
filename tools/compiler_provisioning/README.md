# tools/compiler_provisioning — Compiler-Bereitstellung pro Plattform (F-EXTRA-4)

Bereitstellung der benoetigten Compiler-Versionen pro Plattform fuer:
1. Hauptcompiler (C++23: GCC 14+ / Clang 17+ / MSVC 19.39+)
2. Original-Compiler pro Fremdalgorithmus (siehe pro Paper `STATUS.md`)

## Status: Skelett (Phase 4.B)

## Strategien

### Strategie A — Cross-Build auf x86-Host + Stage-2 native (Empfehlung)
- Cross-Toolchain wird auf x86-Host gebaut (schneller als nativer Bau auf Target)
- Deployment auf Target (Pi 5 / VisionFive 2)
- Stage-2 Native-Build zur Verifikation

### Strategie B — Self-Built nativ auf Target
- Bei Bedarf, falls Cross-Build-Probleme auftreten

### Strategie C — System-Compiler via apt/dnf
- Fuer Workstation-Builds (Linux/macOS) ausreichend

## Verzeichnisstruktur (zu erstellen)

```
tools/compiler_provisioning/
├── x86_64/
│   ├── gcc-14.sh                 (Hauptcompiler)
│   ├── clang-17.sh
│   └── ext-compilers/            (Originalcode-Compiler)
│       ├── gcc-4.7.sh            (P01 ART original)
│       ├── gcc-5.sh              (P02 HOT, P10 SuRF)
│       └── ...
├── arm64/                        (Pi 5 / Apple M1 / Grace Hopper)
│   └── ...
├── riscv64/                      (VisionFive 2)
│   └── ...
└── installed/                    (provisionierte Binaries, gitignored)
```

## Verifikation (Pre-Build-Phase)

Vor jeder Permutations-Build-Phase prueft `compiler_check.sh`:
- Sind alle erforderlichen Original-Compiler verfuegbar?
- Stimmen die Versionen mit den Paper-Anforderungen ueberein?
- Ergebnis: GO/NO-GO fuer Permutations-Build.
