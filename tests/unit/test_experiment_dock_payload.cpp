// test_experiment_dock_payload.cpp -- Welle 5 (§38-Dock-Protokoll, W5-C, 2026-07-19). Gate fuer TEIL 1 (§38 hinab)
// von profile_facade/planner/experiment_dock_payload.hpp: ExperimentSubtreePayload <-> R5-XML.
//
//   BYTE-ROUNDTRIP (emit->parse->emit byte-identisch) + Semantik-Roundtrip (parse(emit(p))==p) + die Negativ-Faelle
//   (unbekanntes kind, axis ohne values/range, falsche Wurzel, kaputtes XML -> nullopt).
//
// TEIL 2 (Fortschritts-Delta-Kanal) lebt nach der Layering-Verlagerung im builder-Leaf progress_delta.hpp und wird
// von test_progress_delta.cpp gegen die reale StaticBinaryView geprueft. Plain-main (check_*; exit 0 = alle OK).

#include <profile_facade/planner/experiment_dock_payload.hpp> // ExperimentSubtreePayload / *_experiment_subtree_xml

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace pl = ::comdare::cache_engine::planner;
namespace tp = ::comdare::cache_engine::topics;

namespace {

int g_fail = 0;

void check(char const* what, bool ok) {
    std::cout << "  [" << (ok ? " ok " : "FAIL") << "] " << what << '\n';
    if (!ok) ++g_fail;
}

} // namespace

int main() {
    std::cout << "==== W5-C experiment_dock_payload Gate (§38 hinab: R5-XML-Roundtrip) ====\n";

    // Eine gemischte Nutzlast: alle drei AxisKind-Faerbungen, BEIDE Formen (values / range), plus Sonderzeichen
    // (im Namen UND in den Werten), damit der xml_encode/decode-Pfad im Byte-Roundtrip mitgeprueft wird.
    pl::ExperimentSubtreePayload p;
    p.axes.push_back(
        {tp::AxisKind::organ, "search_algo", pl::AxisRangeForm::enumerated, {"art", "hot", "swiss"}, 0, 0});
    p.axes.push_back({tp::AxisKind::system_config, "opt_level", pl::AxisRangeForm::index_range, {}, 0, 5});
    p.axes.push_back(
        {tp::AxisKind::system_measurement, "numa_node", pl::AxisRangeForm::enumerated, {"n0", "n1"}, 0, 0});
    p.axes.push_back({tp::AxisKind::organ, "memory_layout", pl::AxisRangeForm::index_range, {}, 2, 8});
    // Sonderzeichen-Achse (Name + Werte tragen &, <, >): der Encode/Decode muss byte-treu round-trippen.
    p.axes.push_back({tp::AxisKind::organ, "weird&name", pl::AxisRangeForm::enumerated, {"v<1", "v>2", "a&b"}, 0, 0});

    std::string const s1 = pl::emit_experiment_subtree_xml(p);
    auto const        p2 = pl::parse_experiment_subtree_xml(s1);
    check("(1a) parse(emit(p)) liefert einen Wert", p2.has_value());
    if (p2.has_value()) {
        check("(1a) Semantik-Roundtrip: parse(emit(p)) == p", *p2 == p);
        std::string const s2 = pl::emit_experiment_subtree_xml(*p2);
        check("(1b) BYTE-Roundtrip: emit(parse(emit(p))) byte-identisch zu emit(p)", s1 == s2);
    }

    // Leere Nutzlast: Roundtrip muss ebenfalls byte-stabil sein (nur Wurzel-Tags).
    {
        pl::ExperimentSubtreePayload const empty;
        std::string const                  e1 = pl::emit_experiment_subtree_xml(empty);
        auto const                         e2 = pl::parse_experiment_subtree_xml(e1);
        check("(1c) leere Nutzlast: parse-Erfolg + Semantik-Roundtrip",
              e2.has_value() && *e2 == empty && e2->axes.empty());
        if (e2.has_value()) check("(1c) leere Nutzlast: Byte-Roundtrip", e1 == pl::emit_experiment_subtree_xml(*e2));
    }

    // Die konkrete R5-Wire-Form ist stabil (Vertrag mit dem Planer/Registry-Weg): Stichprobe an Substrings.
    check("(1d) values-Form emittiert '<values>art hot swiss</values>'",
          s1.find("<values>art hot swiss</values>") != std::string::npos);
    check("(1d) range-Form emittiert '<range start=\"0\" count=\"5\"/>'",
          s1.find("<range start=\"0\" count=\"5\"/>") != std::string::npos);
    check("(1d) kind-Faerbung system_config im Wire-Format", s1.find("kind=\"system_config\"") != std::string::npos);
    check("(1d) kind-Faerbung system_measurement im Wire-Format",
          s1.find("kind=\"system_measurement\"") != std::string::npos);

    // ── Negativ-Faelle: jede Abweichung -> nullopt (nie stille Fehlfaerbung / halbe Nutzlast). ──
    check("(1e) unbekanntes kind -> nullopt",
          !pl::parse_experiment_subtree_xml(
               "<experiment_subtree><axis kind=\"bogus\" name=\"x\"><values>a</values></axis></experiment_subtree>")
               .has_value());
    check("(1e) axis ohne values UND ohne range -> nullopt",
          !pl::parse_experiment_subtree_xml(
               "<experiment_subtree><axis kind=\"organ\" name=\"x\"></axis></experiment_subtree>")
               .has_value());
    check("(1e) falsche Wurzel -> nullopt",
          !pl::parse_experiment_subtree_xml("<other_root><axis kind=\"organ\" name=\"x\"/></other_root>").has_value());
    check("(1e) kaputtes XML -> nullopt", !pl::parse_experiment_subtree_xml("<experiment_subtree><axis ").has_value());

    std::cout << (g_fail == 0 ? "\nDOCK_PAYLOAD_OK\n" : "\nDOCK_PAYLOAD_FAIL\n");
    return g_fail == 0 ? 0 : 1;
}
