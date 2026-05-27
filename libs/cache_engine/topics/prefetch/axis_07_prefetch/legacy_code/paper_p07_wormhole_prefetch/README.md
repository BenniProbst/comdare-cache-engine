# paper_p07_wormhole_prefetch — Wormhole Prefetch-Pattern (Wu EuroSys 2019)

**Paper-ID:** P07
**Topic:** axis_07_prefetch
**Wrapper:** HardwarePrefetch

## Paper-Referenz

- **Titel:** Wormhole: A Fast Ordered Index for In-memory Data Management
- **Autoren:** Xingbo Wu, Fan Ni, Song Jiang
- **Venue:** Proceedings of the Fourteenth EuroSys Conference (EuroSys 2019)
- **Jahr:** 2019
- **DOI:** 10.1145/3302424.3303955
- **URL:** https://dl.acm.org/doi/10.1145/3302424.3303955

## Original-Code

- **Repository:** https://github.com/wuxb45/wormhole
- **License:** GPL-3.0
- **Kuratiert in Forschungsarbeiten:** `Forschungsarbeiten/code/P07-Wormhole/wormhole/`
- **Kopiert hier:** `src/wh.c`, `src/wh.h` (vor weiterer Kuratur)

## Prefetch-Pattern

Wormhole nutzt PREFETCHT0-Instruction (T0 = höchste Cache-Hierarchie) explizit
auf Hash-Anchor-Chain ohne Software-Heuristik. CPU schaetzt Distance autonom.

Relevante Funktionen:
- `wh_iter_seek_anchor_*` — Pre-Loads next anchor before search

## Habich-Compliance

- **Status:** Original-Code kopiert (Kopie-vor-Kuratur, User-Direktive 2026-05-27)
- **Compiler:** GCC 7+ (Original-Anforderung)
- **Mixin-Inheritance:** PrefetchOriginalCodeMixin (TBD — siehe Task R7.6.b axis_07)
- **is_original_module:** wird zur Build-Time gepruefft via apps/is_original_validator
