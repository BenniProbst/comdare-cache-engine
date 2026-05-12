#pragma once
// CachePageAwareLock - Multi-Writer mit Pro-Cache-Page-Mutex (REV 7 §3.3)
//
// Verwendet std::scoped_lock (C++17) ueber pro-Page-Mutexe. Vermeidet
// Cross-Page-Contention bei Multi-Threading. Optional-Variante des
// Comdare-Allokator-Concurrency-Modells (REV 7 §1.4 A4).

#include "../concepts/locking_concept.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <shared_mutex>

namespace comdare::cache_engine::allocator::locking {

// Pflicht-Konstante: Cache-Page-Groesse (typ. 4 KiB OS-Page; konfigurierbar)
inline constexpr std::size_t kCachePageBytes = 4096;

// Anzahl Page-Mutexe pro Lock-Manager (Power-of-2 fuer schnelle modulo)
inline constexpr std::size_t kPageMutexCount = 256;

class CachePageAwareLock {
public:
    using model_tag = locking_models::cache_page_aware_multi_writer_tag;

    // Globale Read/Write fuer komplette Allokator-Konfiguration
    void read_lock_acquire()  { global_.lock_shared(); }
    void read_lock_release()  { global_.unlock_shared(); }
    void write_lock_acquire() { global_.lock(); }
    void write_lock_release() { global_.unlock(); }

    // Pro-Page-Lock fuer Multi-Writer ohne globale Contention
    void write_lock_for_size(std::size_t bytes) {
        page_mutex_for_size(bytes).lock();
    }
    void release_write_for_size(std::size_t bytes) {
        page_mutex_for_size(bytes).unlock();
    }

    // RAII-Helper: Multi-Page-Akquisition deadlock-vermeidend via std::scoped_lock
    class MultiPageGuard {
    public:
        MultiPageGuard(CachePageAwareLock& parent, std::size_t bytes_a, std::size_t bytes_b)
            : guard_{parent.page_mutex_for_size(bytes_a),
                     parent.page_mutex_for_size(bytes_b)} {}
    private:
        std::scoped_lock<std::mutex, std::mutex> guard_;
    };

private:
    [[nodiscard]] std::mutex& page_mutex_for_size(std::size_t bytes) noexcept {
        // Hash-Mapping: bytes → Page-Mutex-Index
        std::uint64_t const page_id = static_cast<std::uint64_t>(bytes / kCachePageBytes);
        return page_mutexes_[page_id & (kPageMutexCount - 1)];
    }

    mutable std::shared_mutex            global_;
    std::array<std::mutex, kPageMutexCount> page_mutexes_;
};

static_assert(LockingStrategy<CachePageAwareLock>);
static_assert(CachePageAwareLockingStrategy<CachePageAwareLock>);

}  // namespace comdare::cache_engine::allocator::locking
