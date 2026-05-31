// V41.F.4 — Smoke-Test der ICacheEngineTools-Tools-Plugin-Facade.
//
// Verifiziert, dass die Facade die vier in-repo-Werkzeuge real vermittelt (Statistik/Codegen/Workload)
// UND die register_external_tool()-Plugin-Registry funktioniert. Damit ist F.4 nicht hohl, sondern
// über die ECHTEN Tool-Aufrufe belegt.

#include <cache_engine/api/i_cache_engine_tools.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace api = ::comdare::cache_engine::api;
namespace wg  = ::comdare::workload_generator;

// Die Facade liefert eine prozessweite Instanz mit der erwarteten Version.
TEST(F4_ToolsFacade, ProvidesFrameworkVersion) {
    auto& tools = api::get_cache_engine_tools();
    EXPECT_EQ(tools.framework_version(), std::string_view{"comdare-tools-v1"});
}

// Statistik-Werkzeug: vermittelt den ECHTEN Welch- + Mann-Whitney-Test.
TEST(F4_ToolsFacade, StatisticsToolRunsRealTests) {
    auto& stats = api::get_cache_engine_tools().statistics();
    // Zwei klar getrennte Latenz-Stichproben (a deutlich kleiner als b).
    std::vector<std::int64_t> const a{100, 102, 98, 101, 99, 103, 97};
    std::vector<std::int64_t> const b{200, 205, 198, 202, 199, 207, 196};

    auto const w = stats.welch_t_test(a, b);
    EXPECT_TRUE(w.valid);
    EXPECT_LT(w.p_value, 0.01);               // hochsignifikant verschieden
    EXPECT_LT(w.mean_a, w.mean_b);

    auto const m = stats.mann_whitney_u_test(a, b);
    EXPECT_TRUE(m.valid);
    EXPECT_TRUE(m.a_stochastically_less);     // a tendiert zu kleineren (schnelleren) Werten
    EXPECT_LT(m.p_value, 0.01);
}

// Codegen-Werkzeug: emittiert kompilierbaren Modul-.cpp-Quelltext aus FQ-Typ-Namen.
TEST(F4_ToolsFacade, CodegenToolEmitsModuleSource) {
    auto& cg = api::get_cache_engine_tools().codegen();
    std::vector<std::string> const fq{
        "ns::A0", "ns::A1", "ns::A2"};
    std::string const src = cg.emit_module_source(7, fq);

    EXPECT_NE(src.find("Permutation 7"), std::string::npos);
    EXPECT_NE(src.find("DO NOT EDIT"), std::string::npos);
    EXPECT_NE(src.find("COMDARE_DEFINE_ANATOMY_MODULE_ADHOC"), std::string::npos);
    EXPECT_NE(src.find("all_axes_umbrella.hpp"), std::string::npos);
    for (auto const& t : fq) EXPECT_NE(src.find(t), std::string::npos);
}

// Workload-Werkzeug: vermittelt den ECHTEN YCSB-Generator (Profil C = 100 % Read).
TEST(F4_ToolsFacade, WorkloadToolGeneratesYcsb) {
    auto& wt = api::get_cache_engine_tools().workload_generator();
    wg::WorkloadConfig cfg{};
    cfg.num_keys       = 1000;
    cfg.num_operations = 500;
    cfg.random_seed    = 7;

    auto const ops = wt.generate_ycsb(cfg, wg::YcsbWorkload::C);
    ASSERT_EQ(ops.size(), 500u);
    for (auto const& op : ops) EXPECT_EQ(op.op, wg::OperationKind::Read);  // YCSB-C ist read-only
}

// Plugin-Registry: register_external_tool / find_external_tool / external_tool_names.
namespace {
class FakePmcTool final : public api::IExternalTool {
public:
    [[nodiscard]] std::string_view tool_name() const override { return "pmc-counter"; }
    [[nodiscard]] std::string_view tool_kind() const override { return "measurement"; }
};
}  // namespace

TEST(F4_ToolsFacade, RegistersAndFindsExternalTool) {
    // Frische Facade-Instanz, um Test-Isolation von der prozessweiten Default-Instanz zu sichern.
    api::DefaultCacheEngineTools tools;
    EXPECT_EQ(tools.find_external_tool("pmc-counter"), nullptr);
    EXPECT_TRUE(tools.external_tool_names().empty());

    tools.register_external_tool("pmc-counter", std::make_shared<FakePmcTool>());

    auto* t = tools.find_external_tool("pmc-counter");
    ASSERT_NE(t, nullptr);
    EXPECT_EQ(t->tool_name(), std::string_view{"pmc-counter"});
    EXPECT_EQ(t->tool_kind(), std::string_view{"measurement"});
    ASSERT_EQ(tools.external_tool_names().size(), 1u);
    EXPECT_EQ(tools.external_tool_names().front(), std::string{"pmc-counter"});
    EXPECT_EQ(tools.find_external_tool("does-not-exist"), nullptr);
}
