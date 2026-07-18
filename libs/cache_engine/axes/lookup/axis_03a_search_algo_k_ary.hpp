#pragma once
// V41.F.6.1 R7.2 axis_03a_search_algo KArySearchAlgo S10 (2026-05-29)
//
// @topic traversal @achse 03a @family S10 KArySearchAlgo
// @subaxis SA2 sparse_access (sortierte Keys, Such-METHODE = k-ary statt binary)
//
// **Algorithmus:** k-ary search — Verallgemeinerung der Binärsuche, beschrieben in:
//   Benjamin Schlegel, Rainer Gemulla, Wolfgang Lehner: "k-ary search on modern
//   processors." DaMoN 2009 (5th Int. Workshop on Data Management on New Hardware),
//   Providence RI. DOI 10.1145/1565694.1565705.
//
// Binärsuche macht 1 Vergleich pro Iteration → Halbierung (log_2 n Iterationen).
// k-ary search macht K Vergleiche pro Iteration gegen K gleichverteilte Separatoren →
// Partition in K+1 Segmente (⌈log_(K+1) n⌉ Iterationen). Auf modernen Prozessoren
// amortisiert die parallele Vergleichsausführung (ILP/SIMD) die Mehrkosten pro Iteration,
// sodass die geringere Iterationszahl gewinnt — exakt das F15-Thema (Such-Methode im
// std::map-Innenleben → Performance). Die Arität K ist hier der iterable_aspect: die
// PermutationEngine misst K ∈ {2,4,8,16} (K=2 ≙ Binärsuche als Baseline).
//
// **Provenienz / Lizenz ([[pseudocode-papers-fallback]]):** Das Paper liefert die
// Algorithmen plattformunabhängig (Pseudocode) + Mess-Studie, KEINEN kanonischen
// permissiven Single-Repo-Code. Diese Implementierung ist eine originalgetreue
// C++23-Re-Implementierung der skalaren k-ary-Suche (sequential layout) →
// is_original = false (via AxisBase-Default). simd_lookup() markiert den
// data-level-parallel-Layout-Fast-Path (Paper §4); Pilot delegiert skalar.
//
// Erfuellt: SearchAlgoVariant, CacheEngineSearchAlgoPermutationStrategy,
//           DensityClassifiedStrategy, SimdCapableStrategy, IterableAspectSearchAlgoStrategy.
//
// Allocation: std::vector dynamisch — [[allocation-failure-exception]]: insert kann
// std::bad_alloc werfen.

#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_iterable_aspect_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <axes/lookup/axis_03a_search_algo_flags.hpp>
#include <measurement/measurable_concept.hpp>
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)
namespace comdare::cache_engine::lookup {

class KArySearchAlgo : public SearchAlgoBase<KArySearchAlgo> {
public:
    static constexpr bool enabled = flags::k_ary_enabled;
    // (E-Welle-A2 / Befund-2 / #188-4a-C5, 2026-06-29) k-ary ist Array-Familie (flacher SORTIERTER Store). Das TREUE
    // Traversal-Organ KAryTraversal<Arity> (axes/lookup/composable/k_ary_traversal_organ.hpp) ist compile-time +
    // std::map-konform (test_conformance_gate, k_ary<Arity=2/4/8/16> alle grün). AKTIVIERT (Weg-A): container_ führt
    // k-ary über DENSELBEN node/layout/allocator-LayoutAwareChunkedStore statt vor #188 entferntem SortedBinary-Spiegel
    // (Meta-Lehre #3 erfüllt). K-Variation = COMPILE-TIME-Permutation (User-Entscheid SE-13, KEIN Runtime-Kanal — der
    // wurde verworfen): der per-K-StaticAxisNode-Build (profile_to_tree) emittiert K∈{2,4,8,16} als EIGENE Binaries
    // (je KAryTraversal<K>). Default-Mapping = KAryTraversal<4u> (traversal_for_search_algo); per-K-Build = Folgestufe
    // (harness-gated #162). Greift in die Daten erst beim #215-320-DLL-Neubau.
    static constexpr bool axis_03a_store_traversable = true;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 10>; // S10

    /// iterable_aspect_t = Arität K (Such-METHODE). K-Variation = COMPILE-TIME-Permutation (SE-13): der
    /// per-K-StaticAxisNode-Build (profile_to_tree) emittiert je K aus kIterableArities eine EIGENE Tier-Binary
    /// (je KAryTraversal<K>) — KEIN Runtime-Loop (der wurde verworfen). K=2 = Binärsuch-Baseline; K∈{4,8,16} = echte k-ary-Varianten.
    using iterable_aspect_t = unsigned;
    static constexpr std::array<unsigned, 4>                 kIterableArities{2u, 4u, 8u, 16u};
    [[nodiscard]] static constexpr std::span<unsigned const> iterable_values() noexcept {
        return std::span<unsigned const>{kIterableArities.data(), kIterableArities.size()};
    }

    [[nodiscard]] static constexpr bool             is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t      max_fanout() noexcept { return 65536; } // u16 Keyraum
    [[nodiscard]] static constexpr std::string_view name() noexcept { return "k_ary"; }
    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::KArySearchAlgo",
                                  "axes/lookup/axis_03a_search_algo_k_ary.hpp");
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "KArySearchAlgo (k-ary search — Schlegel/Gemulla/Lehner DaMoN 2009)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "K_ARY"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; } // data-parallel Layout (Paper §4)
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sortiert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    static constexpr unsigned kDefaultArity = 4; // 5-Wege-Partition (Paper-Beispiel)

    KArySearchAlgo() noexcept : arity_(kDefaultArity) {}
    explicit KArySearchAlgo(unsigned arity) noexcept : arity_(arity < 2u ? 2u : arity) {}

    [[nodiscard]] bool operator==(KArySearchAlgo const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(idx), v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// k-ary search (Schlegel DaMoN 2009): pro Iteration werden bis zu K gleichverteilte
    /// Separatoren verglichen → Partition in K+1 Segmente. Bei Restbreite <= K linearer Scan.
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        std::size_t               lo = 0, hi = n; // Halb-offenes Intervall [lo, hi)
        std::size_t const         K = arity_;
        while (hi - lo > K) {
            std::size_t const width  = hi - lo;
            std::size_t       new_lo = lo, new_hi = hi;
            bool              narrowed = false;
            for (std::size_t j = 1; j <= K; ++j) {
                std::size_t const pos = lo + (width * j) / (K + 1); // Separator-Position in [lo, hi)
                key_type const    sep = keys_[pos];
                if (sep == k) { // Direkt-Treffer auf Separator
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_lookup_count;
                    ++stats_.total_hit_count;
                    observer_.notify(stats_);
#endif
                    return values_[pos];
                }
                if (k < sep) {
                    new_hi   = pos;
                    narrowed = true;
                    break;
                } // Ziel im Segment vor pos
                new_lo = pos + 1; // Ziel hinter diesem Separator
            }
            lo = new_lo;
            if (narrowed) hi = new_hi; // sonst: k > alle Separatoren → [letzter_sep+1, hi)
        }
        // Rest-Segment (Breite <= K): linearer Scan
        for (std::size_t i = lo; i < hi; ++i) {
            if (keys_[i] == k) {
                result = values_[i];
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

    /// SIMD-Fast-Path ([[simd-capable-strategy]]): data-level-parallel Layout (Paper §4) vergleicht
    /// die K Separatoren in einem SIMD-Register. Pilot delegiert skalar an lookup().
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation
    /// (k-ary search lohnt erst ab groesseren sortierten Regionen).
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

    /// IterableAspectSearchAlgoStrategy [[iterable-aspect-strategy]]: Setter fuer die Arität K
    /// (konsolidierte Laufzeit-Permutation analog 03a-Schablone).
    void set_iterable_aspect(unsigned new_arity) noexcept { arity_ = (new_arity < 2u ? 2u : new_arity); }

    /// Accessor (Diagnostik): aktuelle Arität K.
    [[nodiscard]] unsigned arity() const noexcept { return arity_; }

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
    unsigned                arity_;
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<KArySearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<KArySearchAlgo>);
static_assert(concepts::SimdCapableStrategy<KArySearchAlgo>);
static_assert(concepts::IterableAspectSearchAlgoStrategy<KArySearchAlgo>);
} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {

// =====================================================================================================
// #188 per-K-Build Increment 1 (2026-07-01) — compile-time-K k-ary Wrapper-Familie (Weg-A).
// =====================================================================================================
// KArySearchAlgoT<K> = die per-K-Permutation der k-ary-Such-Achse als EIGENER, compile-time fixierter Typ
// (StaticAxisNode-Kandidat). Es ERSETZT den verworfenen SE-13-Runtime-Kanal (iterable_aspect_t/arity_/
// set_iterable_aspect): K ist hier ein NON-TYPE-TEMPLATE-PARAMETER, kein Laufzeit-Setter — reale k-ary-Impls
// waehlen K ebenfalls statisch (SIMD-Breite, Paper §4). Jede Instanz ist ein DISTINKTER Typ mit DISTINKTEM
// name() ("k_ary_k2".."k_ary_k16") -> in Increment 2 eine EIGENE Tier-Binary je K (kein binary_id-Kollaps/
// Dedup, Risk#3). Marker axis_03a_store_traversable=true -> container_ (abi_adapter:1890 container_traversal_t)
// fuehrt sie ueber KAryTraversal<K> (traversal_for_search_algo, Header 2), NICHT SortedBinary (Weg-A, Risk#2).
//
// Increment 1 (HIER) = rein ADDITIV, KEINE Registry-Aenderung: die Typen + das per-K-Organ-Mapping (Header 2)
// + self-proving static_asserts (unten). Increment 2 (harness-gated #162) registriert sie in AllStrategies +
// per-K enable-flags (dann 4 static search_algo-Werte -> 4 Binaries via profile_to_tree/adhoc_emitter). Die
// Legacy KArySearchAlgo (Runtime, name "k_ary", S10) bleibt bis zur Increment-2-Registry-Umschaltung UNANGE-
// TASTET (Profile/adhoc_emitter/Fixtures haengen am Literal "k_ary"; ihre Identitaet darf hier nicht brechen).
//
// lookup = bit-identisch zur Legacy KArySearchAlgo::lookup, nur K = kArity (compile-time-Konstante) statt des
// Laufzeit-arity_. Die std::map-Konformitaet je K ist am treuen Organ KAryTraversal<K> bereits festgenagelt
// (test_conformance_gate.cpp run_kary_arity_gate<2/4/8/16>); der Wrapper-lookup traegt denselben Separator-Pfad.
// (Bewusste, TRANSIENTE Duplikation der Legacy-Schleife: die Legacy wird in Increment 2 mit der Registry-
//  Umschaltung entfernt; ohne lokalen Compiler ist ein Refactoring des verifizierten Legacy-Hot-Pfads ein
//  nicht verifizierbares Regressions-Risiko -> additiv + Legacy unberuehrt ist der sauberere, sichere Weg.)
namespace detail {
/// #188 per-K Increment 2: compile-time K -> SEIN eigener Enable-Flag (Default OFF; opt-in wie OriginalXxx). Nicht-
/// kanonisches K ist nie enabled (nur K in {2,4,8,16} werden registriert/gebaut). Saubere Trennung statt Familien-Flag.
template <unsigned K>
[[nodiscard]] constexpr bool k_ary_per_k_enabled() noexcept {
    if constexpr (K == 2u)
        return flags::k_ary_k2_enabled;
    else if constexpr (K == 4u)
        return flags::k_ary_k4_enabled;
    else if constexpr (K == 8u)
        return flags::k_ary_k8_enabled;
    else if constexpr (K == 16u)
        return flags::k_ary_k16_enabled;
    else
        return false;
}
} // namespace detail

template <unsigned K>
class KArySearchAlgoT : public SearchAlgoBase<KArySearchAlgoT<K>> {
    static_assert(K >= 2u, "#188 per-K: Aritaet K muss >= 2 sein (K=2 = Binaersuch-Baseline; K<2 waere linearer Scan)");

public:
    // Je-K EIGENER Enable-Flag (Default OFF, opt-in wie der OriginalXxx-Praezedenzfall) -> die Registrierung in
    // AllStrategies ist nicht-disruptiv (EnabledStrategies waechst NICHT); die Aktivierung erfolgt gezielt fuer den
    // per-K-Mess-Lauf (Increment 2b), ohne golden-320/EnabledStrategies-Tests pauschal zu stoeren.
    static constexpr bool enabled = detail::k_ary_per_k_enabled<K>();
    // Weg-A-Marker (Risk#2: fehlt er, faellt container_traversal_t auf SortedBinaryTraversal = falsches Organ zurueck).
    static constexpr bool axis_03a_store_traversable = true;
    /// Compile-time-Aritaet K (Risk#5: KEIN Runtime-arity_ -> Pfad-A run_workload misst K nicht als K=4-Phantom).
    static constexpr unsigned kArity = K;

    using key_type   = std::uint16_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::sparse_access_tag;
    using family_id  = std::integral_constant<int, 10>; // S10 (dieselbe k-ary-Familie wie Legacy)

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; }
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 65536; } // u16 Keyraum
    /// DISTINKTE name() je K (Risk#3: binary_id/Hash wird aus name() abgeleitet -> gleiche Namen kollidieren ->
    /// Dedup -> nur 1 Binary statt 4). Die 4 kanonischen Aritaeten sind paarweise + vs Legacy-"k_ary" distinkt.
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (K == 2u)
            return "k_ary_k2";
        else if constexpr (K == 4u)
            return "k_ary_k4";
        else if constexpr (K == 8u)
            return "k_ary_k8";
        else if constexpr (K == 16u)
            return "k_ary_k16";
        else
            return "k_ary_kN"; // Fallback (nicht-kanonisches K; nur die 4 Standard-Aritaeten werden registriert)
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "KArySearchAlgoT<K> (k-ary search compile-time per-K — Schlegel/Gemulla/Lehner DaMoN 2009)";
    }
    /// Distinkter flag_suffix je K (korrespondiert 1:1 mit COMDARE_AXIS_03A_ENABLE_K_ARY_K<N> — je eigenes Flag).
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept {
        if constexpr (K == 2u)
            return "K_ARY_K2";
        else if constexpr (K == 4u)
            return "K_ARY_K4";
        else if constexpr (K == 8u)
            return "K_ARY_K8";
        else if constexpr (K == 16u)
            return "K_ARY_K16";
        else
            return "K_ARY_KN";
    }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "v1";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; } // data-parallel Layout (Paper §4)
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; } // sortiert
    [[nodiscard]] static constexpr bool is_dense() noexcept { return false; }           // sparse sortiert
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    KArySearchAlgoT() noexcept = default; // KEIN arity_-Ctor (K ist compile-time)

    [[nodiscard]] bool operator==(KArySearchAlgoT const& other) const noexcept {
        return keys_.size() == other.keys_.size();
    }

    /// SONDERFALL [[allocation-failure-exception]]: insert kann std::bad_alloc werfen.
    void insert(key_type k, value_type v) {
        auto        it  = std::lower_bound(keys_.begin(), keys_.end(), k);
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        if (it != keys_.end() && *it == k) {
            values_[idx] = v; // update
        } else {
            keys_.insert(it, k);
            values_.insert(values_.begin() + static_cast<std::ptrdiff_t>(idx), v);
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (keys_.size() > stats_.peak_occupancy) stats_.peak_occupancy = keys_.size();
        observer_.notify(stats_);
#endif
    }

    /// k-ary search (Schlegel DaMoN 2009), K = kArity compile-time fixiert: pro Iteration bis zu K gleichverteilte
    /// Separatoren -> Partition in K+1 Segmente; Restbreite <= K -> linearer Scan. Bit-identisch zur Legacy
    /// KArySearchAlgo::lookup (nur K statt arity_) -> selbe lower_bound-Semantik (Wert iff Key vorhanden, sonst nullopt).
    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
        std::optional<value_type> result = std::nullopt;
        std::size_t const         n      = keys_.size();
        std::size_t               lo = 0, hi = n; // Halb-offenes Intervall [lo, hi)
        constexpr std::size_t     Kc = kArity;    // compile-time Aritaet
        while (hi - lo > Kc) {
            std::size_t const width  = hi - lo;
            std::size_t       new_lo = lo, new_hi = hi;
            bool              narrowed = false;
            for (std::size_t j = 1; j <= Kc; ++j) {
                std::size_t const pos = lo + (width * j) / (Kc + 1); // Separator-Position in [lo, hi)
                key_type const    sep = keys_[pos];
                if (sep == k) { // Direkt-Treffer auf Separator
#ifdef COMDARE_CE_ENABLE_STATISTICS
                    ++stats_.total_lookup_count;
                    ++stats_.total_hit_count;
                    observer_.notify(stats_);
#endif
                    return values_[pos];
                }
                if (k < sep) {
                    new_hi   = pos;
                    narrowed = true;
                    break;
                } // Ziel im Segment vor pos
                new_lo = pos + 1; // Ziel hinter diesem Separator
            }
            lo = new_lo;
            if (narrowed) hi = new_hi; // sonst: k > alle Separatoren -> [letzter_sep+1, hi)
        }
        for (std::size_t i = lo; i < hi; ++i) { // Rest-Segment (Breite <= K): linearer Scan
            if (keys_[i] == k) {
                result = values_[i];
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

    /// SIMD-Fast-Path ([[simd-capable-strategy]]): data-level-parallel Layout (Paper §4); Pilot delegiert skalar.
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return lookup(k); }

    bool erase(key_type k) {
        auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return false;
        std::size_t idx = static_cast<std::size_t>(it - keys_.begin());
        keys_.erase(it);
        values_.erase(values_.begin() + static_cast<std::ptrdiff_t>(idx));
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_erase_count;
        observer_.notify(stats_);
#endif
        return true;
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return keys_.size(); }
    [[nodiscard]] double    density_percent() const noexcept {
        return 100.0 * static_cast<double>(keys_.size()) / 65536.0;
    }
    void clear() noexcept {
        keys_.clear();
        values_.clear();
    }

    /// DensityClassifiedStrategy [[density-classified-strategy]]: Belegungs-basierte Klassifikation.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept {
        std::size_t const n = keys_.size();
        if (n > 1024) return concepts::DensityClass::Dense;
        if (n > 64) return concepts::DensityClass::Balanced;
        return concepts::DensityClass::Sparse;
    }

    // KEIN set_iterable_aspect / arity() / iterable_values (verworfener SE-13-Runtime-Kanal; K ist compile-time).

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
    std::vector<key_type>   keys_;
    std::vector<value_type> values_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

// Die 4 kanonischen per-K-Wrapper (K in {2,4,8,16}; K=2 = Binaersuch-Baseline, Paper-Mess-Set DaMoN 2009).
using KArySearchAlgoK2  = KArySearchAlgoT<2u>;
using KArySearchAlgoK4  = KArySearchAlgoT<4u>;
using KArySearchAlgoK8  = KArySearchAlgoT<8u>;
using KArySearchAlgoK16 = KArySearchAlgoT<16u>;

// ── Self-proving static_asserts (Increment-1-Verifikation, Codebase-Idiom — vgl. Registry/Organ-Selbstbeweis).
// (a) Jeder per-K-Wrapper erfuellt die Pflicht-Concepts (in Increment 2 registrierbar; wie Legacy MINUS dem
//     verworfenen IterableAspect-Runtime-Kanal). Boundary-Aritaeten K=2 + K=16 stellvertretend geprueft.
static_assert(concepts::SearchAlgoVariant<KArySearchAlgoK2> && concepts::SearchAlgoVariant<KArySearchAlgoK16>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgoK2> &&
              concepts::CacheEngineSearchAlgoPermutationStrategy<KArySearchAlgoK16>);
static_assert(concepts::DensityClassifiedStrategy<KArySearchAlgoK4> &&
              concepts::DensityClassifiedStrategy<KArySearchAlgoK8>);
static_assert(concepts::SimdCapableStrategy<KArySearchAlgoK4> && concepts::SimdCapableStrategy<KArySearchAlgoK16>);
// (b) Risk#5 — KEIN Runtime-Kanal: die per-K-Wrapper erfuellen IterableAspect NICHT (kein iterable_aspect_t/
//     set_iterable_aspect). K ist compile-time. (Legacy KArySearchAlgo erfuellt es noch — Runtime, retiring in Inc 2.)
static_assert(!concepts::IterableAspectSearchAlgoStrategy<KArySearchAlgoK4>,
              "#188 per-K: K ist COMPILE-TIME (kein Runtime-iterable_aspect) — sonst Phantom-Messung (Risk#5)");
// (c) Risk#2 — Weg-A-Marker vorhanden (sonst SortedBinary-Fallback in container_traversal_t).
static_assert(KArySearchAlgoK2::axis_03a_store_traversable && KArySearchAlgoK4::axis_03a_store_traversable &&
                  KArySearchAlgoK8::axis_03a_store_traversable && KArySearchAlgoK16::axis_03a_store_traversable,
              "#188 per-K: store_traversable-Marker je Wrapper (Weg-A; Risk#2)");
// (d) Risk#3 — DISTINKTE name() je K + verschieden vom Legacy-"k_ary" (binary_id-Trennung).
static_assert(KArySearchAlgoK2::name() == "k_ary_k2" && KArySearchAlgoK4::name() == "k_ary_k4" &&
              KArySearchAlgoK8::name() == "k_ary_k8" && KArySearchAlgoK16::name() == "k_ary_k16");
static_assert(KArySearchAlgoK2::name() != KArySearchAlgo::name() &&
                  KArySearchAlgoK4::name() != KArySearchAlgo::name() &&
                  KArySearchAlgoK8::name() != KArySearchAlgo::name() &&
                  KArySearchAlgoK16::name() != KArySearchAlgo::name(),
              "#188 per-K: name() distinkt vom Legacy-\"k_ary\" (binary_id-Trennung, Risk#3)");
// (e) compile-time-Aritaet korrekt propagiert (Grundlage der per-K-Organ-Wahl in Header 2).
static_assert(KArySearchAlgoK2::kArity == 2u && KArySearchAlgoK4::kArity == 4u && KArySearchAlgoK8::kArity == 8u &&
              KArySearchAlgoK16::kArity == 16u);

} // namespace comdare::cache_engine::lookup
