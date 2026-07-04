// SPDX-License-Identifier: Apache-2.0
#pragma once

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX // sonst leaken die windows.h-Makros min/max und brechen z.B. std::numeric_limits<T>::max() bei Konsumenten
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sched.h>
#endif

namespace comdare::cache_engine::builder {

class ScopedThreadPin {
public:
    ScopedThreadPin() noexcept = default;
    explicit ScopedThreadPin(unsigned core) noexcept { pin(core); }

    ~ScopedThreadPin() noexcept { restore(); }

    ScopedThreadPin(ScopedThreadPin const&)            = delete;
    ScopedThreadPin& operator=(ScopedThreadPin const&) = delete;

    ScopedThreadPin(ScopedThreadPin&& other) noexcept { move_from(other); }

    ScopedThreadPin& operator=(ScopedThreadPin&& other) noexcept {
        if (this != &other) {
            restore();
            move_from(other);
        }
        return *this;
    }

    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    void pin(unsigned core) noexcept {
#if defined(_WIN32)
        constexpr unsigned kMaskBits = static_cast<unsigned>(sizeof(DWORD_PTR) * 8u);
        if (core >= kMaskBits) return;

        DWORD_PTR const target_mask = (DWORD_PTR{1} << core);
        DWORD_PTR const previous    = ::SetThreadAffinityMask(::GetCurrentThread(), target_mask);
        if (previous == 0) return;

        previous_mask_ = previous;
        active_        = true;
#elif defined(__linux__)
        if (core >= static_cast<unsigned>(CPU_SETSIZE)) return;

        cpu_set_t previous{};
        if (::sched_getaffinity(0, sizeof(previous), &previous) != 0) return;

        cpu_set_t target{};
        CPU_ZERO(&target);
        CPU_SET(core, &target);
        if (::sched_setaffinity(0, sizeof(target), &target) != 0) return;

        previous_mask_ = previous;
        active_        = true;
#else
        (void)core;
#endif
    }

    void restore() noexcept {
        if (!active_) return;
#if defined(_WIN32)
        (void)::SetThreadAffinityMask(::GetCurrentThread(), previous_mask_);
#elif defined(__linux__)
        (void)::sched_setaffinity(0, sizeof(previous_mask_), &previous_mask_);
#endif
        active_ = false;
    }

    void move_from(ScopedThreadPin& other) noexcept {
#if defined(_WIN32) || defined(__linux__)
        previous_mask_ = other.previous_mask_;
#endif
        active_       = other.active_;
        other.active_ = false;
    }

#if defined(_WIN32)
    DWORD_PTR previous_mask_{};
#elif defined(__linux__)
    cpu_set_t previous_mask_{};
#endif
    bool active_{false};
};

struct NoPinPolicy {
    [[nodiscard]] ScopedThreadPin pin() const noexcept { return {}; }
};

struct CorePinPolicy {
    unsigned core{};

    [[nodiscard]] ScopedThreadPin pin() const noexcept { return ScopedThreadPin{core}; }
};

} // namespace comdare::cache_engine::builder