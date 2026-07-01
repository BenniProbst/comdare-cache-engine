// #230-Rest (W1, 2026-07-02) - Compile+Smoke-Gate fuer die Host-Treiber-Schicht des thesis_tiere-Harness.
//
// Warum: Die contract-Stage compiliert ABI-Adapter, Pool-Organe und das Konformitaets-Gate, aber bisher nicht den
// schweren Host-Treiber-Stack um cache_engine_builder_iterator.hpp / perm_runner.hpp. Genau dort laufen Registry ->
// Iterator -> Runner -> CSV/Resume zusammen; die kommenden Node-Shape-Achsen muessen ab Entstehung diese Header
// mitcompilieren, statt erst im lokalen Windows-Harness aufzufallen.
//
// Bewusst NICHT: kein run_lazy_static_then_dynamic, kein DLL-/AnatomyModuleLoader-Lauf, keine echte Messung, kein
// CSV-/Resume-Dateizugriff und keine Threads. Dieser Test ist nur ein schneller, deterministischer Compile+Smoke
// der Treiber-Header und ihrer side-effect-freien Datenpfade.

#include <builder/experiment_tree/axis_reflect.hpp>
#include <builder/experiment_tree/cache_engine_builder_iterator.hpp>
#include <builder/experiment_tree/experiment_tree.hpp>
#include <builder/experiment_tree/perm_runner.hpp>
#include <builder/experiment_tree/result_ingest.hpp>
#include <builder/pmc_source_factory.hpp>
#include <builder/workload_driver/workload_orchestrator.hpp>

#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace ce = ::comdare::cache_engine;

namespace {

using AxNodeType        = ce::nodes::TopicConfigSet::StaticAxisVariants_04;
using AxPathCompression = ce::nodes::TopicConfigSet::StaticAxisVariants_02;
using AxMemoryLayout    = ce::memory_layout::TopicConfigSet::StaticAxisVariants;

int g_fail = 0;

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << '\n';
}

void check(char const* what, bool ok) {
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << '\n';
    if (!ok) ++g_fail;
}

void check_contains(std::string const& haystack, char const* needle, char const* what) {
    check(what, haystack.find(needle) != std::string::npos);
}

std::vector<ex::AxisLevel> build_registry_subset_levels() {
    std::vector<ex::AxisLevel> levels;
    ex::push_static_axis<AxNodeType>(levels, "node_type");
    ex::push_static_axis<AxPathCompression>(levels, "path_compression");
    ex::push_static_axis<AxMemoryLayout>(levels, "memory_layout");
    levels.push_back(ex::AxisLevel{"workload", {"YCSB_C_read_only", "YCSB_A_update_heavy"}, false, "workload_id",
                                   "workload"});
    return levels;
}

void smoke_tree_and_view() {
    std::cout << "== Registry -> AxisLevel -> ExperimentTree/StaticBinaryView ==\n";

    std::vector<ex::AxisLevel> levels = build_registry_subset_levels();
    check_eq("AxisLevels gesamt (3 statisch registry-getrieben + 1 dynamisch)", levels.size(), std::size_t{4});
    check("node_type-Achse hat reale Wrapper", !levels[0].values.empty());
    check("path_compression-Achse hat reale Wrapper", !levels[1].values.empty());
    check("memory_layout-Achse hat reale Wrapper", !levels[2].values.empty());
    check("node_type enthaelt realen Wrapper node4",
          std::find(levels[0].values.begin(), levels[0].values.end(), "node4") != levels[0].values.end());

    std::size_t const expected_static = ex::enabled_count<AxNodeType> * ex::enabled_count<AxPathCompression> *
                                        ex::enabled_count<AxMemoryLayout>;

    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};
    tree.build(levels);

    check_eq("binary_count == registry-getriebenes Produkt", tree.binary_count(), expected_static);
    check_eq("experiment_setting_count == static * dynamische Workloads", tree.experiment_setting_count(),
             expected_static * std::size_t{2});

    ex::StaticBinaryView view = tree.static_binary_view();
    check_eq("StaticBinaryView.size == binary_count", view.size(), expected_static);

    std::size_t visited = 0;
    bool        ids_ok  = true;
    for (auto it = view.begin(); it != view.end() && visited < 3; ++it, ++visited) {
        ex::BinarySpec spec = *it;
        ids_ok             = ids_ok && !spec.binary_id.empty() && spec.axes.size() == 3 &&
                 spec.binary_id.find("node_type=") != std::string::npos &&
                 spec.binary_id.find("path_compression=") != std::string::npos &&
                 spec.binary_id.find("memory_layout=") != std::string::npos;
        std::cout << "    view[" << spec.index << "] = " << spec.binary_id << '\n';
    }
    check_eq("wenige BinarySpecs lazy iteriert", visited, std::min<std::size_t>(3, expected_static));
    check("BinarySpec-IDs tragen die Registry-Achsen", ids_ok);

    std::size_t nodes_seen   = 0;
    std::size_t dynamic_seen = 0;
    bool        block_ok     = true;
    tree.for_each_node([&](ex::INodeDescription const& d) {
        ++nodes_seen;
        if (d.kind() == ex::NodeKind::Dynamic) ++dynamic_seen;
        block_ok = block_ok && !d.block_id().empty() && !d.axis().empty() && !d.value().empty();
    });
    check_eq("Knoten-Repraesentanten (3 statisch + 1 dynamisch)", nodes_seen, std::size_t{4});
    check_eq("dynamischer Workload-Knoten sichtbar", dynamic_seen, std::size_t{1});
    check("Knoten tragen block_id/axis/value", block_ok);
}

void smoke_runner_and_csv() {
    std::cout << "== perm_runner / LazyMeasuredRow / CSV ==\n";

    ex::LazyRunConfig const cfg;
    ex::LazyRunResult const empty_run;
    check("LazyRunConfig Default bleibt side-effect-frei instanziierbar", cfg.n_ops > 0 && cfg.max_binaries > 0);
    check_eq("LazyRunResult Default selected", empty_run.selected, std::size_t{0});

    std::string const header = ex::lazy_csv_header();
    check("lazy_csv_header nicht leer", !header.empty());
    check_contains(header, "seg_node_type_ns", "Header enthaelt seg_*-Spalte");
    check_contains(header, "stat_search_algo_lookup", "Header enthaelt stat_*-Spalte");
    check_contains(header, "stat_node_type_find", "Header enthaelt Node-Shape-stat-Spalte");
    check_contains(header, "pmc_available", "Header enthaelt PMC-Endspalte");

    ex::PermResult pr;
    pr.line      = ex::format_perm_result("node_type=smoke", pr.unified);
    pr.total_ns  = 12345;
    pr.n_ops     = 64;
    pr.timed_ops = 128;
    check("format_perm_result(Default-POD) erzeugt Wire-Zeile", !pr.line.empty());
    check("ungueltige Wire-ID wird hart abgelehnt", ex::format_perm_result("bad;id", pr.unified).empty());
    check("binary_id_is_wire_safe akzeptiert einfache ID", ex::binary_id_is_wire_safe("node_type=smoke"));
    check("binary_id_is_wire_safe lehnt Semikolon ab", !ex::binary_id_is_wire_safe("bad;id"));

    ex::LazyMeasuredRow row;
    row.binary_id          = "node_type=smoke/path_compression=smoke/memory_layout=smoke";
    row.setting_label      = "workload.workload_id=YCSB_C_read_only/repetition.repetition_index=0";
    row.setting_id         = row.binary_id + "/" + row.setting_label;
    row.applied_axis_count = 3;
    row.total_ns           = pr.total_ns;
    row.n_ops              = pr.n_ops;
    row.timed_ops          = pr.timed_ops;
    row.op_lat             = pr.op_lat;
    row.unified            = pr.unified;
    row.unified_real       = pr.unified_real;
    row.profile_name       = "YCSB_C_read_only";
    row.two_phase_valid    = pr.two_phase_valid;
    row.series             = "W1-harness";
    row.sweep_axis         = "node_type";
    row.working_set_n      = 64;
    row.platform           = "ci-linux";
    row.build_version      = "compile-smoke";
    row.pruefling_type     = "abstract";
    row.pmc                = pr.pmc;

    std::string const csv = ex::format_csv_row(row);
    check("format_csv_row(Default-Werte) erzeugt Zeile", !csv.empty());
    check_contains(csv, "YCSB_C_read_only", "CSV-Zeile traegt Workload-Tag");
    check_contains(csv, "W1-harness", "CSV-Zeile traegt Serien-Tag");
    check_contains(csv, "node_type", "CSV-Zeile traegt Sweep-Achse");
    check("Default-Zeile bleibt einzeilig mit Newline am Ende",
          !csv.empty() && csv.back() == '\n' && csv.find('\n') == csv.size() - 1);
}

} // namespace

int main() {
    std::cout << "==== #230-Rest Harness Compile+Smoke Gate ====\n";
    smoke_tree_and_view();
    smoke_runner_and_csv();
    std::cout << (g_fail == 0 ? "SMOKE_OK\n" : "SMOKE_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
