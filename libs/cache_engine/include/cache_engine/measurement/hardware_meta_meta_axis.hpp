// measurement/hardware_meta_meta_axis.hpp -- die SYSTEM-Meta-Meta-Achsen-TYP-Familie
// (Lane C, Paket C-1', Bauplan-v3 IV.2.3, 26.07.2026).
//
// ZUSCHNITT (Ledger §69.1 / R-G, Owner-KERN): dieser Header traegt AUSSCHLIESSLICH SYSTEM-Meta-Metas --
// SIMD/AVX, externe Hardware, spaeter GPU/FPGA/NPU-Familien. `external_utils` (heute noch
// extension_hardware) bleibt deren HUB. load_framework wird hier AUSDRUECKLICH NICHT instanziiert: es
// ist per R-G eine Meta-Meta-HAUPT-Achse der MESS-Achsen (Planer-Stufe -- der Planer generiert die Loads
// und delegiert sie ans CEB-Interface) und verlaesst die System-Welt ersatzlos (Bauplan IV.2.1).
// Die aeltere Lesart "load_framework = ERSTE Meta-Meta unter dem external_utils-Hub" ist im
// HUB-ZUORDNUNGS-Teil SUPERSEDED (§69.1 GLOBAL-KLAUSEL); der Teil "NICHT eine System-Haupt-Achse"
// bleibt richtig.
//
// ARBEITSTEILUNG MIT A1 (meta_meta_identity.hpp): dort liegt die IDENTITAET -- MetaMetaSet als
// mp_list-Typliste plus subsumes_v als Halbordnung auf Mengen. HIER liegt die TYP-FAMILIE: was eine
// Meta-Meta IST, wie ein Hub seine Glieder traegt, und wie die Rekursion offen bleibt. Die Halbordnung
// wird hier NICHT nachgebaut -- es gibt genau EINE, und das ist subsumes_v.
//
// WARUM DIESER HEADER BOOST-FREI IST (Lane-C-Entscheid, vom Manager als O-2 endorsed): er wird vom HUB
// gezogen, und der Hub hat drei Compile-Konsumenten -- validate_profile.hpp:59, profile_run_facade.cpp:16
// und tools/system_axis_registry_gen/main.cpp:36. Das Generator-Target
// (tools/system_axis_registry_gen/CMakeLists.txt:10-21) linkt KEIN Boost::mp11 und setzt keinen
// Boost-Include-Pfad. Ein <boost/mp11.hpp> von hier aus wuerde nur deshalb bauen, weil /usr/include ein
// System-Suchpfad ist -- exakt die Hermetik-Luecke, die das Repo an tests/unit/CMakeLists.txt:3476-3477
// schon einmal bewusst geschlossen hat. Die Glied-Reihenfolge wird stattdessen LIST-TEMPLATE-AGNOSTISCH
// exponiert (MetaMetaMembers::as), sodass mp_list erst dort angewandt wird, wo Boost auch verlinkt ist.
// Es entsteht dadurch KEINE zweite Liste: es gibt eine einzige Glied-Reihenfolge, nur ohne festes
// Listen-Vokabular an der Definitionsstelle.
//
// NAMENS-KONVENTION: die ACHSEN-Typen tragen den Realm im Namen (SystemMetaMetaAxis), weil sie auf
// topics::AxisKind::system_meta_meta abbilden und ein kuenftiger MESS-Meta-Meta (RF-1) einen eigenen
// Diskriminator bekaeme -- so kollidieren die beiden Realms nicht. Der REKURSIONS-MECHANISMUS
// (MetaMetaMembers, transitive_members_t, hub_depth_v) bleibt realm-frei benannt, weil die
// Komplexbildung per Owner Q-F ueberall gueltig ist und auch im Mess-/Organ-Realm verwendet werden darf.
//
// LAYER-MODELL D3/D4/D5 (Session-Doc 26.07.):
//   D3  Meta-Metas sind TYPEN, keine Daten -- kein std::span<MetaMetaDescriptor const>, kein Deskriptor-Array.
//   D4  KEIN festes drittes Level: ein Glied darf selbst Hub sein (Q-D: NVIDIA-GPUs als Cluster am PCIe
//       -> Meta-Meta-Meta ...). Die Tiefe ist emergent (hub_depth_v), nirgends deklariert.
//   D5  KEIN hartkodiertes Sammel-Label: die Huelle ist ein generischer rekursiver Mechanismus ueber
//       Typen; sie erfindet keinen Namen und hat keine eigene Identitaet (Q-A: nur INDIREKTE Identitaet
//       ueber die gewrappten Glieder).
//
// KLAMMERUNG (Owner Q-A, bindend): Achsen werden IMMER getrennt geklammert, nie fusioniert. Deshalb sind
// hier ZWEI Formen streng getrennt:
//   - `meta_metas` (MetaMetaMembers) ist die GESCHACHTELTE Form. Ein Glied, das selbst Hub ist, behaelt
//     seine eigene Klammer. Das ist die stempel-taugliche Form (Klammer-ANZAHL kodiert die Ebene).
//   - `transitive_members_t` ist die FLACHE Form und existiert AUSSCHLIESSLICH fuer die Mengen-Relation
//     (Halbordnung). Sie ist NIE die Stempel-Form -- wer sie stempelt, fusioniert Achsen und bricht Q-A.
//
// BYTE-NEUTRALITAET: rein additiv. Dieser Header definiert nur Typen und constexpr-Praedikate; er
// emittiert keinen String, traegt sich in keine Achsen-Tabelle ein und ist fuer den Registry-Generator
// unsichtbar (der emittiert hartkodierte Bloecke, tools/system_axis_registry_gen/main.cpp:164-296).
//
// Benannter Pattern-Stapel (compile-time-only, keine vtable, kein Runtime-Switch): CRTP (Coplien) +
// Marker Interface + Type Traits + Layer Supertype (Fowler, PoEAA) + Concept-based Static Interface.

#pragma once

#include <cache_engine/measurement/ceb_system_axis.hpp>

#include <concepts>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

// =================================================================================================
// 1. WAS EINE SYSTEM-META-META IST
// =================================================================================================

/// Marker-Basis (Marker Interface, leer). Sie ist der OPT-IN: eine Achse wird nicht dadurch zur
/// Meta-Meta, dass sie zufaellig die richtigen Member hat, sondern dadurch, dass sie es ERKLAERT.
/// Leer und nicht-polymorph, damit alle is_empty_v-Wachen (ceb_system_axis.hpp:34, topics/axis.hpp:40,
/// extension_hardware_family_axis.hpp) unveraendert gruen bleiben.
struct SystemMetaMetaAxisTag {
protected:
    constexpr SystemMetaMetaAxisTag() noexcept = default;
};

/// SystemMetaMetaAxis<Derived> -- eine System-Meta-Meta IST eine VOLLE Konfig-System-Haupt-Achse
/// (Auftrag 1.4), die zusaetzlich als Meta-Meta markiert ist. Sie erbt das gesamte Achsen-Verhalten von
/// CebSystemAxis; diese Schicht fuegt BEWUSST keinen einzigen Member hinzu.
///
/// WARUM HIER KEIN axis_kind()-OVERRIDE STEHT: topics::AxisKind::system_meta_meta existiert seit A1
/// (topics/axis.hpp), ist dort aber ausdruecklich als rein ADDITIVER Diskriminator angelegt -- solange
/// keine Achse ihn zurueckgibt, ist er verhaltens- und byte-neutral. Die Umschaltung der konkreten
/// Achsen auf diesen Wert ist ein STEMPEL-/ORDNUNGS-Vorgang und gehoert damit in das EINE Byte-Fenster
/// hinter O-8 (Bauplan IV.4.4), nicht in dieses byte-neutrale Paket. Der Uebergabe-Punkt ist genau diese
/// Klasse: ein axis_kind()-Override hier schaltet die gesamte Familie auf einmal um.
template <class Derived>
struct SystemMetaMetaAxis : CebSystemAxis<Derived>, SystemMetaMetaAxisTag {
protected:
    constexpr SystemMetaMetaAxis() noexcept = default;
};

/// Mitgliedschafts-Trait. Default = "hat sich per Marker-Basis erklaert". Die Spezialisierbarkeit ist
/// der nicht-intrusive Weg, eine bestehende Achse aufzunehmen, ohne ihre Datei anzufassen (Adapter ueber
/// Type Traits). HEUTE wird KEINE Spezialisierung gebraucht: der einzige Kandidat waere load_framework,
/// und der ist per R-G (§69.1) ausdruecklich KEIN System-Meta-Meta.
template <class A>
struct is_system_meta_meta_axis : std::bool_constant<std::derived_from<A, SystemMetaMetaAxisTag>> {};

template <class A>
inline constexpr bool is_system_meta_meta_axis_v = is_system_meta_meta_axis<A>::value;

/// Das EINE Concept fuer System-Meta-Metas (Layer-Modell D4: kein eigenes Universum, sondern das volle
/// Achsen-Concept PLUS die beiden Eigenschaften aus Auftrag 1.4 -- Haupt-Achsen-Etikett und eine EIGENE
/// RT-Unter-Achse). Wer keine eigene Unter-Achse spannt, ist keine Meta-Meta, sondern eine gewoehnliche
/// Konfig-Achse.
template <class A>
concept SystemMetaMetaAxisConcept = CebSystemAxisConcept<A> && is_system_meta_meta_axis_v<A> && requires {
    { A::axis_label() } -> std::same_as<std::string_view>;
    { A::sub_axis_label() } -> std::same_as<std::string_view>;
};

// =================================================================================================
// 2. WIE EIN HUB SEINE GLIEDER TRAEGT (realm-freier Rekursions-Mechanismus, Owner Q-F)
// =================================================================================================

/// Die Glied-Reihenfolge eines Hubs: EIN variadisches Typ-Pack (kein std::variant, keine Daten-Sequenz).
///
/// `as<List>` exponiert dasselbe Pack in JEDEM Listen-Vokabular -- `as<MetaMetaSet>` ergibt A1s
/// mp_list-Typliste an den Stellen, wo Boost::mp11 verlinkt ist, ohne dass dieser Header mp11 zieht.
/// Dadurch existiert die Reihenfolge genau EINMAL (an der Hub-Definition) und wird nirgends nachgebildet.
template <class... Ms>
struct MetaMetaMembers {
    template <template <class...> class List>
    using as = List<Ms...>;

    /// Emergente Groesse -- deklariert wird sie nirgends.
    static constexpr std::size_t size = sizeof...(Ms);

    /// Mengen-PRIMITIVE (nicht die Halbordnung): ist M eines meiner Glieder? Die Halbordnung darauf ist
    /// und bleibt subsumes_v (meta_meta_identity.hpp) -- hier steht bewusst nur die Grundoperation.
    template <class M>
    static constexpr bool contains = (std::is_same_v<M, Ms> || ...);
};

/// Typ-Tag fuer den compile-time-Besucher (Tag Dispatch). Es transportiert einen TYP an eine Funktion,
/// ohne ihn zu instanziieren -- die boost-freie Entsprechung von mp_identity.
template <class T>
struct MetaMetaTag {
    using type = T;
};

/// Compile-time-Visitor ueber die Glieder eines Hubs (Metaprogrammierungs-Interface; Muster
/// for_each_run_methodology, run_methodology_registry.hpp:154-160). Reine Fold-Expression: kein
/// Runtime-Switch, keine vtable, keine Schleife im erzeugten Code.
template <class... Ms, class Visitor>
constexpr void for_each_meta_meta(MetaMetaMembers<Ms...>, Visitor&& visitor) {
    (visitor(MetaMetaTag<Ms>{}), ...);
}

namespace detail {

/// Ist A selbst ein Hub? Blatt-Default: nein, also leere Glied-Menge.
template <class A>
struct hub_members_of {
    using type = MetaMetaMembers<>;
};
/// Ein Glied, das SELBST Manager ist, exponiert `meta_metas` -- genau daran erkennt die Huelle die
/// naechste Ebene. Kein Level-Zaehler, kein Maximum, keine Fallunterscheidung nach Tiefe (D4).
template <class A>
    requires requires { typename A::meta_metas; }
struct hub_members_of<A> {
    using type = typename A::meta_metas;
};

/// Pack-Verkettung (Hilfsmittel der Huelle).
template <class... Lists>
struct concat_members {
    using type = MetaMetaMembers<>;
};
template <class... As>
struct concat_members<MetaMetaMembers<As...>> {
    using type = MetaMetaMembers<As...>;
};
template <class... As, class... Bs, class... Rest>
struct concat_members<MetaMetaMembers<As...>, MetaMetaMembers<Bs...>, Rest...>
    : concat_members<MetaMetaMembers<As..., Bs...>, Rest...> {};

template <class Members>
struct transitive_members_impl;

/// Ein Glied traegt sich selbst BEI und danach rekursiv alles, was es seinerseits verwaltet.
template <class A>
struct transitive_member_of {
    using type =
        typename concat_members<MetaMetaMembers<A>,
                                typename transitive_members_impl<typename hub_members_of<A>::type>::type>::type;
};

template <class... Ms>
struct transitive_members_impl<MetaMetaMembers<Ms...>> {
    using type = typename concat_members<typename transitive_member_of<Ms>::type...>::type;
};

[[nodiscard]] constexpr std::size_t max_of() noexcept { return 0; }
template <class... Rest>
[[nodiscard]] constexpr std::size_t max_of(std::size_t head, Rest... rest) noexcept {
    std::size_t const tail = max_of(rest...);
    return head > tail ? head : tail;
}

/// Primaer-Template: alles, was keine Glied-Liste ist, hat Tiefe 0.
template <class Members>
struct hub_depth_of_members {
    static constexpr std::size_t value = 0;
};
template <class A>
struct hub_depth_of {
    static constexpr std::size_t value = hub_depth_of_members<typename hub_members_of<A>::type>::value;
};
template <class... Ms>
struct hub_depth_of_members<MetaMetaMembers<Ms...>> {
    static constexpr std::size_t value = sizeof...(Ms) == 0 ? 0 : 1 + max_of(hub_depth_of<Ms>::value...);
};

} // namespace detail

/// Die FLACHE Huelle eines Hubs: alle Glieder, transitiv ueber beliebig viele Ebenen.
/// NUR fuer die Mengen-Relation (Halbordnung). NIE fuer den Stempel -- siehe Klammerungs-Auflage im
/// Kopf-Kommentar. Die geschachtelte Form ist und bleibt `meta_metas` selbst.
template <class Hub>
using transitive_members_t = typename detail::transitive_members_impl<typename detail::hub_members_of<Hub>::type>::type;

/// Emergente Verschachtelungs-Tiefe eines Hubs: 0 = kein Hub (Blatt), 1 = Hub ueber Blaettern,
/// 2 = Hub ueber Hubs, ... Der Wert ist die Gegenprobe zu "KEIN festes 3. Level": er hat kein Maximum,
/// weil ihn niemand deklariert (D4/Q-D).
template <class A>
inline constexpr std::size_t hub_depth_v = detail::hub_depth_of<A>::value;

// =================================================================================================
// 3. WOHLGEFORMTHEITS-BEWEISE (compile-time; lokale Beweis-Typen, KEINE echten Achsen)
// =================================================================================================
namespace detail {

// Ein Blatt-Glied (Tiefe 0) und drei Hub-Ebenen darueber -- der Beleg, dass die Rekursion offen ist.
struct ProofLeafA {};
struct ProofLeafB {};
struct ProofLeafC {};
struct ProofInnerHub {
    using meta_metas = MetaMetaMembers<ProofLeafB, ProofLeafC>;
};
struct ProofOuterHub {
    using meta_metas = MetaMetaMembers<ProofLeafA, ProofInnerHub>;
};
struct ProofTopHub {
    using meta_metas = MetaMetaMembers<ProofOuterHub>;
};

/// Der Besucher laeuft wirklich ueber jedes Glied -- und zwar zur UEBERSETZUNGSZEIT (consteval).
[[nodiscard]] consteval std::size_t proof_visit_count() {
    std::size_t n = 0;
    for_each_meta_meta(MetaMetaMembers<ProofLeafA, ProofLeafB, ProofLeafC>{}, [&](auto) { ++n; });
    return n;
}

} // namespace detail

static_assert(detail::proof_visit_count() == 3);

// Groesse und Mitgliedschaft sind emergent.
static_assert(MetaMetaMembers<>::size == 0);
static_assert(MetaMetaMembers<detail::ProofLeafA, detail::ProofLeafB>::size == 2);
static_assert(MetaMetaMembers<detail::ProofLeafA>::contains<detail::ProofLeafA>);
static_assert(!MetaMetaMembers<detail::ProofLeafA>::contains<detail::ProofLeafB>);

// Ein Blatt ist kein Hub; ein Hub erkennt seine Glieder ohne Level-Deklaration.
static_assert(hub_depth_v<detail::ProofLeafA> == 0);
static_assert(hub_depth_v<detail::ProofInnerHub> == 1);
static_assert(hub_depth_v<detail::ProofOuterHub> == 2);
// DER Beleg fuer "kein festes 3. Level": die vierte Ebene braucht keine einzige Zeile Code.
static_assert(hub_depth_v<detail::ProofTopHub> == 3);

// KLAMMERUNG: die geschachtelte Form behaelt ihre Struktur (2 direkte Glieder), die flache Huelle
// zaehlt alle Ebenen (4). Beide Zahlen MUESSEN verschieden sein -- waeren sie gleich, waere die
// Schachtelung eingeebnet und Q-A gebrochen.
static_assert(detail::ProofOuterHub::meta_metas::size == 2);
static_assert(transitive_members_t<detail::ProofOuterHub>::size == 4);
static_assert(transitive_members_t<detail::ProofOuterHub>::size != detail::ProofOuterHub::meta_metas::size,
              "Klammerung (Owner Q-A): die geschachtelte Form darf nie mit der flachen zusammenfallen.");
// Die flache Huelle enthaelt die Zwischen-Ebene MIT -- ein Hub ist selbst auch ein Glied.
static_assert(transitive_members_t<detail::ProofOuterHub>::contains<detail::ProofInnerHub>);
static_assert(transitive_members_t<detail::ProofOuterHub>::contains<detail::ProofLeafC>);
// Leerer Hub / Blatt: leere Huelle, keine Sonderbehandlung noetig.
static_assert(transitive_members_t<detail::ProofLeafA>::size == 0);

// Der Marker ist ein OPT-IN: ein beliebiger Typ ist keine Meta-Meta, auch wenn er wie eine aussieht.
static_assert(!is_system_meta_meta_axis_v<detail::ProofLeafA>);
static_assert(!is_system_meta_meta_axis_v<detail::ProofOuterHub>);

// Die Marker-Schicht bleibt leer und nicht-polymorph -- sonst brechen die is_empty_v-Wachen der Achsen.
static_assert(std::is_empty_v<SystemMetaMetaAxisTag> && !std::is_polymorphic_v<SystemMetaMetaAxisTag>);

} // namespace comdare::cache_engine::measurement
