#pragma once
// V41.F.6.1.P2.D.tr.s2 OriginalArtSearchAlgo S04 (2026-05-26)
//
// @topic traversal @achse 03a @family S04 OriginalArtSearchAlgo
// @subaxis SA1 dense_access (analog Array256SearchAlgo)
// @paper P01 ART (Leis/Kemper/Neumann, ICDE 2013)
//
// **Habich-Compliance-Wrapper:** parallel zu Array256SearchAlgo (Re-Impl).
// get_compiler()="gcc-9.5" via Paper-Mixin (Tool-validierte Source-Identity ueber SHA256).
// is_original_module()=true (alle 4 Functions: insert/lookup/erase/clear originall).
//
// **Body-Strategie (s2):** Standalone Re-Impl analog Array256SearchAlgo (std::array<optional<u64>,256>).
// Compile-Time-Switch via `enabled = flags::original_art_enabled`. Wenn aktiviert,
// signalisiert die Wrapper-Klasse die Paper-Bindung via Mixin-Properties. Tatsaechliches
// Linking gegen unodb::db erfolgt in s4 (Library-Build mit Original-Compiler via
// cmake/compiler_cache.cmake + cmake/paper_binary.cmake).
//
// ===================================================================================================
// A8-S5 Familie 01c, Scheibe 3 (2026-08-05) -- MINIMAL-FORM des Schnitts (KLASSEN-ENTSCHEID begruendet)
// ===================================================================================================
// KLASSEN-ENTSCHEID: **MINIMAL-FORM, NICHT das Voll-Rezept.** Und zwar nicht aus Bequemlichkeit,
// sondern weil das Voll-Rezept hier eine Verschlechterung waere.
//
// DER BEFUND, DER DEN ENTSCHEID TRAEGT: dieses Organ hat GAR KEINEN dynamischen Speicher. Sein
// Zustand ist ein INLINE std::array<std::optional<value_type>, 256> plus ein Zaehler -- die
// 01b-Schlussbilanz fuehrt es ausdruecklich als ENTLASTET ("Form A heap-frei, nichts zu tun"), es
// steht in KEINER der 39 Rest-Zeilen. Es gibt also keinen Container zu tauschen.
//
// WARUM DANN UEBERHAUPT ETWAS AENDERN: die Kompositions-Naht soll TOTAL sein. Ein Organ ohne
// Migrations-Ausweis faellt aus der abgeleiteten Familien-Population heraus (sie filtert ueber genau
// diesen Ausweis) -- und ein Organ, das aus der Wache herausfaellt, ist der stille Ausfall, gegen den
// die Wache gebaut wurde. Der Ausweis macht die Achse LUECKENLOS pruefbar; ohne ihn koennte der
// Vollstaendigkeits-Pin nie ueber ALLE 22 Organe laufen.
//
// WAS DIE MINIMAL-FORM WEGLAESST -- und warum jedes Weglassen eine Aussage ist:
//   * KEINE allocator_-INSTANZ. Ein Strategie-Objekt im Organ waere hier reine Zeremonie: es haette
//     nichts zu allozieren. Schlimmer noch, es wuerde die STAERKERE Aussage zerstoeren -- das Organ
//     erfuellt Form (A) heap-frei (trivially destructible + trivially copyable, der harte TYP-Beweis
//     der Familien-Wache), und ein Strategie-Member mit Statistik-Zustand koennte genau das kippen.
//     Form A ist die staerkere der beiden zulaessigen Schnitt-Formen; sie hier gegen eine schwaechere
//     Deklarations-Wache einzutauschen waere eine Regression im Gewand einer Migration.
//   * KEINE search_allocator_statistics()-NAHT. Eine Naht, die per Konstruktion immer 0 meldet, waere
//     eine API in der FORM einer Aussage ohne deren Inhalt -- genau die Sorte Zeile, die eine spaetere
//     CSV-Lektuere in die Irre fuehrt. Es gibt hier nichts einzusammeln, also gibt es keine Naht.
//   * KEIN Container-Tausch (es gibt keinen Container).
//
// WAS SIE BEHAELT: die Zwei-Ebenen-KONSTRUKTION. Die ist hier NICHT Zeremonie, sondern das Minimum:
// Fassade und Rebound-Leaf muessen denselben Algorithmus tragen, ohne voneinander abzuleiten (der Leaf
// darf die ORGAN_LOCATION der Fassade NICHT erben, sonst wuerde die Substanz-Ebene reflektierbar).
// Zwei Typen mit gemeinsamer Implementierung und ohne Ableitungs-Beziehung heissen: EIN gemeinsamer
// Core. Die Alternative waere ein zweiter, kopierter Algorithmus -- und der ist immer falsch.
// Der Alloc-Parameter des Cores traegt hier folglich NUR den Typ-Ausweis, keine Speicher-Rolle; ein
// static_assert unter der Fassade haelt genau das fest, damit die Absicht nicht als Versehen gelesen wird.
//
// ORGAN_LOCATION NEU (Default-OFF -> Byte-Effekt NULL, s. XML-ABWESENHEITS-Probe (8b) der Familien-Wache).
#include "axis_03a_search_algo_base.hpp"
#include "axis_03a_search_algo_subaxes_sa1_to_sa3.hpp"
#include "concepts/axis_03a_search_algo_concept.hpp"
#include "concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp"
#include "concepts/axis_03a_search_algo_density_classified_strategy_concept.hpp"
#include "concepts/axis_03a_search_algo_simd_capable_strategy_concept.hpp"
#include <topics/traversal/concepts/topic_traversal_concept.hpp>

#include <organ_axes/lookup/axis_03a_search_algo_flags.hpp>

#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
// V41.F.6.1.P2.D.tr.s2 Paper-Mixin (Tool-generated, SHA256-validiert gegen art.hpp)
#include "concepts/axis_03a_search_algo_original_code_mixin.hpp"
#include <topics/traversal/axis_03a_search_algo/legacy_code/paper_p01_art_is_original.hpp>
#endif

#include <organ_axes/alloc/axis_06_allocator_exgen.hpp>
#include <organ_axes/alloc/concepts/axis_06_allocator_concept.hpp>
#include <organ_axes/lookup/composable/search_algo_rebind.hpp>
#include <measurement/measurable_concept.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include <anatomy/organ_location.hpp> // INC-A #6: per-Organ-Codegen-Lokation (header_include)

namespace comdare::cache_engine::lookup {

// Vorwaerts-Deklaration: die Fassade nennt ihren eigenen Rebound-Leaf als Member-Alias, und der
// Rebound-Leaf erbt vom selben Core -- beide brauchen den Namen, bevor der andere vollstaendig ist.
template <class A2>
class OriginalArtSearchAlgoRebound;

namespace detail {

/// DIE SUBSTANZ des S04-Organs (ART Paper-Bindung, Leis ICDE 2013) -- HEAP-FREI (Form A).
///
/// @tparam Alloc  Die Allokator-Achsen-Strategie (axis_06) -- hier NUR Typ-Ausweis, ohne Speicher-Rolle
///                (MINIMAL-FORM, s. Datei-Kopf). Default-Bindung der Fassade: ExgenAllocator.
/// @tparam Self   Der most-derived Typ -- PFLICHT wegen der CRTP-Guards der SearchAlgoBase.
template <class Alloc, class Self>
class OriginalArtSearchAlgoCore : public SearchAlgoBase<Self>
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    ,
                                  public generated::p01_art::OriginalCodeMixin // Habich-Compliance Mixin
#endif
{
public:
    // Diamond-Disambiguation: Mixin-Pfad wins fuer get_compiler/is_original_*
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
    using generated::p01_art::OriginalCodeMixin::get_compiler;
    using generated::p01_art::OriginalCodeMixin::is_original_clear;
    using generated::p01_art::OriginalCodeMixin::is_original_erase;
    using generated::p01_art::OriginalCodeMixin::is_original_insert;
    using generated::p01_art::OriginalCodeMixin::is_original_lookup;
    using generated::p01_art::OriginalCodeMixin::is_original_module;
#endif

    static constexpr bool enabled = flags::original_art_enabled;

    using key_type   = std::uint8_t;
    using value_type = std::uint64_t;
    using size_type  = std::size_t;
    using topic_tag  = ::comdare::cache_engine::traversal::concepts::TraversalTopicTag;
    using axis_tag   = subaxes::dense_access_tag;
    using family_id  = std::integral_constant<int, 4>; // S04

    /// A8-S5 MINIMAL-FORM: der ACHSEN-AUSWEIS -- und ausdruecklich NICHT die Speicher-Bindung.
    /// Dieses Organ ist heap-frei (Form A, die staerkere Aussage); der Typ steht hier, damit die
    /// Kompositions-Naht TOTAL ist und der Vollstaendigkeits-Pin der Familien-Wache ueber ALLE Organe
    /// der Achse laufen kann. Er traegt keinen Puffer -- es gibt keinen.
    using allocator_type = Alloc;
    static_assert(::comdare::cache_engine::alloc::concepts::AllocatorStrategy<allocator_type>,
                  "A8-S5: der ausgewiesene Allokator erfuellt das axis_06-Achsen-Concept nicht mehr -- der "
                  "Ausweis zeigte dann auf etwas, das gar keine Achsen-Strategie ist.");

    [[nodiscard]] static constexpr bool        is_thread_safe() noexcept { return false; } // unodb::db (NICHT olc_db)
    [[nodiscard]] static constexpr std::size_t max_fanout() noexcept { return 256; }
    [[nodiscard]] static constexpr std::string_view name() noexcept {
        if constexpr (enabled) {
            return "original_art";
        } else {
            return "original_art(disabled)";
        }
    }
    [[nodiscard]] static constexpr std::string_view family_name() noexcept {
        return "OriginalArtSearchAlgo (ART Leis ICDE 2013, unodb::db Paper-Bindung)";
    }
    [[nodiscard]] static constexpr std::string_view flag_suffix() noexcept { return "ORIGINAL_ART"; }

    /// Algorithmus-Version (Organ-Provenienz, inkrementeller Tier-Binary-Cache): Bump bei algorithmischer
    /// Aenderung dieser Variante ODER eines von ihr allein genutzten Helfers. Fliesst in algo_sig/perm.algos
    /// (build_orchestrator .algos-Sidecar) -> nur betroffene Tier-Binaries werden neu gebaut/gemessen; die
    /// binary_id bleibt unberuehrt (Version lebt im Sidecar). Startwert "v1"; Bump-Disziplin ab dem 1. Bump.
    static constexpr std::string_view algo_version = "1.0.0.c";

    [[nodiscard]] static constexpr bool supports_simd() noexcept { return true; }
    [[nodiscard]] static constexpr bool supports_range_scan() noexcept { return true; }
    [[nodiscard]] static constexpr bool is_dense() noexcept { return true; }
    [[nodiscard]] static constexpr bool has_cache_line_alignment() noexcept { return true; }

    OriginalArtSearchAlgoCore() noexcept : count_(0) {}

    [[nodiscard]] bool operator==(OriginalArtSearchAlgoCore const& other) const noexcept {
        return count_ == other.count_;
    }

    void insert(key_type k, value_type v) {
        if constexpr (enabled) {
            // s2 Standalone-Body (Array256SearchAlgo-Pattern). s4 wird ueber extern "C" Adapter
            // unodb::db<key,value>::insert_internal aufrufen.
            if (!data_[k].has_value()) ++count_;
            data_[k] = v;
        } else {
            (void)k;
            (void)v;
        }
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_insert_count;
        if (count_ > stats_.peak_occupancy) stats_.peak_occupancy = count_;
        observer_.notify(stats_);
#endif
    }

    [[nodiscard]] std::optional<value_type> lookup(key_type k) const {
#ifdef COMDARE_CE_ENABLE_STATISTICS
        ++stats_.total_lookup_count;
        if (data_[k].has_value())
            ++stats_.total_hit_count;
        else
            ++stats_.total_miss_count;
        observer_.notify(stats_);
#endif
        if constexpr (enabled) {
            return data_[k];
        } // s4: unodb::db::get(...)
        else {
            return std::nullopt;
        }
    }

    /// SIMD-Fast-Path (Sub-Concept Pflicht). ART Node256 ist O(1) direct addressed.
    [[nodiscard]] std::optional<value_type> simd_lookup(key_type k) const { return data_[k]; }

    bool erase(key_type k) {
        if constexpr (enabled) {
            if (!data_[k].has_value()) return false;
            data_[k].reset();
            --count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
            ++stats_.total_erase_count;
            observer_.notify(stats_);
#endif
            return true;
        } else {
            (void)k;
            return false;
        }
    }

    [[nodiscard]] size_type occupied_count() const noexcept { return count_; }
    [[nodiscard]] double    density_percent() const noexcept { return 100.0 * static_cast<double>(count_) / 256.0; }
    void                    clear() noexcept {
        if constexpr (enabled) {
            for (auto& slot : data_) slot.reset();
            count_ = 0;
        }
    }

    /// DensityClassifiedStrategy: ART Node256 ist per Konstruktion DENSE.
    [[nodiscard]] concepts::DensityClass density_class() const noexcept { return concepts::DensityClass::Dense; }

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
    std::array<std::optional<value_type>, 256> data_{};
    std::size_t                                count_;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    mutable concepts::SearchAlgoStatistics stats_{};
    mutable observer_t                     observer_{};
#endif
};

} // namespace detail

/// DIE IDENTITAET -- das Registry-Organ S04. Nicht-Template, exakt der historische Typ-Name.
class OriginalArtSearchAlgo final
    : public detail::OriginalArtSearchAlgoCore<::comdare::cache_engine::alloc::ExgenAllocator, OriginalArtSearchAlgo> {
public:
    /// Die Default-Bindung der Identitaets-Ebene: der BENANNTE Achsen-Default, nie std::allocator.
    using default_allocator_type = ::comdare::cache_engine::alloc::ExgenAllocator;

    COMDARE_DEFINE_ORGAN_LOCATION("::comdare::cache_engine::lookup::OriginalArtSearchAlgo",
                                  "organ_axes/lookup/axis_03a_search_algo_original_art.hpp");

    /// Der Migrations-Ausweis (composable::AllocatorRebindableSearchAlgo).
    template <class A2>
    using rebind_allocator = OriginalArtSearchAlgoRebound<A2>;

    using detail::OriginalArtSearchAlgoCore<default_allocator_type, OriginalArtSearchAlgo>::OriginalArtSearchAlgoCore;
};

/// DIE GEBUNDENE FORM -- traegt BEWUSST KEIN COMDARE_DEFINE_ORGAN_LOCATION.
template <class A2>
class OriginalArtSearchAlgoRebound final
    : public detail::OriginalArtSearchAlgoCore<A2, OriginalArtSearchAlgoRebound<A2>> {
public:
    /// Der EBENEN-AUSWEIS (s. composable::IsReboundSearchAlgoLeaf).
    using axis03a_rebound_tag = void;

    using detail::OriginalArtSearchAlgoCore<A2, OriginalArtSearchAlgoRebound<A2>>::OriginalArtSearchAlgoCore;
};

} // namespace comdare::cache_engine::lookup

namespace comdare::cache_engine::lookup {
static_assert(concepts::SearchAlgoVariant<OriginalArtSearchAlgo>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<OriginalArtSearchAlgo>);
static_assert(concepts::DensityClassifiedStrategy<OriginalArtSearchAlgo>);
static_assert(concepts::SimdCapableStrategy<OriginalArtSearchAlgo>);
#if defined(COMDARE_A03A_IS_ORIGINAL_CODEGEN)
// Habich-Compliance: alle 4 Functions sind im Paper originall (P01 ART 4/4)
static_assert(OriginalArtSearchAlgo::is_original_module(),
              "OriginalArtSearchAlgo MUSS is_original_module()=true liefern (4/4 ART-API originall)");
#else
static_assert(!OriginalArtSearchAlgo::is_original_module(),
              "OriginalArtSearchAlgo: is_original_module()=false wenn is_original-Codegen-Gate AUS ist");
#endif

// ---------------------------------------------------------------------------------------------
// Der Zwei-Ebenen-Vertrag, self-proving an der Datei, die ihn eingeht (Pilot-Rezept, linear_scan:352).
// ---------------------------------------------------------------------------------------------
static_assert(std::is_same_v<composable::search_algo_for_composition_t<OriginalArtSearchAlgo,
                                                                       ::comdare::cache_engine::alloc::ExgenAllocator>,
                             OriginalArtSearchAlgo>,
              "01c Level-0-IDENTITAET verletzt: die Kompositions-Naht liefert am ACHSEN-DEFAULT nicht mehr die "
              "Fassade selbst. Damit laege ein anderer Typ auf dem golden-Pfad -- Typ-Neutralitaet weg.");
static_assert(
    composable::AllocatorRebindableSearchAlgo<OriginalArtSearchAlgo, ::comdare::cache_engine::alloc::ExgenAllocator>,
    "01c: OriginalArtSearchAlgo traegt keinen rebind_allocator mehr -- nicht migriert.");
static_assert(!composable::IsReboundSearchAlgoLeaf<OriginalArtSearchAlgo>,
              "01c EBENEN-VERMISCHUNG: die Fassade traegt den Rebound-Tag -- Emitter-type_name-Reise und "
              "Registry-Reflektion zeigten dann auf die Substanz-Ebene.");
static_assert(
    composable::IsReboundSearchAlgoLeaf<OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c: der Rebound-Leaf traegt seinen Ausweis nicht -- der Identitaets-Pin kann nicht mehr greifen.");
static_assert(composable::search_algo_name_is_allocator_invariant_v<OriginalArtSearchAlgo,
                                                                    ::comdare::cache_engine::alloc::ExgenAllocator>,
              "01c name()-INVARIANZ (Level 0) verletzt.");
static_assert(OriginalArtSearchAlgo::name() ==
                  OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>::name(),
              "01c name()-INVARIANZ verletzt: der Rebound-Leaf traegt einen anderen Organ-Namen als die Fassade -- "
              "die T6-Wahl leckte in den serialize-/binary_id-Schluessel.");

/// DER MINIMAL-FORM-PIN -- die Zeile, die den Klassen-Entscheid dieser Datei self-proving macht.
/// Beide Ebenen sind und bleiben HEAP-FREI: keine dynamische Allokation, kein Strategie-Member. Wer
/// hier spaeter einen Puffer einbaut, faellt compile-hart auf und muss dann das VOLL-Rezept fahren
/// (allocator_-Instanz + Container ueber die Achse + Einsammel-Naht) -- statt still ein Organ zu
/// hinterlassen, das einen Achsen-Ausweis traegt und trotzdem am Default-Allokator alloziert.
///
/// WARUM DER PIN AM OPT-IN-PUSH-BUILD RUHT (am Objekt gefunden, nicht wegdefiniert): mit
/// COMDARE_CE_ENABLE_OBSERVER_PUSH haelt MeasurableObserver einen std::function-Slot
/// (measurement/measurable_concept.hpp:65-75), und der besitzt Heap. Das ist eine Eigenschaft des
/// MESS-HOOKS, nicht dieses Organs -- die Aussage "das Organ hat keinen eigenen dynamischen Speicher"
/// bleibt dort wahr, sie ist nur nicht mehr mit dem Trivialitaets-TYP-Beweis greifbar. Der Pin greift
/// deshalb im Default-Build (dem Mess- und CI-Build); ihn im Push-Build zu erzwingen hiesse, eine
/// wahre Aussage an einem Werkzeug scheitern zu lassen, das sie gar nicht betrifft.
#ifndef COMDARE_CE_ENABLE_OBSERVER_PUSH
static_assert(std::is_trivially_destructible_v<OriginalArtSearchAlgo> &&
                  std::is_trivially_copyable_v<OriginalArtSearchAlgo>,
              "01c MINIMAL-FORM VERLETZT: die Fassade ist nicht mehr heap-frei -- ab jetzt gilt hier das "
              "VOLL-Rezept, sonst laeuft der neue Puffer am Allokator-Achsen-Interface vorbei.");
static_assert(
    std::is_trivially_destructible_v<OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>> &&
        std::is_trivially_copyable_v<OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>,
    "01c MINIMAL-FORM VERLETZT: der Rebound-Leaf ist nicht mehr heap-frei.");
#endif
static_assert(sizeof(OriginalArtSearchAlgo) ==
                  sizeof(OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>),
              "01c MINIMAL-FORM: die beiden Ebenen haben verschiedene Groesse -- dann traegt eine von ihnen "
              "einen Zustand, den die andere nicht hat, und der Ausweis waere keiner mehr.");

static_assert(
    concepts::SearchAlgoVariant<OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(concepts::CacheEngineSearchAlgoPermutationStrategy<
              OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
static_assert(
    concepts::DensityClassifiedStrategy<OriginalArtSearchAlgoRebound<::comdare::cache_engine::alloc::ExgenAllocator>>);
} // namespace comdare::cache_engine::lookup
