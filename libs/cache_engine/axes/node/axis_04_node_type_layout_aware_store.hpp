#pragma once
// Layout-honorierender Storage — LayoutAwareChunkedStore<N,L,A> (P-MD1-ERDUNG / #167, 2026-06-18).
//
// @topic nodes @achse 04 @schicht composable-bridge (node_type ⊕ layout ⊕ allocator ALS Storage-Organ, L-WIRKSAM)
//
// **Zweck (User 2026-06-18 „5 REALE distinkte memory_layout-Repraesentationen"):** Frueher (Phase 1, 2026-06-04)
// speicherte der Store ALLE Layouts als 16-B-`[key|value]` an einem einzigen `eff_stride` (16 oder 64) — nur die
// AoS-Familie war ehrlich; SoA/AoSoA/packed_bitmap wurden wie AoS abgelegt, und die CLU kam aus einem ENTKOPPELTEN
// Deskriptor-Modell (record_useful_bytes/record_line_span je Strategie), das den realen Store IGNORIERTE
// (Phantom-Muster P-MD1). Diese Fassung ERDET das: jedes Layout speichert ueber seine `RepresentationKind`
// (axis_05_memory_layout_strategy_base.hpp) WIRKLICH unterschiedlich, compile-time-dispatched (`if constexpr`,
// zero-cost), und die CLU/field_bytes/cache_lines kommen aus dem REAL beruehrten Key-Scan-Footprint der echten
// Repraesentation.
//
// **Die 5 REALEN Repraesentationen (Strategy je Layout, store_record/load_key/load_value):**
//   • aos_interleaved_packed (aos_strict):        [key|value] adjazent, 16-B-Stride, dicht.
//   • aos_interleaved_padded (cache_line_aligned): [key|value|48 B pad], 64-B-Cache-Line-Stride.
//   • soa_split_columns (soa):                     keys[]-Spalte gefolgt von values[]-Spalte (ZWEI Arrays je Chunk).
//   • aosoa_blocked_columns (aosoa):               pro Block B keys dann B values, Bloecke als Array (SIMD-tiled).
//   • succinct_hot_cold_split (packed_bitmap):     2-B-Hot-Key-Spalte + 6-B-Cold-Residue + values; VERLUSTFREI.
// JEDE Rep speichert den vollen uint64-Key+Value VERLUSTFREI (Round-Trip insert→lookup korrekt) — der UNTERSCHIED
// liegt im physischen Byte-Layout und damit im REALEN Key-Scan-Footprint (CLU 5-fach distinkt).
//
// Erfuellt das StorageOrgan-Concept (aktuelle Default-Key-Breite uint64, 8 Methoden, byte-codiert) → Drop-in fuer ComposedSearch,
// parallel zu NodeChunkedStore (das Bestehende bleibt unangetastet). Memento ist LOGISCH (copy_from_ kopiert die
// Chunk-Buffer byte-genau → deckt ALLE Reps ab, weil die Bytes selbst die Repraesentation sind).

#include "axis_04_node_type_node4.hpp" // Pilot-NodeType (Selbstbeweis)
#include "concepts/axis_04_node_type_concept.hpp"
#include <topics/memory_layout/axis_05_memory_layout/concepts/axis_05_memory_layout_concept.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_cache_line_aligned.hpp>
#include <topics/memory_layout/axis_05_memory_layout/axis_05_memory_layout_strategy_base.hpp> // RepresentationKind
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

/// layout-honorierendes, node-gechunktes 3-Achsen-Storage-Organ: Slots liegen in node-grossen Chunks der
/// Kapazitaet N::max_capacity(). Das PHYSISCHE Byte-Layout je Chunk ist compile-time-dispatched ueber
/// L::representation_kind() (zero-cost `if constexpr`) — 5 REALE distinkte Repraesentationen (#167).
template <class N, class L, class A>
    requires concepts::NodeTypeStrategy<N> && _ml_la::concepts::MemoryLayoutStrategy<L> &&
             _al_la::concepts::AllocatorStrategy<A>
class LayoutAwareChunkedStore {
private:
    using RK                 = ::comdare::cache_engine::layout::RepresentationKind;
    static constexpr RK kRep = L::representation_kind();

    static constexpr std::size_t kKeyBytes  = sizeof(std::uint64_t); // 8
    static constexpr std::size_t kValBytes  = sizeof(std::uint64_t); // 8
    static constexpr std::size_t kKvBytes   = kKeyBytes + kValBytes; // 16 logische Nutzlast
    static constexpr std::size_t kLineBytes = 64;                    // Cache-Line (CLU-Bezugsgroesse)
    static constexpr std::size_t kHotBytes  = 2;                     // succinct: low16-Hot-Key-Spalte
    static constexpr std::size_t kColdBytes = kKeyBytes - kHotBytes; // succinct: high48-Cold-Residue (6)

    static constexpr std::size_t round_up_(std::size_t v, std::size_t a) noexcept {
        return a <= 1 ? v : ((v + a - 1u) & ~(a - 1u));
    }

public:
    using key_type       = std::uint64_t; // aktuelle Default-Breite; native schmalere Container = #217-2b
    using value_type     = std::uint64_t;
    using node_type      = N;
    using layout_type    = L;
    using allocator_type = A;

    static constexpr std::size_t cap_ = (N::max_capacity() == 0 ? std::size_t{1} : N::max_capacity());
    /// AoSoA PHYSISCHE Store-Blockbreite (B keys dann B values, Bloecke als Array). Bewusst KEINE Line-teilende
    /// Lane-Zahl (waere lane-aligned identisch zum SoA-Key-Footprint), sondern eine Tile-Breite, deren Key-Lane
    /// die 64-B-Linien STRADDLET → der Key-Scan-Footprint liegt ECHT zwischen SoA (dicht) und AoS (strided),
    /// byte-distinkt von beiden (Node-kapazitaets-gedeckelt; „B = node-Kapazitaet o.ae.", Aufgabe #167). Distinkt
    /// von der SIMD-`block_width()` (=8) der Strategie, die NUR den scan_field_sum-Vergleich steuert (unveraendert).
    static constexpr std::size_t kStoreBlock = 10;
    static constexpr std::size_t kBlockW     = (kStoreBlock < cap_) ? kStoreBlock : cap_;

    /// record_phys_bytes: die PHYSISCH pro Record im Chunk verbrauchten Bytes (Single-Record-Stride bzw.
    /// amortisierter Spaltenanteil). AoS-padded = 64, sonst 16 (alle uebrigen Reps speichern Key+Value
    /// verlustfrei in 16 B, nur ANDERS angeordnet). Bestimmt die Chunk-Allokationsgroesse.
    static constexpr std::size_t record_phys_bytes() noexcept {
        if constexpr (kRep == RK::aos_interleaved_padded)
            return round_up_(kKvBytes, kLineBytes); // 64
        else
            return kKvBytes; // 16
    }
    static constexpr std::size_t eff_stride = record_phys_bytes(); // Rueckwaerts-Kompat-Name (AoS-Stride)

    static constexpr std::size_t node_capacity_v = cap_;
    static constexpr std::size_t cache_line_size = L::cache_line_size();

    /// chunk_bytes: die GESAMTE ueber A allozierte Chunk-Kapazitaet (cap_ Records) je Repraesentation. AoSoA
    /// rundet auf VOLLE Bloecke auf (ceil(cap/B) Bloecke * B * 16 B), weil ein angebrochener letzter Block sein
    /// volles Key+Value-Lane-Paar reserviert (sonst OOB, wenn cap kein B-Vielfaches ist).
    static constexpr std::size_t chunk_bytes() noexcept {
        if constexpr (kRep == RK::aosoa_blocked_columns) {
            std::size_t const blocks = (cap_ + kBlockW - 1u) / kBlockW;
            return blocks * kBlockW * kKvBytes;
        } else {
            return cap_ * record_phys_bytes();
        }
    }

    [[nodiscard]] static constexpr std::size_t      node_capacity() noexcept { return cap_; }
    [[nodiscard]] static constexpr std::size_t      record_stride() noexcept { return record_phys_bytes(); }
    [[nodiscard]] static constexpr std::string_view organ_name() noexcept { return "node_layout_aware"; }
    [[nodiscard]] static constexpr std::string_view node_name() noexcept { return N::name(); }
    [[nodiscard]] static constexpr std::string_view layout_name() noexcept { return L::name(); }
    [[nodiscard]] static constexpr std::string_view allocator_name() noexcept { return A::name(); }
    [[nodiscard]] static constexpr RK               representation() noexcept { return kRep; }

    [[nodiscard]] std::size_t chunk_count() const noexcept { return chunks_.size(); }
    [[nodiscard]] std::size_t chunk_alloc_count() const noexcept { return chunk_allocs_; }

    LayoutAwareChunkedStore() = default;
    ~LayoutAwareChunkedStore() { free_chunks_(); }
    LayoutAwareChunkedStore(LayoutAwareChunkedStore const& o) { copy_from_(o); }
    LayoutAwareChunkedStore& operator=(LayoutAwareChunkedStore const& o) {
        if (this != &o) {
            free_chunks_();
            chunks_.clear();
            size_         = 0;
            chunk_allocs_ = 0;
            alloc_        = A{};
            copy_from_(o);
        }
        return *this;
    }
    LayoutAwareChunkedStore(LayoutAwareChunkedStore&& o) noexcept
        : alloc_(std::move(o.alloc_)), chunks_(std::move(o.chunks_)), size_(o.size_), chunk_allocs_(o.chunk_allocs_) {
        o.chunks_.clear();
        o.size_         = 0;
        o.chunk_allocs_ = 0;
    }
    LayoutAwareChunkedStore& operator=(LayoutAwareChunkedStore&& o) noexcept {
        if (this != &o) {
            free_chunks_();
            alloc_        = std::move(o.alloc_);
            chunks_       = std::move(o.chunks_);
            size_         = o.size_;
            chunk_allocs_ = o.chunk_allocs_;
            o.chunks_.clear();
            o.size_         = 0;
            o.chunk_allocs_ = 0;
        }
        return *this;
    }

    // --- StorageOrgan-Concept: 8 Methoden ueber logischem Flach-Index (Chunk c=i/cap_, Slot j=i%cap_) ---
    [[nodiscard]] std::size_t slot_count() const noexcept { return size_; }
    [[nodiscard]] key_type key_at(std::size_t i) const noexcept { return load_key_(chunks_[i / cap_].data, i % cap_); }
    [[nodiscard]] value_type value_at(std::size_t i) const noexcept {
        return load_value_(chunks_[i / cap_].data, i % cap_);
    }
    void set_value_at(std::size_t i, value_type v) noexcept { store_value_(chunks_[i / cap_].data, i % cap_, v); }

    // K9-Fix (prefetch REAL): NUR-LESE-Adresse des KEY von Slot i im realen Chunk-Backing (representation-aware).
    [[nodiscard]] unsigned char const* slot_address(std::size_t i) const noexcept {
        return (i < size_) ? key_ptr_(chunks_[i / cap_].data, i % cap_) : nullptr;
    }
    [[nodiscard]] unsigned char const* backing_begin() const noexcept {
        return chunks_.empty() ? nullptr : chunks_.front().data;
    }
    [[nodiscard]] unsigned char const* backing_end() const noexcept {
        return chunks_.empty() ? nullptr : chunks_.front().data + chunks_.front().capacity;
    }
    [[nodiscard]] std::size_t chunk_capacity_bytes() const noexcept {
        return chunks_.empty() ? 0u : chunks_.front().capacity;
    }

    void append_slot(key_type k, value_type v) {
        if (chunks_.empty() || chunks_.back().count == cap_) {
            Chunk c;
            c.capacity = chunk_bytes();
            c.data     = static_cast<unsigned char*>(alloc_.allocate(c.capacity, kChunkAlign));
            std::memset(c.data, 0, c.capacity);
            c.count = 0;
            chunks_.push_back(c);
            ++chunk_allocs_;
        }
        Chunk& c = chunks_.back();
        store_record_(c.data, c.count, k, v);
        ++c.count;
        ++size_;
    }
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
    void clear() noexcept {
        free_chunks_();
        chunks_.clear();
        size_         = 0;
        chunk_allocs_ = 0;
    }

    // --- Drop-in-Paritaet zu ComposedStore/NodeChunkedStore ---
#ifdef COMDARE_CE_ENABLE_STATISTICS
    using allocator_snapshot_t = typename A::snapshot_t;
    [[nodiscard]] allocator_snapshot_t allocator_statistics() const noexcept { return alloc_.statistics(); }
#endif

    // V2-Auto-Kopplung node_type: low-Byte je gespeichertem Key → Format-divergenter Self-Lookup.
    template <class NodeOrgan>
    std::uint64_t organ_observe_node_type(NodeOrgan& org) const {
        std::vector<std::uint8_t> kb;
        kb.reserve(size_);
        for_each_slot_([&](key_type k, value_type) { kb.push_back(static_cast<std::uint8_t>(k & 0xFFu)); });
        return org.observe_node_find(kb.data(), kb.size(), kb.data(), kb.size());
    }

    // V2-Auto-Kopplung layout (CLU-Treiber, P-MD1-ERDUNG): treibt den REALEN, representation-spezifischen
    // Key-Scan-Footprint je Chunk in den Observer. Der Footprint (field_bytes = real beruehrte Key-Nutzbytes,
    // cache_lines = real beruehrte 64-B-Linien) wird byte-genau aus der echten Repraesentation berechnet
    // (key_scan_footprint_), die Checksumme aus dem echten Key-Scan (Korrektheits-Anker). KEIN entkoppelter
    // Deskriptor mehr.
    template <class LayoutOrgan>
    std::uint64_t organ_observe_layout(LayoutOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) {
            std::uint64_t checksum  = 0;
            std::uint64_t key_bytes = 0, lines = 0;
            key_scan_footprint_(c, checksum, key_bytes, lines);
            if constexpr (requires {
                              org.observe_real_footprint(checksum, std::size_t{}, std::uint64_t{}, std::uint64_t{});
                          }) {
                acc += org.observe_real_footprint(checksum, c.count, key_bytes, lines);
            } else {
                // Fallback (alte Observer ohne Real-Footprint-API): der bestehende observe_scan-Pfad.
                acc += org.observe_scan(c.data, c.count, record_phys_bytes());
            }
        }
        return acc;
    }
    template <class SerOrgan>
    std::uint64_t organ_observe_serialization(SerOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.observe_serialize(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class VhOrgan>
    std::uint64_t organ_observe_value_handle(VhOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.observe_value_handle(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class IsaOrgan>
    std::uint64_t organ_observe_isa(IsaOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) {
            std::size_t const words = (c.count * record_phys_bytes()) / sizeof(std::uint32_t);
            acc += org.observe_simd_field_sum(c.data, words);
        }
        return acc;
    }
    template <class IdxOrgan>
    std::uint64_t organ_observe_index_org(IdxOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.index_org_observe(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class IoOrgan>
    std::uint64_t organ_observe_io_dispatch(IoOrgan& org) const {
        std::uint64_t acc = 0;
        for (auto const& c : chunks_) acc += org.observe_dispatch(c.data, c.count, record_phys_bytes());
        return acc;
    }
    template <class MigOrgan>
    std::uint64_t organ_observe_migration(MigOrgan& org) const {
        for (auto const& c : chunks_) org.observe_decide(c.data, c.count, record_phys_bytes());
        return 0;
    }
    template <class FltOrgan>
    std::uint64_t organ_observe_filter(FltOrgan& org) const {
        if constexpr (requires { org.observe_probe_keys(static_cast<std::uint64_t const*>(nullptr), std::size_t{}); }) {
            std::vector<std::uint64_t> ks;
            ks.reserve(size_);
            for_each_slot_([&](key_type k, value_type) { ks.push_back(k); });
            return org.observe_probe_keys(ks.data(), ks.size());
        } else {
            std::vector<unsigned char> kb;
            kb.reserve(size_);
            for_each_slot_([&](key_type k, value_type) { kb.push_back(static_cast<unsigned char>(k & 0xFFu)); });
            return org.observe_probe(kb.data(), kb.size(), kb.data(), kb.size());
        }
    }

    // P4 — ECHTER 2-Ebenen-Migrations-Schritt (representation-agnostisch ueber die logische (k,v)-Sicht).
    template <class MigOrgan>
    std::uint64_t organ_migrate_step(MigOrgan const& org, LayoutAwareChunkedStore& tier1, std::uint64_t max_moves = 0) {
        auto const          flat = flatten_();
        std::vector<slot_t> survivors;
        survivors.reserve(flat.size());
        std::uint64_t moved                                      = 0;
        std::uint32_t prev_recency                               = 0;
        unsigned char rec[sizeof(key_type) + sizeof(value_type)] = {};
        for (auto const& s : flat) {
            std::memcpy(rec, &s.first, sizeof(s.first));
            std::memcpy(rec + sizeof(s.first), &s.second, sizeof(s.second));
            std::uint32_t cur = 0;
            std::memcpy(&cur, rec, sizeof(cur));
            bool const migrate =
                (max_moves == 0 || moved < max_moves) && org.should_migrate_record(rec, sizeof(rec), prev_recency);
            if (migrate) {
                tier1.append_slot(s.first, s.second);
                ++moved;
            } else {
                survivors.push_back(s);
            }
            prev_recency = cur;
        }
        if (moved != 0) rebuild_(survivors);
        return moved;
    }

private:
    using slot_t = std::pair<std::uint64_t, std::uint64_t>;

    static constexpr std::size_t kChunkAlign = 64;
    struct Chunk {
        unsigned char* data     = nullptr;
        std::size_t    count    = 0; // belegte Records (<= cap_)
        std::size_t    capacity = 0; // ueber A allozierte Bytes (== chunk_bytes())
    };

    // ── Representation-aware store/load (compile-time-dispatch, zero-cost) ─────────────────────────────────
    // Adress-Helfer pro Repraesentation. `base` = Chunk-Start, `j` = Slot-Index im Chunk (0..cap_-1).

    [[nodiscard]] static unsigned char* key_ptr_(unsigned char* base, std::size_t j) noexcept {
        if constexpr (kRep == RK::aos_interleaved_packed)
            return base + j * kKvBytes; // [k|v] @ 16
        else if constexpr (kRep == RK::aos_interleaved_padded)
            return base + j * record_phys_bytes(); // [k|v|pad] @ 64
        else if constexpr (kRep == RK::soa_split_columns)
            return base + j * kKeyBytes; // keys[] Spalte
        else if constexpr (kRep == RK::aosoa_blocked_columns) {
            std::size_t const b = j / kBlockW, w = j % kBlockW;
            return base + b * (kBlockW * kKvBytes) + w * kKeyBytes; // Block-Key-Lane
        } else {                                                    /* succinct_hot_cold_split */
            return base + j * kHotBytes;                            // 2-B-Hot-Spalte
        }
    }
    [[nodiscard]] static unsigned char const* key_ptr_(unsigned char const* base, std::size_t j) noexcept {
        return key_ptr_(const_cast<unsigned char*>(base), j);
    }
    [[nodiscard]] static unsigned char* value_ptr_(unsigned char* base, std::size_t j) noexcept {
        if constexpr (kRep == RK::aos_interleaved_packed)
            return base + j * kKvBytes + kKeyBytes;
        else if constexpr (kRep == RK::aos_interleaved_padded)
            return base + j * record_phys_bytes() + kKeyBytes;
        else if constexpr (kRep == RK::soa_split_columns)
            return base + cap_ * kKeyBytes + j * kValBytes; // values[] nach keys[]
        else if constexpr (kRep == RK::aosoa_blocked_columns) {
            std::size_t const b = j / kBlockW, w = j % kBlockW;
            return base + b * (kBlockW * kKvBytes) + kBlockW * kKeyBytes + w * kValBytes; // Block-Value-Lane
        } else { /* succinct: [hot2[cap]][cold6[cap]][values8[cap]] — value-Region nach hot+cold */
            return base + cap_ * kHotBytes + cap_ * kColdBytes + j * kValBytes;
        }
    }
    [[nodiscard]] static unsigned char const* value_ptr_(unsigned char const* base, std::size_t j) noexcept {
        return value_ptr_(const_cast<unsigned char*>(base), j);
    }
    /// succinct-only: Cold-Residue (high48) Adresse.
    [[nodiscard]] static unsigned char* cold_ptr_(unsigned char* base, std::size_t j) noexcept {
        return base + cap_ * kHotBytes + j * kColdBytes;
    }
    [[nodiscard]] static unsigned char const* cold_ptr_(unsigned char const* base, std::size_t j) noexcept {
        return cold_ptr_(const_cast<unsigned char*>(base), j);
    }

    static void store_record_(unsigned char* base, std::size_t j, key_type k, value_type v) noexcept {
        if constexpr (kRep == RK::succinct_hot_cold_split) {
            std::uint16_t const hot  = static_cast<std::uint16_t>(k & 0xFFFFu);
            std::uint64_t const cold = k >> 16; // high48 (passt in 6 B)
            std::memcpy(key_ptr_(base, j), &hot, kHotBytes);
            std::memcpy(cold_ptr_(base, j), &cold, kColdBytes); // nur die unteren 6 B von cold
            std::memcpy(value_ptr_(base, j), &v, kValBytes);
        } else {
            std::memcpy(key_ptr_(base, j), &k, kKeyBytes);
            std::memcpy(value_ptr_(base, j), &v, kValBytes);
        }
    }
    static void store_value_(unsigned char* base, std::size_t j, value_type v) noexcept {
        std::memcpy(value_ptr_(base, j), &v, kValBytes);
    }
    static key_type load_key_(unsigned char const* base, std::size_t j) noexcept {
        if constexpr (kRep == RK::succinct_hot_cold_split) {
            std::uint16_t hot = 0;
            std::memcpy(&hot, key_ptr_(base, j), kHotBytes);
            std::uint64_t cold = 0;
            std::memcpy(&cold, cold_ptr_(base, j), kColdBytes); // liest 6 B in low48
            return static_cast<key_type>(hot) | (cold << 16);   // VERLUSTFREIE Rekonstruktion
        } else {
            key_type k;
            std::memcpy(&k, key_ptr_(base, j), kKeyBytes);
            return k;
        }
    }
    static value_type load_value_(unsigned char const* base, std::size_t j) noexcept {
        value_type v;
        std::memcpy(&v, value_ptr_(base, j), kValBytes);
        return v;
    }

    template <class F>
    void for_each_slot_(F&& f) const {
        for (auto const& c : chunks_)
            for (std::size_t j = 0; j < c.count; ++j) f(load_key_(c.data, j), load_value_(c.data, j));
    }

    [[nodiscard]] std::vector<slot_t> flatten_() const {
        std::vector<slot_t> fl;
        fl.reserve(size_);
        for_each_slot_([&](key_type k, value_type v) { fl.emplace_back(k, v); });
        return fl;
    }
    void rebuild_(std::vector<slot_t> const& flat) {
        clear();
        for (auto const& s : flat) append_slot(s.first, s.second);
    }

    // ── REALER Key-only-Scan-Footprint (P-MD1-ERDUNG, CLU-Quelle) ─────────────────────────────────────────
    // Liest ALLE Keys EINES Chunks aus der ECHTEN Repraesentation (Checksumme = Korrektheits-Anker) und zaehlt
    // dabei (a) die NUTZbaren Key-Bytes (`key_bytes`) und (b) die DISTINKTEN 64-B-Cache-Linien (`lines`), die
    // die realen Key-Adressen beruehren. Die Lines werden aus den realen Byte-Offsets des Chunk-Backings
    // bestimmt (Offset_div_64-Markierung) — kein Modell, sondern der echte Speicher-Footprint des Zugriffs.
    void key_scan_footprint_(Chunk const& c, std::uint64_t& checksum, std::uint64_t& key_bytes,
                             std::uint64_t& lines) const noexcept {
        checksum  = 0;
        key_bytes = 0;
        // Bitset der beruehrten 64-B-Linien innerhalb des Chunks (chunk_bytes()/64 + 2 Linien-Indizes).
        constexpr std::size_t kMaxLines          = (chunk_bytes() / kLineBytes) + 2u;
        bool                  touched[kMaxLines] = {};
        auto                  mark               = [&](std::size_t off, std::size_t w) noexcept {
            for (std::size_t b = off; b < off + w; ++b) {
                std::size_t const li = b / kLineBytes;
                if (li < kMaxLines) touched[li] = true;
            }
        };
        for (std::size_t j = 0; j < c.count; ++j) {
            checksum += load_key_(c.data, j);
            if constexpr (kRep == RK::succinct_hot_cold_split) {
                // succinct: der Key-Scan beruehrt die 2-B-HOT-Spalte (diskriminierende, NUTZbare Bytes) UND die
                // 6-B-COLD-Residue (fuer die verlustfreie Rekonstruktion mit-beruehrt, aber NICHT als Nutz-Key
                // zaehlend) → useful = 2 B/Key, touched lines = Hot- + Cold-Spalten-Linien → CLU echt NIEDRIG.
                mark(static_cast<std::size_t>(key_ptr_(c.data, j) - c.data), kHotBytes);
                mark(static_cast<std::size_t>(cold_ptr_(c.data, j) - c.data), kColdBytes);
                key_bytes += kHotBytes;
            } else {
                // dichte/strided Reps: die vollen 8 Key-Bytes sind NUTZbar; touched lines = ihre realen Adressen.
                mark(static_cast<std::size_t>(key_ptr_(c.data, j) - c.data), kKeyBytes);
                key_bytes += kKeyBytes;
            }
        }
        std::uint64_t cnt = 0;
        for (std::size_t li = 0; li < kMaxLines; ++li)
            if (touched[li]) ++cnt;
        lines = cnt;
    }

    void free_chunks_() noexcept {
        for (auto& c : chunks_)
            if (c.data) {
                alloc_.deallocate(c.data, c.capacity, kChunkAlign);
                c.data = nullptr;
            }
    }
    void copy_from_(LayoutAwareChunkedStore const& o) {
        chunks_.reserve(o.chunks_.size());
        for (auto const& oc : o.chunks_) {
            Chunk c;
            c.capacity = oc.capacity;
            c.data     = static_cast<unsigned char*>(alloc_.allocate(c.capacity, kChunkAlign));
            std::memcpy(c.data, oc.data, oc.capacity); // byte-genaue Kopie → deckt ALLE Reps ab (Memento)
            c.count = oc.count;
            chunks_.push_back(c);
        }
        size_         = o.size_;
        chunk_allocs_ = o.chunk_allocs_;
    }

    mutable A          alloc_{};
    std::vector<Chunk> chunks_{};
    std::size_t        size_         = 0;
    std::size_t        chunk_allocs_ = 0;
};

// Compile-Time-Selbstbeweis: layout-honorierendes Organ erfuellt StorageOrgan UND ist von beiden Traversal-Organen nutzbar.
namespace _la_cmp = ::comdare::cache_engine::traversal::axis_03a_search_algo::composable;
using PilotLayoutAwareStore =
    LayoutAwareChunkedStore<Node4NodeType, _ml_la::CacheLineAlignedMemoryLayout, _al_la::MimallocAllocator>;
static_assert(_la_cmp::StorageOrgan<PilotLayoutAwareStore>);
static_assert(_la_cmp::TraversalOrgan<_la_cmp::LinearScanTraversal, PilotLayoutAwareStore>);
static_assert(_la_cmp::TraversalOrgan<_la_cmp::SortedBinaryTraversal, PilotLayoutAwareStore>);
static_assert(PilotLayoutAwareStore::record_phys_bytes() == 64); // CLA → 64-B-Stride (Padding) verifiziert

} // namespace comdare::cache_engine::node
