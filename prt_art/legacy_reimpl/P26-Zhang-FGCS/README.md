# P26-Zhang-FGCS — Re-Implementation-Skelett

**Paper:** A prefetching indexing scheme for in-memory database systems
**Autor/Jahr:** Q. Zhang et al. FGCS 2024
**Kernkonzept:** Path-Prefetcher + Jump-Pointer-Prefetcher mit Read-Counter pro Block
**Concept-Anbindung:** `PrefetchStrategy+Telemetry` (Achse: Prefetch / Measurement)

## Status: SKELETT (Phase 4.B Vorbereitung)

Dieses Verzeichnis enthaelt **kein** Originalcode (Habich-Direktive zoll
nur `ext/` einhalten). Stattdessen ist hier eine **PRT-ART-eigene
Implementation** des im Paper beschriebenen Algorithmus geplant.

### Architekt-Direktive (2026-05-08)

> "Wir verwenden ohnehin nur modularisierte Bruchstuecke des Original codes,
> was technisch gesehen neuen Code erzeugt, der zwar die Funktionalitaet,
> aber nicht mehr die Struktur enthaelt, wie der Originalcode. Unser Code
> wird C++23 und damit ist durch die Metaprogrammierung alles neu."

**Konsequenz:** Hier wird in C++23 mit Concepts/`requires` neu implementiert.
Original-Paper ist die KONZEPT-Quelle, nicht die CODE-Quelle.

## Re-Implementations-Plan

1. **Pseudocode-Extraktion** aus Originalpaper (siehe `Forschungsarbeiten/Aprefetching indexingschemeforin-memorydatabasesystems.pdf`)
2. **Concept-Anbindung:** Implementiert das `PrefetchStrategy+Telemetry`-Concept
   (siehe `search_engine/memory_layout/`)
3. **C++23-Implementation** mit modernem Constexpr / Concepts / Modules
4. **Adapter** an die Bausteine-Matrix-Achse "Prefetch / Measurement"
5. **Code-Review** mit Habich vor Integration

## Vorgesehene Dateien (zu implementieren)

```
P26-Zhang-FGCS/
├── README.md                      (dieses Dokument)
├── CMakeLists.txt                 (Build-Stub)
├── include/
│   └── path_jumppointer_prefetch.hpp             (Concept-Anbindung)
├── src/
│   └── path_jumppointer_prefetch.cpp             (Implementation)
└── tests/
    └── path_jumppointer_prefetch_test.cpp        (Google-Tests)
```

## Bausteine-Matrix-Eintrag

In `docs/bausteine/Bausteine_Matrix.txt` ist dieser Baustein als
LEGACY_REIMPL markiert. Im Flag-System (siehe `docs/architecture/Flag_System.txt`)
erhaelt er ein eigenes Bit in der entsprechenden Bank.

## Hinweis zur Authentizitaet

Diese Re-Implementation ist KEIN "Originalcode" im Sinne der Habich-Direktive
(`docs/email/20260508-1800-email_kontakte.md`). Sie ist eine **PRT-ART-eigene Implementation**
des im Paper beschriebenen Algorithmus, basierend auf der Konzept-Beschreibung
im Originalpaper.

Bei Vergleichsmessungen muss darauf hingewiesen werden, dass es sich um
eine Re-Implementation handelt — die Performance-Charakteristik kann von
der originalen Publikation abweichen.
