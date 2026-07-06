// V5-#46-REFERENZ — Disk-persistierender MementoAxis: Beweis „Memento inkl. IO/Disk-Persistenz".
//
// Beweist: DiskCheckpointStore erfüllt das MementoAxis-Concept UND sein Zustand quert beim save/restore die
// DISK (der memento_t trägt nur den Pfad). Demonstriert den /goal-Kontrakt für künftige echte Disk-Achsen.

#include "anatomy/memento_aggregate.hpp"
#include "comdare_test_tmp.hpp" // #278/#24: per-User-Temp gegen CI-Kollisionen
#include "builder/disk_checkpoint_memento.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace an = ::comdare::cache_engine::anatomy;
namespace b  = ::comdare::cache_engine::builder;

namespace {
std::filesystem::path tmp_ckpt(char const* name) {
    return ::comdare::test::user_tmp_dir() / "comdare_v5_disk_memento" / name;
}
} // namespace

// DiskCheckpointStore ist eine MementoAxis (memento_aggregate.hpp), trotz Disk-residentem Zustand.
TEST(V5DiskMemento, IsMementoAxis) {
    static_assert(an::MementoAxis<b::DiskCheckpointStore>, "Disk-Achse MUSS MementoAxis sein (#46-Kontrakt)");
    static_assert(std::is_same_v<an::memento_of_t<b::DiskCheckpointStore>, std::string>,
                  "memento_t = Pfad (Zustand liegt auf DISK, nicht im Memento)");
    SUCCEED();
}

// save_state schreibt den Zustand auf DISK; restore_state liest ihn ZURÜCK — Round-Trip über die Platte.
TEST(V5DiskMemento, SaveRestoreRoundTripsViaDisk) {
    auto const      path = tmp_ckpt("rt.ckpt");
    std::error_code ec;
    std::filesystem::remove(path, ec);
    b::DiskCheckpointStore store{path};
    store.set(42);
    auto const saved = an::save_axis(store); // schreibt Checkpoint-Datei, liefert Pfad
    EXPECT_FALSE(saved.empty());
    EXPECT_TRUE(std::filesystem::exists(path)) << "Checkpoint-Datei muss auf Disk existieren";
    store.set(999);                 // Warmup-Mutation (nur im RAM)
    an::restore_axis(store, saved); // liest von Disk zurück
    EXPECT_EQ(store.get(), 42u);
    // Beweis, dass der Zustand WIRKLICH auf der Disk lag: die Datei enthält die 8 Roh-Bytes von 42.
    std::ifstream is{path, std::ios::binary};
    std::uint64_t on_disk = 0;
    is.read(reinterpret_cast<char*>(&on_disk), sizeof on_disk);
    EXPECT_EQ(on_disk, 42u);
    std::filesystem::remove(path, ec);
}

// Aggregat-Integration: save_axis/restore_axis (die einheitlichen Memento-Hilfsfunktionen) tragen die Disk-Achse.
TEST(V5DiskMemento, WorksThroughUnifiedHelpers) {
    auto const      path = tmp_ckpt("helpers.ckpt");
    std::error_code ec;
    std::filesystem::remove(path, ec);
    b::DiskCheckpointStore store{path};
    store.set(7);
    auto const m = an::save_axis(store);
    store.set(0);
    an::restore_axis(store, m);
    EXPECT_EQ(store.get(), 7u);
    std::filesystem::remove(path, ec);
}
