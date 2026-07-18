// Limits-Entkopplung Vorstufe: generierter Katalog bleibt zum bestehenden
// FullSourceCatalog positionsidentisch und liefert Quellen fuer alle Golden-IDs.

#include "generated_source_catalog.hpp"

#include <builder/experiment_tree/experiment_tree.hpp>
#include <cache_engine/fingerprint/crc64.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace fs  = ::std::filesystem;
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

std::vector<std::string> load_golden(fs::path const& p) {
    std::vector<std::string> ids;
    std::ifstream            f{p};
    std::string              line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        ids.push_back(line);
    }
    return ids;
}

std::vector<std::string> binary_ids(std::vector<ex::AxisLevel> const& levels) {
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);
    ex::StaticBinaryView const view = tree.static_binary_view();

    std::vector<std::string> ids;
    ids.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
    return ids;
}

void check_level_equivalence(std::vector<ex::AxisLevel> const& generated, std::vector<ex::AxisLevel> const& reference) {
    check_eq("Stufe 1: static_levels count", generated.size(), reference.size());
    check_eq("Stufe 1: static_levels count == 17", generated.size(), std::size_t{17}); // INC-2d: isa raus

    bool axes_ok   = generated.size() == reference.size();
    bool values_ok = axes_ok;
    for (std::size_t i = 0; i < generated.size() && i < reference.size(); ++i) {
        axes_ok   = axes_ok && (generated[i].axis == reference[i].axis) &&
                    (generated[i].is_static == reference[i].is_static) &&
                    (generated[i].block_id == reference[i].block_id);
        values_ok = values_ok && (generated[i].values == reference[i].values);
    }
    check_true("Stufe 1: Achsen-Metadaten positionsidentisch zu FullSourceCatalog", axes_ok);
    check_true("Stufe 1: Wertelisten positionsidentisch zu FullSourceCatalog", values_ok);
}

void check_golden(std::vector<std::string> const& generated_ids, std::vector<std::string> const& golden_ids) {
    check_eq("Stufe 2: generated binary_count", generated_ids.size(), std::size_t{320});
    check_eq("Stufe 2: golden binary_count", golden_ids.size(), std::size_t{320});

    std::size_t mismatches = 0;
    for (std::size_t i = 0; i < generated_ids.size() && i < golden_ids.size(); ++i)
        if (generated_ids[i] != golden_ids[i]) ++mismatches;
    check_eq("Stufe 2: Golden-Positionsvergleich Mismatch", mismatches, std::size_t{0});
    if (mismatches == 0 && generated_ids.size() == golden_ids.size() && generated_ids.size() == 320) {
        std::cout << "Golden-Positionsvergleich OK: 320 Positionen identisch\n";
    }
}

void check_source_gen(std::vector<std::string> const& golden_ids) {
    ex::SourceGenFn gen = tlz::generated_make_catalog_source_gen();

    std::size_t empty = 0;
    for (std::string const& id : golden_ids) {
        std::string const source = gen(id);
        if (source.empty()) ++empty;
    }
    check_eq("Stufe 3: leere Quellen fuer Golden-IDs", empty, std::size_t{0});
    check_true("Stufe 3: unbekannte ID liefert leere Quelle", gen("__unknown_binary_id__").empty());
}

// NEW-GOLDEN-ALL-AXES (2026-07-18): der neue golden-REFERENZ-Anker. FullSourceCatalog variiert ALLE 17 Achsen je 2 =
// 2^17 = 131072. Der Count ist compile-verankert (static_assert(catalog_axis_product<FullSourceCatalog>()==131072) in
// source_catalog.hpp); HIER die test-Verankerung ueber den LAZY StaticBinaryView (view.size(); O(Tiefe) je Dekodierung,
// KEINE mp_product-Materialisierung).
//
// FEASIBILITY (§0-GOAL CRC64): der Referenz-Anker ist eine CRC-64/ECMA-182, NICHT eine 62-MB-Datei (die kaeme ueber
// die Mirrors + CI als irreversibler Repo-Bloat ins git). Wir regenerieren die 131072 ids ON-DEMAND (lazy) und
// akkumulieren die CRC64 in EXAKT der Datei-Byte-Konvention (je binary_id gefolgt von '\n', OHNE Kommentar-Header),
// dann Abgleich gegen den committeten constexpr kNewGolden131072Crc64. O(N) Zeit, O(1) Zusatzspeicher fuer die CRC.
void check_new_golden_131072() {
    std::cout << "\n---- NEW-GOLDEN (2^17 = 131072, alle 17 Achsen je 2; CRC64-Anker statt 62-MB-Datei) ----\n";

    // compile-verankerter Count (∏ mp_size, EXPLOSIONSFREI) == der emergente Referenz-Count.
    check_eq("catalog_axis_product<FullSourceCatalog>() == 131072", tlz::catalog_axis_product<tlz::FullSourceCatalog>(),
             std::size_t{131072});
    check_eq("catalog_axis_product<golden_320_catalog>() == 320", tlz::catalog_axis_product<tlz::golden_320_catalog>(),
             std::size_t{320});

    std::vector<ex::AxisLevel> const ref_levels = tlz::catalog_static_levels<tlz::FullSourceCatalog>();
    check_eq("NEW-GOLDEN: FullSourceCatalog static_levels == 17", ref_levels.size(), std::size_t{17});
    std::vector<std::string> const ref_ids = binary_ids(ref_levels); // LAZY view-Iteration (kein mp_product)
    check_eq("NEW-GOLDEN: FullSourceCatalog view.size() == 131072", ref_ids.size(), std::size_t{131072});

    // CRC64 ueber die regenerierten ids (Datei-Byte-Konvention: je id + '\n') gegen den committeten Anker.
    std::uint64_t crc = 0;
    for (std::string const& id : ref_ids) {
        crc = fp::crc64_ecma182_update(crc, std::string_view{id});
        crc = fp::crc64_ecma182_update(crc, std::string_view{"\n"});
    }
    std::cout << "NEW-GOLDEN CRC64-ECMA-182 = 0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0')
              << crc << "  (anchor kNewGolden131072Crc64 = 0x" << std::setw(16) << std::setfill('0')
              << tlz::kNewGolden131072Crc64 << ")" << std::dec << "\n";
    check_eq("NEW-GOLDEN: CRC64 der 131072 ids == kNewGolden131072Crc64", crc, tlz::kNewGolden131072Crc64);
    if (crc == tlz::kNewGolden131072Crc64 && ref_ids.size() == 131072)
        std::cout << "NEW-GOLDEN OK: 131072 ids regeneriert, CRC64 identisch zum Anker (keine 62-MB-Datei noetig)\n";
}

} // namespace

int main(int argc, char** argv) {
    fs::path const golden_txt =
        (argc >= 2) ? fs::path(argv[1])
                    : (fs::path("tests") / "unit" / "thesis_tiere" / "golden_fullpilot_320_binary_ids.txt");

    std::cout << "==== Limits-Entkopplung Vorstufe ====\n";
    std::cout << "Golden: " << golden_txt.string() << "\n";

    // ENTKOPPLUNGS-GRENZE (NEW-GOLDEN-ALL-AXES): der MATERIALISIERTE Katalog (GeneratedFullSourceCatalog, aus
    // m3v2_study.profile.xml via catalog_codegen.cmake) bleibt bei 320. Die Positionsidentitaet wird deshalb gegen
    // golden_320_catalog (die messdaten-erhaltende 4·4·5·4-Grundlage) geprueft — NICHT gegen den auf 2^17 angehobenen
    // FullSourceCatalog (der ist der SEPARATE, lazy golden-REFERENZ-Detektor, s. check_new_golden_131072).
    std::vector<ex::AxisLevel> const generated = tlz::generated_catalog_static_levels();
    std::vector<ex::AxisLevel> const reference = tlz::catalog_static_levels<tlz::golden_320_catalog>();
    check_level_equivalence(generated, reference);

    std::vector<std::string> const golden_ids    = load_golden(golden_txt);
    std::vector<std::string> const generated_ids = binary_ids(generated);
    check_golden(generated_ids, golden_ids);

    if (golden_ids.size() == 320) check_source_gen(golden_ids);

    // NEW-GOLDEN: der 2^17-Referenz-Anker — regeneriert lazy + CRC64 gegen kNewGolden131072Crc64 (keine 62-MB-Datei).
    check_new_golden_131072();

    std::cout << "\n==== Limits-Entkopplung Vorstufe: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
