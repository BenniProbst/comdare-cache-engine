#pragma once
// Layout-honorierender Storage (2026-06-04, Plan dynamic-frolicking-truffle) — LayoutAwareChunkedStore<N,L,A>.
//
// @topic nodes @achse 04 @schicht composable-bridge (node_type ⊕ layout ⊕ allocator ALS Storage-Organ, L-WIRKSAM)
//
// **Zweck (User 2026-06-04 „layout-honorierenden Store bauen"):** `NodeChunkedStore` speichert Slots IMMER als
// `std::pair<uint64,uint64>` (16 B), UNABHÄNGIG von der memory_layout-Achse L → der Layout-Stride
// (cache_line_aligned = 64 B, aos_strict = 16 B) ist in der Speicherung NICHT abgebildet. Folge: die
// memory_layout-Achse ist nicht echt messbar, und `cache_line_aligned::scan_field_sum` liest bei Stride
// round_up(16,64)=64 über ein nur n*16 B großes Backing → Out-of-Bounds (latenter Bug in `organ_observe_layout`).
//
// LayoutAwareChunkedStore speichert die Records in einem **Byte-Backing am layout-getriebenen Stride** (`eff_stride`):
// jeder Record belegt `eff_stride` Bytes (Key 0..8, Value 8..16, Rest Padding 0). Damit ist
// - die memory_layout-Achse ECHT: cache_line_aligned (eff_stride=64) padded vs aos_strict (eff_stride=16) packed,
// - der `organ_observe_layout`-Scan OOB-frei (record_size == eff_stride == realer Backing-Stride),
// - die allocator-Achse layout-abhängig (CLA-Padding kostet echte Bytes).
//
// **Scope Phase 1 = AoS-Familie (aos_strict + cache_line_aligned), VOLL ehrlich.** SoA (columnar) / packed_bitmap
// (verdichtet) / aosoa (geblockt) haben Nicht-AoS-Zugriffsmuster — ihr eigener `scan_field_sum` matcht den
// eff_stride-Block NICHT; über diesen Store sind sie OOB-SICHER (eff_stride = round_up(16, cls) ≥ ihr Scan-Range),
// aber die Layout-DATEN-Treue für sie ist Phase 2 (eigene columnar/packed-Repräsentation + Concept-Erweiterung).
//
// Erfüllt das StorageOrgan-Concept (gemeinsamer uint64-Key, 8 Methoden, byte-codiert) → Drop-in für ComposedSearch,
// parallel zu NodeChunkedStore (das Bestehende bleibt unangetastet). Memento ist LOGISCH (save_state→(k,v)-Liste),
// daher braucht der Store keine trivial-serialisierbare Byte-Form.

#include "axis_04_node_type_node4.hpp"                 // Pilot-NodeType (Selbstbeweis)
#include "concepts/axis_04_node_type_concept.hpp"
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_concept.hpp>
#include <topics/allocator/axis_06_allocator/axis_06_allocator_mimalloc.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/storage_organ_concept.hpp>
#include <topics/traversal/axis_03a_search_algo/composable/composable_search.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

namespace comdare::cache_engine::node {

namespace _ml_la = ::comdare::cache_engine::memory_layout::axis_05_memory_layout;
namespace _al_la = ::comdare::cache_engine::allocator::axis_06_allocator;

/// layout-honorierendes, node-gechunktes 3-Achsen-Storage-Organ: Slots liegen als eff_stride-Byte-Records in
/// node-großen Chunks der Kapazität N::max_capacity(). eff_stride = der layout-getriebene Record-Stride.
template <class N, class L, class A>
    requires concepts::NodeTypeStrategy<N>
          && _ml_la::concepts::MemoryLayoutStrategy<L>
          && _al_la::concepts::AllocatorStrategy<A>
class LayoutAwareChunkedStore {
private:
    static constexpr std::size_t kKvBytes = 16;   // logische Record-Nutzlast: uint64 Key + uint64 Value
    static constexpr std::size_t round_up_(std::size_t v, std::size_t a) noexcept {
        return a <= 1 ? v : ((v + a - 1u) & ~(a - 1u));
    }
    // eff_stride: bevorzugt die Strategie-eigene effective_stride(record_size) (single source, Phase-2-fähig);
    // sonst die kanonische Formel round_up(record_size, L::cache_line_size()) (deckt aos_strict cls=1→16 UND
    // cache_line_aligned cls=64→64 exakt ab — identisch zu cache_line_aligned::scan_field_sum).
    static constexpr std::size_t eff_stride_compute_() noexcept {
        if constexpr (requires { L::effective_stride(kKvBytes); }) return L::effective_stride(kKvBytes);
        else return round_up_(kKvBytes, L::cache_line_size());
    }

public:
    using key_type       = std::uint64_t;   // GEMEINSAMER breiter Key (Doku-24-§5.5)
    using value_type     = std::uint64_t;
    using node_type      = N;
    using layout_type    = L;
    using allocator_type = A;

    static constexpr std::size_t cap_           = (N::max_capacity() == 0 ? std::size_t{1} : N::max_capacity());
    static constexpr std::size_t eff_stride     = eff_stride_compute_();   // layout-getriebener Record-Stride (Bytes)
    static constexpr std::size_t node_capacity_v = cap_;
    static constexpr std::size_t cache_line_size = L::cache_line_size();

    [[nodiscard]] static constexpr std::size_t      node_capacity()    noexcept { return cap_; }
    [[nodiscard]] static constexpr std::size_t      record_stride()    noexcept { return eff_stride; }
    [[nodiscard]] static constexpr std::string_view organ_name()       noexcept { return "node_layout_aware"; }
    [[nodiscard]] static constexpr std::string_view node_name()        noexcept { return N::name(); }
    [[nodiscard]] static constexpr std::string_view layout_name()      noexcept { return L::name(); }
    [[nodiscard]] static constexpr std::string_view allocator_name()   noexcept { return A::name(); }

    // node-WIRKSAME Observables (node_type-Beweis): #aktive Chunks + #je angelegter Chunks (Node-Allokationen).
    [[nodiscard]] std::size_t chunk_count()       const noexcept { return chunks_.size(); }
    [[nodiscard]] std::size_t chunk_alloc_count() const noexcept { return chunk_allocs_; }

    // --- StorageOrgan-Concept: 8 Methoden über logischem Flach-Index (Chunk c=i/cap_, Offset=(i%cap_)*eff_stride) ---
    [[nodiscard]] std::size_t slot_count()            const noexcept { return size_; }
    [[nodiscard]] key_type    key_at(std::size_t i)   const noexcept { return load_key_(slot_ptr_(i)); }
    [[nodiscard]] value_type  value_at(std::size_t i) const noexcept { return load_value_(slot_ptr_(i)); }
    void set_value_at(std::size_t i, value_type v)          noexcept { store_value_(slot_ptr_(i), v); }

    void append_slot(key_type k, value_type v) {
        if (chunks_.empty() || (chunks_.back().size() / eff_stride) == cap_) {
            chunks_.emplace_back(); chunks_.back().reserve(cap_ * eff_stride); ++chunk_allocs_;
        }
        auto& c = chunks_.back();
        std::size_t const off = c.size();
        c.resize(off + eff_stride, static_cast<unsigned char>(0));   // Padding 0 → deterministische Checksum
        store_record_(c.data() + off, k, v);
        ++size_;
    }
    // concept-vollständig (SortedBinary): logisch an Position i einfügen/löschen über Flatten-Rebuild (O(n)).
    void insert_slot_at(std::size_t i, key_type k, value_type v) {
        auto flat = flatten_();
        flat.emplace(flat.begin() + static_cast<std::ptrdiff_t>(i), k, v);
        rebuild_(flat);
    }
    void erase_slot_at(std::size_t i) {
        auto flat = flatten_();
        flat.erase(flat.begin() + static_cast<std::ptrdiff_t>(i));
        rebuild_(flat);
    }
    void clear() noexcept { chunks_.clear(); size_ = 0; chunk_allocs_ = 0; }

    // --- Drop-in-Parität zu ComposedStore/NodeChunkedStore (von ObservableComposedSearch requires-detektiert) ---
#ifdef COMDARE_CE_ENABLE_STATISTICS
    // Säule-2 Allocator-Durchgriff — NODE- UND LAYOUT-WIRKSAM: Bytes je Chunk = cap_ * eff_stride → der CLA-Padding-
    // Aufschlag (eff_stride 64 statt 16) verteuert total_bytes layout-abhängig (gewünschte Layout→allocator-Kopplung).
    using allocator_snapshot_t = typename A::snapshot_t;
    [[nodiscard]] allocator_snapshot_t allocator_statistics() const noexcept {
        allocator_snapshot_t a{};
        a.allocation_count      = chunk_allocs_;
        a.deallocation_count    = 0;
        a.failure_count         = 0;
        a.total_bytes_allocated = chunk_allocs_ * cap_ * eff_stride;
        a.total_bytes_in_use    = size_ * eff_stride;
        return a;
    }
#endif

    // V2-Auto-Kopplung node_type: low-Byte je gespeichertem Key → Format-divergenter Self-Lookup.
    template <class NodeOrgan>
    std::uint64_t organ_observe_node_type(NodeOrgan& org) const {
        std::vector<std::uint8_t> kb; kb.reserve(size_);
        for_each_slot_([&](unsigned char const* p) { kb.push_back(static_cast<std::uint8_t>(load_key_(p) & 0xFFu)); });
        return org.observe_node_find(kb.data(), kb.size(), kb.data(), kb.size());
    }
    // V2-Auto-Kopplung layout/serialization: Scan über das chunk-lokale Byte-Backing AM ECHTEN eff_stride → der
    // memory_layout-Scan (CLA Stride 64 vs aos 16) läuft jetzt über ein Backing, dessen Stride dem Layout entspricht
    // (OOB-frei, layout-echt). record_size = eff_stride (nicht fest 16!).
    template <class LayoutOrgan>
    std::uint64_t organ_observe_layout(LayoutOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_scan(c.data(), c.size() / eff_stride, eff_stride);
        return acc;
    }
    template <class SerOrgan>
    std::uint64_t organ_observe_serialization(SerOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_serialize(c.data(), c.size() / eff_stride, eff_stride);
        return acc;
    }
    template <class VhOrgan>
    std::uint64_t organ_observe_value_handle(VhOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_value_handle(c.data(), c.size() / eff_stride, eff_stride);
        return acc;
    }
    template <class IsaOrgan>
    std::uint64_t organ_observe_isa(IsaOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) {
            std::size_t const words = c.size() / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(c.data(), words);
        }
        return acc;
    }
    template <class IdxOrgan>
    std::uint64_t organ_observe_index_org(IdxOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.index_org_observe(c.data(), c.size() / eff_stride, eff_stride);
        return acc;
    }
    template <class IoOrgan>
    std::uint64_t organ_observe_io_dispatch(IoOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_)
            acc += org.observe_dispatch(c.data(), c.size() / eff_stride, eff_stride);
        return acc;
    }
    template <class MigOrgan>
    std::uint64_t organ_observe_migration(MigOrgan& org) const {
        for (auto const& c : chunks_)
            org.observe_decide(c.data(), c.size() / eff_stride, eff_stride);
        return 0;   // observe_decide ist void (Treibe-Op exerziert)
    }
    template <class FltOrgan>
    std::uint64_t organ_observe_filter(FltOrgan& org) const {
        std::vector<unsigned char> kb; kb.reserve(size_);
        for_each_slot_([&](unsigned char const* p) { kb.push_back(static_cast<unsigned char>(load_key_(p) & 0xFFu)); });
        return org.observe_probe(kb.data(), kb.size(), kb.data(), kb.size());
    }

private:
    using slot_t = std::pair<std::uint64_t, std::uint64_t>;

    [[nodiscard]] unsigned char const* slot_ptr_(std::size_t i) const noexcept {
        return chunks_[i / cap_].data() + (i % cap_) * eff_stride;
    }
    [[nodiscard]] unsigned char* slot_ptr_(std::size_t i) noexcept {
        return chunks_[i / cap_].data() + (i % cap_) * eff_stride;
    }
    static void store_record_(unsigned char* dst, key_type k, value_type v) noexcept {
        std::memcpy(dst, &k, sizeof(k)); std::memcpy(dst + sizeof(k), &v, sizeof(v));
    }
    static void store_value_(unsigned char* dst, value_type v) noexcept { std::memcpy(dst + sizeof(key_type), &v, sizeof(v)); }
    static key_type   load_key_(unsigned char const* src)   noexcept { key_type k; std::memcpy(&k, src, sizeof(k)); return k; }
    static value_type load_value_(unsigned char const* src) noexcept { value_type v; std::memcpy(&v, src + sizeof(key_type), sizeof(v)); return v; }

    template <class F>
    void for_each_slot_(F&& f) const {
        for (auto const& c : chunks_) {
            std::size_t const n = c.size() / eff_stride;
            for (std::size_t j = 0; j < n; ++j) f(c.data() + j * eff_stride);
        }
    }

    [[nodiscard]] std::vector<slot_t> flatten_() const {
        std::vector<slot_t> fl; fl.reserve(size_);
        for_each_slot_([&](unsigned char const* p) { fl.emplace_back(load_key_(p), load_value_(p)); });
        return fl;
    }
    void rebuild_(std::vector<slot_t> const& flat) {
        clear();
        for (auto const& s : flat) append_slot(s.first, s.second);
    }

    std::vector<std::vector<unsigned char>> chunks_{};
    std::size_t size_         = 0;
    std::size_t chunk_allocs_ = 0;
};

// Compile-Time-Selbstbeweis: layout-honorierendes Organ erfüllt StorageOrgan UND ist von beiden Traversal-Organen nutzbar.
namespace _la_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
using PilotLayoutAwareStore = LayoutAwareChunkedStore<Node4NodeType, _ml_la::CacheLineAlignedMemoryLayout, _al_la::MimallocAllocator>;
static_assert(_la_cmp::StorageOrgan<PilotLayoutAwareStore>);
static_assert(_la_cmp::TraversalOrgan<_la_cmp::LinearScanTraversal,   PilotLayoutAwareStore>);
static_assert(_la_cmp::TraversalOrgan<_la_cmp::SortedBinaryTraversal, PilotLayoutAwareStore>);
static_assert(PilotLayoutAwareStore::eff_stride == 64);   // CLA über 16-B-Kv → 64-B-Stride (Padding) verifiziert

}  // namespace comdare::cache_engine::node
