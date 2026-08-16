#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo LinearScanSearchAlgo S15 (2026-05-29)
//
// @topic traversal @achse 03a @family S15 LinearScanSearchAlgo
// @subaxis SA2 sparse_access (UNSORTIERT — Einfuege-Reihenfolge, linearer Scan)
//
// **Algorithmus:** linearer Scan über ein UNSORTIERTES Key-Value-Array. Klassische Strategie für
// KLEINE Knoten: der ART Node4 (Leis ICDE 2013) nutzt linearen Scan, da bei <16 Eintraegen die
// fehlende Branch-Misprediction + die sequentielle Cache-Locality die O(n)-Kosten schlagen (keine
// Sortier-/Hash-/Baum-Overhead). Komplettiert die axis_03a-Palette um das unsortierte Baseline-
// Paradigma: WEDER dense (Array256/65535) noch sortiert (Vector/k-ary/interpolation/eytzinger) noch
// geordnete Struktur (skip-list) noch Hash (hash_search) noch Trie (ART/HOT/…), sondern der einfachste
// Vergleichs-Nullpunkt. supports_range_scan=false (keine Ordnung); insert ist O(1)-amortisiert (kein
// Sortieren), erase O(1) via swap-and-pop (Reihenfolge irrelevant), lookup O(n).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Konzept-Ableitung des ART-Node4-Linear-
// Scan (Leis ICDE 2013) bzw. Lehrbuch-Baseline → C++23-Re-Impl, is_original=false.
//
// Allocation: NUR ueber das Allokator-Achsen-Interface (axis_06) -- s. den ZWEI-EBENEN-SCHNITT unten.
// [[allocation-failure-exception]]: insert kann std::bad_alloc werfen (StdAllocatorAdapter, Posten 64).
//
// ===================================================================================================
// A8-S5 Familie 01c (2026-08-05) -- ZWEI-EBENEN-SCHNITT (PILOT der Fassaden-Konstruktion)
// ===================================================================================================
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4.
// Owner-KERN: LEDGER 04.08.2026 abend-11 ("Option B strikt"), Design-Entscheid abend-14 (D1).
//
// Diese Datei traegt jetzt DREI Typen statt einem, und die Aufteilung ist keine Geschmacksfrage:
//
//   (1) detail::LinearScanSearchAlgoCore<Alloc, Self>  -- DIE SUBSTANZ. Der komplette Algorithmus,
//       parametriert ueber die Allokator-Achsen-Strategie. CRTP mit Self, weil SearchAlgoBase CRTP ist
//       und seine Ctor-Guards auf Derived laufen: mit Self bleibt Derived in JEDER Verwendung der
//       most-derived Typ, die Guard-Kette laeuft also auf der Fassade UND auf dem Rebound-Leaf.
//
//   (2) LinearScanSearchAlgo  -- DIE IDENTITAET. Nicht-Template-Klasse mit EXAKT dem alten Namen.
//       Sie ist das Registry-Organ: sie traegt COMDARE_DEFINE_ORGAN_LOCATION, sie steht in
//       AllStrategies, auf sie sind die Trait-Specializations gekeyt, ihr type_name reist in die
//       Registry-XML und durch den adhoc_emitter.
//
//   (3) LinearScanSearchAlgoRebound<A2>  -- DIE GEBUNDENE FORM. Materialisiert NUR am
//       Genus-Erst-Instanziierungs-Punkt, wenn die Komposition eine ANDERE Strategie fuehrt.
//
// **WARUM DIE FASSADE NICHT-TEMPLATE SEIN MUSS (die Registry-Kante, GELOEST statt umgangen):**
// tools/axis_registry_gen reflektiert `b.type = "::" + strip_all_elaborated(type_name<W>())` und
// `b.wrapper = short_name(b.type)` (main.cpp:161-162) in die committete cache_engine_axis_registry.xml,
// und der F30-Guard (:255-263) verlangt, dass `type=` mit dem COMDARE_DEFINE_ORGAN_LOCATION-Literal
// BEGINNT. Bekaeme dieser Wrapper einen Template-Kopf, waere `LinearScanSearchAlgo` der Alias einer
// Template-Id -- `type=` und `wrapper=` drehten sich, das Makro-Literal driftete, das Byte-diff-Gate
// (test_axis_registry_roundtrip) schluege an, und Auflage 10 ("KEINE Registry-XML-Aenderung") waere
// verletzt. Eine ERBENDE Nicht-Template-Klasse dagegen hat exakt denselben type_name wie eine, die
// alles selbst implementiert -- die XML bewegt sich um NULL Byte, ohne Regeneration.
// (Das ist derselbe Befund wie in 01d, axis_03b_cache_traversal_linear_fanout.hpp:60-71 -- dort war
// die Konsequenz "fest gebunden, kein Template-Kopf". Diese Scheibe holt die Durchbindung nach, OHNE
// die Kante zu bezahlen: der Template-Kopf wandert eine Ebene tiefer, unter den stabilen Namen.)
//
// **WARUM DIE 34 KONSUMENTEN-TUs UNVERAENDERT BLEIBEN:** sie nennen den Typ-NAMEN. Der ist stabil --
// inklusive der Vorwaerts-Deklaration `class LinearScanSearchAlgo;` in
// composable/traversal_for_search_algo.hpp:35, die eine Alias-Loesung sofort gebrochen haette.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <organ_axes/lookup/axis_03a_search_algo_flags.hpp>
#include <organ_axes/lookup/composable/search_algo_rebind.hpp>
#include <measurement/measurable_concept.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

// Vorwaerts-Deklaration: die Fassade nennt ihren eigenen Rebound-Leaf als Member-Alias, und der
// Rebound-Leaf erbt vom selben Core -- beide brauchen den Namen, bevor der andere vollstaendig ist.
template <class A2>
class LinearScanSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S15-Organs: unsortierter linearer Scan, Eintrags-Speicher ueber die Allokator-ACHSE.
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ (Fassade oder Rebound-Leaf) -- PFLICHT, weil SearchAlgoBase CRTP
///                ist und seine Ctor-Guards (axis_03a_search_algo_base.hpp:47-59) auf Derived laufen.
template <class Alloc, class Self>
class LinearScanSearchAlgoCore : public SearchAlgoBase<Self> {
public:
    static constexpr bool enabled = flags::linear_scan_enabled;
    // (E-Welle-A2 / Befund-2 / A2.4-S1) Array-Familie (unsortierter Scan über flachen KV-Slot-Store) → store-traversierbar:
    // die Suche kann über DENSELBEN LayoutAwareChunkedStore laufen (node/layout/allocator wirken real). G3-klassifiziert.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 15>; // S15

    /// A8-S5 SCHNITT-FORM (B): der Eintrags-Speicher haengt an der Allokator-ACHSE. Diese Zeile IST
    /// der Ausweis, den die Familien-Konformitaets-Wache liest (tests/unit/s5_family_alloc_conformance.hpp).
    /// Anders als in 01d ist sie hier NICHT fest, sondern der Kompositions-Parameter -- genau das ist
    /// der Unterschied zwischen "an der Achse" (01d/01a-Zwischenstand) und "Option B strikt".
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der gebundene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- dann liefe "
                  "der Eintrags-Speicher wieder an der Allokator-Achse vorbei (Schnitt-Regel Dossier 3.4).");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; }
    /// SERIALISIERUNGS-SCHLUESSEL -- bewusst OHNE jeden Allokator-Bezug. Die T6-Wahl darf hier NIE
    /// hineinlecken: sonst truege eine mimalloc-gebundene Komposition einen anderen Organ-Namen als
    /// dieselbe Komposition mit exgen, und binary_id-/serialize-Pfad drifteten gegen die Allokator-
    /// Achse. Die Wache dazu steht als static_assert unter der Fassade (name()-Invarianz).
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "linear_scan"; }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "LinearScanSearchAlgo (unsorted linear scan, ART Node4-Strategie — Leis ICDE 2013)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "LINEAR_SCAN"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    ///
    /// NICHT gebumpt in dieser Scheibe -- deklariert, nicht stillschweigend: der Allokations-PFAD aendert
    /// sich, der Algorithmus nicht. Der Bump-Entscheid fuer alle 16 S5-01c-Varianten liegt im Mess-/A13-
    /// Fenster (S5-04-Praezedenz: v1.0.0c blieb trotz perf-relevantem Pfadwechsel).
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return false; }       // skalare Baseline
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return false; } // UNSORTIERT
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return false; }

private:
    using entry_type   = std::pair<key_type, value_type>;
    using entry_alloc  = typename Alloc::template StdAllocatorAdapter<entry_type>;
    using entry_vector = std::vector<entry_type, entry_alloc>;

public:
    /// Der Vektor wird an das EIGENE allocator_ gebunden (der Adapter haelt &allocator_).
    LinearScanSearchAlgoCore() : entries_(allocator_.template as_std_allocator<entry_type>()) {}

    /// KF-6-NAHT (Posten 62, LEDGER 04.08. abend-12): eine vor-parametrierte Strategie-Instanz
    /// uebernehmen, statt sie default zu konstruieren. Heute nirgends benutzt und bewusst `explicit` --
    /// die Naht existiert, damit das KF-6-Fenster (CacheLineCfg-NTTP durch die Cores) sie nicht erst
    /// nachtraeglich in 16 Dateien schneiden muss.
    explicit LinearScanSearchAlgoCore(allocator_type a)
        : allocator_(std::move(a)), entries_(allocator_.template as_std_allocator<entry_type>()) {}

    /// Copy: Strategie mitkopieren, den Vektor an das EIGENE allocator_ binden, dann die transiente
    /// Kopier-Allokation aus der Statistik nehmen (Memento) -- 1:1 btree_node_pool_store.hpp:86.
    /// MOVE ist bewusst NICHT deklariert (der user-definierte Copy unterdrueckt ihn implizit): jede
    /// Bewegung faellt damit auf den REBINDENDEN Copy zurueck, statt einen Vektor mitsamt fremdem
    /// Adapter-Zeiger zu verschieben.
    LinearScanSearchAlgoCore(LinearScanSearchAlgoCore const& o)
        : allocator_(o.allocator_), entries_(o.entries_, allocator_.template as_std_allocator<entry_type>()) {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        stats_ = o.stats_;
        allocator_.restore_statistics(o.allocator_.statistics());
#endif
    }

    LinearScanSearchAlgoCore& operator=(LinearScanSearchAlgoCore const& o) {
        if (this != &o) {
            // POCCA=false (der Adapter fuehrt keine propagate_-Typedefs) -> entries_ behaelt sein an
            // this-allocator_ gebundenes Adapter-Objekt; die Zuweisung re-alloziert ueber this-allocator_.
            entries_ = o.entries_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            stats_ = o.stats_;
            allocator_.restore_statistics(o.allocator_.statistics());
#endif
        }
        return *this;
    }

    ~LinearScanSearchAlgoCore() = default;

    [[nodiscard]] bool operator==(LinearScanSearchAlgoCore const& other) const noexcept {
        return entries_.size() == other.entries_.size();
    }

    /// O(n)-Scan auf Duplikat (Update), sonst O(1)-amortisiertes Anhaengen (kein Sortieren).
    /// SONDERFALL [[allocation-failure-exception]]: push_back kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        for (auto& e : entries_) {
            if (e.first == k) {
                e.second = v;
                notify_insert();
                return;
            } // Update
        }
        entries_.emplace_back(k, v);
        notify_insert();
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        for (auto const& e : entries_) {
            if (e.first == k) {
                result = e.second;
                break;
            }
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

    bool erase(key_type k) {
        for (std::size_t i = 0; i < entries_.size(); ++i) {
            if (entries_[i].first == k) {
                entries_[i] = entries_.back(); // swap-and-pop: O(1), Reihenfolge irrelevant (unsortiert)
                entries_.pop_back();
#ifdef COMDARE_CE_ENABLE_STATISTICS
                ++stats_.total_erase_count;
                observer_.notify(stats_);
#endif
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return entries_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(entries_.size()) / 65536.0;
    }
    void clear() noexcept { entries_.clear(); }

    /// DensityClassifiedStrategy [[density-classified-strategy]] — linear scan lohnt nur bei kleiner Belegung.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = entries_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
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

    /// EINSAMMEL-NAHT der T6-Durchbindung (Owner-KERN abend-11, Pflicht (a)) -- NUR die Naht.
    ///
    /// BEWUSST EIN VIERTER NAME neben store_/traversal_/mapping_/slot_allocator_statistics: jede
    /// Organ-Instanz haelt ihre EIGENE Strategie-Instanz, die Snapshots sind also DISJUNKT zum
    /// konstitutiven Store-Snapshot. Ob und wie sie in die T6-CSV-Spalte summiert werden
    /// (Doppelzaehlungs-Regel), ist der EXPLIZITE Schritt des Mess-Schnitt-Fensters VOR Messbeginn --
    /// nicht dieser Scheibe. Die Namens-Trennung IST die Absicherung dagegen, dass ein kuenftiger
    /// generischer Leser stillschweigend doppelt zaehlt.
    using allocator_snapshot_t = typename allocator_type::snapshot_t;
    [[nodiscard]] allocator_snapshot_t search_allocator_statistics() const noexcept { return allocator_.statistics(); }
#endif

private:
    void notify_insert() noexcept {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (entries_.size() > stats_.peak_occupancy) stats_.peak_occupancy = entries_.size();
        observer_.notify(stats_);
#endif
    }

    // allocator_ MUSS VOR entries_ stehen: der Adapter haelt &allocator_, und die Member-
    // Initialisierungs-/Zerstoerungsreihenfolge ist die Deklarationsreihenfolge (entries_ muss VOR
    // der Strategie sterben). Dieselbe Reihenfolge wie in den Pool-Stores (01a) und in 01d.
    allocator_type allocator_{};
    entry_vector   entries_; // UNSORTIERT (Einfuege-Reihenfolge)
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S15. Nicht-Template, exakt der historische Typ-Name.
/// `final`: die Ebenen-Trennung ist Vertrag, nicht Vorschlag -- ein weiterer Ableitungs-Schritt
/// unter der Fassade wuerde den CRTP-Self-Vertrag des Cores brechen.
class LinearScanSearchAlgo final
    : public detail::LinearScanSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, LinearScanSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    /// Kompositionen, die eine andere Strategie fuehren, bekommen ueber rebind_allocator den Leaf.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::LinearScanSearchAlgo",
                                  "organ_axes/lookup/axis_03a_search_algo_linear_scan.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo): auf WELCHEN Typ bindet die
    /// Kompositions-Naht um, wenn die Komposition eine fremde Strategie fuehrt?
    template <class A2>
    using rebind_allocator = LinearScanSearchAlgoRebound<A2>;

    using detail::LinearScanSearchAlgoCore<default_allocator_type, LinearScanSearchAlgo>::LinearScanSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- materialisiert nur am Genus-Erst-Instanziierungs-Punkt. Erbt name(),
/// family_id und algo_version unveraendert aus dem Core: der serialize-/binary_id-Schluessel ist
/// derselbe, die T6-Identitaet reist separat im Kompositions-Pfad.
/// Traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION -- er ist kein Registry-Organ und darf nie in
/// die Registry-XML reflektiert werden.
template <class A2>
class LinearScanSearchAlgoRebound final : public detail::LinearScanSearchAlgoCore<A2, LinearScanSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS. Konsumenten, die die Identitaets-Ebene erwarten (adhoc_emitter-type_name-
    /// Reise, Registry, Fixtures), pinnen dagegen -- s. composable::IsReboundSearchAlgoLeaf.
    using axis03a_rebound_tag = void;

    using detail::LinearScanSearchAlgoCore<A2, LinearScanSearchAlgoRebound<A2>>::LinearScanSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<LinearScanSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<LinearScanSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<LinearScanSearchAlgo>);

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht.
// ---------------------------------------------------------------------------------------------

/// LEVEL 0 (der golden-Pfad): die Kompositions-Naht mit dem Achsen-Default liefert die FASSADE SELBST
/// zurueck -- kein Rebound-Typ, kein Typ-Shift, kein neuer Symbolname. Das ist der ganze golden-Beweis
/// dieser Scheibe: was typ-identisch ist, kann sich in binary_id/serialize/XML nicht bewegen.
static_assert(std::is_same_v<composable::search_algo_for_composition_t<LinearScanSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             LinearScanSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");

/// Der Migrations-Ausweis ist da (sonst faele das Organ still auf Level 2 = unveraendert zurueck und
/// die Durchbindung waere eine Behauptung).
static_assert(
    composable::AllocatorRebindableSearchAlgo<LinearScanSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: LinearScanSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");

/// EBENEN-TRENNUNG: die Identitaets-Ebene traegt NIE den Rebound-Ausweis, der Leaf traegt ihn IMMER.
static_assert(!composable::IsReboundSearchAlgoLeaf<LinearScanSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<LinearScanSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");

/// name()-ALLOKATOR-INVARIANZ: der serialize-Schluessel darf sich mit der T6-Wahl NICHT bewegen.
/// Ueber die Naht geprueft, nicht nur am Typ -- so faengt die Wache auch einen kuenftigen Rebound-Leaf,
/// der name() ueberschriebe.
static_assert(composable::search_algo_name_is_allocator_invariant_v<LinearScanSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(LinearScanSearchAlgo::name() ==
                  LinearScanSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");

/// Der Rebound-Leaf ist ein VOLLWERTIGES Organ, nicht nur eine Typ-Huelle: dieselben Concepts wie die
/// Fassade (die CRTP-Guard-Kette laeuft auf beiden Leaves -- R2 des Design-Risikoblatts).
static_assert(concepts::SearchAlgoVariant<LinearScanSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              LinearScanSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<LinearScanSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
