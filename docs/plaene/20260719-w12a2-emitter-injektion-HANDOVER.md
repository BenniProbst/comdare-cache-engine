# W12-A2 — Emitter-Injektion der Versionierungs-Stempel: chirurgischer HANDOVER (19.07. nachts)

> Quelle: Abschluss-Deliverable des W12-A-Agenten (Ausstiegspfad statt Muede-Edit am golden-Pfad — bewusste
> Entscheidung, vom Architekten angenommen). Gruene Baseline: alle W12-A-Bausteine liegen committet (ce 6771e736,
> Sammel-Kadenz 10805cfe), Suite 287/287, Byte-Wache aktiv, CRC64 unberuehrt. Ein FRISCHER Agent setzt NUR die
> 5 Edits + 6-Punkt-Verifikation um. Fork-Entscheid A.1 (geteilter Helfer, Round-Trip STRIKT, kein modulo).

## Gruene Baseline (vorhanden + verifiziert)
- measurement/algo_semver.hpp (X.Y.Z-Parse) · measurement/axis_version_stamp.hpp (Formatter)
- abi/anatomy_version_stamp.hpp (system_stamp_line(); organ_stamp_line<Comp> = MOCK-ONLY, s. Fallstricke)
- profile_facade/planner/planner_version.hpp (planner_version_stamp())
- experiment_tree/axis_variant_version_table.hpp -> **compose_organ_stamp_line(axes, table)** = der GETEILTE Helfer
- Byte-Wache in tests/unit/test_reflect_versions_all17.cpp · tests/unit/test_m_w12_stamp_bausteine.cpp

## Die 5 Edits (exakte Stellen)
**E1 — POD + Stempel-Makro** (leicht, KEINE Version-Tabelle im Modul):
- abi/anatomy_module_abi_v1_decl.hpp NACH dem extern-"C"-Block der 4 Pflicht-Symbole (~Z.102):
  POD `AnatomyVersionLines {uint32 stamp_layout_version; uint32 reserved; char const* organ_line; uint64 organ_len;
  char const* system_line; uint64 system_len;}` in namespace comdare::cache_engine::abi
  + `inline constexpr uint32 kAnatomyVersionLinesLayout=1;`
  + `extern "C" { COMDARE_ANATOMY_ABI_EXPORT AnatomyVersionLines const* comdare_anatomy_version_lines() noexcept; }`
  (OPTIONALES Symbol -> KEIN Major-Bump; Loader verlangt weiter nur die 4.)
- abi/anatomy_module_abi_v1.hpp: Makro (nur String-Literale, constexpr, kein std::string im Modul):
```
#define COMDARE_ANATOMY_VERSION_STAMP(organ_lit, system_lit)                                    \
  extern "C" COMDARE_ANATOMY_ABI_EXPORT ::comdare::cache_engine::abi::AnatomyVersionLines const*\
  comdare_anatomy_version_lines() noexcept {                                                    \
    static constexpr char kO[] = organ_lit; static constexpr char kS[] = system_lit;            \
    static constexpr ::comdare::cache_engine::abi::AnatomyVersionLines kL{                       \
      ::comdare::cache_engine::abi::kAnatomyVersionLinesLayout,0u,kO,sizeof(kO)-1,kS,sizeof(kS)-1};\
    return &kL; }
```
**E2 — render_adhoc_module_source erweitern** (builder/codegen/adhoc_emitter.hpp:83): Signatur
`+ std::string_view organ_stamp = {}, std::string_view system_stamp = {}`; im Return NACH der
COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(...)-Zeile: bei nicht-leerem organ_stamp die Zeile
`COMDARE_ANATOMY_VERSION_STAMP("<organ>", "<system>")` anhaengen (Stempel-Strings sind C-literal-sicher:
nur =@;.+alnum). SHAPED-Zwilling (adhoc_emitter_shaped.hpp) analog, falls SHAPED-Stempel gewuenscht.
**E3 — lazy-Aufrufer** (profile_facade/lazy_adhoc_source_gen.hpp:184-187, lazy_adhoc_source_for):
version_table EINMAL bauen (shared_ptr in make_lazy_adhoc_source_gen, wie die tables);
axes = ex::ceb_parse_path(binary_id); organ = ex::compose_organ_stamp_line(axes, *table);
system = abi::system_stamp_line(); an render_adhoc_module_source(0, macro_args, organ, system).
**E4 — Katalog-Aufrufer** (builder/experiment_tree/pilot_source_map.hpp:42): path=binary_id vorhanden;
Tabelle EINMAL vor der mp_for_each-Schleife; render_adhoc_module_source(idx, adhoc_macro_args<Comp>(),
compose_organ_stamp_line(ceb_parse_path(path), table), system_stamp_line()).
-> BEIDE Pfade: derselbe Helfer + derselbe binary_id -> BYTE-IDENTISCH -> Round-Trip STRIKT.
**E5 — Katalog regenerieren:** apps/catalog_codegen_tool/main.cpp bauen+laufen lassen ->
generated_source_catalog.hpp neu erzeugen (generierte Datei NIE handeditieren) -> committen (cf22 durch Architekt).

## 6-Punkt-Pflicht-Verifikation (build-conf)
1. `ctest -R test_lazy_adhoc_source_gen --output-on-failure` — 320-Byte-Diff Katalog-vs-lazy == 0 + CRC64-Fall gruen (STRIKT).
2./3. `ctest -R "test_reflect_versions_all17|test_m_w12"` — Byte-Wache .algos + Bausteine.
4. Voll: `ctest -E test_v41_anatomy_adhoc_autobuilt_load` == 100% (287+).
5. super-Sub-Build: messung_driver linkt.
6. In-Binary-Beweis: ADHOC-Modul bauen + `strings <perm>.so | grep '@1.0.0'` bzw. `objdump -T | grep comdare_anatomy_version_lines`.

## Fallstricke
- CRC64 0xF1C1F26A1232073B (source_catalog.hpp) NICHT anfassen — Stempel steht im Source-VALUE, nicht im binary_id-KEY.
- organ_stamp_line<Comp> (anatomy_version_stamp.hpp) ist MOCK-ONLY — fuer die reale Injektion compose_organ_stamp_line(axes,table) nutzen.
- Beide Aufrufer MUESSEN denselben compose_organ_stamp_line + ceb_parse_path(binary_id) nutzen, sonst kippt der Round-Trip.
- ASCII-only. Kein Commit durch den Agenten (Integration durch Architekt).
- W12-B-Anschluss danach: Stempel in ArtifactCache::cache_key_prefix einhaengen + YAML-GN_PREFIX (~Z.738) SPIEGELBILDLICH (sonst Pull-Key != Push-Key).
