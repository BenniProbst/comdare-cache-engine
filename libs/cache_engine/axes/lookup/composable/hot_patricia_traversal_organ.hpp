#pragma once
// V41 Umstufung-A s4 (Task #43) — HotPatriciaTraversal-Concept + HotPatriciaTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Analog zum ArtTrieTraversalOrgan, aber BIT-level statt byte-level: binary crit-bit-Patricia (HOT-Korrektheits-
// Basis, Binna et al. SIGMOD 2018), portiert als is_original=false C++23-Re-Impl ([[pseudocode-papers-fallback]]).
// Zwei Schluessel werden durch GENAU EIN Bit getrennt (das hoechstwertige divergierende, `countl_zero(a^b)`,
// MSB-first); nicht-diskriminative Bits werden uebersprungen (Single-Bit-Split Path-Compression). Distinkt von
// ART (byte-level, 256-Fanout, LSB-first) — NICHT vermischen.
//
// INVARIANTEN (binary Patricia): I1 jeder Internal hat GENAU 2 nicht-nil Kinder (Erase kollabiert IMMER);
// I2 crit_bit STRENG MONOTON STEIGEND auf jedem Root->Leaf-Pfad (Folge der strikt-`<`-Einfuegebedingung);
// I3 child[0]=Bit-0-Teilbaum, child[1]=Bit-1 (sortiert, MSB-first -> supports_range_scan); I4 alle Keys unter
// einem Internal teilen Bits 0..crit_bit-1; I5 size==#Leaves (n Leaves -> n-1 Internals).
// Multi-Bit-Height-Optimization (SparsePartialKeys + SIMD) = Folge-Increment (aendert Semantik NICHT).
// [[no-runtime-switch]]: rein statische Templates.

#include "hot_patricia_node_pool_concept.hpp"
#include "hot_patricia_node_pool_store.hpp"   // fuer den Selbstbeweis am Dateiende

#include <bit>          // std::countl_zero
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// HOT-PATRICIA-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem HotPatriciaNodePool.
template <class T, class Pool>
concept HotPatriciaTraversal = HotPatriciaNodePool<Pool> && requires(Pool& p, Pool const& cp,
                                  typename Pool::key_type k, typename Pool::value_type v) {
    { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
    { T::template lookup_in<Pool>(cp, k) }     -> std::same_as<std::optional<typename Pool::value_type>>;
    { T::template erase_from<Pool>(p, k) }     -> std::same_as<bool>;
};

/// Binary crit-bit-Patricia-Traversal-Organ. Navigiert + transformiert ausschliesslich ueber die Pool-API.
struct HotPatriciaTraversalOrgan {
    /// Hoechstwertiges divergierendes Bit zweier Schluessel (MSB-first, 0..63). NUR mit a!=b rufen!
    /// (FALLE 1: a==b -> countl_zero(0)==64 -> ungueltiger crit_bit -> dir-Shift-UB. In insert via Update-Pfad abgefangen.)
    [[nodiscard]] static constexpr unsigned first_diff_bit(std::uint64_t a, std::uint64_t b) noexcept {
        return static_cast<unsigned>(std::countl_zero(a ^ b));
    }
    /// Richtung (0/1) des Schluessels am crit_bit (MSB-first). child[0]=kleinere, child[1]=groessere Keys.
    [[nodiscard]] static constexpr unsigned dir(std::uint64_t key, unsigned crit_bit) noexcept {
        return static_cast<unsigned>((key >> (63U - crit_bit)) & 1ULL);
    }

    template <class Pool>
    static void link_parent(Pool& p, std::size_t parent, unsigned parent_dir, std::size_t new_ref) {
        if (parent == Pool::kNil) p.set_root(new_ref);
        else                      p.set_child(parent, parent_dir, new_ref);
    }

    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type key, typename Pool::value_type value) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) { p.set_root(p.new_leaf(key, value)); p.inc_size(); return; }

        // Phase 1: Leaf-Descent OHNE Zwischenvergleich (Patricia ueberspringt nicht-diskriminative Bits).
        std::size_t ref = p.root();
        while (!p.is_leaf(ref)) ref = p.child(ref, dir(key, p.crit_bit(ref)));

        // Phase 2: Kandidat-Pruefung (a==b-Schutz vor first_diff_bit).
        typename Pool::key_type const ek = p.leaf_key(ref);
        if (ek == key) { p.set_leaf_value(ref, value); return; }   // Update, KEIN inc_size

        unsigned const newbit = first_diff_bit(ek, key);           // garantiert 0..63

        // Phase 3: Re-Descent, Einfuegepunkt suchen (STRIKT `<` — siehe I2).
        std::size_t parent = NIL;
        unsigned    parent_dir = 0;
        ref = p.root();
        while (!p.is_leaf(ref) && p.crit_bit(ref) < newbit) {
            parent = ref; parent_dir = dir(key, p.crit_bit(ref)); ref = p.child(ref, parent_dir);
        }

        // Internal einziehen: bisheriger Teilbaum `ref` auf Seite (1-nd), neuer Leaf auf Seite nd.
        unsigned const nd = dir(key, newbit);
        std::size_t const newleaf = p.new_leaf(key, value);
        std::size_t const newint  = (nd == 1U) ? p.new_internal(newbit, /*c0=*/ref,     /*c1=*/newleaf)
                                               : p.new_internal(newbit, /*c0=*/newleaf, /*c1=*/ref);
        link_parent(p, parent, parent_dir, newint);
        p.inc_size();
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        std::size_t ref = p.root();
        if (ref == NIL) return std::nullopt;
        while (!p.is_leaf(ref)) ref = p.child(ref, dir(key, p.crit_bit(ref)));
        return (p.leaf_key(ref) == key) ? std::optional<typename Pool::value_type>{p.leaf_value(ref)}
                                        : std::nullopt;   // EIN voller Full-Key-Vergleich, zwingend (Kandidat!)
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        std::size_t ref = p.root();
        if (ref == NIL) return false;

        // Sonderfall: Wurzel ist Leaf (Einzel-Element).
        if (p.is_leaf(ref)) {
            if (p.leaf_key(ref) != key) return false;
            p.free_node(ref); p.set_root(NIL); p.dec_size(); return true;
        }

        // Descent mit grandparent/parent/dir-Tracking.
        std::size_t gp = NIL;  unsigned gp_dir = 0;
        std::size_t par = ref; unsigned par_dir = dir(key, p.crit_bit(ref));
        std::size_t cur = p.child(ref, par_dir);
        while (!p.is_leaf(cur)) {
            gp = par; gp_dir = par_dir;
            par = cur; par_dir = dir(key, p.crit_bit(cur)); cur = p.child(cur, par_dir);
        }
        if (p.leaf_key(cur) != key) return false;

        // Collapse: par hat GENAU 2 Kinder (I1) -> Geschwister-Kind ersetzt par.
        std::size_t const sibling = p.child(par, 1U - par_dir);
        if (gp == NIL) p.set_root(sibling);                 // par IST root
        else           p.set_child(gp, gp_dir, sibling);
        p.free_node(cur); p.free_node(par);                 // Doppel-free (Leaf + kollabierter Internal)
        p.dec_size();
        return true;
    }
};

// Selbstbeweis: HotPatriciaTraversalOrgan erfuellt das HotPatriciaTraversal-Concept ueber dem Pilot-Pool.
static_assert(HotPatriciaTraversal<HotPatriciaTraversalOrgan, HotPatriciaNodePoolStore>);

}  // namespace comdare::cache_engine::lookup::composable
