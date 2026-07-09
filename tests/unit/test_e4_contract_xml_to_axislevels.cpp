// test_e4_contract_xml_to_axislevels - E4->E3-Vertrag: RC-Felder aus XML bleiben bis AxisLevels verlustfrei.

#include "builder/experiment_tree/profile_to_tree.hpp"
#include "xml_config_parser/xml_config_parser.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;
namespace cx = comdare::builder::xml;
namespace fs = std::filesystem;

static int g_fail = 0;

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

void check_true(char const* what, bool cond) {
    std::cout << (cond ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!cond) ++g_fail;
}

void check_vec(char const* what, std::vector<std::string> const& got, std::vector<std::string> const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " size=" << got.size();
    if (!ok) {
        std::cout << "  (erwartet size=" << want.size() << ")";
        ++g_fail;
    }
    std::cout << "\n";
}

void check_axis_level(char const* what, ex::AxisLevel const& level, std::string const& axis,
                      std::vector<std::string> const& values, bool is_static, std::string const& variable,
                      std::string const& block_id) {
    check_eq((std::string{what} + ".axis").c_str(), level.axis, axis);
    check_vec((std::string{what} + ".values").c_str(), level.values, values);
    check_eq((std::string{what} + ".is_static").c_str(), level.is_static, is_static);
    check_eq((std::string{what} + ".variable").c_str(), level.variable, variable);
    check_eq((std::string{what} + ".block_id").c_str(), level.block_id, block_id);
}

void check_dynamic(char const* what, ex::DynamicDim const& dim, std::string const& axis, std::string const& variable,
                   std::vector<std::string> const& values, std::string const& block_id) {
    check_eq((std::string{what} + ".axis").c_str(), dim.axis, axis);
    check_eq((std::string{what} + ".variable").c_str(), dim.variable, variable);
    check_vec((std::string{what} + ".values").c_str(), dim.values, values);
    check_eq((std::string{what} + ".block_id").c_str(), dim.block_id, block_id);
}

std::vector<ex::DynamicDim> dynamic_dims(std::vector<ex::AxisLevel> const& levels) {
    ex::ExperimentTree tree{std::make_shared<ex::ExperimentNodeFactory>()};
    tree.build(levels);
    return tree.dynamic_filter();
}

fs::path write_negative_fixture() {
    fs::path const p = fs::temp_directory_path() /
                       ("comdare_e4_contract_negative_" +
                        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    std::ofstream  out{p};
    out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="e4_contract_negative" schema_version="1">
  <base_tiers>
    <tier id="tier_a" profile_ref="../sota/art.profile.xml" paper_ref="P01"/>
    <tier id="tier_b" profile_ref="../sota/hot.profile.xml" paper_ref="P02"/>
  </base_tiers>
  <permute_axes>
    <axis ref="search_algo">
      <value>art</value>
      <value>hot</value>
    </axis>
    <axis ref="cache_traversal">
      <value>linear</value>
    </axis>
    <axis ref="value_handle">
      <value>inline</value>
      <value>external</value>
    </axis>
  </permute_axes>
  <compile_dims>
    <workloads>YCSB_A</workloads>
    <telemetry mode="off" silent="true"/>
  </compile_dims>
  <runtime_dynamic>
    <thread_count>1 4</thread_count>
    <hw_prefetcher>all_on all_off</hw_prefetcher>
  </runtime_dynamic>
  <repetitions count="2" interpolate="false" overlay_in_chart="true"/>
  <modes>
    <mode name="ce_only" merge="Stufe1_CeOnly" active_axes="search_algo cache_traversal value_handle"/>
  </modes>
  <static_axes from="base_tier"/>
</comdare_thesis_profile>
)";
    return p;
}

int main(int argc, char** argv) {
    fs::path const positive_xml =
        (argc > 1) ? fs::path{argv[1]} : fs::path{"tests/unit/fixtures/e4_contract_profile.xml"};

    cx::XmlConfigParser parser;
    auto const          tp = parser.parse_thesis_profile(positive_xml);
    check_true("Positiv-Fixture parse_thesis_profile", tp.has_value());
    if (!tp) return 1;

    check_vec("Parser: thread_counts", tp->thread_counts, {"1", "4"});
    check_vec("Parser: hw_prefetcher", tp->hw_prefetcher, {"all_on", "all_off"});
    check_vec("Parser: prefetch_distances", tp->prefetch_distances, {"0", "16", "64"});
    check_vec("Parser: pool_budgets_bytes", tp->pool_budgets_bytes, {"0", "4096"});
    check_vec("Parser: batch_sizes", tp->batch_sizes, {"1", "32"});
    check_vec("Parser: inline_thresholds_bytes", tp->inline_thresholds_bytes, {"0", "128"});

    std::vector<ex::AxisLevel> const levels = ex::build_axis_levels(*tp, "ce_only", ex::AxisRegistry{});
    check_eq("Positiv: AxisLevel-Anzahl", levels.size(), std::size_t{11});
    if (levels.size() == 11) {
        check_axis_level("static[0]", levels[0], "tier", {"tier_a", "tier_b"}, true, "", "");
        check_axis_level("static[1]", levels[1], "search_algo", {"art", "hot"}, true, "", "");
        check_axis_level("static[2]", levels[2], "cache_traversal", {"linear"}, true, "", "");
        check_axis_level("static[3]", levels[3], "value_handle", {"inline", "external"}, true, "", "");
        check_axis_level("dyn-level[4]", levels[4], "concurrency", {"1", "4"}, false, "thread_count", "concurrency");
        check_axis_level("dyn-level[5]", levels[5], "prefetch", {"all_on", "all_off"}, false, "hw_prefetcher",
                         "prefetch");
        check_axis_level("dyn-level[6]", levels[6], "prefetch", {"0", "16", "64"}, false, "prefetch_distance",
                         "prefetch");
        check_axis_level("dyn-level[7]", levels[7], "allocator", {"0", "4096"}, false, "pool_budget_bytes",
                         "allocator");
        check_axis_level("dyn-level[8]", levels[8], "cache_traversal", {"1", "32"}, false, "batch_size",
                         "cache_traversal");
        check_axis_level("dyn-level[9]", levels[9], "value_handle", {"0", "128"}, false, "inline_threshold_bytes",
                         "value_handle");
        check_axis_level("dyn-level[10]", levels[10], "repetition", {"0", "1"}, false, "repetition_index",
                         "repetition");
    }

    std::vector<ex::DynamicDim> const dyn = dynamic_dims(levels);
    check_eq("Positiv: dynamic_filter-Anzahl", dyn.size(), std::size_t{7});
    if (dyn.size() == 7) {
        check_dynamic("dyn[0]", dyn[0], "concurrency", "thread_count", {"1", "4"}, "concurrency");
        check_dynamic("dyn[1]", dyn[1], "prefetch", "hw_prefetcher", {"all_on", "all_off"}, "prefetch");
        check_dynamic("dyn[2]", dyn[2], "prefetch", "prefetch_distance", {"0", "16", "64"}, "prefetch");
        check_dynamic("dyn[3]", dyn[3], "allocator", "pool_budget_bytes", {"0", "4096"}, "allocator");
        check_dynamic("dyn[4]", dyn[4], "cache_traversal", "batch_size", {"1", "32"}, "cache_traversal");
        check_dynamic("dyn[5]", dyn[5], "value_handle", "inline_threshold_bytes", {"0", "128"}, "value_handle");
        check_dynamic("dyn[6]", dyn[6], "repetition", "repetition_index", {"0", "1"}, "repetition");
    }

    fs::path const  negative_xml = write_negative_fixture();
    auto const      neg          = parser.parse_thesis_profile(negative_xml);
    std::error_code ec;
    fs::remove(negative_xml, ec);
    check_true("Negativ-Fixture parse_thesis_profile", neg.has_value());
    if (!neg) return 1;

    check_true("Negativ: prefetch_distances leer", neg->prefetch_distances.empty());
    check_true("Negativ: pool_budgets_bytes leer", neg->pool_budgets_bytes.empty());
    check_true("Negativ: batch_sizes leer", neg->batch_sizes.empty());
    check_true("Negativ: inline_thresholds_bytes leer", neg->inline_thresholds_bytes.empty());

    std::vector<ex::AxisLevel> const neg_levels = ex::build_axis_levels(*neg, "ce_only", ex::AxisRegistry{});
    check_eq("Negativ: AxisLevel-Anzahl wie Bestand", neg_levels.size(), std::size_t{7});
    if (neg_levels.size() == 7) {
        check_axis_level("neg.static[0]", neg_levels[0], "tier", {"tier_a", "tier_b"}, true, "", "");
        check_axis_level("neg.static[1]", neg_levels[1], "search_algo", {"art", "hot"}, true, "", "");
        check_axis_level("neg.static[2]", neg_levels[2], "cache_traversal", {"linear"}, true, "", "");
        check_axis_level("neg.static[3]", neg_levels[3], "value_handle", {"inline", "external"}, true, "", "");
    }

    std::vector<ex::DynamicDim> const neg_dyn = dynamic_dims(neg_levels);
    check_eq("Negativ: dynamic_filter-Anzahl wie Bestand", neg_dyn.size(), std::size_t{3});
    if (neg_dyn.size() == 3) {
        check_dynamic("neg.dyn[0]", neg_dyn[0], "concurrency", "thread_count", {"1", "4"}, "concurrency");
        check_dynamic("neg.dyn[1]", neg_dyn[1], "prefetch", "hw_prefetcher", {"all_on", "all_off"}, "prefetch");
        check_dynamic("neg.dyn[2]", neg_dyn[2], "repetition", "repetition_index", {"0", "1"}, "repetition");
    }

    // Validierungs-Fall: nicht-numerisches Token in einem RC-Feld => Profil ist UNLESBAR (nullopt),
    // damit parse_u64 spaeter nie still auf 0 mappt (SCHEMA verspricht uint64-Listen).
    fs::path const invalid_xml = fs::temp_directory_path() /
                                 ("comdare_e4_contract_invalid_" +
                                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".xml");
    {
        std::ofstream out{invalid_xml};
        out << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="e4_contract_invalid" schema_version="1">
  <base_tiers><tier id="tier_a" profile_ref="../sota/art.profile.xml" paper_ref="P01"/></base_tiers>
  <permute_axes><axis ref="search_algo"><value>art</value></axis></permute_axes>
  <runtime_dynamic>
    <batch_size>1 abc</batch_size>
  </runtime_dynamic>
  <modes><mode name="ce_only" merge="Stufe1_CeOnly" active_axes="search_algo"/></modes>
</comdare_thesis_profile>
)";
    }
    auto const invalid = parser.parse_thesis_profile(invalid_xml);
    fs::remove(invalid_xml, ec);
    check_true("Invalid: nicht-numerisches RC-Token => nullopt", !invalid.has_value());

    std::cout << "\n==== E4 Contract XML->AxisLevels: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
