// V41.F.6.1 R5.G — COMDARE_DEFINE_ANATOMY_MODULE_ADHOC: Materialisierung auto-enumerierter
// Permutationen (AdHocComposition) ohne benannten Composition-Header.
//
// Verifiziert das Makro END-TO-END: es definiert (wie in einem auto-generierten Permutations-Modul)
// die 4 extern-C-ABI-Symbole für eine AdHoc-Komposition aus 17 explizit übergebenen Achsen-Typen
// (hier: ArtCompositions 17 Achsen → AdHocComposition mit identischem Innenleben). Der Test ruft die
// vom Makro definierte Factory comdare_create_anatomy() auf und prüft die Anatomie + Mess-Hook.
//
// Damit ist der R5.G-Materialisierungs-Kern bewiesen: der Komma-im-Makro-Blocker (AdHocComposition<A,B,…>)
// ist via variadischem Makro gelöst; auto-enumerierte Permutationen sind codegen-fähig. Der verbleibende
// R5.G-Schritt ist nur noch der Auto-Emitter (for_each_composition_type → ein Modul-.cpp pro Permutation,
// das genau dieses Makro mit den 17 Achsen-Typen der Permutation aufruft).
//
// COMDARE_ANATOMY_ABI_STATIC=1 (via CMake) → Export-Makro = leer → die extern-C-Symbole sind im
// Test-Executable plain definierbar + direkt aufrufbar (kein dllimport).

#include <cache_engine/abi/anatomy_module_abi_v1.hpp>
#include <compositions/art_reference.hpp>

#include <gtest/gtest.h>
#include <cstdint>
#include <string_view>
#include <vector>

namespace comp = ::comdare::cache_engine::compositions;
namespace ana  = ::comdare::cache_engine::anatomy;

// MAKRO UNTER TEST: definiert comdare_anatomy_abi_version/magic/create/destroy für eine AdHoc-Permutation
// (= ArtCompositions 19 Achsen-Typen, variadisch übergeben — löst das Komma-im-Template-Arg-Problem; Doc 30 §8.0).
// cppcheck kennt die COMDARE-Codegen-Emitter-Makros nicht (Definition via Include-Kette, kein -I im Lint-Lauf).
// cppcheck-suppress unknownMacro
COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(comp::ArtComposition::search_algo, comp::ArtComposition::cache_traversal,
                                    comp::ArtComposition::mapping, comp::ArtComposition::path_compression,
                                    comp::ArtComposition::node_type, comp::ArtComposition::memory_layout,
                                    comp::ArtComposition::allocator, comp::ArtComposition::prefetch,
                                    comp::ArtComposition::concurrency, comp::ArtComposition::serialization,
                                    comp::ArtComposition::value_handle, comp::ArtComposition::index_organization,
                                    comp::ArtComposition::io_dispatch, comp::ArtComposition::migration_policy,
                                    comp::ArtComposition::filter, comp::ArtComposition::queuing_q1,
                                    comp::ArtComposition::queuing_q2)

TEST(R5G_AdHocCodegenMacro, MacroProducesWorkingAdHocAnatomyFactory) {
    // ABI-Probe-Symbole (vom Makro definiert).
    EXPECT_EQ(comdare_anatomy_abi_magic(), COMDARE_ANATOMY_ABI_MAGIC);
    auto const v = ::comdare::cache_engine::abi::AnatomyAbiVersion::unpack(comdare_anatomy_abi_version());
    EXPECT_EQ(v.major, static_cast<std::uint32_t>(COMDARE_ANATOMY_ABI_MAJOR));

    // Factory (vom Makro definiert) → AdHoc-Anatomie.
    ana::IAnatomyBase* base = comdare_create_anatomy();
    ASSERT_NE(base, nullptr);
    EXPECT_EQ(base->organ_count(), 17u); // volle 19-Achsen-Anatomie (Doc 30 §8.0)
    EXPECT_EQ(base->composition_name(), std::string_view{"AdHocComposition"});
    EXPECT_EQ(base->genus(), ana::AnatomyGenus::SearchAlgorithm);

    // Mess-Hook (Stufe B) funktioniert auch auf der AdHoc-Komposition.
    auto* mw = dynamic_cast<ana::IMeasurableWorkload*>(base);
    ASSERT_NE(mw, nullptr);
    std::vector<std::int64_t> samples(10);
    auto const                n =
        mw->run_workload(/*ops_per_batch=*/1000, /*batches=*/10, /*seed=*/7u, samples.data(), samples.size());
    EXPECT_EQ(n, 10u);
    EXPECT_GT(samples[0], 0);

    comdare_destroy_anatomy(base); // Gegenstück zur Factory
}
