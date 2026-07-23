# VENDOR PROVENANCE -- liburing (G3 / #46b Lager-Gate, Scheibe B7)

Stufe-1-Vendor (vendor -> faithful -> self-contained; hier Stufe 1 = faithful copy) des
io_uring-User-Space-Wrappers liburing, fuer das io_uring-Writer-Backend des Bestandslog-
Spools (`libs/cache_engine/builder/artifact_transport/writer_backend_io_uring.hpp`).

## Quelle

- Upstream:   https://github.com/axboe/liburing
- Tag:        liburing-2.6
- Tag-Objekt: 96141c985b1e6bcb082759627071c762563099c2  (annotiertes Tag)
- Commit:     f7dcc1ea60819475dffd3a45059e16f04381bee7  (Ziel des Tags)
- Bezogen:    2026-07-23 via `git clone --depth 1 --branch liburing-2.6`
- Lizenz:     MIT (siehe LICENSE / COPYING im Verzeichnis; liburing ist MIT bzw. dual MIT/GPL)

## Was am Checkout geaendert wurde (faithful, minimal)

1. `.git/` entfernt (Vendor, kein Submodule -- Haus-Doktrin: keine Git-Submodules).
2. `./configure` EINMAL ausgefuehrt, um die generierten Header zu erzeugen; davon BEHALTEN:
   - `src/include/liburing/compat.h`         (von liburing.h inkludiert -- NOETIG)
   - `src/include/liburing/io_uring_version.h`(Versions-Makros -- NOETIG)
   ENTFERNT (Host-spezifische Build-Artefakte, nicht vom C++-Bau gebraucht):
   - `config-host.h`, `config-host.mak`, `config.log`
   - `man/IO_URING_VERSION_MAJOR.3`, `man/IO_URING_VERSION_MINOR.3` (configure-generierte Man-Pages)
3. GEPRUNT auf den essentiellen Bibliotheks-Satz (kleines statisches Target, Repo-Gewicht): BEHALTEN
   `src/` (der komplette Bibliotheks-Quelltext 1:1), `LICENSE`, `COPYING`, `COPYING.GPL`, `README`
   sowie die zwei Comdare-Wrapper (`CMakeLists.txt`, diese Datei). ENTFERNT (fuer unseren Bau nicht
   gebraucht, per Tag jederzeit wiederherstellbar): `test/`, `examples/`, `man/`, `debian/`,
   `configure`, `Makefile*`, `*.pc.in`, `make-debs.sh`, `liburing.spec`, `CHANGELOG`, `CITATION.cff`,
   `SECURITY.md`. Der Quelltext unter `src/` selbst ist 1:1 unveraendert (nur nicht alle .c werden
   kompiliert -- s.u.).

## Bau (Mode B: Standard-libc, NICHT nolibc)

Das CMake-Target `comdare_vendored_liburing` (siehe `CMakeLists.txt` hier) baut die fuenf Kern-
Quellen `src/{setup,queue,register,syscall,version}.c` gegen die Standard-libc mit
`-D_GNU_SOURCE -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64` und Include `src/include`.

Bewusst NICHT der nolibc-Modus: der wuerde `config-host.h` (`#define CONFIG_NOLIBC`) voraussetzen,
wodurch `lib.h` `memset`/`malloc` auf liburings interne `__uring_memset`/`__uring_malloc`
(`src/nolibc.c`, `-nostdlib -ffreestanding`) umbiegt. Da das Backend ohnehin in eine libc-basierte
C++-Binary linkt, ist der Standard-libc-Bau korrekt, portabel ueber die 8er-Docker-Matrix und ohne
libgcc-/freestanding-Sonderfaelle. `config-host.h` wird deshalb NICHT `-include`d.

Nur im io_uring-Pfad gebaut: das Target wird ausschliesslich gezogen, wenn ein Consumer es braucht
(COMDARE_WRITER_BACKEND=io_uring bzw. der io_uring-Unit-Test).

## Verworfene Alternative (Kontext)

Eine erste self-contained UAPI-Fassung (rohe io_uring_setup/mmap/io_uring_enter ueber
<linux/io_uring.h>) wurde zugunsten dieses liburing-Vendors verworfen (Vendoring-Doktrin ist
geordnet: vendoren, wenn moeglich). liburing macht die SQ/CQ-Ring-Barriers korrekt; die rohe
Syscall-Handarbeit entfaellt.

## NACHTRAG 23.07. (CI-Fix untracked-generated-headers)
Die upstream .gitignore des Snapshots schloss die von ./configure generierten Header
(src/include/liburing/compat.h, io_uring_version.h) vom Commit aus -> CI-Builds brachen
(liburing/compat.h: No such file). Da eine .gitignore in einem Vendor-SNAPSHOT funktionslos
ist und aktiv schadet, wurde sie ENTFERNT und die generierten Header sind jetzt Teil des
Snapshots (Mode-B-Build braucht sie).
