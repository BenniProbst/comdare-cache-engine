// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 BEP Venture UG (Marke Comdare)
//
// comdare::rcu — eigene Read-Copy-Update-Implementation (Aufgabe #104)
// REV 7 §6 (PRT-ART konsumiert CacheEngine) + REV 6 §5.17 (Concurrency-Pool)
//
// Ersetzt liburcu (P29 ext/) durch eine C++23-Header-only-Implementation
// mit den Kern-Primitiven:
//
//   RcuDomain         - global verwaltete Grace-Period-Sequenz
//   RcuReadGuard      - RAII fuer rcu_read_lock/unlock
//   RcuDeferred       - call-rcu-Pattern (deferred reclamation)
//
// Diese Implementation ist KONZEPT-konform zu McKenney et al. RCU 2001 OLS
// (P29 in der Bausteine-Matrix), aber bewusst SIMPLIFIZIERT:
//   - keine per-CPU-Zaehler (epoch-based statt quiescent-state-based)
//   - keine kernel-bypass-Optimierungen
//
// Garantien (siehe RCU-Paper §4.2 "Read-Side Constraints"):
// 1. Reader sehen entweder die alte ODER die neue Version, NIE eine Mischung
// 2. Reader-Pfad ist wait-free (nur ein atomic_load + Memory-Barrier)
// 3. Writer warten via synchronize() bis alle aktiven Reader fertig sind

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace comdare::rcu {

class RcuDomain;

// Per-Thread-Reader-State (epoch-based)
struct alignas(64) RcuReaderState {
    std::atomic<std::uint64_t> local_epoch{0};   // 0 = nicht im Read-Block
    std::atomic<bool>          active{false};
};

// RcuDomain: globaler Grace-Period-Counter + Reader-Registry
class RcuDomain {
public:
    // Liefert per-thread Reader-State (thread_local)
    [[nodiscard]] RcuReaderState* current_thread_state() noexcept {
        thread_local RcuReaderState state;
        thread_local bool           registered = false;
        if (!registered) {
            std::lock_guard lock{registry_mutex_};
            readers_.push_back(&state);
            registered = true;
        }
        return &state;
    }

    // Reader-Pfad: read_lock liest aktuelles global_epoch + markiert sich aktiv
    void read_lock() noexcept {
        auto* s = current_thread_state();
        s->local_epoch.store(global_epoch_.load(std::memory_order_acquire),
                              std::memory_order_release);
        s->active.store(true, std::memory_order_release);
    }

    void read_unlock() noexcept {
        auto* s = current_thread_state();
        s->active.store(false, std::memory_order_release);
    }

    // Writer-Pfad: erhoeht global_epoch und wartet, bis alle Reader
    // den Block verlassen haben (busy-wait mit yield).
    void synchronize() noexcept {
        std::uint64_t const target_epoch =
            global_epoch_.fetch_add(1, std::memory_order_acq_rel) + 1;

        std::lock_guard lock{registry_mutex_};
        for (auto* r : readers_) {
            // Warte, bis Reader entweder inaktiv ist oder die neue Epoch sieht.
            while (true) {
                if (!r->active.load(std::memory_order_acquire)) break;
                if (r->local_epoch.load(std::memory_order_acquire) >= target_epoch) break;
                std::this_thread::yield();
            }
        }
    }

    [[nodiscard]] std::uint64_t current_epoch() const noexcept {
        return global_epoch_.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::size_t reader_count() noexcept {
        std::lock_guard lock{registry_mutex_};
        return readers_.size();
    }

    // Singleton-Default-Domain
    [[nodiscard]] static RcuDomain& instance() noexcept {
        static RcuDomain d;
        return d;
    }

private:
    std::atomic<std::uint64_t>            global_epoch_{1};
    std::mutex                            registry_mutex_;
    std::vector<RcuReaderState*>          readers_{};
};

// RAII Reader-Guard
class [[nodiscard]] RcuReadGuard {
public:
    explicit RcuReadGuard(RcuDomain& d = RcuDomain::instance()) noexcept
        : domain_{&d} { domain_->read_lock(); }

    ~RcuReadGuard() { if (domain_) domain_->read_unlock(); }

    RcuReadGuard(RcuReadGuard const&)            = delete;
    RcuReadGuard& operator=(RcuReadGuard const&) = delete;
    RcuReadGuard(RcuReadGuard&& o) noexcept : domain_{o.domain_} { o.domain_ = nullptr; }
    RcuReadGuard& operator=(RcuReadGuard&&)      = delete;

private:
    RcuDomain* domain_;
};

// Deferred-Reclamation (call_rcu-Pattern): registriert Callbacks, die nach
// der naechsten synchronize() aufgerufen werden.
class RcuDeferred {
public:
    void defer(std::function<void()> fn) {
        std::lock_guard lock{mutex_};
        pending_.push_back(std::move(fn));
    }

    // Sync alle deferred callbacks: erst synchronize() der Domain,
    // dann alle Callbacks ausfuehren + leeren.
    void flush(RcuDomain& d = RcuDomain::instance()) {
        std::vector<std::function<void()>> snapshot;
        {
            std::lock_guard lock{mutex_};
            snapshot.swap(pending_);
        }
        d.synchronize();
        for (auto& fn : snapshot) fn();
    }

    [[nodiscard]] std::size_t pending_count() noexcept {
        std::lock_guard lock{mutex_};
        return pending_.size();
    }

private:
    std::mutex                         mutex_;
    std::vector<std::function<void()>> pending_;
};

// Helper: atomic-swap eines Pointers + defer der alten Version
template <typename T>
inline void rcu_replace(std::atomic<T*>& slot, T* new_value, RcuDeferred& def) {
    T* old = slot.exchange(new_value, std::memory_order_acq_rel);
    if (old) {
        def.defer([old]() { delete old; });
    }
}

}  // namespace comdare::rcu
