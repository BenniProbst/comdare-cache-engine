// V41.F.6.1.R7.3 axis_08_concurrency Tests (Goldstandard-konform)
//
// @memory [[axis-gold-standard-checklist]]
// @task #718 V41.F.6.1.R7.3 (Queuing+Concurrency-Vollausbau: axis_08)

#include <gtest/gtest.h>

#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_none.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_blocking.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_reader_writer.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_olc.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_lock_free.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_wait_free.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_rcu.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_hazard_pointer.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_registry.hpp>
#include <topics/concurrency/axis_08_concurrency/axis_08_concurrency_subaxes_cc1_to_cc2.hpp>
#include <axes/concurrency_axis/axis_08_concurrency_flags.hpp>
#include <topics/concurrency/topic_concurrency_config_set.hpp>

#include <boost/mp11.hpp>
#include <string_view>
#include <type_traits>

namespace ax08 = ::comdare::cache_engine::concurrency::axis_08_concurrency;
namespace cc   = ::comdare::cache_engine::concurrency;
namespace mp   = ::boost::mp11;

using CP = ax08::concepts::ConcurrencyPattern;

TEST(R7_3_Axis08, AllStrategiesSatisfyConcepts) {
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::NoneConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::BlockingConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::ReaderWriterConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::OlcOptimisticConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::LockFreeConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::WaitFreeConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::RcuConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::HazardPointerConcurrency>);
    static_assert(ax08::concepts::ConcurrencyStrategy<ax08::OlcReservedBlocksConcurrency>);
    // R7.1.b (Ledger §a): CacheEnginePermutationStrategy fuer ALLE 9 Wrapper (vorher nur 5) — volle per-Wrapper-Coverage.
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::NoneConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::BlockingConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::ReaderWriterConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::OlcOptimisticConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::LockFreeConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::WaitFreeConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::RcuConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::HazardPointerConcurrency>);
    static_assert(ax08::concepts::CacheEnginePermutationStrategy<ax08::OlcReservedBlocksConcurrency>);
    SUCCEED();
}

TEST(R7_3_Axis08, PatternsDistinctAndCorrect) {
    static_assert(ax08::NoneConcurrency::concurrency_pattern() == CP::None);
    static_assert(ax08::BlockingConcurrency::concurrency_pattern() == CP::Blocking);
    static_assert(ax08::ReaderWriterConcurrency::concurrency_pattern() == CP::ReaderWriter);
    static_assert(ax08::OlcOptimisticConcurrency::concurrency_pattern() == CP::Optimistic);
    static_assert(ax08::LockFreeConcurrency::concurrency_pattern() == CP::LockFree);
    static_assert(ax08::WaitFreeConcurrency::concurrency_pattern() == CP::WaitFree);
    static_assert(ax08::RcuConcurrency::concurrency_pattern() == CP::RCU);
    static_assert(ax08::HazardPointerConcurrency::concurrency_pattern() == CP::HazardPtr);
    SUCCEED();
}

TEST(R7_3_Axis08, FlagSuffixUppercase) {
    static_assert(ax08::NoneConcurrency::flag_suffix() == std::string_view{"NONE"});
    static_assert(ax08::BlockingConcurrency::flag_suffix() == std::string_view{"BLOCKING"});
    static_assert(ax08::ReaderWriterConcurrency::flag_suffix() == std::string_view{"READER_WRITER"});
    static_assert(ax08::OlcOptimisticConcurrency::flag_suffix() == std::string_view{"OPTIMISTIC"});
    static_assert(ax08::LockFreeConcurrency::flag_suffix() == std::string_view{"LOCK_FREE"});
    static_assert(ax08::WaitFreeConcurrency::flag_suffix() == std::string_view{"WAIT_FREE"});
    static_assert(ax08::RcuConcurrency::flag_suffix() == std::string_view{"RCU"});
    static_assert(ax08::HazardPointerConcurrency::flag_suffix() == std::string_view{"HAZARD_PTR"});
    static_assert(ax08::OlcReservedBlocksConcurrency::flag_suffix() == std::string_view{"OLC_RESERVED_BLOCKS"});
    SUCCEED();
}

TEST(R7_3_Axis08, SubaxesOrthogonal) {
    static_assert(std::is_same_v<ax08::NoneConcurrency::axis_tag, ax08::subaxes::synchronization_pattern_tag>);
    static_assert(std::is_same_v<ax08::OlcOptimisticConcurrency::axis_tag, ax08::subaxes::synchronization_pattern_tag>);
    static_assert(std::is_same_v<ax08::WaitFreeConcurrency::axis_tag, ax08::subaxes::synchronization_pattern_tag>);
    static_assert(std::is_same_v<ax08::RcuConcurrency::axis_tag, ax08::subaxes::reclamation_scheme_tag>);
    static_assert(std::is_same_v<ax08::HazardPointerConcurrency::axis_tag, ax08::subaxes::reclamation_scheme_tag>);
    SUCCEED();
}

TEST(R7_3_Axis08, RegistryHas9Strategies) {
    static_assert(mp::mp_size<ax08::AllStrategies>::value == 9); // F.6: + OlcReservedBlocks
    static_assert(mp::mp_size<ax08::EnabledStrategies>::value > 0);
    SUCCEED();
}

TEST(R7_3_Axis08, FamilyIdsDistinct) {
    static_assert(ax08::NoneConcurrency::family_id::value == 1);
    static_assert(ax08::BlockingConcurrency::family_id::value == 2);
    static_assert(ax08::ReaderWriterConcurrency::family_id::value == 3);
    static_assert(ax08::OlcOptimisticConcurrency::family_id::value == 4);
    static_assert(ax08::LockFreeConcurrency::family_id::value == 5);
    static_assert(ax08::WaitFreeConcurrency::family_id::value == 6);
    static_assert(ax08::RcuConcurrency::family_id::value == 7);
    static_assert(ax08::HazardPointerConcurrency::family_id::value == 8);
    static_assert(ax08::OlcReservedBlocksConcurrency::family_id::value == 9);
    SUCCEED();
}

TEST(R7_3_Axis08, FlagsHeaderConstexprBools) {
    static_assert(std::is_same_v<decltype(ax08::flags::none_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax08::flags::optimistic_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax08::flags::rcu_enabled), const bool>);
    static_assert(std::is_same_v<decltype(ax08::flags::hazard_ptr_enabled), const bool>);
    SUCCEED();
}

TEST(R7_3_Concurrency, TopicConfigSetExposesAxis08) {
    static_assert(mp::mp_size<cc::TopicConfigSet::StaticAxisVariants_08>::value > 0);
    static_assert(std::is_same_v<cc::TopicConfigSet::StaticAxisVariants, cc::TopicConfigSet::StaticAxisVariants_08>);
    SUCCEED();
}
