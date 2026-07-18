#pragma once
// Gattungs-Generik (2026-06-02, User-Option-B Schritt 2) — GenusBindingTraits<G>: die gattungs-PARAMETRISCHE
// Bau-Brücke. Der Baum-KERN ist bereits gattungs-agnostisch (AxisLevel/StaticAxisNode tragen beliebige Achsen);
// was eine Permutation in eine baubare Tier-Binary verwandelt, war bisher auf SearchAlgorithm (AdHocComposition
// <19>) hartcodiert. Dieses Traits hebt die Bindung auf eine pro-Gattung-Spezialisierung: jede Gattung deklariert
// ihre Slot-Zahl, Achsen-Namen, Composition-/Anatomie-Bindung. SearchAlgorithm ist der VERIFIZIERTE Spezialfall
// (BR-2/BR-3/BR-4); die Container-Gattung dockt über ihre Tier-Unterklassen (Adapter §28 13 Achsen / Set / Sequence /
// View) an, ebenso die Graph-Gattung, sobald ihre Komposition/Anatomie existiert
// sobald ihre Komposition/Anatomie existiert (Cross-Genus type-unmöglich, Doku 14 §32 — daher GETRENNTE Traits).
// C++23, header-only.

#include "axis_path_serialization.hpp"          // kCompositionAxisNames (19 SearchAlgorithm-Achsen, Doc 30 §8.0)
#include "anatomy/anatomy_base.hpp"             // AnatomyGenus
#include "anatomy/composition_factory.hpp"      // CompositionFromPermTuple / AdHocComposition<19>
#include "anatomy/search_algorithm_anatomy.hpp" // SearchAlgorithmAnatomy
#include "anatomy/adapter_anatomy.hpp"          // AdapterAnatomy / AdapterComposition (Container-Gattung)
#include "anatomy/set_anatomy.hpp"              // SetAnatomy / SetComposition (Set-Gattung, D9)
#include "anatomy/sequence_anatomy.hpp"         // SequenceAnatomy / SequenceComposition (Sequence-Gattung, D10)
#include "anatomy/view_anatomy.hpp"             // ViewAnatomy / ViewComposition (View-Gattung, D11)

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

namespace cea = ::comdare::cache_engine::anatomy;

/// Primär UNDEFINIERT — jede UNTERSTÜTZTE Gattung liefert eine vollständige Spezialisierung. Eine Gattung ohne
/// Spezialisierung ist (noch) nicht baubar — der Baum trägt ihre Achsen, aber es gibt keine Komposition/Anatomie.
template <cea::AnatomyGenus G>
struct GenusBindingTraits;

/// SearchAlgorithm — der VERIFIZIERTE Spezialfall (alle 4 Brücken literal grün). 19-Slot-Komposition
/// (17 Such-Achsen + queuing q1/q2 als reguläre SA-Achse, Doc 30 §8.0).
template <>
struct GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm> {
    static constexpr cea::AnatomyGenus genus      = cea::AnatomyGenus::SearchAlgorithm;
    static constexpr std::size_t       slot_count = 17; // INC-2d: isa raus (war 18 nach INC-2c-telemetry, 19 davor)
    static constexpr std::string_view  name       = "SearchAlgorithm";

    /// Blatt-PermTuple<17> → reale Komposition (AdHocComposition<17>) → Gattungs-Anatomie.
    template <class PermT>
    using CompositionFor = cea::CompositionFromPermTuple<PermT>;
    template <class Comp>
    using AnatomyFor = cea::SearchAlgorithmAnatomy<Comp>;

    /// Die Achsen-Namen der Komposition-Slots (Reihenfolge T0..T16) — zentrale Pfad-Konvention (BR-2).
    [[nodiscard]] static constexpr std::array<std::string_view, 17> const& axis_names() noexcept {
        return kCompositionAxisNames;
    }
};

/// Adapter — Tier-Unterklasse der CONTAINER-Gattung (std::stack/queue/priority_queue), #87+#90 / Doku 14 §28.
/// 13 Achsen = 12 geteilt/delegiert (§28 Invertebrate) + inner_container (NEU axis_inner, spezifisch). KEINE
/// „ordering"-Achse (§28 kennt keine; frühere inner+ordering-Version war geraten + verworfen). Gebaut analog
/// SequenceComposition. name = Tier-Unterklasse "Adapter" (konsistent mit Set/Sequence/View); Gattung = Container
/// (cea::gattung_of(Adapter)). Cross-Genus type-unmöglich → getrennte Komposition/Anatomie/Observer.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::Adapter> {
    static constexpr cea::AnatomyGenus   genus      = cea::AnatomyGenus::Adapter;     // Tier-Unterklasse (Ebene 2)
    static constexpr cea::AnatomyGattung gattung    = cea::AnatomyGattung::Container; // Außen-Interface (Ebene 1)
    static constexpr std::size_t         slot_count = 11; // 10 geteilt/delegiert + inner_container (INC-2d: isa raus)
    static constexpr std::string_view    name       = "Adapter";

    /// 10 geteilte/delegierte (§28; INC-2c: telemetry / INC-2d: isa sind System-Achsen) + inner_container (Inner
    /// defaultet auf DequeInner → stack/queue-Default, §26.4).
    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9,
              class Inner = cea::DequeInner<>>
    using CompositionFor = cea::AdapterComposition<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, Inner>;
    template <class Comp>
    using AnatomyFor = cea::AdapterAnatomy<Comp>;

    [[nodiscard]] static constexpr std::array<std::string_view, 11> const& axis_names() noexcept {
        static constexpr std::array<std::string_view, 11> kNames = {
            "search_algo",   "cache_traversal", "memory_layout", "allocator",        "prefetch",       "concurrency",
            "serialization", "value_handle",    "io_dispatch",   "migration_policy", "inner_container"};
        return kNames;
    }
};

/// Set (Vogel, K-only) — die 3. Gattungs-Instanz (D9/L-76a). 13 Achsen-Slots (§28 Bird; INC-2c: telemetry / INC-2d: isa sind System-Achsen; kein mapping/value_handle,
/// K-A aufgelöst). EIGENE Komposition/Anatomie (SetAnatomy treibt Composition::search_algo als Menge K=V) +
/// eigener Set-Observer (Cross-Genus type-unmöglich → getrennt). Belegt: der EINE Baum bindet auch die Set-Gattung.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::Set> {
    static constexpr cea::AnatomyGenus genus      = cea::AnatomyGenus::Set;
    static constexpr std::size_t       slot_count = 13; // INC-2d: isa raus (war 14 nach INC-2c-telemetry, 15 davor)
    static constexpr std::string_view  name       = "Set";

    /// 13-Slot-Komposition (Reihenfolge = §28 Bird-Spalte; INC-2c: telemetry / INC-2d: isa sind System-Achsen).
    /// Blatt-PermTuple<13> → SetComposition → SetAnatomy.
    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9,
              class T10, class T11, class T12>
    using CompositionFor = cea::SetComposition<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>;
    template <class Comp>
    using AnatomyFor = cea::SetAnatomy<Comp>;

    [[nodiscard]] static constexpr std::array<std::string_view, 13> const& axis_names() noexcept {
        static constexpr std::array<std::string_view, 13> kNames = {
            "search_algo",   "cache_traversal",    "path_compression", "node_type",
            "memory_layout", "allocator",          "prefetch",         "concurrency",
            "serialization", "index_organization", "io_dispatch",      "migration_policy",
            "filter"};
        return kNames;
    }
};

/// Sequence (Reptil, V-indexed) — die 4. Gattungs-Instanz (D10/L-76b). 9 Slots = 8 geteilte Achsen (§28 Reptile; INC-2c ohne telemetry, INC-2d ohne isa,
/// K-B aufgelöst) + axis_growth (eigene). EIGENE Komposition/Anatomie (SequenceAnatomy treibt V-Speicher + Growth-
/// Policy) + eigener Sequence-Observer (Cross-Genus getrennt). Belegt: der EINE Baum bindet auch die Sequence-Gattung.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::Sequence> {
    static constexpr cea::AnatomyGenus genus      = cea::AnatomyGenus::Sequence;
    static constexpr std::size_t       slot_count = 9; // 8 geteilte + axis_growth (INC-2d: isa raus)
    static constexpr std::string_view  name       = "Sequence";

    /// 8-geteilte + Growth-Slot (INC-2c: telemetry / INC-2d: isa sind System-Achsen; Growth defaultet auf
    /// DoublingGrowth → 8-arg-Aufrufe bleiben gültig).
    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7,
              class Growth = cea::DoublingGrowth>
    using CompositionFor = cea::SequenceComposition<T0, T1, T2, T3, T4, T5, T6, T7, Growth>;
    template <class Comp>
    using AnatomyFor = cea::SequenceAnatomy<Comp>;

    [[nodiscard]] static constexpr std::array<std::string_view, 9> const& axis_names() noexcept {
        static constexpr std::array<std::string_view, 9> kNames = {
            "memory_layout", "allocator",   "prefetch",         "concurrency",  "serialization",
            "value_handle",  "io_dispatch", "migration_policy", "growth_policy"};
        return kNames;
    }
};

/// View (Pflanze, non-owning) — die 5. (letzte) Gattungs-Instanz (D11/L-76c). 5 Slots = 2 geteilte Achsen (INC-2c/2d)
/// (§28 Plant, K-C aufgelöst) + axis_extent/axis_layout/axis_accessor (eigene). EIGENE Komposition/Anatomie
/// (ViewAnatomy referenziert externen Puffer non-owning, liest über layout/accessor) + eigener View-Observer.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::View> {
    static constexpr cea::AnatomyGenus genus      = cea::AnatomyGenus::View;
    static constexpr std::size_t       slot_count = 5; // 2 geteilt + extent/layout/accessor (INC-2d: isa raus)
    static constexpr std::string_view  name       = "View";

    /// 2-geteilte + extent/layout/accessor (INC-2c: telemetry / INC-2d: isa sind System-Achsen; alle drei
    /// defaulten → 2-arg-Aufrufe bleiben gültig).
    template <class T0, class T1, class Extent = cea::DynamicExtent, class Layout = cea::LayoutRight,
              class Accessor = cea::DefaultAccessor>
    using CompositionFor = cea::ViewComposition<T0, T1, Extent, Layout, Accessor>;
    template <class Comp>
    using AnatomyFor = cea::ViewAnatomy<Comp>;

    [[nodiscard]] static constexpr std::array<std::string_view, 5> const& axis_names() noexcept {
        static constexpr std::array<std::string_view, 5> kNames = {"memory_layout", "value_handle", "extent_policy",
                                                                   "layout_policy", "accessor_policy"};
        return kNames;
    }
};

/// GenusBound<G> — true gdw. die Gattung G eine Bau-Bindung (GenusBindingTraits-Spezialisierung) hat.
/// ALLE 5 Gattungen gebunden (D11): SearchAlgorithm + Adapter + Set + Sequence + View == true (5/5).
template <cea::AnatomyGenus G>
concept GenusBound = requires {
    { GenusBindingTraits<G>::slot_count } -> std::convertible_to<std::size_t>;
    GenusBindingTraits<G>::name;
};

} // namespace comdare::cache_engine::builder::experiment
