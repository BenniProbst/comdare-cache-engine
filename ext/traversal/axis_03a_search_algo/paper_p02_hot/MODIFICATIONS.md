# MODIFICATIONS.md — paper_p02_hot

Keine direkten Modifikationen am Original-Code. Aber:

**Luecken-Fueller (NICHT-Original):**
- erase: HOT ist append-only, kein remove im Paper. Cache-engine Wrapper liefert eigene erase()-Impl.
- clear: HOT hat kein clear im Paper. Cache-engine Wrapper liefert eigene clear()-Impl.

Beide markiert via `is_original_erase()=false` und `is_original_clear()=false`
(via Achs-Mixin if constexpr requires Default-Pattern).
