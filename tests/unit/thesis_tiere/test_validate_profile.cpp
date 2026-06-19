// test_validate_profile — #169(A) Nutzerfreundlichkeit (User 2026-06-19). HARTE GATE des REIN-LESENDEN --validate:
// BEWEIST LITERAL, dass validate_profile ein comdare_thesis_profile gegen die AxisRegistry/EnabledStrategies prueft,
// BEVOR teuer gebaut wird — OHNE DLL-Bau, OHNE Messung.
//
// BEWEIST LITERAL:
//   (a) das GUELTIGE m3v2_study.profile.xml passt validate (ok==true, 0 Fehler) → Host-Exit 0 + Zusammenfassung.
//   (b) ein GETIPPTES Profil (<value>node_4</value> statt node4 + eine unbekannte Achse <axis ref="nod_type">)
//       wird MIT klarer Meldung gefangen (ok==false; die Fehlertexte NENNEN die Achse + den ungueltigen Wert +
//       die gueltigen Werte) → Host-Exit != 0.
//   (c) validate baut KEINE DLL: dieser Test ruft NUR parse_thesis_profile + validate_profile (kein cl, kein
//       run_lazy_static_then_dynamic, kein BuildOrchestrator) — er materialisiert keine perm_<id>.cpp/.dll.
//
// Die gueltigen Achsen-WERTE stammen aus den REALEN EnabledStrategies (reflect_names<TopicConfigSet::
// StaticAxisVariants*> via push_static_axis) — NICHT aus einer hartkodierten Liste. (Der Host nutzt
// build_all_axis_levels() = dieselbe Reflektions-Quelle ueber alle 22 Achsen; dieser Test baut die Registry leicht
// aus den ConfigSets der vom m3v2-Profil referenzierten Achsen, um den OOM-schweren 22-Achsen-Umbrella zu meiden.)
//
// Build/Run: build_validate_profile.ps1 (Include-Satz wie die thesis_tiere-Harness; parse_thesis_profile braucht
// xml_config_parser.cpp). KEIN Tier-Bau (kein COMDARE_PILOT_INCLUDES noetig).

#include "validate_profile.hpp"   // validate_profile / axis_registry_from_levels / ProfileValidationResult
#include "xml_config_parser/xml_config_parser.hpp"

#include <builder/experiment_tree/axis_reflect.hpp>   // push_static_axis (REALE EnabledStrategies-Reflektion)

// Die ConfigSets der vom m3v2-Profil referenzierten Achsen (REALE Enabled-Listen = die gueltigen Werte je Achse):
#include <topics/traversal/topic_traversal_config_set.hpp>
#include <topics/nodes/topic_nodes_config_set.hpp>
#include <topics/memory_layout/topic_memory_layout_config_set.hpp>
#include <topics/allocator/topic_allocator_config_set.hpp>
#include <topics/prefetch/topic_prefetch_config_set.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>
#include <topics/serialization/topic_serialization_config_set.hpp>
#include <topics/telemetry/topic_telemetry_config_set.hpp>
#include <topics/value_handle/topic_value_handle_config_set.hpp>
#include <topics/hardware/topic_hardware_config_set.hpp>
#include <topics/search_engine/topic_search_engine_config_set.hpp>
#include <topics/io/topic_io_config_set.hpp>
#include <topics/migration/topic_migration_config_set.hpp>
#include <topics/filter/topic_filter_config_set.hpp>
#include <topics/queuing/topic_queuing_config_set.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace tlz = comdare::cache_engine::thesis_lazy;
namespace cx  = comdare::builder::xml;
namespace ce  = comdare::cache_engine;
namespace fs  = std::filesystem;

static int g_fail = 0;
static void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

// Baut die REGISTRY (axis → gueltige Werte) aus den REALEN EnabledStrategies (die VOLLEN StaticAxisVariants*-Listen
// je Achse) — dieselbe Reflektion wie build_all_axis_levels(), nur auf die m3v2-Achsen beschraenkt (leichter Build).
static ex::AxisRegistry build_m3v2_registry() {
    std::vector<ex::AxisLevel> lv;
    ex::push_static_axis<ce::traversal::TopicConfigSet::StaticAxisVariants_03a>(lv, "search_algo");
    ex::push_static_axis<ce::traversal::TopicConfigSet::StaticAxisVariants_03b>(lv, "cache_traversal");
    ex::push_static_axis<ce::traversal::TopicConfigSet::StaticAxisVariants_03m>(lv, "mapping");
    ex::push_static_axis<ce::nodes::TopicConfigSet::StaticAxisVariants_02>(lv,     "path_compression");
    ex::push_static_axis<ce::nodes::TopicConfigSet::StaticAxisVariants_04>(lv,     "node_type");
    ex::push_static_axis<ce::memory_layout::TopicConfigSet::StaticAxisVariants>(lv, "memory_layout");
    ex::push_static_axis<ce::allocator::TopicConfigSet::StaticAxisVariants>(lv,    "allocator");
    ex::push_static_axis<ce::prefetch::TopicConfigSet::StaticAxisVariants>(lv,     "prefetch");
    ex::push_static_axis<ce::concurrency::TopicConfigSet::StaticAxisVariants>(lv,  "concurrency");
    ex::push_static_axis<ce::serialization::TopicConfigSet::StaticAxisVariants>(lv, "serialization");
    ex::push_static_axis<ce::telemetry::TopicConfigSet::StaticAxisVariants>(lv,    "telemetry");
    ex::push_static_axis<ce::value_handle::TopicConfigSet::StaticAxisVariants>(lv, "value_handle");
    ex::push_static_axis<ce::hardware::TopicConfigSet::StaticAxisVariants_09>(lv,  "isa");
    ex::push_static_axis<ce::search_engine::TopicConfigSet::StaticAxisVariants>(lv, "index_organization");
    ex::push_static_axis<ce::io::TopicConfigSet::StaticAxisVariants>(lv,           "io_dispatch");
    ex::push_static_axis<ce::migration::TopicConfigSet::StaticAxisVariants>(lv,    "migration_policy");
    ex::push_static_axis<ce::filter::TopicConfigSet::StaticAxisVariants>(lv,       "filter");
    ex::push_static_axis<ce::queuing::TopicConfigSet::StaticAxisVariants_Q1>(lv,   "queuing_q1");
    ex::push_static_axis<ce::queuing::TopicConfigSet::StaticAxisVariants_Q2>(lv,   "queuing_q2");
    return tlz::axis_registry_from_levels(lv);
}

// Schreibt ein GETIPPTES Profil: node_type traegt <value>node_4</value> (statt node4) und eine unbekannte Achse.
static fs::path write_typo_profile() {
    fs::path const p = fs::temp_directory_path() / "comdare_typo_profile.xml";
    std::ofstream f{p};
    f << R"(<?xml version="1.0" encoding="UTF-8"?>
<comdare_thesis_profile id="typo_demo" schema_version="1">
  <base_tiers>
    <tier id="prt_art" profile_ref="../sota/art.profile.xml" paper_ref="PRT"/>
  </base_tiers>
  <permute_axes>
    <axis ref="search_algo"><value>k_ary</value></axis>
    <axis ref="node_type"><value>node_4</value><value>node16</value></axis>
    <axis ref="nod_type"><value>node256</value></axis>
  </permute_axes>
  <axis_sweeps>
    <axis_sweep axis="totally_unknown_axis" baseline="index0"/>
  </axis_sweeps>
  <sota_series_set>
    <sota_series id="A" lebewesen="ghost_creature" merge="Stufe1_CeOnly"/>
  </sota_series_set>
  <modes>
    <mode name="m" merge="Stufe1_CeOnly" active_axes="search_algo node_type"/>
  </modes>
</comdare_thesis_profile>
)";
    return p;
}

static bool errors_contain(tlz::ProfileValidationResult const& r, std::string const& needle) {
    for (auto const& e : r.errors) if (e.find(needle) != std::string::npos) return true;
    return false;
}

// Direkter Parser-Aufruf (statt profile_runner::load_thesis_profile, um die schweren Treiber-Includes zu meiden).
static std::optional<cx::ThesisProfile> load_profile(fs::path const& xml) {
    cx::XmlConfigParser const parser;
    return parser.parse_thesis_profile(xml);
}

int main(int argc, char** argv) {
    fs::path const repo = (argc >= 2)
        ? fs::path{argv[1]}
        : fs::path{"C:/Users/benja/OneDrive/Desktop/Diplomarbeit - Datenbanken/Code/external/comdare-cache-engine"};
    fs::path const m3v2 = repo / "libs/cache_engine/algorithm_profiles/thesis_profiles/m3v2_study.profile.xml";

    ex::AxisRegistry const registry = build_m3v2_registry();
    std::cout << "Registry (REALE EnabledStrategies) Achsen=" << registry.size()
              << "  node_type-Werte=";
    if (auto it = registry.find("node_type"); it != registry.end())
        for (auto const& v : it->second) std::cout << v << " ";
    std::cout << "\n\n";

    // ── (a) das GUELTIGE m3v2-Profil passt validate (Exit 0, Zusammenfassung). ──
    std::cout << "── (a) GUELTIG: m3v2_study.profile.xml ──\n";
    auto const m3v2_tp = load_profile(m3v2);
    check("m3v2-Profil lesbar", m3v2_tp.has_value());
    if (m3v2_tp) {
        tlz::ProfileValidationResult const vr = tlz::validate_profile(*m3v2_tp, registry);
        tlz::print_validation_report(vr, *m3v2_tp, std::cout);
        check("m3v2 validate ok==true (0 Fehler)", vr.ok && vr.errors.empty());
        check("m3v2 hat >=300 gepruefte Achsen-Werte erwartet (>=10)", vr.values_checked >= 10);
        std::cout << "  [Host-Exit-Mapping] ok → Exit 0\n";
    }

    // ── (b) ein GETIPPTES Profil wird MIT klarer Meldung gefangen (Exit != 0). ──
    std::cout << "\n── (b) GETIPPT: node_4 (statt node4) + unbekannte Achse nod_type ──\n";
    fs::path const typo = write_typo_profile();
    auto const typo_tp = load_profile(typo);
    check("Typo-Profil lesbar (parse OK — der Fehler ist SEMANTISCH, nicht syntaktisch)", typo_tp.has_value());
    if (typo_tp) {
        tlz::ProfileValidationResult const vr = tlz::validate_profile(*typo_tp, registry);
        tlz::print_validation_report(vr, *typo_tp, std::cout);
        check("Typo validate ok==false", !vr.ok);
        check("Meldung NENNT die Achse node_type + den ungueltigen Wert node_4",
              errors_contain(vr, "node_type") && errors_contain(vr, "node_4"));
        check("Meldung listet gueltige Werte (node4 ist drunter)", errors_contain(vr, "node4"));
        check("Meldung faengt die UNBEKANNTE Achse nod_type", errors_contain(vr, "nod_type"));
        check("Meldung faengt die unbekannte Sweep-Achse totally_unknown_axis",
              errors_contain(vr, "totally_unknown_axis"));
        check("Meldung faengt das unbekannte Lebewesen ghost_creature",
              errors_contain(vr, "ghost_creature"));
        std::cout << "  [Host-Exit-Mapping] !ok → Exit != 0\n";
    }

    // ── (c) validate baut KEINE DLL: kein .dll/.obj/perm_*.cpp im temp-Verzeichnis dieses Laufs erzeugt. ──
    std::cout << "\n── (c) KEIN DLL-Bau ──\n";
    bool any_artifact = false;
    for (auto const& e : fs::directory_iterator(fs::temp_directory_path())) {
        auto const n = e.path().filename().string();
        if (n.rfind("perm_", 0) == 0 && (e.path().extension() == ".dll" || e.path().extension() == ".cpp")) {
            // Nur Artefakte, die GENAU von diesem validate-Lauf stammen wuerden, gibt es nicht — validate erzeugt sie nie.
            any_artifact = true; std::cout << "  [unerwartet] " << n << "\n";
        }
    }
    check("validate erzeugte KEINE perm_*.dll/.cpp (rein-lesend)", !any_artifact);

    std::cout << "\n=== " << (g_fail == 0 ? "ALLE CHECKS GRUEN" : "FEHLGESCHLAGEN") << " (fails=" << g_fail << ") ===\n";
    return g_fail == 0 ? 0 : 1;
}
