# P22-Chen-Fractal — Re-Implementation-Skelett

**Paper:** Fractal Prefetching B+-Trees
**Autor/Jahr:** Chen et al. 2002
**Kernkonzept:** Fraktale Disk+Cache-Kombination: Disk-Page enthaelt Cache-optimierten B+-Subtree
**Concept-Anbindung:** `Page+PrefetchStrategy` (Achse: Page-Type / Prefetch)

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

1. **Pseudocode-Extraktion** aus Originalpaper (siehe `Forschungsarbeiten/Fractal Prefetching B+-Trees (Chen et al. 2002).pdf`)
2. **Concept-Anbindung:** Implementiert das `Page+PrefetchStrategy`-Concept
   (siehe `search_engine/page/`)
3. **C++23-Implementation** mit modernem Constexpr / Concepts / Modules
4. **Adapter** an die Bausteine-Matrix-Achse "Page-Type / Prefetch"
5. **Code-Review** mit Habich vor Integration

## Vorgesehene Dateien (zu implementieren)

```
P22-Chen-Fractal/
├── README.md                      (dieses Dokument)
├── CMakeLists.txt                 (Build-Stub)
├── include/
│   └── fractal_prefetch_bplus.hpp             (Concept-Anbindung)
├── src/
│   └── fractal_prefetch_bplus.cpp             (Implementation)
└── tests/
    └── fractal_prefetch_bplus_test.cpp        (Google-Tests)
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
