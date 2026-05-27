# MODIFICATIONS.md — paper_a04_mimalloc

**Stand:** 2026-05-26 (V41.F.6.1.P2.B Pilot)

Dokumentation aller Modifikationen an der lokalen kuratierten Kopie der
mimalloc-Sources. Bei Modifikation MUSS hier ein Eintrag mit
[[legacy-code-sha256-validation]]-konformer Beschreibung erfolgen.

## Aktuelle Modifikationen

**Keine.**

Die Sources werden zur Build-Time 1:1 aus `ext/A04-mimalloc/` kopiert
(via `comdare_paper_init()`) — kuratierte Snapshot ohne Anpassungen.
`sha256_locked.txt` validiert dass die Sources byte-identisch zur
Upstream-Original-Distribution sind.

## Modifikations-Workflow (wenn doch noetig)

1. Source unter `legacy_code/paper_a04_mimalloc/src/...` editieren
2. Tool `apps/is_original_validator` neu laufen lassen (CMake-Build)
3. Tool erkennt SHA-Mismatch → `kIsOriginal_<fn> = false` automatisch
4. Hier in dieser Datei dokumentieren:
   - **Datum**, **Funktion**, **Grund** (z.B. Win-Build-Fix), **Beschluss-Verweis**
5. `is_original_module()` wird `false` → Wrapper-Klassifikation "ADAPTED"
   in Diplomarbeit-Reports (statt "ORIGINAL ✓")

## Beispiel-Eintrag (wenn benoetigt)

```
### 2026-XX-XX: mi_free Windows-Build-Fix
- **Funktion:** mi_free (src/free.c)
- **Grund:** MSVC verarbeitet TLS-Variable im no-op-Pfad anders als GCC
- **Loesung:** `#ifdef _MSC_VER` Branch hinzugefuegt fuer no-op-Sentinel-Check
- **SHA-Konsequenz:** kIsOriginal_deallocate = false
- **Beschluss-Verweis:** Session-Doku 20260XYZ-V41-F-6-1-msvc-mimalloc-fix.md
```
