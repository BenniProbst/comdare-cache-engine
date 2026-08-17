#pragma once
// A8-S5 Familie 01c (Fassaden-Scheibe, 2026-08-05) -- die EINE Naht, an der ein Such-Organ die
// Allokator-Wahl SEINER KOMPOSITION uebernimmt.
//
// @topic traversal @achse 03a @schicht composable (Organ-statt-Tier)
//
// Dossier: docs/architecture/20260803-a8_f2_benchmarking_schnitt_soll_design.md Abschn. 3.4
// (Schnitt-Regel: Speicher NUR ueber das Allokator-Achsen-Interface, inline CT, kein std::variant).
// Owner-KERN: LEDGER 04.08.2026 abend-11 -- "Option B strikt: multiple Allokatoren liegen HINTER dem
// Allokator-Achsen-Interface (Metaprogrammierung des eingebauten Organ-Algorithmus)".
//
// **DAS PROBLEM, DAS DIESE DATEI LOEST.** Ein Registry-Organ traegt seinen Allokator bisher FEST
// (benannter Achsen-Default ExgenAllocator, Praezedenz 01d/01a). Die Komposition darueber waehlt aber
// eine EIGENE Strategie (compositions/art_paper_binding_reference.hpp: MimallocAllocator). Solange das
// Organ die Wahl nicht uebernimmt, misst die T6-Spalte den Speicher der Komposition NUR fuer den Store,
// nicht fuer das Organ -- der Achsen-Default waere eine stille zweite Strategie im selben Tier.
//
// **WARUM DAS NICHT MIT EINEM HANDLE GEHT (Wiedergaenger-Kommentar -- diese Idee kommt wieder, und sie
// ist jedes Mal falsch).** Die naheliegende Loesung ist ein type-erased Allokator-Handle (oder
// std::pmr::memory_resource) im Organ: EIN Organ-Typ, Strategie zur Laufzeit gesteckt, keine
// Template-Vermehrung, Registry-XML voellig unberuehrt. Verworfen, und zwar nicht aus Geschmack:
//   (a) Jede Allokation liefe dann ueber eine Funktions-Zeiger-/vtable-Indirektion. Diese Organe sind
//       der T6-MESSPFAD -- abi_adapter instanziiert sie im heissen Segment-1-Einfuege-Pfad. Eine
//       Indirektion je Allokation misst sich selbst mit und verfaelscht genau die Groesse, wegen der
//       die Achse existiert.
//   (b) Es bricht die CT-Doktrin (Auflage 5: kein Runtime-Switch, kein std::variant, CT-Dispatch per
//       CRTP/Concept). Die Achse ist eine COMPILE-ZEIT-Achse; ein Laufzeit-Steckplatz waere die
//       Rueckkehr genau des Musters, das die Metaprogrammierung ersetzt hat.
//   (c) Der PmrResourceAdapter-Weg traegt zusaetzlich den offenen Wurf-Vertrag (Posten 71: reicht den
//       Strategie-nullptr ungeprueft durch, waehrend StdAllocatorAdapter ihn seit Posten 64 in
//       std::bad_alloc uebersetzt). Die Cores nutzen deshalb AUSSCHLIESSLICH den StdAllocatorAdapter.
// Der Preis der CT-Loesung ist ein zweiter Typ je gebundener Strategie -- den zahlt die Zwei-Ebenen-
// Konstruktion (namens-stabile Fassade + Rebound-Leaf), damit die Registry-XML sich um NULL Byte bewegt.
//
// **DIE DREI STUFEN dieses Traits** (in genau dieser Reihenfolge, weil die erste die golden-Kette rettet):
//   LEVEL 0 -- IDENTITAET: die Komposition will exakt die Strategie, an die das Organ ohnehin gebunden
//     ist (`is_same_v<A, typename S::allocator_type>`). Dann ist das Ergebnis S SELBST. Kein
//     Rebound-Typ, kein Typ-Shift, kein neuer Symbolname. Das ist der Default-/golden-Pfad und der
//     Grund, warum der golden-Beweis dieser Scheibe ein is_same-Einzeiler sein darf statt eines
//     Mess-Laufs (Muster 234-V: self-proving Level-0-Neutralitaet, organ_for_search_algo_shaped.hpp:85-100).
//   LEVEL 1 -- REBIND: das Organ ist migriert (traegt `rebind_allocator<A2>`) und die Komposition will
//     eine ANDERE Strategie. Dann liefert der Trait den Rebound-Leaf des Organs.
//   LEVEL 2 -- IDENTITAET (nicht migriert): das Organ traegt noch keinen Rebind. Dann bleibt es, wie es
//     ist. Das ist KEINE Regression nach der Owner-Definition (abend-11: Regression ist ein Organ OHNE
//     Achsen-Zuordnung; ein Organ am benannten Achsen-DEFAULT ist zugeordnet) -- es ist der deklarierte
//     Zwischenstand, der die Familie inkrementell migrierbar macht, statt 16 Organe in EINEM Commit zu
//     drehen.
//
// KEIN std::variant, kein Runtime-Switch, keine virtuelle Funktion -- reine CT-Komposition (Auflage 5).
// BEWUSST OHNE boost/mp11: dieser Header reist ueber die Registry-Organe in sehr viele Uebersetzungs-
// einheiten; jede zusaetzliche Abhaengigkeit waere eine neue Link-Kante (s. den 05.08.-Hotfix
// test_conformance_gate/Boost::mp11 -- Klasse "lokale Voll-Bau-Luecken = falsches Gruen").

#include <type_traits>

namespace comdare::cache_engine::lookup::composable {

/// Traegt das Organ ueberhaupt eine Allokator-Achsen-Bindung? (Alle Form-B-Organe tun das; Form-A-
/// Organe -- heap-frei -- tun es nicht und brauchen es auch nicht.)
template <class S>
concept HasAxisAllocatorType = requires { typename S::allocator_type; };

/// Ist das Organ auf eine FREMDE Strategie umbindbar? Das ist der Ausweis der Migration: ein Organ,
/// das den Zwei-Ebenen-Schnitt hat, exponiert seinen Rebound-Leaf unter diesem Namen.
template <class S, class A>
concept AllocatorRebindableSearchAlgo = requires { typename S::template rebind_allocator<A>; };

/// Level 0: Organ und Komposition wollen dieselbe Strategie -> das Organ bleibt, wie es ist.
template <class S, class A>
concept SearchAlgoAlreadyBoundTo = HasAxisAllocatorType<S> && std::is_same_v<A, typename S::allocator_type>;

/// Wird ueberhaupt umgebunden? NUR wenn das Organ migriert IST **und** die Komposition eine ANDERE
/// Strategie will. Der zweite Teil ist der Level-0-Vorrang: ohne ihn laege im Default-Pfad
/// `Rebound<ExgenAllocator>` statt der Fassade -- ein anderer Typ mit anderem Symbolnamen auf dem
/// golden-Pfad, obwohl semantisch dasselbe. Genau das darf nicht sein.
template <class S, class A>
inline constexpr bool search_algo_needs_rebind_v =
    AllocatorRebindableSearchAlgo<S, A> && !SearchAlgoAlreadyBoundTo<S, A>;

namespace detail {

// Die Fallunterscheidung laeuft ueber einen BOOL-Parameter, nicht ueber conditional_t: conditional_t
// verlangt BEIDE Zweige als gebildeten Typ-Ausdruck, und `typename S::template rebind_allocator<A>`
// existiert bei nicht-migrierten Organen schlicht nicht. Von den beiden Spezialisierungen wird immer
// genau eine instanziiert -- der Rebound-Typ wird also nur dort ueberhaupt benannt, wo es ihn gibt.
template <class S, class A, bool kRebind>
struct search_algo_for_composition {
    using type = S; // Level 0 (schon gebunden) UND Level 2 (nicht migriert) -- beide: Identitaet.
};

template <class S, class A>
struct search_algo_for_composition<S, A, true> {
    using type = typename S::template rebind_allocator<A>; // Level 1
};

} // namespace detail

/// DIE Naht: welcher Such-Organ-Typ gilt in einer Komposition mit der Allokator-Strategie A?
/// Der Genus-Erst-Instanziierungs-Punkt (abi_adapter) bildet ihn EINMAL und benutzt ihn ueberall --
/// die abend-6-Ausnahme der generalisierten Schnitt-Regel.
template <class S, class A>
using search_algo_for_composition_t =
    typename detail::search_algo_for_composition<S, A, search_algo_needs_rebind_v<S, A>>::type;

/// Der SERIALISIERUNGS-Schluessel eines Organs ist sein name(). Die T6-Wahl darf dort NIE hineinlecken:
/// eine Komposition mit mimalloc und eine mit exgen tragen DENSELBEN Organ-Namen, sonst driften
/// binary_id-/serialize-Pfad und Registry-XML gegen die Allokator-Achse. Dieses Praedikat ist die
/// Wache dazu -- je Organ EINMAL als static_assert instanziiert.
template <class S, class A>
inline constexpr bool search_algo_name_is_allocator_invariant_v =
    (S::name() == search_algo_for_composition_t<S, A>::name());

/// Der Ebenen-Ausweis: ein Rebound-Leaf traegt `axis03a_rebound_tag`. Die IDENTITAETS-Ebene (die
/// namens-stabile Fassade, die in der Registry-XML steht und deren type_name auf die Reise geht)
/// darf ihn NIE tragen -- sonst drehen Emitter-Typnamen und Fixtures.
template <class S>
concept IsReboundSearchAlgoLeaf = requires { typename S::axis03a_rebound_tag; };

} // namespace comdare::cache_engine::lookup::composable
