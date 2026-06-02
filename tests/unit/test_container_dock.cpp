// test_container_dock — Container-Prüf-Dock (2026-06-02, Doc 24 §8.8) — die per-Gattung Mess-Seite für die
// Container-Gattung treibt ein Queue-Tier + misst dessen Observer + persistiert. Schließt „q1/q2 Container+Dock".

#include "builder/pruef_dock/container_dock.hpp"
#include "anatomy/container_anatomy.hpp"
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_registry.hpp>

#include <iostream>
#include <string>

namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace cea = comdare::cache_engine::anatomy;
namespace q1  = comdare::cache_engine::queuing::axis_q1_queuing;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

int main() {
    std::cout << "Container-Prüf-Dock (Doc 24 §8.8): treibt Queue-Tier + misst Container-Observer:\n";

    using Comp    = cea::ContainerComposition<q1::FIFOQueueBuffer>;
    using Anatomy = cea::ContainerAnatomy<Comp>;
    pd::ContainerDock<Anatomy> dock;

    check_true("Dock-Gattung == Adapter (Container)", dock.genus() == cea::AnatomyGenus::Adapter);

    auto const r = dock.measure(/*n_puts=*/1000, /*n_gets=*/400);   // treibt das echte FIFO-Organ
    check_eq("total_ops == 1400", r.total_ops, std::uint64_t{1400});
    check_eq("Observer: put_count == 1000", r.observer.put_count, std::uint64_t{1000});
    check_eq("Observer: get_count == 400", r.observer.get_count, std::uint64_t{400});
    check_eq("Observer: peak_occupancy == 1000", r.observer.peak_occupancy, std::uint64_t{1000});
    check_eq("Observer: current_occupancy == 600 (1000-400)", r.observer.current_occupancy, std::uint64_t{600});

    std::string const csv = dock.serialize_csv(r);
    check_true("CSV-Persistierung nicht leer + enthält 'Container'", !csv.empty() && csv.find("Container,1400,1000,400,1000,600") != std::string::npos);
    std::cout << "    CSV:\n" << csv;

    std::cout << "\n==== Container-Prüf-Dock: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
