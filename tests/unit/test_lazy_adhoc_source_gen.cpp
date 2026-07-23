// INC-G6 (Ledger 33/34/35, 2026-07-19) -- Round-Trip-Gate fuer den lazy Per-Index-Adhoc-Emitter
// (lazy_adhoc_source_gen.hpp). Belegt VOR einem golden-N-Voll-Lauf (BAUPLAN Abschnitt 1.3), dass der lazy
// Emitter zu JEDER golden-N-binary_id die REALE Modul-Quelle liefert -- ohne mp_product zu materialisieren:
//
//   (a) golden-320: fuer die materialisierten 320er-ids liefert der lazy Gen dieselbe Modul-Quelle wie der
//       bestehende Katalog-Gen (generated_make_catalog_source_gen). Byte-Vergleich MODULO der ersten Zeile
//       (dem laufenden Permutations-Index im Kommentar-Kopf, der reine Doku ist -- pilot_source_map.hpp:31);
//       der Rest (Umbrella-Include + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC-Block) muss byte-identisch sein.
//   (b) Nicht-320 golden-N: fuer ids aus der FullSourceCatalog-StaticBinaryView, die NICHT im 320er-Satz
//       liegen, liefert der lazy Gen NICHT-LEER; die 17 binary_id-Werte treffen die gerenderten FQ-Typen
//       (Werte-Round-Trip), und distinkte ids ergeben distinkte Modul-Ruempfe.
//   (c) CRC64-Konsistenz: die StaticBinaryView-ids reproduzieren den committeten Anker kNewGolden131072Crc64
//       (NUR referenziert, NICHT geaendert), und der lazy Pfad materialisiert JEDE enumerierte id (Stichprobe
//       nicht-leer) -- der Emitter deckt genau den enumerierten Indexraum ab.
//
// Plain-main-Test (wie test_limits_entkopplung_vorstufe.cpp): check_eq/check_true, exit 0 = alle OK.

#include "generated_source_catalog.hpp" // generated_make_catalog_source_gen / generated_catalog_static_levels (320)
#include "lazy_adhoc_source_gen.hpp" // make_lazy_adhoc_source_gen / lazy_slot_type_tables / lazy_adhoc_macro_args_for
#include "source_catalog.hpp"        // catalog_static_levels<FullSourceCatalog> / kNewGolden131072Crc64

#include <builder/experiment_tree/experiment_tree.hpp> // ExperimentTree / StaticBinaryView / ExperimentNodeFactory
#include <cache_engine/abi/anatomy_version_stamp.hpp>  // S6-P1b: abi::measurement_stamp_line (Mess-Tooling-Stempel)
#include <cache_engine/fingerprint/crc64.hpp>          // crc64_ecma182_update
#include <sha512/ctsha512.hpp>                         // I2: Runtime-SHA-512 fuer den Fingerprint-Drift-Beweis

#include <algorithm> // K7b-2: std::count (Trenner-Zaehlung der Mengen-Stempel-Zeile)
#include <cstddef>
#include <cstdint>
#include <cstdlib> // S6-P1b Env-Bruecke: setenv/unsetenv (COMDARE_MEASUREMENT_COMBO)
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
#include <span> // I2: std::span fuer die SHA-512-Primitive im Drift-Beweis
#include <string>
#include <string_view>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace tlz = ::comdare::cache_engine::thesis_lazy;
namespace fp  = ::comdare::fingerprint;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

// Die binary_ids EINES Baums in kanonischer StaticBinaryView-Reihenfolge (lazy view[i], KEIN mp_product).
std::vector<std::string> binary_ids(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();
    std::vector<std::string>   ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}

// Alles NACH der ersten Zeile. Der lazy Gen und der Katalog-Gen unterscheiden sich NUR im Permutations-Index
// des Kommentar-Kopfs (reine Doku); der Rest -- Umbrella-Include + ADHOC-Block -- muss byte-identisch sein.
std::string body_after_first_line(std::string const& s) {
    std::size_t const nl = s.find('\n');
    return nl == std::string::npos ? std::string{} : s.substr(nl + 1);
}

// -- (a) golden-320: lazy Gen == Katalog Gen (modulo Index-Kommentar) --------------------------------------
void check_320_byte_identity(std::vector<std::string> const& g320_ids) {
    std::cout << "\n---- (a) golden-320: lazy Gen == Katalog Gen (modulo Index-Kommentar) ----\n";
    check_eq("(a) generated 320 binary_count", g320_ids.size(), std::size_t{320});
    ex::SourceGenFn const cat  = tlz::generated_make_catalog_source_gen();
    ex::SourceGenFn const lazy = tlz::make_lazy_adhoc_source_gen();

    std::size_t const sample =
        g320_ids.size() < 8 ? g320_ids.size() : 8; // >=5 gefordert; 8 ueber den Indexraum verteilt
    std::size_t nonempty = 0, matched = 0;
    for (std::size_t k = 0; k < sample; ++k) {
        std::size_t const i = (g320_ids.size() * k) / sample; // verteilt: 0, N/8, 2N/8, ...
        std::string const a = cat(g320_ids[i]);
        std::string const b = lazy(g320_ids[i]);
        if (!a.empty() && !b.empty()) ++nonempty;
        if (!a.empty() && body_after_first_line(a) == body_after_first_line(b)) ++matched;
    }
    check_eq("(a) beide Gens nicht-leer (Stichprobe >=5)", nonempty, sample);
    check_eq("(a) Katalog- und lazy-Quelle byte-identisch (modulo Index-Zeile)", matched, sample);
    check_true("(a) unbekannte id liefert leere lazy-Quelle", lazy("__unknown_binary_id__").empty());
}

// Die (erste) COMDARE_ANATOMY_VERSION_STAMP(...)-Zeile einer Modul-Quelle (oder leer, wenn keine).
std::string stamp_line(std::string const& src) {
    std::size_t const p = src.find("COMDARE_ANATOMY_VERSION_STAMP(");
    if (p == std::string::npos) return {};
    std::size_t const nl = src.find('\n', p);
    return src.substr(p, nl == std::string::npos ? std::string::npos : nl - p);
}

// -- (a2) W12-A2 Emitter-Injektion: die Stempel-Zeile ist REAL PRAESENT (nicht nur beidseitig-leer) -----------
// Ergaenzt (a): die Byte-Identitaet allein bliebe gruen, falls BEIDE Pfade den Stempel stumm nicht emittierten.
// Diese Wache belegt die tatsaechliche Injektion -- Organ- + System-Stempel in X.Y.Z-Voll-Form, in BEIDEN Pfaden
// byte-gleich. Golden-kritisch (Section 43): faellt der Stempel weg, gilt keine chirurgische Cache-Invalidierung.
void check_stamp_injected(std::vector<std::string> const& g320_ids) {
    std::cout << "\n---- (a2) W12-A2 Emitter-Injektion: Versionierungs-Stempel real praesent (beide Pfade) ----\n";
    ex::SourceGenFn const cat  = tlz::generated_make_catalog_source_gen();
    ex::SourceGenFn const lazy = tlz::make_lazy_adhoc_source_gen();
    std::string const     id   = g320_ids.front();
    std::string const     cs   = stamp_line(cat(id));
    std::string const     ls   = stamp_line(lazy(id));
    std::cout << "  lazy-Stempel-Zeile:    " << ls << "\n";
    std::cout << "  Katalog-Stempel-Zeile: " << cs << "\n";
    check_true("(a2) lazy-Quelle traegt COMDARE_ANATOMY_VERSION_STAMP(", !ls.empty());
    check_true("(a2) Katalog-Quelle traegt COMDARE_ANATOMY_VERSION_STAMP(", !cs.empty());
    check_true("(a2) Stempel-Zeile byte-gleich (lazy == Katalog)", ls == cs);
    check_true("(a2) Organ-Stempel in X.Y.Z-Voll-Form (@1.0.0)", ls.find("@1.0.0") != std::string::npos);
    // Der System-Stempel (system_stamp_line) traegt die statischen System-Achsen -- als 2. Makro-Argument.
    check_true("(a2) System-Stempel-Achse compiler=code@ eingebettet", ls.find("compiler=code@") != std::string::npos);
}

// -- (b) Nicht-320 golden-N ids: lazy Gen nicht-leer + Werte-Round-Trip ------------------------------------
void check_golden_n_nonempty(std::vector<std::string> const& g320_ids, std::vector<std::string> const& full_ids) {
    std::cout << "\n---- (b) Nicht-320 golden-N ids: lazy Gen nicht-leer + Werte-Round-Trip ----\n";
    check_eq("(b) FullSourceCatalog view.size() == 131072", full_ids.size(), std::size_t{131072});
    ex::SourceGenFn const       lazy   = tlz::make_lazy_adhoc_source_gen();
    tlz::LazySlotTables const   tables = tlz::lazy_slot_type_tables(); // unabhaengige Referenz fuer den Werte-Check
    std::set<std::string> const golden320(g320_ids.begin(), g320_ids.end());

    // >=5 ids, die NICHT im golden-320-Satz liegen, ueber den Indexraum verteilt (die 320er pinnen 13 Achsen auf
    // Index 0 + search_algo/node_type/memory_layout/prefetch auf die First-K; jede id, die eine andere Achse auf
    // Index 1 stellt bzw. deren Basis-Achse Index >=2 traegt, ist definitiv NICHT im 320er-Satz).
    std::vector<std::string> picks;
    for (std::size_t step = 1; step <= 12 && picks.size() < 6; ++step) {
        std::size_t const i = (full_ids.size() * step) / 13;
        if (i < full_ids.size() && golden320.find(full_ids[i]) == golden320.end()) picks.push_back(full_ids[i]);
    }
    for (std::size_t back = 1; picks.size() < 6 && back <= full_ids.size(); ++back) {
        std::string const& id = full_ids[full_ids.size() - back]; // die letzten: viele Achsen nicht-baseline
        if (golden320.find(id) == golden320.end()) picks.push_back(id);
    }
    check_true("(b) >=5 Nicht-320 golden-N ids gefunden", picks.size() >= 5);

    std::size_t           nonempty = 0, roundtrip = 0;
    std::set<std::string> distinct_bodies;
    for (std::string const& id : picks) {
        std::string const src = lazy(id);
        if (!src.empty()) ++nonempty;
        distinct_bodies.insert(body_after_first_line(src));
        // Werte-Round-Trip: (1) der aus dem id gejointe Macro-Arg-Block steht im Modul; (2) je Achsen-Segment
        // "achse=wert" steht der zugehoerige cpp_type (unabhaengiger Namens-Lookup in tables) im Modul.
        std::string const args    = tlz::lazy_adhoc_macro_args_for(tables, id);
        bool              vals_ok = !args.empty() && src.find(args) != std::string::npos;
        auto const        axes    = ex::ceb_parse_path(id);
        for (std::size_t slot = 0; slot < ex::kCompositionAxisNames.size(); ++slot) {
            std::string_view const ax = ex::kCompositionAxisNames[slot];
            std::string            val;
            for (auto const& kv : axes)
                if (kv.first == ax) {
                    val = kv.second;
                    break;
                }
            std::string cpp;
            for (auto const& e : tables[slot])
                if (e.name == val) {
                    cpp = e.cpp_type;
                    break;
                }
            if (cpp.empty() || src.find(cpp) == std::string::npos) vals_ok = false;
        }
        if (vals_ok) ++roundtrip;
    }
    check_eq("(b) lazy Gen nicht-leer fuer alle Nicht-320-Stichproben", nonempty, picks.size());
    check_eq("(b) binary_id-Werte treffen die gerenderten Typen (Round-Trip)", roundtrip, picks.size());
    check_eq("(b) distinkte ids -> distinkte Modul-Ruempfe", distinct_bodies.size(), picks.size());
}

// -- (c) CRC64-Anker + lazy materialisiert jede enumerierte id ---------------------------------------------
void check_crc64_and_lazy_cover(std::vector<std::string> const& full_ids) {
    std::cout << "\n---- (c) CRC64-Anker (kNewGolden131072Crc64) + lazy deckt jede enumerierte id ----\n";
    check_eq("(c) FullSourceCatalog view.size() == 131072", full_ids.size(), std::size_t{131072});
    // CRC64 ueber die enumerierten view-ids in der committeten Datei-Byte-Konvention (je id + '\n') gegen den
    // Anker. Referenziert kNewGolden131072Crc64 (source_catalog.hpp), aendert ihn NICHT.
    std::uint64_t crc = 0;
    for (std::string const& id : full_ids) {
        crc = fp::crc64_ecma182_update(crc, std::string_view{id});
        crc = fp::crc64_ecma182_update(crc, std::string_view{"\n"});
    }
    std::cout << "  CRC64 = 0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << crc
              << "  (Anker kNewGolden131072Crc64 = 0x" << std::setw(16) << std::setfill('0')
              << tlz::kNewGolden131072Crc64 << ")" << std::dec << "\n";
    check_eq("(c) CRC64 der 131072 view-ids == kNewGolden131072Crc64", crc, tlz::kNewGolden131072Crc64);

    // Der lazy Pfad materialisiert JEDE enumerierte id: Stichprobe ueber den Indexraum (~10 verteilt + die letzte).
    ex::SourceGenFn const lazy    = tlz::make_lazy_adhoc_source_gen();
    std::size_t           checked = 0, nonempty = 0;
    for (std::size_t i = 0; i < full_ids.size(); i += 13107) { // 131072/13107 ~= 10 Stichproben
        ++checked;
        if (!lazy(full_ids[i]).empty()) ++nonempty;
    }
    ++checked; // die letzte id (alle 17 Achsen auf Index 1 = maximal nicht-baseline)
    if (!lazy(full_ids.back()).empty()) ++nonempty;
    check_eq("(c) lazy Gen materialisiert jede Stichproben-id (nicht-leer)", nonempty, checked);
}

// -- (d) Freigabe-Kopplung (Ledger 36/37/37.b): der Emitter rendert SIMD-Organ-Varianten SYSTEM-BLIND ----------
// Belegt, dass der Zulaessigkeits-Filter (Organ-SIMD <= System-Freigabe) NICHT im Emitter sitzt (37.b): der lazy
// Gen rendert eine SIMD-faehige search_algo-Organ-Variante (SimdCapableStrategy: array256/vector_u8u8) UNBEDINGT,
// ohne jede Kenntnis der freigegebenen System-Zelle. Die Kopplung/Degradation liegt an der CompileFn/provision_all-
// Naht (-march), nicht hier. Reiner Render-Check (kein Compile) -- die no_extension-Degradation selbst traegt die
// -march-Abwesenheit + die #ifdef-Skalar-Pfade der Organ-Wrapper (Concept-Vertrag), separater Bau-Beleg.
void check_simd_organ_system_blind(std::vector<std::string> const& full_ids) {
    std::cout
        << "\n---- (d) Freigabe-Kopplung: Emitter rendert SIMD-Organ system-blind (Filter an CompileFn-Naht) ----\n";
    tlz::LazySlotTables const tables = tlz::lazy_slot_type_tables();
    ex::SourceGenFn const     lazy   = tlz::make_lazy_adhoc_source_gen();
    // Eine SIMD-gekoppelte search_algo-Organ-Variante (Slot 0) im enabled-Satz suchen. k_ary traegt
    // supports_simd()==true + SimdCapableStrategy (data-parallel Layout) und ist im Default-Enabled-Satz;
    // array256/vector_u8u8/original_* sind die weiteren SIMD-gekoppelten Kandidaten (falls opt-in enabled).
    std::string simd_name, simd_cpp;
    for (char const* cand : {"k_ary", "array256", "vector_u8u8", "original_art", "original_hot"}) {
        for (auto const& e : tables[0])
            if (e.name == cand) {
                simd_name = e.name;
                simd_cpp  = e.cpp_type;
                break;
            }
        if (!simd_name.empty()) break;
    }
    check_true("(d) SIMD-gekoppelte search_algo-Variante im enabled-Satz (SimdCapableStrategy)", !simd_name.empty());
    if (simd_name.empty()) return;

    // Gueltigen 17-Achsen-binary_id bauen: Baseline aus full_ids[0], search_algo := die SIMD-Organ-Variante.
    auto const  base_axes = ex::ceb_parse_path(full_ids.front());
    std::string bid;
    for (std::size_t slot = 0; slot < ex::kCompositionAxisNames.size(); ++slot) {
        std::string_view const ax = ex::kCompositionAxisNames[slot];
        std::string            val;
        for (auto const& kv : base_axes)
            if (kv.first == ax) {
                val = kv.second;
                break;
            }
        if (ax == std::string_view{"search_algo"}) val = simd_name;
        if (!bid.empty()) bid += '/';
        bid += std::string{ax} + "=" + val;
    }
    std::string const src   = lazy(bid);
    std::string const label = "(d) lazy Gen rendert SIMD-Organ (" + simd_name + ") nicht-leer (system-blind)";
    check_true(label.c_str(), !src.empty());
    check_true("(d) gerenderte Quelle traegt den SIMD-Organ-Typ", src.find(simd_cpp) != std::string::npos);
}

// -- (e) S6-P1b: die Mess-Tooling-HAUPT-Stempel-Zeile (kMeasurementAxisVersionLine) wird in die Modul-Quelle
//    einkompiliert, sobald die vom Planer gewaehlte Combo ein Tooling traegt. Default (LEERE/[all]-Combo, make_lazy_
//    adhoc_source_gen() ohne Arg) => EXAKT die 2-arg-Makro-Zeile => byte-identisch (die Wachen (a)/(a2) bleiben gruen);
//    explizite wallclock/macro-Combo => 3-arg _M-Form mit verschiedenen measurement_tooling=..@1.0.0-Zeilen. Der
//    ADHOC-Block (Organ/System/Umbrella) bleibt unveraendert => binary_id/CRC UNBERUEHRT (der Stempel != binary_id).
void check_measurement_stamp_wiring(std::vector<std::string> const& g320_ids) {
    std::cout << "\n---- (e) S6-P1b: Mess-Tooling-Stempel je Combo in die Modul-Quelle einkompiliert ----\n";
    namespace abi                 = ::comdare::cache_engine::abi;
    std::string const     id      = g320_ids.front();
    ex::SourceGenFn const def     = tlz::make_lazy_adhoc_source_gen(); // [all]/leer
    ex::SourceGenFn const wc      = tlz::make_lazy_adhoc_source_gen(abi::measurement_stamp_line("wallclock"));
    ex::SourceGenFn const ma      = tlz::make_lazy_adhoc_source_gen(abi::measurement_stamp_line("macro"));
    std::string const     src_def = def(id);
    std::string const     src_wc  = wc(id);
    std::string const     src_ma  = ma(id);

    // Default: EXAKT die 2-arg-Makro-Zeile, KEIN _M, KEIN measurement_tooling (byte-identisch zu (a)/(a2)).
    check_true("(e) Default-Quelle traegt 2-arg COMDARE_ANATOMY_VERSION_STAMP(",
               src_def.find("COMDARE_ANATOMY_VERSION_STAMP(") != std::string::npos);
    check_true("(e) Default-Quelle traegt KEIN _M / kein measurement_tooling (byte-stabil)",
               src_def.find("COMDARE_ANATOMY_VERSION_STAMP_M(") == std::string::npos &&
                   src_def.find("measurement_tooling=") == std::string::npos);

    // wallclock: 3-arg _M-Form mit measurement_tooling=wallclock@1.0.0.
    check_true("(e) wallclock-Quelle traegt 3-arg COMDARE_ANATOMY_VERSION_STAMP_M(",
               src_wc.find("COMDARE_ANATOMY_VERSION_STAMP_M(") != std::string::npos);
    check_true("(e) wallclock-Quelle traegt measurement_tooling=wallclock@1.0.0",
               src_wc.find("measurement_tooling=wallclock@1.0.0") != std::string::npos);

    // macro: verschiedene measurement-Zeile => 2 Combos => 2 verschiedene DLL-Quellen.
    check_true("(e) macro-Quelle traegt measurement_tooling=macro@1.0.0",
               src_ma.find("measurement_tooling=macro@1.0.0") != std::string::npos);
    check_true("(e) wallclock- und macro-Quelle verschieden (je Combo verschiedene Bytes)", src_wc != src_ma);
    check_true("(e) wallclock-Quelle != Default-Quelle (Stempel additiv, nicht ersetzend)", src_wc != src_def);

    // Der ADHOC-Block (alles VOR der Stempel-Makro-Zeile: Umbrella + ADHOC-Block) bleibt byte-gleich -> nur die
    // Mess-Zeile differenziert -> binary_id/CRC neutral (der Stempel lebt separat im kompilierten Binary).
    auto adhoc_block = [](std::string const& s) {
        std::size_t const p = s.find("COMDARE_ANATOMY_VERSION_STAMP");
        return p == std::string::npos ? s : s.substr(0, p);
    };
    check_true("(e) ADHOC-Block byte-gleich Default==wallclock (Organ/System unveraendert, Stempel additiv)",
               adhoc_block(src_def) == adhoc_block(src_wc));
}

// -- (f) S6-P1b Env-Bruecke + K7b-2 (Section 64-D1-B): COMDARE_MEASUREMENT_COMBO (Combo-Legende) -> Mengen-Stempel ->
//    Quelle. Der Legende-Helfer (abi::measurement_stamp_line_from_combo_legend) und die LIVE-Naht
//    (make_lazy_adhoc_source_gen_from_env) bilden die letzte Meile: die vom Planer per Env gereichte Combo stempelt die
//    je-Combo-DLL-Quelle als MENGE. BEWUSSTE Byte-Aenderung (K7b-2, 22.07.): [all]/UNGESETZT stempelt NEU die VOLLE
//    3-Tool-Vollmenge (3-arg _M-Zeilen) statt leer -- die Section-64-Regression (leere [all]-Provenienz) ist geschlossen.
//    UNBERUEHRT (2-arg, byte-stabil): der No-Arg-Default make_lazy_adhoc_source_gen() bleibt "" -> die 320er-Katalog-
//    Wachen (a)/(a2) + der binary_id/CRC-Anker (c) bleiben GRUEN (der Stempel != binary_id).
void check_measurement_combo_env_bridge(std::vector<std::string> const& g320_ids) {
    std::cout << "\n---- (f) K7b-2 Env-Bruecke: COMDARE_MEASUREMENT_COMBO -> Mengen-Stempel in der Quelle ----\n";
    namespace abi = ::comdare::cache_engine::abi;
    // Einzel-Legende [wallclock] -> genau EIN Eintrag (unveraendert).
    check_eq("(f) legend [wallclock] -> Einzel-Stempel", abi::measurement_stamp_line_from_combo_legend("[wallclock]"),
             std::string{"measurement_tooling=wallclock@1.0.0"});
    // Mehr-Token-Legende [wallclock,macro] -> MENGE (K7b-2): 2 Eintraege, ';'-getrennt (Eingabe-Reihenfolge).
    check_eq("(f) legend [wallclock,macro] -> Mengen-Stempel (2 Eintraege)",
             abi::measurement_stamp_line_from_combo_legend("[wallclock,macro]"),
             std::string{"measurement_tooling=wallclock@1.0.0;measurement_tooling=macro@1.0.0"});
    // [all] -> die VOLLE Vollmenge (Section 64-D1-B): == measurement_stamp_line_full_set(), 3 Eintraege {wc,macro,micro}.
    std::string const full = abi::measurement_stamp_line_full_set();
    check_eq("(f) legend [all] -> Vollmenge (== full_set)", abi::measurement_stamp_line_from_combo_legend("[all]"),
             full);
    check_true("(f) full_set traegt wallclock+macro+micro (3 Eintraege, 2 Trenner)",
               full.find("measurement_tooling=wallclock@1.0.0") != std::string::npos &&
                   full.find("measurement_tooling=macro@1.0.0") != std::string::npos &&
                   full.find("measurement_tooling=micro@1.0.0") != std::string::npos &&
                   std::count(full.begin(), full.end(), ';') == 2);
    // Leere Legende (== "keine Legende gereicht") -> leer; die Vollmengen-Default-Semantik entscheidet der from_env-Zweig.
    check_true("(f) leere legend -> leer", abi::measurement_stamp_line_from_combo_legend("").empty());

    std::string const id = g320_ids.front();
    // Env gesetzt ([macro]) -> die from_env-Naht stempelt die Quelle mit measurement_tooling=macro.
    ::setenv("COMDARE_MEASUREMENT_COMBO", "[macro]", 1);
    std::string const src_env = tlz::make_lazy_adhoc_source_gen_from_env()(id);
    ::unsetenv("COMDARE_MEASUREMENT_COMBO");
    check_true("(f) Env [macro] -> Quelle traegt measurement_tooling=macro@1.0.0",
               src_env.find("measurement_tooling=macro@1.0.0") != std::string::npos);
    // Env UNGESETZT == [all] -> NEU die VOLLE Vollmenge (Section 64-D1-B); byte-gleich zu einem explizit full_set-
    // gestempelten Gen. Der No-Arg-Default make_lazy_adhoc_source_gen() bleibt "" (2-arg) -> UNGESETZT != No-Arg-Default.
    std::string const src_unset = tlz::make_lazy_adhoc_source_gen_from_env()(id);
    check_true("(f) Env ungesetzt -> traegt die volle 3-Tool-Vollmenge",
               src_unset.find("measurement_tooling=wallclock@1.0.0") != std::string::npos &&
                   src_unset.find("measurement_tooling=macro@1.0.0") != std::string::npos &&
                   src_unset.find("measurement_tooling=micro@1.0.0") != std::string::npos);
    check_true("(f) Env ungesetzt == explizit full_set-gestempelte Quelle",
               src_unset == tlz::make_lazy_adhoc_source_gen(abi::measurement_stamp_line_full_set())(id));
    // Der No-Arg-Default bleibt 2-arg/leer (byte-stabil) -> BEWUSST verschieden vom [all]/UNGESETZT-Vollmengen-Pfad.
    std::string const src_noarg = tlz::make_lazy_adhoc_source_gen()(id);
    check_true("(f) No-Arg-Default traegt KEIN measurement_tooling (2-arg byte-stabil)",
               src_noarg.find("measurement_tooling=") == std::string::npos);
    check_true("(f) UNGESETZT-Vollmenge != No-Arg-Default (bewusste [all]-3-arg-Byte-Aenderung)",
               src_unset != src_noarg);
}

// -- (a3) I2 Fingerprint-Drift-Beweis: der FingerprintFn == sha512 der EMITTIERTEN Stempel-Zeilen ---------------
// Unabhaengige Referenz = der emittierte Makro-Text (den die DLL kompiliert): parse die gequoteten organ/system-
// Argumente aus COMDARE_ANATOMY_VERSION_STAMP(...) + runtime-sha512(concat) == lazy_adhoc_fingerprint_for. Damit deckt
// sich der .fingerprint-Sidecar-Wert byte-genau mit dem sha512_line, den die DLL einkompiliert traegt (anatomy_
// fingerprint_hex = dieselbe concat+sha512+to_hex-Primitive). measurement/merge = "" im 2-arg-Default.
std::vector<std::string> quoted_args(std::string const& line) {
    std::vector<std::string> out;
    std::size_t              pos = 0;
    while (true) {
        std::size_t const a = line.find('"', pos);
        if (a == std::string::npos) break;
        std::size_t const b = line.find('"', a + 1);
        if (b == std::string::npos) break;
        out.push_back(line.substr(a + 1, b - (a + 1)));
        pos = b + 1;
    }
    return out;
}

void check_fingerprint_drift_free(std::vector<std::string> const& g320_ids) {
    std::cout
        << "\n---- (a3) I2 Fingerprint-Drift-Beweis: FingerprintFn == sha512 der emittierten Stempel-Zeilen ----\n";
    auto const                     version_table = ex::build_axis_variant_version_table();
    auto const                     tables        = tlz::lazy_slot_type_tables();
    ex::SourceGenFn const          lazy          = tlz::make_lazy_adhoc_source_gen(); // 2-arg, measurement=""
    std::string const              id            = g320_ids.front();
    std::vector<std::string> const args          = quoted_args(stamp_line(lazy(id)));
    check_true("(a3) 2-arg-Stempel traegt organ+system", args.size() >= 2);
    if (args.size() < 2) return;
    std::string const preimage = args[0] + args[1]; // organ+system (measurement="" merge="" im 2-arg-Default)
    auto const        digest   = ::comdare::cache_engine::sha512::sha512(
        std::span<std::uint8_t const>{reinterpret_cast<std::uint8_t const*>(preimage.data()), preimage.size()});
    auto const        hex = ::comdare::cache_engine::sha512::to_hex(digest);
    std::string const emitted_fp(hex.data(), hex.size());
    std::string const provider_fp = tlz::lazy_adhoc_fingerprint_for(tables, id, version_table, /*measurement=*/{});
    check_true("(a3) FingerprintFn ist 128-hex", provider_fp.size() == 128);
    check_eq("(a3) FingerprintFn == sha512(emittierte organ+system) -- drift-frei zum DLL-sha512_line", provider_fp,
             emitted_fp);
}

} // namespace

int main() {
    std::cout << "==== INC-G6 lazy_adhoc_source_gen Round-Trip-Gate ====\n";

    // Die zwei Referenz-Baeume EINMAL lazy aufbauen (view[i], kein mp_product):
    //   g320  = der MATERIALISIERTE 320er-Katalog (generated_source_catalog.hpp, aus m3v2 via catalog_codegen.cmake),
    //   full  = die golden-REFERENZ FullSourceCatalog (alle 17 Achsen je 2 = 2^17 = 131072).
    std::vector<std::string> const g320_ids = binary_ids(tlz::generated_catalog_static_levels());
    std::vector<std::string> const full_ids = binary_ids(tlz::catalog_static_levels<tlz::FullSourceCatalog>());

    check_320_byte_identity(g320_ids);
    check_stamp_injected(g320_ids);
    check_measurement_stamp_wiring(g320_ids);
    check_measurement_combo_env_bridge(g320_ids);
    check_golden_n_nonempty(g320_ids, full_ids);
    check_fingerprint_drift_free(g320_ids); // I2: der .fingerprint-Sidecar-Provider deckt sich mit dem DLL-sha512_line
    check_crc64_and_lazy_cover(full_ids);
    check_simd_organ_system_blind(full_ids);

    std::cout << "\n==== INC-G6 lazy Gate: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
