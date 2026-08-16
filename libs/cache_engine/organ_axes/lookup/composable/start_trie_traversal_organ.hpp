#pragma once
// V41 Umstufung-A s4 (Task #43) — StartTrieTraversal-Concept + StartTrieTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Das Multibyte-Span Adaptive-Radix-Descent-Organ (START, Fent et al. ICDEW 2020; is_original=false Re-Impl):
// wie ARTs Byte-Descent, aber ein Knoten konsumiert span(ref) BYTES (1/2/3) je Hop statt fix 1 — der
// charakteristische START-Multibyte-Span (Rewired64K=16-Bit / Rewired16M=24-Bit). depth advanciert um pl+span.
// Distinkt von ART (fixer 1-Byte-Span). Leaf-Split erzeugt Knoten mit PolicySpan (genuine Multibyte-Branches);
// Prefix-Split erzeugt span-1-Knoten (sicher: branched auf das EINE divergente Byte, prefix_cut(shared+1) immer
// gueltig). Mischspans im selben Baum sind korrekt, da der Descent span(ref) PRO Knoten liest.
//
// PolicySpan = S1-Knoten-Form-Policy (statisch). Die ADAPTIVE/optimale Span-Wahl je Subtree (Cost-Model-DP)
// ist die Folge-Achse axis_03t_node_tuning (data-driven Struktur-Wartung, KEIN Hot-Path-Switch). PolicySpan=1
// degeneriert exakt zu ART (Korrektheits-Anker). [[no-runtime-switch]]: rein statische Templates.
//
// INVARIANTEN: I1 (PolicySpan=1 == ART-Verhalten); I2 (Descent-Advance MUSS pl+span(ref) sein, nie pl+1 bei
// span>1); I3 (Leaf/Prefix-Split divergieren am ersten Disk-Byte -> discs verschieden); I4 (Erase lookup-korrekt
// ohne Shrink/Kollaps — Folge-Refinement); I5 (Free-List-Stabilitaet).

#include "start_trie_node_pool_concept.hpp"
#include "start_trie_node_pool_store.hpp" // fuer den Selbstbeweis am Dateiende
#include <topics/nodes/axis_02_path_compression/axis_02_path_compression_byte_wise.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// START-TRIE-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem StartTrieNodePool.
template <class T, class Pool>
concept StartTrieTraversal = StartTrieNodePool<Pool> &&
                             requires(Pool& p, Pool const& cp, typename Pool::key_type k, typename Pool::value_type v) {
                                 { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
                                 {
                                     T::template lookup_in<Pool>(cp, k)
                                 } -> std::same_as<std::optional<typename Pool::value_type>>;
                                 { T::template erase_from<Pool>(p, k) } -> std::same_as<bool>;
                             };

/// Multibyte-Span Adaptive-Radix-Traversal-Organ. PolicySpan = Span neuer Leaf-Split-Knoten (1/2/3).
template <unsigned PolicySpan = 2>
struct StartTrieTraversalOrgan {
    static_assert(PolicySpan >= 1 && PolicySpan <= 3, "PolicySpan in {1,2,3}");
    using prefix_t = ::comdare::cache_engine::nodes::axis_02_path_compression::ByteWiseKeyPrefix;

    [[nodiscard]] static constexpr std::uint32_t span_mask(unsigned s) noexcept {
        return (s == 1U) ? 0xFFu : (s == 2U) ? 0xFFFFu : 0xFFFFFFu;
    }
    /// span-breiter Diskriminator bei Tiefe d (Bytes); d>=8 -> 0 (Schluessel erschoepft).
    [[nodiscard]] static constexpr std::uint32_t slice(std::uint64_t key, unsigned d, unsigned s) noexcept {
        return (d >= 8U) ? std::uint32_t{0} : static_cast<std::uint32_t>((key >> (d * 8U)) & span_mask(s));
    }

    template <class Pool>
    static void link_parent(Pool& p, std::size_t parent, std::uint32_t parent_disc, std::size_t new_ref) {
        if (parent == Pool::kNil)
            p.set_root(new_ref);
        else
            p.set_child(parent, parent_disc, new_ref);
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type key, typename Pool::value_type value) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) {
            p.set_root(p.new_leaf(key, value));
            p.inc_size();
            return;
        }

        std::size_t   parent      = NIL;
        std::uint32_t parent_disc = 0;
        std::size_t   ref         = p.root();
        unsigned      depth       = 0;
        for (;;) {
            if (p.is_leaf(ref)) {
                typename Pool::key_type const ek = p.leaf_key(ref);
                if (ek == key) {
                    p.set_leaf_value(ref, value);
                    return;
                } // Update
                // Leaf-Split: neuer Multibyte-Span-Knoten (PolicySpan) mit gemeinsamem Byte-Prefix.
                std::uint64_t const rem_e = (depth >= 8U) ? 0ULL : (ek >> (depth * 8U));
                std::uint64_t const rem_k = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
                unsigned const      plen  = prefix_t::shared_len(rem_e, rem_k, prefix_t::kCapacity);
                std::size_t const   nn    = p.new_inner(PolicySpan);
                p.set_prefix(nn, prefix_t::from_bytes(rem_k, plen));
                unsigned const d2 = depth + plen;
                p.add_child(nn, slice(ek, d2, PolicySpan), ref);
                p.add_child(nn, slice(key, d2, PolicySpan), p.new_leaf(key, value));
                link_parent(p, parent, parent_disc, nn);
                p.inc_size();
                return;
            }
            prefix_t const      prefix = p.prefix_of(ref);
            unsigned const      pl     = prefix.length();
            std::uint64_t const rem    = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
            unsigned const      shared = prefix.common_prefix_len(rem);
            if (shared < pl) {
                // Prefix-Split: neuer span-1-Knoten (branched auf das EINE divergente Byte; cut(shared+1) sicher).
                std::size_t const nn = p.new_inner(1);
                p.set_prefix(nn, prefix_t::from_bytes(prefix.packed_, shared));
                std::uint8_t const be = prefix[shared];
                p.prefix_cut(ref, shared + 1);
                p.add_child(nn, static_cast<std::uint32_t>(be), ref);
                std::uint32_t const bk = slice(key, depth + shared, 1);
                p.add_child(nn, bk, p.new_leaf(key, value));
                link_parent(p, parent, parent_disc, nn);
                p.inc_size();
                return;
            }
            // Voller Prefix-Match -> span(ref) Bytes tiefer dispatchen.
            depth += pl;
            unsigned const      s     = p.span(ref);
            std::uint32_t const disc  = slice(key, depth, s);
            std::size_t const   child = p.find_child(ref, disc);
            if (child == NIL) {
                p.add_child(ref, disc, p.new_leaf(key, value)); // sparse: kein Growth, keine Ref-Aenderung
                p.inc_size();
                return;
            }
            parent      = ref;
            parent_disc = disc;
            ref         = child;
            depth += s;
        }
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type key) {
        std::size_t const NIL   = Pool::kNil;
        std::size_t       ref   = p.root();
        unsigned          depth = 0;
        while (ref != NIL) {
            if (p.is_leaf(ref)) {
                return (p.leaf_key(ref) == key) ? std::optional<typename Pool::value_type>{p.leaf_value(ref)}
                                                : std::nullopt;
            }
            prefix_t const      prefix = p.prefix_of(ref);
            unsigned const      pl     = prefix.length();
            std::uint64_t const rem    = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
            if (prefix.common_prefix_len(rem) < pl) return std::nullopt;
            depth += pl;
            unsigned const    s     = p.span(ref);
            std::size_t const child = p.find_child(ref, slice(key, depth, s));
            if (child == NIL) return std::nullopt;
            ref = child;
            depth += s;
        }
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        std::size_t       ref = p.root();
        if (ref == NIL) return false;
        std::size_t   parent      = NIL;
        std::uint32_t parent_disc = 0;
        unsigned      depth       = 0;
        for (;;) {
            if (p.is_leaf(ref)) {
                if (p.leaf_key(ref) != key) return false;
                if (parent == NIL)
                    p.set_root(NIL);
                else
                    p.remove_child(parent, parent_disc);
                p.free_node(ref);
                p.dec_size();
                return true;
            }
            prefix_t const      prefix = p.prefix_of(ref);
            unsigned const      pl     = prefix.length();
            std::uint64_t const rem    = (depth >= 8U) ? 0ULL : (key >> (depth * 8U));
            if (prefix.common_prefix_len(rem) < pl) return false;
            depth += pl;
            unsigned const      s     = p.span(ref);
            std::uint32_t const disc  = slice(key, depth, s);
            std::size_t const   child = p.find_child(ref, disc);
            if (child == NIL) return false;
            parent      = ref;
            parent_disc = disc;
            ref         = child;
            depth += s;
        }
    }
};

// Selbstbeweis: StartTrieTraversalOrgan erfuellt das StartTrieTraversal-Concept ueber dem Pilot-Pool (span-2 + span-1).
static_assert(StartTrieTraversal<StartTrieTraversalOrgan<2>, StartTrieNodePoolStore<>>);
static_assert(StartTrieTraversal<StartTrieTraversalOrgan<1>, StartTrieNodePoolStore<>>);

} // namespace comdare::cache_engine::lookup::composable
