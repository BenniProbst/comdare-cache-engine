// test_ff2_node_width_subaxis — C2 (GO4/#8 F-C, 2026-07-12), Muster test_kf3/test_kf9.
// Belegt die NEUE FF2-Unterachse "Knoten-Breite in Cache-Lines" (node_width_config.hpp) auf allen 3 Ebenen:
//   (1) WERTRAUM: 5 Messwerte {1,2,4,8,16}, distinkt; make_node_width-Bruecke; Native=0 als Aus-Default.
//   (2) BINARY-ID-NEUTRALITAET: ohne Profil-Aktivierung entsteht KEINE node_width-Ebene -> binary_ids
//       byte-identisch (golden-320/base_pilot/m3v2 unberuehrt); NUR ein aktivierendes Profil (hier
//       ff2_node_width_study.profile.xml, argv[1]) erzeugt die statische Sub-Ebene node_width.width_in_lines.
//   (3) REALER KONSUM (kein Phantom): das Node-Organ deklariert die Breite (NodeWidthAware via
//       NodeTypeStrategyBase-NTTP); der LayoutAwareChunkedStore (Node-Layout-Pfad) macht das Chunk-/Knoten-
//       Backing NACHWEISLICH W Cache-Lines breit — Kapazitaet, chunk_bytes und chunk_count aendern sich
//       compile-time-berechenbar mit W, Round-Trip bleibt verlustfrei. Default (alle bestehenden Node-
//       Blaetter) = Native -> byte-identisch zum Ist-Stand.
// Thesis FF2 (kapitel/de/01_einleitung.tex:94-99): CSS/CSB+ = 1 Cache-Line vs. Hankins/Patel = 16 Cache-Lines.

#include <axes/cacheline/node_width_config.hpp>
#include <axes/node/axis_04_node_type_strategy_base.hpp>
#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/node/axis_04_node_type_layout_aware_store.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_aos_strict.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>

#include <builder/experiment_tree/experiment_tree.hpp>
#include <builder/experiment_tree/profile_to_tree.hpp>
#include "xml_config_parser/xml_config_parser.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cl = comdare::cache_engine::cacheline;
namespace nd = comdare::cache_engine::node;
namespace ml = comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace al = comdare::cache_engine::allocator::axis_06_allocator;
namespace ex = comdare::cache_engine::builder::experiment;
namespace cx = comdare::builder::xml;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) {
        std::cout << "  (erwartet: " << want << ")";
        ++g_fail;
    }
    std::cout << "\n";
}
void check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

// ── (3) Wide-Node-Varianten: exakt so wuerde der Codegen eine width-konfigurierte Node-Variante bauen ──
// (Muster KF-5: Blatt = konkrete Klasse, explizites NTTP an der CRTP-Basis; gleiche intrinsische Kapazitaet
// wie Node4, NUR die deklarierte Breite unterscheidet sich -> jede Divergenz unten kommt aus der Breite).
struct WideNodeW1
    : nd::NodeTypeStrategyBase<WideNodeW1, cl::CacheLineConfig{}, cl::NodeWidthConfig{cl::NodeWidthInLines::W1}> {
    using topic_tag = comdare::cache_engine::nodes::concepts::NodesTopicTag;
    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 4; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "wide_node_w1"; }
};
struct WideNodeW16
    : nd::NodeTypeStrategyBase<WideNodeW16, cl::CacheLineConfig{}, cl::NodeWidthConfig{cl::NodeWidthInLines::W16}> {
    using topic_tag = comdare::cache_engine::nodes::concepts::NodesTopicTag;
    [[nodiscard]] static constexpr std::size_t      max_capacity() noexcept { return 4; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "wide_node_w16"; }
};

// ── Compile-Time-Belege (1): Wertraum + Mixin/Concept ────────────────────────────────────────────────────
static_assert(cl::all_node_widths().size() == 5);
static_assert(cl::make_node_width(1).width_in_lines == cl::NodeWidthInLines::W1);
static_assert(cl::make_node_width(16).width_in_lines == cl::NodeWidthInLines::W16);
static_assert(cl::make_node_width(0).width_in_lines == cl::NodeWidthInLines::Native); // Aus-Default
static_assert(cl::make_node_width(3).width_in_lines == cl::NodeWidthInLines::Native); // kein Raten
static_assert(cl::NodeWidthConfigurable<nd::Node4NodeType>);                          // Einwebung (Basis)
static_assert(nd::Node4NodeType::node_width_in_lines() == 0);                         // Default = Native
static_assert(cl::NodeWidthConfigurable<WideNodeW1> && cl::NodeWidthConfigurable<WideNodeW16>);
static_assert(WideNodeW1::node_width_in_lines() == 1 && WideNodeW16::node_width_in_lines() == 16);
static_assert(WideNodeW16::node_width_bytes() == 16 * 64);

// ── Compile-Time-Belege (3): REALER Konsum im Node-Layout-Pfad (LayoutAwareChunkedStore) ────────────────
using NativeStoreCLA =
    nd::LayoutAwareChunkedStore<nd::Node4NodeType, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;
using W1StoreCLA  = nd::LayoutAwareChunkedStore<WideNodeW1, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;
using W16StoreCLA = nd::LayoutAwareChunkedStore<WideNodeW16, ml::CacheLineAlignedMemoryLayout, al::MimallocAllocator>;
using W1StoreAoS  = nd::LayoutAwareChunkedStore<WideNodeW1, ml::AoSStrictMemoryLayout, al::MimallocAllocator>;
using W16StoreAoS = nd::LayoutAwareChunkedStore<WideNodeW16, ml::AoSStrictMemoryLayout, al::MimallocAllocator>;

// Native (Ist-Stand, byte-identisch): Kapazitaet aus dem Node-Organ, KEINE Breiten-Vorgabe.
static_assert(NativeStoreCLA::node_width_in_lines() == 0);
static_assert(NativeStoreCLA::node_capacity() == 4 && NativeStoreCLA::chunk_bytes() == 4 * 64);
// W1 (CSS/CSB+): Knoten-Backing = 1 Cache-Line. CLA-Records sind 64 B -> 1 Record; AoS-strict 16 B -> 4.
static_assert(W1StoreCLA::node_width_in_lines() == 1);
static_assert(W1StoreCLA::node_capacity() == 1 && W1StoreCLA::chunk_bytes() == 64);
static_assert(W1StoreAoS::node_capacity() == 4 && W1StoreAoS::chunk_bytes() == 64);
// W16 (Hankins/Patel): Knoten-Backing = 16 Cache-Lines = 1024 B. CLA -> 16 Records; AoS-strict -> 64.
static_assert(W16StoreCLA::node_capacity() == 16 && W16StoreCLA::chunk_bytes() == 16 * 64);
static_assert(W16StoreAoS::node_capacity() == 64 && W16StoreAoS::chunk_bytes() == 16 * 64);
// Die Breite ist eine ECHTE Layout-Divergenz: gleicher Node-Kern (max_capacity 4), gleiche Layout-/Alloc-
// Achse — NUR W unterscheidet die physische Chunk-Geometrie.
static_assert(W1StoreCLA::chunk_bytes() != W16StoreCLA::chunk_bytes());
static_assert(W1StoreCLA::node_capacity() != NativeStoreCLA::node_capacity());

// ── Helper: Round-Trip-Fuellung ──────────────────────────────────────────────────────────────────────────
template <class Store>
Store make_store(std::size_t n) {
    Store s;
    for (std::size_t i = 0; i < n; ++i) s.append_slot(0xC0FFEE00u + i, 7u * i + 1u);
    return s;
}
template <class Store>
bool roundtrip_ok(Store const& s, std::size_t n) {
    if (s.slot_count() != n) return false;
    for (std::size_t i = 0; i < n; ++i)
        if (s.key_at(i) != 0xC0FFEE00u + i || s.value_at(i) != 7u * i + 1u) return false;
    return true;
}

int main(int argc, char** argv) {
    std::cout << "FF2 node_width-Unterachse (C2): Wertraum + Neutralitaet + realer Konsum\n";

    // ── (1) Wertraum: 5 distinkte Messwerte ──
    auto widths = cl::all_node_widths();
    check_eq("all_node_widths().size()", widths.size(), std::size_t{5});
    std::set<int> uniq;
    for (auto w : widths) uniq.insert(static_cast<int>(w.width_in_lines));
    check_eq("all_node_widths distinkt", uniq.size(), std::size_t{5});
    check_true("Native ist KEIN Messwert", uniq.find(0) == uniq.end());

    // ── (3) REALER Konsum, runtime: identische Daten, NUR die Breite variiert -> divergente Chunk-Geometrie ──
    constexpr std::size_t kN  = 32;
    auto const            nat = make_store<NativeStoreCLA>(kN);
    auto const            w1  = make_store<W1StoreCLA>(kN);
    auto const            w16 = make_store<W16StoreCLA>(kN);
    check_eq("Native/CLA: chunk_count (32 Records / cap 4)", nat.chunk_count(), std::size_t{8});
    check_eq("W1/CLA:     chunk_count (32 Records / cap 1)", w1.chunk_count(), std::size_t{32});
    check_eq("W16/CLA:    chunk_count (32 Records / cap 16)", w16.chunk_count(), std::size_t{2});
    check_eq("W1/CLA:  chunk_capacity_bytes == 1 Cache-Line", w1.chunk_capacity_bytes(), std::size_t{64});
    check_eq("W16/CLA: chunk_capacity_bytes == 16 Cache-Lines", w16.chunk_capacity_bytes(), std::size_t{1024});
    check_true("Round-Trip verlustfrei (Native)", roundtrip_ok(nat, kN));
    check_true("Round-Trip verlustfrei (W1)", roundtrip_ok(w1, kN));
    check_true("Round-Trip verlustfrei (W16)", roundtrip_ok(w16, kN));

    // ── (2a) Binary-id-Neutralitaet, in-code (Muster test_kf9 Teil 2) ──
    // P0 = Profil OHNE node_width; P1 = MIT node_width, aber Modus aktiviert sie NICHT; P2 = aktiviert.
    auto ids_of = [](cx::ThesisProfile const& tp, std::string const& mode) {
        auto               factory = std::make_shared<ex::ExperimentNodeFactory>();
        ex::ExperimentTree tree{factory};
        tree.build(ex::build_axis_levels(tp, mode, ex::AxisRegistry{}));
        ex::StaticBinaryView const view = tree.static_binary_view();
        std::vector<std::string>   ids;
        ids.reserve(view.size());
        for (std::size_t i = 0; i < view.size(); ++i) ids.push_back(view[i].binary_id);
        return ids;
    };
    cx::ThesisProfile p0;
    p0.id         = "p0";
    p0.base_tiers = {{"art", "../sota/art.profile.xml", "P01"}, {"hot", "../sota/hot.profile.xml", "P02"}};
    // #1-Fix-konform: eine ECHTE Organ-Kompositions-Achse (memory_layout ∈ kCompositionAxisNames) als generische
    // 2-Wert-Baseline (der Organ-only-binary_id-Guard laesst nur Organ-Achsen ins statische Level; die frueher hier
    // benutzte System-Achse isa ist seit INC-2d KEIN Kompositions-Slot mehr → nicht binary_id-tragend).
    cx::ThesisAxisSpec ml;
    ml.ref          = "memory_layout";
    ml.values       = {"aos", "soa"};
    p0.permute_axes = {ml};
    cx::ThesisMode m0;
    m0.name        = "ce_only";
    m0.merge       = "Stufe1_CeOnly";
    m0.active_axes = {"memory_layout"};
    p0.modes       = {m0};

    cx::ThesisProfile  p1 = p0; // + node_width deklariert, NICHT aktiviert
    cx::ThesisAxisSpec nw;
    nw.ref            = "node_width";
    nw.width_in_lines = {"1", "2", "4", "8", "16"};
    p1.permute_axes.push_back(nw);

    cx::ThesisProfile p2 = p1; // + aktiviert
    p2.modes[0].active_axes.push_back("node_width");

    auto const ids0 = ids_of(p0, "ce_only");
    auto const ids1 = ids_of(p1, "ce_only");
    auto const ids2 = ids_of(p2, "ce_only");
    check_eq("P0: binary_count (tier2 x memory_layout2)", ids0.size(), std::size_t{4});
    check_true("NEUTRALITAET: node_width deklariert-aber-inaktiv -> binary_ids byte-identisch", ids1 == ids0);
    check_eq("AKTIVIERT: binary_count x5 (node_width.width_in_lines)", ids2.size(), std::size_t{20});
    check_true("AKTIVIERT: binary_id traegt node_width.width_in_lines-Segment",
               !ids2.empty() && ids2.front().find("node_width.width_in_lines=1") != std::string::npos);

    // ── (2b) Das REALE Studien-Profil (argv[1] = ff2_node_width_study.profile.xml) ──
    if (argc > 1) {
        cx::XmlConfigParser parser;
        auto                tp = parser.parse_thesis_profile(argv[1]);
        check_true("ff2-Profil geparst", tp.has_value());
        if (tp) {
            check_eq("ff2: id", tp->id, std::string{"ff2_node_width_study"});
            cx::ThesisAxisSpec const* ax = nullptr;
            for (auto const& a : tp->permute_axes)
                if (a.ref == "node_width") ax = &a;
            check_true("ff2: node_width-Achse vorhanden", ax != nullptr);
            if (ax) check_eq("ff2: width_in_lines (1 2 4 8 16)", ax->width_in_lines.size(), std::size_t{5});
            auto const levels = ex::build_axis_levels(*tp, "ce_only", ex::AxisRegistry{});
            bool       found  = false;
            for (auto const& l : levels)
                if (l.axis == "node_width.width_in_lines") {
                    found = true;
                    check_true("ff2: node_width-Ebene ist STATISCH (Binary-Identitaet)", l.is_static);
                    check_eq("ff2: node_width-Ebene traegt 5 Werte", l.values.size(), std::size_t{5});
                }
            check_true("ff2: statische Ebene node_width.width_in_lines emittiert", found);
        }
    } else {
        std::cout << "  [SKIP] kein Profil-Pfad (argv[1]) - (2b) uebersprungen\n";
    }

    std::cout << "\n==== FF2 node_width-Unterachse (C2): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
