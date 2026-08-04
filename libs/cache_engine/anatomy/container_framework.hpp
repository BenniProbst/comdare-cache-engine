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
// BEHAELT seinen bisherigen Achsen-Satz ("exakt die bisherigen Container-Achsen": Adapter 11 / Set 13 /
// Sequence 9 / View 5; L4/K-3-Sync auf die INC-2d-Slot-Counts der static_asserts unten) -- dieser Header
// RE-EXPORTIERT die bestehende gattungs-parametrische Bau-Bindung,
// er baut sie NICHT um. Eine echte Genus->Typ-UMSTRUKTURIERUNG (Set/Sequence als bloede Typen statt
// eigener Genus) waere ABI/golden-beruehrend und bleibt ein SEPARATER User-GO (Ledger #29-GEPARKT).
//
// **#29-ENTPARKT (F1a-Vokabular-Versoehnung, User-GO 2026-07-16):** Der obige "Ledger #29-GEPARKT"-Vermerk
// ist hiermit ENTPARKT -- die Ebene-1-Frage ist per User-GO 2026-07-16 (Increment F1b) FREIGEGEBEN.
// Dieser additive Re-Export BLEIBT als Kompatibilitaets-Sicht bestehen.
//
// **E-24 C7-7 -- HISTORIEN-NACHZUG (Kommentar-Heilung, C7-Auflage C7-7):** Zwei Angaben dieses Kopfes
// waren stale und werden hier auf den Ist gezogen, ohne die Historie zu loeschen:
//   (a) "Version 4->5": das war der Stand von F1a (Juli). Der koordinierte ABI-Schritt ist heute 7->8
//       (LEDGER:3731) und wird im E-24-Fenster als C8 vollzogen -- NICHT in diesem Header.
//   (b) "Set/Sequence als eigene AnatomyGattung": diese Zielform ist durch den Owner-KERN NACHTRAG 4
//       (LEDGER:3836) GEGENSTANDSLOS geworden. Set/Sequence/Adapter/View sind GENERA der Gattung
//       Container -- eine Promotion NEBEN Container gibt es nicht. Was das E-24-Fenster stattdessen tut
//       (S12.3 Option A): die GATTUNG wird ABI-sichtbar, die Genera bleiben, wo sie sind (C7-6).
// Unveraendert gilt: KEINE Aenderung an der AnatomyGattung/AnatomyGenus-ENUM-REIHENFOLGE (TABU), an
// golden_fullpilot_320 oder an permutation_axes.

#include <anatomy/anatomy_base.hpp>                         // AnatomyGattung/AnatomyGenus + gattung_of (constexpr)
#include <anatomy/organ_concept.hpp>                        // E-24 C7-2: OrganSized/OrganClearable (ERHOBEN, C1)
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
/// (Ebene-1-Aussen-Interface) UND (2) eine Bau-Bindung existiert (baubare Tier-Unterklasse). Das Genus
/// SearchAlgorithm gehoert zur Gattung MAP und erfuellt (1) deshalb NICHT -- es ist kein Container-Typ
/// (self-proving unten).
/// NACHZUG E-24 C11 (OP-9): dieser Satz fuehrte SearchAlgorithm als Ebene-1-Kategorie. C7-2 hat den
/// static_assert-TEXT weiter unten korrekt auf "SearchAlgorithm ist ein GENUS der Gattung Map" gezogen,
/// diesen Doku-Kommentar direkt ueber demselben Concept aber nicht -- die Datei widersprach sich selbst.
template <cea::AnatomyGenus G>
concept ContainerType = (cea::gattung_of(G) == cea::AnatomyGattung::Container) && cexp::GenusBound<G>;

/// type_list -- die Container-TYPEN als compile-time-Liste (Adapter/Set/Sequence/View). Jeder Eintrag ist
/// die AnatomyGenus-Tier-Unterklasse als integral_constant (mp11-iterierbar).
/// E-24 C7-7 (Kommentar-Heilung, C7-Auflage C7-7): hier stand "Reihenfolge = Enum-Reihenfolge" -- das war
/// FAKTISCH FALSCH. Die Liste beginnt mit Adapter (Enum-Wert 3) vor Set (1), Sequence (2) und View (4);
/// sie folgt der BAU-Reihenfolge der Genus-Instanzen (Adapter war die erste gebaute Container-Tier-
/// Unterklasse, D4b/L-75), nicht dem Enum. Die Reihenfolge ist fuer die Semantik unerheblich -- type_list
/// wird ausschliesslich compile-time iteriert -- aber sie darf nicht falsch BEHAUPTET werden.
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

    /// E-24 C7-3 -- DER GATTUNGS-TYP-VERTRAG (C7-Auflage C7-3).
    /// ElementTypeFor<Comp> == der Element-Typ, den die Anatomie dieses Container-Typs fuehrt.
    /// ABGELEITET aus der Anatomie, NICHT als Literal gepflegt: waere hier `uint64_t` hartkodiert,
    /// liefe der Vertrag beim ersten abweichenden Genus still ins Leere.
    ///
    /// WAS DAMIT EINGELOEST IST (Owner-Huellen-Kriterium, LEDGER:3834/:3836): die Container-Gattung ist
    /// das vector-Gleichnis mit EINEM Element-Parameter <T>, die Map-Gattung das map-Gleichnis mit
    /// <Key,Value>. Am Ist tragen ALLE fuenf Anatomie-Huellen genau EINEN Template-Parameter -- die
    /// Composition (das Achsen-Tupel), nicht T. Das Owner-Gleichnis lebt also in den Typ-MEMBERN, nicht
    /// in der Signatur. C7-3 macht daraus einen ausdruecklichen VERTRAG auf der Gattungs-Ebene.
    /// DEKLARIERTE LUECKE (K5, Manager-Entscheid LEDGER:3844): die ECHTE Template-Parametrisierung der
    /// Huellen (<T> bzw. <Key,Value> als Signatur) ist ABI-/golden-relevant, Scope-Erweiterung und
    /// NICHT in diesem Fenster -- eigener Owner-Entscheid nach Abgabe. Bis dahin ist der Fixpunkt die
    /// INSTANZIIERTE Gattungs-Huelle (am Ist T == uint64_t ueber alle vier Container-Genera; die Wache
    /// dafuer steht in der Test-TU, wo die vier Kompositionen instanziierbar sind).
    template <class Comp>
    using ElementTypeFor = typename AnatomyFor<Comp>::element_type;
};

// ── Self-proving (compile-time; kein Raten) ─────────────────────────────────────────────────────
// (a) Alle 4 Tier-Unterklassen der Container-Gattung sind Container-TYPEN; SearchAlgorithm ist es NICHT.
static_assert(type_count == 4, "#29: Container-Typen heute = Adapter/Set/Sequence/View");
static_assert(ContainerType<cea::AnatomyGenus::Adapter>);
static_assert(ContainerType<cea::AnatomyGenus::Set>);
static_assert(ContainerType<cea::AnatomyGenus::Sequence>);
static_assert(ContainerType<cea::AnatomyGenus::View>);
static_assert(!ContainerType<cea::AnatomyGenus::SearchAlgorithm>,
              "E-24 C7-2: SearchAlgorithm ist ein GENUS der Gattung Map, kein Container-Typ "
              "(der Assert-INHALT bleibt; der frueher hier stehende Text 'eine EIGENE Gattung' war "
              "die Ebene-Vermengung, die C7-1 aufgeloest hat).");
// (b) "exakt die bisherigen Container-Achsen": jeder Typ behaelt seinen Slot-Satz (keine Vereinheitlichung).
static_assert(type_traits<cea::AnatomyGenus::Adapter>::slot_count == 11); // INC-2d: isa raus (war 12 nach INC-2c)
static_assert(type_traits<cea::AnatomyGenus::Set>::slot_count == 13);
static_assert(type_traits<cea::AnatomyGenus::Sequence>::slot_count == 9);
static_assert(type_traits<cea::AnatomyGenus::View>::slot_count == 5);
// (c) Gattungs-Konsistenz: alle Container-Typen tragen das Container-Aussen-Interface (Ebene 1).
static_assert(type_traits<cea::AnatomyGenus::Adapter>::gattung == cea::AnatomyGattung::Container);
static_assert(type_traits<cea::AnatomyGenus::View>::gattung == cea::AnatomyGattung::Container);

// ================================================================================================
// E-24 C7-2 -- DER CONTAINER-GATTUNGS-KERN (ERHOBEN, nicht erfunden)
// ================================================================================================
//
// AUFTRAG (C7-Auflage C7-2, LEDGER:3843; Owner-KERN NACHTRAG 4 LEDGER:3836 "Das Genus erbt von der
// gemeinsamen Gattung", NACHTRAG 5 LEDGER:3838 "gestaffelte Interface Definitionen"): das
// Kopf-Framework formt den Gattungs-Kern der Ebene 1 -- die mathematische SCHNITTMENGE der
// Genus-Interfaces -- und laesst die Genus-Erweiterungen daneben stehen.
//
// WAS DER KERN ENTHAELT (am Ist erhoben, 4/4 ueber Set/Sequence/Adapter/View):
//   IDENTITAET    AnatomyConcept (composition_t / composition_name / paper_id / organ_count / genus)
//   BEOBACHTUNG   observe_axes() + axis_observation_t + observable_axis_count()  [die C3-Flaeche]
//   FUELLSTAND    size()
// WAS ALS GESTUFTER OPTIONALER BLOCK DANEBEN STEHT (3/4 -- die View fehlt BEWUSST):
//   LEERUNG       clear()
//
// WARUM GENAU DAS UND NICHTS MEHR -- die drei Gruende, jeder am Objekt belegt:
//  (1) DER STANDARD SELBST gibt fuer eine so breite Gattung nicht mehr her: die Container-named-
//      requirements werden von Adaptoren und Views NICHT erfuellt (stack hat kein begin/end, span kein
//      ==/swap). Ueber vector+set+stack+span schrumpft der ehrliche std-Schnitt auf
//      {size/empty/Lebenszyklus} -- und der Standard fuehrt den Rest als OPTIONALE Bloecke. Genau
//      dieses Muster (kleiner Kern + gestufte Bloecke) ist hier nachgebaut.
//  (2) DAS IST-SCHNITTMENGEN-VERBOT: ueber alle FUENF Genera ist die Op-Verb-Schnittmenge LEER
//      (organ_concept.hpp:92-101 -- selbst size() fehlt der SA-Gattung). Ueber die VIER
//      Container-Genera traegt size() 4/4 und clear() 3/4. Ein vereinheitlichtes Schreib-/Lese-Op-Verb
//      waere ERFUNDEN, nicht erhoben -- und das ist untersagt.
//  (3) DIE GENUS-ERWEITERUNGEN EXISTIEREN BEREITS und bleiben, wo sie sind: OrganOpSurface mit den
//      fuenf DISJUNKTEN Op-Familien (KeyedOrganOps/IndexedOrganOps/AdaptedOrganOps/BoundViewOrganOps/
//      AxisOrganAccessOps, C1). "Genus erbt von der Gattung, Genus erweitert" ist damit am Ist
//      realisiert: Kern hier, Erweiterung dort, keine Vermengung.
//
// VIEW-AUSNAHME DEKLARIERT (D2-Doktrin: leere Spalte MIT Grund): die View ist non-owning und kennt
// bewusst kein clear (view_tier.hpp:31-32, view_anatomy.hpp:4). Sie faellt deshalb aus dem
// OPTIONALEN Block -- nicht aus dem Kern. Das ist kein Mangel, sondern die Semantik der Sicht.
//
// ABGRENZUNG: die 4x-Genus-Nachweis-MATRIX steht in der Test-TU
// (tests/unit/test_e24_c7_container_gattungs_kern.cpp), nicht hier -- sie braucht instanziierte
// Kompositionen (13/9/11/5 Achsen-Typen), und die gehoeren nicht in einen Kopf-Header.

namespace cean = ::comdare::cache_engine::anatomy;

/// ContainerObservesAxes<A> -- die BEOBACHTUNGS-Haelfte des Kerns: die per-Achsen-Abnahme, die C3
/// an allen vier Container-Anatomien gebaut hat. noexcept ist Teil des Vertrags (eine Beobachtung
/// darf einen Messlauf nie abbrechen -- dieselbe Begruendung wie bei OrganObservable).
template <class A>
concept ContainerObservesAxes = requires(A const& a) {
    typename A::axis_observation_t;
    { a.observe_axes() } noexcept -> std::same_as<typename A::axis_observation_t>;
    { A::observable_axis_count() } -> std::convertible_to<std::size_t>;
};

/// ContainerGattungsKern<A> -- der Ebene-1-KERN der Container-Gattung: Identitaet + Beobachtung +
/// Fuellstand. Das ist die vollstaendige, am Ist beweisbare Schnittmenge der vier Genus-Interfaces.
template <class A>
concept ContainerGattungsKern = cean::AnatomyConcept<A> && ContainerObservesAxes<A> && cean::OrganSized<A>;

/// ContainerClearBlock<A> -- der GESTUFTE OPTIONALE Block ueber dem Kern (std-Muster "optional
/// container requirements"). 3/4 am Ist; die View fehlt deklariert (s. o.). Generischer Code fragt
/// ihn per `if constexpr` ab -- compile-time, zero-cost, ohne Runtime-Switch.
template <class A>
concept ContainerClearBlock = ContainerGattungsKern<A> && cean::OrganClearable<A>;

/// ContainerElementTyped<A> -- E-24 C7-3: die Anatomie fuehrt den Gattungs-Element-Typ oeffentlich.
/// Am Ist 4/4 (Set seit C7-3, Sequence/Adapter/View seit L-76). Er ist NICHT Teil des Kerns: der Kern
/// ist die Interface-Schnittmenge, dies ist ein TYP-Vertrag -- beides getrennt zu halten ist genau die
/// Staffelung, die NACHTRAG 5 verlangt.
template <class A>
concept ContainerElementTyped = requires { typename A::element_type; };

/// container_gattungs_kern_stufen<A>() -- wie viele der gestuften Bloecke traegt A UEBER dem Kern?
/// Diagnose-Form fuer Treiber und Tests; ABGELEITET, nicht gepflegt.
template <class A>
[[nodiscard]] consteval std::size_t container_gattungs_kern_stufen() noexcept {
    std::size_t n = 0;
    if constexpr (ContainerGattungsKern<A>) ++n; // Stufe 0: der Kern
    if constexpr (ContainerClearBlock<A>) ++n;   // Stufe 1: der optionale Leerungs-Block
    return n;
}

} // namespace comdare::container
