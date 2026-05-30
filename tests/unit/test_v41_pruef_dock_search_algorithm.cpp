// V41.F.6.1.R6 — Prüf-Dock SearchAlgorithm: In-Process-Verkabelungs-Test (Doku 24 §8.8).
//
// Belegt, dass das SearchAlgorithmDock die drei bestehenden Zahnräder (IObservableTier + AnatomyModuleHandle +
// drive_tier_observe_trace_abi/serialize) uniform hinter dem per-Gattung IPruefDock-Vertrag zusammenhält —
// und dass die PruefDockRegistry per IM MODUL deklarierter Gattung (anatomy()->genus()) das passende Dock
// auswählt. In-Process-Modul: AnatomyModuleHandle um einen Stack-Adapter (native=nullptr, destroy=nullptr →
// unload() no-op via Guards) — Stand-in für ein dlopen-geladenes Tier-Modul (identische vtable über die Grenze).

#include <gtest/gtest.h>

#include <builder/pruef_dock/pruef_dock.hpp>
#include <builder/pruef_dock/search_algorithm_dock.hpp>
#include <builder/pruef_dock/pruef_dock_registry.hpp>
#include <anatomy/abi_adapter.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <compositions/art_reference.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace pd   = ::comdare::cache_engine::builder::pruef_dock;
namespace al   = ::comdare::cache_engine::builder::anatomy_loader;
namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;

TEST(PruefDock, SearchAlgorithmDockMeasuresInProcessModule) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
    using Anatomy = an::SearchAlgorithmAnatomy<comp::ArtComposition>;
    an::SearchAlgorithmAbiAdapter<Anatomy> adapter;                      // In-Process-"Tier-Modul"
    al::AnatomyModuleHandle h{nullptr, &adapter, nullptr, {1, 0}};       // native/destroy=null → unload() no-op

    pd::SearchAlgorithmDock dock;
    EXPECT_EQ(dock.dock_genus(), an::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(dock.dock_name(), std::string_view{"SearchAlgorithmDock"});
    EXPECT_TRUE(dock.accepts(h));                                        // Gattungs-Match über genus()

    // Registry waehlt per im Modul deklarierter Gattung das passende Dock.
    pd::PruefDockRegistry reg;
    reg.register_dock(std::make_unique<pd::SearchAlgorithmDock>());
    EXPECT_EQ(reg.size(), 1u);
    auto* selected = reg.select_for(h);
    ASSERT_NE(selected, nullptr);
    EXPECT_EQ(selected->dock_genus(), an::AnatomyGenus::SearchAlgorithm);
    EXPECT_EQ(reg.dock_for_genus(an::AnatomyGenus::SearchAlgorithm), selected);

    // measure(): zieht IObservableTier per dynamic_cast + faehrt den bestehenden Treiber + serialisiert.
    pd::PruefDockMeasureOptions opts;
    opts.fill_checkpoints       = {10, 100};
    opts.lookups_per_checkpoint = 200;
    opts.deletes_per_checkpoint = 20;
    std::string csv, json;
    int const rc = selected->measure(h, opts, csv, json);
    EXPECT_EQ(rc, pd::dock_status_ok) << pd::dock_status_name(rc);
    EXPECT_NE(csv.find("checkpoint,observe_wall_ns"), std::string::npos);   // CSV-Header
    ASSERT_FALSE(json.empty());
    EXPECT_EQ(json.front(), '[');                                          // JSON-Array
    EXPECT_NE(json.find("\"search_insert\""), std::string::npos);          // Observer-Zaehler korreliert
    SUCCEED();
#else
    GTEST_SKIP() << "COMDARE_CE_ENABLE_STATISTICS aus — Prüf-Dock-Mess-Test n/a";
#endif
}

// Negativ-Pfad: ein leeres Handle (keine Anatomie) wird vom Dock sauber abgelehnt (kein Match, status).
TEST(PruefDock, EmptyHandleRejectedCleanly) {
    al::AnatomyModuleHandle empty{};   // valid()==false, anatomy()==nullptr
    pd::SearchAlgorithmDock dock;
    EXPECT_FALSE(dock.accepts(empty));

    pd::PruefDockRegistry reg;
    reg.register_dock(std::make_unique<pd::SearchAlgorithmDock>());
    EXPECT_EQ(reg.select_for(empty), nullptr);   // kein Dock akzeptiert ein anatomie-loses Handle

    pd::PruefDockMeasureOptions opts;
    std::string csv, json;
    EXPECT_EQ(dock.measure(empty, opts, csv, json), pd::dock_status_no_anatomy);
}
