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
#include "../../../nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp"  // ByteWiseKeyPrefix

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

class ArtTrieNodePoolStore {
public:
    using key_type    = std::uint64_t;
    using value_type  = std::uint64_t;
    using prefix_type = ::comdare::cache_engine::nodes::axis_02_path_compression::ByteWiseKeyPrefix;
    static constexpr std::size_t kNil = std::numeric_limits<std::size_t>::max();

    // NodeRef-Kodierung: Kind in Bits 56-63, Index in Bits 0-55.
    enum Kind : std::uint8_t { kLeaf = 0, kN4 = 1, kN16 = 2, kN48 = 3, kN256 = 4 };
    [[nodiscard]] static constexpr std::size_t  make_ref(Kind kind, std::size_t idx) noexcept {
        return (static_cast<std::size_t>(kind) << 56U) | idx;
    }
    [[nodiscard]] static constexpr Kind         ref_kind(std::size_t r) noexcept { return static_cast<Kind>(r >> 56U); }
    [[nodiscard]] static constexpr std::size_t  ref_idx(std::size_t r)  noexcept { return r & 0x00FF'FFFF'FFFF'FFFFULL; }

    // ── Wurzel + Groesse ──
    [[nodiscard]] std::size_t root() const noexcept { return root_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    void set_root(std::size_t r) noexcept { root_ = r; }
    void inc_size()              noexcept { ++size_; }
    void dec_size()              noexcept { --size_; }
    void clear() noexcept {
        leaves_.clear(); n4_.clear(); n16_.clear(); n48_.clear(); n256_.clear();
        fl_leaf_.clear(); fl_n4_.clear(); fl_n16_.clear(); fl_n48_.clear(); fl_n256_.clear();
        root_ = kNil; size_ = 0;
    }

    // ── Leaf ──
    [[nodiscard]] bool       is_leaf(std::size_t r)    const noexcept { return ref_kind(r) == kLeaf; }
    [[nodiscard]] key_type   leaf_key(std::size_t r)   const noexcept { return leaves_[ref_idx(r)].key; }
    [[nodiscard]] value_type leaf_value(std::size_t r) const noexcept { return leaves_[ref_idx(r)].val; }
    void set_leaf_value(std::size_t r, value_type v)   noexcept { leaves_[ref_idx(r)].val = v; }
    [[nodiscard]] std::size_t new_leaf(key_type k, value_type v) {
        std::size_t idx;
        if (!fl_leaf_.empty()) { idx = fl_leaf_.back(); fl_leaf_.pop_back(); leaves_[idx] = Leaf{k, v}; }
        else { idx = leaves_.size(); leaves_.push_back(Leaf{k, v}); }
        return make_ref(kLeaf, idx);
    }

    // ── Inner: Prefix + Kind-Zahl ──
    [[nodiscard]] std::size_t new_node4() {
        std::size_t idx;
        if (!fl_n4_.empty()) { idx = fl_n4_.back(); fl_n4_.pop_back(); n4_[idx] = N4{}; }
        else { idx = n4_.size(); n4_.push_back(N4{}); }
        return make_ref(kN4, idx);
    }
    [[nodiscard]] prefix_type prefix_of(std::size_t r) const noexcept {
        switch (ref_kind(r)) {
            case kN4:   return n4_[ref_idx(r)].prefix;
            case kN16:  return n16_[ref_idx(r)].prefix;
            case kN48:  return n48_[ref_idx(r)].prefix;
            case kN256: return n256_[ref_idx(r)].prefix;
            default:    return prefix_type{};
        }
    }
    void set_prefix(std::size_t r, prefix_type p) noexcept { mutable_prefix(r) = p; }
    void prefix_cut(std::size_t r, unsigned n)    noexcept { mutable_prefix(r).cut(n); }
    [[nodiscard]] int node_n(std::size_t r) const noexcept {
        switch (ref_kind(r)) {
            case kN4:   return n4_[ref_idx(r)].n;
            case kN16:  return n16_[ref_idx(r)].n;
            case kN48:  return n48_[ref_idx(r)].n;
            case kN256: return n256_[ref_idx(r)].n;
            default:    return 0;
        }
    }

    // ── Byte-Dispatch (skalar) ──
    [[nodiscard]] std::size_t find_child(std::size_t r, std::uint8_t b) const noexcept {
        switch (ref_kind(r)) {
            case kN4: {
                N4 const& x = n4_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i) if (x.keys[i] == b) return x.kids[i];
                return kNil;
            }
            case kN16: {
                N16 const& x = n16_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i) if (x.keys[i] == b) return x.kids[i];
                return kNil;
            }
            case kN48: {
                N48 const& x = n48_[ref_idx(r)];
                std::uint8_t const slot = x.child_index[b];
                return (slot == kEmpty48) ? kNil : x.kids[slot];
            }
            case kN256: return n256_[ref_idx(r)].kids[b];
            default:    return kNil;
        }
    }

    /// Fuegt (b, child) ein (b ist garantiert NICHT vorhanden). Bei vollem Knoten: GROWTH zum naechsten Typ,
    /// dann Einfuegen. Liefert die (ggf. neue, gewachsene) Node-Ref.
    [[nodiscard]] std::size_t add_child(std::size_t r, std::uint8_t b, std::size_t child) {
        switch (ref_kind(r)) {
            case kN4: {
                if (n4_[ref_idx(r)].n < 4) { insert_sorted_n4(ref_idx(r), b, child); return r; }
                std::size_t const g = grow_n4_to_n16(ref_idx(r));
                insert_sorted_n16(ref_idx(g), b, child); return g;
            }
            case kN16: {
                if (n16_[ref_idx(r)].n < 16) { insert_sorted_n16(ref_idx(r), b, child); return r; }
                std::size_t const g = grow_n16_to_n48(ref_idx(r));
                insert_n48(ref_idx(g), b, child); return g;
            }
            case kN48: {
                if (n48_[ref_idx(r)].n < 48) { insert_n48(ref_idx(r), b, child); return r; }
                std::size_t const g = grow_n48_to_n256(ref_idx(r));
                insert_n256(ref_idx(g), b, child); return g;
            }
            case kN256: insert_n256(ref_idx(r), b, child); return r;
            default:    return r;
        }
    }

    /// Ersetzt den Kind-Slot fuer das bereits vorhandene Byte b (kein Growth, keine Zahl-Aenderung).
    void set_child(std::size_t r, std::uint8_t b, std::size_t child) noexcept {
        switch (ref_kind(r)) {
            case kN4: { N4& x = n4_[ref_idx(r)];  for (int i = 0; i < x.n; ++i) if (x.keys[i] == b) { x.kids[i] = child; return; } return; }
            case kN16:{ N16& x = n16_[ref_idx(r)]; for (int i = 0; i < x.n; ++i) if (x.keys[i] == b) { x.kids[i] = child; return; } return; }
            case kN48:{ N48& x = n48_[ref_idx(r)]; std::uint8_t const s = x.child_index[b]; if (s != kEmpty48) x.kids[s] = child; return; }
            case kN256: n256_[ref_idx(r)].kids[b] = child; return;
            default: return;
        }
    }

    /// Entfernt den Kind-Slot fuer Byte b (Erase). KEIN Shrink (under-volle Knoten sind lookup-korrekt;
    /// die N4<-N16<-N48<-N256-Schrumpfung + Kollaps ist ein Folge-Refinement-Increment). b ist vorhanden.
    void remove_child(std::size_t r, std::uint8_t b) noexcept {
        switch (ref_kind(r)) {
            case kN4: { N4& x = n4_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i) if (x.keys[i] == b) {
                    for (int j = i + 1; j < x.n; ++j) { x.keys[j - 1] = x.keys[j]; x.kids[j - 1] = x.kids[j]; }
                    --x.n; return;
                } return; }
            case kN16: { N16& x = n16_[ref_idx(r)];
                for (int i = 0; i < x.n; ++i) if (x.keys[i] == b) {
                    for (int j = i + 1; j < x.n; ++j) { x.keys[j - 1] = x.keys[j]; x.kids[j - 1] = x.kids[j]; }
                    --x.n; return;
                } return; }
            case kN48: { N48& x = n48_[ref_idx(r)];
                if (x.child_index[b] != kEmpty48) { x.child_index[b] = kEmpty48; --x.n; } return; }   // Slot geleakt bis free_node (ok)
            case kN256: { N256& x = n256_[ref_idx(r)];
                if (x.kids[b] != kNil) { x.kids[b] = kNil; --x.n; } return; }
            default: return;
        }
    }

    void free_node(std::size_t r) noexcept {
        switch (ref_kind(r)) {
            case kLeaf: fl_leaf_.push_back(ref_idx(r)); return;
            case kN4:   fl_n4_.push_back(ref_idx(r));   return;
            case kN16:  fl_n16_.push_back(ref_idx(r));  return;
            case kN48:  fl_n48_.push_back(ref_idx(r));  return;
            case kN256: fl_n256_.push_back(ref_idx(r)); return;
        }
    }

private:
    static constexpr std::uint8_t kEmpty48 = 0xFFu;   // "kein Slot" im N48-child_index

    struct Leaf { key_type key{}; value_type val{}; };
    struct N4   { prefix_type prefix{}; int n = 0; std::array<std::uint8_t, 4>  keys{}; std::array<std::size_t, 4>  kids{}; };
    struct N16  { prefix_type prefix{}; int n = 0; std::array<std::uint8_t, 16> keys{}; std::array<std::size_t, 16> kids{}; };
    struct N48  { prefix_type prefix{}; int n = 0; std::array<std::uint8_t, 256> child_index{}; std::array<std::size_t, 48> kids{};
                  N48() { child_index.fill(kEmpty48); } };
    struct N256 { prefix_type prefix{}; int n = 0; std::array<std::size_t, 256> kids{};
                  N256() { kids.fill(kNil); } };

    [[nodiscard]] prefix_type& mutable_prefix(std::size_t r) noexcept {
        switch (ref_kind(r)) {
            case kN4:   return n4_[ref_idx(r)].prefix;
            case kN16:  return n16_[ref_idx(r)].prefix;
            case kN48:  return n48_[ref_idx(r)].prefix;
            default:    return n256_[ref_idx(r)].prefix;
        }
    }

    // ── sortiertes Einfuegen (N4/N16) ──
    void insert_sorted_n4(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N4& x = n4_[idx];
        int pos = x.n;
        while (pos > 0 && x.keys[pos - 1] > b) { x.keys[pos] = x.keys[pos - 1]; x.kids[pos] = x.kids[pos - 1]; --pos; }
        x.keys[pos] = b; x.kids[pos] = child; ++x.n;
    }
    void insert_sorted_n16(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N16& x = n16_[idx];
        int pos = x.n;
        while (pos > 0 && x.keys[pos - 1] > b) { x.keys[pos] = x.keys[pos - 1]; x.kids[pos] = x.kids[pos - 1]; --pos; }
        x.keys[pos] = b; x.kids[pos] = child; ++x.n;
    }
    void insert_n48(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N48& x = n48_[idx];
        int const slot = x.n;                          // naechster freier Slot (append-only innerhalb Lebensdauer)
        x.kids[static_cast<std::size_t>(slot)] = child;
        x.child_index[b] = static_cast<std::uint8_t>(slot);
        ++x.n;
    }
    void insert_n256(std::size_t idx, std::uint8_t b, std::size_t child) noexcept {
        N256& x = n256_[idx];
        x.kids[b] = child; ++x.n;
    }

    // ── GROWTH (kopiert Prefix + alle Kinder in den groesseren Typ, gibt alten frei) ──
    [[nodiscard]] std::size_t grow_n4_to_n16(std::size_t idx) {
        std::size_t const r16 = new_node16();
        N16& d = n16_[ref_idx(r16)];
        N4&  s = n4_[idx];
        d.prefix = s.prefix; d.n = s.n;
        for (int i = 0; i < s.n; ++i) { d.keys[i] = s.keys[i]; d.kids[i] = s.kids[i]; }
        fl_n4_.push_back(idx);
        return r16;
    }
    [[nodiscard]] std::size_t grow_n16_to_n48(std::size_t idx) {
        std::size_t const r48 = new_node48();
        N48& d = n48_[ref_idx(r48)];
        N16& s = n16_[idx];
        d.prefix = s.prefix; d.n = s.n;
        for (int i = 0; i < s.n; ++i) { d.kids[static_cast<std::size_t>(i)] = s.kids[i]; d.child_index[s.keys[i]] = static_cast<std::uint8_t>(i); }
        fl_n16_.push_back(idx);
        return r48;
    }
    [[nodiscard]] std::size_t grow_n48_to_n256(std::size_t idx) {
        std::size_t const r256 = new_node256();
        N256& d = n256_[ref_idx(r256)];
        N48&  s = n48_[idx];
        d.prefix = s.prefix; d.n = s.n;
        for (int b = 0; b < 256; ++b) { std::uint8_t const slot = s.child_index[static_cast<std::size_t>(b)]; if (slot != kEmpty48) d.kids[static_cast<std::size_t>(b)] = s.kids[slot]; }
        fl_n48_.push_back(idx);
        return r256;
    }
    [[nodiscard]] std::size_t new_node16() {
        std::size_t idx;
        if (!fl_n16_.empty()) { idx = fl_n16_.back(); fl_n16_.pop_back(); n16_[idx] = N16{}; }
        else { idx = n16_.size(); n16_.push_back(N16{}); }
        return make_ref(kN16, idx);
    }
    [[nodiscard]] std::size_t new_node48() {
        std::size_t idx;
        if (!fl_n48_.empty()) { idx = fl_n48_.back(); fl_n48_.pop_back(); n48_[idx] = N48{}; }
        else { idx = n48_.size(); n48_.push_back(N48{}); }
        return make_ref(kN48, idx);
    }
    [[nodiscard]] std::size_t new_node256() {
        std::size_t idx;
        if (!fl_n256_.empty()) { idx = fl_n256_.back(); fl_n256_.pop_back(); n256_[idx] = N256{}; }
        else { idx = n256_.size(); n256_.push_back(N256{}); }
        return make_ref(kN256, idx);
    }

    std::vector<Leaf> leaves_{};
    std::vector<N4>   n4_{};
    std::vector<N16>  n16_{};
    std::vector<N48>  n48_{};
    std::vector<N256> n256_{};
    std::vector<std::size_t> fl_leaf_{}, fl_n4_{}, fl_n16_{}, fl_n48_{}, fl_n256_{};
    std::size_t root_ = kNil;
    std::size_t size_ = 0;
};

// Selbstbeweis: das Substrat erfuellt das ArtTrieNodePool-Concept.
static_assert(ArtTrieNodePool<ArtTrieNodePoolStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
