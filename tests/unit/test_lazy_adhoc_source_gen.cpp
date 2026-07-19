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
#include <cache_engine/fingerprint/crc64.hpp>          // crc64_ecma182_update

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <set>
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

} // namespace

int main() {
    std::cout << "==== INC-G6 lazy_adhoc_source_gen Round-Trip-Gate ====\n";

    // Die zwei Referenz-Baeume EINMAL lazy aufbauen (view[i], kein mp_product):
    //   g320  = der MATERIALISIERTE 320er-Katalog (generated_source_catalog.hpp, aus m3v2 via catalog_codegen.cmake),
    //   full  = die golden-REFERENZ FullSourceCatalog (alle 17 Achsen je 2 = 2^17 = 131072).
    std::vector<std::string> const g320_ids = binary_ids(tlz::generated_catalog_static_levels());
    std::vector<std::string> const full_ids = binary_ids(tlz::catalog_static_levels<tlz::FullSourceCatalog>());

    check_320_byte_identity(g320_ids);
    check_golden_n_nonempty(g320_ids, full_ids);
    check_crc64_and_lazy_cover(full_ids);
    check_simd_organ_system_blind(full_ids);

    std::cout << "\n==== INC-G6 lazy Gate: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
