// test_br2_descriptor_foundation — BR-2-Fundament (2026-06-02, Doc 27 §6)
// Belegt die compile-time Knoten-Deskriptor-Hierarchie (Head-Concept → static/dynamic-CRTP-Base → per-Achse-
// Spezial, typsicher, KEIN struct). Umbrella-unabhängig. Build: cl /std:c++latest /EHsc /I<libs/cache_engine>

#include "builder/experiment_tree/axis_node_descriptor.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <tuple>

namespace ex = comdare::cache_engine::builder::experiment;

// Per-Achse-SPEZIAL-Deskriptor (statisch), erbt die CRTP-Base + liefert typisierte Achsen-Properties.
struct AllocatorTestDescriptor : ex::StaticAxisDescriptorBase<AllocatorTestDescriptor> {
    static constexpr std::string_view axis_name() noexcept { return "allocator"; }
    static constexpr std::string_view block_id() noexcept { return "allocator"; }
    using variants = std::tuple<int, double, char>; // Platzhalter für die Enabled-mp_list (Größe 3)
    // achsen-eigene typsichere Property (Beispiel):
    static constexpr std::size_t max_alignment() noexcept { return 64; }
};

// Per-Achse-SPEZIAL-Deskriptor (dynamisch).
struct ConcurrencyDynTestDescriptor : ex::DynamicAxisDescriptorBase<ConcurrencyDynTestDescriptor> {
    static constexpr std::string_view axis_name() noexcept { return "concurrency"; }
    static constexpr std::string_view block_id() noexcept { return "concurrency"; }
    static constexpr std::string_view variable() noexcept { return "thread_count"; }
};

// ── Compile-time Belege ──
static_assert(ex::AxisNodeDescriptor<AllocatorTestDescriptor>, "Spezial-Deskriptor erfüllt Head-Concept");
static_assert(ex::StaticAxisNodeDescriptor<AllocatorTestDescriptor>, "ist statischer Deskriptor (mit variants)");
static_assert(!ex::DynamicAxisNodeDescriptor<AllocatorTestDescriptor>, "ist NICHT dynamisch");
static_assert(ex::AxisNodeDescriptor<ConcurrencyDynTestDescriptor>, "dyn. Spezial-Deskriptor erfüllt Head-Concept");
static_assert(ex::DynamicAxisNodeDescriptor<ConcurrencyDynTestDescriptor>,
              "ist dynamischer Deskriptor (mit variable())");
static_assert(!ex::StaticAxisNodeDescriptor<ConcurrencyDynTestDescriptor>, "ist NICHT statisch");
static_assert(AllocatorTestDescriptor::descriptor_kind() == ex::DescriptorKind::Static);
static_assert(ConcurrencyDynTestDescriptor::descriptor_kind() == ex::DescriptorKind::Dynamic);
static_assert(std::tuple_size_v<AllocatorTestDescriptor::variants> == 3);

static int g_fail = 0;
void       check_true(char const* what, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!c) ++g_fail;
}

int main() {
    std::cout << "BR-2-Fundament: compile-time per-Achse Spezial-Deskriptoren:\n";
    check_true("static: descriptor_kind == Static",
               AllocatorTestDescriptor::descriptor_kind() == ex::DescriptorKind::Static);
    check_true("static: axis_name == allocator", AllocatorTestDescriptor::axis_name() == "allocator");
    check_true("static: block_id == allocator (Bidir.-Anker)", AllocatorTestDescriptor::block_id() == "allocator");
    check_true("static: typisierte Property max_alignment==64", AllocatorTestDescriptor::max_alignment() == 64);
    check_true("dynamic: descriptor_kind == Dynamic",
               ConcurrencyDynTestDescriptor::descriptor_kind() == ex::DescriptorKind::Dynamic);
    check_true("dynamic: variable == thread_count", ConcurrencyDynTestDescriptor::variable() == "thread_count");

    std::cout << "\n==== BR-2-Fundament Deskriptor-Hierarchie: "
              << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
