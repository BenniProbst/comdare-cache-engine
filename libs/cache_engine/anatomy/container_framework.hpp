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

// SF-1 (2026-08-08, Schichtschnitt): dieser Header GEHOERT zu anatomy/ (untere Schicht) und darf keinen
// Namen aus builder/ (obere Schicht) kennen -- hier waere sie ein Include-Zyklus, exakt wie in den vier
// Geschwister-Headern, die denselben Verzicht schon dokumentieren (adapter_anatomy.hpp:205,
// genus_observer_aggregate.hpp:130, organ_concept.hpp:26, sequence_anatomy.hpp:43). Bis hierher war
// dieser Header der EINE Ausreisser: er zog builder/experiment_tree/genus_binding_traits.hpp per Include.
// Die Bau-Bindung ist deshalb ab hier ein Template-PARAMETER (Binding), keine interne Suche mehr -- wer
// sie braucht, reicht sie herein. Die Builder-Seite tut das ueber einen eigenen kleinen Adapter-Header
// (builder/experiment_tree/container_type_traits.hpp), wo der Include auf genus_binding_traits.hpp
// legitim ist (Abhaengigkeitsrichtung builder/ -> anatomy/, nicht umgekehrt).
#include <anatomy/anatomy_base.hpp>  // AnatomyGattung/AnatomyGenus + gattung_of (constexpr)
#include <anatomy/organ_concept.hpp> // E-24 C7-2: OrganSized/OrganClearable (ERHOBEN, C1)

#include <boost/mp11.hpp>

#include <cstddef>
#include <string_view>
#include <type_traits>

namespace comdare::container {

namespace cea = ::comdare::cache_engine::anatomy;
namespace mp  = boost::mp11;

/// GenusBuildBinding<G, Binding> -- Schicht-lokaler Spiegel von builder::experiment::GenusBound<G>
/// (genus_binding_traits.hpp), OHNE den Builder-Typ zu kennen: die Bau-Bindung wird als Template-
/// PARAMETER hereingereicht statt hier per GenusBindingTraits<G> nachgeschlagen (SF-1-Umkehrung, s.o.).
/// Die Struktur-Anforderungen sind dieselben, die GenusBound bereits prueft (slot_count/name); dazu
/// verlangt sie Binding::genus == G, damit niemand versehentlich die Bindung eines FREMDEN Genus
/// hereinreicht (der alte Weg per globaler Spezialisierung GenusBindingTraits<G> konnte das nicht
/// versehentlich falsch machen, weil G die Spezialisierung selbst auswaehlte -- als Template-Parameter
/// muss die Zusicherung jetzt explizit stehen).
template <cea::AnatomyGenus G, class Binding>
concept GenusBuildBinding = requires {
    { Binding::slot_count } -> std::convertible_to<std::size_t>;
    Binding::name;
    requires Binding::genus == G;
};

/// ContainerType<G, Binding> -- G ist ein Container-TYP gdw. (1) seine Gattung die Container-Gattung ist
/// (Ebene-1-Aussen-Interface) UND (2) Binding eine passende Bau-Bindung ist (baubare Tier-Unterklasse).
/// Das Genus SearchAlgorithm gehoert zur Gattung MAP und erfuellt (1) deshalb NICHT -- es ist kein
/// Container-Typ (self-proving unten).
/// NACHZUG E-24 C11 (OP-9): dieser Satz fuehrte SearchAlgorithm als Ebene-1-Kategorie. C7-2 hat den
/// static_assert-TEXT weiter unten korrekt auf "SearchAlgorithm ist ein GENUS der Gattung Map" gezogen,
/// diesen Doku-Kommentar direkt ueber demselben Concept aber nicht -- die Datei widersprach sich selbst.
template <cea::AnatomyGenus G, class Binding>
concept ContainerType = (cea::gattung_of(G) == cea::AnatomyGattung::Container) && GenusBuildBinding<G, Binding>;

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

/// type_traits<G, Binding> -- das generische comdare::container-Interface je Container-TYP.
/// RE-EXPORTIERT die hereingereichte Binding (Achsen-Satz/Slot-Zahl/Komposition/Anatomie bleiben
/// unveraendert je Typ). Binding ist am Ist praktisch immer builder::experiment::GenusBindingTraits<G>
/// (Bau-Bindung) -- dieser Header schlaegt sie aber nicht mehr selbst nach (SF-1). Der bequeme
/// 1-Parameter-Aufrufpunkt fuer bestehende Aufrufstellen lebt im Builder-Adapter
/// (builder/experiment_tree/container_type_traits.hpp).
template <cea::AnatomyGenus G, class Binding>
    requires ContainerType<G, Binding>
struct type_traits {
    static constexpr cea::AnatomyGenus   genus      = G;
    static constexpr cea::AnatomyGattung gattung    = cea::AnatomyGattung::Container; // Ebene-1-Aussen-Interface
    static constexpr std::size_t         slot_count = Binding::slot_count;
    static constexpr std::string_view    name       = Binding::name;

    /// Blatt-PermTuple -> reale Komposition (unveraendert je Typ; nur weitergereicht).
    template <class... T>
    using CompositionFor = typename Binding::template CompositionFor<T...>;
    template <class Comp>
    using AnatomyFor = typename Binding::template AnatomyFor<Comp>;

    /// Der bisherige Achsen-Satz dieses Typs (exakt beibehalten).
    [[nodiscard]] static constexpr auto const& axis_names() noexcept { return Binding::axis_names(); }

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
// (a) type_count/type_list sind genus-eigen und brauchen keine Bindung -- unveraendert pruefbar.
static_assert(type_count == 4, "#29: Container-Typen heute = Adapter/Set/Sequence/View");
// (b) die KONKRETEN 4 Container-Typen (echte GenusBindingTraits-Bindung, Slot-Pins 11/13/9/5,
// Gattungs-Konsistenz) beweist seit SF-1 der Builder-Adapter, NICHT mehr dieser Header --
// builder/experiment_tree/container_type_traits.hpp, dort im gleichnamigen Self-proving-Block.
// Dieser Header selbst kennt keine reale Bindung mehr (das ist der ganze Punkt des Schnitts); er kann
// also nur noch beweisen, dass sein EIGENER Mechanismus (Concept + Struct) mit IRGENDEINER strukturell
// passenden Bindung funktioniert -- dafuer dient die Mock-Bindung unten.
namespace container_framework_self_proof_detail {
/// MockGenusBinding -- winziger Fake, der NICHTS mit builder/ zu tun hat. Er beweist, dass
/// GenusBuildBinding/ContainerType/type_traits mit JEDER strukturell passenden Bindung arbeiten, nicht
/// nur mit der echten GenusBindingTraits (die dieser Header seit SF-1 nicht mehr kennt).
struct MockGenusBinding {
    static constexpr cea::AnatomyGenus genus      = cea::AnatomyGenus::Adapter;
    static constexpr std::size_t       slot_count = 3;
    static constexpr std::string_view  name       = "MockGenusBinding";

    // CompositionFor/AnatomyFor sind hier ohne Bedeutung (dieser Selbstbeweis ruft sie nie mit
    // konkreten Argumenten auf) -- sie muessen trotzdem als Member-Templates EXISTIEREN: der
    // Compiler loest `Binding::template CompositionFor` in type_traits schon bei der
    // Instanziierung der Klasse auf (Binding ist dort nicht mehr dependent), nicht erst beim Aufruf.
    template <class... T>
    using CompositionFor = void;
    // AnatomyFor MUSS von Comp abhaengen (Identity statt void): type_traits<G,Binding>::ElementTypeFor
    // bildet AnatomyFor<Comp>::element_type. Waere AnatomyFor<Comp> immer 'void' (unabhaengig von
    // Comp), faltet der Compiler das schon bei der Klassen-Instanziierung zu void::element_type und
    // meldet einen Fehler, obwohl ElementTypeFor hier nie mit einem konkreten Comp aufgerufen wird.
    template <class Comp>
    using AnatomyFor = Comp;
};
} // namespace container_framework_self_proof_detail

static_assert(GenusBuildBinding<cea::AnatomyGenus::Adapter, container_framework_self_proof_detail::MockGenusBinding>);
// Genus-Mismatch (Set statt Adapter) MUSS die Bindung ablehnen -- das ist der Schutz, den frueher die
// globale Spezialisierung GenusBindingTraits<G> implizit gab (G waehlte die Spezialisierung selbst).
static_assert(!GenusBuildBinding<cea::AnatomyGenus::Set, container_framework_self_proof_detail::MockGenusBinding>,
              "SF-1: eine Bindung fuer Adapter darf nicht als Bindung fuer Set durchgehen");
static_assert(ContainerType<cea::AnatomyGenus::Adapter, container_framework_self_proof_detail::MockGenusBinding>);
static_assert(
    type_traits<cea::AnatomyGenus::Adapter, container_framework_self_proof_detail::MockGenusBinding>::slot_count == 3);
static_assert(
    type_traits<cea::AnatomyGenus::Adapter, container_framework_self_proof_detail::MockGenusBinding>::gattung ==
    cea::AnatomyGattung::Container);
// SearchAlgorithm ist auch mit einer (falsch typisierten) Adapter-Mock-Bindung kein Container-Typ --
// die Gattungs-Pruefung (1) greift schon vor der Bindungs-Pruefung (2).
static_assert(
    !ContainerType<cea::AnatomyGenus::SearchAlgorithm, container_framework_self_proof_detail::MockGenusBinding>,
    "E-24 C7-2: SearchAlgorithm ist ein GENUS der Gattung Map, kein Container-Typ -- unabhaengig "
    "von der Bindung.");

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
