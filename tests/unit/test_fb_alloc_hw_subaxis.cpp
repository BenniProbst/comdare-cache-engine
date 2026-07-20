// test_fb_alloc_hw_subaxis — F-B (GO4/#8, 2026-07-12), Muster test_ff2_node_width_subaxis (C2).
// Belegt die NEUE F-B-Unterachse "NUMA/Page->allocator" (alloc_hw_config.hpp) auf allen 3 Ebenen:
//   (1) WERTRAUM + HW-GATE: numa_node {auto,0,1} + page {4k,2m}, distinkt; make_*-Bruecken; Auto/Native
//       als Aus-Default. Das HW-Gate konsumiert die axis_12-Eigenschaften ALS TYP-PARAMETER an der
//       Allocator-Kante (Dossier GO4 §1/F-B): gate_alloc_hw_for<Generic> (numa_capable=false,
//       huge_page_capable=false) kompiliert Pinning/2m-Hint weg; X86_64 behaelt sie; page_bytes_for
//       liest memory_page_size() — damit sind memory_page_size/huge_page_capable erstmals konsumiert
//       (vorher honest-0, Dossier §2.1).
//   (2) BINARY-ID-NEUTRALITAET: ohne Profil-Aktivierung entsteht KEINE alloc_hw-Ebene -> binary_ids
//       byte-identisch (golden-320/base_pilot/m3v2 unberuehrt); NUR ein aktivierendes Profil (hier
//       fb_numa_page_study.profile.xml, argv[1]) erzeugt die statischen Sub-Ebenen
//       alloc_hw.numa_node/alloc_hw.page.
//   (3) REALER KONSUM (kein Phantom): NUMAllocAllocatorBody bindet numa_node_ an den COMPILE-TIME-
//       Parameter (kDefaultNumaNode statt Hartcode -1) — unterschiedlicher Parameter -> unterschiedliche
//       Node-Bindung (Getter numa_node(), derselbe Member geht in numalloc_alloc(bytes, alignment, node));
//       PoolResourceAllocatorBody setzt aus dem Page-Hint die pmr::pool_options (largest_required_pool_block)
//       — beobachtbar via pool_options_in_effect(). Default (beide Registry-Blaetter) = Auto/Native ->
//       byte-identisch zum Ist-Stand.
// HW-GATE-GRENZE (ehrlich): der NUMA-EFFEKT braucht Multi-Socket-HW (~Sep); hier wird die KONFIGURATIONS-
// Ankunft bewiesen (Knopf existiert + wirkt im Code), nicht der Multi-Socket-Durchsatz-Effekt.

#include <axes/alloc/alloc_hw_config.hpp>
#include <axes/alloc/axis_06_allocator_numalloc.hpp>
#include <axes/alloc/axis_06_allocator_pool_resource.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_generic.hpp>
#include <topics/hardware/axis_12_general_hardware/axis_12_general_hardware_x86_64.hpp>

#include <builder/experiment_tree/experiment_tree.hpp>
#include <builder/experiment_tree/profile_to_tree.hpp>
#include "xml_config_parser/xml_config_parser.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace al = comdare::cache_engine::alloc;
namespace hw = comdare::cache_engine::hardware::axis_12_general_hardware;
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

// ── (3) Konfigurierte Varianten: exakt so wuerde der Codegen eine alloc_hw-Variante backen ──────────────
// (Muster KF-5/C2: Blatt = konkrete Klasse, explizites NTTP am CRTP-Body; gleicher Koerper wie das
// Registry-Blatt, NUR die Unterachsen-Konfiguration unterscheidet sich -> jede Divergenz unten kommt
// aus dem Parameter).
struct PinnedNumaNode0 : al::NUMAllocAllocatorBody<PinnedNumaNode0, al::AllocHwConfig{al::AllocNumaNode::Node0}> {};
struct PinnedNumaNode1 : al::NUMAllocAllocatorBody<PinnedNumaNode1, al::AllocHwConfig{al::AllocNumaNode::Node1}> {};
struct PagePool4k
    : al::PoolResourceAllocatorBody<PagePool4k, al::AllocHwConfig{al::AllocNumaNode::Auto, al::AllocPageHint::Page4k}> {
};
struct PagePool2m
    : al::PoolResourceAllocatorBody<PagePool2m, al::AllocHwConfig{al::AllocNumaNode::Auto, al::AllocPageHint::Page2m}> {
};

// ── Compile-Time-Belege (1): Wertraum + Mixin/Concept ────────────────────────────────────────────────────
static_assert(al::all_alloc_numa_nodes().size() == 3);
static_assert(al::all_alloc_page_hints().size() == 2);
static_assert(al::make_alloc_numa_node("auto") == al::AllocNumaNode::Auto);
static_assert(al::make_alloc_numa_node("0") == al::AllocNumaNode::Node0);
static_assert(al::make_alloc_numa_node("1") == al::AllocNumaNode::Node1);
static_assert(al::make_alloc_numa_node("7") == al::AllocNumaNode::Auto); // kein Raten
static_assert(al::make_alloc_page_hint("4k") == al::AllocPageHint::Page4k);
static_assert(al::make_alloc_page_hint("2m") == al::AllocPageHint::Page2m);
static_assert(al::make_alloc_page_hint("1g") == al::AllocPageHint::Native); // kein Raten
static_assert(al::AllocHwConfigurable<al::NUMAllocAllocator>);              // Einwebung (Basis)
static_assert(al::AllocHwConfigurable<al::PoolResourceAllocator>);
static_assert(al::AllocHwConfigurable<PinnedNumaNode0> && al::AllocHwConfigurable<PagePool2m>);

// ── Compile-Time-Belege (1): HW-Gate — die axis_12-Eigenschaften als Typ-Parameter an der Kante ─────────
static_assert(al::AllocHwPlatformProfile<hw::X86_64HardwareProfile>);
static_assert(al::AllocHwPlatformProfile<hw::GenericHardwareProfile>);
inline constexpr al::AllocHwConfig kPinned2m{al::AllocNumaNode::Node1, al::AllocPageHint::Page2m};
// X86_64 (numa_capable=true, huge_page_capable=true): Anforderung bleibt vollstaendig erhalten.
static_assert(al::gate_alloc_hw_for<hw::X86_64HardwareProfile>(kPinned2m) == kPinned2m);
// Generic (numa_capable=false, huge_page_capable=false): Pinning WEGKOMPILIERT, 2m -> 4k (Basis-Page).
static_assert(al::gate_alloc_hw_for<hw::GenericHardwareProfile>(kPinned2m) ==
              al::AllocHwConfig{al::AllocNumaNode::Auto, al::AllocPageHint::Page4k});
// memory_page_size() wird konsumiert (vorher honest-0): 4k-Hint = Basis-Page der Plattform.
static_assert(al::alloc_hw_page_bytes_for<hw::X86_64HardwareProfile>(al::AllocPageHint::Page4k) ==
              hw::X86_64HardwareProfile::memory_page_size());
static_assert(al::alloc_hw_page_bytes_for<hw::X86_64HardwareProfile>(al::AllocPageHint::Page2m) ==
              al::kAllocHugePage2mBytes);
// huge_page_capable()==false gated den 2m-Hint auf die Basis-Page zurueck.
static_assert(al::alloc_hw_page_bytes_for<hw::GenericHardwareProfile>(al::AllocPageHint::Page2m) ==
              hw::GenericHardwareProfile::memory_page_size());
static_assert(al::alloc_hw_page_bytes_for<hw::X86_64HardwareProfile>(al::AllocPageHint::Native) == 0);

// ── Compile-Time-Belege (3): REALER Konsum — der Parameter kommt in den Allocator-Koerpern an ───────────
// NUMAlloc: kDefaultNumaNode ist NICHT mehr der Hartcode -1, sondern der Unterachsen-Parameter.
static_assert(al::NUMAllocAllocator::kDefaultNumaNode == -1); // Default Auto = byte-identisch zum Ist-Stand
static_assert(PinnedNumaNode0::kDefaultNumaNode == 0);
static_assert(PinnedNumaNode1::kDefaultNumaNode == 1);
static_assert(PinnedNumaNode0::kDefaultNumaNode != al::NUMAllocAllocator::kDefaultNumaNode);
// Pool: Page-Hint-Bytes divergieren compile-time mit dem Parameter (Native=0 / 4k / 2m).
static_assert(al::PoolResourceAllocator::alloc_hw_page_bytes() == 0);
static_assert(PagePool4k::alloc_hw_page_bytes() == 4096);
static_assert(PagePool2m::alloc_hw_page_bytes() == al::kAllocHugePage2mBytes);
static_assert(PagePool2m::alloc_hw_page_bytes() != PagePool4k::alloc_hw_page_bytes());

int main(int argc, char** argv) {
    std::cout << "F-B alloc_hw-Unterachse: Wertraum/HW-Gate + Neutralitaet + realer Konsum\n";

    // ── (1) Wertraum: distinkte Messwerte ──
    {
        std::set<int> uniq_n;
        for (auto n : al::all_alloc_numa_nodes()) uniq_n.insert(static_cast<int>(n));
        check_eq("all_alloc_numa_nodes distinkt", uniq_n.size(), std::size_t{3});
        std::set<int> uniq_p;
        for (auto p : al::all_alloc_page_hints()) uniq_p.insert(static_cast<int>(p));
        check_eq("all_alloc_page_hints distinkt", uniq_p.size(), std::size_t{2});
        check_true("Native ist KEIN Page-Messwert", uniq_p.find(0) == uniq_p.end());
    }

    // ── (3) REALER Konsum, runtime: NUMAlloc-Node-Bindung folgt dem compile-time Parameter ──
    {
        al::NUMAllocAllocator nat;
        PinnedNumaNode0       p0;
        PinnedNumaNode1       p1;
        check_eq("NUMAlloc default: numa_node() (Auto/kernel-Default)", nat.numa_node(), -1);
        check_eq("NUMAlloc Node0-Variante: numa_node()", p0.numa_node(), 0);
        check_eq("NUMAlloc Node1-Variante: numa_node()", p1.numa_node(), 1);
        check_true("Node-Bindung divergiert mit dem Parameter", p0.numa_node() != nat.numa_node());
        // Der gebundene Member geht real in den Allokations-Pfad (numalloc_alloc(..., numa_node_)):
        void* mem = p0.allocate(64, alignof(std::max_align_t));
        check_true("Node0-Variante: allocate liefert Speicher", mem != nullptr);
        if (mem != nullptr) {
            std::memset(mem, 0xAB, 64);
            p0.deallocate(mem, 64, alignof(std::max_align_t));
        }
    }

    // ── (3) REALER Konsum, runtime: Pool-Page-Hint setzt die pmr::pool_options ──
    {
        al::PoolResourceAllocator nat;
        PagePool2m                p2m;
        auto const                on = nat.pool_options_in_effect();
        auto const                o2 = p2m.pool_options_in_effect();
        check_eq("Pool 2m-Variante: largest_required_pool_block >= 2 MiB",
                 o2.largest_required_pool_block >= al::kAllocHugePage2mBytes, true);
        check_true("Pool-Geometrie divergiert mit dem Parameter",
                   o2.largest_required_pool_block != on.largest_required_pool_block);
        void* mem = p2m.allocate(4096, alignof(std::max_align_t));
        check_true("2m-Pool: allocate liefert Speicher", mem != nullptr);
        if (mem != nullptr) {
            std::memset(mem, 0xCD, 4096);
            p2m.deallocate(mem, 4096, alignof(std::max_align_t));
        }
    }

    // ── (2a) Binary-id-Neutralitaet, in-code (Muster test_ff2 Teil 2a) ──
    // P0 = Profil OHNE alloc_hw; P1 = MIT alloc_hw, aber Modus aktiviert sie NICHT; P2 = aktiviert.
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

    cx::ThesisProfile  p1 = p0; // + alloc_hw deklariert, NICHT aktiviert
    cx::ThesisAxisSpec ah;
    ah.ref              = "alloc_hw";
    ah.alloc_numa_nodes = {"auto", "0", "1"};
    ah.alloc_pages      = {"4k", "2m"};
    p1.permute_axes.push_back(ah);

    cx::ThesisProfile p2 = p1; // + aktiviert
    p2.modes[0].active_axes.push_back("alloc_hw");

    auto const ids0 = ids_of(p0, "ce_only");
    auto const ids1 = ids_of(p1, "ce_only");
    auto const ids2 = ids_of(p2, "ce_only");
    check_eq("P0: binary_count (tier2 x memory_layout2)", ids0.size(), std::size_t{4});
    check_true("NEUTRALITAET: alloc_hw deklariert-aber-inaktiv -> binary_ids byte-identisch", ids1 == ids0);
    check_eq("AKTIVIERT: binary_count x6 (numa_node3 x page2)", ids2.size(), std::size_t{24});
    check_true("AKTIVIERT: binary_id traegt alloc_hw.numa_node-Segment",
               !ids2.empty() && ids2.front().find("alloc_hw.numa_node=auto") != std::string::npos);
    check_true("AKTIVIERT: binary_id traegt alloc_hw.page-Segment",
               !ids2.empty() && ids2.front().find("alloc_hw.page=4k") != std::string::npos);

    // ── (2b) Das REALE Studien-Profil (argv[1] = fb_numa_page_study.profile.xml) ──
    if (argc > 1) {
        cx::XmlConfigParser parser;
        auto                tp = parser.parse_thesis_profile(argv[1]);
        check_true("fb-Profil geparst", tp.has_value());
        if (tp) {
            check_eq("fb: id", tp->id, std::string{"fb_numa_page_study"});
            cx::ThesisAxisSpec const* ax = nullptr;
            for (auto const& a : tp->permute_axes)
                if (a.ref == "alloc_hw") ax = &a;
            check_true("fb: alloc_hw-Achse vorhanden", ax != nullptr);
            if (ax) {
                check_eq("fb: numa_node (auto 0 1)", ax->alloc_numa_nodes.size(), std::size_t{3});
                check_eq("fb: page (4k 2m)", ax->alloc_pages.size(), std::size_t{2});
            }
            auto const levels     = ex::build_axis_levels(*tp, "ce_only", ex::AxisRegistry{});
            bool       found_node = false;
            bool       found_page = false;
            for (auto const& l : levels) {
                if (l.axis == "alloc_hw.numa_node") {
                    found_node = true;
                    check_true("fb: numa_node-Ebene ist STATISCH (Binary-Identitaet)", l.is_static);
                    check_eq("fb: numa_node-Ebene traegt 3 Werte", l.values.size(), std::size_t{3});
                }
                if (l.axis == "alloc_hw.page") {
                    found_page = true;
                    check_true("fb: page-Ebene ist STATISCH (Binary-Identitaet)", l.is_static);
                    check_eq("fb: page-Ebene traegt 2 Werte", l.values.size(), std::size_t{2});
                }
            }
            check_true("fb: statische Ebene alloc_hw.numa_node emittiert", found_node);
            check_true("fb: statische Ebene alloc_hw.page emittiert", found_page);
        }
    } else {
        std::cout << "  [SKIP] kein Profil-Pfad (argv[1]) - (2b) uebersprungen\n";
    }

    std::cout << "\n==== F-B alloc_hw-Unterachse: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
