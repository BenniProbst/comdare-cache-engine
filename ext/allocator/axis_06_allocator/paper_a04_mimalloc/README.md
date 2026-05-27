# paper_a04_mimalloc — Kuratierte mimalloc-Snapshot

**Stand:** 2026-05-26 (V41.F.6.1.P2.B Pilot)
**Topic:** allocator
**Achse:** axis_06_allocator
**Wrapper-Klasse:** `comdare::cache_engine::allocator::axis_06_allocator::MimallocAllocator`

## Paper-Referenz (Vollangabe)

Leijen, D. *Mimalloc: Free List Sharding in Action.* Microsoft Research
Technical Report MSR-TR-2019-18, June 2019.

ISMM 2019 Conference Paper:
Leijen, D., Zorn, B., de Moura, L. *Mimalloc: Free List Sharding in
Action.* In Proceedings of the 2019 ACM SIGPLAN International Symposium
on Memory Management (ISMM '19), Phoenix, AZ, USA, June 23, 2019, ACM,
pp. 53-66. DOI: 10.1145/3315573.3329987

## Quellen

- **Upstream-Repo:** https://github.com/microsoft/mimalloc
- **Lizenz:** MIT (siehe LICENSE)
- **Cache-Engine-Kopie aus:** `ext/A04-mimalloc/` (User-Original-Distribution,
  unkuratiert)

## Curation-Pattern

User-Direktive 2026-05-26:
> "Original Paper sources vorher kopieren, weil ich nur eine Version davon
> besitze, ich behalte also die unkuratierte und die Cache Engine hat eine
> Kopie die wir anpassen können"

Workflow:
- `ext/A04-mimalloc/` = unangetastete User-Original-Distribution (Read-Only)
- `legacy_code/paper_a04_mimalloc/` = kuratierte Cache-Engine-Kopie
- `comdare_paper_init()` kopiert src+include zur Build-Time aus ext/ hierher
- `sha256_locked.txt` (auto-generated) trackt Source-Identitaet
- `manifest.txt` (manuell) definiert Function-Mappings + Compiler

## Files in dieser Bibliothek

| Datei | Inhalt | git-tracked |
|---|---|---|
| `LICENSE` | MIT-Lizenz (Kopie von ext/A04-mimalloc/LICENSE) | ja |
| `README.md` | Diese Datei | ja |
| `compiler_info.txt` | Empfohlene Compiler-Flags | ja |
| `manifest.txt` | Function-Mappings + @-Annotations fuer is_original_validator | ja |
| `MODIFICATIONS.md` | Dokumentation lokaler Anpassungen (leer bei Erstinstanziierung) | ja |
| `sha256_locked.txt` | Auto-generated Source-Hashes (First-Build-Init) | ja |
| `.extracted.marker` | Marker fuer comdare_paper_init Idempotenz | nein (gitignored) |
| `src/*.c`, `src/*.h` | Source-Files kopiert aus ext/A04-mimalloc/src/ (Build-Time) | nein (gitignored) |
| `include/*.h` | Header-Files kopiert aus ext/A04-mimalloc/include/ (Build-Time) | nein (gitignored) |

## Cross-Refs

- Architektur: `docs/architektur/13_paper_legacy_code_architektur.md`
- Achsen-Mixin: `topics/allocator/axis_06_allocator/concepts/axis_06_allocator_original_code_mixin.hpp`
- Wrapper: `topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp`
- Memory: `[[paper-original-code-pattern]]` `[[legacy-code-sha256-validation]]` `[[axis-base-pattern]]`
