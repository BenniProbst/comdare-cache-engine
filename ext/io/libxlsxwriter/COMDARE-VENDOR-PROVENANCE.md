# VENDOR PROVENANCE -- libxlsxwriter (A9-S1, xlsx-Writer Vendor-Scheibe)

Stufe-1-Vendor (vendor -> faithful -> self-contained; hier Stufe 1 = faithful copy) des
xlsx-Schreibers libxlsxwriter, fuer die Auswerte-/Lager-Ablage-Strecke A9 (Design-Dossier
`docs/architecture/20260803-a9_xlsx_writer_f3_soll_design.md`).

Der Vendor liefert AUSSCHLIESSLICH die xlsx-Serialisierung. Der "eigene Writer" im Sinne der
Owner-Spezifikation F3 -- Interfaces, Factory, Dateinamen-/Baum-Grammatik, INFO-Blatt,
Fehlerklassen -- ist Comdare-Code und entsteht in den Scheiben A9-S2/S3/S4.

## Quelle

- Upstream:   https://github.com/jmcnamara/libxlsxwriter
- Tag:        v1.2.4
- Tag-Objekt: 2894634d65cee6021901a165bfc2bb0fad6da193
  (LEICHTGEWICHTIGES Tag -- `git ls-remote --tags` liefert keine `^{}`-Zeile, das Tag zeigt also
   direkt auf den Commit; anders als bei zlib und liburing gibt es hier kein separates Tag-Objekt.)
- Commit:     2894634d65cee6021901a165bfc2bb0fad6da193
- Bezogen:    2026-08-07 via `git clone --depth 1 --branch v1.2.4`; zum Bezugszeitpunkt das
              neueste Tag des Upstream
- Lizenz:     FreeBSD-Lizenz = BSD-2-Clause (siehe `License.txt` in diesem Verzeichnis),
              Copyright 2014-2026 John McNamara

Kein GitHub-Fork, kein neues Repo, kein Submodule: der Snapshot liegt im BESTEHENDEN Repo, es
entsteht kein Remote. Kein Eigenbau des Formats: xlsx ist ZIP-Container + OOXML; ein
Minimal-Writer waere eine Neuimplementierung von deflate + XML-Escaping + Format-Details und
verstiesse gegen die geordnete Vendoring-Doktrin (vendoren, wenn moeglich).

## Gebuendelte Fremdanteile und ihre Lizenzen (aus `License.txt` des Tags erhoben)

| Anteil | Lizenz | Im Bau? |
|---|---|---|
| libxlsxwriter selbst (`src/`, `include/`) | BSD-2-Clause | ja |
| `queue.h`, `tree.h` (FreeBSD) | BSD | ja (Header) |
| `third_party/minizip` (ioapi, zip) | zlib-Lizenz; (C) 1998-2010 Gilles Vollant, Zip64 (C) 2009-2010 Mathias Svensson | ja |
| `third_party/md5` (Openwall MD5, Alexander Peslyak) | Public Domain | ja |
| `third_party/dtoa` (`emyg_dtoa`, Doug Currie / Milo Yip) | MIT | ja (`USE_DTOA_LIBRARY`) |
| `third_party/tmpfileplus` | **MPL-2.0** | **NEIN -- geprunt und nicht uebersetzt** |

## Was am Checkout geaendert wurde (faithful, minimal)

1. `.git/` entfernt (Vendor, kein Submodule -- Haus-Doktrin: keine Git-Submodules).
2. Upstream-`.gitignore` NICHT uebernommen (liburing-Nachtrag 23.07.: im Snapshot funktionslos und
   aktiv schaedlich).
3. GEPRUNT auf den Bibliotheks-Kern (Repo-Gewicht: 37 MB -> 1,9 MB; allein `docs/` 20 MB und
   `test/` 14 MB). BEHALTEN: `src/*.c` (26 Quellen), `include/` vollstaendig, die vier oben
   gelisteten `third_party`-Anteile in genau dem Umfang, den der Bau braucht, `License.txt`,
   `Readme.md`, `Changes.txt` sowie die zwei Comdare-Dateien (`CMakeLists.txt`, diese Datei).
   ENTFERNT (Top-Ebene): `build.zig`, `build.zig.zon`, `.cirrus.yml`, `cocoapods/`,
   `CONTRIBUTING.md`, `dev/`, `docs/`, `examples/`, `.github/`, `.gitignore`, `.indent.pro`,
   `lib/`, `libxlsxwriter.podspec`, `Makefile`, `Package.swift`, `test/`.
   ENTFERNT (weiter innen): `src/Makefile`, `third_party/*/Makefile`,
   `third_party/tmpfileplus/` (MPL, s.u.) sowie aus `third_party/minizip/` die nicht gebrauchten
   Anteile `configure.ac`, `iowin32.c`, `iowin32.h`, `Makefile`, `Makefile.am`, `Makefile.orig`,
   `make_vms.com`, `miniunz.c`, `miniunzip.1`, `minizip.1`, `MiniZip64_Changes.txt`, `minizip.c`,
   `minizip.pc.in`, `mztools.c`, `mztools.h`, `unzip.c`, `unzip.h`.
   Die Upstream-`CMakeLists.txt` wurde durch den Comdare-Wrapper ERSETZT (gleicher Dateiname).
   Der Quelltext der behaltenen Dateien ist 1:1 unveraendert.
4. **MPL-Ausnahme, benannt (Abweichung vom Design-Dossier Abschnitt 3.2):** das Dossier sagt
   "tmpfileplus geprunt". Vollstaendig prunen laesst sich nur die IMPLEMENTIERUNG
   `third_party/tmpfileplus/tmpfileplus.c` -- der HEADER
   `include/xlsxwriter/third_party/tmpfileplus.h` (ebenfalls MPL-2.0) wird von `src/utility.c:22`
   UNBEDINGT inkludiert, ausserhalb jeder `#ifdef`-Klammer. Ihn zu entfernen hiesse, den
   Fremdquelltext zu aendern -- das widerspraeche dem treuen Snapshot. Er bleibt daher im
   Verzeichnis, mit unveraendertem Lizenzkopf.
   Wirkung: MPL-2.0 ist file-level-Copyleft; die Datei behaelt ihre Lizenz, ihr Quelltext liegt
   unveraendert bei, und sie faerbt nichts anderes ein. Es wird KEIN MPL-Code uebersetzt oder
   gelinkt -- der Header enthaelt nur zwei Funktionsdeklarationen, und `USE_STANDARD_TMPFILE`
   schaltet `utility.c:667` auf das POSIX-`tmpfile()` um. Beleg am Objekt:
   `nm libcomdare_vendored_xlsxwriter.a | grep -ci tmpfileplus` -> `0`, waehrend `utility.c.o`
   ein `U tmpfile` (libc) traegt.

## Bau

Das CMake-Target `comdare_vendored_xlsxwriter` (siehe `CMakeLists.txt` hier) baut statisch:
`src/*.c` + `third_party/minizip/{ioapi,zip}.c` + `third_party/md5/md5.c` +
`third_party/dtoa/emyg_dtoa.c`. Die Quell-Auswahl der `third_party`-Anteile entspricht exakt dem
Upstream-Bau (`src/Makefile` am Tag: `MINIZIP_OBJ = ioapi.o zip.o`).

Bau-Schalter:

- `USE_STANDARD_TMPFILE` -- POSIX `tmpfile()` statt tmpfileplus (kein MPL-Code, s.o.). Traegt Linux
  (x86/ARM/RISC-V) und macOS gleichermassen; Windows ist nicht in der Flotte.
- `USE_DTOA_LIBRARY` -- Milo-Yip-dtoa (MIT) statt `sprintf()` fuer die double-Ausgabe:
  locale-unabhaengig und byte-deterministisch. Beleg am Objekt: `utility.c.o` traegt
  `U emyg_dtoa`, aufgeloest aus `emyg_dtoa.c.o` (`T emyg_dtoa`).
- `NOCRYPT` / `NOUNCRYPT` -- minizip ohne Verschluesselungs-Zweig (Upstream-Default fuer
  libxlsxwriter).
- NICHT gesetzt: `USE_FMEMOPEN` (`open_memstream`), `USE_OPENSSL_MD5`, `USE_NO_MD5`,
  `USE_SYSTEM_MINIZIP`.

Abhaengigkeit: `comdare::vendored_zlib` (`ext/io/zlib`), PRIVATE gelinkt -- `<zlib.h>` gehoert zum
Bau von minizip/packager, nicht zur Consumer-Schnittstelle. Consumer-only wie liburing: das Target
wird nur gezogen, wenn ein Consumer es hereinholt (NOT-TARGET-Guard). Am Ist ist der einzige
Consumer der Rauchtest `tests/unit/test_a9_xlsx_vendor_rauchtest.cpp`; kein Produktions-Target und
kein golden-/Mess-Pfad haengt daran.

## Beleg, dass der Snapshot laeuft

`tests/unit/test_a9_xlsx_vendor_rauchtest.cpp` schreibt eine echte `.xlsx`, oeffnet sie wieder als
ZIP und liest die Zellen zurueck (eigener ZIP-Leser ueber die Zentral-Directory, entpackt mit dem
vendorierten zlib -- kein Python, kein externes Werkzeug). Damit haengt der Beleg an BEIDEN
Snapshots. "Es kompiliert" waere kein Beleg.

## Offen / Merkposten

- Die Locale-Negativprobe (`LC_NUMERIC` mit Komma-Dezimaltrenner => byte-identische Ausgabe) ist im
  Design-Dossier der Scheibe A9-S3 zugeordnet und steht hier NICHT als bestanden: auf diesem Host
  ist ueberhaupt kein Komma-Dezimal-Locale erzeugt (`locale -a` kennt nur `C`, `C.utf8`,
  `en_US.utf8`, `POSIX`), die Probe waere hier also stillschweigend wirkungslos. S1 belegt
  stattdessen am Objekt, dass der dtoa-Zweig aktiv ist (s.o.).
