// Audit A1 / MAJOR-MESS-05 — parse_load_profile: fehlendes/leeres <op_mix> HART ablehnen (nullopt) statt still durch
// WorkloadConfig-Defaults (inkl. 1% Clear) zu ersetzen. Negativ-Test: XML ohne op_mix → nullopt; XML mit op_mix → ok.
// Self-contained (schreibt Temp-XMLs), kein Boost. Build: cl /I libs/cache_engine.

#include "builder/workload_driver/load_profile_parser.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace wd = comdare::cache_engine::builder::workload_driver;

static int  g_fail = 0;
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

static std::filesystem::path write_tmp(std::string const& name, std::string const& xml) {
    auto          p = ::comdare::test::user_tmp_dir() / name;
    std::ofstream o{p, std::ios::binary};
    o << xml;
    o.close();
    return p;
}

int main() {
    std::cout << "==== A1 / MAJOR-MESS-05: parse_load_profile op_mix-Pflicht ====\n";

    // (1) Vollständiges Profil MIT op_mix → akzeptiert, Anteile exakt aus dem XML (kein Default).
    std::string const ok_xml =
        "<comdare_load_profile id=\"lp_ok\" paper_ref=\"P00\">"
        "  <workload>"
        "    <records>1000</records><num_operations>2000</num_operations>"
        "    <op_mix insert=\"0.0\" lookup=\"1.0\" erase=\"0.0\" clear=\"0.0\" scan=\"0.0\" rmw=\"0.0\"/>"
        "    <key_distribution>uniform</key_distribution>"
        "  </workload>"
        "</comdare_load_profile>";
    auto const ok = wd::parse_load_profile(write_tmp("a1_lp_ok.xml", ok_xml));
    tr("MIT op_mix → has_value()", ok.has_value());
    if (ok) {
        tr("op_mix exakt aus XML: pct_lookup == 1.0", ok->config.pct_lookup == 1.0);
        tr("op_mix exakt aus XML: pct_clear == 0.0 (KEIN 1%-Default!)", ok->config.pct_clear == 0.0);
        tr("op_mix exakt aus XML: pct_insert == 0.0", ok->config.pct_insert == 0.0);
        tr("id == lp_ok", ok->id == "lp_ok");
    }

    // (2) FEHLENDES <op_mix> → nullopt (sonst still 50/40/9/1-Default inkl. 1% Clear = falsches Profil).
    std::string const no_opmix = "<comdare_load_profile id=\"lp_no_opmix\" paper_ref=\"P00\">"
                                 "  <workload>"
                                 "    <records>1000</records><num_operations>2000</num_operations>"
                                 "    <key_distribution>uniform</key_distribution>"
                                 "  </workload>"
                                 "</comdare_load_profile>";
    auto const        r2       = wd::parse_load_profile(write_tmp("a1_lp_no_opmix.xml", no_opmix));
    tr("OHNE op_mix → nullopt (HART abgelehnt, kein stiller Default)", !r2.has_value());

    // (3) LEERES op_mix (alle Anteile fehlen/0) → nullopt (Summe 0 = kein gültiges Profil).
    std::string const empty_opmix = "<comdare_load_profile id=\"lp_empty_opmix\" paper_ref=\"P00\">"
                                    "  <workload>"
                                    "    <records>1000</records><num_operations>2000</num_operations>"
                                    "    <op_mix/>"
                                    "    <key_distribution>uniform</key_distribution>"
                                    "  </workload>"
                                    "</comdare_load_profile>";
    auto const        r3          = wd::parse_load_profile(write_tmp("a1_lp_empty_opmix.xml", empty_opmix));
    tr("LEERES op_mix (Summe 0) → nullopt", !r3.has_value());

    std::cout << "\n==== A1 / MAJOR-MESS-05: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
