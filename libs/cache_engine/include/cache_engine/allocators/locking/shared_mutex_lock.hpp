#pragma once
// SharedMutexLock - Default Single-Writer-Multi-Reader Lock (REV 7 §1.5)
//
// Implementiert via std::shared_mutex (C++17). Default fuer alle Comdare-
// Allokator-Bausteine sofern nicht explizit andere Variante gewaehlt.

#include "../concepts/locking_concept.hpp"

#include <shared_mutex>

namespace comdare::cache_engine::allocator::locking {

class SharedMutexLock {
public:
    using model_tag = locking_models::single_writer_multi_reader_tag;

    void read_lock_acquire() { mutex_.lock_shared(); }
    void read_lock_release() { mutex_.unlock_shared(); }
    void write_lock_acquire() { mutex_.lock(); }
    void write_lock_release() { mutex_.unlock(); }

private:
    mutable std::shared_mutex mutex_;
};

static_assert(LockingStrategy<SharedMutexLock>);

} // namespace comdare::cache_engine::allocator::locking
