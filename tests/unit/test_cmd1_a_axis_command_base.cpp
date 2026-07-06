// CMD-1-a (#267/#251) -- Compile-time-Command-Basis + Mess-Visitor-Slot.

#include <topics/axis_command_base.hpp>

#include <axes/cache_traversal/axis_03b_cache_traversal_linear_fanout.hpp>
#include <axes/concurrency_axis/axis_08_concurrency_none.hpp>
#include <axes/filter_axis/axis_filter_bloom.hpp>
#include <axes/index_organization/axis_01_index_organization_clustered.hpp>
#include <axes/io_dispatch/axis_io_in_memory_only.hpp>
#include <axes/layout/axis_05_memory_layout_soa.hpp>
#include <axes/lookup/axis_03a_search_algo_linear_scan.hpp>
#include <axes/mapping/axis_03m_mapping_direct_placement.hpp>
#include <axes/migration_policy/axis_migration_none.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/path_compression/axis_02_path_compression_none.hpp>
#include <axes/prefetch_axis/axis_07_prefetch_none.hpp>
#include <axes/serialization_axis/axis_10_serialization_raw_binary.hpp>
#include <axes/simd/axis_09_isa_amd64.hpp>
#include <axes/telemetry_axis/axis_11_telemetry_insert_counter.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_inline.hpp>
#include <topics/hardware/axis_09b_simd_extension/axis_09b_simd_extension_no_extension.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_generic.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_bplus.hpp>
#include <topics/nodes/axis_bst_shape/axis_bst_shape_ptr_u32.hpp>
#include <topics/nodes/axis_btree_order/axis_btree_order_kt4.hpp>
#include <topics/nodes/axis_hash_probe_shape/axis_hash_probe_shape_chaining.hpp>
#include <topics/nodes/axis_skip_list_shape/axis_skip_list_shape_max16_p50.hpp>
#include <topics/queuing/axis_q1_queuing/axis_q1_queuing_no_buffer.hpp>
#include <topics/queuing/axis_q2_queuing/axis_q2_queuing_eager.hpp>

#include <boost/mp11.hpp>
#include <gtest/gtest.h>

#include <string_view>

namespace an      = ::comdare::cache_engine::anatomy;
namespace alloc   = ::comdare::cache_engine::alloc;
namespace bst     = ::comdare::cache_engine::nodes::axis_bst_shape;
namespace btree   = ::comdare::cache_engine::nodes::axis_btree_order;
namespace conc    = ::comdare::cache_engine::concurrency_axis;
namespace ct      = ::comdare::cache_engine::cache_traversal;
namespace filt    = ::comdare::cache_engine::filter_axis;
namespace hw      = ::comdare::cache_engine::hardware::axis_12_general_hardware;
namespace idx     = ::comdare::cache_engine::index_organization;
namespace io      = ::comdare::cache_engine::io_dispatch;
namespace layout  = ::comdare::cache_engine::layout;
namespace lk      = ::comdare::cache_engine::lookup;
namespace mapping = ::comdare::cache_engine::mapping;
namespace mig     = ::comdare::cache_engine::migration_policy;
namespace mp      = ::boost::mp11;
namespace node    = ::comdare::cache_engine::node;
namespace page    = ::comdare::cache_engine::nodes::axis_01_page_type;
namespace pc      = ::comdare::cache_engine::path_compression;
namespace pref    = ::comdare::cache_engine::prefetch_axis;
namespace q1      = ::comdare::cache_engine::queuing::axis_q1_queuing;
namespace q2      = ::comdare::cache_engine::queuing::axis_q2_queuing;
namespace serial  = ::comdare::cache_engine::serialization_axis;
namespace simd    = ::comdare::cache_engine::simd;
namespace simdex  = ::comdare::cache_engine::hardware::axis_09b_simd_extension;
namespace skip    = ::comdare::cache_engine::nodes::axis_skip_list_shape;
namespace tel     = ::comdare::cache_engine::telemetry_axis;
namespace topics  = ::comdare::cache_engine::topics;
namespace vh      = ::comdare::cache_engine::value_handle_axis;

namespace {

struct Cmd1aPaperManifest {
    static constexpr std::string_view kCompiler = "cmd1a-test";
};

struct Cmd1aOriginalMixinAxis : ::comdare::cache_engine::concepts::OriginalCodeMixinBase<Cmd1aPaperManifest> {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "cmd1a_test_original_mixin_axis"; }
    [[nodiscard]] static constexpr bool             is_original_module() noexcept { return true; }
};

using AxisCommandSamples =
    mp::mp_list<mp::mp_identity<idx::ClusteredIndexOrganization>, mp::mp_identity<pc::PathCompressionNone>,
                mp::mp_identity<lk::LinearScanSearchAlgo>, mp::mp_identity<ct::LinearFanout>,
                mp::mp_identity<mapping::DirectPlacement>, mp::mp_identity<node::Node4NodeType>,
                mp::mp_identity<layout::SoAMemoryLayout>, mp::mp_identity<alloc::StdMalloc>,
                mp::mp_identity<pref::NonePrefetch>, mp::mp_identity<conc::NoneConcurrency>,
                mp::mp_identity<simd::Amd64Isa>, mp::mp_identity<serial::RawBinarySerialization>,
                mp::mp_identity<tel::InsertCounter>, mp::mp_identity<vh::InlineValueHandle>,
                mp::mp_identity<io::InMemoryOnly>, mp::mp_identity<mig::NoMigration>,
                mp::mp_identity<filt::BloomFilter>, mp::mp_identity<hw::GenericHardwareProfile>,
                mp::mp_identity<simdex::NoSimdExtension>, mp::mp_identity<page::BPlusPageType>,
                mp::mp_identity<bst::BstPtrU32>, mp::mp_identity<btree::BtreeOrderKt4>,
                mp::mp_identity<::comdare::cache_engine::nodes::axis_hash_probe_shape::HashChaining>,
                mp::mp_identity<skip::SkipListMax16P50>, mp::mp_identity<q1::NoBuffer>, mp::mp_identity<q2::EagerFlush>,
                mp::mp_identity<Cmd1aOriginalMixinAxis>>;

struct CountingVisitor {
    int observable_count = 0;

    template <class Axis>
    constexpr void visit_observable() noexcept {
        ++observable_count;
    }
};

template <class Axis>
[[nodiscard]] constexpr int measurement_visit_count() {
    CountingVisitor v{};
    topics::axis_accept_measurement<Axis>(v);
    return v.observable_count;
}

static_assert(an::ObservableAxis<alloc::StdMalloc>);
static_assert(!an::ObservableAxis<tel::InsertCounter>);
static_assert(measurement_visit_count<alloc::StdMalloc>() == 1);
static_assert(measurement_visit_count<tel::InsertCounter>() == 0);

static_assert(topics::AxisLimitations<tel::InsertCounter>::original_module == false);
static_assert(topics::AxisLimitations<tel::InsertCounter>::compiler == std::string_view{"original"});
static_assert(topics::AxisLimitations<alloc::StdMalloc>::observable);
static_assert(topics::AxisLimitations<Cmd1aOriginalMixinAxis>::original_module);
static_assert(topics::AxisLimitations<Cmd1aOriginalMixinAxis>::compiler == std::string_view{"cmd1a-test"});

} // namespace

TEST(Cmd1aAxisCommandBase, AxisBaseDescendantsSatisfyAxisCommand) {
    mp::mp_for_each<AxisCommandSamples>([](auto identity) {
        using T = typename decltype(identity)::type;
        static_assert(topics::AxisCommand<T>);
    });

    SUCCEED();
}

TEST(Cmd1aAxisCommandBase, MeasurementVisitorSlotCountsOnlyObservableAxes) {
    CountingVisitor observable{};
    topics::axis_accept_measurement<alloc::StdMalloc>(observable);
    EXPECT_EQ(observable.observable_count, 1);

    CountingVisitor non_observable{};
    topics::axis_accept_measurement<tel::InsertCounter>(non_observable);
    EXPECT_EQ(non_observable.observable_count, 0);
}

TEST(Cmd1aAxisCommandBase, AxisLimitationsExposeCompileTimeFacts) {
    EXPECT_TRUE(topics::AxisLimitations<alloc::StdMalloc>::observable);
    EXPECT_FALSE(topics::AxisLimitations<tel::InsertCounter>::observable);
    EXPECT_FALSE(topics::AxisLimitations<tel::InsertCounter>::original_module);
    EXPECT_EQ(topics::AxisLimitations<tel::InsertCounter>::compiler, std::string_view{"original"});
    EXPECT_TRUE(topics::AxisLimitations<Cmd1aOriginalMixinAxis>::original_module);
}
