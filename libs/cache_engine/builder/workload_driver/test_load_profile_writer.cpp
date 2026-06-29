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
#include <filesystem>
#include <fstream>
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
    if (g_log) std::fclose(g_log);
    return g_fail == 0 ? 0 : 1;
}
