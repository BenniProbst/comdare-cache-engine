#include "builder/measurement/thread_pinning.hpp"

#include "builder/measurement_snapshot.hpp"
#include <anatomy/observable_tier.hpp>
#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace b       = ::comdare::cache_engine::builder;
namespace anatomy = ::comdare::cache_engine::anatomy;

namespace {

[[nodiscard]] std::size_t count_cols(std::string const& csv_first_line) {
    return static_cast<std::size_t>(std::count(csv_first_line.begin(), csv_first_line.end(), ',') + 1);
}

[[nodiscard]] std::string first_line(std::string const& s) { return s.substr(0, s.find('\n')); }

#if defined(_WIN32)

[[nodiscard]] std::optional<DWORD_PTR> process_affinity_mask() {
    DWORD_PTR process_mask = 0;
    DWORD_PTR system_mask  = 0;
    if (::GetProcessAffinityMask(::GetCurrentProcess(), &process_mask, &system_mask) == 0 || process_mask == 0) {
        return std::nullopt;
    }
    return process_mask;
}

[[nodiscard]] std::optional<DWORD_PTR> current_thread_affinity_mask() {
    auto const process_mask = process_affinity_mask();
    if (!process_mask.has_value()) return std::nullopt;

    DWORD_PTR const previous = ::SetThreadAffinityMask(::GetCurrentThread(), *process_mask);
    if (previous == 0) return std::nullopt;

    (void)::SetThreadAffinityMask(::GetCurrentThread(), previous);
    return previous;
}

[[nodiscard]] bool masks_equal(DWORD_PTR lhs, DWORD_PTR rhs) { return lhs == rhs; }

[[nodiscard]] std::string mask_to_text(DWORD_PTR mask) {
    std::ostringstream os;
    os << "0x" << std::hex << std::uppercase << static_cast<unsigned long long>(mask);
    return os.str();
}

#elif defined(__linux__)

[[nodiscard]] std::optional<cpu_set_t> current_thread_affinity_mask() {
    cpu_set_t mask{};
    if (::sched_getaffinity(0, sizeof(mask), &mask) != 0) return std::nullopt;
    return mask;
}

[[nodiscard]] bool masks_equal(cpu_set_t const& lhs, cpu_set_t const& rhs) {
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &lhs) != CPU_ISSET(cpu, &rhs)) return false;
    }
    return true;
}

[[nodiscard]] bool mask_contains_core(cpu_set_t const& mask, int core) { return CPU_ISSET(core, &mask) != 0; }

[[nodiscard]] bool mask_is_only_core(cpu_set_t const& mask, int core) {
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        bool const expected = (cpu == core);
        if ((CPU_ISSET(cpu, &mask) != 0) != expected) return false;
    }
    return true;
}

[[nodiscard]] std::string mask_to_text(cpu_set_t const& mask) {
    std::string out{"{"};
    bool        first = true;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &mask) == 0) continue;
        if (!first) out += ',';
        out += std::to_string(cpu);
        first = false;
    }
    if (first) out += "empty";
    out += '}';
    return out;
}

#endif

} // namespace

TEST(AP13ThreadPinning, NeutralityGuardsStayIntact) {
    static_assert(std::is_trivially_copyable_v<b::ComdareMeasurementSnapshotV1>);
    static_assert(std::is_trivially_copyable_v<anatomy::ComdareTierObserverSnapshot>);

    EXPECT_EQ(COMDARE_ANATOMY_ABI_MAJOR, 4);
    EXPECT_EQ(sizeof(anatomy::ComdareTierObserverSnapshot), 1416u);
    EXPECT_EQ(anatomy::kTierObserverSnapshotVersionUnified, 5u);

    std::vector<b::ComdareMeasurementSnapshotV1> rows(1);
    std::vector<std::string> ids{"neutrality_guard"};
    std::vector<std::string> workloads{"ap9"};

    auto const full_csv = b::serialize_measurements_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(full_csv)), 25u);

    auto const pipeline_csv = b::serialize_measurements_pipeline16_csv(rows, ids, workloads);
    EXPECT_EQ(count_cols(first_line(pipeline_csv)), 16u);
}

TEST(AP13ThreadPinning, NoPinPolicyIsInactiveAndLeavesThreadMaskUnchanged) {
#if defined(_WIN32) || defined(__linux__)
    auto const before = current_thread_affinity_mask();
    ASSERT_TRUE(before.has_value());

    {
        auto guard = b::NoPinPolicy{}.pin();
        EXPECT_FALSE(guard.active());

        auto const during = current_thread_affinity_mask();
        ASSERT_TRUE(during.has_value());
        EXPECT_TRUE(masks_equal(*before, *during));
    }

    auto const after = current_thread_affinity_mask();
    ASSERT_TRUE(after.has_value());
    EXPECT_TRUE(masks_equal(*before, *after));
#else
    auto guard = b::NoPinPolicy{}.pin();
    EXPECT_FALSE(guard.active());
#endif
}

TEST(AP13ThreadPinning, CorePinPolicyPinsCoreZeroAndRestoresPreviousMask) {
#if defined(_WIN32)
    auto const before = current_thread_affinity_mask();
    ASSERT_TRUE(before.has_value());

    auto const process_mask = process_affinity_mask();
    ASSERT_TRUE(process_mask.has_value());
    constexpr DWORD_PTR kCore0Mask = DWORD_PTR{1};
    if ((*process_mask & kCore0Mask) == 0) {
        GTEST_SKIP() << "Core 0 is outside the process affinity mask";
    }

    std::optional<DWORD_PTR> pinned;
    {
        auto guard = b::CorePinPolicy{0}.pin();
        ASSERT_TRUE(guard.active());
        pinned = current_thread_affinity_mask();
        ASSERT_TRUE(pinned.has_value());
        EXPECT_EQ(*pinned, kCore0Mask);
    }

    auto const restored = current_thread_affinity_mask();
    ASSERT_TRUE(restored.has_value());
    EXPECT_TRUE(masks_equal(*before, *restored));
    std::cout << "AP13 CorePin RoundTrip: before=" << mask_to_text(*before)
              << " pinned=" << mask_to_text(*pinned) << " restored=" << mask_to_text(*restored) << "\n";
#elif defined(__linux__)
    auto const before = current_thread_affinity_mask();
    ASSERT_TRUE(before.has_value());
    if (!mask_contains_core(*before, 0)) {
        GTEST_SKIP() << "Core 0 is outside the process affinity mask";
    }

    std::optional<cpu_set_t> pinned;
    {
        auto guard = b::CorePinPolicy{0}.pin();
        ASSERT_TRUE(guard.active());
        pinned = current_thread_affinity_mask();
        ASSERT_TRUE(pinned.has_value());
        EXPECT_TRUE(mask_is_only_core(*pinned, 0));
    }

    auto const restored = current_thread_affinity_mask();
    ASSERT_TRUE(restored.has_value());
    EXPECT_TRUE(masks_equal(*before, *restored));
    std::cout << "AP13 CorePin RoundTrip: before=" << mask_to_text(*before)
              << " pinned=" << mask_to_text(*pinned) << " restored=" << mask_to_text(*restored) << "\n";
#else
    auto guard = b::CorePinPolicy{0}.pin();
    EXPECT_FALSE(guard.active());
    SUCCEED();
#endif
}