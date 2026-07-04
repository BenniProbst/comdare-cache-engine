// test_load_profile_writer.cpp — LITERALER Beleg für #175 (write_load_profile_xml + extract_load_profile_from_measurements).
//
// (a) ROUND-TRIP: ein Bestands-LP-XML parsen → write_load_profile_xml → erneut parsen → die zwei LoadProfiles
//     sind FELD-IDENTISCH (semantischer Round-Trip; Formatierungs-Unterschiede ok).
// (b) EXTRAKTION: gegen die echte Pilot-CSV je search_algo (Architekturfokus) ein LoadProfile extrahieren + als
//     XML ablegen → op-mix spiegelt die echten op_*_n; das XML ist via parse_load_profile RE-KONSUMIERBAR.
//
// Standalone, header-only Targets: nur Standard-Lib. vcvars64 + cl.

#include "load_profile_parser.hpp"
#include "load_profile_writer.hpp"

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace wd = comdare::cache_engine::builder::workload_driver;
namespace fs = std::filesystem;

namespace {

std::FILE* g_log = nullptr; // optionaler Tee-Log (Argument 4) für robusten BELEG-Mitschnitt
template <class... A>
void emit(char const* fmt, A... a) {
    std::printf(fmt, a...);
    std::fflush(stdout);
    if (g_log) {
        std::fprintf(g_log, fmt, a...);
        std::fflush(g_log);
    }
}

int  g_fail = 0;
void check(bool ok, char const* what) {
    emit("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_fail;
}

bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

bool fields_identical(wd::LoadProfile const& a, wd::LoadProfile const& b, std::string const& tag) {
    bool ok    = true;
    auto cmp_s = [&](char const* f, std::string const& x, std::string const& y) {
        if (x != y) {
            emit("    DIFF %s: '%s' != '%s'\n", f, x.c_str(), y.c_str());
            ok = false;
        }
    };
    auto cmp_u = [&](char const* f, std::uint64_t x, std::uint64_t y) {
        if (x != y) {
            emit("    DIFF %s: %llu != %llu\n", f, (unsigned long long)x, (unsigned long long)y);
            ok = false;
        }
    };
    auto cmp_d = [&](char const* f, double x, double y) {
        if (!near(x, y)) {
            emit("    DIFF %s: %.9f != %.9f\n", f, x, y);
            ok = false;
        }
    };
    cmp_s("id", a.id, b.id);
    cmp_s("paper_ref", a.paper_ref, b.paper_ref);
    cmp_s("catalog_lp_id", a.catalog_lp_id, b.catalog_lp_id);
    cmp_s("pretty_name", a.pretty_name, b.pretty_name);
    cmp_u("records", a.records, b.records);
    cmp_u("num_operations", a.num_operations, b.num_operations);
    cmp_u("seed", a.config.seed, b.config.seed);
    cmp_d("pct_insert", a.config.pct_insert, b.config.pct_insert);
    cmp_d("pct_lookup", a.config.pct_lookup, b.config.pct_lookup);
    cmp_d("pct_erase", a.config.pct_erase, b.config.pct_erase);
    cmp_d("pct_clear", a.config.pct_clear, b.config.pct_clear);
    cmp_d("pct_scan", a.config.pct_scan, b.config.pct_scan);
    cmp_d("pct_rmw", a.config.pct_rmw, b.config.pct_rmw);
    cmp_u("key_distribution", (std::uint64_t)a.config.key_distribution, (std::uint64_t)b.config.key_distribution);
    cmp_d("zipfian_theta", a.config.zipfian_theta, b.config.zipfian_theta);
    cmp_d("negative_query_pct", a.config.negative_query_pct, b.config.negative_query_pct);
    cmp_u("scan_length_max", a.config.scan_length_max, b.config.scan_length_max);
    emit("  [%s] field-identity %s\n", ok ? "PASS" : "FAIL", tag.c_str());
    return ok;
}

struct CatalogExpectation {
    char const* file;
    char const* lp_id;
    double      insert;
    double      lookup;
    double      erase;
    double      clear;
    double      scan;
    double      rmw;
    double      negative_query_pct;
};

std::vector<CatalogExpectation> catalog_expectations() {
    return {{"lp_bulk_insert.xml", "LP01", 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {"lp_read_uniform.xml", "LP04", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {"ycsb_c.xml", "LP05", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {"coco_p04_neg0.xml", "LP06", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
            {"coco_p04_neg25.xml", "LP06", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 25.0},
            {"coco_p04_neg50.xml", "LP06", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 50.0},
            {"coco_p04_neg75.xml", "LP06", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 75.0},
            {"coco_p04_neg100.xml", "LP06", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 100.0},
            {"lp_range_scan.xml", "LP08", 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0},
            {"lp_balanced_5050.xml", "LP09", 0.50, 0.50, 0.0, 0.0, 0.0, 0.0, 0.0},
            {"lp_delete_heavy.xml", "LP10", 0.50, 0.0, 0.50, 0.0, 0.0, 0.0, 0.0},
            {"lp_mixed_oltp.xml", "LP11", 0.25, 0.50, 0.0, 0.0, 0.0, 0.25, 0.0},
            {"lp_concurrent_rmw.xml", "LP12", 0.25, 0.09, 0.36, 0.0, 0.09, 0.09, 0.0},
            {"lp_dynamic_trace.xml", "LP14", 0.09, 0.91, 0.0, 0.0, 0.0, 0.0, 0.0}};
}

std::map<std::string, CatalogExpectation> catalog_by_file() {
    std::map<std::string, CatalogExpectation> out;
    for (auto const& e : catalog_expectations()) out.emplace(e.file, e);
    return out;
}

std::map<std::string, int> expected_lp_id_counts() {
    return {{"LP01", 1}, {"LP04", 1}, {"LP05", 1}, {"LP06", 5}, {"LP08", 1},
            {"LP09", 1}, {"LP10", 1}, {"LP11", 1}, {"LP12", 1}, {"LP14", 1}};
}

std::vector<std::string> documented_absent_lp_ids() { return {"LP02", "LP03", "LP07", "LP13"}; }

std::string read_text(fs::path const& p) {
    std::ifstream in{p, std::ios::binary};
    if (!in) return {};
    std::string out;
    std::getline(in, out, '\0');
    return out;
}

bool op_mix_matches(wd::LoadProfile const& lp, CatalogExpectation const& e) {
    return near(lp.config.pct_insert, e.insert) && near(lp.config.pct_lookup, e.lookup) &&
           near(lp.config.pct_erase, e.erase) && near(lp.config.pct_clear, e.clear) &&
           near(lp.config.pct_scan, e.scan) && near(lp.config.pct_rmw, e.rmw);
}

void run_ap11_catalog_checks(fs::path const& lp_dir) {
    auto const expected_by_file = catalog_by_file();
    auto const expected_counts  = expected_lp_id_counts();
    std::map<std::string, int> actual_counts;
    std::set<std::string>      seen_files;

    auto const discovered = wd::discover_load_profiles(lp_dir);
    check(!discovered.empty(), "discover_load_profiles returns real profiles");

    for (auto const& entry : discovered) {
        auto parsed = wd::parse_load_profile(entry.second);
        std::string const parse_msg = "parse discovered " + entry.second.filename().string();
        check(parsed.has_value(), parse_msg.c_str());
        if (!parsed || parsed->catalog_lp_id.empty()) continue;

        std::string const file = entry.second.filename().string();
        seen_files.insert(file);
        auto const expected = expected_by_file.find(file);
        std::string const known_file_msg = "catalog-tagged file is expected: " + file;
        check(expected != expected_by_file.end(), known_file_msg.c_str());
        ++actual_counts[parsed->catalog_lp_id];
        if (expected == expected_by_file.end()) continue;

        std::string const id_msg = "catalog lp_id mapping " + file + " -> " + expected->second.lp_id;
        check(parsed->catalog_lp_id == expected->second.lp_id, id_msg.c_str());
        std::string const mix_msg = "catalog op_mix faithful " + file;
        check(op_mix_matches(*parsed, expected->second), mix_msg.c_str());
        std::string const neg_msg = "catalog negative_query_pct faithful " + file;
        check(near(parsed->config.negative_query_pct, expected->second.negative_query_pct), neg_msg.c_str());
    }

    check(seen_files.size() == expected_by_file.size(), "exactly 14 real catalog files carry lp_id");
    for (auto const& [file, expected] : expected_by_file) {
        fs::path const xml = lp_dir / file;
        auto parsed = wd::parse_load_profile(xml);
        std::string const msg = "explicit catalog file present+tagged " + file;
        check(parsed.has_value() && parsed->catalog_lp_id == expected.lp_id, msg.c_str());
    }

    for (auto const& [lp_id, expected_count] : expected_counts) {
        std::string const msg = "covered LP-ID count " + lp_id;
        check(actual_counts[lp_id] == expected_count, msg.c_str());
    }
    for (auto const& [lp_id, actual_count] : actual_counts) {
        (void)actual_count;
        std::string const msg = "no unknown/non-realized catalog LP-ID " + lp_id;
        check(expected_counts.count(lp_id) == 1, msg.c_str());
    }

    for (auto const& absent : documented_absent_lp_ids()) {
        std::string const absent_msg = "documented absence is not materialized as XML " + absent;
        check(actual_counts.count(absent) == 0, absent_msg.c_str());
    }

    std::string const schema = read_text(lp_dir / "SCHEMA.md");
    check(!schema.empty(), "SCHEMA.md readable for absence documentation");
    for (auto const& absent : documented_absent_lp_ids()) {
        std::string const schema_msg = "SCHEMA.md documents absence " + absent;
        check(schema.find(absent) != std::string::npos, schema_msg.c_str());
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 4) {
        emit("usage: %s <load_profiles_dir> <pilot_csv> <out_dir>\n", argv[0]);
        return 2;
    }
    fs::path const  lp_dir{argv[1]};
    fs::path const  csv{argv[2]};
    fs::path const  out_dir{argv[3]};
    std::error_code ec;
    fs::create_directories(out_dir, ec);
    if (argc >= 5) g_log = std::fopen(argv[4], "wb"); // optionaler Tee-Log für robusten BELEG-Mitschnitt

    // ── (a) ROUND-TRIP über mehrere Bestands-LP-XMLs ───────────────────────────────────────────────
    emit("=== (a) ROUND-TRIP (parse -> write -> parse, feld-identisch) ===\n");
    std::vector<std::string> const samples = {"ycsb_c.xml", "ycsb_a.xml", "coco_p04_neg25.xml", "ycsb_e.xml"};
    int                            rt_done = 0;
    for (auto const& name : samples) {
        fs::path const xml = lp_dir / name;
        if (!fs::exists(xml)) {
            emit("  (skip, fehlt) %s\n", name.c_str());
            continue;
        }
        auto p1 = wd::parse_load_profile(xml);
        check(p1.has_value(), (std::string("parse#1 ") + name).c_str());
        if (!p1) continue;
        std::string const written = wd::write_load_profile_xml(*p1);
        if (!p1->catalog_lp_id.empty()) {
            check(written.find(" lp_id=\"" + p1->catalog_lp_id + "\"") != std::string::npos,
                  (std::string("writer emits lp_id ") + name).c_str());
        }
        // re-parse über Datei (echter Round-Trip durch den Writer-Dateipfad)
        fs::path const rt = out_dir / (std::string("roundtrip_") + name);
        check(wd::write_load_profile_xml(*p1, rt), (std::string("write ") + name).c_str());
        auto p2 = wd::parse_load_profile(rt);
        check(p2.has_value(), (std::string("parse#2 ") + name).c_str());
        if (!p2) continue;
        check(fields_identical(*p1, *p2, name), (std::string("ROUND-TRIP ") + name).c_str());
        ++rt_done;
        (void)written;
    }
    check(rt_done > 0, "at least one round-trip executed");

    // -- (ap11) LP-ID-Katalogtreue ------------------------------------------------------------
    emit("\n=== (ap11) LP-ID-KATALOG (Doc 32 mapping, no synthetic files) ===\n");
    run_ap11_catalog_checks(lp_dir);

    // ── (b) EXTRAKTION gegen die echte Pilot-CSV je Architekturfokus ──────────────────────────────
    emit("\n=== (b) EXTRAKTION (Pilot-CSV je search_algo -> XML, re-konsumierbar) ===\n");
    std::vector<std::string> const foci      = {"k_ary", "sota::A::ArtComposition", "sota::A::PrtArtComposition",
                                                "sota::B::HotPrtStufe2ReplaceComposition"};
    int                            extracted = 0;
    for (auto const& focus : foci) {
        auto lp = wd::extract_load_profile_from_measurements(csv, focus);
        if (!lp) {
            emit("  [FAIL] extract '%s' -> nullopt\n", focus.c_str());
            ++g_fail;
            continue;
        }
        emit("  focus=%-42s ops=%llu ws(records)=%llu  mix[i/l/e/c/s/r]=%.3f/%.3f/%.3f/%.3f/%.3f/%.3f\n", focus.c_str(),
             (unsigned long long)lp->num_operations, (unsigned long long)lp->records, lp->config.pct_insert,
             lp->config.pct_lookup, lp->config.pct_erase, lp->config.pct_clear, lp->config.pct_scan,
             lp->config.pct_rmw);
        // op-mix spiegelt Daten: Summe ~ 1.0
        double const sum = lp->config.pct_insert + lp->config.pct_lookup + lp->config.pct_erase + lp->config.pct_clear +
                           lp->config.pct_scan + lp->config.pct_rmw;
        check(near(sum, 1.0), (std::string("op-mix sums to 1.0 [") + focus + "]").c_str());
        // Pilot ist lookup-lastig -> pct_lookup dominiert
        check(lp->config.pct_lookup >= lp->config.pct_insert && lp->config.pct_lookup >= lp->config.pct_erase,
              (std::string("lookup dominates [") + focus + "]").c_str());

        // sanitize Fokus-Name für Dateinamen (':' / '/' -> '_')
        std::string fn = focus;
        for (char& ch : fn)
            if (ch == ':' || ch == '/' || ch == ' ') ch = '_';
        fs::path const xml_out = out_dir / (std::string("extracted_") + fn + ".xml");
        check(wd::write_load_profile_xml(*lp, xml_out), (std::string("write extracted XML [") + focus + "]").c_str());

        // RE-KONSUMIERBAR: das exportierte XML ist via parse_load_profile wieder parsbar + feld-identisch
        auto reparsed = wd::parse_load_profile(xml_out);
        check(reparsed.has_value(), (std::string("re-parse extracted [") + focus + "]").c_str());
        if (reparsed)
            check(fields_identical(*lp, *reparsed, std::string("extracted ") + focus),
                  (std::string("EXTRACT ROUND-TRIP [") + focus + "]").c_str());
        ++extracted;
    }
    check(extracted == (int)foci.size(), "all foci extracted");

    emit("\n=== RESULT: %s (failures=%d) ===\n", g_fail == 0 ? "ALL PASS" : "FAILURES", g_fail);
    if (g_fail == 0) emit("[ PASSED ] test_load_profile_writer\n");
    if (g_log) std::fclose(g_log);
    return g_fail == 0 ? 0 : 1;
}
