#pragma once
// Gattungs-Generik (2026-06-02, User-Option-B Schritt 2) — GenusBindingTraits<G>: die gattungs-PARAMETRISCHE
// Bau-Brücke. Der Baum-KERN ist bereits gattungs-agnostisch (AxisLevel/StaticAxisNode tragen beliebige Achsen);
// was eine Permutation in eine baubare Tier-Binary verwandelt, war bisher auf SearchAlgorithm (AdHocComposition
// <17>) hartcodiert. Dieses Traits hebt die Bindung auf eine pro-Gattung-Spezialisierung: jede Gattung deklariert
// ihre Slot-Zahl, Achsen-Namen, Composition-/Anatomie-Bindung. SearchAlgorithm ist der VERIFIZIERTE Spezialfall
// (BR-2/BR-3/BR-4); weitere Gattungen (Container/queuing als Adapter/Sequence, Graph) docken als 2. Instanz an,
// sobald ihre Komposition/Anatomie existiert (Cross-Genus type-unmöglich, Doku 14 §32 — daher GETRENNTE Traits).
// C++23, header-only.

#include "axis_path_serialization.hpp"                  // kCompositionAxisNames (17 SearchAlgorithm-Achsen)
#include "anatomy/anatomy_base.hpp"                      // AnatomyGenus
#include "anatomy/composition_factory.hpp"               // CompositionFromPermTuple / AdHocComposition<17>
#include "anatomy/search_algorithm_anatomy.hpp"          // SearchAlgorithmAnatomy
#include "anatomy/container_anatomy.hpp"                 // ContainerAnatomy / ContainerComposition (Container-Gattung)
#include "anatomy/set_anatomy.hpp"                       // SetAnatomy / SetComposition (Set-Gattung, D9)

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

namespace cea = ::comdare::cache_engine::anatomy;

/// Primär UNDEFINIERT — jede UNTERSTÜTZTE Gattung liefert eine vollständige Spezialisierung. Eine Gattung ohne
/// Spezialisierung ist (noch) nicht baubar — der Baum trägt ihre Achsen, aber es gibt keine Komposition/Anatomie.
template <cea::AnatomyGenus G>
struct GenusBindingTraits;

/// SearchAlgorithm — der VERIFIZIERTE Spezialfall (alle 4 Brücken literal grün). 17-Slot-Komposition.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::SearchAlgorithm> {
    static constexpr cea::AnatomyGenus genus = cea::AnatomyGenus::SearchAlgorithm;
    static constexpr std::size_t       slot_count = 17;
    static constexpr std::string_view  name = "SearchAlgorithm";

    /// Blatt-PermTuple<17> → reale Komposition (AdHocComposition<17>) → Gattungs-Anatomie.
    template <class PermT> using CompositionFor = cea::CompositionFromPermTuple<PermT>;
    template <class Comp>  using AnatomyFor     = cea::SearchAlgorithmAnatomy<Comp>;

    /// Die Achsen-Namen der Komposition-Slots (Reihenfolge T0..T16) — zentrale Pfad-Konvention (BR-2).
    [[nodiscard]] static constexpr std::array<std::string_view, 17> const& axis_names() noexcept {
        return kCompositionAxisNames;
    }
};

/// Adapter (Container/Queue) — die 2. Gattungs-Instanz (queuing q1/q2). D4/L-75: 2 Slots — Q1 (buffer_strategy)
/// + Q2 (flush_policy). EIGENE Komposition/Anatomie + eigener Container-Observer (Cross-Genus type-unmöglich →
/// getrennt von SearchAlgorithm). Belegt: der EINE Baum bindet auch die Mehr-Achsen-Container-Gattung.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::Adapter> {
    static constexpr cea::AnatomyGenus genus = cea::AnatomyGenus::Adapter;
    static constexpr std::size_t       slot_count = 2;   // Q1 buffer_strategy + Q2 flush_policy
    static constexpr std::string_view  name = "Container";

    /// 2-Slot-Komposition (Q2 defaultet auf ContainerNoFlushPolicy → 1-arg-Aufrufe bleiben gültig).
    template <class Q1Buffer, class Q2Flush = cea::ContainerNoFlushPolicy>
    using CompositionFor = cea::ContainerComposition<Q1Buffer, Q2Flush>;
    template <class Comp> using AnatomyFor = cea::ContainerAnatomy<Comp>;

    [[nodiscard]] static constexpr std::array<std::string_view, 2> const& axis_names() noexcept {
        static constexpr std::array<std::string_view, 2> kNames = {"queuing_q1", "queuing_q2"};
        return kNames;
    }
};

/// Set (Vogel, K-only) — die 3. Gattungs-Instanz (D9/L-76a). 15 Achsen-Slots (§28 Bird, kein mapping/value_handle,
/// K-A aufgelöst). EIGENE Komposition/Anatomie (SetAnatomy treibt Composition::search_algo als Menge K=V) +
/// eigener Set-Observer (Cross-Genus type-unmöglich → getrennt). Belegt: der EINE Baum bindet auch die Set-Gattung.
template <>
struct GenusBindingTraits<cea::AnatomyGenus::Set> {
    static constexpr cea::AnatomyGenus genus = cea::AnatomyGenus::Set;
    static constexpr std::size_t       slot_count = 15;
    static constexpr std::string_view  name = "Set";

    /// 15-Slot-Komposition (Reihenfolge = §28 Bird-Spalte). Blatt-PermTuple<15> → SetComposition → SetAnatomy.
    template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7,
              class T8, class T9, class T10, class T11, class T12, class T13, class T14>
    using CompositionFor = cea::SetComposition<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>;
    template <class Comp> using AnatomyFor = cea::SetAnatomy<Comp>;

    [[nodiscard]] static constexpr std::array<std::string_view, 15> const& axis_names() noexcept {
        static constexpr std::array<std::string_view, 15> kNames = {
            "search_algo", "cache_traversal", "path_compression", "node_type", "memory_layout",
            "allocator", "prefetch", "concurrency", "serialization", "telemetry",
            "isa", "index_organization", "io_dispatch", "migration_policy", "filter"};
        return kNames;
    }
};

/// GenusBound<G> — true gdw. die Gattung G eine Bau-Bindung (GenusBindingTraits-Spezialisierung) hat.
/// SearchAlgorithm + Adapter (Container) + Set == true (3/5, D9); Sequence/View == false (Achsen im Baum, Bau-Brücke folgt).
template <cea::AnatomyGenus G>
concept GenusBound = requires {
    { GenusBindingTraits<G>::slot_count } -> std::convertible_to<std::size_t>;
    GenusBindingTraits<G>::name;
};

}  // namespace comdare::cache_engine::builder::experiment
