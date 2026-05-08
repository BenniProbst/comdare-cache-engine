"""Lizenz-Analyse aller geklonten Repos in ext/.

Pruefe: existiert LICENSE/COPYING/NOTICE? Lese die ersten 50 Zeilen.
Output: Markdown-Tabelle fuer LIZENZEN_UEBERSICHT.md
"""
from pathlib import Path

ROOT = Path(__file__).parent / "ext"

LICENSE_NAMES = ["LICENSE", "LICENSE.md", "LICENSE.txt", "COPYING", "COPYING.md",
                 "COPYING.txt", "LICENCE", "license", "license.txt", "MIT-LICENSE",
                 "LICENSE.MIT", "LICENSE.APACHE", "LICENSE-MIT", "LICENSE-APACHE",
                 "NOTICE", "NOTICE.md"]


def find_license_files(repo_dir: Path):
    found = []
    for name in LICENSE_NAMES:
        path = repo_dir / name
        if path.exists() and path.is_file():
            found.append(path)
    # Auch GitHub-typisches: LICENSE in /docs oder /licenses
    for sub in ["docs", "licenses"]:
        subdir = repo_dir / sub
        if subdir.is_dir():
            for name in LICENSE_NAMES:
                path = subdir / name
                if path.exists() and path.is_file():
                    found.append(path)
    return found


def detect_license_type(content: str) -> str:
    """Heuristische Erkennung des Lizenztyps."""
    text = content.lower()[:5000]
    # Reihenfolge: spezifischere zuerst
    if "apache license" in text and "version 2.0" in text:
        return "Apache-2.0"
    if "mit license" in text or "permission is hereby granted, free of charge" in text:
        return "MIT"
    if "bsd" in text and ("redistribution and use" in text or "3-clause" in text or "2-clause" in text):
        if "neither the name" in text or "3-clause" in text:
            return "BSD-3-Clause"
        return "BSD-2-Clause" if "endorsed" not in text else "BSD-3-Clause"
    if "gnu lesser general public license" in text or "gnu lgpl" in text or "lgpl" in text:
        if "version 3" in text:
            return "LGPL-3.0"
        if "version 2.1" in text:
            return "LGPL-2.1"
        return "LGPL"
    if "gnu general public license" in text or "gnu gpl" in text or "gpl" in text:
        if "version 3" in text:
            return "GPL-3.0"
        if "version 2" in text:
            return "GPL-2.0"
        return "GPL"
    if "mozilla public license" in text:
        return "MPL-2.0"
    if "isc license" in text:
        return "ISC"
    if "creative commons" in text or "cc-" in text:
        return "Creative-Commons"
    if "boost software license" in text:
        return "BSL-1.0"
    if "the unlicense" in text or "public domain" in text:
        return "Unlicense/Public-Domain"
    return "UNBEKANNT"


def apache_compatibility(license_type: str) -> tuple[str, str]:
    """Rueckgabe: (Status, Hinweis) fuer Apache-2.0-Kompatibilitaet."""
    table = {
        "Apache-2.0":             ("KOMPATIBEL", "Direkt kompatibel; identische Lizenz"),
        "MIT":                    ("KOMPATIBEL", "Permissive, kompatibel mit Apache 2.0"),
        "BSD-2-Clause":           ("KOMPATIBEL", "Permissive, kompatibel"),
        "BSD-3-Clause":           ("KOMPATIBEL", "Permissive, kompatibel"),
        "ISC":                    ("KOMPATIBEL", "Permissive, kompatibel"),
        "BSL-1.0":                ("KOMPATIBEL", "Boost License, kompatibel"),
        "MPL-2.0":                ("KOMPATIBEL_BEDINGT", "MPL-2.0 file-level copyleft; ext/-Isolation reicht"),
        "LGPL-2.1":               ("KOMPATIBEL_BEDINGT", "LGPL: dynamisches Linking erlaubt; statisches Linken (F-EXTRA-1!) BEDARF Quellcode-Offenlegung"),
        "LGPL-3.0":               ("KOMPATIBEL_BEDINGT", "LGPL-3 noch strenger; F-EXTRA-1 statisches Linken kritisch — pruefen"),
        "GPL-2.0":                ("INKOMPATIBEL", "GPL-Copyleft — nicht in Apache-2.0-Projekt einbindbar bei statischem Linking"),
        "GPL-3.0":                ("INKOMPATIBEL", "GPL-3 inkompatibel mit Apache 2.0 (statisch); waere komplette Re-Implementation"),
        "Creative-Commons":       ("PRUEFEN", "CC-Lizenzen sind nicht fuer Software gedacht; Klaerung mit Autor"),
        "Unlicense/Public-Domain":("KOMPATIBEL", "Public Domain, voll kompatibel"),
        "UNBEKANNT":              ("PRUEFEN", "Keine LICENSE-Datei gefunden — DRINGEND klaeren"),
    }
    return table.get(license_type, ("PRUEFEN", "Unbekannter Lizenztyp"))


def main():
    print("# Lizenz-Analyse — geklonte Repos in ext/")
    print()
    print("Stand:", "automatisch generiert via _analyze_licenses.py")
    print()
    print("| P-ID | Repo | Lizenz-Datei | Lizenztyp | Apache-2.0-Kompatibel? | Hinweis |")
    print("|------|------|--------------|-----------|------------------------|---------|")

    for paper_dir in sorted(ROOT.iterdir()):
        if not paper_dir.is_dir():
            continue
        for repo_dir in sorted(paper_dir.iterdir()):
            if not repo_dir.is_dir():
                continue
            license_files = find_license_files(repo_dir)
            if not license_files:
                license_type = "UNBEKANNT"
                license_files_str = "keine"
            else:
                # Nimm die erste gefundene
                primary = license_files[0]
                try:
                    content = primary.read_text(encoding="utf-8", errors="replace")
                except Exception:
                    content = ""
                license_type = detect_license_type(content)
                license_files_str = ", ".join(p.name for p in license_files)

            status, note = apache_compatibility(license_type)
            print(f"| {paper_dir.name} | {repo_dir.name} | {license_files_str} | {license_type} | {status} | {note} |")


if __name__ == "__main__":
    main()
