#pragma once
// BR-2-Fundament (2026-06-02, Doc 27 §6) — per-Achse COMPILE-TIME Spezial-Knoten-Deskriptoren (typsicher).
//
// User-Architektur 2026-06-02: die Knoten-Klassen sind KEINE generisch-flachen structs (Doc 26 §4), sondern
// eine compile-time Klassen-Hierarchie — Head-Concept (Wurzel der nodes) → static/dynamic-CRTP-Base →
// per-Achse-SPEZIAL-Deskriptor (jede Achse formt durch ihre typisierten Properties eine besondere Klasse).
// Jeder Deskriptor ist eine KLEINE, voll type-checkte Einheit (ein typsicherer Block-Teilbaum); der LAUFZEIT-
// Baum-Knoten verweist per block_id auf ihn zurück (Bidirektionalität). Der volle Typ-Baum wird NIE
// materialisiert (C1060) — nur einzelne Deskriptoren/Kompositionen. C++23, header-only, KEINE Achsen-Includes
// (umbrella-unabhängiges Fundament; die per-Achse-Spezial-Deskriptoren binden ihre Enabled-Listen via `variants`).

#include <concepts>
#include <cstddef>
#include <string_view>

namespace comdare::cache_engine::builder::experiment {

/// Die zwei Knotenarten (= die compile-time/runtime-Semantik, Doc 26 §4): Static → lädt eine Tier-Binary;
/// Dynamic → Laufzeit-for-Schleife auf einer geladenen Binary. KEIN struct-enum-Flag — die Art ist der TYP.
enum class DescriptorKind { Static, Dynamic };

/// Head-Concept (Wurzel der Knoten-Deskriptoren) — der compile-time Vertrag, den JEDE Spezial-Klasse erfüllt.
template <class D>
concept AxisNodeDescriptor = requires {
    { D::descriptor_kind() } -> std::same_as<DescriptorKind>;
    { D::axis_name() } -> std::convertible_to<std::string_view>;
    { D::block_id() } -> std::convertible_to<std::string_view>;
};

/// CRTP-Basis für STATISCHE Achsen-Deskriptoren. Der per-Achse-Spezial-Deskriptor erbt dies und liefert
/// `axis_name()`, `block_id()`, `using variants = <Enabled-mp_list>` + seine achsen-eigenen typisierten Properties.
template <class Derived>
struct StaticAxisDescriptorBase {
    [[nodiscard]] static constexpr DescriptorKind descriptor_kind() noexcept { return DescriptorKind::Static; }
};

/// CRTP-Basis für DYNAMISCHE Achsen-Deskriptoren (Laufzeit-Variable). Der per-Achse-Spezial-Deskriptor liefert
/// `axis_name()`, `block_id()`, `variable()` + seinen Wertebereich (Laufzeit-gebunden über Algorithm_Resource_Control).
template <class Derived>
struct DynamicAxisDescriptorBase {
    [[nodiscard]] static constexpr DescriptorKind descriptor_kind() noexcept { return DescriptorKind::Dynamic; }
};

/// Verfeinertes Concept: statischer Deskriptor (trägt `variants` = das Enabled-Wrapper-Inventar der Achse).
template <class D>
concept StaticAxisNodeDescriptor =
    AxisNodeDescriptor<D> && (D::descriptor_kind() == DescriptorKind::Static) && requires { typename D::variants; };

/// Verfeinertes Concept: dynamischer Deskriptor (trägt `variable()` = die Laufzeit-Variable der Achse).
template <class D>
concept DynamicAxisNodeDescriptor =
    AxisNodeDescriptor<D> && (D::descriptor_kind() == DescriptorKind::Dynamic) && requires {
        { D::variable() } -> std::convertible_to<std::string_view>;
    };

} // namespace comdare::cache_engine::builder::experiment
