// V41.F.6.1.R5.B — AnatomyExecutionContext + 5 Builder-Commands Tests
//
// Beweist:
// 1. AnatomyExecutionContext<Composition> wraps Anatomie + Container
// 2. Container-Operationen funktionieren via Context-API
// 3. AnatomyInsertCommand/LookupCommand/EraseCommand/ClearCommand/ObserveCommand
//    erfuellen ICommand-Interface + arbeiten auf Context
// 4. Composition-Inspection durchgereicht via Context
// 5. Roundtrip-Test pro 11 Algorithmen (6 CE-Re-Impl + 5 PaperBinding)
//
// User-Direktive 2026-05-26 sehr spaet (Doku 14 Teil 3 §17.3+§24):
// "Anatomie nur Achsen + Observer. Container-Operationen in CacheEngineBuilder."
//
// @task #698 V41.F.6.1.R5.B

#include <gtest/gtest.h>

#include <anatomy/known_algorithms.hpp>
#include <builder/anatomy_commands/anatomy_execution_context.hpp>
#include <builder/anatomy_commands/anatomy_insert_command.hpp>
#include <builder/anatomy_commands/anatomy_lookup_command.hpp>
#include <builder/anatomy_commands/anatomy_erase_command.hpp>
#include <builder/anatomy_commands/anatomy_clear_command.hpp>
#include <builder/anatomy_commands/anatomy_observe_command.hpp>

namespace bcmd      = ::comdare::cache_engine::builder::anatomy_commands;
namespace ana       = ::comdare::cache_engine::anatomy;
namespace ce_compos = ::comdare::cache_engine::compositions;

// ─────────────────────────────────────────────────────────────────────────────
// §1 — AnatomyExecutionContext Instantiation + Container-Operationen
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5B_ExecutionContext, DefaultConstructsEmpty) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    EXPECT_TRUE(ctx.empty());
    EXPECT_EQ(ctx.size(), 0u);
}

TEST(R5B_ExecutionContext, InsertLookupEraseClearRoundtrip) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    EXPECT_TRUE(ctx.insert(1, 100));
    EXPECT_TRUE(ctx.insert(2, 200));
    EXPECT_EQ(ctx.size(), 2u);
    auto v1 = ctx.lookup(1);
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, 100u);
    EXPECT_FALSE(ctx.lookup(999).has_value());
    EXPECT_TRUE(ctx.erase(1));
    EXPECT_FALSE(ctx.erase(1));  // already erased
    EXPECT_EQ(ctx.size(), 1u);
    ctx.clear();
    EXPECT_TRUE(ctx.empty());
}

TEST(R5B_ExecutionContext, CompositionInspectionDurchgereicht) {
    using Ctx = bcmd::AnatomyExecutionContext<ce_compos::ArtComposition>;
    static_assert(Ctx::composition_name() == std::string_view{"ArtComposition"});
    static_assert(Ctx::paper_id().starts_with("P01"));
    static_assert(Ctx::organ_count() == 17);
    SUCCEED();
}

TEST(R5B_ExecutionContext, ObserveAllDelegatesToAnatomy) {
    bcmd::AnatomyExecutionContext<ce_compos::HotComposition> ctx;
    auto agg = ctx.observe_all();
    static_assert(decltype(agg)::total_slots() == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §2 — Builder-Commands (ICommand-Interface + Container-Ops via Context)
// ─────────────────────────────────────────────────────────────────────────────

TEST(R5B_Commands, AnatomyInsertCommandRoundtrip) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    bcmd::AnatomyInsertCommand<ce_compos::ArtComposition> cmd(ctx, 42, 4242);
    EXPECT_EQ(cmd.command_name(), std::string_view{"AnatomyInsertCommand"});
    EXPECT_EQ(cmd.execute(), 0);
    EXPECT_EQ(ctx.size(), 1u);
}

TEST(R5B_Commands, AnatomyLookupCommandResult) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    ctx.insert(7, 77);
    bcmd::AnatomyLookupCommand<ce_compos::ArtComposition> cmd(ctx, 7);
    EXPECT_EQ(cmd.execute(), 0);  // hit
    ASSERT_TRUE(cmd.result().has_value());
    EXPECT_EQ(*cmd.result(), 77u);

    bcmd::AnatomyLookupCommand<ce_compos::ArtComposition> miss(ctx, 999);
    EXPECT_EQ(miss.execute(), 1);  // miss
    EXPECT_FALSE(miss.result().has_value());
}

TEST(R5B_Commands, AnatomyEraseCommandReports) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    ctx.insert(5, 50);
    bcmd::AnatomyEraseCommand<ce_compos::ArtComposition> erase_existing(ctx, 5);
    EXPECT_EQ(erase_existing.execute(), 0);
    EXPECT_TRUE(erase_existing.erased());

    bcmd::AnatomyEraseCommand<ce_compos::ArtComposition> erase_missing(ctx, 999);
    EXPECT_EQ(erase_missing.execute(), 1);
    EXPECT_FALSE(erase_missing.erased());
}

TEST(R5B_Commands, AnatomyClearCommandResets) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    ctx.insert(1, 10);
    ctx.insert(2, 20);
    EXPECT_EQ(ctx.size(), 2u);
    bcmd::AnatomyClearCommand<ce_compos::ArtComposition> clear(ctx);
    EXPECT_EQ(clear.execute(), 0);
    EXPECT_TRUE(ctx.empty());
}

TEST(R5B_Commands, AnatomyObserveCommandSnapshot) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    bcmd::AnatomyObserveCommand<ce_compos::ArtComposition> obs(ctx);
    EXPECT_EQ(obs.execute(), 0);
    auto const& snap = obs.last_snapshot();
    static_assert(std::remove_cvref_t<decltype(snap)>::total_slots() == 17);
    SUCCEED();
}

// ─────────────────────────────────────────────────────────────────────────────
// §3 — Roundtrip TYPED_TEST_SUITE fuer alle 11 Anatomien
// ─────────────────────────────────────────────────────────────────────────────

template <typename Composition>
class CompositionContextRoundtrip : public ::testing::Test {};

using All11Compositions = ::testing::Types<
    ce_compos::ArtComposition,
    ce_compos::HotComposition,
    ce_compos::WormholeComposition,
    ce_compos::SurfComposition,
    ce_compos::MasstreeComposition,
    ce_compos::StartComposition,
    ce_compos::ArtPaperBindingComposition,
    ce_compos::HotPaperBindingComposition,
    ce_compos::StartPaperBindingComposition,
    ce_compos::WormholePaperBindingComposition,
    ce_compos::SurfPaperBindingComposition
>;
TYPED_TEST_SUITE(CompositionContextRoundtrip, All11Compositions);

TYPED_TEST(CompositionContextRoundtrip, FullInsertLookupEraseClearViaCommands) {
    bcmd::AnatomyExecutionContext<TypeParam> ctx;

    // Insert via Command
    bcmd::AnatomyInsertCommand<TypeParam> ins1(ctx, 1, 100);
    bcmd::AnatomyInsertCommand<TypeParam> ins2(ctx, 2, 200);
    bcmd::AnatomyInsertCommand<TypeParam> ins3(ctx, 3, 300);
    EXPECT_EQ(ins1.execute(), 0);
    EXPECT_EQ(ins2.execute(), 0);
    EXPECT_EQ(ins3.execute(), 0);
    EXPECT_EQ(ctx.size(), 3u);

    // Lookup via Command
    bcmd::AnatomyLookupCommand<TypeParam> lk2(ctx, 2);
    EXPECT_EQ(lk2.execute(), 0);
    ASSERT_TRUE(lk2.result().has_value());
    EXPECT_EQ(*lk2.result(), 200u);

    // Erase via Command
    bcmd::AnatomyEraseCommand<TypeParam> er(ctx, 2);
    EXPECT_EQ(er.execute(), 0);
    EXPECT_TRUE(er.erased());
    EXPECT_EQ(ctx.size(), 2u);

    // Observe via Command (R5.A Snapshot)
    bcmd::AnatomyObserveCommand<TypeParam> obs(ctx);
    EXPECT_EQ(obs.execute(), 0);
    static_assert(std::remove_cvref_t<decltype(obs.last_snapshot())>::total_slots() == 17);

    // Clear via Command
    bcmd::AnatomyClearCommand<TypeParam> cl(ctx);
    EXPECT_EQ(cl.execute(), 0);
    EXPECT_TRUE(ctx.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// §4 — Anatomie hat KEINE Container-API mehr (Compile-Time-Beweis)
// ─────────────────────────────────────────────────────────────────────────────

template <typename T>
concept HasInsertMethod = requires(T t, std::uint64_t k, std::uint64_t v) {
    { t.insert(k, v) } -> std::convertible_to<bool>;
};

TEST(R5B_AnatomyApiStripped, SearchAlgorithmAnatomyHasNoInsert) {
    // Beweis: SearchAlgorithmAnatomy<C>::insert existiert NICHT mehr nach R5.B
    static_assert(!HasInsertMethod<ana::Art>,
        "SearchAlgorithmAnatomy<C> darf KEINE insert() Methode mehr haben (R5.B)");
    static_assert(!HasInsertMethod<ana::HotPaperBinding>,
        "Auch PaperBinding-Variante darf keine insert() haben");
    // ExecutionContext hat insert() — Pflicht
    static_assert(HasInsertMethod<bcmd::AnatomyExecutionContext<ce_compos::ArtComposition>>);
    SUCCEED();
}

// V41 Saeule-2 (Doku 24 §5.2/§5.3) — observe_all() liefert ECHTE Per-Achsen-Statistik aus dem GETRIEBENEN
// uint64-Container (ObservableComposedSearch). Dieser Test scheiterte mit der frueheren losgelosten std::map
// (alle Zaehler 0) → er beweist die Schliessung der observe_all-NULL-Luecke. Nur unter STATISTICS (sonst
// existieren snapshot_t/statistics() nicht und observe_all liefert reine Anatomie-Defaults).
#ifdef COMDARE_CE_ENABLE_STATISTICS
namespace comp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;

// Compile-Selbstbeweis: der getriebene Container IST eine ObservableAxis (observer_aggregate.hpp:40-44).
static_assert(ana::ObservableAxis<comp::ObservableComposedSearch<comp::SortedBinaryTraversal, comp::RawSlotStore>>);

TEST(R5B_ObserveReal, SearchAlgoCountersReflectDrivenOps) {
    bcmd::AnatomyExecutionContext<ce_compos::ArtComposition> ctx;
    EXPECT_TRUE(ctx.insert(1, 100));
    EXPECT_TRUE(ctx.insert(2, 200));
    EXPECT_TRUE(ctx.insert(3, 300));
    auto const s1 = ctx.observe_all().search_algo;
    EXPECT_EQ(s1.total_insert_count, 3u);
    EXPECT_GE(s1.peak_occupancy,     3u);

    EXPECT_TRUE (ctx.lookup(2).has_value());    // hit
    EXPECT_FALSE(ctx.lookup(999).has_value());  // miss (uint64 → kein narrow-key-Alias wie 999%256)
    auto const s2 = ctx.observe_all().search_algo;
    EXPECT_EQ(s2.total_lookup_count, 2u);
    EXPECT_EQ(s2.total_hit_count,    1u);
    EXPECT_EQ(s2.total_miss_count,   1u);

    EXPECT_TRUE(ctx.erase(2));
    auto const s3 = ctx.observe_all().search_algo;
    EXPECT_EQ(s3.total_erase_count, 1u);

    // Idempotenz: observe_all ohne State-Aenderung liefert identische Zaehler (reiner Snapshot-Read).
    auto const s3b = ctx.observe_all().search_algo;
    EXPECT_EQ(s3.total_insert_count, s3b.total_insert_count);
    EXPECT_EQ(s3.total_lookup_count, s3b.total_lookup_count);

    // search_algo-Slot ist observable (>= 1 observable Achse im Aggregat).
    EXPECT_GE(decltype(ctx.observe_all())::observable_count(), 1u);
}
#endif
