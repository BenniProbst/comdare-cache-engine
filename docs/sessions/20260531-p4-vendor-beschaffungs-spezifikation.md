# P4-Vendor — Beschaffungs-/Setup-Spezifikation (jemalloc + tcmalloc echt linken)

**Stand:** 2026-05-31 · **Kontext:** /goal V3 P4 („Vendor/PMC beschaffungs-gated; soweit offline/lokal
machbar; sonst konkrete Beschaffungs-Anforderung an User"). User-Entscheidung (AskUserQuestion 2026-05-31):
„Vendor-Allokatoren/PMC vorher beschaffen".

Dieses Dokument ist die **konkrete Beschaffungs-Anforderung** — nach lokaler Machbarkeits-Prüfung, die
ergab, dass jemalloc + tcmalloc auf diesem Rechner NICHT offline-lokal mit der Projekt-Toolchain (MSVC)
baubar sind.

---

## 1. Was bereits ECHT gelinkt ist (kein Beschaffungs-Bedarf)

Die Vendor-Linking-**Mechanik** ist im cache-engine vollständig vorhanden und an CMake-nativen
Allokatoren real bewiesen — es fehlt KEINE Architektur, nur zwei spezifische Unix-Build-System-Libs:

| Allokator | Build-System | Header eingecheckt? | Status |
|-----------|--------------|---------------------|--------|
| A04-mimalloc | CMake-nativ | ✅ ja (`ext/.../A04-mimalloc/include`) | **echt linkbar** (`COMDARE_HAVE_MIMALLOC`) |
| A07-snmalloc | CMake-nativ | ✅ ja | **echt linkbar** (`COMDARE_HAVE_SNMALLOC`) |
| A20-dlmalloc | CMake-nativ | ✅ ja | **echt linkbar** (`COMDARE_HAVE_DLMALLOC`) |

Der Adapter-Mechanismus (`adapters/A0x-*/` mit `COMDARE_HAVE_*`-Gate → `target_compile_definitions` +
`target_include_directories`) ist für ALLE 22 Vendor identisch verdrahtet. Die 25 Vendor-Wrapper sind
testgrün (`test_v41_topic_allocator_axis_06` 266/266). Es fehlt ausschliesslich das **echte Lib-Linking**
für jemalloc + tcmalloc, das an deren Build-Systemen scheitert (siehe §2).

---

## 2. Warum jemalloc + tcmalloc NICHT lokal-offline baubar sind (verifiziert 2026-05-31)

### 2.1 jemalloc (A05, BSD-2) — autotools-gated

- Vendored: `ext/allocator/A05-jemalloc/` enthält `autogen.sh` + `configure.ac`, ABER **kein**
  vorgeneriertes `configure` und **keinen** öffentlichen Header `include/jemalloc/jemalloc.h` —
  `include/jemalloc/` hat nur `.h.in`-**Templates** (`jemalloc_defs.h.in`, `jemalloc_macros.h.in`,
  `jemalloc_protos.h.in`, `jemalloc_typedefs.h.in`) + Shell-Skripte (`jemalloc.sh`, `*_mangle.sh`).
- Die öffentlichen + internen Header (`jemalloc/jemalloc.h`, `internal/jemalloc_internal_defs.h`)
  werden von `configure` (autoconf) plattform-spezifisch aus den `.h.in` generiert. Die MSVC-Solution
  (`msvc/jemalloc_vc2022.sln`) hat **keinen** PreBuildEvent zur Header-Generierung — sie SETZT die
  generierten Header VORAUS.
- **Lokale Toolchain-Prüfung:** Windows-Host hat KEIN autoconf/automake/make/gcc (nur `sh`/`bash` aus
  Git-Bash). WSL-Ubuntu ist vorhanden, aber **bare** (kein gcc/g++/make/autoconf, nur `ar`).
- Der Adapter (`adapters/A05-jemalloc/a05_jemalloc_adapter.hpp`) erwartet `#include <jemalloc/jemalloc.h>`
  + ruft `je_malloc`/`je_free` → ohne generierten Header + gebaute `jemalloc.lib` nicht kompilierbar.
- **Ein Hand-Port der generierten Header wurde bewusst NICHT gemacht** — er würde ein nicht-autoritatives
  jemalloc erzeugen (verletzt Direktiven „Algorithmus-Korrektheit bei Namensanspruch" + „Niemals
  Quick-Fixes"). jemalloc `INSTALL.md` schreibt autotools zwingend vor.

### 2.2 tcmalloc (A06, Apache-2.0) — Bazel/abseil/Linux-only

- Vendored: `ext/allocator/A06-tcmalloc/` ist das **Google-tcmalloc** (Bazel `BUILD` + abseil-Abhängigkeit),
  NICHT das ältere gperftools-tcmalloc. Google-tcmalloc unterstützt **offiziell nur Linux + Bazel** und
  hat keine MSVC-Unterstützung. Ein lokaler Windows/MSVC-Build ist ausgeschlossen.

---

## 3. Konkrete Beschaffungs-/Setup-Anforderung an den User

Eine der folgenden Optionen genügt, um jemalloc echt zu linken (tcmalloc-Variante getrennt darunter):

### Option A (empfohlen, geringster Aufwand): vcpkg-Pre-Built-Libs auf dem Windows-Host
1. vcpkg installieren (`git clone https://github.com/microsoft/vcpkg`, `bootstrap-vcpkg.bat`).
2. `vcpkg install jemalloc:x64-windows gperftools:x64-windows` (gperftools liefert ein
   MSVC-baubares tcmalloc — Vendor-Substitution für A06, mit User-Freigabe).
3. cache-engine konfigurieren mit `-DCOMDARE_HAVE_JEMALLOC=ON -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake`
   + Adapter-CMakeLists um `target_link_libraries(... jemalloc)` ergänzen (1-Zeilen-Änderung, sobald Lib da).
   → **FortiGate-Egress:** vcpkg lädt von GitHub — benötigt eine Egress-Ausnahme (analog Boost.MP11-Problem).

### Option B: WSL-Ubuntu vollständig einrichten + gcc-Cross-Build der cache-engine
1. In WSL: `sudo apt update && sudo apt install -y build-essential autoconf automake` (benötigt
   apt-Netzzugang — FortiGate-Egress-Ausnahme für archive.ubuntu.com).
2. jemalloc bauen: `cd ext/.../A05-jemalloc && ./autogen.sh && ./configure && make` → `lib/libjemalloc.a`.
3. cache-engine mit dem **gcc-release-Preset** (existiert in `CMakePresets.json`) bauen, dabei
   `-DCOMDARE_HAVE_JEMALLOC=ON` + Link gegen `libjemalloc.a`.
   → Erzeugt Linux-Artefakte; die i7-1270P-Mindest-Messreihe (P3) müsste dann auf der Linux-Seite
   wiederholt werden (eigene Mess-Charge).

### Option C: Pre-Built-Binär direkt bereitstellen
- User legt eine gebaute `jemalloc.lib` (x64, MSVC-ABI-kompatibel) + den generierten
  `include/jemalloc/jemalloc.h` unter `ext/allocator/A05-jemalloc/{lib,include}` ab. Dann genügt
  `-DCOMDARE_HAVE_JEMALLOC=ON` + die 1-Zeilen-`target_link_libraries`-Ergänzung.

### tcmalloc
- Entweder Google-tcmalloc unter Linux/Bazel bauen (Option-B-analog, schwer), ODER — empfohlen —
  **gperftools-tcmalloc** als Vendor-Substitution (MSVC-baubar, vcpkg-`gperftools`). Diese Substitution
  ist eine **User-Entscheidung** (anderer Paper-Code als Google-tcmalloc; Doku-18-Provenienz-Map müsste
  den Wechsel A06 = gperftools statt Google dokumentieren).

---

## 4. Auswirkung auf das /goal

- P4-Vendor wird von §(a) actionable-non-blockiert → **§(b) extern/toolchain-gated** umklassifiziert
  (analog P4-PMC = Intel-PCM/MSR). Damit ist C(c) (Ledger frei von actionable nicht-extern-gateten
  Punkten) erreichbar, sobald die übrigen Punkte erledigt sind.
- Die F15-Mindest-Messreihe (P3) wurde mit dem **std-Allokator + mimalloc/snmalloc (echt)** erhoben —
  jemalloc/tcmalloc fehlen als zwei zusätzliche Vendor-Datenpunkte, nicht als Architektur-Lücke.
