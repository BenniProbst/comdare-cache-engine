// test_kf3_cacheline_subaxis — KF-3 (2026-06-02)
// Standalone-Test (kein gtest) fuer die per-Organ cacheline-Unterachse.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf3_cacheline_subaxis.cpp

#include "axes/cacheline/cacheline_config.hpp"

#include <cstddef>
#include <iostream>
#include <set>
#include <string>

namespace cl = comdare::cache_engine::cacheline;

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

// Fake-Organ: per-Organ-Config als NTTP (compile-time gebacken).
struct FakeOrganA : cl::CacheLineAware<cl::CacheLineConfig{cl::CacheLineSize::B128, cl::CacheLineAlignment::Padded,
                                                           cl::SwPrefetchHint::T0}> {};
struct FakeOrganB : cl::CacheLineAware<cl::CacheLineConfig{cl::CacheLineSize::B64, cl::CacheLineAlignment::None,
                                                           cl::SwPrefetchHint::None}> {};

// Concept muss erfuellt sein (compile-time).
static_assert(cl::CacheLineConfigurable<FakeOrganA>, "FakeOrganA muss CacheLineConfigurable sein");
static_assert(cl::CacheLineConfigurable<FakeOrganB>, "FakeOrganB muss CacheLineConfigurable sein");

int main() {
    // alignment_bytes
    check_eq("alignment_bytes(None) == max_align",
             cl::alignment_bytes(
                 cl::CacheLineConfig{cl::CacheLineSize::B64, cl::CacheLineAlignment::None, cl::SwPrefetchHint::None}),
             alignof(std::max_align_t));
    check_eq("alignment_bytes(B128,Aligned) == 128",
             cl::alignment_bytes(cl::CacheLineConfig{cl::CacheLineSize::B128, cl::CacheLineAlignment::CacheLineAligned,
                                                     cl::SwPrefetchHint::None}),
             std::size_t{128});
    check_eq("alignment_bytes(B256,Padded) == 256",
             cl::alignment_bytes(cl::CacheLineConfig{cl::CacheLineSize::B256, cl::CacheLineAlignment::Padded,
                                                     cl::SwPrefetchHint::None}),
             std::size_t{256});

    // Enumeration: 60 distinkte Configs.
    // BEWUSSTE Pin-Fortschreibung 45 -> 60 (C1, GO4/#8 F-C, 2026-07-12): das Werteset wurde ADDITIV
    // thesis-treu erweitert (KF-5 {32,64,128} ⊂ neuem {B32,B64,B128,B256}; B256 bleibt). 4 x 3 x 5 = 60.
    // Der B32-Block ist ANGEHAENGT: Indizes [0..44] tragen exakt die bisherigen 45 Konfigurationen.
    auto configs = cl::all_configs();
    check_eq("all_configs().size()", configs.size(), std::size_t{60});
    std::set<std::tuple<int, int, int>> uniq;
    for (auto c : configs)
        uniq.insert({static_cast<int>(c.line_size), static_cast<int>(c.alignment), static_cast<int>(c.sw_hint)});
    check_eq("all_configs distinkt", uniq.size(), std::size_t{60});
    // C1-Index-Stabilitaet: [0] bleibt der bisherige Anfang (B64-Block), [45] beginnt der neue B32-Block.
    check_true("all_configs[0].line_size == B64 (alte Indizes stabil)", configs[0].line_size == cl::CacheLineSize::B64);
    check_true("all_configs[44].line_size == B256 (Ende des alten Blocks)",
               configs[44].line_size == cl::CacheLineSize::B256);
    check_true("all_configs[45].line_size == B32 (neuer Block angehaengt)",
               configs[45].line_size == cl::CacheLineSize::B32);
    // C1: make_config kennt 32 additiv; alignment_bytes(B32, Aligned) == 32.
    check_true("make_config(32,1,0).line_size == B32", cl::make_config(32, 1, 0).line_size == cl::CacheLineSize::B32);
    check_eq("alignment_bytes(B32,Aligned) == 32",
             cl::alignment_bytes(cl::CacheLineConfig{cl::CacheLineSize::B32, cl::CacheLineAlignment::CacheLineAligned,
                                                     cl::SwPrefetchHint::None}),
             std::size_t{32});

    // Per-Organ-Mixin: jedes Organ traegt seine EIGENE Config
    check_true("OrganA.config == {B128,Padded,T0}",
               FakeOrganA::cacheline_config() == cl::CacheLineConfig{cl::CacheLineSize::B128,
                                                                     cl::CacheLineAlignment::Padded,
                                                                     cl::SwPrefetchHint::T0});
    check_eq("OrganA.cacheline_alignment()", FakeOrganA::cacheline_alignment(), std::size_t{128});
    check_eq("OrganB.cacheline_alignment() (None->max_align)", FakeOrganB::cacheline_alignment(),
             alignof(std::max_align_t));
    check_true("OrganA != OrganB Config", !(FakeOrganA::cacheline_config() == FakeOrganB::cacheline_config()));

    // Prefetch laeuft (kein Crash), compile-time Hint gebacken
    alignas(64) unsigned char buf[256] = {};
    FakeOrganA::cacheline_prefetch(buf);
    cl::prefetch<cl::SwPrefetchHint::NTA>(buf + 64);
    cl::prefetch<cl::SwPrefetchHint::None>(buf); // no-op
    check_true("prefetch ausgefuehrt ohne Crash", true);

    std::cout << "\n==== KF-3 cacheline-Unterachse: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
