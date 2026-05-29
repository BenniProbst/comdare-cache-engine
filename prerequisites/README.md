# Prerequisites — externe Abhängigkeits-Archive

Dieses Verzeichnis ist der **Standard-Ablageort für externe Prerequisite-Archive** der Cache-Engine
(derzeit **Boost** für die header-only Bibliothek **Boost.MP11**, die das Permutations-Metaprogramming trägt).

## Wie es funktioniert (autonom bei der Einrichtung)

`cmake/boost_mp11_setup.cmake` löst Boost.MP11 beim `configure`-Schritt in dieser Reihenfolge auf:

1. **Vendored** — `cmake/third_party/boost_mp11/include/boost/mp11.hpp` ist im Repo getrackt
   (klein, header-only, BSL-1.0). → **Offline-Build out-of-the-box, kein Download nötig.**
2. **Prerequisite-Archiv** — fehlt (1), wird ein Archiv aus *diesem* Verzeichnis (oder
   `-DCOMDARE_BOOST_ARCHIVE=<pfad>`) **autonom entpackt** (`file(ARCHIVE_EXTRACT)`, libarchive →
   `tar.bz2` / `tar.gz` / `tar.xz` / `7z` / `zip`); nur der `boost/mp11`-Teilbaum wird vendored.
3. **FetchContent** — fehlen (1) und (2), wird `boostorg/mp11 @ boost-1.91.0` von GitHub geladen
   (nur bei Internet-Egress).

Expliziter, eigenständiger Einrichtungs-Pfad (z. B. CI), ohne vollen `configure`:

```sh
# Linux / macOS / Git-Bash
tools/prerequisites/bootstrap_prerequisites.sh  [pfad/zu/boost_archiv]
```
```bat
REM Windows
tools\prerequisites\bootstrap_prerequisites.bat  [pfad\zu\boost_archiv]
```

## Welche Archive hier ablegen

- **Linux/macOS:** `boost_1_91_0.tar.bz2` (oder `.tar.gz` / `.tar.xz`)
- **Windows:** `boost_1_91_0.7z` oder `boost_1_91_0.zip`

> Quelle: offizielle Boost-1.91.0-Distribution. Es genügt ein beliebiges davon — `file(ARCHIVE_EXTRACT)`
> zieht nur die mp11-Header heraus. (Standalone `boostorg/mp11`-Archive werden ebenfalls erkannt.)

## Warum die Archive NICHT in Git liegen

Die Boost-Archive sind ~200 MB und überschreiten das **GitHub-100-MB-Datei-Limit** → sie sind in
`.gitignore` ausgenommen. Getrackt sind nur der **kleine vendored mp11-Header-Satz**
(`cmake/third_party/boost_mp11/`), diese README und die Bootstrap-/Provision-Skripte.
Lege die großen Archive lokal hier ab (oder verweise per `COMDARE_BOOST_ARCHIVE`).
