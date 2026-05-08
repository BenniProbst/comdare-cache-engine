"""Generiert Re-Implementations-Skelette fuer LEGACY_REIMPL-Paper unter
prt_art/legacy_reimpl/.

(d) Phase 4.B-detail: 14 Paper, fuer die kein oeffentliches Repo existiert
oder fuer die wir aufgrund Architekt-Direktive eine eigene C++23-Variante
bauen wollen.
"""
from pathlib import Path

ROOT = Path(__file__).parent / "prt_art" / "legacy_reimpl"

# Paper-Definitionen fuer Re-Implementation-Skelette
# Format: (dirname, paper-titel, autor-jahr, kernkonzept, concept-typ, achse, hpp-name)
PAPERS = [
    ("P11-CSS-tree", "Cache-Sensitive Search Trees",
     "Rao/Ross 1999",
     "Pointer-frei, Cache-Line-grosse Knoten, Index-Arithmetik statt Pointers",
     "Page", "Page-Type", "css_node_page"),
    ("P12-CSB-tree", "Making B+-Trees Cache Conscious in Main Memory",
     "Rao/Ross 2000",
     "Sibling-Cluster, Parent haelt 1 Pointer + Offsets",
     "Page", "Page-Type", "csb_node_group_page"),
    ("P13-Hankins", "Effect of Node Size on the Performance of Cache-Conscious B+-Trees",
     "Hankins/Patel 2003",
     "Wider Nodes (4-16 Cache-Lines pro Knoten); I/M/B/T-Cost-Modell",
     "Page", "Page-Type", "wider_bplus_page"),
    ("P14-Samuel", "Making CSB+-Trees Processor Conscious",
     "Samuel/Pedersen/Bonnet 2005",
     "Configuration Table indexed by (key size, value size, update ratio)",
     "Page", "Page-Type", "config_table_bplus_page"),
    ("P16-Bender-TreeLayout", "Tree Layout in Multilevel Memory",
     "Bender/Demaine/Farach-Colton 2002",
     "Probability-driven Layout (Greedy O(N log B)) ueber Access-Verteilung",
     "MemoryLayout", "Memory-Layout", "probability_layout"),
    ("P17-Bender-CacheOblivious", "Cache-Oblivious B-Trees",
     "Bender/Demaine/Farach-Colton 2005",
     "Performance ohne Kenntnis konkreter Cache-Parameter (Gegenpol PRT-ART)",
     "MemoryLayout", "Memory-Layout", "cache_oblivious_layout"),
    ("P18-Saikkonen-MultiLevel", "Multi-Level Block Reorganization",
     "Saikkonen/Soisalon-Soininen 2008",
     "Multi-Level Relocation: BFS-Reorganisation ueber Block-Hierarchie",
     "Allocator+Relocation", "Memory-Layout / Allocator", "multi_level_reloc"),
    ("P19-Saikkonen-LayoutInvariant", "Layout Invariants with Constant-Time Updates",
     "Saikkonen/Soisalon-Soininen 2016",
     "Layout-Invariante mit konstanten Update-Operationen",
     "MemoryLayout", "Memory-Layout", "layout_invariant"),
    ("P21-Chen-PrefetchBPlus", "Improving Index Performance through Prefetching",
     "Chen/Gibbons/Mowry 2001",
     "Wider Nodes via Software-Prefetch, Leaf-Pointer-Arrays",
     "PrefetchStrategy", "Prefetch", "prefetch_bplus"),
    ("P22-Chen-Fractal", "Fractal Prefetching B+-Trees",
     "Chen et al. 2002",
     "Fraktale Disk+Cache-Kombination: Disk-Page enthaelt Cache-optimierten B+-Subtree",
     "Page+PrefetchStrategy", "Page-Type / Prefetch", "fractal_prefetch_bplus"),
    ("P23-Khan-AdaptivePrefetch", "Data Cache Prefetching With Dynamic Adaptation",
     "Khan 2010",
     "Code-Specializer adaptiert Prefetch-Distance zur Laufzeit",
     "PrefetchStrategy", "Prefetch", "adaptive_prefetch_distance"),
    ("P24-NaderanTahan", "Why Does Data Prefetching Not Work for Modern Workloads",
     "Naderan-Tahan/Sarbazi-Azad 2016",
     "Studie/Analyse: Useless Prefetches dominieren bei modernen Workloads",
     "Telemetry", "Measurement", "useless_prefetch_study"),
    ("P26-Zhang-FGCS", "A prefetching indexing scheme for in-memory database systems",
     "Q. Zhang et al. FGCS 2024",
     "Path-Prefetcher + Jump-Pointer-Prefetcher mit Read-Counter pro Block",
     "PrefetchStrategy+Telemetry", "Prefetch / Measurement", "path_jumppointer_prefetch"),
    ("P27-Zhang-ASPLOS-Hierarchical", "Hierarchical Prefetching",
     "T. Zhang et al. ASPLOS 2025",
     "Bundle-Prefetching mit Software/Hardware-Hybrid-Mechanik (Konzept-Quelle, gem5-Hardware-Fork)",
     "PrefetchStrategy", "Prefetch", "hierarchical_bundle_prefetch"),
]


README_TEMPLATE = """# {dirname} — Re-Implementation-Skelett

**Paper:** {paper}
**Autor/Jahr:** {author}
**Kernkonzept:** {concept}
**Concept-Anbindung:** `{cpp_concept}` (Achse: {achse})

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

1. **Pseudocode-Extraktion** aus Originalpaper (siehe `Forschungsarbeiten/{paper_pdf}`)
2. **Concept-Anbindung:** Implementiert das `{cpp_concept}`-Concept
   (siehe `search_engine/{concept_dir}/`)
3. **C++23-Implementation** mit modernem Constexpr / Concepts / Modules
4. **Adapter** an die Bausteine-Matrix-Achse "{achse}"
5. **Code-Review** mit Habich vor Integration

## Vorgesehene Dateien (zu implementieren)

```
{dirname}/
├── README.md                      (dieses Dokument)
├── CMakeLists.txt                 (Build-Stub)
├── include/
│   └── {hpp_name}.hpp             (Concept-Anbindung)
├── src/
│   └── {hpp_name}.cpp             (Implementation)
└── tests/
    └── {hpp_name}_test.cpp        (Google-Tests)
```

## Bausteine-Matrix-Eintrag

In `docs/bausteine/Bausteine_Matrix.txt` ist dieser Baustein als
LEGACY_REIMPL markiert. Im Flag-System (siehe `docs/architecture/Flag_System.txt`)
erhaelt er ein eigenes Bit in der entsprechenden Bank.

## Hinweis zur Authentizitaet

Diese Re-Implementation ist KEIN "Originalcode" im Sinne der Habich-Direktive
(`docs/EMAIL_KONTAKTE.md`). Sie ist eine **PRT-ART-eigene Implementation**
des im Paper beschriebenen Algorithmus, basierend auf der Konzept-Beschreibung
im Originalpaper.

Bei Vergleichsmessungen muss darauf hingewiesen werden, dass es sich um
eine Re-Implementation handelt — die Performance-Charakteristik kann von
der originalen Publikation abweichen.
"""

CMAKELISTS_TEMPLATE = """# {dirname} — Re-Implementation-Skelett (Phase 4.B)

# Skelett — Inhalt kommt in Phase 5+
# add_library(comdare_legacy_{lib_suffix} INTERFACE)
# target_include_directories(comdare_legacy_{lib_suffix} INTERFACE include)
"""

HPP_TEMPLATE = """// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// {dirname} — Re-Implementation in PRT-ART
// Paper: {paper} ({author})
// Concept: {cpp_concept}
//
// PRT-ART-eigene C++23-Re-Implementation. Original-Paper ist Konzept-Quelle.

#pragma once

#include <cstdint>
#include <span>

namespace comdare::prt_art::legacy_reimpl::{namespace} {{

// TODO Phase 5+: C++23-Concepts-Anbindung an `{cpp_concept}`
// TODO Phase 5+: {concept} (siehe README.md fuer Pseudocode-Hinweise)

}}  // namespace comdare::prt_art::legacy_reimpl::{namespace}
"""


def main():
    written = 0
    for entry in PAPERS:
        dirname, paper, author, concept, cpp_concept, achse, hpp_name = entry
        target = ROOT / dirname
        (target / "include").mkdir(parents=True, exist_ok=True)
        (target / "src").mkdir(parents=True, exist_ok=True)
        (target / "tests").mkdir(parents=True, exist_ok=True)

        # README
        concept_dir_map = {
            "Page": "page",
            "Page+PrefetchStrategy": "page",
            "MemoryLayout": "memory_layout",
            "Allocator+Relocation": "memory_layout",
            "PrefetchStrategy": "memory_layout",
            "PrefetchStrategy+Telemetry": "memory_layout",
            "Telemetry": "memory_layout",
        }
        concept_dir = concept_dir_map.get(cpp_concept, "page")
        paper_pdf_map = {
            "P11-CSS-tree": "Cache-Sensitive Memory Layout for Binary Trees.pdf",
            "P12-CSB-tree": "Making B+-Trees Cache Conscious in Main Memory.pdf",
            "P13-Hankins": "Effect of Node Size on the Performance ofCache-Conscious B+-trees.pdf",
            "P14-Samuel": "Making CSB+-Trees Processor Conscious.pdf",
            "P16-Bender-TreeLayout": "Tree Layout in Multilevel Memory (BenderDemaineFarach-Colton 2002).pdf",
            "P17-Bender-CacheOblivious": "CACHE-OBLIVIOUS B-TREES.pdf",
            "P18-Saikkonen-MultiLevel": "Cache-Sensitive Memory Layout for Dynamic Binary Trees.pdf",
            "P19-Saikkonen-LayoutInvariant": "Efficient Tree Layout in a Multilevel Memory Hierarchy.pdf",
            "P21-Chen-PrefetchBPlus": "Improving Index Performance through Prefetching.pdf",
            "P22-Chen-Fractal": "Fractal Prefetching B+-Trees (Chen et al. 2002).pdf",
            "P23-Khan-AdaptivePrefetch": "Data Cache Prefetching With DynamicAdaptation.pdf",
            "P24-NaderanTahan": "WhyDoes Data Prefetching Not Work for Modern Workloads.pdf",
            "P26-Zhang-FGCS": "Aprefetching indexingschemeforin-memorydatabasesystems.pdf",
            "P27-Zhang-ASPLOS-Hierarchical": "ZhangEtalASPLOS2025HierarchicalPrefetching.pdf",
        }
        paper_pdf = paper_pdf_map.get(dirname, "(unbekannt)")

        readme = README_TEMPLATE.format(
            dirname=dirname,
            paper=paper,
            author=author,
            concept=concept,
            cpp_concept=cpp_concept,
            achse=achse,
            hpp_name=hpp_name,
            concept_dir=concept_dir,
            paper_pdf=paper_pdf,
        )
        (target / "README.md").write_text(readme, encoding="utf-8")

        # CMakeLists
        cmakelists = CMAKELISTS_TEMPLATE.format(
            dirname=dirname, lib_suffix=hpp_name,
        )
        (target / "CMakeLists.txt").write_text(cmakelists, encoding="utf-8")

        # HPP-Stub
        namespace = hpp_name.replace("-", "_")
        hpp_content = HPP_TEMPLATE.format(
            dirname=dirname,
            paper=paper,
            author=author,
            concept=concept,
            cpp_concept=cpp_concept,
            namespace=namespace,
        )
        (target / "include" / f"{hpp_name}.hpp").write_text(hpp_content, encoding="utf-8")

        # .gitkeep fuer src/ und tests/
        (target / "src" / ".gitkeep").write_text("", encoding="utf-8")
        (target / "tests" / ".gitkeep").write_text("", encoding="utf-8")

        written += 1

    # Top-level CMakeLists.txt fuer prt_art/legacy_reimpl/
    top_cmake = ROOT / "CMakeLists.txt"
    if not top_cmake.exists():
        top_cmake.parent.mkdir(parents=True, exist_ok=True)
    top_lines = ["# prt_art/legacy_reimpl — Re-Implementations-Skelette (Phase 4.B)", ""]
    for entry in PAPERS:
        dirname = entry[0]
        top_lines.append(f"add_subdirectory({dirname})")
    top_lines.append("")
    top_cmake.write_text("\n".join(top_lines), encoding="utf-8")

    print(f"OK: {written} Re-Implementations-Skelette generiert in {ROOT}")
    print(f"     Plus 1 Top-Level CMakeLists.txt")


if __name__ == "__main__":
    main()
