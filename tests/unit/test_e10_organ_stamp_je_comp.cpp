// tests/unit/test_e10_organ_stamp_je_comp.cpp -- E-10/#38a2 SCHRITT-4-GATE (k9-KOEDER, T-1 rot-zuerst;
// Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19.md Schritt 4 + VERIFY-Fix FIX-1 (iv), 27.08.2026).
//
// GEGENSTAND: der Organ-Meta-Meta-Klammer-Anhang wird JE COMP gewaehlt -- an BEIDEN Zeilen-Quellen (Mock-Pfad
// abi::organ_stamp_line<Comp>, realer Emitter-Pfad ex::compose_organ_stamp_line aus der Versions-Tabelle).
// MemoryOnly-Compositions (die gesamte Flotte all_axes_golden) bleiben BYTE-IDENTISCH zur Vor-Zug-Form; ein
// IO-Traeger (persistence_target = DiskWritebackTarget) bekommt ";[disk_io=code@1.0.0.c]" (Form des gelandeten
// System-Vorbilds "[simd=code@1.0.0.c]").
//
// T-1/K13: im S1-S4-Korridor (abi::OrganMetaMetas TYP-GLOBAL an beiden Quellen, R-10) traegt JEDE Binary den
// Anhang -> die Flotten-Pins unten sind ROT (Literal im Beweisort bau/t1-rot/); nach 4a/4b GRUEN.
//
// OBJEKT-ENTSCHEID S4-1 (STAND-BAU): organ_stamp_line<Comp> verlangt name()/algo_version an allen 18 Slots
// (GELTUNGSBEREICH-Absatz des Headers). Die prt-art-Strategie-Typen der PrtArtComposition tragen beides NICHT
// (Vorzug-Probe bau/s4-vorzug: "'name' is not a member of ... ObservableComposedContainer<...>"). Die Stempel-
// Pins fahren darum am REGISTRY-Comp der ersten enabled Wrapper (== golden-320 id #0); PrtArtComposition wird
// auf TRAIT-Ebene gepinnt (organ_meta_metas_of_t leer -> Renderer liefert "" -> kein Anhang).
//
// OBJEKT-ENTSCHEID S4-2 (STAND-BAU): die 4a-Vollmengen-Wache an der Stempel-Stelle weist jede Comp mit test-
// lokalen Meta-Metas ab -- ihr Zweck. Die FIX-1-(iv)-Zwei-Traeger-Probe faehrt darum am RENDERER (dieselbe
// Grammatik-Quelle) gegen den REALEN Pfad und pinnt die Ein-Gruppen-Form "[a=..;b=..]" literal.

#include <gtest/gtest.h>

#include <builder/experiment_tree/axis_variant_version_table.hpp> // Tabelle, reflect_versions, compose_organ_stamp_line
#include <builder/experiment_tree/ceb_generator.hpp>              // ceb_parse_path
#include <cache_engine/abi/anatomy_fingerprint.hpp>               // kAnatomyFingerprintOrganMax (Budget-Reihe)
#include <cache_engine/abi/anatomy_version_stamp.hpp>             // organ_stamp_line<Comp>, OrganMetaMetas
#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>            // meta_meta_stamp_suffix_from_members, append
#include <cache_engine/abi/organ_meta_meta_selection.hpp>         // organ_meta_metas_of_t + Wachen-Traits
#include <cache_engine/measurement/axis_version_stamp.hpp>        // AxisVersionEntry, build_axis_version_stamp_line
#include <compositions/prt_art_reference.hpp>                     // PrtArtComposition (18 Slot-Aliase, MemoryOnly)
#include <organ_axes/organ_meta_meta/axis_disk_io_organ_meta_meta.hpp>
#include <organ_axes/persistence_target/axis_persistence_target_registry.hpp> // AllTargets (RAW-Typen)
#include <topics/organ_meta_meta_axis.hpp>

#include <boost/mp11.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

namespace abi  = ::comdare::cache_engine::abi;
namespace cet  = ::comdare::cache_engine::topics;
namespace ex   = ::comdare::cache_engine::builder::experiment;
namespace meas = ::comdare::cache_engine::measurement;
namespace mp   = boost::mp11;
namespace omm  = ::comdare::cache_engine::organ_meta_meta;
namespace pt   = ::comdare::cache_engine::persistence_target;
using ::comdare::cache_engine::compositions::PrtArtComposition;

// -- FLOTTEN-COMP: die ERSTEN enabled Registry-Wrapper jeder der 18 Kompositions-Achsen (== golden-320 id #0,
// == die all_axes_golden-Binary mit allen ersten Werten). Registry-Wrapper tragen name()/algo_version.
struct FlottenComp {
    using search_algo        = mp::mp_first<ex::axes26::T00_search_algo>;
    using cache_traversal    = mp::mp_first<ex::axes26::T01_cache_traversal>;
    using mapping            = mp::mp_first<ex::axes26::T02_mapping>;
    using path_compression   = mp::mp_first<ex::axes26::T03_path_compression>;
    using node_type          = mp::mp_first<ex::axes26::T04_node_type>;
    using memory_layout      = mp::mp_first<ex::axes26::T05_memory_layout>;
    using allocator          = mp::mp_first<ex::axes26::T06_allocator>;
    using prefetch           = mp::mp_first<ex::axes26::T07_prefetch>;
    using concurrency        = mp::mp_first<ex::axes26::T08_concurrency>;
    using serialization      = mp::mp_first<ex::axes26::T09_serialization>;
    using value_handle       = mp::mp_first<ex::axes26::T11_value_handle>;
    using index_organization = mp::mp_first<ex::axes26::T13_index_organization>;
    using io_dispatch        = mp::mp_first<ex::axes26::T14_io_dispatch>;
    using migration_policy   = mp::mp_first<ex::axes26::T15_migration_policy>;
    using filter             = mp::mp_first<ex::axes26::T16_filter>;
    using queuing_q1         = mp::mp_first<ex::axes26::T20_queuing_q1>;
    using queuing_q2         = mp::mp_first<ex::axes26::T21_queuing_q2>;
    using persistence_target = mp::mp_first<ex::axes26::T26_persistence_target>;
};
static_assert(std::is_same_v<FlottenComp::persistence_target, pt::MemoryOnlyTarget>,
              "Q-1 Fall B: der erste (und einzige) enabled persistence_target-Baustein ist MemoryOnlyTarget");

// -- IO-COMP: dieselben Slots, persistence_target = DiskWritebackTarget (der Typ existiert vollstaendig, ist aber
// enabled=false -- genau dafuer ist die Selektion da: ein spaeteres option() stempelt ohne Emitter-Edit).
struct DiskComp : FlottenComp {
    using persistence_target = pt::DiskWritebackTarget;
};
// Der REGISTRY-Typ des Disk-Bausteins (AllTargets[1], RAW-Typ ohne Observable-Huelle): reflect_versions sieht
// in der Produktion GENAU diesen Typ, sobald option() ihn in EnabledTargets hebt.
using DiskRegistryTyp = mp::mp_at_c<pt::AllTargets, 1>;
static_assert(std::is_same_v<DiskRegistryTyp, pt::DiskWritebackTarget>);
static_assert(!DiskRegistryTyp::enabled, "Q-1 Fall B (heute): DiskWritebackTarget ist NICHT enabled");

// -- TRAIT-PINS (kompilieren): Flotte ohne Traeger, IO-Comp mit genau ORG-19-IO, prt-art ohne Traeger -----------
static_assert(abi::organ_meta_metas_of_t<FlottenComp>::size == 0, "Flotte: KEIN Meta-Meta-Traeger (N-1)");
static_assert(abi::organ_meta_metas_of_t<DiskComp>::size == 1);
static_assert(abi::organ_meta_metas_of_t<DiskComp>::contains<omm::DiskIoOrganMetaMeta>);
static_assert(abi::organ_meta_metas_of_t<PrtArtComposition>::size == 0,
              "PrtArtComposition (MemoryOnly): kein Traeger -> kein Anhang (prt-art-Vertrag 18 Aliase, K-15)");
static_assert(abi::organ_meta_meta_subsumiert<abi::OrganMetaMetas, abi::organ_meta_metas_of_t<DiskComp>>::value);
static_assert(abi::organ_meta_metas_sind_duplikatfrei<abi::organ_meta_metas_of_t<DiskComp>>::value);

// -- LITERALE (Vor-Zug-Form @ d3b5a393, bau/s4-vorzug/probe_flotte_hk.out; K-03 Literal-Pin der Flotten-Form) --
// 17 Segmente + Trenner; das 18. Segment (persistence_target) folgt je Comp.
constexpr char             kOrgan17[]  = "search_algo=k_ary@1.0.0.c;"
                                         "cache_traversal=linear_fanout@1.0.0.c;"
                                         "mapping=direct_placement@1.0.0.c;"
                                         "path_compression=path_compression_none@1.0.0.c;"
                                         "node_type=node4@1.0.0.c;"
                                         "memory_layout=memory_layout_cache_line_aligned@1.0.0.c;"
                                         "allocator=std_malloc@1.0.2.c;"
                                         "prefetch=prefetch_none@1.0.0.c;"
                                         "concurrency=concurrency_none@1.0.0.c;"
                                         "serialization=serialization_raw_binary@1.0.0.c;"
                                         "value_handle=value_handle_inline@1.0.0.c;"
                                         "index_organization=index_org_heap@1.0.0.c;"
                                         "io_dispatch=io_in_memory_only@1.0.0.c;"
                                         "migration_policy=migration_none@1.0.0.c;"
                                         "filter=filter_bloom@1.0.0.c;"
                                         "queuing_q1=no_buffer@1.0.0.c;"
                                         "queuing_q2=eager_flush@1.0.0.c;";
constexpr std::string_view kFlotteTail = "persistence_target=persistence_memory_only@1.0.0.c";
constexpr std::string_view kDiskTail   = "persistence_target=persistence_disk_writeback@1.0.0.c;[disk_io=code@1.0.0.c]";
constexpr std::string_view kDiskAnhang = ";[disk_io=code@1.0.0.c]";
// golden-320 id #0 (tests/unit/thesis_tiere/golden_fullpilot_320_binary_ids.txt, 18 Segmente):
constexpr char kFlotteBinaryId[] = "search_algo=k_ary/cache_traversal=linear_fanout/mapping=direct_placement/"
                                   "path_compression=path_compression_none/node_type=node4/"
                                   "memory_layout=memory_layout_cache_line_aligned/allocator=std_malloc/"
                                   "prefetch=prefetch_none/concurrency=concurrency_none/"
                                   "serialization=serialization_raw_binary/value_handle=value_handle_inline/"
                                   "index_organization=index_org_heap/io_dispatch=io_in_memory_only/"
                                   "migration_policy=migration_none/filter=filter_bloom/queuing_q1=no_buffer/"
                                   "queuing_q2=eager_flush/persistence_target=persistence_memory_only";

[[nodiscard]] std::string flotte_vorzug_form() { return std::string{kOrgan17} + std::string{kFlotteTail}; }
[[nodiscard]] std::string disk_form() { return std::string{kOrgan17} + std::string{kDiskTail}; }

[[nodiscard]] std::size_t count_char(std::string const& s, char c) {
    return static_cast<std::size_t>(std::count(s.begin(), s.end(), c));
}

/// Die (achse,wert)-Paare einer Comp ueber DENSELBEN Weg wie der Emitter (binary_id -> ceb_parse_path).
template <class Comp>
[[nodiscard]] std::vector<std::pair<std::string, std::string>> axes_of() {
    return ex::ceb_parse_path(ex::serialize_composition_from_slots<Comp>());
}

/// Produktions-Tabelle + der Disk-Baustein ueber DENSELBEN reflect_versions-Weg (assert_version_grammar +
/// organ_meta_meta_entries_for_variant) -- test-lokal das, was option() in der Produktion bewirkt.
[[nodiscard]] std::vector<ex::AxisVariantVersion> tabelle_mit_disk(std::vector<ex::AxisVariantVersion> table) {
    ex::reflect_versions<mp::mp_list<DiskRegistryTyp>>("persistence_target", table);
    return table;
}

// -- (A) FLOTTE, MOCK-PFAD: byte-identisch zur Vor-Zug-Form, KEIN Klammer-Anhang (N-1, Koeder k9 i) ----------
TEST(E10OrganStampJeComp, FlotteMockPfadByteIdentischZurVorZugForm) {
    std::string const line = abi::organ_stamp_line<FlottenComp>();
    EXPECT_EQ(line, flotte_vorzug_form());
    EXPECT_EQ(line.size(), 666u) << "Vor-Zug-Laenge der golden-320-id-#0-Organ-Zeile (probe_flotte_hk.out)";
    EXPECT_EQ(line.find('['), std::string::npos) << "MemoryOnly: KEIN Klammer-Anhang. line=" << line;
    EXPECT_EQ(count_char(line, ';'), 17u) << "18 Slots -> exakt 17 Trenner (test_m_w12-Anker)";
    EXPECT_TRUE(line.ends_with(kFlotteTail)) << line;
    EXPECT_EQ(ex::serialize_composition_from_slots<FlottenComp>(), std::string{kFlotteBinaryId})
        << "FlottenComp == golden-320 id #0 (18 Segmente)";
}

// -- (B) prt-art: Trait leer -> der EINE Renderer liefert den leeren Anhang (kein "[]") ------------------------
TEST(E10OrganStampJeComp, PrtArtTraitLeerRenderertKeinenAnhang) {
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from_members<abi::organ_meta_metas_of_t<PrtArtComposition>>(), "");
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from_members<abi::organ_meta_metas_of_t<FlottenComp>>(), "");
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from_members<abi::organ_meta_metas_of_t<DiskComp>>(),
              "[disk_io=code@1.0.0.c]");
}

// -- (C) IO-COMP, MOCK-PFAD: endet auf ";[disk_io=code@1.0.0.c]" (Koeder k9 ii) ------------------------------
TEST(E10OrganStampJeComp, IoTraegerBekommtGenauDenDiskIoAnhang) {
    std::string const line = abi::organ_stamp_line<DiskComp>();
    EXPECT_TRUE(line.ends_with(kDiskAnhang)) << line;
    EXPECT_EQ(line, disk_form());
    EXPECT_EQ(line.size(), 692u) << "666 - 23 (memory_only) + 26 (disk_writeback) + 23 (Anhang)";
    EXPECT_EQ(count_char(line, '['), 1u);
    EXPECT_EQ(count_char(line, ']'), 1u);
    EXPECT_EQ(count_char(line, ';'), 18u) << "18 Slots + 1 Klammer-Anhang -> 18 Trenner";
    // Der 18-Slot-Praefix ist bis auf den persistence_target-WERT die Flotten-Zeile (kein zweiter Effekt).
    EXPECT_EQ(line.substr(0, sizeof(kOrgan17) - 1), std::string{kOrgan17});
}

// -- (D) PFAD-GLEICHHEIT: Mock-Pfad == realer Emitter-Pfad (Tabelle), Flotte UND IO-Comp (Koeder k9 iii) -------
TEST(E10OrganStampJeComp, MockPfadUndEmitterPfadLiefernDenselbenString) {
    auto const        table       = ex::build_axis_variant_version_table();
    std::string const real_flotte = ex::compose_organ_stamp_line(axes_of<FlottenComp>(), table);
    EXPECT_EQ(real_flotte, abi::organ_stamp_line<FlottenComp>()) << "Byte-Regel 4b: BEIDE Quellen DENSELBEN String";
    EXPECT_EQ(real_flotte, flotte_vorzug_form());
    EXPECT_EQ(real_flotte.find('['), std::string::npos) << real_flotte;
    // Q-1 Fall B: die Produktions-Tabelle traegt KEINEN Disk-Eintrag -> Sentinel-Version UND kein Anhang (es gibt
    // keinen registrierten Traeger, also nichts zu stempeln -- kein Raten).
    std::string const real_disk_ohne = ex::compose_organ_stamp_line(axes_of<DiskComp>(), table);
    EXPECT_TRUE(real_disk_ohne.ends_with("persistence_target=persistence_disk_writeback@0.0.0")) << real_disk_ohne;
    EXPECT_EQ(real_disk_ohne.find('['), std::string::npos);
    // Tabelle test-lokal um den REGISTRY-Typ erweitert -> der Suffix erscheint automatisch (Plan 1d/4b).
    auto const table_disk = tabelle_mit_disk(table);
    EXPECT_EQ(table_disk.size(), table.size() + 1u);
    EXPECT_EQ(ex::lookup_meta_meta_entries(table_disk, "persistence_target", "persistence_disk_writeback"),
              "disk_io=code@1.0.0.c")
        << "FIX-1 (i): die Tabelle traegt die UNGEWRAPPTE Entry-Form";
    EXPECT_EQ(ex::lookup_meta_meta_entries(table_disk, "persistence_target", "persistence_memory_only"), "");
    std::string const real_disk = ex::compose_organ_stamp_line(axes_of<DiskComp>(), table_disk);
    EXPECT_EQ(real_disk, abi::organ_stamp_line<DiskComp>());
    EXPECT_EQ(real_disk, disk_form());
    EXPECT_TRUE(real_disk.ends_with(kDiskAnhang)) << real_disk;
    // Die Flotte bleibt auch MIT registriertem Disk-Baustein anhangfrei (Selektion per Wert, nicht per Tabelle).
    EXPECT_EQ(ex::compose_organ_stamp_line(axes_of<FlottenComp>(), table_disk), flotte_vorzug_form());
}

// -- (E) LAENGE: Flotten-Worst-Case (laengster all_axes_golden-Wert je permute-Achse, Plan a.5/L-7) im Budget --
TEST(E10OrganStampJeComp, FlottenWorstCaseBleibtImOrganBudget) {
    auto const                                             table = ex::build_axis_variant_version_table();
    std::vector<std::pair<std::string, std::string>> const worst{{"search_algo", "interpolation"},
                                                                 {"cache_traversal", "linear_fanout"},
                                                                 {"mapping", "direct_placement"},
                                                                 {"path_compression", "path_compression_patricia"},
                                                                 {"node_type", "node16"},
                                                                 {"memory_layout", "memory_layout_cache_line_aligned"},
                                                                 {"allocator", "pmr_resource"},
                                                                 {"prefetch", "prefetch_distance_estimator"},
                                                                 {"concurrency", "concurrency_blocking"},
                                                                 {"serialization", "serialization_raw_binary"},
                                                                 {"value_handle", "value_handle_external_pool"},
                                                                 {"index_organization", "index_org_clustered"},
                                                                 {"io_dispatch", "io_in_memory_only"},
                                                                 {"migration_policy", "migration_hot_cold"},
                                                                 {"filter", "filter_cuckoo"},
                                                                 {"queuing_q1", "fifo_queue"},
                                                                 {"queuing_q2", "watermark_flush"},
                                                                 {"persistence_target", "persistence_memory_only"}};
    for (auto const& [a, v] : worst)
        EXPECT_FALSE(ex::lookup_algo_version(table, a, v).empty())
            << "Worst-Case-Wert nicht registriert: " << a << "=" << v;
    std::string const w = ex::compose_organ_stamp_line(worst, table);
    EXPECT_EQ(w.size(), 721u) << "Flotten-Worst-Case OHNE Anhang (a.5 BUDGET, L-7 gemessen)";
    EXPECT_EQ(w.find('['), std::string::npos);
    auto worst_disk          = worst;
    worst_disk.back().second = "persistence_disk_writeback";
    std::string const wd     = ex::compose_organ_stamp_line(worst_disk, tabelle_mit_disk(table));
    EXPECT_TRUE(wd.ends_with(kDiskAnhang)) << wd;
    EXPECT_EQ(wd.size(), 747u) << "721 - 23 + 26 + 23: der IO-Worst-Case der Flotten-Achsenwerte";
    EXPECT_LE(wd.size(), abi::kAnatomyFingerprintOrganMax) << "Organ-Budget der Budget-Reihe (R-4, kein Ereignis)";
    EXPECT_EQ(abi::kAnatomyFingerprintOrganMax, 768u);
}

// -- (F) FIX-1 (iv): ZWEI-TRAEGER-COMP -> Ein-Gruppen-Form "[a=..;b=..]", Mock-Renderer == realer Pfad ---------
// Zwei test-lokale Meta-Metas an zwei test-lokalen Strategie-Typen (Trait-Spezialisierung = der Bindungs-
// Kundenpunkt aus 1c). MockAxis traegt name()/algo_version fuer die uebrigen 16 Slots.
struct ProbeMmA final : cet::OrganMetaMetaAxis<ProbeMmA> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "probe_mm_a"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "probe_mm_a"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
};
struct ProbeMmB final : cet::OrganMetaMetaAxis<ProbeMmB> {
    [[nodiscard]] static constexpr std::string_view do_axis_label() noexcept { return "probe_mm_b"; }
    [[nodiscard]] static constexpr std::string_view sub_axis_label() noexcept { return "probe_mm_b"; }
    static constexpr std::string_view               axis_code_version = "1.0.0.c";
};
struct ProbeStratA {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "probe_strat_a"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};
struct ProbeStratB {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "probe_strat_b"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};
struct MockAxis {
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "algoA"; }
    static constexpr std::string_view               algo_version = "1.0.0.c";
};

} // namespace

namespace comdare::cache_engine::abi {
template <>
struct organ_meta_meta_binding<ProbeStratA> {
    using type = ::comdare::cache_engine::measurement::MetaMetaMembers<ProbeMmA>;
};
template <>
struct organ_meta_meta_binding<ProbeStratB> {
    using type = ::comdare::cache_engine::measurement::MetaMetaMembers<ProbeMmB>;
};
} // namespace comdare::cache_engine::abi

namespace {

template <class Filter, class Persist>
struct ZweiTraegerComp {
    using search_algo        = MockAxis;
    using cache_traversal    = MockAxis;
    using mapping            = MockAxis;
    using path_compression   = MockAxis;
    using node_type          = MockAxis;
    using memory_layout      = MockAxis;
    using allocator          = MockAxis;
    using prefetch           = MockAxis;
    using concurrency        = MockAxis;
    using serialization      = MockAxis;
    using value_handle       = MockAxis;
    using index_organization = MockAxis;
    using io_dispatch        = MockAxis;
    using migration_policy   = MockAxis;
    using filter             = Filter; // Slot 14
    using queuing_q1         = MockAxis;
    using queuing_q2         = MockAxis;
    using persistence_target = Persist; // Slot 17
};
using CompAB  = ZweiTraegerComp<ProbeStratA, ProbeStratB>; // A am filter-Slot, B am persistence_target-Slot
using CompBA  = ZweiTraegerComp<ProbeStratB, ProbeStratA>; // umgekehrt gebunden -> Ordnung = SLOT-Ordnung (R-11)
using CompDup = ZweiTraegerComp<ProbeStratA, ProbeStratA>; // dasselbe Glied an zwei Slots -> Duplikat

static_assert(std::is_same_v<abi::organ_meta_metas_of_t<CompAB>, meas::MetaMetaMembers<ProbeMmA, ProbeMmB>>);
static_assert(std::is_same_v<abi::organ_meta_metas_of_t<CompBA>, meas::MetaMetaMembers<ProbeMmB, ProbeMmA>>);
static_assert(abi::organ_meta_metas_sind_duplikatfrei<abi::organ_meta_metas_of_t<CompAB>>::value);
static_assert(!abi::organ_meta_metas_sind_duplikatfrei<abi::organ_meta_metas_of_t<CompDup>>::value,
              "FIX-1 (iii): zwei Slots mit demselben Glied fallen an der Duplikatfrei-Wache");
static_assert(!abi::organ_meta_meta_subsumiert<abi::OrganMetaMetas, abi::organ_meta_metas_of_t<CompAB>>::value,
              "test-lokale Glieder liegen AUSSERHALB der Vollmenge -> organ_stamp_line<CompAB> wird von der "
              "4a-Vollmengen-Wache abgewiesen (Compile-Rot-Beleg bau/t1-rot/R10); die Probe faehrt am Renderer");

/// Test-lokale Tabelle ueber DENSELBEN reflect_versions-Weg wie die Produktion (18 Slots).
template <class Comp>
[[nodiscard]] std::vector<ex::AxisVariantVersion> zwei_traeger_tabelle() {
    std::vector<ex::AxisVariantVersion> t;
    ex::reflect_versions<mp::mp_list<typename Comp::search_algo>>("search_algo", t);
    ex::reflect_versions<mp::mp_list<typename Comp::cache_traversal>>("cache_traversal", t);
    ex::reflect_versions<mp::mp_list<typename Comp::mapping>>("mapping", t);
    ex::reflect_versions<mp::mp_list<typename Comp::path_compression>>("path_compression", t);
    ex::reflect_versions<mp::mp_list<typename Comp::node_type>>("node_type", t);
    ex::reflect_versions<mp::mp_list<typename Comp::memory_layout>>("memory_layout", t);
    ex::reflect_versions<mp::mp_list<typename Comp::allocator>>("allocator", t);
    ex::reflect_versions<mp::mp_list<typename Comp::prefetch>>("prefetch", t);
    ex::reflect_versions<mp::mp_list<typename Comp::concurrency>>("concurrency", t);
    ex::reflect_versions<mp::mp_list<typename Comp::serialization>>("serialization", t);
    ex::reflect_versions<mp::mp_list<typename Comp::value_handle>>("value_handle", t);
    ex::reflect_versions<mp::mp_list<typename Comp::index_organization>>("index_organization", t);
    ex::reflect_versions<mp::mp_list<typename Comp::io_dispatch>>("io_dispatch", t);
    ex::reflect_versions<mp::mp_list<typename Comp::migration_policy>>("migration_policy", t);
    ex::reflect_versions<mp::mp_list<typename Comp::filter>>("filter", t);
    ex::reflect_versions<mp::mp_list<typename Comp::queuing_q1>>("queuing_q1", t);
    ex::reflect_versions<mp::mp_list<typename Comp::queuing_q2>>("queuing_q2", t);
    ex::reflect_versions<mp::mp_list<typename Comp::persistence_target>>("persistence_target", t);
    return t;
}

/// Die Mock-Vollzeile aus DENSELBEN zwei Bausteinen wie organ_stamp_line (build_axis_version_stamp_line +
/// append_meta_meta_suffix ueber den EINEN Renderer) -- ohne die Vollmengen-Wache (S4-2).
template <class Comp>
[[nodiscard]] std::string mock_vollzeile() {
    std::array<meas::AxisVersionEntry, 18> const e{{
        {"search_algo", Comp::search_algo::name(), Comp::search_algo::algo_version},
        {"cache_traversal", Comp::cache_traversal::name(), Comp::cache_traversal::algo_version},
        {"mapping", Comp::mapping::name(), Comp::mapping::algo_version},
        {"path_compression", Comp::path_compression::name(), Comp::path_compression::algo_version},
        {"node_type", Comp::node_type::name(), Comp::node_type::algo_version},
        {"memory_layout", Comp::memory_layout::name(), Comp::memory_layout::algo_version},
        {"allocator", Comp::allocator::name(), Comp::allocator::algo_version},
        {"prefetch", Comp::prefetch::name(), Comp::prefetch::algo_version},
        {"concurrency", Comp::concurrency::name(), Comp::concurrency::algo_version},
        {"serialization", Comp::serialization::name(), Comp::serialization::algo_version},
        {"value_handle", Comp::value_handle::name(), Comp::value_handle::algo_version},
        {"index_organization", Comp::index_organization::name(), Comp::index_organization::algo_version},
        {"io_dispatch", Comp::io_dispatch::name(), Comp::io_dispatch::algo_version},
        {"migration_policy", Comp::migration_policy::name(), Comp::migration_policy::algo_version},
        {"filter", Comp::filter::name(), Comp::filter::algo_version},
        {"queuing_q1", Comp::queuing_q1::name(), Comp::queuing_q1::algo_version},
        {"queuing_q2", Comp::queuing_q2::name(), Comp::queuing_q2::algo_version},
        {"persistence_target", Comp::persistence_target::name(), Comp::persistence_target::algo_version},
    }};
    std::string                                  line = meas::build_axis_version_stamp_line(e);
    abi::append_meta_meta_suffix(line, abi::meta_meta_stamp_suffix_from_members<abi::organ_meta_metas_of_t<Comp>>());
    return line;
}

TEST(E10OrganStampJeComp, ZweiTraegerEinGruppenFormMockGleichReal) {
    constexpr std::string_view kGruppeAB = "[probe_mm_a=code@1.0.0.c;probe_mm_b=code@1.0.0.c]";
    constexpr std::string_view kGruppeBA = "[probe_mm_b=code@1.0.0.c;probe_mm_a=code@1.0.0.c]";
    // Renderer (Mock-Grammatik): EINE Gruppe, innen ';', EINMAL gewrappt.
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from_members<abi::organ_meta_metas_of_t<CompAB>>(), kGruppeAB);
    EXPECT_EQ(abi::meta_meta_stamp_suffix_from_members<abi::organ_meta_metas_of_t<CompBA>>(), kGruppeBA);
    // Realer Pfad (Tabelle, UNGEWRAPPTE Entries je Variante -> 4b verkettert und wrappt GENAU EINMAL).
    auto const t_ab = zwei_traeger_tabelle<CompAB>();
    EXPECT_EQ(ex::lookup_meta_meta_entries(t_ab, "filter", "probe_strat_a"), "probe_mm_a=code@1.0.0.c");
    EXPECT_EQ(ex::lookup_meta_meta_entries(t_ab, "persistence_target", "probe_strat_b"), "probe_mm_b=code@1.0.0.c");
    EXPECT_EQ(ex::lookup_meta_meta_entries(t_ab, "search_algo", "algoA"), "");
    std::string const real_ab = ex::compose_organ_stamp_line(axes_of<CompAB>(), t_ab);
    EXPECT_TRUE(real_ab.ends_with(std::string{";"} + std::string{kGruppeAB})) << real_ab;
    EXPECT_EQ(count_char(real_ab, '['), 1u) << "Ein-Gruppen-Form, NICHT [a=..];[b=..]: " << real_ab;
    EXPECT_EQ(count_char(real_ab, ']'), 1u);
    EXPECT_EQ(real_ab.find("];["), std::string::npos) << "die O-8-Schritt-12-Falle (zwei Gruppen): " << real_ab;
    EXPECT_EQ(count_char(real_ab, ';'), 19u) << "18 Slots + 1 Gruppe (innen 1 Trenner) -> 19 Trenner";
    EXPECT_EQ(mock_vollzeile<CompAB>(), real_ab) << "Mock-Bausteine == realer Pfad (Ein-Renderer-Doktrin)";
    // Ordnung = kCompositionAxisNames-Ordnung der Traeger-Slots (R-11), nicht die Bindungs-Reihenfolge.
    auto const        t_ba    = zwei_traeger_tabelle<CompBA>();
    std::string const real_ba = ex::compose_organ_stamp_line(axes_of<CompBA>(), t_ba);
    EXPECT_TRUE(real_ba.ends_with(std::string{";"} + std::string{kGruppeBA})) << real_ba;
    EXPECT_EQ(mock_vollzeile<CompBA>(), real_ba);
    // Ein-Traeger-Gegenprobe im selben Rahmen: nur B gebunden -> genau ein Glied, keine Trenner in der Gruppe.
    using CompB              = ZweiTraegerComp<MockAxis, ProbeStratB>;
    std::string const real_b = ex::compose_organ_stamp_line(axes_of<CompB>(), zwei_traeger_tabelle<CompB>());
    EXPECT_TRUE(real_b.ends_with(";[probe_mm_b=code@1.0.0.c]")) << real_b;
    EXPECT_EQ(mock_vollzeile<CompB>(), real_b);
}

// -- (G) 18+1-NENNER: die Kompositions-Welt bleibt 18, die Vollmenge traegt genau ORG-19-IO -------------------
TEST(E10OrganStampJeComp, NennerBleibt18Plus1) {
    EXPECT_EQ(abi::kOrganAxisCount, 18u);
    EXPECT_EQ(ex::kCompositionAxisNames.size(), 18u);
    EXPECT_EQ(abi::OrganMetaMetas::size, 1u);
}

} // namespace
