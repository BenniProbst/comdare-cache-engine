// Prospektiver ABI-Transport-POD-Striktheits-Guard (#50-Rest, Goal-V3 Metaprog/ABI-Striktheit).
//
// **Zweck:** zentralisiert die verstreuten `is_trivially_copyable`/`is_standard_layout`-static_asserts der
// 12 ABI-Transport-Snapshot-PODs (heute je Header einzeln) in EINEN mp_all_of-Guard. Prospektiv: jeder
// neue/geänderte Transport-POD, der die C-ABI-Striktheit bricht (nicht-triviales Member: std::string,
// vtable, Pointer-Owner), bricht ab jetzt automatisch den Build/ctest — die Structs queren die DLL-Grenze
// (anatomy_module_abi_v1, extern "C") und MÜSSEN trivially_copyable + standard_layout bleiben.
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner static_assert-Test über Bestandstypen; keine Änderung an den
// PODs/ABI/golden. Leichte anatomy-Header (kein topic_config_set) → schnell, kein Cold-Cache-ICE-Risiko.

#include <anatomy/adapter_tier.hpp>               // AdapterObserverSnapshotV1
#include <anatomy/allocator_proxy_tier.hpp>       // ComdareAllocatorProxyV1
#include <anatomy/build_variant_definition.hpp>   // BuildVariantDefinitionV1
#include <anatomy/measurable_workload.hpp>        // ComdareSegmentLatencyV1/V2
#include <anatomy/memento_aggregate.hpp>          // EmptyMemento
#include <anatomy/observable_tier.hpp>            // ComdareTierObserverSnapshot
#include <anatomy/observer_aggregate.hpp>         // EmptyAxisSnapshot
#include <anatomy/resource_controllable_tier.hpp> // ComdareResourceControlV1
#include <anatomy/sequence_tier.hpp>              // SequenceObserverSnapshotV1
#include <anatomy/set_tier.hpp>                   // SetObserverSnapshotV1
#include <anatomy/view_tier.hpp>                  // ViewObserverSnapshotV1

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

namespace cea = ::comdare::cache_engine::anatomy;
namespace mp  = ::boost::mp11;

// Die 12 ABI-Transport-Snapshot-PODs, die über die DLL-/extern-"C"-Grenze queren.
using AbiTransportPods =
    mp::mp_list<cea::ComdareSegmentLatencyV1, cea::ComdareSegmentLatencyV2, cea::ComdareAllocatorProxyV1,
                cea::EmptyMemento, cea::EmptyAxisSnapshot, cea::AdapterObserverSnapshotV1,
                cea::ComdareTierObserverSnapshot, cea::ComdareResourceControlV1, cea::SequenceObserverSnapshotV1,
                cea::ViewObserverSnapshotV1, cea::SetObserverSnapshotV1, cea::BuildVariantDefinitionV1>;

// is_abi_transport_pod<T> == true gdw. T binär-transportsicher ist (C-ABI: trivially_copyable + standard_layout).
template <class Pod>
struct is_abi_transport_pod : std::bool_constant<std::is_trivially_copyable_v<Pod> && std::is_standard_layout_v<Pod>> {
};

static_assert(mp::mp_size<AbiTransportPods>::value == 12, "12 ABI-Transport-Snapshot-PODs erfasst.");
static_assert(mp::mp_all_of<AbiTransportPods, is_abi_transport_pod>::value,
              "ABI-Striktheit (Organ-Snapshot-Transport): ALLE 12 Snapshot-PODs müssen trivially_copyable "
              "+ standard_layout sein (C-ABI-sicher über die anatomy_module_abi_v1-DLL-Grenze). Ein "
              "nicht-triviales Member (std::string/vtable/Owner-Pointer) bräche den binären Transport.");

TEST(AbiTransportPodGuard, AllTwelveSnapshotPodsAreTriviallyCopyableStandardLayout) {
    std::size_t count = 0;
    bool        all   = true;
    mp::mp_for_each<AbiTransportPods>([&]<class Pod>(Pod) {
        ++count;
        if (!(std::is_trivially_copyable_v<Pod> && std::is_standard_layout_v<Pod>)) { all = false; }
    });
    EXPECT_EQ(count, 12u);
    EXPECT_TRUE(all);
}
