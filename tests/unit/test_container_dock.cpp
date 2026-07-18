// test_container_dock — Container-Prüf-Dock (2026-06-02, Doc 24 §8.8) — die per-Gattung Mess-Seite für die
// Container-Gattung treibt ein Container-Tier + misst dessen Observer + persistiert.
// #87+#90 (2026-06-03, Doku 14 §28): die Adapter-Tier-Unterklasse hat 13 Achsen (12 geteilt/delegiert +
// inner_container), KEINE „ordering"-Achse. DequeInner = queue-Default (get == FIFO pop_front).

#include "builder/pruef_dock/adapter_dock.hpp"
#include "anatomy/adapter_anatomy.hpp"

#include <iostream>
#include <string>

namespace pd  = comdare::cache_engine::builder::pruef_dock;
namespace cea = comdare::cache_engine::anatomy;

// Platzhalter für die 12 geteilten/delegierten §28-Achsen (analog test_container_genus.cpp).
struct DelegatedAxis {};

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "Container-Prüf-Dock (Doc 24 §8.8): treibt Container-Tier + misst Container-Observer:\n";

    using D       = DelegatedAxis;
    using Comp    = cea::AdapterComposition<D, D, D, D, D, D, D, D, D, D, cea::DequeInner<>>; // 11 + inner (INC-2c)
    using Anatomy = cea::AdapterAnatomy<Comp>;
    pd::AdapterDock<Anatomy> dock;

    check_true("Dock-Gattung == Adapter (Container)", dock.genus() == cea::AnatomyGenus::Adapter);

    auto const r = dock.measure(/*n_puts=*/1000, /*n_gets=*/400); // treibt inner_container (push + get/pop_front)
    check_eq("total_ops == 1400", r.total_ops, std::uint64_t{1400});
    check_eq("Observer: push_count == 1000", r.observer.push_count, std::uint64_t{1000});
    check_eq("Observer: pop_count == 400", r.observer.pop_count, std::uint64_t{400});
    check_eq("Observer: front_reads == 400 (get == pop_front)", r.observer.front_reads, std::uint64_t{400});
    check_eq("Observer: peak_occupancy == 1000", r.observer.peak_occupancy, std::uint64_t{1000});
    check_eq("Observer: current_occupancy == 600 (1000-400)", r.observer.current_occupancy, std::uint64_t{600});

    std::string const csv = dock.serialize_csv(r);
    check_true("CSV-Persistierung nicht leer + enthält 'Container'",
               !csv.empty() && csv.find("Container,1400,1000,400,400") != std::string::npos);
    std::cout << "    CSV:\n" << csv;

    std::cout << "\n==== Container-Prüf-Dock: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
