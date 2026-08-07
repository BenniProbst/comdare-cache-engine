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
#include "toolchain_stamp_naht.hpp" // NB/CX-4: die LIVE-Werte der Glieder [5]/[6] -- DIESELBEN wie im Bau-Kanal
#include "mess_achsen_naht.hpp"     // M-1/D-1: die EINE Aufloesung der Mess-Combo -- DIESELBE wie im Bau-Kanal

#include <builder/codegen/adhoc_emitter.hpp>                   // render_adhoc_module_source / strip_all_elaborated
#include <builder/codegen/type_name.hpp>                       // type_name<W>
#include <builder/experiment_tree/axis_path_serialization.hpp> // kCompositionAxisNames (kanonische 17-Slot-Ordnung)
#include <builder/experiment_tree/ceb_generator.hpp> // ceb_parse_path (bestehende axis=value/-Parse-Konvention)
#include <builder/experiment_tree/axis_variant_version_table.hpp> // ex::compose_organ_stamp_line / build_axis_variant_version_table (W12-A2)
#include <builder/build_orchestrator/build_orchestrator.hpp> // ex::SourceGenFn

#include <cache_engine/abi/anatomy_fingerprint.hpp>   // A13-M3: anatomy_fingerprint_glieder/-preimage (EINE Ordnung)
#include <cache_engine/abi/anatomy_version_stamp.hpp> // abi::system_stamp_line (W12-A2 System-Stempel-Zeile)
#include <cache_engine/abi/system_cell_values.hpp>    // W10-C3: Zellwerte + Vervollstaendiger (Laufzeit-Zwilling)
#include <sha512/ctsha512.hpp>                        // I2: Runtime-SHA-512 fuer den drift-freien Fingerprint-Provider

#include <boost/mp11.hpp>

#include <array>
#include <cstddef>
#include <cstdint> // I2: std::uint8_t fuer das SHA-512-Preimage
#include <cstdlib> // S6-P1b Env-Bruecke: std::getenv (COMDARE_MEASUREMENT_COMBO)
#include <memory>
#include <optional>  // NB2-5: "nicht uebergeben" vs. "leer uebergeben" -- die Unterscheidung wohnt im Typ
#include <span>      // I2: std::span fuer die SHA-512-Primitive
#include <stdexcept> // W2: Widerspruchs-Wache der CT-Combo (fehlerklasse=konfiguration_widerspruch)
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

using LazySlotTables = std::array<std::vector<LazyVariantEntry>, 18>; // STRUKT-R ORG-18

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
        lazy_variants_for_list<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2>(),    // 16 queuing_q2
        lazy_variants_for_list<ce::io::TopicConfigSet::StaticAxisVariants_PT>()};        // 17 persistence_target
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
    // S6-P1b (Section 43/47): APPEND-ONLY measurement_stamp = die Mess-Tooling-Stempel-Zeile der gewaehlten Combo
    // (abi::measurement_stamp_line, K7b-2 auch als MENGE). No-Arg-Default "" => render_adhoc_module_source emittiert
    // EXAKT die 2-arg-Makro-Zeile -> byte-identisch zur heutigen Quelle (die 320-Round-Trip-/Byte-Wache bleibt STRIKT).
    // Nicht-leer (explizite Combo bzw. [all]/UNGESETZT-Vollmenge ueber die from_env-Naht) => 3-arg _M-Form.
    return cg::render_adhoc_module_source(0, macro_args, organ, system, measurement_stamp);
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

/// measurement_stamp_from_env() -- die EINE Stelle, an der die Env-Bruecke COMDARE_MEASUREMENT_COMBO in die
/// Mess-Tooling-Stempel-Zeile uebersetzt wird. Sie stand bis A13-M3/C1 ZWEIMAL wortgleich da
/// (make_lazy_adhoc_source_gen_from_env + make_lazy_adhoc_fingerprint_fn_from_env) und war damit genau die
/// Sorte Doppel-Ableitung, die der O-8-Schritt-12-Befund ("uebersehener dritter Ableitungsweg") meint: waere
/// eine der beiden Kopien gewandert, haette die DLL eine andere Mess-Zeile getragen als ihr Fingerprint-
/// Sidecar. Seit C1 speist sie zusaetzlich den SOTA-Emitter (sota_catalog.hpp) -- dieselbe Combo in ALLEN
/// Quellen EINES Laufs, per Konstruktion und nicht per Absprache.
/// SEMANTIK unveraendert (K7b-2): gesetzt -> die gewaehlte Combo-Legende als MENGE; UNGESETZT/leer == [all]
/// == die VOLLE 3-Tool-Vollmenge (Vollmengen-Provenienz).
///
/// W2 (Owner-GO mittag-6 R1 "-D-Define wie empfohlen", Fork-A-Minimalhaerte): ist die Mess-Combo COMPILE-hart
/// einkompiliert (COMDARE_MEASUREMENT_COMBO_CT, gesetzt vom Emissions-Weg ueber die CMake-Cache-Variable
/// COMDARE_MEASUREMENT_COMBO), dann ist SIE die Wahrheit -- das ist der Stufe-2-CT-EINBAU der Mess-Achsen
/// (Stufen-Doktrin mittag-9/-10: die Stufe-1-RT-Freigabe lebt im PLANER, die Stufe-2-Haertung in der CEB).
/// Weil DIESE Funktion seit A13-M3/C1 die EINE Uebersetzung fuer Source-Gen UND Fingerprint-Fn UND
/// SOTA-Emitter ist, erreicht die CT-Wahl alle drei Konsumenten KONSTRUKTIONSBEDINGT gleich -- es gibt keinen
/// vierten Weg, der die Env noch einmal selbst liest.
///
/// BYTE-NEUTRALITAET (DAUER-AUFLAGE): der Renderer bleibt DERSELBE
/// (abi::measurement_stamp_line_from_combo_legend) und bekommt DENSELBEN Legenden-String -- die MESS-ZEILE der
/// Tier-Fingerprints ist damit per Konstruktion byte-identisch zum Env-Weg, nicht per Zufall. OHNE Define
/// (der gesamte golden-/CI-Bestand, [all]-Emission) laeuft der #else-Zweig WORTGLEICH weiter.
///
/// WIDERSPRUCHS-WACHE (Fehlerklassen-Doktrin): eine ABWEICHENDE Laufzeit-Env gegen ein einkompiliertes
/// Kompilat ist ein Konfigurationswiderspruch -- fail-loud, nie still. Eine GLEICHE Env ist stumm zulaessig
/// (die emittierten Jobs tragen weiterhin BEIDE Formen mit demselben Wert; der Env-Kanal speist +mtool und
/// die Bestandslog-Zelle -- die W-11-Flaeche, hier bewusst UNANGETASTET).
///
/// F-B2 (CODEX-NACHREVIEW W1/W2, Ledger-Nachtrag 05.08.2026 nachmittag-7) -- DIE LEERE ENV IST KEIN SONDERFALL
/// MEHR: bis hierher war eine leere/fehlende Env vom Abgleich AUSGENOMMEN und der CT-Wert galt stumm. Damit
/// fuehrte der STEMPEL die CT-Welt, waehrend der Objekt-Cache-Key (+mtool, aus DERSELBEN Env) und die
/// Bestandslog-Zelle z.combo die leere == [all]-Identitaet fuehrten -- ein SCHLUESSEL-WELT-SPLIT gegen die
/// Anker-Doktrin "EINE Schluessel-Welt" (Ledger-Nachtrag nachmittag-2). Ab jetzt gilt: ist eine SPEZIFISCHE
/// Combo einkompiliert, MUSS die Env sie tragen -- fehlt sie, ist das derselbe Konfigurationswiderspruch wie
/// eine abweichende Env. Kein legaler Lauf wird davon getroffen: die emittierten Jobs exportieren Env UND
/// Define gemeinsam je Combo, und die [all]-Emission traegt seit F-B1 gar kein Define mehr (sie LOESCHT die
/// CMake-Cache-Variable explizit). Damit ist diese Wache zugleich das Sicherheitsnetz fuer jeden Weg, den F-B1
/// nicht mechanisch erreicht (z.B. ein bare-metal-Bediener mit sticky konfiguriertem Build-Verzeichnis).
///
/// AUSNAHME [all]: ist der einkompilierte Wert selbst die Vollmenge ("[all]" bzw. leer), ist er KEINE
/// spezifische Wahl -- dann ist die leere Env die SYNCHRONE Form (die Emission exportiert fuer [all] bewusst
/// keine Env) und es wird nicht geworfen. Diese Kombination kann aus der Emission nicht entstehen (F-B1: [all]
/// => -U => kein Define); der Zweig deckt nur einen von Hand gesetzten -DCOMDARE_MEASUREMENT_COMBO=[all] ab und
/// rendert dort ueber denselben Renderer die Vollmengen-Zeile (measurement_stamp_line_from_combo_legend bildet
/// "[all]" auf measurement_stamp_line_full_set ab) -- also byte-gleich zum #else-Zweig.
///
/// BYTE-BILANZ von F-B2: KEIN Stempel-Byte bewegt sich. Der Wurf ersetzt ausschliesslich ein Ergebnis, das der
/// Alt-Stand still aus einem widerspruechlichen Zustand erzeugte; jeder LEGALE Lauf rendert unveraendert.
///
/// M-1/D-1 (06.08.2026) -- DIE AUFLOESUNG WOHNT AB HIER IN DER MESS-NAHT, NICHT MEHR IN DIESER FUNKTION.
/// Der frueher hier stehende #ifdef-Block (CT-Lesung + die beiden F-B2-Wuerfe + der Env-Zweig) ist
/// WORTGLEICH nach profile_facade/mess_achsen_naht.hpp gewandert (resolve_live_measurement_combo_legend).
/// GRUND: der BAU-Kanal (perm_mess_defines -> perm_compile_flags) leitet ab M-1 seinen Define-Vektor aus
/// derselben Mess-Achse ab. Stuende die Aufloesung weiter NUR hier, muesste die Bau-Seite eine ZWEITE
/// aufmachen -- und genau daraus entstand D-1: das Preimage-Glied [3] deklarierte eine Mess-Ausstattung,
/// die der Bau nicht kannte. Eine Aufloesung, zwei Verbraucher, keine Absprache.
/// BYTE-BILANZ: der Renderer ist unveraendert und bekommt denselben Legenden-String
/// (der CT-lose UNGESETZT-Zweig liefert "[all]", und measurement_stamp_line_from_combo_legend("[all]")
/// IST measurement_stamp_line_full_set()) -- kein Stempel-Byte bewegt sich.
[[nodiscard]] inline std::string measurement_stamp_from_env() {
    return ::comdare::cache_engine::abi::measurement_stamp_line_from_combo_legend(
        ::comdare::cache_engine::profile_facade::resolve_live_measurement_combo_legend());
}

/// make_lazy_adhoc_source_gen_from_env() -- die LIVE-Naht der S6-P1b Env-Bruecke (d)-(f): die vom Planer gewaehlte
/// Mess-Combo reist ueber die Umgebungsvariable COMDARE_MEASUREMENT_COMBO (die Director-Tier-Kommandos exportieren die
/// [a,b,c]-Legende ab N>1; der CLI-Parser --measurement-combo im messung_driver waehlt die Combo im gefilterten Walk).
/// run_profile ruft DIESE Naht: die Legende wird zur Mess-Tooling-Stempel-Zeile (abi::measurement_stamp_line_from_
/// combo_legend) und in den lazy Source-Gen gespeist -> die je-Combo-Bauten stempeln ihre DLLs REAL.
/// K7b-2 (Section 64-D1-B, 2026-07-22): COMDARE_MEASUREMENT_COMBO gesetzt -> die gewaehlte Combo-Legende stempelt als
/// MENGE (N x measurement_tooling=<t>@1.0.0c). UNGESETZT == [all] == die VOLLE 3-Tool-Vollmenge (Vollmengen-Provenienz;
/// die [all]-Lane misst mit dem vollen Tooling-Angebot -> ihre DLL-Provenienz traegt ALLE Tools). NUR dieser LIVE-Pfad
/// wechselt bewusst auf die 3-arg-_M-Form; der No-Arg-Default make_lazy_adhoc_source_gen() bleibt "" (2-arg) -> die
/// 320er-Byte-Identitaets-Wachen + der binary_id/CRC-Anker (kNewGolden131072Crc64, ueber die view-ids) unberuehrt.
[[nodiscard]] inline ex::SourceGenFn make_lazy_adhoc_source_gen_from_env() {
    std::string measurement_stamp = measurement_stamp_from_env();
    return make_lazy_adhoc_source_gen(std::move(measurement_stamp));
}

/// I2 (Lager-Gate): der drift-freie Fingerprint fuer binary_id -- 128-hex K7b-Fingerprint, IDENTISCH zu dem, den die
/// DLL via comdare_anatomy_version_lines()->sha512_line traegt. Komponiert EXAKT dieselben Stempel-Zeilen wie
/// lazy_adhoc_source_for (organ via compose_organ_stamp_line, system via system_stamp_line, measurement = dieselbe
/// Combo) und hasht sie mit derselben Primitive (sha512 + to_hex) wie anatomy_fingerprint_hex.
///
/// A13-M3: die Preimage-ORDNUNG steht NICHT mehr hier -- sie kommt aus abi::anatomy_fingerprint_glieder(), der
/// EINEN Quelle, aus der auch der consteval-Zwilling zieht. Vorher stand die Glied-Folge zweimal wortgleich da
/// und konnte auseinanderlaufen (Risiko R6: Lager-Key-Drift gegen das einkompilierte sha512_line). Der Trenner
/// zwischen den Gliedern ('\n', OF-M3-1 Option A) steckt in abi::anatomy_fingerprint_preimage.
/// Nicht materialisierbar -> "".
///
/// W10-C3 (Bauplan-Dossier 20260803, Sektion 2) -- DER LAUFZEIT-ZWILLING DER ZELLWERT-NAHT. Die System-Zeile
/// wird hier mit DENSELBEN Zellwerten vervollstaendigt, die W10-C4 als
/// -DCOMDARE_SYSTEM_CELL_VALUES an perm_compile haengt. Damit bleibt die drift-freie Zusage dieser Funktion
/// ("IDENTISCH zu dem, den die DLL via comdare_anatomy_version_lines()->sha512_line traegt") auch nach W10
/// wahr: consteval-Makro und Laufzeit-Zwilling rechnen ueber DIESELBE vervollstaendigte Zeile, weil beide
/// denselben Vervollstaendiger aus abi/system_cell_values.hpp rufen -- keine zweite Ableitung.
///
/// K-1-MUSTER: die Werte reisen als BENANNTER Typ (abi::SystemCellValues, Praezedenz OverlayHash) und nicht
/// als weiterer nackter string_view -- die Signatur traegt bereits drei std::string-Parameter, ein vierter
/// String koennte an einem Alt-Aufruf still in den falschen Slot rutschen. Der Default ist LEER == die
/// IDENTITAET: jeder Bestands-Aufruf rechnet byte-identisch weiter (Bestands-Beweis: der Zwillings-Test in
/// test_lazy_adhoc_source_gen bleibt ohne Edit gruen).
/// O-2/C-2 (Format 3) -- DIE TOOLCHAIN-/BVSET-NAHT DES LAUFZEIT-ZWILLINGS. Die beiden neuen Glieder reisen
/// als BENANNTE Traeger (K-1), mit LEEREM Default == der Identitaet: jeder Bestands-Aufruf rechnet
/// byte-identisch weiter. Die Werte selbst kennt diese Funktion nicht und darf sie nicht erraten -- sie
/// entstehen an der Perm-Schleife (Toolchain-Wahl dieser Permutation) bzw. am Treiber (bvset). Wer sie
/// hier injiziert, MUSS im selben Zug das passende Compile-Define an perm_compile haengen
/// (COMDARE_TOOLCHAIN_STAMP_GLIED / COMDARE_BUILD_VARIANT_SET_SIGNATURE), sonst rechnet der
/// consteval-Zwilling in der Tier-Binary ueber ANDERE Glieder als dieser Laufzeit-Weg -- und die
/// drift-freie Zusage dieser Funktion waere gebrochen. Genau deshalb bleibt die Verdrahtung EIN Schritt
/// (Buendel-Scheibe C-3) und wird hier nur vorbereitet.
///
/// R-3 (Format 4) -- DAS MESS-GATES-GLIED [8]. Es ist der EINE Traeger, dessen Wert dieser Weg NICHT
/// aus einem Compile-Define ziehen kann: er ist die Praeprozessor-WAHRHEIT der Tier-Uebersetzungseinheit
/// (abi::kMessGatesTuGlied), und die entsteht erst beim Uebersetzen der emittierten Quelle. Der Host
/// kann ihn nur VORHERSAGEN -- pf::mess_gates_glied_for_legend(legende), die die Vorhersage aus
/// demselben Define-Vektor liest, den der Bau-Kanal wirklich anhaengt. Der Default ist die LEERE
/// Identitaet (Bestands-Aufrufer rechnen byte-identisch weiter); die LIVE-Naht unten reicht den
/// Vorhersage-Wert (NB/CX-4-Muster: "kein Argument" heisst LIVE, nicht leer).
[[nodiscard]] inline std::string lazy_adhoc_fingerprint_for(
    LazySlotTables const& tables, std::string const& binary_id,
    std::vector<ex::AxisVariantVersion> const& version_table, std::string const& measurement_stamp = {},
    ::comdare::cache_engine::abi::SystemCellValues system_cell_values = {},
    ::comdare::cache_engine::abi::ToolchainGlied   toolchain_glied =
        ::comdare::cache_engine::abi::ToolchainGlied{::comdare::cache_engine::abi::kToolchainStampGlied},
    ::comdare::cache_engine::abi::BvsetGlied bvset_glied =
        ::comdare::cache_engine::abi::BvsetGlied{::comdare::cache_engine::abi::kBuildVariantSetSignatureGlied},
    ::comdare::cache_engine::abi::MessGatesGlied mess_gates_glied = ::comdare::cache_engine::abi::MessGatesGlied{""}) {
    std::string const macro_args = lazy_adhoc_macro_args_for(tables, binary_id);
    if (macro_args.empty()) return {}; // nicht materialisierbar -> keine DLL -> kein Fingerprint
    std::string const organ  = ex::compose_organ_stamp_line(ex::ceb_parse_path(binary_id), version_table);
    std::string const system = ::comdare::cache_engine::abi::complete_system_stamp_line(
        ::comdare::cache_engine::abi::system_stamp_line(), system_cell_values);
    // R-3: das Overlay-Glied wird ab hier EXPLIZIT gereicht -- es ist kein Schwanz-Glied mehr und kann
    // nicht mehr per Default hinter dem Mess-Gates-Glied stehenbleiben. Der Wert ist derselbe wie zuvor
    // (der Default), also bewegt allein das neunte Glied den Digest.
    auto const glieder = ::comdare::cache_engine::abi::anatomy_fingerprint_glieder(
        organ, system, measurement_stamp, toolchain_glied, bvset_glied,
        ::comdare::cache_engine::abi::OverlayHash{::comdare::cache_engine::abi::kOverlaySourceHash}, mess_gates_glied);
    std::string const preimage = ::comdare::cache_engine::abi::anatomy_fingerprint_preimage(
        std::span<std::string_view const>{glieder.data(), glieder.size()});
    auto const digest = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(preimage.data()), preimage.size()});
    auto const hex = ::comdare::cache_engine::sha512::to_hex(digest); // array<char, 128>
    return std::string(hex.data(), hex.size());
}

/// make_lazy_adhoc_fingerprint_fn_from_env() -- die LIVE-Naht: ein FingerprintFn (binary_id -> 128-hex), der dieselbe
/// Mess-Combo (COMDARE_MEASUREMENT_COMBO) einfaengt wie make_lazy_adhoc_source_gen_from_env -> die Fingerprint-Zeilen
/// decken sich byte-genau mit den emittierten Stempel-Zeilen der DLL (drift-frei). Fuer den Lager-Index-Anker
/// (bestand_fingerprint_fn): der Orchestrator schreibt je Binary das .fingerprint-Sidecar.
///
/// W10-C4: die System-ZELLWERTE dieser Zelle kommen als Parameter herein und werden per WERT gehalten. Das ist
/// kein Stil-Detail: der Traeger-Typ abi::SystemCellValues haelt nur eine SICHT, und dieser Provider ueberlebt
/// den Perm-Schleifen-Durchlauf, in dem der Werte-String entsteht -- eine gefangene string_view zeigte spaeter
/// ins Leere. Der Default (leerer String) ist die IDENTITAET: jeder Bestands-Aufrufer rechnet byte-identisch
/// weiter, weil ein leeres Werte-Set die System-Zeile unveraendert laesst.
///
/// O-2/C-2: die beiden neuen Glieder kommen ebenso per WERT herein und aus demselben Grund -- der Provider
/// ueberlebt den Perm-Schleifen-Durchlauf, in dem der Werte-String entsteht.
///
/// NB/CX-4 -- DIE ZWEITE HAELFTE DER LIVE-NAHT. Bis zur Nachbesserung hiess "kein Argument" hier "leerer
/// Default aus dem Compile-Define", also: der produktive Fingerprint trug die zwei neuen Glieder GAR NICHT.
/// Ab jetzt heisst "kein Argument": DIE LIVE-WERTE -- exakt die, die der Bau-Kanal
/// (profile_run_facade perm_compile_flags -> -DCOMDARE_TOOLCHAIN_STAMP_GLIED /
/// -DCOMDARE_BUILD_VARIANT_SET_SIGNATURE) in die Tier-Uebersetzung haengt. Beide Seiten rufen DIESELBEN
/// argumentlosen, reinen Funktionen der Toolchain-Naht; damit ist die drift-freie Zusage dieser Funktion
/// nicht mehr eine Bitte an den Aufrufer, sondern Mechanik.
///
/// NB2-5 (Codex-Zweitreview [MITTEL], am Code bestaetigt) -- KOMMENTAR UND VERHALTEN DECKEN SICH WIEDER.
/// Der Satz "ein EXPLIZIT uebergebener Wert gewinnt" stand hier, WAEHREND die Implementierung auf
/// `if (toolchain_glied.empty())` pruefte -- ein bewusst LEER uebergebener Wert wurde also gerade nicht
/// respektiert, sondern durch den Live-Wert ersetzt. Damit war fuer eine absichtlich OHNE die beiden
/// Defines gebaute Binary (Frozen-Vektor-Nachbau, Fallback-/Bestands-Binary aus der Zeit vor der Live-Naht)
/// KEIN passender Laufzeit-Fingerprint bildbar: der Zwilling rechnete zwangsweise mit Gliedern, die in
/// jener Binary gar nicht einkompiliert sind.
///
/// DIE UNTERSCHEIDUNG, DIE GEFEHLT HAT, IST JETZT IM TYP: std::optional trennt "NICHT UEBERGEBEN" von
/// "LEER UEBERGEBEN". std::nullopt (der Default, also jeder Bestands-Aufruf) == die LIVE-Werte, unveraendert
/// zur CX-4-Absicht. Ein uebergebener Wert gewinnt IMMER -- auch der leere String, der dann genau das
/// bedeutet, was er sagt: "diese Binary traegt das Glied nicht". Kein Zweig rechnet mehr etwas anderes,
/// als der Aufrufer verlangt hat.
[[nodiscard]] inline ex::FingerprintFn make_lazy_adhoc_fingerprint_fn_from_env(
    std::string system_cell_values = {}, std::optional<std::string> toolchain_glied = std::nullopt,
    std::optional<std::string> bvset_glied = std::nullopt, std::optional<std::string> mess_gates_glied = std::nullopt) {
    namespace pfn = ::comdare::cache_engine::profile_facade;
    std::string tc_wert =
        toolchain_glied.has_value() ? std::move(*toolchain_glied) : pfn::compose_live_toolchain_stamp_glied();
    std::string bv_wert =
        bvset_glied.has_value() ? std::move(*bvset_glied) : pfn::live_build_variant_set_signature_glied();
    // R-3: "kein Argument" == der LIVE-Wert (NB/CX-4). live_mess_gates_glied() zieht durch DIESELBE
    // EINE Aufloesung wie der Bau-Kanal (resolve_live_measurement_combo_legend -> mess_achsen_defines);
    // eine zweite Ableitung waere die Drift-Klasse aus D-1. Ein explizit uebergebener Wert gewinnt
    // IMMER -- auch der leere, der dann genau das heisst, was er sagt: "diese Binary traegt das Glied
    // nicht" (NB2-5, gilt hier unveraendert; Frozen-/Bestands-Binaries aus der Zeit vor Format 4).
    std::string mg_wert = mess_gates_glied.has_value() ? std::move(*mess_gates_glied) : pfn::live_mess_gates_glied();
    auto        tables  = std::make_shared<LazySlotTables const>(lazy_slot_type_tables());
    auto        version_table =
        std::make_shared<std::vector<ex::AxisVariantVersion> const>(ex::build_axis_variant_version_table());
    std::string measurement_stamp = measurement_stamp_from_env(); // dieselbe EINE Env-Bruecke wie der Source-Gen
    return [tables, version_table, measurement_stamp = std::move(measurement_stamp),
            cell_values = std::move(system_cell_values), toolchain = std::move(tc_wert), bvset = std::move(bv_wert),
            mess_gates = std::move(mg_wert)](std::string const& binary_id) {
        // NB/CX-4: die Werte stehen zu diesem Zeitpunkt FEST (oben aufgeloest -- explizit uebergeben oder
        // live komponiert). Der frueher hier stehende empty()-Zweig ist ersatzlos entfallen: er war genau
        // die Konflation "nicht injiziert" == "leer injiziert", die den Zwilling still auf den CEB-eigenen
        // Compile-Define zurueckfallen liess, waehrend die Tier-Binary einen anderen Wert eingebaut bekam.
        // NB2-5: die Aufloesung oben unterscheidet jetzt auch "leer UEBERGEBEN" von "nicht uebergeben".
        auto const tc = ::comdare::cache_engine::abi::ToolchainGlied{toolchain};
        auto const bv = ::comdare::cache_engine::abi::BvsetGlied{bvset};
        auto const mg = ::comdare::cache_engine::abi::MessGatesGlied{mess_gates};
        return lazy_adhoc_fingerprint_for(*tables, binary_id, *version_table, measurement_stamp,
                                          ::comdare::cache_engine::abi::SystemCellValues{cell_values}, tc, bv, mg);
    };
}

} // namespace comdare::cache_engine::thesis_lazy
