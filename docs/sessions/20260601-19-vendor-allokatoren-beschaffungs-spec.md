# #19 — Beschaffungs-/Setup-Spezifikation: Vendor-Allokatoren (Achse axis_06) auf Windows/MSVC + Linux/ZIH real einlinken

**Status: Beschaffungs-Spec (Umsetzung extern / durch User).** Dieses Dokument beschreibt, WIE die noch offenen Vendor-Allokatoren beschafft und in den Build der `comdare-cache-engine` eingehängt werden. Es führt die Beschaffung nicht selbst aus (Netz-/Toolchain-/Rechte-gebunden). Alle CMake-/Flag-/Shim-Mechanik ist **real bewiesen** an den bereits funktionierenden Vendor **mimalloc (A04, `src/static.c`-Direktbuild)**, **snmalloc (A07, header-only)** und **dlmalloc (A20, manueller Opt-in)** — der Pfad ist also kein Neuland, sondern Nachbau eines verifizierten Musters.

**Lokale Toolchain LITERAL geprüft (2026-06-01):** Host hat NUR `cmake` (C:\Program Files\CMake). `make`/`gcc`/`g++`/`bazel`/`gyp`/`ninja` und `cl` (im Bash-PATH) = ABSENT. `vcpkg` = nicht in PATH. WSL `Ubuntu` existiert (Stopped, WSL2), ist laut bestehender Spec aber **bare** (kein gcc/make/autoconf). Konsequenz: keiner der offenen Vendor ist auf diesem Host hier-und-jetzt offline mit der Projekt-Toolchain (MSVC) baubar — Beschaffung erfordert vcpkg-Install, MSYS2-bash oder den ZIH-Linux-Zweig.

---

## 1. Verifizierter Ist-Mechanismus (repo-konkret, am Source gelesen)

Drei Variablen pro Vendor `<V>`:

- **`COMDARE_AXIS_06_ENABLE_<V>`** = User-Wunsch. `option(...)` in der Wurzel-`CMakeLists.txt` Z.85–116, default **`ON`**.
- **`COMDARE_HAVE_<V>`** = Source baubar/gefunden. Wird **NICHT** in der Wurzel gesetzt, sondern in `ext/CMakeLists.txt` (Auto-Detection) ODER `adapters/A0x-<v>/CMakeLists.txt` (manueller Opt-in). Default **`OFF`**. `FORCE`-Cache-Variable → **nicht manuell** setzen, sondern durch eine echte Quelle erfüllen lassen.
- **`COMDARE_AXIS_06_USE_<V>`** = `ENABLE AND HAVE`, berechnet in der Wurzel-`CMakeLists.txt` Z.484–646, dann via `configure_file(... @ONLY)` (Z.734–737) in `axis_06_allocator_flags.hpp` als `#cmakedefine01` materialisiert.

**Datenfluss:** `flags.hpp.in` (`#cmakedefine01 COMDARE_AXIS_06_USE_<V>`) → generierte `build/generated/axes/alloc/axis_06_allocator_flags.hpp` → `inline constexpr bool <v>_enabled` → Shim `vendor_includes/<v>_include.hpp` (`#if COMDARE_AXIS_06_USE_<V>` zieht echten Header, sonst no-op-Stubs) → Wrapper `axis_06_allocator_<v>.hpp` (`static constexpr bool enabled = flags::<v>_enabled;`; im `if constexpr (enabled)`-Zweig echte Vendor-API, sonst `portable_aligned_alloc`-Fallback).

> **Namens-Realität:** Die wahre Quelle liegt unter `libs/cache_engine/axes/alloc/` (NICHT `topics/allocator/...`). Die `topics/allocator/axis_06_allocator/*.hpp` sind 3-Zeilen-Re-Export-Shims (`using namespace ...::alloc;`). Bearbeitet/gelesen werden Wrapper/Shims/Template in **`libs/cache_engine/axes/alloc/`**.

> **Trigger-Caveat (zentral, gilt für ALLE Vendor):** Die HAVE-Detection-Branches in `ext/CMakeLists.txt` stehen unter `option(COMDARE_VENDOR_<V> ... ${COMDARE_BUILD_PERMUTATIONS})`. `add_subdirectory(ext)` läuft nur im `if(COMDARE_BUILD_PERMUTATIONS)`-Block (Wurzel Z.380–388). **Ohne `-DCOMDARE_BUILD_PERMUTATIONS=ON` wird kein Vendor gebaut.** Der aktuelle Build-Cache zeigt entsprechend `USE_MIMALLOC=0 / SNMALLOC=0 / DLMALLOC=0 / JEMALLOC=0` (nur `USE_STD=1 / USE_PMR=1 / USE_POOL=1`, weil diese kein HAVE-Gate haben). Also IMMER mit `-DCOMDARE_BUILD_PERMUTATIONS=ON` (oder gezielt `-DCOMDARE_VENDOR_<V>=ON`) konfigurieren.

**Bewiesenes Vorbild:**
- **mimalloc (A04)** baut wirklich auto (HAVE in `ext/CMakeLists.txt` Z.48 gesetzt, eigener `static.c`-Direktbuild) — **das Ziel-Muster für neue Direktbuild-Vendor**.
- **snmalloc (A07)** = header-only Auto-Detect (`ext/CMakeLists.txt` Z.120–137).
- **dlmalloc (A20)** = nur manuelle Option in `adapters/A20-dlmalloc/CMakeLists.txt` (default OFF); Header `ext/allocator/A20-dlmalloc/malloc.h` vorhanden, aber für echtes USE=1 fehlt aktuell ein `ext`-Build-Branch.

Der erzeugte Target heißt **immer `comdare::vendor_<v>`** — exakt diesen Alias erwartet der Permutations-Codegen (`tools/permutation_codegen/codegen.cmake` Z.609–645: `if(TARGET comdare::vendor_<v>) target_link_libraries(perm_... PRIVATE comdare::vendor_<v>)`).

---

## 2. Generisches Schritt-für-Schritt — neuen Vendor `<V>` real einlinken

### (1) Source/Lib nach `ext/allocator/A0x-<vendor>/` legen
Vollen **baubaren** Tree (mit GENERIERTEN Headern!) ablegen. Beispiel jemalloc: autotools-Tree hat nur `*.h.in` → vor dem Einchecken einmalig `autogen.sh`/`configure` laufen lassen, damit `include/jemalloc/jemalloc.h` UND `include/jemalloc/internal/jemalloc_preamble.h` existieren (genau diese beiden prüft `ext/CMakeLists.txt` Z.76–77). Parallel die kuratierte Paper-Code-Teilmenge unter `ext/allocator/axis_06_allocator/paper_a0x_<vendor>/` mit `manifest.txt` (für is_original, Schritt 4).

### (2) HAVE-Branch in `ext/CMakeLists.txt` ergänzen (analog mimalloc-Branch Z.16–50)
Pro Vendor: `option(COMDARE_VENDOR_<V> ... ${COMDARE_BUILD_PERMUTATIONS})`, `set(COMDARE_HAVE_<V> OFF CACHE BOOL "" FORCE)`, dann Detection. Drei bewährte Branch-Typen:
- **Direktbuild** (Vorbild mimalloc, Z.23–49): `EXISTS`-Guard auf Schlüsseldatei → `enable_language(C)` → `add_library(comdare_vendor_<v> STATIC <sources>)` → `target_include_directories(... PUBLIC <include>)` → MSVC-Defs → `set_target_properties(... POSITION_INDEPENDENT_CODE ON FOLDER "vendor")` → `add_library(comdare::vendor_<v> ALIAS ...)` → `set(COMDARE_HAVE_<V> ON CACHE BOOL "" FORCE)` + `message(STATUS ...)`. **Für jemalloc existiert dieser Branch bereits** (Z.74–100), gegated auf vorhandene generierte Header.
- **Header-only** (Vorbild snmalloc, Z.117–137): `INTERFACE`-Target + INTERFACE-Includes → Alias → HAVE ON.
- **System / find_library** (Vorbild tcmalloc/hoard/scalloc, Z.139–200): `find_library`+`find_path` → `INTERFACE`-Target mit `target_link_libraries(... INTERFACE ${LIB})` → Alias → HAVE ON. Primärpfad auf vcpkg und ZIH (`module load`).

Falls Vendor noch nicht im Flag-Set: `#cmakedefine01 COMDARE_AXIS_06_USE_<V>` + `inline constexpr bool <v>_enabled` in `axis_06_allocator_flags.hpp.in`, ENABLE-Option + USE-Block in der Wurzel, Shim + Wrapper nach dlmalloc-Muster. **Für die 5 offenen (jemalloc/tcmalloc/hoard/scalloc/tcmalloc_wh) sind Flags, Shims und Wrapper bereits vorhanden** — dort genügt der HAVE-Branch (bzw. die Beschaffung).

### (3) Reconfigure → USE_<V>=1 prüfen
```
cmake -S . -B build -DCOMDARE_BUILD_PERMUTATIONS=ON -DCOMDARE_VENDOR_<V>=ON -DCOMDARE_AXIS_06_ENABLE_<V>=ON
```
(bzw. VS-Preset aus `CMakePresets.json`; auf ZIH ggf. zusätzlich `-DCMAKE_PREFIX_PATH=<modul-prefix>`).

### (4) Wrapper-Test + is_original-Validierung
- Wrapper erbt von `generated::a0x_<v>::OriginalCodeMixin` (siehe `axis_06_allocator_dlmalloc.hpp` Z.29/48). `paper_a0x_<v>_is_original.hpp` wird zur Build-Zeit von `apps/is_original_validator` aus der `manifest.txt` (mit `@axis_mixin_type`) + `sha256_locked.txt` generiert (`cmake/is_original_codegen.cmake`).
- Test: Target `test_v41_topic_allocator_axis_06` (`tests/unit/CMakeLists.txt` Z.283–290) iteriert via `TYPED_TEST_SUITE` über `axis_06::AllVendors` → neuer Vendor wird ohne Test-Änderung mitgetestet.
```
cmake --build build --target test_v41_topic_allocator_axis_06
ctest --test-dir build -R test_v41_topic_allocator_axis_06 --output-on-failure
```

---

## 3. Option-Matrix pro offenem Vendor

| Vendor | Win/MSVC offline-baubar? | Realistische Beschaffungs-Option | Wie `COMDARE_HAVE_<V>` ON wird | Quelle (Include/Lib) |
|--------|--------------------------|----------------------------------|--------------------------------|----------------------|
| **jemalloc (A05)** | bedingt (Header erst per autogen) | (A) **vcpkg** `x64-windows-static`; (C1) MSYS2-bash nur als Header-Generator → MSVC-Build; (D) Pre-Built `.lib`; (B) WSL/ZIH = Linux-only | Stufe 1 (`find_library`/`find_path`) ODER Stufe 2 (Vendored `src/*.c`-Build, Branch existiert Z.74–100) | vcpkg-`installed`, Vendored `ext/allocator/A05-jemalloc/include`, oder beliebiger `.lib`-Pfad |
| **tcmalloc (A06)** | **nein** (Google/Bazel/Linux) | **gperftools-Substitution** via vcpkg (`x64-windows-static`, /MT!) — *User-Entscheidung*, Doku-18 anpassen; ODER echtes Google-tcmalloc nur auf ZIH-Linux | `find_library` bereits verdrahtet (Z.142–158); HAVE=ON erst nach vcpkg-Install | vcpkg-gperftools `gperftools/tcmalloc.h` |
| **hoard (A01)** | als DLL ja, als saubere Symbol-API **nein** (Detours-Interpose, /MD-Zwang, nur Hauptmodul) | Win: HAVE=OFF/Fallback (empfohlen); echte Symbol-API nur auf ZIH-Linux (`libhoard.a`+`hoardheap.h`) | `find_library` only (Z.161–181); bleibt auf Win OFF | ZIH-Linux |
| **scalloc (A08)** | **nein** (gyp, LD_PRELOAD, 64-bit-VM, Linux/macOS) | „nur Linux-Cluster"; Win HAVE=OFF/Fallback. Shim-Zitat korrigieren | `find_library` only (Z.184–200); bleibt auf Win OFF | ZIH-Linux `libscalloc.so` |
| **tcmalloc_wh (A14)** | **nein** (kein Source vorhanden, = A06-intern) | als A06-Variante deklarieren ODER aus Vendor-Set nehmen — *User-Entscheidung*. **KEIN Fake-Header.** | KEIN HAVE/VENDOR-Block → strukturell immer 0 | — |

---

## 4. jemalloc (A05) — Detailpfade

Der Vendored Tree `ext/allocator/A05-jemalloc/` ist **bare autotools** (NICHT vorgeneriert): nur `*.h.in` + `internal/jemalloc_preamble.h.in`, **keine** Umbrella `jemalloc.h`, **kein** generiertes `jemalloc_preamble.h` → Stufe 2 schlägt heute fehl (literal verifiziert). Vorhanden sind `msvc/jemalloc_vc{2015,2017,2019,2022}.sln`, `autogen.sh`, `configure.ac`, `m4/`, `src/jemalloc.c`, `include/msvc_compat/`.

### (A) vcpkg — empfohlen, mündet in Detection-Stufe 1
Offizieller vcpkg-Port (BSD-2-Clause), Portversion **5.3.1** (mit `vcpkg search jemalloc` prüfen). Triplet `x64-windows-static` (statisch, passend zur Engine) oder `x64-windows` (DLL).
```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
& C:\dev\vcpkg\bootstrap-vcpkg.bat
& C:\dev\vcpkg\vcpkg.exe install jemalloc:x64-windows-static
```
Configure (Detection nutzt `find_library`/`find_path`, NICHT `find_package`):
```
-DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake
-DVCPKG_TARGET_TRIPLET=x64-windows-static
-DCOMDARE_BUILD_PERMUTATIONS=ON  -DCOMDARE_VENDOR_JEMALLOC=ON
```
Falls Toolchain-Datei nicht setzbar: `-DCMAKE_PREFIX_PATH=C:/dev/vcpkg/installed/x64-windows-static` ODER explizit `-DJEMALLOC_LIB=.../lib/jemalloc.lib -DJEMALLOC_INC=.../include` (find-Cache-Variablen vorbelegen unterdrückt die Suche).

### (C1) MSYS2-bash als Header-Generator → MSVC-Build (deckt sich mit Z.106)
Im „x64 Native Tools Command Prompt for VS 2022" (cl im PATH), MSYS2/Cygwin mit `autoconf, gawk, grep, sed`:
```
cd ext\allocator\A05-jemalloc
sh -c "CC=cl ./autogen.sh"
```
Danach existieren die zwei generierten Header → **Detection-Stufe 2 greift automatisch** (baut `src/*.c` mit `cl`, `JEMALLOC_NO_PRIVATE_NAMESPACE=1`, `/wd4267 /wd4244 /wd4146`). Da Compiler = `cl`, ist die Lib COFF/MSVC-ABI-kompatibel. Alternativ `.sln` in VS bauen → resultierende `jemalloc.lib` als Pre-Built (D).

### (D) Pre-Built — fertige `.lib` + Header anflanschen (Stufe 1)
```
-DCOMDARE_BUILD_PERMUTATIONS=ON  -DCOMDARE_VENDOR_JEMALLOC=ON
-DJEMALLOC_LIB=C:/path/to/jemalloc/lib/jemalloc.lib
-DJEMALLOC_INC=C:/path/to/jemalloc/include   # muss jemalloc/jemalloc.h enthalten
```

### (B) WSL2/gcc — NUR Linux/ZIH, NICHT MSVC-Host
WSL2-gcc erzeugt eine **ELF-`.a`/`.so`** → **nicht** mit dem MSVC-Windows-Build linkbar (ELF vs COFF). Nur für den Linux-/ZIH-Zweig (Barnard):
```bash
sudo apt-get install -y build-essential autoconf
cd .../ext/allocator/A05-jemalloc
./autogen.sh && ./configure && make -j$(nproc)
```
Auf ZIH ohne root: `./configure --prefix=$HOME/.local && make install`, dann `-DCMAKE_PREFIX_PATH=$HOME/.local`. **Whitespace-Caveat:** OneDrive-Pfad enthält Leerzeichen (`Diplomarbeit - Datenbanken`); autotools meist tolerant, ein vorhandenes Drittanbieter-CMake bricht aber mit `FATAL_ERROR "jemalloc cannot be built with whitespaces in the directory path."` → ggf. außerhalb bauen, Artefakte zurückkopieren (→ Pfad D).

### jemalloc-Caveats
- **Upstream am 2025-06-02 archiviert (read-only)**, letzter Strang 5.3.x — vcpkg-Port 5.3.1 = pragmatischer Referenzstand.
- **Mangling (`je_`-Prefix):** Der Shim ruft `je_aligned_alloc`/`je_free`/`je_calloc`/`je_realloc`/`je_malloc_usable_size` — also den `je_`-Prefix (Default unter Windows/MSVC). Wurde ohne Prefix gebaut (`--with-jemalloc-prefix=""`) → unresolved symbols. **Vor erstem Link prüfen:** `dumpbin /SYMBOLS jemalloc.lib | findstr aligned_alloc`.
- Statisch sauberer für F15 (kein DLL-Heap-Cross-Talk); falls nur `x64-windows` (DLL) baut → DLL neben die Test-`.exe`.

---

## 5. tcmalloc (A06) — gperftools-Substitution (User-Entscheidung)

`ext/allocator/A06-tcmalloc/CMakeLists.txt` ist **Google-tcmalloc** (C++17, FetchContent abseil/protobuf/googletest/benchmark/fuzztest, alle `GIT_TAG main` → Netz nötig; Linux/Bazel-only, kein MSVC). Der Shim erwartet `#include <gperftools/tcmalloc.h>` + `tc_malloc_size`/`tc_free` (**gperftools-API**), NICHT die vendored Google-API (`tcmalloc/malloc_extension.h`) → Shim und vendored Source sind bereits inkonsistent.

**Empfohlene Option (User-Entscheidung): gperftools statt Google-tcmalloc für A06.**
- `vcpkg install gperftools` baut auf MSVC als **statische** Lib — **nur mit statischer CRT** (`VCPKG_CRT_LINKAGE static`, Triplet `x64-windows-static`, also /MT). Mit dynamischer CRT scheitert es. gperftools-CMake-Support ist „preliminary"; primär Autotools/`vsprojects/`.
- **Provenienz:** gperftools-tcmalloc ≠ Google-tcmalloc → **Doku-18-Map muss A06 = gperftools dokumentieren** (sonst Verstoß „Algorithmus-Korrektheit bei Namensanspruch"). Deshalb User-Entscheidung, kein Automatismus.
- `tcmalloc_include.hpp` passt API-seitig bereits zu gperftools (`tc_*`) — kein Shim-Umbau.

Aktivierung (HAVE-Branch existiert bereits, Z.142–158):
```
-DCOMDARE_BUILD_PERMUTATIONS=ON -DCOMDARE_VENDOR_TCMALLOC=ON
-DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake
-DVCPKG_TARGET_TRIPLET=x64-windows-static
```
**Caveat:** statische CRT muss zur CRT des Haupt-Builds passen (sonst LNK4098/Runtime-Mismatch) — **vor Aktivierung CRT-Linkage prüfen.** Achtung Konflikt: gperftools verlangt /MT, Hoard verlangt /MD — nicht im selben Build mischbar.
**Alternative:** echtes Google-tcmalloc nur auf ZIH-Linux (Barnard, gcc/Bazel) als eigene Linux-Mess-Charge; auf Windows bleibt A06 = Fallback.

---

## 6. hoard (A01) — Windows nur als Interpose-DLL (keine saubere API)

`ext/allocator/A01-hoard/CMakeLists.txt` hat einen echten WIN32-Zweig: baut hoard als **SHARED** DLL, holt Microsoft **Detours** per FetchContent (`GIT_TAG main` → Netz nötig), Sources `winwrapper-detours.cpp`/`wintls.cpp`/`libhoard.cpp`, Flags `/Ob2 /Oi /Ot`. Das ist **Interposition** (Detours hookt malloc/free global), KEIN exportiertes `hoard_malloc`/`hoard_free`-Symbolset. Hoard auf Windows erfordert **/MD** (entgegengesetzt zu gperftools' /MT). Der Shim `hoard_include.hpp` hat bewusst keinen echten Header-Include; die Stubs `hoard_memalign`/`hoard_free` existieren als öffentliche API in der Windows-DLL **nicht**.

**Realistische Optionen:**
1. **Windows: HAVE=OFF/Fallback belassen** — ehrliche Empfehlung. Ein global hookendes Interpose-Modell ersetzt auch die std-Baseline → für per-Allocator-F15-Messung unsauber.
2. **ZIH-Linux:** `hoardheap.h` + statisches `libhoard.a` mit echter Symbol-API → Wrapper linkt benannte Symbole; eigene Linux-Mess-Charge.
3. **(nur auf explizite User-Freigabe)** vendored Windows-DLL bauen (Detours-Netzzugang + /MD-Zwang) und als **Interpose**-Vendor messen → erfordert Mess-Methodik-Anpassung (global statt per-Allocator) + Shim-Umbau.

HAVE-Setzung: `ext/CMakeLists.txt` Z.161–181 sucht nur `find_library(hoard)` + `find_path(hoard.h)` → auf Windows weder System-Lib noch saubere API → HAVE bleibt korrekt OFF.

---

## 7. scalloc (A08) — nur Linux-Cluster

`ext/allocator/A08-scalloc/` ist Forschungscode mit **gyp** (`scalloc.gyp`, `common.gypi`), `.travis.yml`, `src/glue.cc` — kein CMake, kein MSVC-Port. Build via `make BUILDTYPE=Release` (Linux) bzw. Xcode (macOS); Nutzung via **LD_PRELOAD**; hängt stark vom **64-bit virtuellen Adressraum** + Linux/POSIX ab → **faktisch nicht auf Windows/MSVC portierbar** ohne erhebliches Re-Engineering.
- **Provenienz-Korrektur fürs Doku-18-Mapping:** Shim zitiert „PPoPP 2015" — korrekt ist **OOPSLA 2015** (ACM SIGPLAN, DOI 10.1145/2814270.2814294; arXiv:1503.09006). Kommentar korrigieren.
- Shim erwartet `#include <scalloc.h>` + `scalloc_malloc/_free/_calloc/_realloc`.

**Empfehlung:** Windows `COMDARE_HAVE_SCALLOC=OFF` (Fallback). Echte Messung nur auf Barnard (`make BUILDTYPE=Release` → `libscalloc.so`, gegen benannte API linken). `ext/CMakeLists.txt` Z.184–200 ist bereits exakt so verdrahtet (`find_library` + Hinweis „gyp, kein CMake-Port").

---

## 8. tcmalloc_wh (A14) — kein Source vorhanden, KEIN Fake-Header

LITERAL geprüft: unter `ext/allocator/` existiert **kein A14-Verzeichnis** (nur A01,A03,A04,A05,A06,A07,A08,A10,A11,A20). Shim `tcmalloc_wh_include.hpp` erwartet `#include <tcmalloc_warehouse/tcmalloc_warehouse.h>` (`tc_wh_alloc/_free/_calloc/_realloc`) — Header existiert nirgends. Kein `COMDARE_VENDOR_TCMALLOC_WH`-Block in `ext/CMakeLists.txt` → USE_TCMALLOC_WH ist strukturell immer 0 (Stub-only).

„TCMalloc-Warehouse" ist **keine** eigenständige beziehbare Distribution — es bezeichnet Hyperscale-/Warehouse-Erweiterungen *innerhalb* von Google-tcmalloc (selbe Codebasis A06, Bazel/abseil, Linux-only). Keine Windows/MSVC-Quelle, keine vom Google-tcmalloc trennbare Lib.

**Empfehlung (User-Entscheidung), zwei saubere Wege:**
1. **A14 als Konfigurations-Variante von A06 markieren** (Doku-18 = „Google-tcmalloc mit Warehouse-Profil"); HAVE_TCMALLOC_WH nur ON, wenn echtes Google-tcmalloc (ZIH/Bazel) mit Warehouse-Optionen gebaut. Auf Windows dauerhaft Stub/Fallback.
2. **A14 vorerst aus dem aktiven Vendor-Set nehmen** (ENABLE default OFF), bis separierbare Quelle existiert.

**Verboten:** Hand-Port von `tcmalloc_warehouse.h` (würde nicht-autoritatives „tcmalloc_wh" erzeugen → Verstoß „Algorithmus-Korrektheit" + „Niemals Quick-Fixes").

---

## 9. Verifikations-Checkliste — was LITERALE Ausgabe „real gelinkt, nicht Stub" beweist

1. **Configure-Log:** Branch-Marker, z.B. `-- V41.A1 jemalloc (system): /usr/lib/.../libjemalloc.a` (bzw. `(ext/A05 pre-configured): direkter Build`) UND in der Batch-Zeile `USE_JEMALLOC=1` (Wurzel Z.649–656). Fehlt der Marker → HAVE blieb OFF.
2. **Generierte Flag-Datei:** `build/generated/axes/alloc/axis_06_allocator_flags.hpp` enthält literal `#define COMDARE_AXIS_06_USE_<V> 1` (nicht `0`).
3. **Vendor-Target existiert:** `comdare::vendor_<v>` ist als Target vorhanden (Codegen-Branch `if(TARGET comdare::vendor_<v>)` greift).
4. **Echter Symbol-Pfad statt Stub:** `if constexpr (enabled)`-Zweig ruft `::je_*`/`::tc_*`/`::mi_*`/`::dl*` statt no-op-Stubs; `name()` liefert `"<v>"` statt `"<v>(real=std)"`. `static_assert(axis_06::<V>Allocator::enabled)` schlägt fehl, solange USE=0.
5. **is_original-Beleg:** generierte `paper_a0x_<v>_is_original.hpp` zeigt literal `// Module-Status: ALL ORIGINAL`, `kHasOriginalPaperCode = true`, pro Funktion `kIsOriginal_allocate = true` mit `computed_sha=...`. `false` ⇒ SHA-Mismatch/Re-Impl, kein Original-Linking.
6. **ctest:** `test_v41_topic_allocator_axis_06` grün; der `AllocateDeallocateRoundtripAllConfigs`/`ConceptConformance`-TYPED_TEST für den neuen Vendor erscheint als bestanden.

Erst wenn 1–6 mit literaler Ausgabe belegt sind, gilt der Vendor als „real gelinkt" ([[feedback_no_success_marks_without_literal_output]]).

---

## 10. Gemeinsame Caveats

- **FortiGate-Egress:** vcpkg (jemalloc/gperftools), Hoards Detours-FetchContent und Google-tcmalloc-FetchContent (abseil/protobuf etc.) laden von GitHub → brauchen FortiGate-Egress-Ausnahme (analog Boost.MP11). Linux-Wege (echtes hoard/scalloc/Google-tcmalloc) gehören auf den **ZIH-Cluster (Barnard) als getrennte Linux-Mess-Charge** — die Windows-i7-Messreihe bleibt davon unberührt.
- **CRT-Konflikt:** gperftools = /MT (`x64-windows-static`), Hoard = /MD — nicht im selben Build mischbar; CRT des Haupt-Builds zuerst klären.
- **Build-Cache:** aktueller `build/`-Cache hat `COMDARE_BUILD_PERMUTATIONS=OFF` → alle echten Vendor USE=0; vor jeder Verifikation neu konfigurieren.
- **Pfad-Inkonsistenz im Tree:** HAVE-`ext`-Branches referenzieren teils `ext/allocator/A0x-...`, teils `ext/.../A05-jemalloc` ohne `allocator/`-Zwischenebene (jemalloc-Branch Z.75) — beim neuen Vendor den real existierenden Ablageort exakt im `EXISTS`-Guard spiegeln.

**Nicht hart verifiziert:** genaue vcpkg-Portversionen (jemalloc 5.3.1 / gperftools), ob gperftools' MSVC-CMake-Pfad statt `vsprojects` greift, Detours-`main`-Kompatibilität mit aktueller MSVC, Stabilität von `x64-windows-static` je jemalloc-Version. Diese erst bei tatsächlicher Beschaffung bestätigen.

---

## 11. Relevante Datei-Pfade (absolut)
- `C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Code\external\comdare-cache-engine\ext\CMakeLists.txt` (HAVE-Detection: mimalloc/snmalloc/jemalloc/tcmalloc/hoard/scalloc)
- `...\comdare-cache-engine\CMakeLists.txt` (Z.85–116 ENABLE; Z.380–392 ext-Einbindung; Z.484–656 USE + Batch-Messages; Z.734–737 configure_file)
- `...\libs\cache_engine\axes\alloc\axis_06_allocator_flags.hpp.in` (Flag-Template)
- `...\libs\cache_engine\axes\alloc\vendor_includes\<v>_include.hpp` (Shim; mimalloc/jemalloc/dlmalloc als Vorbild)
- `...\libs\cache_engine\axes\alloc\axis_06_allocator_<v>.hpp` (Wrapper; dlmalloc-Header = vollständige Vorlage)
- `...\adapters\A04-mimalloc\CMakeLists.txt` + `...\adapters\A20-dlmalloc\CMakeLists.txt` (manueller Opt-in)
- `...\cmake\is_original_codegen.cmake` (is_original-Mixin-Generator)
- `...\tools\permutation_codegen\codegen.cmake` (Z.609–645: linkt `comdare::vendor_<v>`)
- `...\tests\unit\CMakeLists.txt` (Z.283–290 Test-Target) + `...\tests\unit\test_v41_topic_allocator_axis_06.cpp`
- Generierte Belege: `...\build\generated\axes\alloc\axis_06_allocator_flags.hpp` und `...\build\generated\topics\allocator\axis_06_allocator\legacy_code\paper_a0x_<v>_is_original.hpp`

**Quellen:** jemalloc INSTALL.md (dev) · jemalloc issue #1099 (msvc) · jemalloc releases/archival · vcpkg jemalloc 5.3.1 · microsoft/vcpkg ports/jemalloc · vcpkg gperftools · gperftools build/CRT (DeepWiki) · vcpkg gperftools issue #31626 · Hoard README + Documentation (Detours/Winhoard) · scalloc GitHub + OOPSLA 2015 DOI 10.1145/2814270.2814294.
