#pragma once
// AP-7b/#262 -- SwissGroupPoolStore: faithful SwissTable slot/control-byte substrate.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Faithful port of the AP-7a SwissTable wrapper substrate from axis_03a_search_algo_swisstable.hpp:
// slots_ + ctrl_ split storage, mask_, size_, tombstones_, 7/8 rehash threshold, and tombstone reuse.
// This mirrors the HashBucketPoolStore responsibility split: the store manages slots/control bytes and
// rehash; the scalar H1/H2 group-probe search logic lives in SwissGroupProbeTraversalOrgan.
//
// A8-S5 Familie 01a (2026-08-04): Slot- und Control-Byte-Speicher kommen REAL aus der Allocator-Achse
// (axis_06), Muster BTreeNodePoolStore. Der Allokator-Template-Kopf bestand schon; geflippt wurde die
// BINDUNG (Default + Rebind ueber den StdAllocatorAdapter der Achse statt ueber std::allocator).
// COW-Kopie rebindet beide Vektoren an das eigene allocator_ und verwirft die Kopier-Pollution per
// restore_statistics-Memento; Move ist nicht deklariert -> degradiert sicher zu Copy.

#include "swiss_group_pool_concept.hpp"
#include <axes/alloc/axis_06_allocator_exgen.hpp>            // axis_06-Default-Strategie + StdAllocatorAdapter
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp> // AllocatorStrategy-Concept (compile-time-strikt)

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

struct SwissSlot {
    std::uint64_t key{};
    std::uint64_t val{};
};

} // namespace detail

template <class Alloc = ::comdare::cache_engine::alloc::ExgenAllocator>
    requires ::comdare::cache_engine::alloc::concepts::AllocatorStrategy<Alloc>
class SwissGroupPoolStore {
public:
    using key_type            = std::uint64_t;
    using value_type          = std::uint64_t;
    using node_type           = detail::SwissSlot;
    using Slot                = detail::SwissSlot;
    using allocator_type      = Alloc;
    using slot_allocator_type = typename Alloc::template StdAllocatorAdapter<Slot>;
    using ctrl_allocator_type = typename Alloc::template StdAllocatorAdapter<std::uint8_t>;

    static constexpr std::uint64_t kFibonacciMul    = 11400714819323198485ULL;
    static constexpr std::size_t   kGroupWidth      = 16;
    static constexpr std::size_t   kInitialCapacity = 16;
    static constexpr std::uint8_t  kEmpty           = 0x80u;
    static constexpr std::uint8_t  kDeleted         = 0xFEu;

    SwissGroupPoolStore()
        : slots_(kInitialCapacity, Slot{}, allocator_.template as_std_allocator<Slot>()),
          ctrl_(kInitialCapacity, kEmpty, allocator_.template as_std_allocator<std::uint8_t>()),
          mask_(kInitialCapacity - 1) {}
    // COW-Pflicht (Memento): allocator_ mitkopieren, beide Vektoren an DAS EIGENE allocator_ rebinden,
    // Kopier-Pollution per restore_statistics verwerfen. Move NICHT deklariert -> degradiert zu Copy.
    SwissGroupPoolStore(SwissGroupPoolStore const& o)
        : allocator_(o.allocator_), slots_(o.slots_, allocator_.template as_std_allocator<Slot>()),
          ctrl_(o.ctrl_, allocator_.template as_std_allocator<std::uint8_t>()), mask_(o.mask_), size_(o.size_),
          tombstones_(o.tombstones_) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }
    SwissGroupPoolStore& operator=(SwissGroupPoolStore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter setzt keine propagate_-Typedefs) -> slots_/ctrl_ behalten ihr an
            // this-allocator_ gebundenes Adapter; die Assigns re-allozieren transient ueber this-allocator_.
            slots_      = o.slots_;
            ctrl_       = o.ctrl_;
            mask_       = o.mask_;
            size_       = o.size_;
            tombstones_ = o.tombstones_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }
    ~SwissGroupPoolStore() = default;

    [[nodiscard]] std::size_t slot_count() const noexcept { return mask_ + 1u; }
    [[nodiscard]] std::size_t group_count() const noexcept { return slot_count() / kGroupWidth; }
    [[nodiscard]] std::size_t occupied() const noexcept { return size_; }
    [[nodiscard]] std::size_t tombstones() const noexcept { return tombstones_; }
    [[nodiscard]] bool        needs_rehash_for_insert() const noexcept {
        return (size_ + tombstones_ + 1u) * 8u >= slot_count() * 7u;
    }

    [[nodiscard]] std::uint8_t        control_byte(std::size_t i) const noexcept { return ctrl_[i]; }
    [[nodiscard]] std::uint8_t const* control_group_ptr(std::size_t group_start) const noexcept {
        return ctrl_.data() + group_start;
    }
    [[nodiscard]] bool slot_is_empty(std::size_t i) const noexcept { return ctrl_[i] == kEmpty; }
    [[nodiscard]] bool slot_is_deleted(std::size_t i) const noexcept { return ctrl_[i] == kDeleted; }
    [[nodiscard]] bool slot_is_occupied(std::size_t i) const noexcept {
        return ctrl_[i] != kEmpty && ctrl_[i] != kDeleted;
    }
    [[nodiscard]] key_type   slot_key(std::size_t i) const noexcept { return slots_[i].key; }
    [[nodiscard]] value_type slot_value(std::size_t i) const noexcept { return slots_[i].val; }

    void place_occupied(std::size_t i, key_type k, value_type v, std::uint8_t h2) noexcept {
        if (ctrl_[i] == kDeleted) --tombstones_;
        ctrl_[i]  = h2;
        slots_[i] = Slot{k, v};
        ++size_;
    }

    void set_slot_value(std::size_t i, value_type v) noexcept { slots_[i].val = v; }

    void mark_deleted(std::size_t i) noexcept {
        ctrl_[i]  = kDeleted;
        slots_[i] = Slot{};
        --size_;
        ++tombstones_;
    }

    /// [[allocation-failure-exception]]: rehash kann werfen. KAUSALITAET (Posten 64/70, 2026-08-04): der
    /// Wurf kommt seit dem A8-S5-Schnitt vom StdAllocatorAdapter der Allokator-ACHSE -- die Strategie meldet
    /// OOM per nullptr, der Adapter uebersetzt ihn an EINER Stelle in std::bad_alloc
    /// (axis_06_allocator_strategy_base.hpp). Fehlerklasse bleibt der FK-5-Boden der Achse.
    void rehash(std::size_t new_capacity) {
        // Die Zwischenpuffer haengen ebenfalls an der Achse (der Adapter ist nicht default-konstruierbar) --
        // sonst waere ausgerechnet die groesste Umschaufel-Allokation an der Achse vorbeigelaufen.
        std::vector<Slot, slot_allocator_type>         old_slots(allocator_.template as_std_allocator<Slot>());
        std::vector<std::uint8_t, ctrl_allocator_type> old_ctrl(allocator_.template as_std_allocator<std::uint8_t>());
        old_slots.swap(slots_);
        old_ctrl.swap(ctrl_);
        slots_.assign(new_capacity, Slot{});
        ctrl_.assign(new_capacity, kEmpty);
        mask_       = new_capacity - 1u;
        size_       = 0;
        tombstones_ = 0;
        for (std::size_t i = 0; i < old_ctrl.size(); ++i) {
            if (old_ctrl[i] == kEmpty || old_ctrl[i] == kDeleted) continue;
            insert_rehashed(old_slots[i].key, old_slots[i].val);
        }
    }

    void clear() noexcept {
        for (auto& s : slots_) s = Slot{};
        std::fill(ctrl_.begin(), ctrl_.end(), kEmpty);
        size_       = 0;
        tombstones_ = 0;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename Alloc::snapshot_t;
    /// T6-Route (A8-S5): die ECHTE Allocator-Achsen-Statistik (rich AllocationStatistics, 5 Felder) statt der
    /// frueheren Store-eigenen Capacity-Delta-Schaetzung (allocator-UNABHAENGIG -> zweite, driftende Wahrheit).
    /// live_nodes speist der ABI-Adapter im Rich-Zweig aus occupied_count() des Organs.
    [[nodiscard]] allocator_snapshot_t store_allocator_statistics() const noexcept { return allocator_.statistics(); }

    template <class IsaOrgan>
    std::uint64_t organ_observe_isa(IsaOrgan& org) const
        requires requires(IsaOrgan& o, unsigned char const* b, std::size_t words) {
            o.observe_simd_field_sum(b, words);
        }
    {
        std::uint64_t acc = 0;
        if (!slots_.empty()) {
            std::size_t const words = (slots_.size() * sizeof(Slot)) / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(reinterpret_cast<unsigned char const*>(slots_.data()), words);
        }
        if (!ctrl_.empty()) {
            std::size_t const words = ctrl_.size() / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(reinterpret_cast<unsigned char const*>(ctrl_.data()), words);
        }
        return acc;
    }
#endif

private:
    static constexpr std::size_t kNpos = static_cast<std::size_t>(-1);

    [[nodiscard]] static constexpr std::uint64_t mixed_hash(key_type k) noexcept {
        return static_cast<std::uint64_t>(k) * kFibonacciMul;
    }

    [[nodiscard]] static constexpr std::uint8_t h2_fingerprint(std::uint64_t hash) noexcept {
        return static_cast<std::uint8_t>(hash & 0x7Fu);
    }

    [[nodiscard]] std::size_t group_start_for(std::uint64_t hash, std::size_t probe) const noexcept {
        std::size_t const group_mask = group_count() - 1u;
        std::size_t const h1_group   = static_cast<std::size_t>(hash >> 7u) & group_mask;
        return ((h1_group + probe) & group_mask) * kGroupWidth;
    }

    void insert_rehashed(key_type k, value_type v) {
        std::uint64_t const hash          = mixed_hash(k);
        std::uint8_t const  h2            = h2_fingerprint(hash);
        std::size_t const   groups        = group_count();
        std::size_t         first_deleted = kNpos;
        for (std::size_t probe = 0; probe < groups; ++probe) {
            std::size_t const group_start = group_start_for(hash, probe);
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == h2 && slots_[pos].key == k) {
                    slots_[pos].val = v;
                    return;
                }
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == kDeleted && first_deleted == kNpos) first_deleted = pos;
            }
            for (std::size_t i = 0; i < kGroupWidth; ++i) {
                std::size_t const pos = group_start + i;
                if (ctrl_[pos] == kEmpty) {
                    std::size_t const target = (first_deleted != kNpos) ? first_deleted : pos;
                    if (first_deleted != kNpos) --tombstones_;
                    ctrl_[target]  = h2;
                    slots_[target] = Slot{k, v};
                    ++size_;
                    return;
                }
            }
        }

        rehash(slot_count() * 2u);
        insert_rehashed(k, v);
    }

    // allocator_ VOR den Vektoren (der Adapter haelt &allocator_ -- Init-/Destruktions-Reihenfolge).
    Alloc                                          allocator_{};
    std::vector<Slot, slot_allocator_type>         slots_;
    std::vector<std::uint8_t, ctrl_allocator_type> ctrl_;
    std::size_t                                    mask_;
    std::size_t                                    size_       = 0;
    std::size_t                                    tombstones_ = 0;
};

static_assert(SwissGroupPool<SwissGroupPoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
