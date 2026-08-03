#pragma once
// E-24 / S12.1 -- C0 (M0-Vorstufe, 2026-08-04): OrganConcept -- die GEMEINSAME Vertrags-Flaeche der
// Gattungs-Anatomien. Zielbild des Fensters: ein Genus-Organ (SearchAlgorithm/Set/Sequence/Adapter/View)
// kann generisch als SUB-ORGAN in einem Anatomie-Slot einer ANDEREN Gattung gehalten werden
// (Cross-Genus-Komposition-als-Sub-Organ). Dieser Header ist der erste, ABI-neutrale Schritt dahin.
//
// SCHNITT C0 (2026-08-04, gelandet als 47be8211 -- ABI-NEUTRAL, bewusst klein):
//   * NUR der Vertrag + die am HEUTIGEN Ist beweisbaren Selbst-Beweise.
//   * KEINE bestehende Anatomie wird geaendert, KEINE ABI-Flaeche beruehrt (abi_adapter.hpp, Wire-PODs,
//     abi/*_decl.hpp, Stempel-/Fingerprint-Flaechen bleiben unangetastet).
//   * KEIN brechender Assert fuer die noch duennen Genus-Anatomien -- ihre Luecken stehen unten als
//     benannter LUECKEN-Abschnitt, nicht als Compile-Bruch.
//
// SCHNITT C1 (2026-08-04, Bauplan-Dossier 20260803-DOSSIER-e24-fenster-bauplan.md Paragraf 3.1-C1 --
// weiterhin ABI-NEUTRAL, abi_adapter.hpp/Wire-PODs/abi/*_decl.hpp/Stempel-Flaechen UNBERUEHRT):
//   * OP-SCHNITTSTELLE als Concept-FAMILIE (Abschnitt "Op-Schnittstelle" unten) -- fuenf DISJUNKTE
//     Faehigkeits-Teilmengen, je Gattung genau eine. Siehe ABWEICHUNGS-VERMERK direkt darunter.
//   * ObservableAxis-/statistics-FORWARDING (Luecke L2): ObservableOrgan<Organ> uebersetzt die ORGAN-Ebene
//     (observe_all()) in die ACHSEN-Ebene (statistics()/snapshot_t) und forwardet dabei die Concept-Member
//     -- damit reicht ein als Sub-Organ gehaltenes Organ seine Beobachtung an das haltende Aggregat durch.
//   * OrganGuard-Vererbung an den fuenf realen Anatomien (Luecke L5) -- eigener Commit derselben Scheibe.
//   * Negativ-Compile-Fixture (Luecke L6) -- eigener Commit derselben Scheibe (ctest-sichtbar).
//
// LAYER-SCHNITT (bewusst, nicht zufaellig): dieser Header inkludiert NUR anatomy_base.hpp. Er darf weder
// die fuenf Gattungs-Anatomien noch observer_aggregate.hpp inkludieren -- die Anatomien sollen ihn selbst
// benutzen (sonst Include-Zyklus). Die Matrix-Beweise GEGEN die fuenf realen Anatomien liegen deshalb in
// den Test-TUs tests/unit/test_e24_c0_organ_concept.cpp (Halte-Flaeche) und
// tests/unit/test_e24_c1_organ_concept.cpp (Op-Familie + Forwarding gegen das echte ObservableAxis), die
// ueber die Bau-Bruecke
// builder/experiment_tree/genus_binding_traits.hpp (Spezialisierungen SearchAlgorithm/Adapter/Set/
// Sequence/View) und anatomy/container_framework.hpp (Slot-Pins 11/13/9/5) geht.
//
// MUSTER: C++23-Concept + CRTP-Wachen-Basis nach dem Repo-Goldstandard
// axes/io_dispatch/axis_io_strategy_base.hpp (protected CRTP-Ctor traegt die static_asserts).
// CT-statisch, zero-cost, dispatch-frei -- keine vtable, kein Runtime-Switch.
//
// @doku docs/architecture/20260803-e24_container_gattungs_abi_dossier.md (S12.1, Abschnitt 2.2/2.4)

#include "anatomy_base.hpp" // AnatomyGenus + AnatomyConcept (Identitaets-Flaeche jeder Anatomie)

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

namespace comdare::cache_engine::anatomy {

// ============================================================================================
// Der Vertrag
// ============================================================================================

/// OrganObservable<A> -- die BEOBACHTUNGS-Flaeche eines Organs: eine const, nicht-werfende
/// observe_all()-Abnahme. Am Ist erfuellen das alle fuenf Gattungs-Anatomien
/// (SearchAlgorithmAnatomy/SetAnatomy/SequenceAnatomy/AdapterAnatomy/ViewAnatomy) -- jede mit einem
/// EIGENEN Snapshot-Typ. Der Vertrag fordert deshalb bewusst NUR die Existenz der Abnahme, NICHT einen
/// gemeinsamen Snapshot-Typ: ein gemeinsamer Typ existiert heute nicht und waere hier geraten.
///
/// noexcept ist Teil des Vertrags (nicht Zierde): die Beobachtung darf einen Messlauf nie abbrechen.
template <class A>
concept OrganObservable = requires(A const& a) {
    { a.observe_all() } noexcept;
};

/// organ_snapshot_t<A> -- der Beobachtungs-Typ, den A::observe_all() liefert (je Gattung ein anderer).
/// Zweck: generischer Code kann den Snapshot benennen, ohne ihn zu vereinheitlichen.
template <class A>
    requires OrganObservable<A>
using organ_snapshot_t = decltype(std::declval<A const&>().observe_all());

/// OrganConcept<A> -- die gemeinsame Organ-Vertrags-Flaeche = IDENTITAET (AnatomyConcept:
/// composition_t / composition_name() / paper_id() / organ_count() / genus()) + BEOBACHTUNG
/// (OrganObservable). Genau diese Schnittmenge ist am heutigen Ist ueber alle fuenf Gattungen beweisbar.
///
/// Was hier BEWUSST NICHT drin steht (und warum), siehe LUECKEN-Abschnitt am Dateiende.
template <class A>
concept OrganConcept = AnatomyConcept<A> && OrganObservable<A>;

/// GenusOrgan<A, G> -- A ist ein Organ der Tier-Unterklasse G. Pinnt die Genus-Identitaet zur
/// Compile-Zeit; ein Slot, der ein Set-Organ erwartet, kann so kein Sequence-Organ annehmen.
/// (Die Cross-Genus-JOIN-Unmoeglichkeit der ABI-Adapter bleibt davon voellig unberuehrt.)
template <class A, AnatomyGenus G>
concept GenusOrgan = OrganConcept<A> && (A::genus() == G);

// ============================================================================================
// C1 -- DIE OP-SCHNITTSTELLE (gemeinsame Schnittstelle, fuenf disjunkte Faehigkeits-Teilmengen)
// ============================================================================================
//
// ABWEICHUNGS-VERMERK (M3-Doktrin "bindend sind die greps, nicht die Saetze"; Bauplan Paragraf 3.1-C1
// fordert wortwoertlich eine "gemeinsame Op-Schnittstelle"):
//   Am Ist existiert KEIN gemeinsames Op-Verb ueber die fuenf Gattungen. Erhebung 04.08. an den fuenf
//   Anatomie-Headern:
//     SearchAlgorithmAnatomy  -> KEINE Container-Verben (R5.B hat sie in den AnatomyExecutionContext
//                                ausgelagert, search_algorithm_anatomy.hpp:174-186); Op-Flaeche = die
//                                neun Organ-Accessoren (search_algo_organ() ... persistence_target_organ()).
//     SetAnatomy              -> insert / contains / erase        (set_anatomy.hpp:44-71)
//     SequenceAnatomy         -> push_back / at                   (sequence_anatomy.hpp:42-60)
//     AdapterAnatomy          -> push / pop_front / pop_back / front / back  (adapter_anatomy.hpp:176-212)
//     ViewAnatomy             -> bind / read                      (view_anatomy.hpp:39-53)
//   Selbst size() ist NICHT gemeinsam (SA hat keins), clear() erst recht nicht (SA und View haben keins).
//   Ein vereinheitlichtes Op-Verb waere deshalb ERFUNDEN, nicht erhoben -- und genau das ist untersagt.
//
// UMSETZUNG STATT ERFINDUNG: die "gemeinsame Op-Schnittstelle" ist hier EIN gemeinsamer NAME
// (OrganOpSurface) ueber FUENF disjunkten Faehigkeits-Concepts -- exakt die Form, die Gate G2 des
// Bauplans verlangt ("static_assert-Matrix 5 Genera x Concept-TEILMENGEN", Paragraf 4.2). Generischer
// Code fragt die Teilmenge per `if constexpr` ab und ruft die gattungs-eigenen Verben: compile-time,
// zero-cost, dispatch-frei, ohne vtable und ohne Runtime-Switch. Die Disjunktheit ist bewiesen
// (organ_op_family_count<A>() == 1 je Gattung), nicht behauptet.

/// OrganSized<A> -- Fuellstands-Abnahme. Am Ist: Set/Sequence/Adapter/View JA, SearchAlgorithm NEIN.
template <class A>
concept OrganSized = requires(A const& a) {
    { a.size() } -> std::convertible_to<std::size_t>;
};

/// OrganClearable<A> -- Leerung. Am Ist: Set/Sequence/Adapter JA, SearchAlgorithm/View NEIN
/// (View ist non-owning und kennt bewusst kein clear, view_anatomy.hpp:4).
template <class A>
concept OrganClearable = requires(A& a) { a.clear(); };

/// KeyedOrganOps<A> -- MENGEN-Verben (Set-Gattung, K-only, K=V).
template <class A>
concept KeyedOrganOps = requires(A& a, A const& ca, std::uint64_t k) {
    { a.insert(k) } -> std::convertible_to<bool>;
    { ca.contains(k) } -> std::convertible_to<bool>;
    { a.erase(k) } -> std::convertible_to<bool>;
};

/// IndexedOrganOps<A> -- INDIZIERTE Verben (Sequence-Gattung, V-indexed, wachsend).
template <class A>
concept IndexedOrganOps = requires(A& a, A const& ca, std::uint64_t i) {
    typename A::element_type;
    a.push_back(std::declval<typename A::element_type>());
    { ca.at(i) } -> std::convertible_to<std::optional<typename A::element_type>>;
};

/// AdaptedOrganOps<A> -- ADAPTER-Verben (Adapter-Gattung; die Disziplin FIFO/LIFO liegt in der
/// API-NUTZUNG front-vs-back, nicht in einer Achse -- adapter_anatomy.hpp:10-12).
template <class A>
concept AdaptedOrganOps = requires(A& a, A const& ca) {
    typename A::element_type;
    a.push(std::declval<typename A::element_type>());
    { a.pop_front() } -> std::convertible_to<std::optional<typename A::element_type>>;
    { a.pop_back() } -> std::convertible_to<std::optional<typename A::element_type>>;
    { ca.front() } -> std::convertible_to<std::optional<typename A::element_type>>;
    { ca.back() } -> std::convertible_to<std::optional<typename A::element_type>>;
};

/// BoundViewOrganOps<A> -- SICHT-Verben (View-Gattung, non-owning: fremden Puffer binden + lesen).
template <class A>
concept BoundViewOrganOps = requires(A& a, A const& ca, std::uint64_t i, std::size_t n) {
    typename A::element_type;
    a.bind(std::declval<typename A::element_type const*>(), n);
    { ca.read(i) } -> std::convertible_to<std::optional<typename A::element_type>>;
};

/// AxisOrganAccessOps<A> -- ACHSEN-ORGAN-Verben (SearchAlgorithm-Gattung). Ihre Op-Flaeche sind die
/// Organ-Accessoren, ueber die der Builder/abi_adapter die real gehaltenen Achsen-Organe treibt; der
/// Container-Zugriff selbst laeuft ueber den AnatomyExecutionContext (R5.B-Grenze).
template <class A>
concept AxisOrganAccessOps = requires(A& a, A const& ca) {
    a.search_algo_organ();
    a.node_type_organ();
    a.memory_layout_organ();
    a.serialization_organ();
    ca.search_algo_organ();
    { A::observable_axis_count() } -> std::convertible_to<std::size_t>;
};

/// organ_op_family_count<A>() -- wie viele der fuenf Faehigkeits-Teilmengen A erfuellt. Der
/// DISJUNKTHEITS-Beweis: fuer jede der fuenf Gattungs-Anatomien ist der Wert exakt 1 (Beleg in der
/// Test-TU tests/unit/test_e24_c1_organ_concept.cpp gegen die produktiven Typen).
template <class A>
[[nodiscard]] constexpr std::size_t organ_op_family_count() noexcept {
    std::size_t n = 0;
    if constexpr (KeyedOrganOps<A>) ++n;
    if constexpr (IndexedOrganOps<A>) ++n;
    if constexpr (AdaptedOrganOps<A>) ++n;
    if constexpr (BoundViewOrganOps<A>) ++n;
    if constexpr (AxisOrganAccessOps<A>) ++n;
    return n;
}

/// OrganOpSurface<A> -- DER gemeinsame Name: A ist ein Organ (OrganConcept) UND traegt eine der fuenf
/// Op-Teilmengen. Das ist die "gemeinsame Op-Schnittstelle" in der einzigen am Ist beweisbaren Form.
///
/// Bewusst NICHT Teil von OrganConcept: OrganConcept ist die HALTE-Flaeche (Identitaet + Beobachtung),
/// die ein Anatomie-Slot braucht, um ein fremdes Genus-Organ ueberhaupt aufnehmen zu koennen. Die
/// Op-Flaeche ist die TREIBE-Flaeche und gattungs-spezifisch. Wer sie in OrganConcept zoege, machte das
/// Halten vom Treiben abhaengig und verboete damit genau die Cross-Genus-Komposition, die das Fenster baut.
template <class A>
concept OrganOpSurface = OrganConcept<A> && (organ_op_family_count<A>() == 1);

// ============================================================================================
// CRTP-Wachen-Basis (Muster axis_io_strategy_base.hpp)
// ============================================================================================

/// OrganGuard<Derived> -- CRTP-Wache: wer sie erbt, wird bei der ERSTEN Konstruktion compile-hart
/// gegen OrganConcept geprueft. Zero-cost (leere Basis, protected Ctor, keine virtuelle Funktion).
///
/// C0-Stand: die fuenf Gattungs-Anatomien erben sie NOCH NICHT -- das Anheften ist eine Aenderung AN
/// den Anatomien und gehoert damit nicht in diesen ABI-neutralen Schritt. Bewiesen wird die Wache in
/// der Test-TU an einem eigenen Traeger-Typ.
template <class Derived>
class OrganGuard {
protected:
    OrganGuard() noexcept {
        static_assert(AnatomyConcept<Derived>,
                      "OrganGuard<Derived>: Derived erfuellt AnatomyConcept nicht (composition_t / "
                      "composition_name() / paper_id() / organ_count() / genus() sind Pflicht).");
        static_assert(OrganObservable<Derived>,
                      "OrganGuard<Derived>: Derived hat keine const-noexcept observe_all()-Abnahme.");
    }
};

// ============================================================================================
// C1 -- FORWARDING: ORGAN-Ebene (observe_all) -> ACHSEN-Ebene (statistics), Luecke L2
// ============================================================================================
//
// DAS PROBLEM (am Ist erhoben): observer_aggregate.hpp kennt zwei Ebenen, die sich nicht beruehren.
//   ACHSEN-Ebene: `ObservableAxis<A>` fordert `typename A::snapshot_t` + `a.statistics() -> snapshot_t`.
//                 Nur DAFUER sammelt SearchAlgorithmAnatomy::observe_all() Slot fuer Slot ein
//                 (search_algorithm_anatomy.hpp:68-110: `if constexpr (ObservableAxis<...>) agg.X = ...`).
//   ORGAN-Ebene:  OrganConcept fordert `a.observe_all()`.
// Ein Genus-Organ, das in einem Anatomie-Slot als SUB-ORGAN gehalten wird, ist damit messtechnisch
// STUMM: das haltende Aggregat fragt statistics(), das Sub-Organ bietet observe_all().
//
// DIE BRUECKE: ObservableOrgan<Organ> stellt genau die fehlende Uebersetzung her und FORWARDET dabei die
// Concept-Member (Memory-Kanon "Observable-Wrapper MUSS die Concept-Member forwarden") -- der Wrapper
// bleibt also selbst ein vollwertiges Organ und behaelt die Genus-Identitaet des Gewrappten.
// Wirkung ohne EINE Zeile Aenderung an SearchAlgorithmAnatomy: sobald ein Kompositions-Slot mit
// ObservableOrgan<X> belegt ist, zieht observe_all() den Sub-Organ-Snapshot automatisch in das Aggregat.
//
// LAYER: dieser Header nennt `ObservableAxis` NICHT beim Namen -- das Einbinden von observer_aggregate.hpp
// wuerde die C0-Layer-Regel (nur anatomy_base.hpp) brechen. Concept-Erfuellung ist strukturell: der
// Wrapper LIEFERT snapshot_t + statistics(). Der Beweis "ObservableAxis<ObservableOrgan<X>> == true"
// steht deshalb -- wie die 5-Genus-Matrix -- in der Test-TU, die beide Header sieht.

namespace organ_forward_detail {

template <class A>
concept HasKeyType = requires { typename A::key_type; };
template <class A>
concept HasValueType = requires { typename A::value_type; };
template <class A>
concept HasElementType = requires { typename A::element_type; };

/// OrganValueTypeForwarder -- reicht den WERT-Typ des gewrappten Organs durch, SOWEIT es einen hat.
/// Am Ist (Luecke L3): SearchAlgorithm traegt key_type/value_type, Sequence/Adapter/View element_type,
/// Set GAR KEINEN oeffentlichen (set_anatomy.hpp:81-82 privat). Deshalb konditional statt vereinheitlicht:
/// ein erfundener gemeinsamer Wert-Typ-Alias waere eine Luege ueber die Set-Gattung.
/// Die beiden Spezialisierungen schliessen sich gegenseitig aus (kein Ambiguitaets-Risiko).
template <class Organ>
struct OrganValueTypeForwarder {};

template <class Organ>
    requires(HasValueType<Organ> && !HasElementType<Organ>)
struct OrganValueTypeForwarder<Organ> {
    using value_type = typename Organ::value_type;
};

template <class Organ>
    requires(HasElementType<Organ> && !HasValueType<Organ>)
struct OrganValueTypeForwarder<Organ> {
    using element_type = typename Organ::element_type;
};

/// OrganKeyTypeForwarder -- dito fuer den Schluessel-Typ (am Ist nur SearchAlgorithm).
template <class Organ>
struct OrganKeyTypeForwarder {};

template <class Organ>
    requires HasKeyType<Organ>
struct OrganKeyTypeForwarder<Organ> {
    using key_type = typename Organ::key_type;
};

} // namespace organ_forward_detail

/// ObservableOrgan<Organ> -- die Beobachtungs-Huelle eines Genus-Organs fuer einen Achsen-Slot.
///
/// FORWARDET (das ist der Kern-Vertrag dieser Huelle):
///   * die Concept-Member der IDENTITAET: composition_t / composition_name() / paper_id() / genus() /
///     organ_count() -- der Wrapper erfuellt daher OrganConcept UND GenusOrgan<., genus des Gewrappten>.
///   * die BEOBACHTUNG in BEIDE Ebenen: observe_all() (Organ-Ebene) und statistics() (Achsen-Ebene),
///     beide const noexcept, beide derselbe snapshot_t.
///   * den WERT-/SCHLUESSEL-Typ, soweit vorhanden (konditional, s. o.).
///
/// FORWARDET BEWUSST NICHT: die gattungs-spezifischen Op-Verben (OrganOpSurface). Sie sind ueber
/// organ() erreichbar. Grund: die fuenf Verb-Saetze sind disjunkt (s. Op-Abschnitt); eine
/// Verb-Durchreichung waere fuenf handgeschriebene Mixins fuer eine Flaeche, die der Treiber ohnehin
/// gattungs-genau kennt. Das ist eine DEKLARIERTE Grenze, keine stille Luecke -- die Test-TU pinnt sie
/// (OrganOpSurface<ObservableOrgan<X>> == false, Ops laufen ueber .organ()).
///
/// ZERO-COST: leere Forwarder-Basen + leere CRTP-Wache (EBO), keine virtuelle Funktion, kein
/// Runtime-Switch. sizeof(ObservableOrgan<X>) == sizeof(X) -- in der Test-TU an den realen Organen gepinnt.
template <class Organ>
    requires OrganConcept<Organ>
class ObservableOrgan : public organ_forward_detail::OrganKeyTypeForwarder<Organ>,
                        public organ_forward_detail::OrganValueTypeForwarder<Organ>,
                        public OrganGuard<ObservableOrgan<Organ>> {
public:
    using organ_t       = Organ;
    using composition_t = typename Organ::composition_t; ///< forwarded (AnatomyConcept-Pflicht)
    using snapshot_t    = organ_snapshot_t<Organ>;       ///< ACHSEN-Ebene: der ObservableAxis-Vertrag

    ObservableOrgan() = default;
    explicit ObservableOrgan(Organ organ) : organ_(std::move(organ)) {}

    // -- forwarded Identitaet (AnatomyConcept) --
    static constexpr std::string_view composition_name() noexcept { return Organ::composition_name(); }
    static constexpr std::string_view paper_id() noexcept { return Organ::paper_id(); }
    static constexpr AnatomyGenus     genus() noexcept { return Organ::genus(); }
    static constexpr std::size_t      organ_count() noexcept { return Organ::organ_count(); }

    /// ACHSEN-Ebene: was das haltende Aggregat ruft (ObservableAxis-Vertrag, strukturell erfuellt).
    [[nodiscard]] snapshot_t statistics() const noexcept { return organ_.observe_all(); }

    /// ORGAN-Ebene: der Wrapper bleibt selbst ein Organ (OrganObservable-Vertrag).
    [[nodiscard]] snapshot_t observe_all() const noexcept { return organ_.observe_all(); }

    /// Zugriff auf das gehaltene Sub-Organ -- hier laufen die gattungs-eigenen Op-Verben.
    [[nodiscard]] Organ&       organ() noexcept { return organ_; }
    [[nodiscard]] Organ const& organ() const noexcept { return organ_; }

private:
    Organ organ_{};
};

// ============================================================================================
// Selbst-Beweise (compile-time; kein Raten, keine Fixture aus dem Anatomie-Bestand)
// ============================================================================================

namespace organ_concept_detail {

/// OrganArchetype -- der MINIMALE Modell-Typ des Vertrags. Er belegt, dass OrganConcept erfuellbar und
/// nicht ueberzogen ist (ein Concept, das niemand erfuellen kann, ist wertlos), OHNE dass dieser Header
/// eine der fuenf Anatomien inkludieren muss. Reines Beweismittel, kein Produktions-Organ.
struct ArchetypeComposition {};

struct ArchetypeSnapshot {};

struct OrganArchetype {
    using composition_t = ArchetypeComposition;

    static constexpr std::string_view composition_name() noexcept { return "OrganArchetype"; }
    static constexpr std::string_view paper_id() noexcept { return "P00 OrganArchetype (C0-Selbstbeweis)"; }
    static constexpr AnatomyGenus     genus() noexcept { return AnatomyGenus::SearchAlgorithm; }
    static constexpr std::size_t      organ_count() noexcept { return 0; }

    [[nodiscard]] ArchetypeSnapshot observe_all() const noexcept { return {}; }
};

/// GuardedArchetype -- belegt, dass OrganGuard<Derived> mit einem konformen Traeger uebersetzt.
/// Der explizit defaultete Ctor ist Absicht: als Aggregat waere der protected Basis-Ctor von aussen
/// nicht erreichbar -- genau die Kapselung, die OrganGuard herstellen soll.
struct GuardedArchetype : OrganArchetype, OrganGuard<GuardedArchetype> {
    GuardedArchetype() noexcept = default;
};

/// C1-Beweismittel: ein Archetyp MIT Op-Flaeche (Mengen-Verben). Belegt, dass die Faehigkeits-Concepts
/// erfuellbar sind und dass organ_op_family_count() zaehlt -- ohne eine Anatomie zu inkludieren.
struct KeyedArchetype : OrganArchetype {
    bool               insert(std::uint64_t) { return true; }
    [[nodiscard]] bool contains(std::uint64_t) const { return false; }
    bool               erase(std::uint64_t) { return false; }
};

/// C1-Beweismittel fuer die WERT-Typ-Durchreichung: je ein Traeger der beiden am Ist vorkommenden
/// Auspraegungen (SearchAlgorithm-Form key_type/value_type vs. Container-Form element_type).
struct KeyValueArchetype : OrganArchetype {
    using key_type   = std::uint64_t;
    using value_type = std::uint64_t;
};

struct ElementArchetype : OrganArchetype {
    using element_type = std::uint64_t;
};

} // namespace organ_concept_detail

// (a) Der Vertrag ist erfuellbar, und OrganConcept ist echt STAERKER als AnatomyConcept allein.
static_assert(AnatomyConcept<organ_concept_detail::OrganArchetype>);
static_assert(OrganObservable<organ_concept_detail::OrganArchetype>);
static_assert(OrganConcept<organ_concept_detail::OrganArchetype>);
static_assert(AnatomyConcept<organ_concept_detail::ArchetypeComposition> == false,
              "Selbstbeweis: die Identitaets-Flaeche ist eine echte Forderung, kein Freibrief");
static_assert(OrganObservable<organ_concept_detail::ArchetypeComposition> == false,
              "Selbstbeweis: die Beobachtungs-Flaeche ist eine echte Forderung");
static_assert(OrganConcept<int> == false, "Selbstbeweis: ein beliebiger Typ ist KEIN Organ");

// (b) organ_snapshot_t benennt genau den Rueckgabetyp der Abnahme (keine Vereinheitlichung).
static_assert(
    std::same_as<organ_snapshot_t<organ_concept_detail::OrganArchetype>, organ_concept_detail::ArchetypeSnapshot>);

// (c) Die Genus-Identitaet ist gepinnt: derselbe Typ ist NICHT Organ einer fremden Tier-Unterklasse.
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::SearchAlgorithm>);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::Set> == false);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::Sequence> == false);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::Adapter> == false);
static_assert(GenusOrgan<organ_concept_detail::OrganArchetype, AnatomyGenus::View> == false);

// (d) Die CRTP-Wache uebersetzt am konformen Traeger (Beweis, dass sie kein toter Buchstabe ist).
static_assert(OrganConcept<organ_concept_detail::GuardedArchetype>);

// (e) C1 -- die Op-Flaeche ist eine ZUSAETZLICHE Teilmenge, nicht Teil der Halte-Flaeche.
static_assert(organ_op_family_count<organ_concept_detail::OrganArchetype>() == 0,
              "Selbstbeweis: OrganConcept fordert KEINE Op-Verben (Halten != Treiben)");
static_assert(OrganOpSurface<organ_concept_detail::OrganArchetype> == false);
static_assert(KeyedOrganOps<organ_concept_detail::KeyedArchetype>);
static_assert(organ_op_family_count<organ_concept_detail::KeyedArchetype>() == 1,
              "Selbstbeweis: die Faehigkeits-Teilmengen sind disjunkt -- ein Traeger faellt in genau eine");
static_assert(OrganOpSurface<organ_concept_detail::KeyedArchetype>);
static_assert(IndexedOrganOps<organ_concept_detail::KeyedArchetype> == false);
static_assert(AdaptedOrganOps<organ_concept_detail::KeyedArchetype> == false);
static_assert(BoundViewOrganOps<organ_concept_detail::KeyedArchetype> == false);
static_assert(AxisOrganAccessOps<organ_concept_detail::KeyedArchetype> == false);
static_assert(OrganSized<organ_concept_detail::KeyedArchetype> == false,
              "Selbstbeweis: size() ist NICHT Teil der Mengen-Teilmenge (am Ist eine eigene Faehigkeit)");

// (f) C1 -- der Wrapper forwardet die Concept-Member: er ist selbst Organ und behaelt die Genus-Identitaet.
static_assert(OrganConcept<ObservableOrgan<organ_concept_detail::OrganArchetype>>);
static_assert(GenusOrgan<ObservableOrgan<organ_concept_detail::OrganArchetype>, AnatomyGenus::SearchAlgorithm>);
static_assert(GenusOrgan<ObservableOrgan<organ_concept_detail::OrganArchetype>, AnatomyGenus::Set> == false);
static_assert(std::same_as<organ_snapshot_t<ObservableOrgan<organ_concept_detail::OrganArchetype>>,
                           organ_concept_detail::ArchetypeSnapshot>);
// ... und die ACHSEN-Ebene strukturell (ObservableAxis wird hier bewusst nicht inkludiert -- Layer-Regel;
// der Beweis gegen das echte Concept steht in der Test-TU).
static_assert(
    requires(ObservableOrgan<organ_concept_detail::OrganArchetype> const& w) {
        typename ObservableOrgan<organ_concept_detail::OrganArchetype>::snapshot_t;
        { w.statistics() } -> std::same_as<typename ObservableOrgan<organ_concept_detail::OrganArchetype>::snapshot_t>;
    }, "Selbstbeweis: der Wrapper liefert die ACHSEN-Ebene (snapshot_t + statistics()) strukturell");

// (g) C1 -- die konditionale Wert-/Schluessel-Typ-Durchreichung trifft beide am Ist vorkommenden Formen
//     und ERFINDET nichts, wo es nichts gibt (Set-Fall: der Wrapper traegt dann ebenfalls keinen).
static_assert(std::same_as<typename ObservableOrgan<organ_concept_detail::KeyValueArchetype>::key_type, std::uint64_t>);
static_assert(
    std::same_as<typename ObservableOrgan<organ_concept_detail::KeyValueArchetype>::value_type, std::uint64_t>);
static_assert(organ_forward_detail::HasElementType<ObservableOrgan<organ_concept_detail::KeyValueArchetype>> == false);
static_assert(
    std::same_as<typename ObservableOrgan<organ_concept_detail::ElementArchetype>::element_type, std::uint64_t>);
static_assert(organ_forward_detail::HasValueType<ObservableOrgan<organ_concept_detail::ElementArchetype>> == false);
static_assert(organ_forward_detail::HasKeyType<ObservableOrgan<organ_concept_detail::ElementArchetype>> == false);
static_assert(organ_forward_detail::HasValueType<ObservableOrgan<organ_concept_detail::OrganArchetype>> == false,
              "Selbstbeweis: kein Wert-Typ am Organ -> keiner an der Huelle (Set-Fall, Luecke L3)");

// (h) C1 -- zero-cost: die drei leeren Basen (2 Forwarder + CRTP-Wache) kosten kein Byte.
static_assert(sizeof(ObservableOrgan<organ_concept_detail::OrganArchetype>) ==
                  sizeof(organ_concept_detail::OrganArchetype),
              "Selbstbeweis: EBO greift -- die Beobachtungs-Huelle ist groessen-neutral");

// ============================================================================================
// LUECKEN-LEDGER L1-L7 -- fortgeschrieben je Stufe (C0 erhoben 04.08., C1 fortgeschrieben 04.08.)
// ============================================================================================
//
// Diese Liste ist am Ist erhoben, nicht aus dem Plan abgeschrieben. Erledigte Punkte werden NICHT
// geloescht, sondern mit ihrem Vollzug fortgeschrieben -- damit bleibt nachlesbar, was wann warum fehlte.
//
// L1  [C1 GESCHLOSSEN -- MIT ABWEICHUNG] GEMEINSAME OP-SCHNITTSTELLE. C0-Befund: die fuenf Gattungen
//     tragen unvereinbare Verben, ein gemeinsamer Op-Nenner existiert am Ist NICHT. C1 erfindet ihn
//     NICHT, sondern liefert die einzige beweisbare Form: EIN gemeinsamer Name (OrganOpSurface) ueber
//     FUENF disjunkten Faehigkeits-Concepts (KeyedOrganOps / IndexedOrganOps / AdaptedOrganOps /
//     BoundViewOrganOps / AxisOrganAccessOps) + Disjunktheits-Beweis organ_op_family_count() == 1.
//     ABWEICHUNG gegenueber dem Bauplan-Wortlaut "gemeinsame Op-Schnittstelle" ist im Op-Abschnitt oben
//     literal deklariert (grep-Belege je Gattung). Gate-Form G2 ("5 Genera x Concept-TEILMENGEN") erfuellt.
//
// L2  [C1 GESCHLOSSEN] OBSERVABLEAXIS-/STATISTICS-FORWARDING. ObservableOrgan<Organ> uebersetzt die
//     ORGAN-Ebene (observe_all()) in die ACHSEN-Ebene (snapshot_t + statistics()) und forwardet die
//     Concept-Member (Identitaet + Beobachtung + Wert-/Schluessel-Typ). Damit reicht ein als Sub-Organ
//     gehaltenes Organ seine Beobachtung an das haltende Aggregat durch -- bei SearchAlgorithmAnatomy
//     OHNE Aenderung an ihr, weil ihr observe_all() bereits ueber ObservableAxis einsammelt.
//     REST (nicht C1): die VIER Container-Anatomien haben noch kein per-Achsen-Aggregat, das einsammeln
//     KOENNTE -- ihr observe_all() liefert je einen flachen Hand-POD. Das ist der Gegenstand von
//     Bauplan-C3 (reale Organ-Member + observe-Verdrahtung) und C6 (XxxObserverAggregate<N>).
//
// L3  [OFFEN -- bewusst] KEIN GEMEINSAMER WERT-TYP. Am Ist: SearchAlgorithmAnatomy traegt
//     key_type/value_type, SequenceAnatomy/AdapterAnatomy/ViewAnatomy tragen element_type, SetAnatomy
//     traegt gar keinen oeffentlichen Wert-Typ (key_t/value_t sind dort privat, set_anatomy.hpp:81-82).
//     Ein gemeinsamer Wert-Typ-Alias ist deshalb NICHT beweisbar und bleibt KEIN Teil von OrganConcept.
//     (Der Bauplan-Schnitt erwartete hier eine "value_type-Teilmenge" -- die Erhebung am Objekt
//     widerlegt das fuer Set.) C1-Antwort: KONDITIONALE Durchreichung im Wrapper statt Vereinheitlichung.
//
// L4  [OFFEN -- bewusst] KEIN GEMEINSAMER SNAPSHOT-TYP. observe_all() liefert je Gattung einen eigenen
//     Typ (ObserverAggregate<Composition> / SetObserverSnapshot / SequenceObserverSnapshot /
//     AdapterObserverSnapshot / ViewObserverSnapshot). organ_snapshot_t benennt ihn nur. Eine
//     Vereinheitlichung waere ein Wire-POD-Ereignis und gehoert damit in den b-Teil (C6), nicht hierher.
//
// L5  [OFFEN in diesem Commit] DIE FUENF ANATOMIEN ERBEN OrganGuard NOCH NICHT. Das Anheften aendert die
//     Anatomien; es ist im C1-Schnitt zulaessig und folgt als eigener Commit derselben Scheibe.
//
// L6  [OFFEN in diesem Commit] KEINE NEGATIV-COMPILE-FIXTURE. Das Repo kennt am Ist KEIN Muster fuer
//     erwartete Uebersetzungs-Fehler (Erhebung 04.08.: 0 Treffer fuer try_compile / WILL_FAIL in tests/
//     und cmake/). Die Negativ-Richtung ist bisher ueber ERFUELLBARE Negativ-Asserts gefuehrt
//     (Abschnitte a/c/e/f/g oben). Die echte Compile-Fehler-Probe folgt als eigener Commit derselben
//     Scheibe -- inklusive des dafuer noetigen, im Repo neuen ctest-Musters.
//
// L7  [C1 TEILWEISE -- Mechanismus gebaut, produktive Kompositionen offen] CROSS-GENUS-KOMPOSITION-ALS-
//     SUB-ORGAN. Mit ObservableOrgan kann ein Kompositions-Slot ein FREMDES Genus-Organ aufnehmen und
//     dessen Beobachtung fliesst in das haltende Aggregat (Beweis am produktiven SA-Pfad in der
//     Test-TU: node_type-Slot <- Sequence-Organ, observable_axis_count() == 1, echte Werte im Snapshot).
//     OFFEN bleibt der produktive Einbau in die realen Kompositionen/Registries (node_type <- Sequence/
//     Adapter, index_organization <- Set, queuing <- Adapter) -- Bauplan-C3. Die Cross-Genus-JOIN-
//     Unmoeglichkeit der ABI-Adapter (set_abi_adapter.hpp:18-21) bleibt davon unberuehrt und bestehen.

} // namespace comdare::cache_engine::anatomy
