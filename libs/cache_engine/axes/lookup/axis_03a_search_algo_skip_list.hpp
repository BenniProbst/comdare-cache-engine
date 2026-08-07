#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo SkipListSearchAlgo S13 (2026-05-29)
//
// @topic traversal @achse 03a @family S13 SkipListSearchAlgo
// @subaxis SA2 sparse_access (geordnete probabilistische STRUKTUR, nicht Array/Trie)
//
// **Algorithmus:** Skip-Liste — probabilistisch balancierte geordnete Struktur, beschrieben in:
//   William Pugh: "Skip Lists: A Probabilistic Alternative to Balanced Trees."
//   Communications of the ACM 33(6), Juni 1990, S. 668-676. DOI 10.1145/78973.78977.
//
// Jeder Knoten hat 1..kMaxLevel Forward-Zeiger; die Level-Zahl wird per Muenzwurf gezogen
// (P=0.5). Suche laeuft von oben nach unten + vorwaerts (ueberspringt grosse Distanzen auf hohen
// Levels) → erwartet O(log n) fuer search/insert/erase, OHNE explizite Rebalancierung. Distinkt
// von allen bisherigen axis_03a-Wrappern: weder Array (Array256/65535) noch sortierter Vektor
// (VectorU8U8/U16U16) noch Such-METHODE (k-ary/interpolation/eytzinger) noch Radix-Trie (ART/HOT/…),
// sondern eine eigenstaendige verzeigerte ORDERED-MAP-Struktur — erweitert die R7.2-Achse von
// Such-Methoden in die Struktur-Dimension. Trifft das F15-Thema (Datenstruktur des std::map-
// Innenlebens → Performance).
//
// **Implementierung:** index-basiert (std::vector<Node> mit uint32-Forward-INDICES + Tombstone-
// Erase) → vollstaendig WERT-semantisch (kopierbar, kein manuelles new/delete, kein Dangling),
// konsistent mit den Geschwister-Wrappern. Level-RNG: deterministischer std::mt19937_64 (fester
// Seed → reproduzierbar/testbar).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Lehrbuch-Algorithmus (Pugh-Pseudocode),
// kein kanonischer permissiver Single-Repo-Code → originalgetreue C++23-Re-Impl, is_original=false.
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy, DensityClassifiedStrategy.
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert kann std::bad_alloc werfen (StdAllocatorAdapter, Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- ZWEI-EBENEN-SCHNITT (Pilot-Rezept linear_scan)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **VOLL-REZEPT, und zwar der VERSCHACHTELTE Fall** -- dieses Organ ist das einzige
// der Achse mit einem Container IM Element: der Knoten-Pool nodes_ (:239) ist ein Vektor, und JEDER
// Knoten traegt seinen eigenen Turm-Vektor `next` (:210), der beim Einfuegen (:112) und fuer den
// Head-Sentinel (:216) angelegt wird -- 4 der 39 Rest-Zeilen der 01b-Schlussbilanz, und die einzigen
// vier, bei denen der Schnitt ZWEI Ebenen tief gehen muss.
//
// WARUM DIE INNERE EBENE NICHT VERGESSEN WERDEN DARF: bei P=0.5-Muenzwurf haelt die Skip-Liste im
// Mittel ~2 Forward-Slots je Knoten in einem SEPARATEN Heap-Block. Bliebe nur der aeussere Vektor an
// der Achse, waere die Zahl der Allokationen, die die T6-Spalte sieht, um GENAU DEN FAKTOR falsch, der
// die Skip-Liste von einem flachen Vektor unterscheidet -- also um die ganze Aussage.
// Praezedenz: 02b hat dieselbe Zwei-Ebenen-Bindung fuer die verschachtelten LOUDS-Vektoren gefahren
// (surf_louds_bitvector.hpp:83/:94-96 -- Strategie als Konstruktor-Argument bis in die innere Ebene).
//
// DIE KOPIER-FALLE, DIE DAMIT EINHERGEHT (der 02b-Befund, hier wiederholt sich das Muster): ein
// std::vector-Copy uebernimmt via select_on_container_copy_construction den Adapter der QUELLE. Fuer
// den AEUSSEREN Vektor faengt der Zwei-Argument-Copy-Ctor das ab -- fuer die INNEREN Turm-Vektoren
// NICHT, denn die werden vom Element-Copy gebaut. Das Ziel liesse dann auf dem fremden Buch allozieren,
// bis die Quelle stirbt. Der Copy-Konstruktor baut die Knoten deshalb EINZELN nach.
//
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/alloc/axis_06_allocator_exgen.hpp>
#include <axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <axes/lookup/composable/search_algo_rebind.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

// Vorwaerts-Deklaration: die Fassade nennt ihren eigenen Rebound-Leaf als Member-Alias, und der
// Rebound-Leaf erbt vom selben Core -- beide brauchen den Namen, bevor der andere vollstaendig ist.
template <class A2>
class SkipListSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S13-Organs: Knoten-Pool UND die Turm-Vektoren IN den Knoten ueber die
/// Allokator-ACHSE -- beide Ebenen an DERSELBEN Strategie-Instanz.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class SkipListSearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::skip_list_enabled;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 13>; // S13

    static constexpr int           kMaxLevel = 16;
    static constexpr std::uint32_t kNil      = 0xFFFFFFFFu; // "kein Nachfolger"
    static constexpr std::uint32_t kHead     = 0u;          // Sentinel-Kopf-Index

    /// A8-S5 SCHNITT-FORM (B): Knoten-Pool UND die Turm-Vektoren IN den Knoten haengen an DERSELBEN
    /// Strategie-Instanz -- beide Ebenen, sonst zaehlte die T6-Spalte die Skip-Liste wie einen
    /// flachen Vektor.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefen "
                  "Knoten-Pool und/oder Turm-Vektoren wieder an der Allokator-Achse vorbei (Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; } // u16 Keyraum
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug (name()-Invarianz, s. unten).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "skip_list"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "SkipListSearchAlgo (probabilistic ordered structure — Pugh CACM 1990)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "SKIP_LIST"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }      // Pointer-Chasing
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // geordnet (Level-0-Kette)
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept {
        return false;
    } // verzeigert, nicht aligned

private:
    static constexpr std::uint32_t kNilInit = 0xFFFFFFFFu; // == kNil, hier schon vor Node gebraucht

    using level_alloc  = typename Alloc::template StdAllocatorAdapter<std::uint32_t>;
    using level_vector = std::vector<std::uint32_t, level_alloc>; // die INNERE Ebene: der Turm eines Knotens

    struct Node {
        key_type     key{};
        value_type   val{};
        bool         live{};
        level_vector next; // Forward-INDICES je Level (kNil = Ende) -- BINDUNG an die Achse, kein Default
    };

    using node_alloc  = typename Alloc::template StdAllocatorAdapter<Node>;
    using node_vector = std::vector<Node, node_alloc>; // die AEUSSERE Ebene: der Knoten-Pool

public:
    /// NICHT MEHR noexcept: init_head() legt den Head-Turm ueber die Strategie an und kann damit
    /// werfen (Posten 64). Die alte Zusicherung waere ab hier eine Behauptung.
    SkipListSearchAlgoCore() : nodes_(allocator_.template as_std_allocator<Node>()), rng_(0xC0FFEEu) { init_head(); }

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit`.
    explicit SkipListSearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), nodes_(allocator_.template as_std_allocator<Node>()), rng_(0xC0FFEEu) {
        init_head();
    }

    /// Copy: die Knoten werden EINZELN nachgebaut, nicht ueber den Vektor-Copy kopiert.
    /// Der Grund steht im Kopf dieser Datei (die Kopier-Falle der INNEREN Ebene, 02b-Praezedenz
    /// surf_louds_bitvector.hpp:94-96): ein Element-Copy truege den Adapter der QUELLE in den
    /// Turm-Vektor des Ziels, und das Ziel allozierte auf fremdem Buch, bis die Quelle stirbt.
    /// Die transiente Kopier-Allokation wird danach aus der Statistik genommen (Memento).
    SkipListSearchAlgoCore(SkipListSearchAlgoCore const& o)
        : allocator_(o.allocator_), nodes_(allocator_.template as_std_allocator<Node>()), live_count_(o.live_count_),
          level_(o.level_), rng_(o.rng_) {
        nodes_.reserve(o.nodes_.size());
        for (auto const& n : o.nodes_) {
            nodes_.push_back(Node{
                n.key, n.val, n.live,
                level_vector(n.next.begin(), n.next.end(), allocator_.template as_std_allocator<std::uint32_t>())});
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    SkipListSearchAlgoCore& operator=(SkipListSearchAlgoCore const& o) {
        if (this != &o) {
            nodes_.clear();
            nodes_.reserve(o.nodes_.size());
            for (auto const& n : o.nodes_) {
                nodes_.push_back(Node{
                    n.key, n.val, n.live,
                    level_vector(n.next.begin(), n.next.end(), allocator_.template as_std_allocator<std::uint32_t>())});
            }
            live_count_ = o.live_count_;
            level_      = o.level_;
            rng_        = o.rng_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~SkipListSearchAlgoCore() = default;

    [[nodiscard]] bool operator==(SkipListSearchAlgoCore const& other) const noexcept {
        return live_count_ == other.live_count_;
    }

    /// SONDERFALL [[allocation-failure-exception]]: nodes_-Wachstum kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        std::array<std::uint32_t, kMaxLevel> update{};
        std::uint32_t const                  cand = find_update(k, update);
        if (cand != kNil && nodes_[cand].key == k && nodes_[cand].live) {
            nodes_[cand].val = v; // Update vorhandener Key
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.total_insert_count;
            observer_.notify(stats_);
#endif
            return;
        }
        int const lvl = random_level();
        if (lvl > level_) {
            for (int i = level_; i < lvl; ++i) update[static_cast<std::size_t>(i)] = kHead;
            level_ = lvl;
        }
        std::uint32_t const idx = static_cast<std::uint32_t>(nodes_.size());
        nodes_.push_back(Node{
            k, v, true,
            level_vector(static_cast<std::size_t>(lvl), kNil, allocator_.template as_std_allocator<std::uint32_t>())});
        for (int i = 0; i < lvl; ++i) {
            std::uint32_t const pred                       = update[static_cast<std::size_t>(i)];
            nodes_[idx].next[static_cast<std::size_t>(i)]  = nodes_[pred].next[static_cast<std::size_t>(i)];
            nodes_[pred].next[static_cast<std::size_t>(i)] = idx;
        }
        ++live_count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (live_count_ > stats_.peak_occupancy) stats_.peak_occupancy = live_count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::uint32_t             x      = kHead;
        for (int i = level_ - 1; i >= 0; --i) {
            std::uint32_t nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            while (nxt != kNil && nodes_[nxt].key < k) {
                x   = nxt;
                nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            }
        }
        std::uint32_t const cand = nodes_[x].next[0];
        if (cand != kNil && nodes_[cand].key == k && nodes_[cand].live) result = nodes_[cand].val;
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

    bool erase(key_type k) {
        std::array<std::uint32_t, kMaxLevel> update{};
        std::uint32_t const                  cand = find_update(k, update);
        if (cand == kNil || nodes_[cand].key != k || !nodes_[cand].live) return false;
        for (int i = 0; i < level_; ++i) {
            std::uint32_t const pred = update[static_cast<std::size_t>(i)];
            if (nodes_[pred].next[static_cast<std::size_t>(i)] == cand) {
                nodes_[pred].next[static_cast<std::size_t>(i)] = nodes_[cand].next[static_cast<std::size_t>(i)];
            }
        }
        nodes_[cand].live = false; // Tombstone (unverlinkt → unerreichbar)
        while (level_ > 1 && nodes_[kHead].next[static_cast<std::size_t>(level_ - 1)] == kNil) --level_;
        --live_count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return live_count_; }
    [[nodiscard]] double density_percent() const noexcept { return 100.0 * static_cast<double>(live_count_) / 65536.0; }
    void                 clear() noexcept {
        // Allokationsfrei: nur den Head behalten + seine Forward-Slots auf kNil zuruecksetzen
        // (kein push_back → kein bad_alloc im noexcept-Pfad).
        //
        // ERASE STATT RESIZE (A8-S5 01c): `resize(1)` verlangt vom Element-Typ Default-Konstruierbarkeit
        // ueber den Allokator -- der Anhaenge-Zweig wird instanziiert, auch wenn er hier nie laeuft. Seit
        // der Zwei-Ebenen-Bindung traegt Node einen Turm-Vektor mit NICHT default-konstruierbarem
        // Achsen-Adapter, und genau das ist beabsichtigt (ein Turm ohne Strategie-Bindung soll compile-hart
        // scheitern). `erase` schneidet den Schwanz ab, ohne je ein Element bauen zu muessen -- identische
        // Semantik, und der Konstruktions-Zweig wird gar nicht erst verlangt.
        nodes_.erase(nodes_.begin() + 1, nodes_.end());
        for (auto& slot : nodes_[kHead].next) slot = kNil;
        live_count_ = 0;
        level_      = 1;
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        if (live_count_ > 1024) return concepts::DensityClass::Dense;
        if (live_count_ > 64) return concepts::DensityClass::Balanced;
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht,
    /// BEWUSST unter einem VIERTEN Namen (Doppelzaehlungs-Absicherung, Pilot-Begruendung).
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    void init_head() {
        // Head-Sentinel (Index 0): kMaxLevel Forward-Slots, alle kNil -- auch dieser Turm haengt an der Achse.
        nodes_.push_back(Node{key_type{}, value_type{}, false,
                              level_vector(static_cast<std::size_t>(kMaxLevel), kNil,
                                           allocator_.template as_std_allocator<std::uint32_t>())});
    }

    /// Fuellt update[i] = Praedezessor-Index auf Level i und liefert den Level-0-Kandidaten (>= k).
    [[nodiscard]] std::uint32_t find_update(key_type k, std::array<std::uint32_t, kMaxLevel>& update) const {
        std::uint32_t x = kHead;
        for (int i = level_ - 1; i >= 0; --i) {
            std::uint32_t nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            while (nxt != kNil && nodes_[nxt].key < k) {
                x   = nxt;
                nxt = nodes_[x].next[static_cast<std::size_t>(i)];
            }
            update[static_cast<std::size_t>(i)] = x;
        }
        return nodes_[x].next[0];
    }

    [[nodiscard]] int random_level() noexcept {
        int lvl = 1;
        while ((rng_() & 1u) != 0u && lvl < kMaxLevel) ++lvl; // Muenzwurf P=0.5
        return lvl;
    }

    // allocator_ MUSS VOR nodes_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge -- hier gilt das fuer
    // BEIDE Ebenen, denn die Turm-Vektoren sterben mit ihren Knoten und damit vor der Strategie.
    allocator_type          allocator_{};
    node_vector             nodes_;
    std::size_t             live_count_ = 0;
    int                     level_      = 1;
    mutable std::mt19937_64 rng_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif

    /// Der ZWEI-EBENEN-BEWEIS am Typ: der Turm-Vektor IM Knoten fuehrt den Achsen-Adapter, nicht
    /// std::allocator. Ohne diesen Pin koennte ein spaeterer Umbau die innere Ebene stillschweigend
    /// zuruecksetzen -- der aeussere Vektor bliebe konform, und die Wache saehe nichts.
    static_assert(std::is_same_v<decltype(Node::next), level_vector> &&
                      std::is_same_v<typename level_vector::allocator_type, level_alloc>,
                  "01c/Skip-Liste: der Turm-Vektor IM Knoten haengt nicht mehr am Achsen-Adapter -- die innere "
                  "Ebene liefe wieder ueber den Default-Allokator, und die T6-Spalte zaehlte die Skip-Liste "
                  "wie einen flachen Vektor.");
    static_assert(!std::is_default_constructible_v<level_alloc>,
                  "01c/Skip-Liste: der Achsen-Adapter ist default-konstruierbar geworden -- dann koennte ein "
                  "Turm-Vektor still ohne Strategie-Bindung entstehen, statt compile-hart zu scheitern.");
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S13. Nicht-Template, exakt der historische Typ-Name.
class SkipListSearchAlgo final
    : public detail::SkipListSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, SkipListSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::SkipListSearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_skip_list.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = SkipListSearchAlgoRebound<A2>;

    using detail::SkipListSearchAlgoCore<default_allocator_type, SkipListSearchAlgo>::SkipListSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class SkipListSearchAlgoRebound final : public detail::SkipListSearchAlgoCore<A2, SkipListSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::SkipListSearchAlgoCore<A2, SkipListSearchAlgoRebound<A2>>::SkipListSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<SkipListSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<SkipListSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<SkipListSearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<SkipListSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             SkipListSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(
    composable::AllocatorRebindableSearchAlgo<SkipListSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: SkipListSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<SkipListSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<SkipListSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<SkipListSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(SkipListSearchAlgo::name() ==
                  SkipListSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");
static_assert(concepts::SearchAlgoVariant<SkipListSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              SkipListSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<SkipListSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
