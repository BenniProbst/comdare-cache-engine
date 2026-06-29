// test_kf5_cacheline_weave — KF-5 (2026-06-02)
// Belegt: die per-Organ Cache-Line-Unterachse (CacheLineAware<Cfg>) ist ADDITIV in die 5 CRTP-Basen
// (axis_01_page_type/axis_04_node_type/axis_03a_search_algo/axis_06_allocator/axis_05_memory_layout)
// eingewebt → JEDER echte Wrapper dieser 5 Achsen ist cacheline-fähig
// (CacheLineConfigurable), Default-Verhalten unverändert (nicht-brechend). + make_config-Factory korrekt.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> /I<.../include> /I<.../src> /I<build/generated> ...

#include <axes/node/axis_04_node_type_node4.hpp>
#include <axes/lookup/axis_03a_search_algo_array256.hpp>
#include <axes/alloc/axis_06_allocator_std_malloc.hpp>
#include <axes/layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/nodes/axis_01_page_type/axis_01_page_type_dense_byte.hpp>
#include <axes/cacheline/cacheline_config.hpp>

#include <cstddef>
#include <iostream>
#include <string>

namespace cl = comdare::cache_engine::cacheline;

using Node   = comdare::cache_engine::node::Node4NodeType;
using Search = comdare::cache_engine::lookup::Array256SearchAlgo;
using Alloc  = comdare::cache_engine::alloc::StdMalloc;
using Layout = comdare::cache_engine::layout::CacheLineAlignedMemoryLayout;
using Page   = comdare::cache_engine::nodes::axis_01_page_type::DenseBytePageType;

// ── Compile-Time-Belege ──────────────────────────────────────────────────────
// make_config: line_size → alignment (CacheLineAligned)
static_assert(cl::CacheLineAware<cl::make_config(64, 1, 0)>::cacheline_alignment() == 64);
static_assert(cl::CacheLineAware<cl::make_config(128, 1, 0)>::cacheline_alignment() == 128);
static_assert(cl::CacheLineAware<cl::make_config(256, 2, 0)>::cacheline_alignment() == 256);
// Default {} (alignment None) → natürliches max_align (nicht-brechend)
static_assert(cl::CacheLineAware<cl::CacheLineConfig{}>::cacheline_alignment() == alignof(std::max_align_t));
// make_config sw_hint korrekt geschichtet
static_assert(cl::make_config(64, 0, 4).sw_hint == cl::SwPrefetchHint::NTA);
static_assert(cl::make_config(64, 0, 1).sw_hint == cl::SwPrefetchHint::T0);

// KERN: ALLE 4 Achsen-Wrapper sind durch die Basen-Einwebung CacheLineConfigurable (erben cacheline_config()).
static_assert(cl::CacheLineConfigurable<Node>, "axis_04_node_type muss Cache-Line tragen");
static_assert(cl::CacheLineConfigurable<Search>, "axis_03a_search_algo muss Cache-Line tragen");
static_assert(cl::CacheLineConfigurable<Alloc>, "axis_06_allocator muss Cache-Line tragen");
static_assert(cl::CacheLineConfigurable<Layout>, "axis_05_memory_layout muss Cache-Line tragen");
static_assert(cl::CacheLineConfigurable<Page>, "axis_01_page_type muss Cache-Line tragen");

// Nicht-brechend: Default-Konfig der Wrapper = None → natürliches Alignment (kein erzwungenes Cache-Line-Padding).
static_assert(Node::cacheline_config() == cl::CacheLineConfig{});
static_assert(Search::cacheline_config() == cl::CacheLineConfig{});
static_assert(Alloc::cacheline_config() == cl::CacheLineConfig{});
static_assert(Layout::cacheline_config() == cl::CacheLineConfig{});
static_assert(Page::cacheline_config() == cl::CacheLineConfig{});

// HARMONISIERUNG axis_05: intrinsisches cache_line_size() (Deskriptor) bleibt unverändert NEBEN der
// permutierbaren Unterachse cacheline_config(); cacheline_subaxis_line_bytes() Default 64 (B64).
static_assert(Layout::cache_line_size() == 64);              // intrinsischer Deskriptor unverändert
static_assert(Layout::cacheline_subaxis_line_bytes() == 64); // Unterachsen-Default (B64)

static int g_fail = 0;
void       check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "KF-5: Cache-Line-Unterachse in die 5 CRTP-Basen eingewebt (echte Wrapper):\n";

    // Die Wrapper exponieren die geerbte CacheLineAware-API (Default-Konfig).
    check_true("Node4NodeType::cacheline_alignment == natural (Default None)",
               Node::cacheline_alignment() == alignof(std::max_align_t));
    check_true("Array256SearchAlgo cacheline-fähig (Default None)",
               Search::cacheline_alignment() == alignof(std::max_align_t));
    check_true("StdMalloc cacheline-fähig (Default None)", Alloc::cacheline_alignment() == alignof(std::max_align_t));
    check_true("CacheLineAlignedMemoryLayout cacheline-fähig (Default None)",
               Layout::cacheline_alignment() == alignof(std::max_align_t));
    check_true("axis_05 Harmonisierung: cache_line_size()==64 unverändert NEBEN Unterachse",
               Layout::cache_line_size() == 64 && Layout::cacheline_subaxis_line_bytes() == 64);
    check_true("DenseBytePageType cacheline-fähig (Default None)",
               Page::cacheline_alignment() == alignof(std::max_align_t));

    // Die ursprünglichen Achsen-APIs bleiben unverändert (nicht-brechend) — Stichproben.
    check_true("Node4 max_capacity unverändert (4)", Node::max_capacity() == 4);
    check_true("StdMalloc name unverändert (std_malloc)", Alloc::name() == std::string_view{"std_malloc"});

    // make_config + cacheline_alignment laufzeitseitig (eine distinkte per-Organ-Konfig).
    constexpr auto cfg = cl::make_config(128, 1, 1); // 128B, cacheline-aligned, T0-prefetch
    check_true("make_config(128,aligned,T0).alignment == 128", cl::alignment_bytes(cfg) == 128);
    check_true("make_config sw_hint == T0", cfg.sw_hint == cl::SwPrefetchHint::T0);

    std::cout << "\n==== KF-5 Cache-Line-Einwebung (5 Achsen): "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
