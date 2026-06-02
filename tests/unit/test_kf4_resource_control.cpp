// test_kf4_resource_control — KF-4 (2026-06-02)
// Standalone-Test (kein gtest) fuer IResourceControllableTier + AlgorithmResourceControl.
//
// Build: cl /std:c++latest /EHsc /I<libs/cache_engine> test_kf4_resource_control.cpp

#include "anatomy/resource_controllable_tier.hpp"
#include "builder/algorithm_resource_control.hpp"

#include <iostream>
#include <string>

namespace an = comdare::cache_engine::anatomy;
namespace bu = comdare::cache_engine::builder;

static int g_fail = 0;
template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) { std::cout << "  (erwartet: " << want << ")"; ++g_fail; }
    std::cout << "\n";
}

// Fake-Tier: thread_count bis 8 + batch_size bis 1000 steuerbar, Rest nicht.
class FakeTier : public an::IResourceControllableTier {
public:
    an::ComdareResourceControlV1 applied{};
    void tier_query_resource_caps(an::ComdareResourceControlV1* out) const noexcept override {
        *out = an::ComdareResourceControlV1{};
        out->thread_count = 8;
        out->batch_size = 1000;
        out->controllable_axis_count = 2;
    }
    std::uint64_t tier_apply_resource_control(an::ComdareResourceControlV1 const* in) noexcept override {
        applied = *in;
        std::uint64_t n = 0;
        if (in->thread_count) ++n;
        if (in->batch_size) ++n;  // prefetch/pool/inline nicht steuerbar -> zaehlen nicht
        return n;
    }
};

int main() {
    bu::AlgorithmResourceControl ctrl;
    ctrl.desired.thread_count      = 4;    // gewuenscht 4
    ctrl.desired.batch_size        = 2000; // ueber Cap 1000
    ctrl.desired.prefetch_distance = 16;   // Achse nicht steuerbar (Cap 0)
    ctrl.env_limits.thread_count   = 2;    // System: nur 2 Kerne

    FakeTier tier;
    std::uint64_t applied = ctrl.apply_to(&tier);

    check_eq("thread_count auf env-Limit 2 geklammert", tier.applied.thread_count, std::uint64_t{2});
    check_eq("batch_size auf Cap 1000 geklammert",       tier.applied.batch_size,   std::uint64_t{1000});
    check_eq("prefetch_distance (nicht steuerbar) = 0",  tier.applied.prefetch_distance, std::uint64_t{0});
    check_eq("pool_budget (nicht gewuenscht) = 0",       tier.applied.pool_budget_bytes, std::uint64_t{0});
    check_eq("#angewandte Achsen",                        applied,                   std::uint64_t{2});

    // Degradierung: nullptr -> 0 (kein Crash)
    check_eq("apply_to(nullptr) = 0", ctrl.apply_to(nullptr), std::uint64_t{0});

    // clamp direkt: want=10, cap=8, env=0 -> 8 ; want=0 -> 0 ; cap=0 -> 0
    an::ComdareResourceControlV1 w{}; w.thread_count = 10; w.batch_size = 0; w.prefetch_distance = 5;
    an::ComdareResourceControlV1 caps{}; caps.thread_count = 8; caps.batch_size = 500; caps.prefetch_distance = 0;
    an::ComdareResourceControlV1 env{};
    auto e = bu::AlgorithmResourceControl::clamp(w, caps, env);
    check_eq("clamp: want10/cap8 -> 8",     e.thread_count,      std::uint64_t{8});
    check_eq("clamp: want0 -> 0",           e.batch_size,        std::uint64_t{0});
    check_eq("clamp: cap0 (unsteuerbar) -> 0", e.prefetch_distance, std::uint64_t{0});

    std::cout << "\n==== KF-4 Resource-Control-Test: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
