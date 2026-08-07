# VENDOR PROVENANCE -- zlib (A9-S1, xlsx-Writer Vendor-Scheibe)

Stufe-1-Vendor (vendor -> faithful -> self-contained; hier Stufe 1 = faithful copy) der
Kompressions-Bibliothek zlib. Gebraucht wird sie als einzige externe Abhaengigkeit von
libxlsxwriter (`ext/io/libxlsxwriter/`): der xlsx-Container ist ein ZIP, und minizip packt ihn
mit zlibs deflate.

## Quelle

- Upstream:   https://github.com/madler/zlib
- Tag:        v1.3.2
- Tag-Objekt: 216c70c020aa53f0c40920d155f808b6b59c9acb  (annotiertes Tag)
- Commit:     da607da739fa6047df13e66a2af6b8bec7c2a498  (Ziel des Tags)
- Bezogen:    2026-08-07 via `git clone --depth 1 --branch v1.3.2`
- Release:    1.3.2 vom 17.02.2026 (ChangeLog-Kopf des Tags), zum Bezugszeitpunkt das neueste Tag
- Lizenz:     zlib-Lizenz (siehe LICENSE in diesem Verzeichnis), (C) 1995-2026 Jean-loup Gailly
              und Mark Adler

Warum mit-vendorn statt `find_package(ZLIB)`: self-contained ueber die 8er-Docker-Matrix, keine
zlib-dev-Header-Annahme auf den Hosts und auf den neu hinzukommenden Plattformen (RISC-V, macOS).
Das ist der sauberste, nicht der einfachste Weg. Umkehrbar: ein Consumer, der System-zlib will,
zieht dieses Verzeichnis schlicht nicht herein.

## Was am Checkout geaendert wurde (faithful, minimal)

1. `.git/` entfernt (Vendor, kein Submodule -- Haus-Doktrin: keine Git-Submodules).
2. Upstream-`.gitignore` NICHT uebernommen (liburing-Nachtrag 23.07.: eine .gitignore ist im
   Vendor-SNAPSHOT funktionslos und schadet aktiv, weil sie generierte Dateien vom Commit
   ausschliesst und damit CI-Builds bricht).
3. `./configure --static` EINMAL im Klon ausgefuehrt und davon AUSSCHLIESSLICH das erzeugte
   `zconf.h` uebernommen (dieselbe Vorgehensweise wie beim liburing-Snapshot). configure aendert an
   `zconf.h` genau zwei Zeilen:

   ```
   446c446
   < #if HAVE_UNISTD_H-0     /* may be set to #if 1 by ./configure */
   ---
   > #if 1     /* was set to #if 1 by ./configure */
   450c450
   < #if HAVE_STDARG_H-0     /* may be set to #if 1 by ./configure */
   ---
   > #if 1     /* was set to #if 1 by ./configure */
   ```

   WARUM das noetig ist: ohne diese Schalter uebersetzen `gzlib.c`/`gzread.c`/`gzwrite.c` nicht
   (`implicit declaration of function 'read'/'write'/'close'` -- `<unistd.h>` wird nicht inkludiert).
   WARUM es NICHT ueber `target_compile_definitions` geloest wurde: `HAVE_UNISTD_H` wirkt in
   `zconf.h`, also im OEFFENTLICHEN Header. Als PRIVATE-Define saehe die Bibliothek andere Typen als
   ihre Consumer (z_off_t), als PUBLIC-Define lecken zwei sehr allgemein benannte Makros in den
   gesamten ce-Bau. Der Upstream-Weg -- der Schalter steht IM ausgelieferten Header, beide Seiten
   sehen dasselbe -- ist der einzige ohne diese Nebenwirkung.
   Alle uebrigen configure-Artefakte (`Makefile`, `configure.log`, `zlib.pc`) wurden NICHT uebernommen.
4. GEPRUNT auf den Bibliotheks-Kern (Repo-Gewicht: 4,3 MB -> 1,2 MB). BEHALTEN: die 15 Quellen und
   9 privaten Header der Upstream-Liste `ZLIB_SRCS`/`ZLIB_PRIVATE_HDRS` (CMakeLists.txt:124-150 am
   Tag), die oeffentlichen Header `zlib.h` + `zconf.h`, `LICENSE`, `README` sowie die zwei
   Comdare-Dateien (`CMakeLists.txt`, diese Datei).
   ENTFERNT (fuer unseren Bau nicht gebraucht, per Tag jederzeit wiederherstellbar):
   `amiga/`, `BUILD.bazel`, `ChangeLog`, `.cmake-format.yaml`, `configure`, `contrib/`, `doc/`,
   `examples/`, `FAQ`, `.github/`, `.gitignore`, `INDEX`, `Makefile`, `Makefile.in`, `make_vms.com`,
   `MODULE.bazel`, `msdos/`, `os400/`, `qnx/`, `README-cmake.md`, `test/`, `treebuild.xml`,
   `watcom/`, `win32/`, `zconf.h.in`, `zlib.3`, `zlib.3.pdf`, `zlibConfig.cmake.in`, `zlib.map`,
   `zlib.pc.cmakein`, `zlib.pc.in`.
   Die Upstream-`CMakeLists.txt` wurde durch den Comdare-Wrapper ERSETZT (gleicher Dateiname).
   Der Quelltext der behaltenen `.c`/`.h` ist 1:1 unveraendert -- einzige Ausnahme ist das oben
   protokollierte, von configure erzeugte `zconf.h`.

## Bau

Das CMake-Target `comdare_vendored_zlib` (siehe `CMakeLists.txt` hier) baut die 15 Kern-Quellen
statisch, mit `${CMAKE_CURRENT_LIST_DIR}` als SYSTEM PUBLIC include-Pfad und ohne zusaetzliche
Praeprozessor-Schalter (Begruendung s. Punkt 3). Consumer-only: das Target wird ausschliesslich
gezogen, wenn ein Consumer es hereinholt (NOT-TARGET-Guard) -- am Ist ist das genau
`ext/io/libxlsxwriter` und der Rauchtest `tests/unit/test_a9_xlsx_vendor_rauchtest.cpp`.

zlib 1.3.2 nutzt `<stdatomic.h>`, wenn der Compiler C11-Atomics anbietet (`zutil.h:263-327`), und
faellt sonst selbsttaetig auf einen nicht-atomaren Pfad zurueck. Es braucht dafuer keinen
Bau-Schalter und keine zusaetzliche Bibliothek.
