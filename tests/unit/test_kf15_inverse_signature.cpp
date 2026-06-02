// test_kf15_inverse_signature — KF-15 (2026-06-02)
// Belegt die inverse Auswertung (Doc 26 §3): multimap<gepinnte Signatur, Paper> (Doppelerkennung) +
// read-only Tree-Interface, das gemessene Blätter per Signatur-Filter auf Paper-Sichten projiziert (linear).
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf15_inverse_signature.cpp

#include "builder/experiment_tree/inverse_signature_eval.hpp"
#include "builder/experiment_tree/experiment_tree.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }
static bool has(std::vector<std::string> const& v, std::string const& s) {
    for (auto const& e : v) if (e == s) return true; return false;
}

int main() {
    std::cout << "KF-15: inverse Auswertung via Signatur-multimap + read-only Tree-Interface:\n";

    // ── Teil 1: PaperSignatureIndex — multimap mit Doppelerkennung (mehrere Paper teilen eine Signatur) ──
    {
        ex::PaperSignatureIndex idx;
        idx.add("P01", "traversal=ART");
        idx.add("P02", "traversal=ART");   // teilt Signatur mit P01 (Doppelerkennung)
        idx.add("P03", "traversal=HOT");
        idx.add("P04", "traversal=ART");   // dritter ART-Paper

        auto art = idx.papers_for("traversal=ART");
        check_eq("papers_for(ART): 3 Paper teilen die Signatur", art.size(), std::size_t{3});
        check_true("  P01,P02,P04 ∈ ART-Paper", has(art, "P01") && has(art, "P02") && has(art, "P04"));
        check_true("  P03 ∉ ART-Paper", !has(art, "P03"));
        check_eq("papers_for(HOT): 1 Paper", idx.papers_for("traversal=HOT").size(), std::size_t{1});
        check_eq("signatures_for(P01) = {ART}", idx.signatures_for("P01").size(), std::size_t{1});
        check_eq("papers_for(unbekannt) = leer", idx.papers_for("traversal=SURF").size(), std::size_t{0});
    }

    // ── Teil 2: ReadOnlyResultView — Projektion der Baum-Binaries auf eine Paper-Signatur (linear) ──
    {
        auto factory = std::make_shared<ex::ExperimentNodeFactory>();
        ex::ExperimentTree tree{factory};
        tree.build({
            ex::AxisLevel{"traversal", {"ART"}, true, ""},          // gepinnt → Paper-Signatur "traversal=ART"
            ex::AxisLevel{"node", {"N4", "N16", "N48"}, true, ""},  // freigegeben → 3 Varianten-Binaries
        });
        // binaries: ART/N4, ART/N16, ART/N48 — alle mit gepinnter Signatur "traversal=ART"

        ex::ReadOnlyResultView view{tree};
        auto sigs = view.signatures();
        check_eq("Baum: 1 distinkte gepinnte Signatur", sigs.size(), std::size_t{1});
        check_true("  Signatur == traversal=ART", !sigs.empty() && sigs[0] == "traversal=ART");

        auto bins = view.binaries_with_signature("traversal=ART");
        check_eq("Projektion: 3 Binaries unter der Signatur", bins.size(), std::size_t{3});
        check_true("  ART/N4 ∈ Projektion", has(bins, "traversal=ART/node=N4"));
        check_true("  ART/N48 ∈ Projektion", has(bins, "traversal=ART/node=N48"));
        check_eq("fremde Signatur → leere Projektion", view.binaries_with_signature("traversal=HOT").size(), std::size_t{0});

        // Paper-Projektion (kombiniert): P01 + P02 teilen ART → beide projizieren auf dieselben 3 Binaries.
        ex::PaperSignatureIndex idx;
        idx.add("P01", "traversal=ART");
        idx.add("P02", "traversal=ART");
        idx.add("P03", "traversal=HOT");
        check_eq("binaries_for_paper(P01) = 3", view.binaries_for_paper("P01", idx).size(), std::size_t{3});
        check_eq("binaries_for_paper(P02) = 3 (geteilte Signatur)", view.binaries_for_paper("P02", idx).size(), std::size_t{3});
        check_eq("binaries_for_paper(P03/HOT) = 0 (keine Binaries dieser Signatur)", view.binaries_for_paper("P03", idx).size(), std::size_t{0});

        // Aggregierte Mess-Projektion (read-only): zählt die 3 Binaries der Signatur.
        auto agg = view.aggregate_for_signature("traversal=ART");
        check_eq("aggregate: measured_setting_count == 3 Binaries", agg.measured_setting_count, std::uint64_t{3});
    }

    std::cout << "\n==== KF-15 inverse Signatur-Auswertung: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
