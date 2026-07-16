#pragma once
// V41 Umstufung-A s4 (Task #43) — ArtTrieNodePoolStore: adaptiver Byte-Trie-Knoten-Pool (erfuellt ArtTrieNodePool).
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Reines Substrat OHNE Such-Logik — originalgetreue (is_original=false [[pseudocode-papers-fallback]]) Re-Impl
// der ART-Knoten-Anatomie (Leis ICDE 2013, unodb art_internal_impl.hpp): Leaf (Key+Value inline) + 4 ADAPTIVE
// Inner-Knoten-Reps Node4/16/48/256 mit ihren je eigenen Kind-Such-Disziplinen (N4/N16: sortierte Byte-Arrays;
// N48: uint8[256]-Index -> 48 Slots; N256: 256 Direkt-Slots) + GROWTH N4->16->48->256 (daten-getrieben in
// add_child — Knoten-Typ ist ein DATEN-Feld, KEIN Algorithmus-Runtime-Switch, [[no-runtime-switch]] gewahrt).
// Path-Compression je Inner-Node via ByteWiseKeyPrefix-Organ (axis_02). Der Byte-Descent + Leaf-Split +
// Prefix-Split lebt im ArtTrieTraversalOrgan, NICHT hier (genetisches Experiment, Doku 14 §1.2).
//
// **Skalar/Korrektheit-zuerst:** find_child ist skalar (kein SIMD); die SIMD-Verdrahtung (Node16 _mm_cmpeq_epi8
// ueber 16 Bytes etc.) ist ein Folge-Increment hinter axis_09b-ISA-Guard. NodeRef = std::size_t (Kind in
// Bits 56-63, Index in Bits 0-55); je Kind eigener Vektor + Free-List (Index-Stabilitaet via Recycling).

#include "art_trie_node_pool_concept.hpp"
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp> // ByteWiseKeyPrefix

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace comdare::cache_engine::lookup::composable {

namespace detail {

using ArtTriePrefix = ::comdare::cache_engine::nodes::axis_02_path_compression::ByteWiseKeyPrefix;

inline constexpr std::size_t  kArtTrieNil     = std::numeric_limits<std::size_t>::max();
inline constexpr std::uint8_t kArtTrieEmpty48 = 0xFFu;

struct ArtTrieLeaf {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;

    key_type   key{};
    value_type val{};
};

struct ArtTrieNode4 {
    ArtTriePrefix               prefix{};
    int                         n = 0;
    std::array<std::uint8_t, 4> keys{};
    std::array<std::size_t, 4>  kids{};
};

struct ArtTrieNode16 {
    ArtTriePrefix                prefix{};
    int                          n = 0;
    std::array<std::uint8_t, 16> keys{};
    std::array<std::size_t, 16>  kids{};
};

struct ArtTrieNode48 {
    ArtTriePrefix                 prefix{};
    int                           n = 0;
    std::array<std::uint8_t, 256> child_index{};
    std::array<std::size_t, 48>   kids{};
    std::array<std::uint8_t, 48>  slot_byte{}; // Reverse-Map Slot->Byte (fuer O(1)-Kompaktierung bei remove_child)
    ArtTrieNode48() { child_index.fill(kArtTrieEmpty48); }
};

struct ArtTrieNode256 {
    ArtTriePrefix                prefix{};
    int                          n = 0;
    std::array<std::size_t, 256> kids{};
    ArtTrieNode256() { kids.fill(kArtTrieNil); }
};

} // namespace detail

template <class A = std::allocator<detail::ArtTrieLeaf>>
class ArtTrieNodePoolStore {
public:
    using node_type            = detail::ArtTrieLeaf;
    using key_type             = typename node_type::key_type;
    using value_type           = typename node_type::value_type;
    using prefix_type          = detail::ArtTriePrefix;
    using allocator_type       = A;
    using leaf_allocator_type  = typename std::allocator_traits<A>::template rebind_alloc<detail::ArtTrieLeaf>;
    using n4_allocator_type    = typename std::allocator_traits<A>::template rebind_alloc<detail::ArtTrieNode4>;
    using n16_allocator_type   = typename std::allocator_traits<A>::template rebind_alloc<detail::ArtTrieNode16>;
    using n48_allocator_type   = typename std::allocator_traits<A>::template rebind_alloc<detail::ArtTrieNode48>;
    using n256_allocator_type  = typename std::allocator_traits<A>::template rebind_alloc<detail::ArtTrieNode256>;
    using index_allocator_type = typename std::allocator_traits<A>::template rebind_alloc<std::size_t>;
    static constexpr std::size_t kNil = detail::kArtTrieNil;

    // NodeRef-Kodierung: Kind in Bits 56-63, Index in Bits 0-55.
    enum Kind : std::uint8_t { kLeaf = 0, kN4 = 1, kN16 = 2, kN48 = 3, kN256 = 4 };
    [[nodiscard]] static constexpr std::size_t make_ref(Kind kind, std::size_t idx) noexcept {
        return (static_cast<std::size_t>(kind) << 56U) | idx;
    }
    [[nodiscard]] static constexpr Kind        ref_kind(std::size_t r) noexcept { return static_cast<Kind>(r >> 56U); }
    [[nodiscard]] static constexpr std::size_t ref_idx(std::size_t r) noexcept { return r & 0x00FF'FFFF'FFFF'FFFFULL; }

    // ── Wurzel + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void                      set_root(std::size_t r) noexcept { root_ = r; }
    void                      inc_size() noexcept { ++size_; }
    void                      dec_size() noexcept { --size_; }
    void                      clear() noexcept {
        leaves_.clear();
        n4_.clear();
        n16_.clear();
        n48_.clear();
        n256_.clear();
        fl_leaf_.clear();
        fl_n4_.clear();
        fl_n16_.clear();
        fl_n48_.clear();
        fl_n256_.clear();
        root_ = kNil;
        size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] bool        is_leaf(std::size_t r) const noexcept { return ref_kind(r) == kLeaf; }
    [[nodiscard]] key_type    leaf_key(std::size_t r) const noexcept { return leaves_[ref_idx(r)].key; }
    [[nodiscard]] value_type  leaf_value(std::size_t r) const noexcept { return leaves_[ref_idx(r)].val; }
    void                      set_leaf_value(std::size_t r, value_type v) noexcept { leaves_[ref_idx(r)].val = v; }
    [[nodiscard]] std::size_t new_leaf(key_type k, value_type v) {
        std::size_t idx;
        if (!fl_leaf_.empty()) {
            idx = fl_leaf_.back();
            fl_leaf_.pop_back();
            leaves_[idx] = node_type{k, v};
        } else {
            idx                            = leaves_.size();
            std::size_t const old_capacity = leaves_.capacity();
            leaves_.push_back(node_type{k, v});
            record_capacity_growth_(old_capacity, leaves_.capacity(), sizeof(node_type));
        }
        return make_ref(kLeaf, idx);
    }

    // ── Inner: Prefix + Kind-Zahl ──
    [[nodiscard]] std::size_t new_node4() {
        std::size_t idx;
        if (!fl_n4_.empty()) {
            idx = fl_n4_.back();
            fl_n4_.pop_back();
            n4_[idx] = N4{};
        } else {
            idx                            = n4_.size();
            std::size_t const old_capacity = n4_.capacity();
            n4_.push_back(N4{});
            record_capacity_growth_(old_capacity, n4_.capacity(), sizeof(N4));
        }
        return make_ref(kN4, idx);
    }
    [[nodiscard]] prefix_type prefix_of(std::size_t r) const noexcept {
        switch (ref_kind(r)) {
            case kN4: return n4_[ref_idx(r)].prefix;
            case kN16: return n16_[ref_idx(r)].prefix;
            case kN48: return n48_[ref_idx(r)].prefix;
            case kN256: return n256_[ref_idx(r)].prefix;
            default: return prefix_type{};
        }
    }
    void              set_prefix(std::size_t r, prefix_type p) noexcept { mutable_prefix(r) = p; }
    void              prefix_cut(std::size_t r, unsigned n) noexcept { mutable_prefix(r).cut(n); }
    [[nodiscard]] int node_n(std::size_t r) const noexcept {
        switch (ref_kind(r)) {
            case kN4: return n4_[ref_idx(r)].n;
            case kN16: return n16_[ref_idx(r)].n;
            case kN48: return n48_[ref_idx(r)].n;
            case kN256: return n256_[ref_idx(r)].n;
            default: return 0;
        }
    }

    // ── Byte-Dispatch (skalar) ──
    [[nodiscard]] std::size_t find_child(std::size_t r, std::uint8_t b) const noexcept {
        switch (ref_kind(r)) {
            case kN4: {
                N4 const& x = n4_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i)
                    if (x.keys[i] == b) return x.kids[i];
                return kNil;
            }
            case kN16: {
                N16 const& x = n16_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i)
                    if (x.keys[i] == b) return x.kids[i];
                return kNil;
            }
            case kN48: {
                N48 const&         x    = n48_[ref_idx(r)];
                std::uint8_t const slot = x.child_index[b];
                return (slot == kEmpty48) ? kNil : x.kids[slot];
            }
            case kN256: return n256_[ref_idx(r)].kids[b];
            default: return kNil;
        }
    }

    /// Fuegt (b, child) ein (b ist garantiert NICHT vorhanden). Bei vollem Knoten: GROWTH zum naechsten Typ,
    /// dann Einfuegen. Liefert die (ggf. neue, gewachsene) Node-Ref.
    [[nodiscard]] std::size_t add_child(std::size_t r, std::uint8_t b, std::size_t child) {
        switch (ref_kind(r)) {
            case kN4: {
                if (n4_[ref_idx(r)].n < 4) {
                    insert_sorted_n4(ref_idx(r), b, child);
                    return r;
                }
                std::size_t const g = grow_n4_to_n16(ref_idx(r));
                insert_sorted_n16(ref_idx(g), b, child);
                return g;
            }
            case kN16: {
                if (n16_[ref_idx(r)].n < 16) {
                    insert_sorted_n16(ref_idx(r), b, child);
                    return r;
                }
                std::size_t const g = grow_n16_to_n48(ref_idx(r));
                insert_n48(ref_idx(g), b, child);
                return g;
            }
            case kN48: {
                if (n48_[ref_idx(r)].n < 48) {
                    insert_n48(ref_idx(r), b, child);
                    return r;
                }
                std::size_t const g = grow_n48_to_n256(ref_idx(r));
                insert_n256(ref_idx(g), b, child);
                return g;
            }
            case kN256: insert_n256(ref_idx(r), b, child); return r;
            default: return r;
        }
    }

    /// Ersetzt den Kind-Slot fuer das bereits vorhandene Byte b (kein Growth, keine Zahl-Aenderung).
    void set_child(std::size_t r, std::uint8_t b, std::size_t child) noexcept {
        switch (ref_kind(r)) {
            case kN4: {
                N4& x = n4_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i)
                    if (x.keys[i] == b) {
                        x.kids[i] = child;
                        return;
                    }
                return;
            }
            case kN16: {
                N16& x = n16_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i)
                    if (x.keys[i] == b) {
                        x.kids[i] = child;
                        return;
                    }
                return;
            }
            case kN48: {
                N48&               x = n48_[ref_idx(r)];
                std::uint8_t const s = x.child_index[b];
                if (s != kEmpty48) x.kids[s] = child;
                return;
            }
            case kN256: n256_[ref_idx(r)].kids[b] = child; return;
            default: return;
        }
    }

    /// Entfernt den Kind-Slot fuer Byte b (Erase). KEIN Shrink (under-volle Knoten sind lookup-korrekt;
    /// die N4<-N16<-N48<-N256-Schrumpfung + Kollaps ist ein Folge-Refinement-Increment). b ist vorhanden.
    // Entfernt das Kind fuer Byte b und gibt den (ggf. GESCHRUMPFTEN) Knoten-Ref zurueck — adaptiver Shrink
    // N256->N48->N16->N4 (Leis ICDE 2013: Knotengroesse passt sich in BEIDE Richtungen an; bisher wuchs nur).
    // Hysterese gegen Thrashing (Grow bei 49/17/5, Shrink bei 36/12/3). N4 ist kleinster Typ -> kein weiterer
    // Shrink hier; der N4-Einzelkind-Praefix-KOLLAPS in den Eltern ist ein separates Folge-Refinement (wie I4).
    std::size_t remove_child(std::size_t r, std::uint8_t b) noexcept {
        switch (ref_kind(r)) {
            case kN4: {
                N4& x = n4_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i)
                    if (x.keys[i] == b) {
                        for (int j = i + 1; j < x.n; ++j) {
                            x.keys[j - 1] = x.keys[j];
                            x.kids[j - 1] = x.kids[j];
                        }
                        --x.n;
                        break;
                    }
                return r;
            }
            case kN16: {
                N16& x = n16_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i)
                    if (x.keys[i] == b) {
                        for (int j = i + 1; j < x.n; ++j) {
                            x.keys[j - 1] = x.keys[j];
                            x.kids[j - 1] = x.kids[j];
                        }
                        --x.n;
                        if (x.n == kShrink16) return shrink_n16_to_n4(ref_idx(r));
                        break;
                    }
                return r;
            }
            case kN48: {
                N48& x = n48_[ref_idx(r)];
                // KOMPAKTIERUNG (Bugfix, adversariale Verifikation w21detpyz): den letzten belegten Slot in den
                // frei werdenden ziehen, damit kids[0..n-1] DICHT bleibt -> insert_n48 (slot=x.n) aliast nie einen
                // lebenden Slot. Ohne Kompaktierung wuerde slot=x.n nach Erase einen noch referenzierten Slot
                // ueberschreiben (stiller Lookup-Verlust). Reverse-Map slot_byte macht das O(1).
                std::uint8_t const s = x.child_index[b];
                if (s != kEmpty48) {
                    int const last = x.n - 1;
                    if (static_cast<int>(s) != last) {
                        std::uint8_t const b_last = x.slot_byte[static_cast<std::size_t>(last)];
                        x.kids[s]                 = x.kids[static_cast<std::size_t>(last)];
                        x.child_index[b_last]     = s;
                        x.slot_byte[s]            = b_last;
                    }
                    x.child_index[b] = kEmpty48;
                    --x.n;
                    if (x.n == kShrink48) return shrink_n48_to_n16(ref_idx(r));
                }
                return r;
            }
            case kN256: {
                N256& x = n256_[ref_idx(r)];
                if (x.kids[b] != kNil) {
                    x.kids[b] = kNil;
                    --x.n;
                    if (x.n == kShrink256) return shrink_n256_to_n48(ref_idx(r));
                }
                return r;
            }
            default: return r;
        }
    }

    // (F57/Muster B, WP-5 2026-07-16): NICHT noexcept — free_.push_back kann beim Free-List-Wachstum
    // allozieren/werfen ([[allocation-failure-exception]]: werfen statt terminate; Concept verlangt kein noexcept).
    void free_node(std::size_t r) {
        switch (ref_kind(r)) {
            case kLeaf: {
                std::size_t const old_capacity = fl_leaf_.capacity();
                fl_leaf_.push_back(ref_idx(r));
                record_capacity_growth_(old_capacity, fl_leaf_.capacity(), sizeof(std::size_t));
                return;
            }
            case kN4: {
                std::size_t const old_capacity = fl_n4_.capacity();
                fl_n4_.push_back(ref_idx(r));
                record_capacity_growth_(old_capacity, fl_n4_.capacity(), sizeof(std::size_t));
                return;
            }
            case kN16: {
                std::size_t const old_capacity = fl_n16_.capacity();
                fl_n16_.push_back(ref_idx(r));
                record_capacity_growth_(old_capacity, fl_n16_.capacity(), sizeof(std::size_t));
                return;
            }
            case kN48: {
                std::size_t const old_capacity = fl_n48_.capacity();
                fl_n48_.push_back(ref_idx(r));
                record_capacity_growth_(old_capacity, fl_n48_.capacity(), sizeof(std::size_t));
                return;
            }
            case kN256: {
                std::size_t const old_capacity = fl_n256_.capacity();
                fl_n256_.push_back(ref_idx(r));
                record_capacity_growth_(old_capacity, fl_n256_.capacity(), sizeof(std::size_t));
                return;
            }
        }
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    struct allocator_statistics_snapshot {
        std::uint64_t alloc_calls     = 0;
        std::uint64_t bytes_allocated = 0;
        std::uint64_t live_nodes      = 0;
    };

    [[nodiscard]] allocator_statistics_snapshot store_allocator_statistics() const noexcept {
        return allocator_statistics_snapshot{alloc_calls_, bytes_allocated_, live_node_count_()};
    }
#endif

private:
    using Leaf = detail::ArtTrieLeaf;
    using N4   = detail::ArtTrieNode4;
    using N16  = detail::ArtTrieNode16;
    using N48  = detail::ArtTrieNode48;
    using N256 = detail::ArtTrieNode256;

    static constexpr std::uint8_t kEmpty48 = detail::kArtTrieEmpty48; // "kein Slot" im N48-child_index
    // Shrink-Schwellen (Hysterese ggue. Grow bei 49/17/5): geschrumpft wird, wenn n NACH dem Entfernen DIESEN
    // Wert erreicht — jeweils unter der Zielkapazitaet (48/16/4) mit Reserve, damit ein direktes Re-Insert
    // nicht sofort wieder waechst.
    static constexpr int kShrink256 = 36; // N256 -> N48
    static constexpr int kShrink48  = 12; // N48  -> N16
    static constexpr int kShrink16  = 3;  // N16  -> N4

#ifdef COMDARE_CE_ENABLE_STATISTICS
    [[nodiscard]] std::uint64_t live_node_count_() const noexcept {
        std::size_t const node_count = leaves_.size() + n4_.size() + n16_.size() + n48_.size() + n256_.size();
        std::size_t const free_count =
            fl_leaf_.size() + fl_n4_.size() + fl_n16_.size() + fl_n48_.size() + fl_n256_.size();
        return static_cast<std::uint64_t>(node_count - free_count);
    }

    // Ehrliche Allokator-Metrik: gezaehlt werden nur erfolgreiche vector-capacity-Zuwaechse, als Capacity-Delta
    // mal Elementgroesse. Reuse/clear ohne Capacity-Wachstum erzeugt bewusst keine kuenstlichen Werte.
    void record_capacity_growth_(std::size_t old_capacity, std::size_t new_capacity, std::size_t elem_bytes) noexcept {
        if (new_capacity <= old_capacity) return;
        ++alloc_calls_;
        bytes_allocated_ +=
            static_cast<std::uint64_t>(new_capacity - old_capacity) * static_cast<std::uint64_t>(elem_bytes);
    }
#else
    static void record_capacity_growth_(std::size_t, std::size_t, std::size_t) noexcept {}
#endif

    [[nodiscard]] prefix_type& mutable_prefix(std::size_t r) noexcept {
        switch (ref_kind(r)) {
            case kN4: return n4_[ref_idx(r)].prefix;
            case kN16: return n16_[ref_idx(r)].prefix;
            case kN48: return n48_[ref_idx(r)].prefix;
            default: return n256_[ref_idx(r)].prefix;
        }
    }

    // ── sortiertes Einfuegen (N4/N16) ──
    void insert_sorted_n4(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N4& x   = n4_[idx];
        int pos = x.n;
        while (pos > 0 && x.keys[pos - 1] > b) {
            x.keys[pos] = x.keys[pos - 1];
            x.kids[pos] = x.kids[pos - 1];
            --pos;
        }
        x.keys[pos] = b;
        x.kids[pos] = child;
        ++x.n;
    }
    void insert_sorted_n16(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N16& x   = n16_[idx];
        int  pos = x.n;
        while (pos > 0 && x.keys[pos - 1] > b) {
            x.keys[pos] = x.keys[pos - 1];
            x.kids[pos] = x.kids[pos - 1];
            --pos;
        }
        x.keys[pos] = b;
        x.kids[pos] = child;
        ++x.n;
    }
    void insert_n48(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N48&      x    = n48_[idx];
        int const slot = x.n; // kids[0..n-1] DICHT (remove_child kompaktiert) -> slot == Hochwasser
        x.kids[static_cast<std::size_t>(slot)]      = child;
        x.child_index[b]                            = static_cast<std::uint8_t>(slot);
        x.slot_byte[static_cast<std::size_t>(slot)] = b;
        ++x.n;
    }
    void insert_n256(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N256& x   = n256_[idx];
        x.kids[b] = child;
        ++x.n;
    }

    // ── GROWTH (kopiert Prefix + alle Kinder in den groesseren Typ, gibt alten frei) ──
    [[nodiscard]] std::size_t grow_n4_to_n16(std::size_t idx) {
        std::size_t const r16 = new_node16();
        N16&              d   = n16_[ref_idx(r16)];
        N4&               s   = n4_[idx];
        d.prefix              = s.prefix;
        d.n                   = s.n;
        for (int i = 0; i < s.n; ++i) {
            d.keys[i] = s.keys[i];
            d.kids[i] = s.kids[i];
        }
        std::size_t const old_capacity = fl_n4_.capacity();
        fl_n4_.push_back(idx);
        record_capacity_growth_(old_capacity, fl_n4_.capacity(), sizeof(std::size_t));
        return r16;
    }
    [[nodiscard]] std::size_t grow_n16_to_n48(std::size_t idx) {
        std::size_t const r48 = new_node48();
        N48&              d   = n48_[ref_idx(r48)];
        N16&              s   = n16_[idx];
        d.prefix              = s.prefix;
        d.n                   = s.n;
        for (int i = 0; i < s.n; ++i) {
            d.kids[static_cast<std::size_t>(i)]      = s.kids[i];
            d.child_index[s.keys[i]]                 = static_cast<std::uint8_t>(i);
            d.slot_byte[static_cast<std::size_t>(i)] = s.keys[i];
        }
        std::size_t const old_capacity = fl_n16_.capacity();
        fl_n16_.push_back(idx);
        record_capacity_growth_(old_capacity, fl_n16_.capacity(), sizeof(std::size_t));
        return r48;
    }
    [[nodiscard]] std::size_t grow_n48_to_n256(std::size_t idx) {
        std::size_t const r256 = new_node256();
        N256&             d    = n256_[ref_idx(r256)];
        N48&              s    = n48_[idx];
        d.prefix               = s.prefix;
        d.n                    = s.n;
        for (int b = 0; b < 256; ++b) {
            std::uint8_t const slot = s.child_index[static_cast<std::size_t>(b)];
            if (slot != kEmpty48) d.kids[static_cast<std::size_t>(b)] = s.kids[slot];
        }
        std::size_t const old_capacity = fl_n48_.capacity();
        fl_n48_.push_back(idx);
        record_capacity_growth_(old_capacity, fl_n48_.capacity(), sizeof(std::size_t));
        return r256;
    }

    // ── Adaptiver SHRINK (Spiegel der Growth, Leis ICDE 2013) — Byte-Reihenfolge bleibt erhalten ──
    [[nodiscard]] std::size_t shrink_n256_to_n48(std::size_t idx) {
        std::size_t const r48 = new_node48();
        N48&              d   = n48_[ref_idx(r48)];
        N256&             s   = n256_[idx];
        d.prefix              = s.prefix;
        d.n                   = s.n;
        int slot              = 0;
        for (int b = 0; b < 256; ++b) {
            std::size_t const kid = s.kids[static_cast<std::size_t>(b)];
            if (kid != kNil) {
                d.kids[static_cast<std::size_t>(slot)]      = kid;
                d.child_index[static_cast<std::size_t>(b)]  = static_cast<std::uint8_t>(slot);
                d.slot_byte[static_cast<std::size_t>(slot)] = static_cast<std::uint8_t>(b);
                ++slot;
            }
        }
        std::size_t const old_capacity = fl_n256_.capacity();
        fl_n256_.push_back(idx);
        record_capacity_growth_(old_capacity, fl_n256_.capacity(), sizeof(std::size_t));
        return r48;
    }
    [[nodiscard]] std::size_t shrink_n48_to_n16(std::size_t idx) {
        std::size_t const r16 = new_node16();
        N16&              d   = n16_[ref_idx(r16)];
        N48&              s   = n48_[idx];
        d.prefix              = s.prefix;
        d.n                   = s.n;
        int i                 = 0;
        for (int b = 0; b < 256; ++b) { // Byte-Reihenfolge -> N16-keys bleiben sortiert
            std::uint8_t const sl = s.child_index[static_cast<std::size_t>(b)];
            if (sl != kEmpty48) {
                d.keys[i] = static_cast<std::uint8_t>(b);
                d.kids[i] = s.kids[sl];
                ++i;
            }
        }
        std::size_t const old_capacity = fl_n48_.capacity();
        fl_n48_.push_back(idx);
        record_capacity_growth_(old_capacity, fl_n48_.capacity(), sizeof(std::size_t));
        return r16;
    }
    [[nodiscard]] std::size_t shrink_n16_to_n4(std::size_t idx) {
        std::size_t const r4 = new_node4();
        N4&               d  = n4_[ref_idx(r4)];
        N16&              s  = n16_[idx];
        d.prefix             = s.prefix;
        d.n                  = s.n;
        for (int i = 0; i < s.n; ++i) {
            d.keys[i] = s.keys[i];
            d.kids[i] = s.kids[i];
        } // bereits sortiert
        std::size_t const old_capacity = fl_n16_.capacity();
        fl_n16_.push_back(idx);
        record_capacity_growth_(old_capacity, fl_n16_.capacity(), sizeof(std::size_t));
        return r4;
    }
    [[nodiscard]] std::size_t new_node16() {
        std::size_t idx;
        if (!fl_n16_.empty()) {
            idx = fl_n16_.back();
            fl_n16_.pop_back();
            n16_[idx] = N16{};
        } else {
            idx                            = n16_.size();
            std::size_t const old_capacity = n16_.capacity();
            n16_.push_back(N16{});
            record_capacity_growth_(old_capacity, n16_.capacity(), sizeof(N16));
        }
        return make_ref(kN16, idx);
    }
    [[nodiscard]] std::size_t new_node48() {
        std::size_t idx;
        if (!fl_n48_.empty()) {
            idx = fl_n48_.back();
            fl_n48_.pop_back();
            n48_[idx] = N48{};
        } else {
            idx                            = n48_.size();
            std::size_t const old_capacity = n48_.capacity();
            n48_.push_back(N48{});
            record_capacity_growth_(old_capacity, n48_.capacity(), sizeof(N48));
        }
        return make_ref(kN48, idx);
    }
    [[nodiscard]] std::size_t new_node256() {
        std::size_t idx;
        if (!fl_n256_.empty()) {
            idx = fl_n256_.back();
            fl_n256_.pop_back();
            n256_[idx] = N256{};
        } else {
            idx                            = n256_.size();
            std::size_t const old_capacity = n256_.capacity();
            n256_.push_back(N256{});
            record_capacity_growth_(old_capacity, n256_.capacity(), sizeof(N256));
        }
        return make_ref(kN256, idx);
    }

    std::vector<Leaf, leaf_allocator_type>         leaves_{};
    std::vector<N4, n4_allocator_type>             n4_{};
    std::vector<N16, n16_allocator_type>           n16_{};
    std::vector<N48, n48_allocator_type>           n48_{};
    std::vector<N256, n256_allocator_type>         n256_{};
    std::vector<std::size_t, index_allocator_type> fl_leaf_{};
    std::vector<std::size_t, index_allocator_type> fl_n4_{};
    std::vector<std::size_t, index_allocator_type> fl_n16_{};
    std::vector<std::size_t, index_allocator_type> fl_n48_{};
    std::vector<std::size_t, index_allocator_type> fl_n256_{};
    std::size_t                                    root_ = kNil;
    std::size_t                                    size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    std::uint64_t alloc_calls_     = 0;
    std::uint64_t bytes_allocated_ = 0;
#endif
};

// Selbstbeweis: das Substrat erfuellt das ArtTrieNodePool-Concept.
static_assert(ArtTrieNodePool<ArtTrieNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
