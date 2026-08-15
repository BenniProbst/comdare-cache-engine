// tests/unit/b2_probe_g3_schichtung_tu.cpp -- B2-ABSCHLUSS (15.08.2026): PROBE-TU der
// Schichtungs-Wache, KEIN ctest-Test (bewusst OHNE "test_"-Praefix, damit keine Registrierungs-Wache
// eine add_test-Registrierung verlangt).
//
// KONSUMENT: ausschliesslich die beiden try_compile-Proben im Block "B2-ABSCHLUSS NEGATIVPROBE" in
// tests/unit/CMakeLists.txt (Configure-Zeit, jeder Baum):
//   (a) NEGATIV  -DCOMDARE_CE_ENABLE_SEGMENT_TIMING=1 OHNE COMDARE_CE_ENABLE_STATISTICS:
//       MUSS am #error der EINEN G3-Ableitung scheitern (mess_gate_segment_timing.hpp:54-56) --
//       der Dual-Review-Fund war woertlich "die #error-Wache nie als feuernd bewiesen".
//   (b) KONTROLLE (K13, der Koeder muss beissen): dieselbe TU MIT beiden Gates MUSS bauen --
//       sonst koennte (a) aus einem fremden Grund scheitern (Include-Pfad, Standard, Toolchain)
//       und die Wache waere still wertlos.
//
// Der Rueckgabewert liest das Makro, damit die TU den abgeleiteten Wert wirklich konsumiert.
// ASCII-only.
#include <cache_engine/abi/mess_gate_segment_timing.hpp>

int main() { return COMDARE_CE_ENABLE_SEGMENT_TIMING == 1 ? 0 : 1; }
