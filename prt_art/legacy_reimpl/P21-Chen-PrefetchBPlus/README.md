# P21-Chen-PrefetchBPlus — Re-Implementation-Skelett

**Paper:** Improving Index Performance through Prefetching
**Autor/Jahr:** Chen/Gibbons/Mowry 2001
**Kernkonzept:** Wider Nodes via Software-Prefetch, Leaf-Pointer-Arrays
**Concept-Anbindung:** `PrefetchStrategy` (Achse: Prefetch)

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

1. **Pseudocode-Extraktion** aus Originalpaper (siehe `Forschungsarbeiten/Improving Index Performance through Prefetching.pdf`)
2. **Concept-Anbindung:** Implementiert das `PrefetchStrategy`-Concept
   (siehe `search_engine/memory_layout/`)
3. **C++23-Implementation** mit modernem Constexpr / Concepts / Modules
4. **Adapter** an die Bausteine-Matrix-Achse "Prefetch"
5. **Code-Review** mit Habich vor Integration

## Vorgesehene Dateien (zu implementieren)

```
P21-Chen-PrefetchBPlus/
├── README.md                      (dieses Dokument)
├── CMakeLists.txt                 (Build-Stub)
├── include/
│   └── prefetch_bplus.hpp             (Concept-Anbindung)
├── src/
│   └── prefetch_bplus.cpp             (Implementation)
└── tests/
    └── prefetch_bplus_test.cpp        (Google-Tests)
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
