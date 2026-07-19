#pragma once
// builder/experiment_tree/genus_organ_binding.hpp -- INC-1b (2026-07-18): die Organ-Achsen DEFINIEREN
// (typisiert + drift-guarded) welche Gattung welche Organ-Achsen VERWENDET und BRAUCHT.
//
// User-Ruling 2026-07-18 (Freigabe-Prinzip): "die Organ-Achsen muessen parallel noch die Gattungen definieren,
// welche Gattung welche Organ-Achsen verwendet und benoetigt." GenusBindingTraits<G> (TABU-Registry) traegt die
// axis_names() heute als FREIE std::string_view-Arrays -> Drift-Gefahr (Tippfehler, falsche Slot-Zahl, versehentlich
// eine System-/Fremd-Achse). Diese Datei fuegt ADDITIV (KEIN Touch an GenusBindingTraits; kCompositionAxisNames,
// slot_count, ABI, golden==320 unberuehrt) einen compile-time Konsistenz-/Bindungs-Vertrag hinzu:
//   (1) das ORGAN-Achsen-UNIVERSUM (die 17 Komposition-Achsen kCompositionAxisNames + die genus-spezifischen
//       Erweiterungs-Organ-Achsen), gegen das eine Gattung binden DARF -- KEINE System-/Mess-Achse.
//   (2) RequiredOrgans<G>() -- die typisierte, drift-geguardte Abfrage "welche Organ-Achsen verlangt Gattung G".
//   (3) genus_organ_binding_consistent<G>() -- der Drift-Guard je Gattung (Groesse == slot_count, jedes Label
//       ist eine echte Organ-Achse, keine Dublette); ein Fehler bricht den Build SICHTBAR statt still zu driften.
//
// Benannter Pattern-Stapel (compile-time-only, keine vtable): Specification/Validator (constexpr Gegen-Pruefung)
// + Type-List/Compile-Time-Sequence (constexpr std::array statt Runtime). C++23, header-only.

#include "genus_binding_traits.hpp" // GenusBindingTraits<G> (READ-ONLY, TABU) + GenusBound<G> + kCompositionAxisNames + cea

#include <array>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

// ── Das Organ-Achsen-UNIVERSUM: die genus-spezifischen Erweiterungs-Organ-Achsen NEBEN den 17 Komposition-Achsen.
//    Diese sind Organ (per-Gattung eigene binary_id-neutrale Slots wie inner_container/growth/extent), KEINE
//    System-Achse. Eine Gattung darf NUR gegen kCompositionAxisNames ∪ diese Menge binden. ──
inline constexpr std::array<std::string_view, 5> kGenusExtensionOrganAxes = {
    "inner_container", // Adapter (Container-Gattung): der innere Speicher-Container
    "growth_policy",   // Sequence: Wachstums-Politik (DoublingGrowth …)
    "extent_policy",   // View: Extent (DynamicExtent …)
    "layout_policy",   // View: Speicher-Layout (LayoutRight …)
    "accessor_policy", // View: Accessor (DefaultAccessor …)
};

/// Ist `label` eine gueltige Organ-Achse (Komposition-Achse ODER genus-spezifische Erweiterungs-Organ-Achse)?
[[nodiscard]] constexpr bool is_organ_axis_label(std::string_view label) noexcept {
    for (auto const& n : kCompositionAxisNames)
        if (n == label) return true;
    for (auto const& n : kGenusExtensionOrganAxes)
        if (n == label) return true;
    return false;
}

/// RequiredOrgans<G>() -- die (drift-geguardte) Menge der Organ-Achsen, die Gattung G verwendet/braucht. Single-
/// Source bleibt die TABU-Bindung GenusBindingTraits<G>::axis_names(); diese Sicht garantiert (via die
/// static_asserts unten), dass die Menge konsistent + rein-Organ ist.
template <cea::AnatomyGenus G>
    requires GenusBound<G>
[[nodiscard]] constexpr auto const& RequiredOrgans() noexcept {
    return GenusBindingTraits<G>::axis_names();
}

/// Drift-Guard je Gattung: (a) axis_names().size() == slot_count, (b) jedes Label ist eine echte Organ-Achse
/// (∈ Universum -> KEINE System-/Fremd-Achse), (c) keine Dublette. Rein compile-time; ein Verstoss (Tippfehler,
/// falsche Slot-Zahl, versehentliche System-Achse, Doppel-Slot) laesst den static_assert unten fehlschlagen.
template <cea::AnatomyGenus G>
    requires GenusBound<G>
[[nodiscard]] constexpr bool genus_organ_binding_consistent() noexcept {
    auto const& names = GenusBindingTraits<G>::axis_names();
    if (names.size() != GenusBindingTraits<G>::slot_count) return false;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (!is_organ_axis_label(names[i])) return false;
        for (std::size_t j = i + 1; j < names.size(); ++j)
            if (names[i] == names[j]) return false;
    }
    return true;
}

// ── Zementierung: alle 5 gebundenen Gattungen verwenden ausschliesslich konsistente Organ-Achsen (drift-frei). ──
static_assert(genus_organ_binding_consistent<cea::AnatomyGenus::SearchAlgorithm>(),
              "SearchAlgorithm-Bindung driftet (Slot-Zahl / Nicht-Organ-Achse / Dublette).");
static_assert(genus_organ_binding_consistent<cea::AnatomyGenus::Adapter>(), "Adapter-Bindung driftet.");
static_assert(genus_organ_binding_consistent<cea::AnatomyGenus::Set>(), "Set-Bindung driftet.");
static_assert(genus_organ_binding_consistent<cea::AnatomyGenus::Sequence>(), "Sequence-Bindung driftet.");
static_assert(genus_organ_binding_consistent<cea::AnatomyGenus::View>(), "View-Bindung driftet.");

// SearchAlgorithm ist der Kanon (BR-2): seine Organ-Achsen sind EXAKT die kCompositionAxisNames (Reihenfolge + Zahl).
static_assert(RequiredOrgans<cea::AnatomyGenus::SearchAlgorithm>().size() == kCompositionAxisNames.size());

} // namespace comdare::cache_engine::builder::experiment
