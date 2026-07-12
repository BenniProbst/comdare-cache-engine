// Prospektiver Metaprogrammierungs-Striktheits-Guard, Teil 4 (GO-3 A2, Task #5 Hebel-A-Rest, 2026-07-12):
// MESS-KERN-REINHEIT der *_scan-Kerne (Interface-Freeze).
//
// **Zweck:** die 9 *_scan-Kernel-Familien der do_seg19-Mess-Segmente (abi_adapter.hpp T3..T16) sind BEWUSST
// skalare, strided Mess-Kerne — ihr ZUGRIFFSMUSTER ist das Achsen-Signal (LAYOUT-FIX X-§4: kRecordSize=48
// differenziert aos_strict(48) vs cache_line_aligned(64); Doc 21 §F Meta-Befund). Ein "Routing ueber isa"
// (Isa-Parameter oder Template-Isa in den Kernen) wuerde das Signal korrumpieren: Muster-Kollaps,
// Attributions-Bruch (die isa-Achse hat ihr EIGENES Segment T12), Zaehler-Desynchronisation der Observable-
// Snapshots, Ergebnis-Drift der last_checksum-Anker (Dossier GO3 §1.3). Dieser Guard friert das Kernel-
// Interface compile-time ein:
//   (i)  POSITIV: jede Strategie traegt die kanonische Kernel-Signatur (static, noexcept, uint64_t) —
//        exakt die do_seg19-Aufrufform;
//   (ii) NEGATIV: KEINE Isa-parametrisierte Form existiert — weder S::kernel<Isa>(...) (Templatisierung)
//        noch S::kernel(..., Isa) (Signatur-Erweiterung) — die beiden realistischen Einspeisungs-Routen
//        (Dossier GO3, Option 3). Route (c), hart eingebundene Plattform-Isa im Kernel-Body, ist per
//        Interface unsichtbar — dokumentiert am do_seg19-Kopf (abi_adapter.hpp) und in Doc 21 §F.
// Prospektiv build-brechend: jede kuenftige Isa-Einspeisung in die Mess-Kerne bricht ab jetzt Build/ctest —
// die Entscheidung ist ERZWUNGEN, nicht nur dokumentiert. SIMD-Verhaltens-Primitive gehoeren in die
// isa-Achse selbst (group_match_mask-Muster 7b-3; simd_field_sum ist der isa-EIGENE T12-Kern, kein Mess-
// Kern dieser Familie und daher hier bewusst NICHT eingefroren).
//
// **Leichte Header (bewusst, Cold-Cache-ICE-Lektion #21/K87b):** nur die 9 Achsen-Registries +
// axis_09_isa_amd64.hpp — KEINE topic_config_sets/registry_to_axis_levels.hpp (die zieht
// test_striktheit_axes_guard separat und isoliert retriggerbar).
//
// **ADDITIV & golden/ABI-NEUTRAL:** reiner requires/static_assert-Test ueber Bestandstypen. KEINE Aenderung
// an Kernen/Registries/golden/permutation_axes/ABI.

#include <axes/filter_axis/axis_filter_registry.hpp>
#include <axes/index_organization/axis_01_index_organization_registry.hpp>
#include <axes/io_dispatch/axis_io_registry.hpp>
#include <axes/layout/axis_05_memory_layout_registry.hpp>
#include <axes/migration_policy/axis_migration_registry.hpp>
#include <axes/node/axis_04_node_type_registry.hpp>
#include <axes/path_compression/axis_02_path_compression_registry.hpp>
#include <axes/serialization_axis/axis_10_serialization_registry.hpp>
#include <axes/simd/axis_09_isa_amd64.hpp>
#include <axes/value_handle_axis/axis_14_value_handle_registry.hpp>

#include <boost/mp11.hpp>

#include <gtest/gtest.h>

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace mp = ::boost::mp11;

namespace {

// Isa-Probe-Typ fuer die NEGATIV-Formen: die reale Achsen-Strategie, mit der ein kuenftiges "Routing ueber
// isa" realistischerweise eingespeist wuerde (C::isa der Composition ist auf x86 Amd64Isa).
using IsaProbe = ::comdare::cache_engine::simd::Amd64Isa;

// Kanonische BUFFER-Kernel-Form: static uint64_t kernel(unsigned char const*, size_t records, size_t
// record_size) noexcept — exakt die do_seg19-Aufrufform (abi_adapter.hpp). Je Kernel-Familie entstehen:
// has_canonical_<kernel> (POSITIV), has_isa_routed_<kernel> (NEGATIV) und <kernel>_is_pure (mp_all_of-Form).
#define COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(KERNEL)                                                                \
    template <class S>                                                                                                 \
    inline constexpr bool has_canonical_##KERNEL = requires(unsigned char const* b, std::size_t n, std::size_t rs) {   \
        { S::KERNEL(b, n, rs) } noexcept -> std::same_as<std::uint64_t>;                                               \
    };                                                                                                                 \
    template <class S>                                                                                                 \
    inline constexpr bool has_isa_routed_##KERNEL = requires(unsigned char const* b, std::size_t n, std::size_t rs) {  \
        S::template KERNEL<IsaProbe>(b, n, rs);                                                                        \
    } || requires(unsigned char const* b, std::size_t n, std::size_t rs, IsaProbe isa) { S::KERNEL(b, n, rs, isa); };  \
    template <class S>                                                                                                 \
    using KERNEL##_is_pure = mp::mp_bool<has_canonical_##KERNEL<S> && !has_isa_routed_##KERNEL<S>>;

// Kanonische QUERY-Kernel-Form: static uint64_t kernel(Byte const* stored, size_t n, Byte const* queries,
// size_t q) noexcept (T4 node_find_scan: uint8_t; T16 filter_probe_scan: unsigned char).
#define COMDARE_AP5_DEFINE_QUERY_KERNEL_PURITY(KERNEL, BYTE_T)                                                         \
    template <class S>                                                                                                 \
    inline constexpr bool has_canonical_##KERNEL =                                                                     \
        requires(BYTE_T const* b, std::size_t n, BYTE_T const* q, std::size_t qn) {                                    \
            { S::KERNEL(b, n, q, qn) } noexcept -> std::same_as<std::uint64_t>;                                        \
        };                                                                                                             \
    template <class S>                                                                                                 \
    inline constexpr bool has_isa_routed_##KERNEL =                                                                    \
        requires(BYTE_T const* b, std::size_t n, BYTE_T const* q, std::size_t qn) {                                    \
            S::template KERNEL<IsaProbe>(b, n, q, qn);                                                                 \
        } || requires(BYTE_T const* b, std::size_t n, BYTE_T const* q, std::size_t qn, IsaProbe isa) {                 \
            S::KERNEL(b, n, q, qn, isa);                                                                               \
        };                                                                                                             \
    template <class S>                                                                                                 \
    using KERNEL##_is_pure = mp::mp_bool<has_canonical_##KERNEL<S> && !has_isa_routed_##KERNEL<S>>;

COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(scan_field_sum)                  // T5/T7 memory_layout (+ Prefetch-Re-Scan)
COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(serialize_scan)                  // T9 serialization
COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(value_access_scan)               // T11 value_handle
COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(index_org_scan)                  // T13 index_organization
COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(io_dispatch_scan)                // T14 io_dispatch
COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(migration_decide_scan)           // T15 migration_policy
COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY(path_descend_scan)               // T3 path_compression (nur Patricia, s.u.)
COMDARE_AP5_DEFINE_QUERY_KERNEL_PURITY(node_find_scan, std::uint8_t)     // T4 node_type
COMDARE_AP5_DEFINE_QUERY_KERNEL_PURITY(filter_probe_scan, unsigned char) // T16 filter

#undef COMDARE_AP5_DEFINE_BUFFER_KERNEL_PURITY
#undef COMDARE_AP5_DEFINE_QUERY_KERNEL_PURITY

// T3-Sonderfall: path_descend_scan ist KEINE Pflicht-API der Achse (do_seg19 detektiert per if-constexpr;
// None/ByteWise treiben das ByteWiseKeyPrefix-Organ). Freeze daher: (i) Patricia HAT die kanonische Form,
// (ii) KEINE Strategie der Achse traegt eine Isa-Form.
template <class S>
using path_descend_scan_is_isa_free = mp::mp_bool<!has_isa_routed_path_descend_scan<S>>;

} // namespace

// ── Die 9 Familien, compile-time eingefroren (Registry-Enabled-Listen = alle aktiven Strategien) ────────────
namespace fam {
namespace pc  = ::comdare::cache_engine::path_compression;
namespace nd  = ::comdare::cache_engine::node;
namespace ly  = ::comdare::cache_engine::layout;
namespace sr  = ::comdare::cache_engine::serialization_axis;
namespace vh  = ::comdare::cache_engine::value_handle_axis;
namespace ix  = ::comdare::cache_engine::index_organization;
namespace io  = ::comdare::cache_engine::io_dispatch;
namespace mg  = ::comdare::cache_engine::migration_policy;
namespace flt = ::comdare::cache_engine::filter_axis;
} // namespace fam

static_assert(mp::mp_all_of<fam::ly::EnabledLayouts, scan_field_sum_is_pure>::value,
              "Mess-Kern-Reinheit T5/T7: scan_field_sum muss die kanonische skalare Signatur tragen und darf "
              "NICHT ueber isa geroutet werden (Zugriffsmuster == Achsen-Signal; Doc 21 §F).");
static_assert(mp::mp_all_of<fam::sr::EnabledSerializers, serialize_scan_is_pure>::value,
              "Mess-Kern-Reinheit T9: serialize_scan muss die kanonische skalare Signatur tragen und darf "
              "NICHT ueber isa geroutet werden (CPU-Kosten-ORDNUNG raw<compressed<var_len<succinct).");
static_assert(mp::mp_all_of<fam::vh::EnabledHandles, value_access_scan_is_pure>::value,
              "Mess-Kern-Reinheit T11: value_access_scan muss die kanonische skalare Signatur tragen und darf "
              "NICHT ueber isa geroutet werden.");
static_assert(mp::mp_all_of<fam::ix::EnabledOrganizations, index_org_scan_is_pure>::value,
              "Mess-Kern-Reinheit T13: index_org_scan muss die kanonische skalare Signatur tragen und darf "
              "NICHT ueber isa geroutet werden.");
static_assert(mp::mp_all_of<fam::io::EnabledIos, io_dispatch_scan_is_pure>::value,
              "Mess-Kern-Reinheit T14: io_dispatch_scan muss die kanonische skalare Signatur tragen und darf "
              "NICHT ueber isa geroutet werden.");
static_assert(mp::mp_all_of<fam::mg::EnabledMigrations, migration_decide_scan_is_pure>::value,
              "Mess-Kern-Reinheit T15: migration_decide_scan muss die kanonische skalare Signatur tragen und "
              "darf NICHT ueber isa geroutet werden.");
static_assert(mp::mp_all_of<fam::nd::EnabledNodeTypes, node_find_scan_is_pure>::value,
              "Mess-Kern-Reinheit T4: node_find_scan (KF-6 Pflicht-API) muss die kanonische skalare Signatur "
              "tragen und darf NICHT ueber isa geroutet werden (order-sensitiver ART-Probe-Scan).");
static_assert(mp::mp_all_of<fam::flt::EnabledFilters, filter_probe_scan_is_pure>::value,
              "Mess-Kern-Reinheit T16: filter_probe_scan muss die kanonische skalare Signatur tragen und darf "
              "NICHT ueber isa geroutet werden (Filter-Proben-Kostenkurve).");
static_assert(has_canonical_path_descend_scan<fam::pc::PatriciaPathCompression>,
              "Mess-Kern-Reinheit T3: PatriciaPathCompression muss den kanonischen path_descend_scan tragen "
              "(Single-Bit-Descent — das strategie-charakteristische Mess-Organ).");
static_assert(mp::mp_all_of<fam::pc::EnabledCompressions, path_descend_scan_is_isa_free>::value,
              "Mess-Kern-Reinheit T3: KEINE path_compression-Strategie darf einen isa-gerouteten "
              "path_descend_scan tragen.");

// Nicht-leere Registry-Listen (sonst waeren die mp_all_of-Asserts vakuum-wahr).
static_assert(mp::mp_size<fam::ly::EnabledLayouts>::value > 0 && mp::mp_size<fam::sr::EnabledSerializers>::value > 0 &&
                  mp::mp_size<fam::vh::EnabledHandles>::value > 0 &&
                  mp::mp_size<fam::ix::EnabledOrganizations>::value > 0 &&
                  mp::mp_size<fam::io::EnabledIos>::value > 0 && mp::mp_size<fam::mg::EnabledMigrations>::value > 0 &&
                  mp::mp_size<fam::nd::EnabledNodeTypes>::value > 0 &&
                  mp::mp_size<fam::flt::EnabledFilters>::value > 0 &&
                  mp::mp_size<fam::pc::EnabledCompressions>::value > 0,
              "Mess-Kern-Reinheit: keine der 9 Kernel-Familien-Registries darf leer sein.");

TEST(ScanKernelPurityGuard, AllNineScanKernelFamiliesAreScalarInterfaceFrozen) {
    // Laufzeit-Spiegel der compile-time-Beweise (sichtbarer ctest-PASS + Zaehlung der eingefrorenen Familien).
    std::size_t const strategy_count =
        mp::mp_size<fam::ly::EnabledLayouts>::value + mp::mp_size<fam::sr::EnabledSerializers>::value +
        mp::mp_size<fam::vh::EnabledHandles>::value + mp::mp_size<fam::ix::EnabledOrganizations>::value +
        mp::mp_size<fam::io::EnabledIos>::value + mp::mp_size<fam::mg::EnabledMigrations>::value +
        mp::mp_size<fam::nd::EnabledNodeTypes>::value + mp::mp_size<fam::flt::EnabledFilters>::value +
        mp::mp_size<fam::pc::EnabledCompressions>::value;
    EXPECT_GT(strategy_count, 0u); // die 9 Familien sind nicht leer
    EXPECT_TRUE((mp::mp_all_of<fam::ly::EnabledLayouts, scan_field_sum_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::sr::EnabledSerializers, serialize_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::vh::EnabledHandles, value_access_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::ix::EnabledOrganizations, index_org_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::io::EnabledIos, io_dispatch_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::mg::EnabledMigrations, migration_decide_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::nd::EnabledNodeTypes, node_find_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::flt::EnabledFilters, filter_probe_scan_is_pure>::value));
    EXPECT_TRUE((mp::mp_all_of<fam::pc::EnabledCompressions, path_descend_scan_is_isa_free>::value));
    EXPECT_TRUE((has_canonical_path_descend_scan<fam::pc::PatriciaPathCompression>));
}
