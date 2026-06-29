#pragma once
// LockingStrategy - Concept fuer Concurrency-Modell des Allokators (REV 7 §1.4 A3+A4)
//
// Default: Single-Writer-Multi-Reader via std::shared_mutex (C++17)
// Optional: Cache-Page-Aware Multi-Writer via std::scoped_lock + Pro-Page-Mutex

#include <concepts>
#include <cstddef>

namespace comdare::cache_engine::allocator {

namespace locking_models {
// Default-Concurrency-Modell: 1 Writer, N Reader gleichzeitig
struct single_writer_multi_reader_tag {};

// Optional: Multi-Writer mit Cache-Page-Awareness (pro Cache-Page eigener Mutex)
struct cache_page_aware_multi_writer_tag {};

// Lock-Free (CAS-basiert, fuer A03/A04/A07/A11)
struct lock_free_tag {};

// Wait-Free (fuer A17 Crystalline Reclamation)
struct wait_free_tag {};

// Single-Thread (fuer A18 Exgen-Malloc, A19 Buddy original)
struct single_thread_tag {};
} // namespace locking_models

template <typename L>
concept LockingStrategy = requires(L l) {
    typename L::model_tag; // Pflicht: welches Modell?
    { l.read_lock_acquire() } -> std::same_as<void>;
    { l.read_lock_release() } -> std::same_as<void>;
    { l.write_lock_acquire() } -> std::same_as<void>;
    { l.write_lock_release() } -> std::same_as<void>;
};

// Cache-Page-Aware-Locking erweitert um Pro-Page-Mutex-Lookup
template <typename L>
concept CachePageAwareLockingStrategy = LockingStrategy<L> && requires(L l, std::size_t bytes) {
    { l.write_lock_for_size(bytes) } -> std::same_as<void>;
    { l.release_write_for_size(bytes) } -> std::same_as<void>;
};

} // namespace comdare::cache_engine::allocator
