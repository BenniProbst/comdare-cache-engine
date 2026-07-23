#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo BTreeSearchAlgo S17 (2026-05-29)
//
// @topic traversal @achse 03a @family S17 BTreeSearchAlgo
// @subaxis SA2 sparse_access (block-orientierte, balancierte Mehrwege-Vergleichs-STRUKTUR)
//
// **Algorithmus:** B-Baum (Bayer/McCreight, "Organization and Maintenance of Large Ordered
// Indexes", Acta Informatica 1(3):173-189, 1972; CLRS 3rd ed. Kap. 18). Der kanonische
// block-orientierte, IMMER balancierte Mehrwege-Suchbaum — das eigentliche Arbeitspferd jedes
// Datenbank-Index. Er schliesst die letzte echte Luecke der Such-Paradigmen-Palette:
//   * BST (S16) ist binaer + UNbalanciert  → O(n) worst-case bei sortiertem Input,
//   * SkipList (S13) ist binaer-aehnlich + PROBABILISTISCH balanciert,
//   * std::map ist binaer (Rot-Schwarz) + DETERMINISTISCH balanciert,
//   * der B-Baum ist MEHRWEGE (Knoten-Fanout 2t) + deterministisch balanciert + cache-block-
//     orientiert (Node alignas(64)) → flachere Baeume, weniger Cache-Misses pro Suche.
// Als Library-Achse macht er den Effekt der Block-Orientierung am einheitlichen std::map-Interface
// MESSBAR (F15): identische Semantik, andere Cache-Charakteristik.
//
// **Implementierung:** index-basiert (std::vector<Node> mit uint32-Kind-Indizes + Free-List) →
// vollstaendig WERT-semantisch (kopierbar, kein new/delete). Minimum-Degree t=4 ⇒ je Knoten
// [t-1 .. 2t-1] = [3 .. 7] Schluessel und bis zu 2t=8 Kinder. Einfuegen: CLRS B-TREE-INSERT
// (volle Kinder beim Abstieg top-down splitten ⇒ Ziel-Knoten nie voll). Loeschen: CLRS
// B-TREE-DELETE (borrow-from-sibling / merge, Invariante: nur in Kinder mit >= t Schluesseln
// absteigen). Knoten alignas(64) ⇒ has_cache_line_alignment()==true (Design-Merkmal des B-Baums).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Lehrbuch-Algorithmus (Paper liefert
// Beschreibung, kein kanonischer permissiver Repo-Code) → C++23-Re-Impl, is_original=false.
// Allocation: std::vector — [[allocation-failure-exception]]: insert → std::bad_alloc.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class BTreeSearchAlgo : public SearchAlgoBase<BTreeSearchAlgo> {
public:
    static constexpr bool enabled = flags::btree_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 17>; // S17

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "btree"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "BTreeSearchAlgo (balanced multi-way B-tree, t=4 — Bayer/McCreight Acta Inf. 1972 / CLRS Kap. 18)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BTREE"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }      // Pointer-Chasing
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // geordnet (In-Order)
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept {
        return true;
    } // Node alignas(64), block-orientiert

    BTreeSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(BTreeSearchAlgo const& other) const noexcept { return size_ == other.size_; }

    /// SONDERFALL [[allocation-failure-exception]]: Knoten-Allokation kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if (root_ == kNil) {
            root_    = new_node(/*leaf=*/true);
            Node& r  = nodes_[root_];
            r.key[0] = k;
            r.val[0] = v;
            r.n      = 1;
            ++size_;
            notify_insert();
            return;
        }
        // CLRS: existiert k bereits, in-place updaten (std::map-Semantik), NICHT einfuegen.
        if (update_existing(k, v)) {
            notify_insert();
            return;
        }
        // Volle Wurzel zuerst splitten (Hoehe waechst nur hier).
        if (nodes_[root_].n == kMaxKeys) {
            std::uint32_t const s = new_node(/*leaf=*/false);
            nodes_[s].child[0]    = root_;
            root_                 = s;
            split_child(s, 0);
        }
        insert_nonfull(root_, k, v);
        ++size_;
        notify_insert();
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::uint32_t             cur    = root_;
        while (cur != kNil) {
            Node const& n = nodes_[cur];
            int         i = 0;
            while (i < n.n && k > n.key[i]) ++i;
            if (i < n.n && n.key[i] == k) {
                result = n.val[i];
                break;
            }
            cur = n.leaf ? kNil : n.child[i];
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (result)
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        return result;
    }

    /// CLRS B-TREE-DELETE. lookup-first garantiert korrekte bool-Rueckgabe (std::map::erase-Semantik).
    bool erase(key_type k) {
        if (root_ == kNil || !contains(k)) return false;
        remove_from(root_, k);
        // Wurzel auf 0 Schluessel geschrumpft: Hoehe verkleinern.
        if (nodes_[root_].n == 0) {
            std::uint32_t const old = root_;
            root_                   = nodes_[old].leaf ? kNil : nodes_[old].child[0];
            free_node(old);
        }
        --size_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return size_; }
    [[nodiscard]] double    density_percent() const noexcept { return 100.0 * static_cast<double>(size_) / 65536.0; }
    void                    clear() noexcept {
        nodes_.clear();
        free_.clear();
        root_ = kNil;
        size_ = 0;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]].
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        if (size_ > 1024) return concepts::DensityClass::Dense;
        if (size_ > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

#ifdef COMDARE_CE_ENABLE_STATISTICS
    using snapshot_t = concepts::SearchAlgoStatistics;
    using observer_t = ::comdare::cache_engine::measurement::MeasurableObserver<snapshot_t>;
    [[nodiscard]] snapshot_t statistics() const noexcept { return stats_; }
    [[nodiscard]] snapshot_t snapshot() const noexcept { return stats_; }
    void                     reset() noexcept {
        stats_ = {};
        observer_.notify(stats_);
    }
    // CoW-Memento (#142/Audit-K3): Stat-POD-Restore -> organ_cow_capable_v aktiv (spiegelt Observable-Huelle).
    void restore_statistics(snapshot_t const& s) noexcept {
        stats_ = s;
        observer_.notify(stats_);
    }
    [[nodiscard]] observer_t const& observer() const noexcept { return observer_; }
    [[nodiscard]] observer_t&       observer() noexcept { return observer_; }
#endif

private:
    static constexpr int           kT           = 4;          // Minimum-Degree
    static constexpr int           kMaxKeys     = 2 * kT - 1; // 7
    static constexpr int           kMaxChildren = 2 * kT;     // 8
    static constexpr std::uint32_t kNil         = 0xFFFFFFFFu;

    // alignas(64): block-orientierter Knoten ⇒ has_cache_line_alignment()==true (B-Baum-Merkmal).
    struct alignas(64) Node {
        std::int16_t                            n    = 0; // belegte Schluessel
        bool                                    leaf = true;
        std::array<key_type, kMaxKeys>          key{};
        std::array<value_type, kMaxKeys>        val{};
        std::array<std::uint32_t, kMaxChildren> child{}; // in new_node auf kNil gesetzt
    };

    [[nodiscard]] std::uint32_t new_node(bool leaf) {
        std::uint32_t idx;
        if (!free_.empty()) {
            idx = free_.back();
            free_.pop_back();
            nodes_[idx] = Node{};
        } else {
            nodes_.push_back(Node{});
            idx = static_cast<std::uint32_t>(nodes_.size() - 1);
        }
        Node& nn = nodes_[idx];
        nn.leaf  = leaf;
        nn.child.fill(kNil);
        return idx;
    }
    void free_node(std::uint32_t idx) { free_.push_back(idx); }

    [[nodiscard]] bool contains(key_type k) const noexcept {
        std::uint32_t cur = root_;
        while (cur != kNil) {
            Node const& n = nodes_[cur];
            int         i = 0;
            while (i < n.n && k > n.key[i]) ++i;
            if (i < n.n && n.key[i] == k) return true;
            cur = n.leaf ? kNil : n.child[i];
        }
        return false;
    }

    /// Sucht k; falls vorhanden, Wert in-place ersetzen (std::map-Update-Semantik).
    [[nodiscard]] bool update_existing(key_type k, value_type v) {
        std::uint32_t cur = root_;
        while (cur != kNil) {
            Node& n = nodes_[cur];
            int   i = 0;
            while (i < n.n && k > n.key[i]) ++i;
            if (i < n.n && n.key[i] == k) {
                n.val[i] = v;
                return true;
            }
            cur = n.leaf ? kNil : n.child[i];
        }
        return false;
    }

    /// CLRS SPLIT-CHILD: volles Kind C[i] von x_idx in zwei Knoten teilen, Median nach x hochziehen.
    void split_child(std::uint32_t x_idx, int i) {
        std::uint32_t const y_idx  = nodes_[x_idx].child[i];
        bool const          y_leaf = nodes_[y_idx].leaf;
        std::uint32_t const z_idx  = new_node(y_leaf); // kann reallozieren ⇒ danach neu fetchen
        Node&               z      = nodes_[z_idx];
        Node&               y      = nodes_[y_idx];
        z.n                        = kT - 1;
        for (int j = 0; j < kT - 1; ++j) {
            z.key[j] = y.key[j + kT];
            z.val[j] = y.val[j + kT];
        }
        if (!y_leaf)
            for (int j = 0; j < kT; ++j) z.child[j] = y.child[j + kT];
        key_type const   med_k = y.key[kT - 1];
        value_type const med_v = y.val[kT - 1];
        y.n                    = kT - 1;
        Node& x                = nodes_[x_idx];
        for (int j = x.n; j >= i + 1; --j) x.child[j + 1] = x.child[j];
        x.child[i + 1] = z_idx;
        for (int j = x.n - 1; j >= i; --j) {
            x.key[j + 1] = x.key[j];
            x.val[j + 1] = x.val[j];
        }
        x.key[i] = med_k;
        x.val[i] = med_v;
        x.n += 1;
    }

    /// CLRS INSERT-NONFULL: x_idx ist garantiert nicht voll. k existiert hier garantiert noch nicht.
    void insert_nonfull(std::uint32_t x_idx, key_type k, value_type v) {
        for (;;) {
            Node& x = nodes_[x_idx];
            int   i = x.n - 1;
            if (x.leaf) {
                while (i >= 0 && k < x.key[i]) {
                    x.key[i + 1] = x.key[i];
                    x.val[i + 1] = x.val[i];
                    --i;
                }
                x.key[i + 1] = k;
                x.val[i + 1] = v;
                x.n += 1;
                return;
            }
            while (i >= 0 && k < x.key[i]) --i;
            ++i; // Kind-Index zum Absteigen
            if (nodes_[x.child[i]].n == kMaxKeys) {
                split_child(x_idx, i);
                // Nach Split steht Median in nodes_[x_idx].key[i]; Richtung neu bestimmen.
                if (k > nodes_[x_idx].key[i]) ++i;
            }
            x_idx = nodes_[x_idx].child[i]; // iterativ absteigen
        }
    }

    // ---- Delete-Hilfen (CLRS Kap. 18.3, index-basiert; keine Allokation ⇒ Referenzen stabil) ----

    [[nodiscard]] static int find_key(Node const& x, key_type k) noexcept {
        int idx = 0;
        while (idx < x.n && x.key[idx] < k) ++idx;
        return idx;
    }

    void remove_from(std::uint32_t x_idx, key_type k) {
        int const idx = find_key(nodes_[x_idx], k);
        Node&     x   = nodes_[x_idx];
        if (idx < x.n && x.key[idx] == k) {
            if (x.leaf) {
                for (int i = idx + 1; i < x.n; ++i) {
                    x.key[i - 1] = x.key[i];
                    x.val[i - 1] = x.val[i];
                }
                x.n -= 1;
            } else {
                remove_from_nonleaf(x_idx, idx);
            }
        } else {
            if (x.leaf) return; // nicht vorhanden (durch contains() ausgeschlossen)
            bool const last = (idx == x.n);
            if (nodes_[x.child[idx]].n < kT) fill(x_idx, idx);
            Node& xr = nodes_[x_idx];
            if (last && idx > xr.n)
                remove_from(xr.child[idx - 1], k);
            else
                remove_from(xr.child[idx], k);
        }
    }

    void remove_from_nonleaf(std::uint32_t x_idx, int idx) {
        Node&          x = nodes_[x_idx];
        key_type const k = x.key[idx];
        if (nodes_[x.child[idx]].n >= kT) {
            // Vorgaenger (groesster Schluessel im linken Teilbaum) ersetzt k, dann rekursiv loeschen.
            std::uint32_t cur = x.child[idx];
            while (!nodes_[cur].leaf) cur = nodes_[cur].child[nodes_[cur].n];
            key_type const   pk = nodes_[cur].key[nodes_[cur].n - 1];
            value_type const pv = nodes_[cur].val[nodes_[cur].n - 1];
            x.key[idx]          = pk;
            x.val[idx]          = pv;
            remove_from(x.child[idx], pk);
        } else if (nodes_[x.child[idx + 1]].n >= kT) {
            // Nachfolger (kleinster Schluessel im rechten Teilbaum).
            std::uint32_t cur = x.child[idx + 1];
            while (!nodes_[cur].leaf) cur = nodes_[cur].child[0];
            key_type const   sk = nodes_[cur].key[0];
            value_type const sv = nodes_[cur].val[0];
            x.key[idx]          = sk;
            x.val[idx]          = sv;
            remove_from(x.child[idx + 1], sk);
        } else {
            // Beide Kinder haben t-1 Schluessel: mergen, dann k aus gemergtem Kind loeschen.
            merge(x_idx, idx);
            remove_from(nodes_[x_idx].child[idx], k);
        }
    }

    /// Stellt sicher, dass C[idx] vor dem Abstieg >= t Schluessel hat (borrow oder merge).
    void fill(std::uint32_t x_idx, int idx) {
        Node& x = nodes_[x_idx];
        if (idx != 0 && nodes_[x.child[idx - 1]].n >= kT)
            borrow_from_prev(x_idx, idx);
        else if (idx != x.n && nodes_[x.child[idx + 1]].n >= kT)
            borrow_from_next(x_idx, idx);
        else {
            if (idx != x.n)
                merge(x_idx, idx);
            else
                merge(x_idx, idx - 1);
        }
    }

    void borrow_from_prev(std::uint32_t x_idx, int idx) {
        Node& x     = nodes_[x_idx];
        Node& child = nodes_[x.child[idx]];
        Node& sib   = nodes_[x.child[idx - 1]];
        for (int i = child.n - 1; i >= 0; --i) {
            child.key[i + 1] = child.key[i];
            child.val[i + 1] = child.val[i];
        }
        if (!child.leaf)
            for (int i = child.n; i >= 0; --i) child.child[i + 1] = child.child[i];
        child.key[0] = x.key[idx - 1];
        child.val[0] = x.val[idx - 1];
        if (!child.leaf) child.child[0] = sib.child[sib.n];
        x.key[idx - 1] = sib.key[sib.n - 1];
        x.val[idx - 1] = sib.val[sib.n - 1];
        child.n += 1;
        sib.n -= 1;
    }

    void borrow_from_next(std::uint32_t x_idx, int idx) {
        Node& x            = nodes_[x_idx];
        Node& child        = nodes_[x.child[idx]];
        Node& sib          = nodes_[x.child[idx + 1]];
        child.key[child.n] = x.key[idx];
        child.val[child.n] = x.val[idx];
        if (!child.leaf) child.child[child.n + 1] = sib.child[0];
        x.key[idx] = sib.key[0];
        x.val[idx] = sib.val[0];
        for (int i = 1; i < sib.n; ++i) {
            sib.key[i - 1] = sib.key[i];
            sib.val[i - 1] = sib.val[i];
        }
        if (!sib.leaf)
            for (int i = 1; i <= sib.n; ++i) sib.child[i - 1] = sib.child[i];
        child.n += 1;
        sib.n -= 1;
    }

    /// Merge C[idx+1] in C[idx], x.key[idx] wandert mit hinunter. C[idx+1] wird freigegeben.
    void merge(std::uint32_t x_idx, int idx) {
        Node&               x       = nodes_[x_idx];
        std::uint32_t const sib_idx = x.child[idx + 1];
        Node&               child   = nodes_[x.child[idx]];
        Node&               sib     = nodes_[sib_idx];
        child.key[kT - 1]           = x.key[idx];
        child.val[kT - 1]           = x.val[idx];
        for (int i = 0; i < sib.n; ++i) {
            child.key[i + kT] = sib.key[i];
            child.val[i + kT] = sib.val[i];
        }
        if (!child.leaf)
            for (int i = 0; i <= sib.n; ++i) child.child[i + kT] = sib.child[i];
        for (int i = idx + 1; i < x.n; ++i) {
            x.key[i - 1] = x.key[i];
            x.val[i - 1] = x.val[i];
        }
        for (int i = idx + 2; i <= x.n; ++i) x.child[i - 1] = x.child[i];
        child.n += sib.n + 1;
        x.n -= 1;
        free_node(sib_idx);
    }

    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (size_ > stats_.peak_occupancy) stats_.peak_occupancy = size_;
        observer_.notify(stats_);
#endif
    }

    std::vector<Node>          nodes_;
    std::vector<std::uint32_t> free_;
    std::uint32_t              root_ = kNil;
    std::size_t                size_ = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<BTreeSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<BTreeSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<BTreeSearchAlgo>);
} // namespace comdare::cache_engine::lookup
