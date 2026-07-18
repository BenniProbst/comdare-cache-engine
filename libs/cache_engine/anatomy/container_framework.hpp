#pragma once
// #29 Schritt 1 (2026-07-08, User-Plan AP-15) -- comdare::container Kopf-Framework-Interface.
//
// **Zweck (F6-Modul-Framework-Doktrin, [[feedback_achsen_thema_modul_framework_metaprogramming_interface]]):**
// Generalisiert die BEREITS GEBAUTE Container-Gattung (Ebene 1 AnatomyGattung::Container) zu einem
// metaprogrammierbaren `comdare::container`-Kopf-Interface ueber ihre TYPEN -- die Tier-Unterklassen
// (Ebene 2 AnatomyGenus) Adapter/Set/Sequence/View. Damit kann compile-time GENERISCH ueber alle
// Container-TYPEN iteriert werden (AP-15: "gleiches Interface, KEINE eigenen Gattungen" -- die 4 Genus
// werden hier als comdare::container-Typen unter EINEM Aussen-Interface praesentiert).
//
// **ADDITIV & golden/ABI-NEUTRAL (bewusster Scope, Anatomie-/ABI-Kern unberuehrt):** KEINE Aenderung an
// AnatomyGattung/AnatomyGenus-Enum, an den 4 Anatomien (adapter/set/sequence/view_anatomy.hpp), an
// GenusBindingTraits, an golden_fullpilot_320_binary_ids.txt oder permutation_axes.xml. Jeder Container-TYP
// BEHAELT seinen bisherigen Achsen-Satz ("exakt die bisherigen Container-Achsen": Adapter 13 / Set 15 /
// Sequence 11 / View 7) -- dieser Header RE-EXPORTIERT die bestehende gattungs-parametrische Bau-Bindung,
// er baut sie NICHT um. Eine echte Genus->Typ-UMSTRUKTURIERUNG (Set/Sequence als bloede Typen statt
// eigener Genus) waere ABI/golden-beruehrend und bleibt ein SEPARATER User-GO (Ledger #29-GEPARKT).
//
// **#29-ENTPARKT (F1a-Vokabular-Versoehnung, User-GO 2026-07-16):** Der obige „Ledger #29-GEPARKT"-Vermerk
// ist hiermit ENTPARKT — die Genus->Ebene-1-Umstrukturierung (Set/Sequence als eigene AnatomyGattung) ist
// per User-GO 2026-07-16 (Increment F1b) FREIGEGEBEN. Dieser additive Re-Export BLEIBT als Kompatibilitaets-
// Sicht bestehen. Die Umsetzung ist ein KOORDINIERTER ABI-Schritt (Version 4->5, gemeinsam mit #37 nach
// F12iii) und laeuft mit eigener Kadenz. HIER (F1a) passiert NUR die Vokabular-Klarstellung: KEINE
// Aenderung an AnatomyGattung/AnatomyGenus-Enum, GenusBindingTraits, golden_fullpilot_320 oder permutation_axes.

#include <anatomy/anatomy_base.hpp>                         // AnatomyGattung/AnatomyGenus + gattung_of (constexpr)
#include <builder/experiment_tree/genus_binding_traits.hpp> // GenusBindingTraits<G> + GenusBound<G> (Bau-Bindung je Genus)

#include <boost/mp11.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::container {

namespace cea  = ::comdare::cache_engine::anatomy;
namespace cexp = ::comdare::cache_engine::builder::experiment;
namespace mp   = boost::mp11;

/// ContainerType<G> -- G ist ein Container-TYP gdw. (1) seine Gattung die Container-Gattung ist
/// (Ebene-1-Aussen-Interface) UND (2) eine Bau-Bindung existiert (baubare Tier-Unterklasse). Die
/// SearchAlgorithm-Gattung erfuellt (1) NICHT und ist damit kein Container-Typ (self-proving unten).
template <cea::AnatomyGenus G>
concept ContainerType = (cea::gattung_of(G) == cea::AnatomyGattung::Container) && cexp::GenusBound<G>;

/// type_list -- die Container-TYPEN als compile-time-Liste (Adapter/Set/Sequence/View). Jeder Eintrag ist
/// die AnatomyGenus-Tier-Unterklasse als integral_constant (mp11-iterierbar). Reihenfolge = Enum-Reihenfolge.
using type_list = mp::mp_list<std::integral_constant<cea::AnatomyGenus, cea::AnatomyGenus::Adapter>,
                              std::integral_constant<cea::AnatomyGenus, cea::AnatomyGenus::Set>,
                              std::integral_constant<cea::AnatomyGenus, cea::AnatomyGenus::Sequence>,
                              std::integral_constant<cea::AnatomyGenus, cea::AnatomyGenus::View>>;

/// type_count -- Anzahl der Container-Genus-TYPEN (heute 4, Ebene 2). Weitere std-Container (linked list,
/// deque, array ...) werden als Ebene-3-Realisierungen UNTER den bestehenden Genus geplant (Option A,
/// empfohlen, type_count bleibt 4) ODER als eigener Genus (Option B, type_count-wachsend) -- Fork +
/// Empfehlung: docs/architecture/37_ap15_container_typen_sequence_plan.md (AP-15 Punkt 3).
inline constexpr std::size_t type_count = mp::mp_size<type_list>::value;

/// type_traits<G> -- das generische comdare::container-Interface je Container-TYP. RE-EXPORTIERT die
/// bestehende GenusBindingTraits<G> (Achsen-Satz/Slot-Zahl/Komposition/Anatomie bleiben unveraendert je Typ).
template <cea::AnatomyGenus G>
    requires ContainerType<G>
struct type_traits {
    static constexpr cea::AnatomyGenus   genus      = G;
    static constexpr cea::AnatomyGattung gattung    = cea::AnatomyGattung::Container; // Ebene-1-Aussen-Interface
    static constexpr std::size_t         slot_count = cexp::GenusBindingTraits<G>::slot_count;
    static constexpr std::string_view    name       = cexp::GenusBindingTraits<G>::name;

    /// Blatt-PermTuple -> reale Komposition (unveraendert je Typ; nur weitergereicht).
    template <class... T>
    using CompositionFor = typename cexp::GenusBindingTraits<G>::template CompositionFor<T...>;
    template <class Comp>
    using AnatomyFor = typename cexp::GenusBindingTraits<G>::template AnatomyFor<Comp>;

    /// Der bisherige Achsen-Satz dieses Typs (exakt beibehalten).
    [[nodiscard]] static constexpr auto const& axis_names() noexcept {
        return cexp::GenusBindingTraits<G>::axis_names();
    }
};

// ── Self-proving (compile-time; kein Raten) ─────────────────────────────────────────────────────
// (a) Alle 4 Tier-Unterklassen der Container-Gattung sind Container-TYPEN; SearchAlgorithm ist es NICHT.
static_assert(type_count == 4, "#29: Container-Typen heute = Adapter/Set/Sequence/View");
static_assert(ContainerType<cea::AnatomyGenus::Adapter>);
static_assert(ContainerType<cea::AnatomyGenus::Set>);
static_assert(ContainerType<cea::AnatomyGenus::Sequence>);
static_assert(ContainerType<cea::AnatomyGenus::View>);
static_assert(!ContainerType<cea::AnatomyGenus::SearchAlgorithm>,
              "#29: SearchAlgorithm ist eine EIGENE Gattung, kein Container-Typ");
// (b) "exakt die bisherigen Container-Achsen": jeder Typ behaelt seinen Slot-Satz (keine Vereinheitlichung).
static_assert(type_traits<cea::AnatomyGenus::Adapter>::slot_count == 11); // INC-2d: isa raus (war 12 nach INC-2c)
static_assert(type_traits<cea::AnatomyGenus::Set>::slot_count == 13);
static_assert(type_traits<cea::AnatomyGenus::Sequence>::slot_count == 9);
static_assert(type_traits<cea::AnatomyGenus::View>::slot_count == 5);
// (c) Gattungs-Konsistenz: alle Container-Typen tragen das Container-Aussen-Interface (Ebene 1).
static_assert(type_traits<cea::AnatomyGenus::Adapter>::gattung == cea::AnatomyGattung::Container);
static_assert(type_traits<cea::AnatomyGenus::View>::gattung == cea::AnatomyGattung::Container);

} // namespace comdare::container
