// test_kf7_runtime_loop — KF-7 (2026-06-02)
// Belegt: der dynamische Laufzeit-Durchlauf am Prüf-Dock (RuntimeVariableLoop) probiert auf EINER geladenen
// Binary nacheinander die dynamischen Variablen-Kombinationen via Algorithm_Resource_Control durch (KEIN Reload),
// geklammert auf tier-caps ∩ env-limits. Architektonische Ausnahmen (hw_prefetcher) → Label, aber kein POD-Feld.
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf7_runtime_loop.cpp

#include "builder/experiment_tree/runtime_variable_loop.hpp"
#include "builder/experiment_tree/experiment_tree.hpp"
#include "anatomy/resource_controllable_tier.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace ex = comdare::cache_engine::builder::experiment;
namespace an = comdare::cache_engine::anatomy;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}
void check_true(char const* what, bool c) { std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n"; if (!c) ++g_fail; }

// Mock-Tier: steuerbar thread_count (cap 4) + batch_size (cap 100); protokolliert jede Anwendung.
struct MockTier : an::IResourceControllableTier {
    std::vector<an::ComdareResourceControlV1> log;
    void tier_query_resource_caps(an::ComdareResourceControlV1* out) const noexcept override {
        *out = {}; out->thread_count = 4; out->batch_size = 100; out->controllable_axis_count = 2;
    }
    std::uint64_t tier_apply_resource_control(an::ComdareResourceControlV1 const* in) noexcept override {
        log.push_back(*in);
        std::uint64_t n = 0;
        if (in->thread_count) ++n; if (in->prefetch_distance) ++n; if (in->pool_budget_bytes) ++n;
        if (in->batch_size) ++n;   if (in->inline_threshold_bytes) ++n;
        return n;
    }
};

int main() {
    std::cout << "KF-7: dynamischer Laufzeit-Durchlauf am Prüf-Dock (eine Binary, N Einstellungen):\n";

    // ── Test 1: thread_count {1,2,8} auf EINER Binary, Clamp auf cap 4 ──
    {
        MockTier tier;
        ex::RuntimeVariableLoop loop{};  // keine env-Grenze
        std::vector<ex::DynamicDim> dims = { {"concurrency", "thread_count", {"1", "2", "8"}} };
        std::vector<ex::RuntimeSetting> got;
        std::size_t n = loop.run(tier, dims, [&](ex::RuntimeSetting const& s) { got.push_back(s); });

        check_eq("Einstellungen (3 Werte)", n, std::size_t{3});
        check_eq("EINE Binary, 3 Anwendungen (kein Reload)", tier.log.size(), std::size_t{3});
        check_eq("applied[0].thread_count", got[0].applied.thread_count, std::uint64_t{1});
        check_eq("applied[1].thread_count", got[1].applied.thread_count, std::uint64_t{2});
        check_eq("applied[2].thread_count (8 → clamp cap 4)", got[2].applied.thread_count, std::uint64_t{4});
        check_eq("requested[2].thread_count (vor Clamp = 8)", got[2].requested.thread_count, std::uint64_t{8});
        check_eq("applied_axis_count[0] (nur thread_count)", got[0].applied_axis_count, std::uint64_t{1});
        check_eq("Label[2]", got[2].setting_label, std::string{"concurrency.thread_count=8"});
    }

    // ── Test 2: env-Limit thread_count=2 klammert strenger als cap 4 ──
    {
        MockTier tier;
        an::ComdareResourceControlV1 env{}; env.thread_count = 2;
        ex::RuntimeVariableLoop loop{env};
        std::vector<ex::DynamicDim> dims = { {"concurrency", "thread_count", {"1", "2", "8"}} };
        std::vector<ex::RuntimeSetting> got;
        loop.run(tier, dims, [&](ex::RuntimeSetting const& s) { got.push_back(s); });
        check_eq("env-Limit: applied[2].thread_count (8 → clamp env 2)", got[2].applied.thread_count, std::uint64_t{2});
        check_eq("env-Limit: applied[1].thread_count (2 ≤ 2)", got[1].applied.thread_count, std::uint64_t{2});
    }

    // ── Test 3: KARTESIK zweier dynamischer Dims (thread_count × batch_size) ──
    {
        MockTier tier;
        ex::RuntimeVariableLoop loop{};
        std::vector<ex::DynamicDim> dims = {
            {"concurrency", "thread_count", {"1", "2"}},
            {"traversal",   "batch_size",   {"10", "20"}},
        };
        std::vector<ex::RuntimeSetting> got;
        std::size_t n = loop.run(tier, dims, [&](ex::RuntimeSetting const& s) { got.push_back(s); });
        check_eq("Kartesik 2×2 = 4 Einstellungen", n, std::size_t{4});
        check_eq("erste Einstellung tc=1,bs=10", got[0].applied.thread_count, std::uint64_t{1});
        check_eq("erste Einstellung batch_size=10", got[0].applied.batch_size, std::uint64_t{10});
        check_eq("applied_axis_count (2 Achsen aktiv)", got[0].applied_axis_count, std::uint64_t{2});
        check_eq("letzte Einstellung tc=2,bs=20", got[3].applied.batch_size, std::uint64_t{20});
    }

    // ── Test 4: architektonische Ausnahme hw_prefetcher → Label, aber KEIN POD-Feld (KF-12-Launcher-gated) ──
    {
        MockTier tier;
        ex::RuntimeVariableLoop loop{};
        std::vector<ex::DynamicDim> dims = { {"cacheline", "hw_prefetcher", {"off", "l2"}} };
        std::vector<ex::RuntimeSetting> got;
        loop.run(tier, dims, [&](ex::RuntimeSetting const& s) { got.push_back(s); });
        check_eq("hw_prefetcher: 2 Einstellungen (Label geführt)", got.size(), std::size_t{2});
        check_eq("hw_prefetcher: kein POD-Feld gesetzt (applied_axis_count 0)", got[1].applied_axis_count, std::uint64_t{0});
        check_eq("hw_prefetcher: Label[1]", got[1].setting_label, std::string{"cacheline.hw_prefetcher=l2"});
    }

    std::cout << "\n==== KF-7 Laufzeit-Durchlauf am Prüf-Dock: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
