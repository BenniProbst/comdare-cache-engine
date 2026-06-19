# Build-Modell-Klarstellung: 1 DLL = 1 TU je Konfiguration (code-belegt)

> **User-Korrektur 2026-06-19, code-verifiziert (Workflow `wuh70wwyi`, 2 unabhängige Linsen + file:line).** Korrigiert die
> verbreitete Falsch-Aussage „ein Forscher soll EINE neue Achse erforschen können, ohne alles andere neu zu bauen". Grundlage für
> den Erweiterungs-Leitfaden (#169) und jede Thesis-Aussage zum Build-Modell.

## Die Falsch-Aussage und ihre Richtigstellung
- ❌ **FALSCH (im Bau-Sinne):** „eine neue Achse erforschen, OHNE alles andere neu zu bauen."
- ✅ **RICHTIG (User):** „Wir bauen IMMER für eine Konfiguration die dafür nötigen DLL neu." `inkrementell_moeglich = false`.

„Ohne alles andere neu zu bauen" gilt **NUR für die QUELLE** (man muss die 18 anderen Achsen-Organe nicht umschreiben/neu
implementieren) — **NICHT für den BAU**.

## Das tatsächliche Build-Modell (file:line)
**`1 DLL = 1 TU = GENAU EINE vollständige 19-Achsen-Komposition` je `binary_id`:**
1. Je Permutation emittiert der Treiber einen eigenständigen Modul-`.cpp`, dessen GESAMTER Quelltext nur lautet:
   `#include <builder/codegen/all_axes_umbrella.hpp>` + `COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<19 FQ-Achsen-Typen>)`
   (`adhoc_emitter.hpp:76-103`; `adhoc_macro_args<C>` :46-70 serialisiert ALLE 19 Slots search_algo..queuing_q2).
2. Der EINE Umbrella zieht **alle 19 Achsen-Registry-Header** (`all_axes_umbrella.hpp:19-40`). Es gibt **keine geteilte vorkompilierte
   Achsen-Objektbibliothek** → **jede Lebewesen-TU kompiliert alle 19 Achsen-Organ-Header neu** in ihre eigene TU.
3. Das ADHOC-Makro materialisiert `AdHocComposition<19>` + Anatomie + AbiAdapter + 4 ABI-Symbole IN dieser einen TU
   (`anatomy_module_abi_v1.hpp:75-78`); `cl /LD` baut daraus genau EINE `.dll` (`build_orchestrator.hpp:333-339`).
   Bestätigt durch den code-tragenden Kommentar `source_catalog.hpp:24-26` („eine DLL = ein TU = EINE Komposition").

## Wann neu gebaut vs. übersprungen (die einzige Inkrementalität = Resume)
- **Build-Ebene (Resume):** `build_orchestrator.hpp:150-159` `dll_is_current` = DLL existiert UND `.version`-Sidecar == geforderte
  `build_version` → Skip; sonst Rebuild. Trigger = neuer `binary_id` (neuer Pfad) ODER geänderte `build_version`.
- **Mess-Ebene (Resume #139):** `cache_engine_builder_iterator.hpp` `lazy_try_resume_binary` — Skip nur bei `result.csv`-Stamp-Match
  (build_version + n_ops + seed + records + dyn-Dims + XML-Lastprofil-Inhalt) + Header-Identität + exakter Zeilenzahl.

## Was beim Erweitern wiederverwendet vs. neu gebaut wird
- **WIEDERVERWENDET (Quelle, kein Rebuild der Quelle):** die Header/Registries der 18 anderen Achsen-Organe (`all_axes_umbrella.hpp`),
  das ADHOC-Makro, der Emitter, die Typ→Quelle-Brücke (`pilot_source_map.hpp:39-44`), BuildOrchestrator, Iterator, Profil.
- **NEU GEBAUT (DLLs):** **ALLE** Konfigurationen, die die neue Achse/den neuen Algorithmus nutzen — jede als eigene, **volle
  19-Achsen-DLL**.

## Der reale Framework-Nutzen (NICHT „weniger bauen")
1. **Modular wiederverwendbare Achsen-Organ-QUELLE** — ein Forscher schreibt EIN neues Organ (Header + Registry nach Goldstandard),
   ohne die 18 anderen Quellen umzuschreiben.
2. **Permutations-Engine komponiert** das neue Organ typ-getrieben mit allen anderen (`pilot_source_map.hpp:39-44`
   `Engine::for_each_permutation → CompositionFromPermTuple<P>`).
3. **Transparente Messung pro Achse UND ganzer Algorithmus.**
Der Hebel ist **kompositionelle Wiederverwendung der QUELLE + transparente Messung**, NICHT reduzierter Build-Umfang.

> **Konsequenz für #169 (Erweiterungs-Leitfaden):** Der Leitfaden muss ehrlich sagen: „neue Achse/Algorithmus = neue QUELLE
> (modular, andere unberührt), aber **jede betroffene Konfiguration wird neu gebaut**." Keine Versprechung von „inkrementellem Bau".
