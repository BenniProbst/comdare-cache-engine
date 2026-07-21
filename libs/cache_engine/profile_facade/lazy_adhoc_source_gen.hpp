#pragma once
// INC-G6 (Ledger 33/34/35, 2026-07-19) -- lazy_adhoc_source_gen: der LAZY Per-Index-Adhoc-Emitter, der zu
// JEDER golden-N-`binary_id` die REALE `perm_<id>.cpp`-Modul-Quelle erzeugt, OHNE das `mp_product` zu
// materialisieren. Er ist der Codegen-Umbau (BAUPLAN-These B, docs/plaene/20260719-inc-g6-materialisierung-
// BAUPLAN.md Abschnitt 1): ein PER-BINARY-Quell-Emitter als Spiegel der lazy `StaticBinaryView`.
//
// WARUM (die Materialisierungs-Grenze, BAUPLAN 0.2): der bestehende `make_catalog_source_gen` /
// `generated_make_catalog_source_gen` loest Quellen ueber eine MATERIALISIERTE Map (`build_pilot_source_map`
// iteriert das VOLLE `mp_product` der Katalog-Form). Das ist compile-time nur bis zur GN-2-Grenze
// (`kMaxMaterializableCatalogCardinality = 4096`, source_catalog.hpp) moeglich -- die 2^17-Vollform waere ein
// GB-TU / g++-ICE. Fuer die ~131k golden-N-`binary_id`s ausserhalb der 320er/Sweep-Maps liefert dieser Pfad
// eine LEERE Quelle. Dieser Emitter fuellt genau diese Luecke.
//
// WIE (O(17) pro Binary, KEIN Produkt): die Bausteine existieren bereits, nur nicht lazy verdrahtet.
//   (1) `lazy_slot_type_tables()` -- je der 17 Kompositions-Achsen die KLEINE Flyweight-Tabelle
//       {W::name() -> FQ-cpp_type}, host-seitig EINMAL ueber die VOLLEN Enabled-Listen reflektiert (Muster
//       `variants_for_list`, apps/catalog_codegen_tool/main.cpp:103). Single-Source = die SELBEN
//       `TopicConfigSet::StaticAxisVariants_*`-Listen wie `CatalogAxes`/`catalog_static_levels`
//       (source_catalog.hpp) -> die Tabellen-Namen == die View-`binary_id`-Segmentwerte. 17 Slots x je
//       ~2..30 Varianten = winzig, KEIN `mp_product`.
//   (2) je angefragter `binary_id`: den Pfad in (achse,wert)-Paare zerlegen (`ex::ceb_parse_path`, die EINE
//       bestehende Parse-Konvention, ceb_generator.hpp:47), je Slot in kanonischer Reihenfolge
//       (`kCompositionAxisNames`) den cpp_type per Namens-Lookup joinen und ueber
//       `render_adhoc_module_source` (adhoc_emitter.hpp:83) rendern.
//
// BYTE-IDENTITAET zum bestehenden Katalog-Pfad (Round-Trip-Gate, test_lazy_adhoc_source_gen.cpp): der Katalog-
// Pfad rendert je Permutation `render_adhoc_module_source(idx, adhoc_macro_args<Comp>())`, wobei
// `adhoc_macro_args<C>()` je Slot `strip_all_elaborated(type_name<C::slot>())` (OHNE fuehrendes "::") mit der
// Fuge ",\n    " zusammensetzt (adhoc_emitter.hpp:52). Dieser Emitter baut die Tabellen mit EXAKT DEMSELBEN
// Transform (`cg::strip_all_elaborated(cg::type_name<W>())`) und derselben Fuge -> die gerenderten Modul-Bloecke
// sind byte-identisch zum Katalog-Pfad (modulo dem laufenden Permutations-Index im Kommentar-Kopf, der reine
// Doku ist -- der Round-Trip-Key ist der Pfad, pilot_source_map.hpp:31). Die Slot-Typen sind identisch, weil
// `anatomy::CompositionFromPermTuple<P>` die PermTuple-Werte 1:1 auf die AdHocComposition-Slots aliast
// (composition_factory.hpp:101) und die Namen je Achse eindeutig sind (Round-Trip-Garant).
//
// GN-2-KONFORM (BAUPLAN 8): der GN-2-Guard sperrt NUR den map-materialisierenden `make_catalog_source_gen`
// (build_pilot_source_map -> mp_product). Dieser lazy Emitter geht GAR NICHT durch `build_pilot_source_map`
// -- er umgeht die Materialisierung, ohne den Guard anzufassen. Der committete Systembeweis-Anker
// (`kNewGolden131072Crc64` + `static_assert(...==131072)`) bleibt unberuehrt.
//
// -- FREIGABE-KOPPLUNG System->Organ (Ledger 36/37/37.b, 2026-07-19; SIMD = PILOT eines generischen Prinzips) --
// Dieser Emitter rendert die Organ-Quelle SYSTEM-BLIND: er kennt NUR den binary_id (die 17 Organ-Achsen), NICHT
// die freigegebene System-Zelle (opt_level/simd_extension). Das ist ARCHITEKTUR-KORREKT (37.b): der
// Zulaessigkeits-Filter (Organ <= System-Freigabe) sitzt NICHT hier, NICHT im Tier und NICHT im Planer, sondern
// EXAKT an der CEB-BAU-DELEGATIONS-NAHT -- der per-System-Zelle montierten CompileFn `perm_compile`
// (profile_run_entry.hpp, run_profile optxsimd-Schleife: `perm_compile = compile_for_perm(opt_flag, march_flag)`),
// die `provision_all` je Binary VOR der Kompilation nutzt. Die CEB bestimmt aus ihrer System-Freigabe
// (system_axis_host_supports_simd + system_axis_march_of) ZUR LAUFZEIT, WAS gebaut werden darf, und DELEGIERT die
// Kompilation der gewaehlten/permutierten Organ-Rekombination (Auswahl VOR der Kompilation).
//
// SIMD-PILOT-INSTANZ (37): die -march-Flag der freigegebenen System-Zelle IST heute das Gate der Organ-SIMD-
// Codegen (Organ-SIMD <= System-SIMD-Zulassung; ISA-Gate E1, profile_run_entry.hpp). Die SIMD-gekoppelten Organ-
// Varianten sind die search_algo-Wrapper mit SimdCapableStrategy-Concept (array256 = dense+SIMD, vector_u8u8 =
// Patricia+SIMD; k_ary/original_art/original_hot). Sie tragen per Concept-Vertrag IMMER einen Skalar-Pfad
// (`simd_lookup` == `lookup`, nur schneller) -> unter simd=no_extension (kein -march) DEGRADIEREN sie via #ifdef
// auf Skalar, sie schlagen NICHT fehl. Damit ist die 36-Forderung "Degradation/Skip statt Bau-Fehler" fuer den
// HEUTIGEN Organ-Satz bereits durch die -march-CompileFn an der richtigen Naht erfuellt; der Emitter muss (und
// DARF) hier NICHT zusaetzlich filtern -- ein Hard-Skip verloere die valide, skalar-degradierte Mess-Zelle
// (Gate-Test (d) belegt: der Emitter rendert array256/vector_u8u8 system-blind nicht-leer).
//
// GENERISCH (37) + ZIELBILD: SIMD ist die PILOT-Instanz. Kuenftige System-Consumer-Achsen (NUMA/NPU/GPU/FPGA
// unter der Hardware-Achse; die Ist-Vorbilder simd_extension-09b / isa-Codegen-Traeger / general_hardware sind
// bereits Dual-Naturen) koppeln an DERSELBEN perm_compile/provision_all-Naht -- mehrere Organ-Systemconsumer-
// Achsen permutieren die Freigabe bis zur maximalen Faehigkeitsstufe. Freigabe betrifft NUR statische System-
// HAUPT-Achsen (dynamische Unter-Achsen wie threads = Runtime-Durchreichung, KEINE Freigabe). Mechanik-Zielbild
// (Folge-Increment, HIER NICHT gebaut): Dual-Aufnahme (System-Aufnahme CEB + Organ-Aufnahme Tier) + Freischaltung
// von der CEB ueber ein STATE PATTERN durchs Pruef-Dock in die Organ-Repraesentation; ein HART SIMD-erfordernder
// Organ-Variant (OHNE Skalar-Pfad, den es heute NICHT gibt) braeuchte dann den constexpr-Zulaessigkeits-Filter
// (Audit-G7 -> Bau-Gate: `requires_simd()` <= System-Freigabe) GENAU an dieser provision_all/CompileFn-Naht. Die
// Naht ist so geschnitten, dass der State-Pattern-Filter dort andockt, OHNE diesen Emitter zu aendern.
//
// Umbrella-schwer (source_catalog.hpp zieht die 17 Topic-ConfigSets) -> gehoert NEBEN source_catalog.hpp in die
// Materialisierungs-Domaene, NICHT in den engine-agnostischen Treiber-Header. C++23, header-only.

#include "source_catalog.hpp" // die 17 TopicConfigSet::StaticAxisVariants_* (Single-Source der Flyweight-Tabellen)

#include <builder/codegen/adhoc_emitter.hpp>                   // render_adhoc_module_source / strip_all_elaborated
#include <builder/codegen/type_name.hpp>                       // type_name<W>
#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (kanonische 17-Slot-Ordnung)
#include <builder/experiment_tree/ceb_generator.hpp> // ceb_parse_path (bestehende axis=value/-Parse-Konvention)
#include <builder/experiment_tree/axis_variant_version_table.hpp> // ex::compose_organ_stamp_line / build_axis_variant_version_table (W12-A2)
#include <builder/build_orchestrator/build_orchestrator.hpp> // ex::SourceGenFn

#include <cache_engine/abi/anatomy_version_stamp.hpp> // abi::system_stamp_line (W12-A2 System-Stempel-Zeile)

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <cstdlib> // S6-P1b Env-Bruecke: std::getenv (COMDARE_MEASUREMENT_COMBO)
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace cg = ::comdare::cache_engine::builder::codegen;
// ex / ce / mp sind bereits in source_catalog.hpp fuer diesen Namespace deklariert (identische Alias-Ziele).

// Ein Flyweight-Eintrag EINER Achse: der binary_id-Segmentwert (W::name()) -> der FQ-cpp_type, gerendert mit
// EXAKT dem adhoc_macro_args-Transform (strip_all_elaborated(type_name<W>()), OHNE fuehrendes "::").
struct LazyVariantEntry {
    std::string name;     // == W::name() == der Wert im binary_id-Segment "achse=<name>"
    std::string cpp_type; // == strip_all_elaborated(type_name<W>()) -> byte-identisch zu adhoc_macro_args
};

using LazySlotTables = std::array<std::vector<LazyVariantEntry>, 17>;

/// lazy_variants_for_list<List>() -- die kleine Flyweight-Tabelle EINER Achse aus ihrer Enabled-Typ-Liste
/// (Muster variants_for_list, catalog_codegen_tool/main.cpp:103). WICHTIG: der cpp_type wird mit demselben
/// Transform wie adhoc_macro_args gebildet (strip_all_elaborated(type_name<W>())) -- NICHT mit dem
/// "::"-praefixierenden cpp_type_name des Codegen-Tools (das schreibt nur die generierte Header-Datei, NICHT
/// die Runtime-Modul-Quelle). Nur so treffen die lazy-gerenderten Bytes den Katalog-Pfad exakt.
template <class List>
[[nodiscard]] inline std::vector<LazyVariantEntry> lazy_variants_for_list() {
    std::vector<LazyVariantEntry> out;
    out.reserve(mp::mp_size<List>::value);
    mp::mp_for_each<mp::mp_transform<mp::mp_identity, List>>([&](auto id) {
        using W = typename decltype(id)::type;
        out.push_back(LazyVariantEntry{std::string{W::name()}, cg::strip_all_elaborated(cg::type_name<W>())});
    });
    return out;
}

/// lazy_slot_type_tables() -- die 17 Flyweight-Tabellen in kanonischer kCompositionAxisNames-Reihenfolge.
/// Single-Source = die SELBEN StaticAxisVariants_*-Listen wie make_slots() (catalog_codegen_tool) und
/// CatalogAxes/catalog_static_levels (source_catalog.hpp). Host-seitig EINMAL, O(sum Slot-Groessen), KEIN
/// mp_product. Die VOLLEN Enabled-Listen (nicht mp_take_c-Praefixe) -> der Emitter deckt JEDEN golden-N-Wert ab.
[[nodiscard]] inline LazySlotTables lazy_slot_type_tables() {
    return LazySlotTables{
        lazy_variants_for_list<ce::traversal::TopicConfigSet::StaticAxisVariants_03a>(), // 00 search_algo
        lazy_variants_for_list<ce::traversal::TopicConfigSet::StaticAxisVariants_03b>(), // 01 cache_traversal
        lazy_variants_for_list<ce::traversal::TopicConfigSet::StaticAxisVariants_03m>(), // 02 mapping
        lazy_variants_for_list<ce::nodes::TopicConfigSet::StaticAxisVariants_02>(),      // 03 path_compression
        lazy_variants_for_list<ce::nodes::TopicConfigSet::StaticAxisVariants_04>(),      // 04 node_type
        lazy_variants_for_list<ce::memory_layout::TopicConfigSet::StaticAxisVariants>(), // 05 memory_layout
        lazy_variants_for_list<ce::allocator::TopicConfigSet::StaticAxisVariants>(),     // 06 allocator
        lazy_variants_for_list<ce::prefetch::TopicConfigSet::StaticAxisVariants>(),      // 07 prefetch
        lazy_variants_for_list<ce::concurrency::TopicConfigSet::StaticAxisVariants>(),   // 08 concurrency
        lazy_variants_for_list<ce::serialization::TopicConfigSet::StaticAxisVariants>(), // 09 serialization
        lazy_variants_for_list<ce::value_handle::TopicConfigSet::StaticAxisVariants>(),  // 10 value_handle
        lazy_variants_for_list<ce::search_engine::TopicConfigSet::StaticAxisVariants>(), // 11 index_organization
        lazy_variants_for_list<ce::io::TopicConfigSet::StaticAxisVariants>(),            // 12 io_dispatch
        lazy_variants_for_list<ce::migration::TopicConfigSet::StaticAxisVariants>(),     // 13 migration_policy
        lazy_variants_for_list<ce::filter::TopicConfigSet::StaticAxisVariants>(),        // 14 filter
        lazy_variants_for_list<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1>(),    // 15 queuing_q1
        lazy_variants_for_list<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2>()};   // 16 queuing_q2
}

/// lazy_adhoc_macro_args_for(tables, binary_id) -- der COMDARE_DEFINE_ANATOMY_MODULE_ADHOC-Argument-Block (die
/// 17 FQ-cpp_types, ",\n    "-gefugt) EINER golden-N-`binary_id`, byte-identisch zu adhoc_macro_args<Comp>().
/// LEER, wenn der binary_id nicht alle 17 Kompositions-Achsen traegt ODER ein Wert nicht in der Enabled-Tabelle
/// liegt (ehrlich = nicht materialisierbar). Reihenfolge-unabhaengig vom binary_id (Namens-Lookup je Slot).
[[nodiscard]] inline std::string lazy_adhoc_macro_args_for(LazySlotTables const& tables, std::string const& binary_id) {
    std::vector<std::pair<std::string, std::string>> const axes = ex::ceb_parse_path(binary_id);
    std::string                                            macro_args;
    macro_args.reserve(512);
    for (std::size_t slot = 0; slot < ex::kCompositionAxisNames.size(); ++slot) {
        std::string_view const axis = ex::kCompositionAxisNames[slot];
        // (a) Segmentwert dieser Achse aus dem binary_id ziehen (Namens-Match).
        std::string const* value = nullptr;
        for (auto const& kv : axes)
            if (kv.first == axis) {
                value = &kv.second;
                break;
            }
        if (value == nullptr) return {}; // Achse fehlt im binary_id -> kein golden-N-id (ehrlich leer)
        // (b) FQ-cpp_type des Wertes aus der Flyweight-Tabelle joinen (Namens-Lookup).
        std::string const* cpp = nullptr;
        for (auto const& e : tables[slot])
            if (e.name == *value) {
                cpp = &e.cpp_type;
                break;
            }
        if (cpp == nullptr) return {};                    // Wert nicht enabled -> nicht materialisierbar (ehrlich leer)
        if (!macro_args.empty()) macro_args += ",\n    "; // Fuge byte-identisch zu adhoc_macro_args
        macro_args += *cpp;
    }
    return macro_args;
}

/// lazy_adhoc_source_for(tables, binary_id) -- die REALE Modul-Quelle (Umbrella-Include +
/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC) EINER golden-N-`binary_id`, oder LEER (nicht materialisierbar).
/// Der Permutations-Index im Kommentar-Kopf ist reine Doku (der Round-Trip-Key ist der Pfad) -> fixer
/// Sentinel 0 (der lazy Emitter hat keinen laufenden Katalog-Index).
[[nodiscard]] inline std::string lazy_adhoc_source_for(LazySlotTables const& tables, std::string const& binary_id,
                                                       std::vector<ex::AxisVariantVersion> const& version_table,
                                                       std::string const& measurement_stamp = {}) {
    std::string const macro_args = lazy_adhoc_macro_args_for(tables, binary_id);
    if (macro_args.empty()) return {};
    // W12-A2 (Section 43): DENSELBEN geteilten Helfer + DENSELBEN binary_id wie der Katalog-Pfad
    // (pilot_source_map.hpp build_pilot_source_map) benutzen -> byte-identische Stempel -> die 320-Round-Trip-
    // Wache bleibt STRIKT. compose_organ_stamp_line = die reale Organ-Zeile aus der Version-TABELLE (NICHT das
    // mock-only organ_stamp_line<Comp>); system_stamp_line = die statischen System-Achsen-Algo-Versionen.
    std::string const organ  = ex::compose_organ_stamp_line(ex::ceb_parse_path(binary_id), version_table);
    std::string const system = ::comdare::cache_engine::abi::system_stamp_line();
    // S6-P1b (Section 43/47): APPEND-ONLY measurement_stamp = die Mess-Tooling-HAUPT-Stempel-Zeile
    // (abi::measurement_stamp_line(tooling)) der vom Planer gewaehlten Combo. Default "" (LEERE/[all]-Combo) =>
    // render_adhoc_module_source emittiert EXAKT die 2-arg-Makro-Zeile -> byte-identisch zur heutigen Quelle (die
    // 320-Round-Trip-/Byte-Wache bleibt STRIKT). Nicht-leer (explizite wallclock/macro/micro-Combo) => 3-arg _M-Form.
    return cg::render_adhoc_module_source(0, macro_args, organ, system, /*merge_stamp=*/{}, measurement_stamp);
}

/// make_lazy_adhoc_source_gen() -- die Naht: eine SourceGenFn (binary_id -> reale Modul-Quelle), die run_profile
/// als ZUSAETZLICHE Quelle hinter die bestehenden (Basis-320 / Sweeps / SOTA) in make_union_source_gen haengt.
/// Die 17 Flyweight-Tabellen werden EINMAL gebaut (shared_ptr, per-Aufruf O(17)). Fuer einen binary_id, dessen
/// 17 Slot-Werte alle enabled sind, liefert sie eine REALE Quelle; sonst leer (ehrlich). Geht NIE durch
/// build_pilot_source_map -> GN-2-Guard unberuehrt.
[[nodiscard]] inline ex::SourceGenFn make_lazy_adhoc_source_gen(std::string measurement_stamp = {}) {
    auto tables = std::make_shared<LazySlotTables const>(lazy_slot_type_tables());
    // W12-A2 (Section 43): die {axis,variant->version}-Tabelle EINMAL bauen (wie die Flyweight-Tabellen,
    // shared_ptr) -> per-Aufruf O(1)-Zugriff; identisch zum Katalog-Pfad (build_axis_variant_version_table).
    auto version_table =
        std::make_shared<std::vector<ex::AxisVariantVersion> const>(ex::build_axis_variant_version_table());
    // S6-P1b (Section 43/47): APPEND-ONLY measurement_stamp = die Mess-Tooling-HAUPT-Stempel-Zeile der gewaehlten
    // Combo (abi::measurement_stamp_line(tooling)), per Wert in die SourceGenFn eingefangen. Default "" => byte-
    // identische Quellen zur heutigen 1-CEB-Strecke (die Wachen bleiben gruen). Die LIVE-Naht reicht die Combo ueber
    // make_lazy_adhoc_source_gen_from_env() (die tier:build/measure-Kommandos exportieren COMDARE_MEASUREMENT_COMBO).
    return [tables, version_table,
            measurement_stamp = std::move(measurement_stamp)](std::string const& binary_id) -> std::string {
        return lazy_adhoc_source_for(*tables, binary_id, *version_table, measurement_stamp);
    };
}

/// make_lazy_adhoc_source_gen_from_env() -- die LIVE-Naht der S6-P1b Env-Bruecke (d)-(f): die vom Planer gewaehlte
/// Mess-Combo reist ueber die Umgebungsvariable COMDARE_MEASUREMENT_COMBO (die Director-Tier-Kommandos exportieren die
/// [a,b,c]-Legende ab N>1; der CLI-Parser --measurement-combo im messung_driver waehlt die Combo im gefilterten Walk).
/// run_profile ruft DIESE Naht: die Legende wird zur Mess-Tooling-Stempel-Zeile (abi::measurement_stamp_line_from_
/// combo_legend) und in den lazy Source-Gen gespeist -> die je-Combo-Bauten stempeln ihre DLLs REAL. UNGESETZT/[all]
/// => "" => byte-identische Quellen (Default-Pfad unberuehrt; golden-CRC/320 neutral).
[[nodiscard]] inline ex::SourceGenFn make_lazy_adhoc_source_gen_from_env() {
    std::string measurement_stamp;
    if (char const* e = std::getenv("COMDARE_MEASUREMENT_COMBO"); e != nullptr && *e != '\0')
        measurement_stamp = ::comdare::cache_engine::abi::measurement_stamp_line_from_combo_legend(e);
    return make_lazy_adhoc_source_gen(std::move(measurement_stamp));
}

} // namespace comdare::cache_engine::thesis_lazy
