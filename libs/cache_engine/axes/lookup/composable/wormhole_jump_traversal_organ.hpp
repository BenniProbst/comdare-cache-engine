#pragma once
// V41 Umstufung-A s4 (Task #43) — WormholeTraversal-Concept + WormholeJumpTraversalOrgan.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Das HYBRID-Organ (Wu/Ni/Jiang EuroSys 2019, is_original=false Re-Impl aus dem Verstaendnis): Hash-JUMP auf
// den groessten Anker<=key (index_lookup_le) statt Wurzel-Abstieg, gefolgt von lokaler sortierter Suche im
// Ziel-Leaf. Distinkt von ART (byte-Trie), HOT (bit-Patricia), B-Baum (Wurzel-Abstieg). Leaf-Split bei
// Overflow + Borrow/Merge bei Unterlauf (B-Baum-Vorlage, auf der doppelt-verketteten LISTE statt im Baum).
//
// **B-Netz (Korrektheit ⊥ Index):** locate_leaf re-justiert nach dem Hash-Jump entlang prev/next, bis das
// Leaf mit groesstem anchor<=key erreicht ist. Damit ist JEDER stale/leere/falsche Index-Treffer harmlos —
// der Hash-Jump verschiebt nur den Startpunkt, die geordnete Leaf-Liste traegt die Korrektheit.
//
// INVARIANTEN: W1 Anchors strikt aufsteigend entlang head->next; W2 Leaf intern sortiert+duplikatfrei,
// 0<=n<=kWhKpn (transient n==kWhKpn+1 zwischen Shift-Insert und split_leaf — daher Array-Kapazitaet kWhKpn+1);
// W3 genau EIN besitzendes Leaf je Key (groesster anchor<=key); W4 leaf_anchor==key[0];
// W5 size==Sum leaf_n; W6 Index-Konsistenz (nur Performance, NIE Korrektheit); W7 Listen-Integritaet.
// [[no-runtime-switch]]: rein statische Templates. Concurrency/SIMD/Cuckoo = Folge-Increments/andere Achsen.

#include "wormhole_leaf_list_pool_concept.hpp"
#include "wormhole_leaf_list_pool_store.hpp"   // fuer den Selbstbeweis am Dateiende

#include <concepts>
#include <cstddef>
#include <optional>

namespace comdare::cache_engine::lookup::composable {

/// WORMHOLE-TRAVERSAL-Organ-Concept: statische insert_into/lookup_in/erase_from auf einem WormholeLeafListPool.
template <class T, class Pool>
concept WormholeTraversal = WormholeLeafListPool<Pool> && requires(Pool& p, Pool const& cp,
                                  typename Pool::key_type k, typename Pool::value_type v) {
    { T::template insert_into<Pool>(p, k, v) } -> std::same_as<void>;
    { T::template lookup_in<Pool>(cp, k) }     -> std::same_as<std::optional<typename Pool::value_type>>;
    { T::template erase_from<Pool>(p, k) }     -> std::same_as<bool>;
};

/// Wormhole-Jump-Traversal-Organ. Navigiert + transformiert ausschliesslich ueber die Pool-API.
struct WormholeJumpTraversalOrgan {
    /// Hash-Jump (groesster Anker<=key) + Re-Justierung entlang der Liste -> besitzendes Leaf (oder Kopf-Leaf).
    template <class Pool>
    [[nodiscard]] static std::size_t locate_leaf(Pool const& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        std::size_t cand = p.index_lookup_le(key);   // Hash-Jump
        if (cand == NIL) cand = p.root();             // Fallback: key < allen Ankern -> Listenkopf
        if (cand == NIL) return NIL;                  // leerer Pool
        // RE-JUSTIERUNG (macht jeden stale/falschen Sprung harmlos):
        while (p.leaf_next(cand) != NIL && p.leaf_anchor(p.leaf_next(cand)) <= key) cand = p.leaf_next(cand);
        while (cand != p.root() && p.leaf_anchor(cand) > key) cand = p.leaf_prev(cand);
        return cand;
    }

    /// Erste Position >= key im sortierten Leaf-Block (Binaersuche).
    template <class Pool>
    [[nodiscard]] static int leaf_lower_bound(Pool const& p, std::size_t leaf, typename Pool::key_type key) {
        int lo = 0, hi = p.leaf_n(leaf);
        while (lo < hi) { int const mid = (lo + hi) / 2; if (p.leaf_key_at(leaf, mid) < key) lo = mid + 1; else hi = mid; }
        return lo;
    }

    /// Anchor-Maintenance: wenn das Leaf-Minimum sich aendert, Anker + Index nachziehen (W4/W6).
    template <class Pool>
    static void reanchor(Pool& p, std::size_t L) {
        typename Pool::key_type const old = p.leaf_anchor(L);
        typename Pool::key_type const neu = p.leaf_key_at(L, 0);
        if (old == neu) return;
        p.index_erase(old); p.set_leaf_anchor(L, neu); p.index_insert(neu, L);
    }

    /// Leaf-Split bei Overflow (B-Baum split_child-Vorlage, listen-adaptiert; KEINE Median-Hochziehung).
    /// Nutzt die ECHTE Schluesselzahl n (NICHT kWhKpn) — ein Leaf kann transient bis kWhKpn+1 Keys haben.
    template <class Pool>
    static void split_leaf(Pool& p, std::size_t L) {
        constexpr int MID = Pool::kWhMid;
        int const n = p.leaf_n(L);
        std::size_t const R = p.new_leaf();
        for (int j = MID; j < n; ++j) {
            p.set_leaf_key_at(R, j - MID, p.leaf_key_at(L, j));
            p.set_leaf_value_at(R, j - MID, p.leaf_value_at(L, j));
        }
        p.set_leaf_n(R, n - MID);
        p.set_leaf_n(L, MID);
        p.set_leaf_anchor(R, p.leaf_key_at(R, 0));
        std::size_t const NIL = Pool::kNil;
        std::size_t const nx = p.leaf_next(L);
        p.set_leaf_prev(R, L);
        p.set_leaf_next(R, nx);
        if (nx != NIL) p.set_leaf_prev(nx, R);
        p.set_leaf_next(L, R);
        p.index_insert(p.leaf_anchor(R), R);   // W6 (L's Anchor unveraendert -> kein reanchor(L))
    }

    /// Borrow/Merge bei Unterlauf (B-Baum borrow/merge-Vorlage, auf der LISTE). Korrektheit haengt NICHT an
    /// Mindestfuellung -> Unterlauf ohne passenden Nachbarn ist toleriert.
    template <class Pool>
    static void borrow_or_merge(Pool& p, std::size_t L) {
        constexpr int MRG = Pool::kWhMrg;
        constexpr int KPN = Pool::kWhKpn;
        std::size_t const NIL = Pool::kNil;
        std::size_t const Np = p.leaf_prev(L), Nn = p.leaf_next(L);
        // BORROW vom volleren Vorgaenger (dessen letzter Key wandert vorn in L).
        if (Np != NIL && p.leaf_n(Np) > MRG) {
            int const pn = p.leaf_n(Np), ln = p.leaf_n(L);
            for (int j = ln; j > 0; --j) { p.set_leaf_key_at(L, j, p.leaf_key_at(L, j - 1)); p.set_leaf_value_at(L, j, p.leaf_value_at(L, j - 1)); }
            p.set_leaf_key_at(L, 0, p.leaf_key_at(Np, pn - 1)); p.set_leaf_value_at(L, 0, p.leaf_value_at(Np, pn - 1));
            p.set_leaf_n(Np, pn - 1); p.set_leaf_n(L, ln + 1);
            reanchor(p, L);   // L's Minimum sank
            return;
        }
        // BORROW vom volleren Nachfolger (dessen erster Key wandert hinten an L).
        if (Nn != NIL && p.leaf_n(Nn) > MRG) {
            int const nn = p.leaf_n(Nn), ln = p.leaf_n(L);
            p.set_leaf_key_at(L, ln, p.leaf_key_at(Nn, 0)); p.set_leaf_value_at(L, ln, p.leaf_value_at(Nn, 0));
            for (int j = 0; j < nn - 1; ++j) { p.set_leaf_key_at(Nn, j, p.leaf_key_at(Nn, j + 1)); p.set_leaf_value_at(Nn, j, p.leaf_value_at(Nn, j + 1)); }
            p.set_leaf_n(Nn, nn - 1); p.set_leaf_n(L, ln + 1);
            reanchor(p, Nn);   // Nn verlor sein Minimum
            return;
        }
        // MERGE in das LINKE Leaf (Nachbar zu klein); rechtes auflösen + Index/Liste pflegen.
        if (Np != NIL && p.leaf_n(Np) + p.leaf_n(L) <= KPN) {
            int const pn = p.leaf_n(Np), ln = p.leaf_n(L);
            for (int j = 0; j < ln; ++j) { p.set_leaf_key_at(Np, pn + j, p.leaf_key_at(L, j)); p.set_leaf_value_at(Np, pn + j, p.leaf_value_at(L, j)); }
            p.set_leaf_n(Np, pn + ln);
            std::size_t const nx = p.leaf_next(L);
            p.set_leaf_next(Np, nx);
            if (nx != NIL) p.set_leaf_prev(nx, Np);
            p.index_erase(p.leaf_anchor(L));   // Np.anchor unveraendert (Minimum bleibt)
            p.free_node(L);
            return;
        }
        if (Nn != NIL && p.leaf_n(L) + p.leaf_n(Nn) <= KPN) {
            int const ln = p.leaf_n(L), nn = p.leaf_n(Nn);
            for (int j = 0; j < nn; ++j) { p.set_leaf_key_at(L, ln + j, p.leaf_key_at(Nn, j)); p.set_leaf_value_at(L, ln + j, p.leaf_value_at(Nn, j)); }
            p.set_leaf_n(L, ln + nn);
            std::size_t const nx = p.leaf_next(Nn);
            p.set_leaf_next(L, nx);
            if (nx != NIL) p.set_leaf_prev(nx, L);
            p.index_erase(p.leaf_anchor(Nn));   // L.anchor unveraendert
            p.free_node(Nn);
            return;
        }
        // Sonst: Unterlauf toleriert (Korrektheit unberuehrt).
    }

    // ── Pflicht-Trio ──
    template <class Pool>
    static void insert_into(Pool& p, typename Pool::key_type key, typename Pool::value_type value) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) {
            std::size_t const L = p.new_leaf();
            p.set_leaf_key_at(L, 0, key); p.set_leaf_value_at(L, 0, value); p.set_leaf_n(L, 1);
            p.set_leaf_anchor(L, key); p.set_leaf_prev(L, NIL); p.set_leaf_next(L, NIL);
            p.set_root(L); p.index_insert(key, L); p.inc_size();
            return;
        }
        std::size_t const leaf = locate_leaf(p, key);
        int const n = p.leaf_n(leaf);
        int const i = leaf_lower_bound(p, leaf, key);
        if (i < n && p.leaf_key_at(leaf, i) == key) { p.set_leaf_value_at(leaf, i, value); return; }   // Update
        for (int j = n; j > i; --j) { p.set_leaf_key_at(leaf, j, p.leaf_key_at(leaf, j - 1)); p.set_leaf_value_at(leaf, j, p.leaf_value_at(leaf, j - 1)); }
        p.set_leaf_key_at(leaf, i, key); p.set_leaf_value_at(leaf, i, value); p.set_leaf_n(leaf, n + 1); p.inc_size();
        if (i == 0) reanchor(p, leaf);                          // neues Minimum
        if (p.leaf_n(leaf) >= Pool::kWhKpn) split_leaf(p, leaf); // Overflow (>= deckt auch das transiente kWhKpn+1 ab)
    }

    template <class Pool>
    static std::optional<typename Pool::value_type> lookup_in(Pool const& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) return std::nullopt;
        std::size_t const leaf = locate_leaf(p, key);
        if (leaf == NIL) return std::nullopt;
        int const n = p.leaf_n(leaf);
        int const i = leaf_lower_bound(p, leaf, key);
        if (i < n && p.leaf_key_at(leaf, i) == key) return p.leaf_value_at(leaf, i);
        return std::nullopt;
    }

    template <class Pool>
    static bool erase_from(Pool& p, typename Pool::key_type key) {
        std::size_t const NIL = Pool::kNil;
        if (p.root() == NIL) return false;
        std::size_t const leaf = locate_leaf(p, key);
        if (leaf == NIL) return false;
        int const n = p.leaf_n(leaf);
        int const i = leaf_lower_bound(p, leaf, key);
        if (i >= n || p.leaf_key_at(leaf, i) != key) return false;
        for (int j = i; j < n - 1; ++j) { p.set_leaf_key_at(leaf, j, p.leaf_key_at(leaf, j + 1)); p.set_leaf_value_at(leaf, j, p.leaf_value_at(leaf, j + 1)); }
        p.set_leaf_n(leaf, n - 1); p.dec_size();
        if (i == 0 && p.leaf_n(leaf) > 0) reanchor(p, leaf);    // Minimum geloescht
        if (p.leaf_n(leaf) == 0) {                               // leeres Leaf entfernen
            p.index_erase(p.leaf_anchor(leaf));
            std::size_t const pv = p.leaf_prev(leaf), nx = p.leaf_next(leaf);
            if (pv != NIL) p.set_leaf_next(pv, nx); else p.set_root(nx);   // root nachziehen wenn Kopf
            if (nx != NIL) p.set_leaf_prev(nx, pv);
            p.free_node(leaf);
        } else if (p.leaf_n(leaf) < Pool::kWhMrg) {
            borrow_or_merge(p, leaf);
        }
        return true;
    }
};

// Selbstbeweis: WormholeJumpTraversalOrgan erfuellt das WormholeTraversal-Concept ueber dem Pilot-Pool.
static_assert(WormholeTraversal<WormholeJumpTraversalOrgan, WormholeLeafListPoolStore>);

}  // namespace comdare::cache_engine::lookup::composable
