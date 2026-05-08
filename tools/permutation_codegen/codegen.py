"""permutation_codegen/codegen.py — Skelett (F-EXTRA-5)

Phase 4.B Skelett — KEINE Implementation, nur Strukturhinweis.

Aufgabe (zu implementieren in Phase 5+):
  1. Liest Bausteine_Matrix (siehe docs/bausteine/Bausteine_Matrix.txt)
  2. Liest Flag_System (siehe docs/architecture/Flag_System.txt)
  3. Enumeriert alle gueltigen Permutationen via ConstraintFilter
  4. Generiert pro Permutation:
       - Schalen-.cpp/.ccm (C++23-Modul-Wrapper)
       - external_objects.cmake (Original-Compiler-Bausteine-Referenzen)
       - CMake `add_library(perm_<flags> ...)` Eintrag
  5. Output: build/generated/permutations.cmake
"""
import argparse
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Permutations-Codegen (Skelett)")
    parser.add_argument("--target-isa", default="auto", help="Target-ISA")
    parser.add_argument("--output", required=True, help="Output-CMake-Datei")
    parser.add_argument("--bausteine-matrix", default="docs/bausteine/Bausteine_Matrix.txt")
    parser.add_argument("--flag-system", default="docs/architecture/Flag_System.txt")
    args = parser.parse_args()

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)

    # Skelett-Output
    output.write_text("""# permutations.cmake — generiert von tools/permutation_codegen/codegen.py
# (Phase 4.B Skelett — keine Permutationen, kommt in Phase 5+)

message(STATUS "Permutations-Codegen-Skelett (keine Permutationen registriert)")
""", encoding="utf-8")

    print(f"OK (Skelett): {output} geschrieben.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
