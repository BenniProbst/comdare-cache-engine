// test_genus_docks — L-76a-c (2026-06-03, Doc 24 §8.8): die per-Gattung Prüf-Docks für Set/Sequence/View treiben
// je ein Tier der Gattung über die Gattungs-API + messen dessen eingebauten Observer + persistieren. Schließt die
// in-process-Mess-Seite für 3 weitere Gattungen (analog AdapterDock für Adapter, SearchAlgorithmDock für
// SearchAlgorithm). Build: cl /I libs/cache_engine /std:c++latest (kein Boost). Erwartungswerte deterministisch.

#include "builder/pruef_dock/set_dock.hpp"
#include "builder/pruef_dock/sequence_dock.hpp"
#include "builder/pruef_dock/view_dock.hpp"

#include "anatomy/set_anatomy.hpp"
#include "anatomy/set_composition.hpp"
#include "anatomy/sequence_anatomy.hpp"
#include "anatomy/sequence_composition.hpp"
#include "anatomy/view_anatomy.hpp"
#include "anatomy/view_composition.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <string>

namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace cea = comdare::cache_engine::anatomy;

static int g_fail = 0;
template <class A, class B>
static void eq(char const* w, A const& g, B const& e) {
    bool ok = (g == e);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << w << " = " << g;
    if (!ok) {
        std::cout << "  (erwartet " << e << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
static void tr(char const* w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// ── Set: minimales search_algo-Kern-Organ (key_type + insert/lookup/erase/occupied_count/clear) auf std::set ──
struct TestKeySet {
    using key_type = std::uint64_t;
    std::set<std::uint64_t>                    s;
    void                                       insert(std::uint64_t k, std::uint64_t /*v*/) { s.insert(k); }
    [[nodiscard]] std::optional<std::uint64_t> lookup(std::uint64_t k) const {
        return s.count(k) ? std::optional<std::uint64_t>{k} : std::nullopt;
    }
    void                      erase(std::uint64_t k) { s.erase(k); }
    [[nodiscard]] std::size_t occupied_count() const { return s.size(); }
    void                      clear() { s.clear(); }
};

using SetComp  = cea::SetComposition<TestKeySet, int, int, int, int, int, int, int, int, int, int, int, int, int, int>;
using SeqComp  = cea::SequenceComposition<int, int, int, int, int, int, int, int, int, int>; // Default DoublingGrowth
using ViewComp = cea::ViewComposition<int, int, int, int>;                                   // Default Layout/Accessor

int main() {
    std::cout << "==== L-76 per-Gattung Prüf-Docks (Doc 24 §8.8): Set / Sequence / View ====\n";

    // ── Set-Dock: insert/contains/erase über das echte K-only-Such-Organ ──
    std::cout << "\n-- SetDock (Vogel, genus==Set) --\n";
    pd::SetDock<cea::SetAnatomy<SetComp>> set_dock;
    tr("Dock-Gattung == Set", set_dock.genus() == cea::AnatomyGenus::Set);
    auto const sr = set_dock.measure(/*n_inserts=*/1000, /*n_contains=*/1500, /*n_erases=*/400);
    eq("total_ops == 2900", sr.total_ops, std::uint64_t{2900});
    eq("insert_count == 1000", sr.observer.insert_count, std::uint64_t{1000});
    eq("contains_count == 1500", sr.observer.contains_count, std::uint64_t{1500});
    eq("contains_hit == 1000 (Keys 0..999 present)", sr.observer.contains_hit_count, std::uint64_t{1000});
    eq("contains_miss == 500 (Keys 1000..1499 absent)", sr.observer.contains_miss_count, std::uint64_t{500});
    eq("erase_count == 400", sr.observer.erase_count, std::uint64_t{400});
    eq("current_size == 600 (1000-400)", sr.observer.current_size, std::uint64_t{600});
    eq("peak_size == 1000", sr.observer.peak_size, std::uint64_t{1000});
    {
        std::string const csv = pd::SetDock<cea::SetAnatomy<SetComp>>::serialize_csv(sr);
        tr("CSV enthält 'Set,2900,1000,1500,1000,500,400,600,1000'",
           csv.find("Set,2900,1000,1500,1000,500,400,600,1000") != std::string::npos);
    }

    // ── Sequence-Dock: push_back/at über ein wachsendes Array; treibt die axis_growth-Policy real ──
    std::cout << "\n-- SequenceDock (Reptil, genus==Sequence) --\n";
    pd::SequenceDock<cea::SequenceAnatomy<SeqComp>> seq_dock;
    tr("Dock-Gattung == Sequence", seq_dock.genus() == cea::AnatomyGenus::Sequence);
    auto const qr = seq_dock.measure(/*n_pushes=*/1000, /*n_reads=*/1200);
    eq("total_ops == 2200", qr.total_ops, std::uint64_t{2200});
    eq("push_count == 1000", qr.observer.push_count, std::uint64_t{1000});
    eq("at_count == 1200", qr.observer.at_count, std::uint64_t{1200});
    eq("at_oob == 200 (Indizes 1000..1199)", qr.observer.at_oob_count, std::uint64_t{200});
    eq("current_size == 1000", qr.observer.current_size, std::uint64_t{1000});
    eq("peak_size == 1000", qr.observer.peak_size, std::uint64_t{1000});
    // axis_growth wurde REAL getrieben (DoublingGrowth: Kapazität 0→1→2→4→…→1024 ⇒ growth_events>0). Exakter Wert
    // hängt an der Default-Policy → wird ausgegeben (nicht hart fixiert, um nicht an die Policy-Mathematik zu koppeln).
    std::cout << "  [i]  growth_events (DoublingGrowth, real getrieben) = " << qr.observer.growth_events << "\n";
    tr("growth_events > 0 (axis_growth-Policy real gefeuert)", qr.observer.growth_events > 0);

    // ── View-Dock: bind externen Puffer + read über axis_layout/axis_accessor (non-owning) ──
    std::cout << "\n-- ViewDock (Pflanze, genus==View, non-owning) --\n";
    pd::ViewDock<cea::ViewAnatomy<ViewComp>> view_dock;
    tr("Dock-Gattung == View", view_dock.genus() == cea::AnatomyGenus::View);
    auto const vr = view_dock.measure(/*buffer_size=*/500, /*n_reads=*/700);
    eq("total_ops == 700", vr.total_ops, std::uint64_t{700});
    eq("read_count == 700", vr.observer.read_count, std::uint64_t{700});
    eq("read_oob == 200 (Indizes 500..699)", vr.observer.read_oob_count, std::uint64_t{200});
    eq("bound_size == 500", vr.observer.bound_size, std::uint64_t{500});
    eq("bind_count == 1", vr.observer.bind_count, std::uint64_t{1});
    {
        std::string const csv = pd::ViewDock<cea::ViewAnatomy<ViewComp>>::serialize_csv(vr);
        tr("CSV enthält 'View,700,700,200,500,1'", csv.find("View,700,700,200,500,1") != std::string::npos);
    }

    std::cout << "\n==== L-76 per-Gattung Docks: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
