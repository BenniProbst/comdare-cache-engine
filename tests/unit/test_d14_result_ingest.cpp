// D14 / L-CLUSTER (gate-frei) — result_ingest: Cluster-/perm_runner-Mess-Zeilen → Baum-NodeValue (sparse).
// Der host-seitige Ergebnis-Rückführungs-Pfad (Doc 28 §5), LOKAL verifizierbar (kein Cluster-Gate). Build: cl /I libs/cache_engine.
//
// KONSOLIDIERUNG (I-B.3): die Wire-Zeile = binary_id + axis_stats[17][8] (136) + seg_ns[17] (17) + 4 Meta
// + P-MD3 (seg_framework_ns, seg_run_total_ns) = 160 Felder. Die Zeilen werden hier über format_perm_result
// (perm_runner) erzeugt → 1:1 das reale Wire-Format. Audit A1 / MAJOR-MESS-09: Negativ-Tests für EXAKTE
// Feldzahl (==169) + binary_id-Hygiene.

#include "builder/experiment_tree/result_ingest.hpp"
#include <harness/perm_runner.hpp>

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace ex  = comdare::cache_engine::builder::experiment;
namespace ana = comdare::cache_engine::anatomy;

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << " (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Baut eine valide 169-Feld-Wire-Zeile für binary_id mit gesetzten T0-search-Feldern (über das reale Format).
static std::string make_line(std::string const& bid, std::uint64_t lookups, std::uint64_t hits, std::uint64_t inserts,
                             std::uint64_t fill) {
    ana::ComdareTierObserverSnapshot s{};
    s.axis_stats[0][0]      = lookups;
    s.axis_stats[0][1]      = hits;
    s.axis_stats[0][3]      = inserts; // T0 search
    s.tier_fill_level       = fill;
    s.observable_axis_count = 2;
    s.filled_axis_count     = 1;
    return ex::format_perm_result(bid, s);
}

// Zählt ';'-getrennte Felder einer Zeile (= Wire-Feldzahl).
static std::size_t field_count(std::string const& line) {
    std::size_t n = 1;
    for (char c : line)
        if (c == ';') ++n;
    return n;
}

int main() {
    std::cout << "==== D14 result_ingest (Cluster-Ergebnis → Baum-NodeValue) ====\n";
    auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
    ex::ExperimentTree tree{factory};

    std::string const id1 = "search_algo=Array256/allocator=mimalloc";
    std::string const id2 = "search_algo=Patricia/allocator=jemalloc";
    std::string const l1  = make_line(id1, 100, 80, 256, 256);
    std::string const l2  = make_line(id2, 50, 40, 128, 128);
    eq("Wire-Zeile l1 hat EXAKT 160 Felder", field_count(l1), std::size_t{160});

    // 2 Ergebnis-Zeilen (volle Matrix) + 1 Kommentar + 1 Leerzeile.
    std::string const text = "# perm-results batch (Cluster/perm_runner)\n" + l1 + "\n\n" + l2 + "\n";

    std::size_t const n = ex::ingest_results(tree, text);
    eq("ingest_results == 2 eingespielte Knoten (Kommentar/Leerzeile ignoriert)", n, std::size_t{2});
    eq("measured_node_count() == 2 (sparse)", tree.measured_node_count(), std::size_t{2});

    auto const nv1 = tree.node_value(id1);
    tr("Knoten 1: observer_real && has_result", nv1.observer_real && nv1.has_result);
    eq("Knoten 1: search_insert_count == 256", nv1.observer.search_insert_count, std::uint64_t{256});
    eq("Knoten 1: search_lookup_count == 100", nv1.observer.search_lookup_count, std::uint64_t{100});
    eq("Knoten 1: observable_axis_count == 2", nv1.observer.observable_axis_count, std::uint64_t{2});
    eq("Knoten 1: tier_fill_level == 256", nv1.observer.tier_fill_level, std::uint64_t{256});

    auto const nv2 = tree.node_value(id2);
    eq("Knoten 2: search_insert_count == 128", nv2.observer.search_insert_count, std::uint64_t{128});
    tr("Knoten 2: observer_real", nv2.observer_real);

    auto const nv3 = tree.node_value("search_algo=nonexistent");
    tr("ungemessener Knoten: observer_real == false (sparse, kein Eintrag)", !nv3.observer_real);

    // Einzelzeile: Kommentar wird nicht eingespielt.
    tr("Kommentar-Zeile → ingest_result_line == false", !ex::ingest_result_line(tree, "# nur kommentar"));
    tr("zu kurze Zeile → false", !ex::ingest_result_line(tree, "id;1;2;3"));

    // ── Audit A1 / MAJOR-MESS-09: EXAKTE Feldzahl (==169) + binary_id-Hygiene ──────────────────────────
    std::cout << "---- MAJOR-MESS-09 Negativ-Tests (Feldzahl-Exaktheit + binary_id-Hygiene) ----\n";

    // (a) Genau 169 Felder → akzeptiert. (b) 168 oder 170 Felder → verworfen (kein stiller Slot-Versatz).
    tr("EXAKT 160 Felder → akzeptiert", ex::ingest_result_line(tree, l1));
    tr("168 Felder (ein Feld zu wenig) → verworfen", !ex::ingest_result_line(tree, l1.substr(0, l1.rfind(';'))));
    tr("161 Felder (ein Feld zu viel) → verworfen", !ex::ingest_result_line(tree, l1 + ";999"));

    // (c) binary_id mit eingebettetem ';' → format_perm_result lehnt ab (leere Zeile) → ingest verwirft.
    std::string const bad_semi = ex::format_perm_result("evil;id_with_semicolon", ana::ComdareTierObserverSnapshot{});
    tr("format_perm_result(binary_id mit ';') → leere Zeile (verworfen)", bad_semi.empty());
    tr("binary_id_is_wire_safe(';') == false", !ex::binary_id_is_wire_safe("a;b"));
    tr("binary_id_is_wire_safe(newline) == false", !ex::binary_id_is_wire_safe("a\nb"));
    tr("binary_id_is_wire_safe(leer) == false", !ex::binary_id_is_wire_safe(""));
    tr("binary_id_is_wire_safe(sauberer Stem) == true", ex::binary_id_is_wire_safe(id1));

    // (d) Ein manuell mit ';'-Injektion verschmolzenes binary_id-Feld ergäbe 170 Felder → verworfen
    //     (zweite, lese-seitige Verteidigung gegen Slot-Versatz selbst wenn die ID anders an die Zeile käme).
    std::string const injected = "evil;id" + l1.substr(l1.find(';')); // 1 zusätzliches ';'-Feld
    eq("injizierte Zeile hat 161 Felder", field_count(injected), std::size_t{161});
    tr("injizierte (161-Feld-)Zeile → verworfen", !ex::ingest_result_line(tree, injected));

    std::cout << "\n==== D14 result_ingest: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
