#pragma once
// SF-1 (2026-08-08, Schichtschnitt) -- Builder-seitiger Adapter fuer comdare::container::type_traits/
// ContainerType. anatomy/container_framework.hpp kennt seit diesem Schnitt KEINEN Namen aus builder/
// mehr (die Bau-Bindung ist dort ein Template-PARAMETER, kein interner Include -- siehe die
// Kommentar-Historie in container_framework.hpp selbst). Dieser Header schliesst die Luecke von der
// ANDEREN Seite: NUR hier (builder/) ist der gemeinsame Include von anatomy/container_framework.hpp und
// genus_binding_traits.hpp legitim (Abhaengigkeitsrichtung builder/ -> anatomy/, nicht umgekehrt).
//
// Er liefert zwei Dinge:
//   (1) die bequemen 1-Parameter-Aliase type_traits<G>/ContainerType<G>, die alle bestehenden
//       Aufrufstellen (Tests) unveraendert weiter nutzen koennen -- G allein reicht wieder, weil die
//       Bindung hier automatisch auf GenusBindingTraits<G> gelegt wird;
//   (2) den KONKRETEN Self-proving-Beweis (Slot-Pins 11/13/9/5), den container_framework.hpp seit SF-1
//       nicht mehr selbst fuehren kann -- er kennt keine echte Bindung mehr, nur noch eine Mock-Bindung
//       fuer den Struktur-Beweis (dort in container_framework_self_proof_detail).

#include "genus_binding_traits.hpp" // GenusBindingTraits<G> (die Bau-Bruecke je Gattung)

#include <anatomy/container_framework.hpp> // comdare::container::type_traits<G, Binding> / ContainerType<G, Binding>

namespace comdare::cache_engine::builder::experiment {

namespace cea = ::comdare::cache_engine::anatomy;

/// type_traits<G> -- Alias auf comdare::container::type_traits<G, GenusBindingTraits<G>>. Der bequeme
/// 1-Parameter-Aufrufpunkt fuer alle bisherigen Aufrufstellen; lebt bewusst HIER (builder-Seite), denn
/// NUR hier ist der Include von genus_binding_traits.hpp legitim (SF-1-Schichtschnitt).
template <cea::AnatomyGenus G>
using type_traits = comdare::container::type_traits<G, GenusBindingTraits<G>>;

/// ContainerType<G> -- derselbe 1-Parameter-Alias fuer das Concept.
template <cea::AnatomyGenus G>
concept ContainerType = comdare::container::ContainerType<G, GenusBindingTraits<G>>;

// -- Self-proving (compile-time; kein Raten) -----------------------------------------------------
// Die KONKRETEN 4 Container-Typen mit der echten Bau-Bindung. Dies ist der Teil des alten
// container_framework.hpp-Selbstbeweises, der eine reale Bindung braucht (11/13/9/5 Slot-Pins,
// Gattungs-Konsistenz) und deshalb seit SF-1 hierher gehoert, nicht mehr auf die anatomy/-Seite.
static_assert(ContainerType<cea::AnatomyGenus::Adapter>);
static_assert(ContainerType<cea::AnatomyGenus::Set>);
static_assert(ContainerType<cea::AnatomyGenus::Sequence>);
static_assert(ContainerType<cea::AnatomyGenus::View>);
static_assert(!ContainerType<cea::AnatomyGenus::SearchAlgorithm>,
              "E-24 C7-2: SearchAlgorithm ist ein GENUS der Gattung Map, kein Container-Typ "
              "(der Assert-INHALT bleibt; der frueher hier stehende Text 'eine EIGENE Gattung' war "
              "die Ebene-Vermengung, die C7-1 aufgeloest hat).");
// "exakt die bisherigen Container-Achsen": jeder Typ behaelt seinen Slot-Satz (keine Vereinheitlichung).
static_assert(type_traits<cea::AnatomyGenus::Adapter>::slot_count == 11); // INC-2d: isa raus (war 12 nach INC-2c)
static_assert(type_traits<cea::AnatomyGenus::Set>::slot_count == 13);
static_assert(type_traits<cea::AnatomyGenus::Sequence>::slot_count == 9);
static_assert(type_traits<cea::AnatomyGenus::View>::slot_count == 5);
// Gattungs-Konsistenz: alle Container-Typen tragen das Container-Aussen-Interface (Ebene 1).
static_assert(type_traits<cea::AnatomyGenus::Adapter>::gattung == cea::AnatomyGattung::Container);
static_assert(type_traits<cea::AnatomyGenus::View>::gattung == cea::AnatomyGattung::Container);

} // namespace comdare::cache_engine::builder::experiment
