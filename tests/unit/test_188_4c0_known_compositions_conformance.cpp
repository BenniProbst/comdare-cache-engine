// #188-4c-0 (2026-07-02) -- Known-Compositions-Conformance-Gate.
//
// Sicherheitsnetz fuer den Spiegel-Abbau (#188-4c): jede bekannte Reference-/
// PaperBinding-Composition wird ueber den realen SearchAlgorithmAbiAdapter
// durch das std::map-Orakel des Pruef-Docks getrieben. #188-4c-i flippt den
// Routing-Snapshot bewusst auf store-/container-autoritativ: Reference-Huellen
// nutzen keinen SortedBinary-Spiegel mehr.

#include <anatomy/composition_concept.hpp>
#include <compositions/known_compositions_list.hpp>

#if COMDARE_MEASUREMENT_ON
#include <anatomy/abi_adapter.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/codegen/all_axes_umbrella.hpp>
#include <builder/pruef_dock/conformance_gate.hpp>
#endif

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace mp11 = ::boost::mp11;

namespace {

using KnownCompositionEntries = comp::KnownReferenceCompositions;

inline constexpr std::size_t kNoCompositions        = 0u;
inline constexpr std::size_t kKnownCompositionCount = mp11::mp_size<KnownCompositionEntries>::value;

template <class Entry>
using EntryCompositionIsComposition = std::bool_constant<an::IsComposition<typename Entry::composition>>;

template <class Entry>
std::string label_for() {
    using Composition = typename Entry::composition;
    std::string label{std::string_view{Composition::name}};
    label += " [";
    label += std::string{std::string_view{Entry::short_name}};
    label += "]";
    return label;
}

#if COMDARE_MEASUREMENT_ON

namespace dock = ::comdare::cache_engine::builder::pruef_dock;

inline constexpr std::uint64_t kNoFailures           = 0u;
inline constexpr std::uint64_t kConformanceSeed      = 42u;
inline constexpr std::uint64_t kConformanceRandomOps = 2000u;
inline constexpr std::uint64_t kInitialInsertCount   = 64u;
inline constexpr std::uint64_t kReuseInsertCount     = 8u;
inline constexpr std::uint64_t kInitialKeyBase       = 1u;
inline constexpr std::uint64_t kReuseKeyBase         = 10'000u;
inline constexpr std::uint64_t kValueSalt            = 0x9E3779B97F4A7C15ull;
inline constexpr std::uint64_t kLookupSentinel       = 0xBAD0C0DEu;

template <class Composition>
using AdapterFor = an::SearchAlgorithmAbiAdapter<an::SearchAlgorithmAnatomy<Composition>>;

[[nodiscard]] std::uint64_t value_for(std::uint64_t key) noexcept { return key ^ kValueSalt; }

template <class Fn>
void for_each_known_composition(Fn&& fn) {
    mp11::mp_for_each<KnownCompositionEntries>([&](auto entry_tag) {
        using KnownEntry = std::decay_t<decltype(entry_tag)>;
        fn.template operator()<KnownEntry>();
    });
}

struct ConformanceRunner {
    std::size_t visited = kNoCompositions;

    template <class Entry>
    void operator()() {
        using Composition = typename Entry::composition;
        using Adapter     = AdapterFor<Composition>;

        SCOPED_TRACE(label_for<Entry>());

        auto tier = std::make_unique<Adapter>();
        auto& drv = static_cast<an::IDriveableTier&>(*tier);

        auto const result = dock::run_conformance_gate(drv, kConformanceSeed, kConformanceRandomOps);
        EXPECT_TRUE(result.passed()) << label_for<Entry>() << " cases=" << result.cases_passed << "/"
                                     << result.cases_total << " first_fail=" << result.first_fail;
        EXPECT_GT(result.cases_total, kNoFailures) << label_for<Entry>() << " Gate wurde nicht getrieben";
        EXPECT_EQ(result.first_fail, kNoFailures) << label_for<Entry>() << " erste Gate-Verletzung";

        ++visited;
    }
};

struct SizeClearReuseRunner {
    std::size_t visited = kNoCompositions;

    template <class Entry>
    void operator()() {
        using Composition = typename Entry::composition;
        using Adapter     = AdapterFor<Composition>;

        SCOPED_TRACE(label_for<Entry>());

        auto tier = std::make_unique<Adapter>();
        auto& drv = static_cast<an::IDriveableTier&>(*tier);
        drv.tier_clear();

        for (std::uint64_t offset = kNoFailures; offset < kInitialInsertCount; ++offset) {
            auto const key = kInitialKeyBase + offset;
            EXPECT_TRUE(drv.tier_insert(key, value_for(key))) << label_for<Entry>() << " insert key=" << key;
        }
        EXPECT_EQ(drv.tier_size(), kInitialInsertCount) << label_for<Entry>() << " size nach Initial-Inserts";

        drv.tier_clear();
        EXPECT_EQ(drv.tier_size(), kNoFailures) << label_for<Entry>() << " size nach clear";

        std::uint64_t stale_value = kLookupSentinel;
        EXPECT_FALSE(drv.tier_lookup(kInitialKeyBase, &stale_value)) << label_for<Entry>() << " alter Key nach clear";

        for (std::uint64_t offset = kNoFailures; offset < kReuseInsertCount; ++offset) {
            auto const key      = kReuseKeyBase + offset;
            auto const expected = value_for(key);
            EXPECT_TRUE(drv.tier_insert(key, expected)) << label_for<Entry>() << " reuse insert key=" << key;

            std::uint64_t actual = kLookupSentinel;
            EXPECT_TRUE(drv.tier_lookup(key, &actual)) << label_for<Entry>() << " reuse lookup key=" << key;
            EXPECT_EQ(actual, expected) << label_for<Entry>() << " reuse value key=" << key;
        }
        EXPECT_EQ(drv.tier_size(), kReuseInsertCount) << label_for<Entry>() << " size nach Wiederverwendung";

        ++visited;
    }
};

struct RoutingSnapshotRunner {
    std::size_t visited = kNoCompositions;

    template <class Entry>
    void operator()() {
        using Composition = typename Entry::composition;
        using Adapter     = AdapterFor<Composition>;

        constexpr bool routes_through_store = Adapter::tier_search_routes_through_store();
        ::testing::Test::RecordProperty(std::string{Composition::name}, routes_through_store ? "true" : "false");

        // #188-4c-i: Known-Reference/PaperBinding-Compositions tragen bereits
        // ObservableComposedContainer<XOrgan> als search_algo; der ABI-Adapter routet
        // T0/insert/lookup/erase jetzt direkt durch diese autoritative Huelle.
        EXPECT_TRUE(routes_through_store) << label_for<Entry>();

        ++visited;
    }
};

#endif // COMDARE_MEASUREMENT_ON

} // namespace

static_assert(kKnownCompositionCount > kNoCompositions, "#188-4c-0: Known-Composition-Liste darf nicht leer sein");
static_assert(mp11::mp_all_of<KnownCompositionEntries, EntryCompositionIsComposition>::value,
              "#188-4c-0: jede Known Composition muss IsComposition erfuellen");

TEST(KnownCompositionsConformance1884c0, CompileTimeListIsNonEmptyAndEveryEntryIsAComposition) {
    EXPECT_GT(kKnownCompositionCount, kNoCompositions);
    EXPECT_TRUE((mp11::mp_all_of<KnownCompositionEntries, EntryCompositionIsComposition>::value));
}

#if COMDARE_MEASUREMENT_ON

TEST(KnownCompositionsConformance1884c0, AllKnownCompositionsPassStdMapOracle) {
    ConformanceRunner runner{};
    for_each_known_composition(runner);
    EXPECT_EQ(runner.visited, kKnownCompositionCount);
}

TEST(KnownCompositionsConformance1884c0, SizeClearReuseRoundTripForEveryKnownComposition) {
    SizeClearReuseRunner runner{};
    for_each_known_composition(runner);
    EXPECT_EQ(runner.visited, kKnownCompositionCount);
}

TEST(KnownCompositionsConformance1884c0, SearchRoutingIsStoreAuthoritativeSince1884ci) {
    RoutingSnapshotRunner runner{};
    for_each_known_composition(runner);
    EXPECT_EQ(runner.visited, kKnownCompositionCount);
}

#else

TEST(KnownCompositionsConformance1884c0, AdapterGateRequiresMeasurementBuild) {
    GTEST_SKIP() << "COMDARE_MEASUREMENT_ON=1 ist fuer Adapter-/Diagnosepfade erforderlich.";
}

#endif // COMDARE_MEASUREMENT_ON
