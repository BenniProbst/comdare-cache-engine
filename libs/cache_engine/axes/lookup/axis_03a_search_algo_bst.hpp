#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo BinarySearchTreeSearchAlgo S16 (2026-05-29)
//
// @topic traversal @achse 03a @family S16 BinarySearchTreeSearchAlgo
// @subaxis SA2 sparse_access (geordnete deterministische Vergleichs-Baum-STRUKTUR)
//
// **Algorithmus:** unbalancierter binaerer Suchbaum (Knuth TAOCP 3 §6.2.2; Hibbard-Deletion 1962).
// Die kanonische deterministische Vergleichs-Baum-Baseline — Pendant zur probabilistisch
// balancierten SkipListSearchAlgo (S13) und zur Komplexitaets-Begruendung, WARUM std::map (ein
// Rot-Schwarz-Baum) balanciert: der unbalancierte BST degeneriert auf O(n) bei sortierter
// Einfuege-Reihenfolge, waehrend skip-list/sorted das vermeiden. Als Library-Achse macht er diesen
// Unterschied am einheitlichen std::map-Interface MESSBAR (F15).
//
// **Implementierung:** index-basiert (std::vector<Node> mit uint32-Kind-Indizes + Free-List fuer
// geloeschte Slots) → vollstaendig WERT-semantisch (kopierbar, kein new/delete, kein Dangling).
// erase = Hibbard-Deletion (3 Faelle: Blatt / 1 Kind / 2 Kinder via In-Order-Nachfolger).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Lehrbuch-Algorithmus → C++23-Re-Impl,
// is_original=false. Allocation: std::vector — [[allocation-failure-exception]]: insert → bad_alloc.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace comdare::cache_engine::lookup {

class BinarySearchTreeSearchAlgo : public SearchAlgoBase<BinarySearchTreeSearchAlgo> {
public:
    static constexpr bool enabled = flags::bst_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 16>; // S16

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; }
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "bst"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "BinarySearchTreeSearchAlgo (unbalanced BST, Hibbard-Deletion — Knuth TAOCP 3 §6.2.2)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "BST"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1.0.0";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }      // Pointer-Chasing
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // geordnet (In-Order)
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

    BinarySearchTreeSearchAlgo() noexcept = default;

    [[nodiscard]] bool operator==(BinarySearchTreeSearchAlgo const& other) const noexcept {
        return size_ == other.size_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: Knoten-Allokation kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        if (root_ == kNil) {
            root_ = new_node(k, v);
            ++size_;
            notify_insert();
            return;
        }
        std::uint32_t cur = root_;
        for (;;) {
            Node& n = nodes_[cur];
            if (k == n.key) {
                n.val = v;
                notify_insert();
                return;
            } // Update
            if (k < n.key) {
                if (n.left == kNil) {
                    std::uint32_t const c = new_node(k, v);
                    nodes_[cur].left      = c;
                    ++size_;
                    notify_insert();
                    return;
                }
                cur = nodes_[cur].left;
            } else {
                if (n.right == kNil) {
                    std::uint32_t const c = new_node(k, v);
                    nodes_[cur].right     = c;
                    ++size_;
                    notify_insert();
                    return;
                }
                cur = nodes_[cur].right;
            }
        }
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::uint32_t             cur    = root_;
        while (cur != kNil) {
            Node const& n = nodes_[cur];
            if (k == n.key) {
                result = n.val;
                break;
            }
            cur = (k < n.key) ? n.left : n.right;
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

    /// Hibbard-Deletion: Blatt / 1 Kind / 2 Kinder (In-Order-Nachfolger = linkster im rechten Teilbaum).
    bool erase(key_type k) {
        std::uint32_t parent = kNil;
        std::uint32_t cur    = root_;
        while (cur != kNil && nodes_[cur].key != k) {
            parent = cur;
            cur    = (k < nodes_[cur].key) ? nodes_[cur].left : nodes_[cur].right;
        }
        if (cur == kNil) return false; // nicht gefunden

        if (nodes_[cur].left != kNil && nodes_[cur].right != kNil) {
            // 2 Kinder: In-Order-Nachfolger (linkster Knoten im rechten Teilbaum) suchen.
            std::uint32_t succ_parent = cur;
            std::uint32_t succ        = nodes_[cur].right;
            while (nodes_[succ].left != kNil) {
                succ_parent = succ;
                succ        = nodes_[succ].left;
            }
            // Nachfolger-Inhalt in cur kopieren, dann Nachfolger (hat kein linkes Kind) entfernen.
            nodes_[cur].key = nodes_[succ].key;
            nodes_[cur].val = nodes_[succ].val;
            // succ aus succ_parent aushaengen (succ hat hoechstens ein rechtes Kind).
            std::uint32_t const succ_child = nodes_[succ].right;
            if (nodes_[succ_parent].left == succ)
                nodes_[succ_parent].left = succ_child;
            else
                nodes_[succ_parent].right = succ_child;
            free_node(succ);
        } else {
            // 0 oder 1 Kind: cur durch sein (einziges oder kein) Kind ersetzen.
            std::uint32_t const child = (nodes_[cur].left != kNil) ? nodes_[cur].left : nodes_[cur].right;
            if (parent == kNil)
                root_ = child; // cur war Wurzel
            else if (nodes_[parent].left == cur)
                nodes_[parent].left = child;
            else
                nodes_[parent].right = child;
            free_node(cur);
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
    static constexpr std::uint32_t kNil = 0xFFFFFFFFu;
    struct Node {
        key_type      key{};
        value_type    val{};
        std::uint32_t left{kNil};
        std::uint32_t right{kNil};
    };

    [[nodiscard]] std::uint32_t new_node(key_type k, value_type v) {
        if (!free_.empty()) {
            std::uint32_t const idx = free_.back();
            free_.pop_back();
            nodes_[idx] = Node{k, v, kNil, kNil};
            return idx;
        }
        nodes_.push_back(Node{k, v, kNil, kNil});
        return static_cast<std::uint32_t>(nodes_.size() - 1);
    }
    void free_node(std::uint32_t idx) { free_.push_back(idx); } // Slot zur Wiederverwendung

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
static_assert(concepts::SearchAlgoVariant<BinarySearchTreeSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<BinarySearchTreeSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<BinarySearchTreeSearchAlgo>);
} // namespace comdare::cache_engine::lookup
