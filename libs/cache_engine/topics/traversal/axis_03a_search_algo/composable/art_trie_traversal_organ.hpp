#pragma once
// V41 Umstufung-A s4 (Task #43) — ArtTrieTraversal-Concept + ArtTrieTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Analog zum TreeTraversalOrgan, aber ueber einem ArtTrieNodePool: statische insert_into/lookup_in/erase_from,
// KEIN Eigenspeicher. ArtTrieTraversalOrgan = ADAPTIVE-RADIX-TREE-Descent (Leis ICDE 2013), portiert aus
// unodb art_internal_impl.hpp (get_internal / insert_internal): Byte-fuer-Byte-Abstieg (256-Fanout), Path-
// Compression-Skip ueber das ByteWiseKeyPrefix-Organ, Leaf-Split (neuer N4 bei Schluessel-Divergenz),
// Prefix-Split (neuer N4 bei Prefix-Divergenz). Knoten-GROWTH (N4->16->48->256) macht der Pool in add_child
// daten-getrieben. is_original=false ([[pseudocode-papers-fallback]]); SIMD/echtes Linking = Folge-Increment.
//
// **Little-Endian-Descent (unodb-treu):** Bei Tiefe d ist das relevante Byte (key >> (d*8)) & 0xFF; der
// "Rest-Schluessel" ist key >> (d*8). Das ByteWiseKeyPrefix speichert die naechsten Prefix-Bytes LSB-first —
// konsistent zu common_prefix_len. Erase ist lookup-korrekt OHNE Shrink/Kollaps (Folge-Refinement).
// [[no-runtime-switch]]: rein statische Templates; Growth ist DATEN-getrieben im Pool.

#include "art_trie_node_pool_concept.hpp"
#include "art_trie_node_pool_store.hpp"   // fuer den Selbstbeweis am Dateiende

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable {

/// ART-TRIE-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem ArtTrieNodePool.
template <class T, class Pool>
concept ArtTrieTraversal = ArtTrieNodePool<Pool> && requires(Pool& p, Pool const& cp,
                                  typename Pool::key_type k, typename Pool::value_type v) {
    { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
    { T::template lookup_in<Pool>(cp, k) }     -> std::same_as<std::optional<typename Pool::value_type>>;
    { T::template erase_from<Pool>(p, k) }     -> std::same_as<bool>;
};

/// Adaptive-Radix-Tree-Traversal-Organ. Navigiert + transformiert ausschliesslich ueber die Pool-API.
struct ArtTrieTraversalOrgan {
    using prefix_t = ::comdare::cache_engine::nodes::axis_02_path_compression::ByteWiseKeyPrefix;

    /// Byte an Trie-Tiefe d (in Bytes); >=8 -> 0 (Schluessel erschoepft; nur defensiv, Inner-Node nie bei d>=8).
    [[nodiscard]] static constexpr std::uint8_t byte_at(std::uint64_t key, unsigned d) noexcept {
        return (d >= 8U) ? std::uint8_t{0} : static_cast<std::uint8_t>((key >> (d * 8U)) & 0xFFU);
    }

    /// Eltern-Slot nach Knoten-Ersetzung (Growth/Split) aktualisieren: Wurzel oder Kind-Slot des Elternteils.
    template <class Pool>
    static void link_parent(Pool& p, std::size_t parent, std::uint8_t parent_byte, std::size_t new_ref) {
        if (parent == Pool::kNil) p.set_root(new_ref);
        else                      p.set_child(parent, parent_byte, new_ref);
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type key, typename Pool::value_type value) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) { p.set_root(p.new_leaf(key, value)); p.inc_size(); return; }

        std::size_t parent = NIL;
        std::uint8_t parent_byte = 0;
        std::size_t ref = p.root();
        unsigned depth = 0;
        for (;;) {
            if (p.is_leaf(ref)) {
                typename Pool::key_type const ek = p.leaf_key(ref);
                if (ek == key) { p.set_leaf_value(ref, value); return; }   // Update (keine neue Schluesselzahl)
                // Leaf-Split: neuer N4 mit gemeinsamem Byte-Prefix der Rest-Schluessel.
                std::uint64_t const rem_e = (depth >= 8U) ? 0ULL : (ek  >> (depth * 8U));
                std::uint64_t const rem_k = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
                unsigned const plen = prefix_t::shared_len(rem_e, rem_k, prefix_t::kCapacity);
                std::size_t n4 = p.new_node4();
                p.set_prefix(n4, prefix_t::from_bytes(rem_k, plen));
                unsigned const d2 = depth + plen;
                n4 = p.add_child(n4, byte_at(ek, d2), ref);
                n4 = p.add_child(n4, byte_at(key, d2), p.new_leaf(key, value));
                link_parent(p, parent, parent_byte, n4);
                p.inc_size();
                return;
            }
            prefix_t const prefix = p.prefix_of(ref);
            unsigned const pl = prefix.length();
            std::uint64_t const rem = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
            unsigned const shared = prefix.common_prefix_len(rem);   // geklemmt auf pl
            if (shared < pl) {
                // Prefix-Split: neuer N4 traegt die ersten `shared` Bytes; der bestehende Knoten verliert
                // `shared`+1 fuehrende Prefix-Bytes (das divergente Byte wandert als Kind-Key in den N4).
                std::size_t n4 = p.new_node4();
                p.set_prefix(n4, prefix_t::from_bytes(prefix.packed_, shared));
                std::uint8_t const be = prefix[shared];               // divergentes Byte (Bestands-Seite)
                p.prefix_cut(ref, shared + 1);
                n4 = p.add_child(n4, be, ref);
                std::uint8_t const bk = byte_at(key, depth + shared);  // divergentes Byte (neuer Schluessel)
                n4 = p.add_child(n4, bk, p.new_leaf(key, value));
                link_parent(p, parent, parent_byte, n4);
                p.inc_size();
                return;
            }
            // Voller Prefix-Match -> ein Byte tiefer dispatchen.
            depth += pl;
            std::uint8_t const b = byte_at(key, depth);
            std::size_t const child = p.find_child(ref, b);
            if (child == NIL) {
                std::size_t const new_ref = p.add_child(ref, b, p.new_leaf(key, value));  // ggf. Growth
                if (new_ref != ref) link_parent(p, parent, parent_byte, new_ref);
                p.inc_size();
                return;
            }
            parent = ref; parent_byte = b; ref = child; depth += 1;
        }
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        std::size_t ref = p.root();
        unsigned depth = 0;
        while (ref != NIL) {
            if (p.is_leaf(ref)) {
                return (p.leaf_key(ref) == key) ? std::optional<typename Pool::value_type>{p.leaf_value(ref)}
                                                : std::nullopt;
            }
            prefix_t const prefix = p.prefix_of(ref);
            unsigned const pl  = prefix.length();
            std::uint64_t const rem = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
            if (prefix.common_prefix_len(rem) < pl) return std::nullopt;   // Prefix-Mismatch
            depth += pl;
            std::size_t const child = p.find_child(ref, byte_at(key, depth));
            if (child == NIL) return std::nullopt;
            ref = child; depth += 1;
        }
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        std::size_t ref = p.root();
        if (ref == NIL) return false;
        std::size_t parent = NIL;
        std::uint8_t parent_byte = 0;
        unsigned depth = 0;
        for (;;) {
            if (p.is_leaf(ref)) {
                if (p.leaf_key(ref) != key) return false;
                if (parent == NIL) p.set_root(NIL);
                else               p.remove_child(parent, parent_byte);
                p.free_node(ref);
                p.dec_size();
                return true;
            }
            prefix_t const prefix = p.prefix_of(ref);
            unsigned const pl = prefix.length();
            std::uint64_t const rem = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
            if (prefix.common_prefix_len(rem) < pl) return false;
            depth += pl;
            std::uint8_t const b = byte_at(key, depth);
            std::size_t const child = p.find_child(ref, b);
            if (child == NIL) return false;
            parent = ref; parent_byte = b; ref = child; depth += 1;
        }
    }
};

// Selbstbeweis: ArtTrieTraversalOrgan erfuellt das ArtTrieTraversal-Concept ueber dem Pilot-Pool.
static_assert(ArtTrieTraversal<ArtTrieTraversalOrgan, ArtTrieNodePoolStore>);

}  // namespace comdare::cache_engine::traversal::axis_03a_search_algo::composable
