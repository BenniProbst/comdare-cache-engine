# Changelog -- comdare-cache-engine

This repository cuts no versioned releases before the thesis submission
(2026-09-15). Its authoritative change record is the KON chain of the umbrella
ledger `probst-diplomarbeit-cache-engine/docs/DIPLOMARBEIT-ZIELE-OFFENE-PUNKTE-LEDGER.md`
(numbered KON entries, newest first; KON128 as of 2026-08-25). The repo-local
view is `docs/ledger-sections/architektur-ziele-offene-punkte-ledger.md`
(superseded banner: the umbrella ledger wins on conflict); session protocols
live under `docs/sessions/`. This file only points at that chain and lists
user-visible changes of the repository layout and licensing.

The format follows Keep a Changelog; dates are ISO (YYYY-MM-DD).

## [Unreleased]

### Added
- 2026-08-25 (A5 / D3a, D4): REUSE 3.3 layout -- `REUSE.toml` (own code
  `LicenseRef-Comdare-Research-1.0`; every `ext/` package with its own license
  and copyright holders; vendored cells under `libs/` as they are licensed on
  disk) and `LICENSES/` with all referenced license texts; community health
  files (`CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, `CITATION.cff`,
  this file). No source file was touched.

## License history (binding text: `LICENSE` Section 9)
- until 2026-08-01: Apache License 2.0.
- 2026-08-02 to 2026-08-09: dual Research-and-Teaching / Commercial license.
- from 2026-08-10: Comdare Research License, Version 1.0
  (`LicenseRef-Comdare-Research-1.0`); Change Date 2031-08-10, then Apache-2.0.
  Earlier grants remain valid for the revisions they covered.

## History
- Up to KON128 (2026-08-25): see the umbrella ledger. The KON chain is the
  changelog of this project; nothing is recorded here that is not booked there.
