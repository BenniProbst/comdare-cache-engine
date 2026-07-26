// measurement/meta_meta_identity.hpp -- V1-Vertrag fuer Lane C (Lane A, Paket A1, 26.07.2026).
//
// AUFTRAG A1, WORTGETREU: "V1-Neuschnitt fuer Lane C: `meta_metas` als mp_list-TYPliste + constexpr
// subsumes, NICHT `std::span<MetaMetaDescriptor const>` (Layer-Modell D3: Meta-Metas sind Typen,
// keine Daten)."
//
// WARUM DER NEUSCHNITT: ein `std::span<MetaMetaDescriptor const>` haette die Meta-Metas zu DATEN
// gemacht -- Laufzeit-Deskriptoren, ueber die man iteriert. Damit waere die Identitaets-Pruefung eine
// Laufzeit-Suche und die Achsen waeren keine Typen mehr, sondern Eintraege. Das verstiesse gegen die
// Direktive "Metaprogrammierung compile-time zwingend" und gegen Abschnitt 1.4 des Auftrags
// (Meta-Metas sind VOLLE CT-Haupt-Achsen-TYPEN mit eigenen RT-Unter-Achsen). Als Typliste ist die
// Zugehoerigkeit eine Compile-Zeit-Eigenschaft und kostet zur Laufzeit exakt nichts.
//
// KEIN std::variant (Direktive feedback_no_std_variant_static_axes_bloat): die Menge ist ein
// variadisches Typ-Pack, keine getaggte Summe.
//
// IDENTITAETS-DOKTRIN (Owner-KERN R-E, Session-Doc 26.07. Abschnitt 2): Identitaet ist NUR AUFWAERTS
// kompatibel, Basis ist CPU-only. GPU einbauen -> CPU-only-Programme laufen weiter, GPU-CPU zusaetzlich;
// GPU ausbauen -> GPU-CPU-Programme laufen NICHT mehr. Formal: eine Maschine mit Meta-Meta-Satz S kann
// genau die Programme fahren, deren Satz T eine TEILMENGE von S ist. Das ist die Halbordnung, die
// `subsumes` unten ausdrueckt -- exakt dieselbe Teilmengen-Semantik, die simd_build_gate.hpp
// (admit_organ_on_machine) heute schon auf SimdFeatureFlag anwendet, nur auf Typ-Ebene gehoben.
//
// A1-AUFLAGE: INERT. Dieser Header hat KEINEN Konsumenten und aendert kein Byte. Lane C (C-1/C-2)
// baut darauf auf; das Scharfschalten des Gates ist ausdruecklich NACH dem Trigger (Q-11).

#pragma once

#include <boost/mp11.hpp>

#include <cstddef>
#include <type_traits>

namespace comdare::cache_engine::measurement {

namespace mp = boost::mp11;

/// Der Meta-Meta-Satz EINER Identitaet: eine Typliste, keine Daten-Sequenz.
/// Basis-Identitaet (CPU-only) ist die LEERE Menge -- sie ist Teilmenge jeder anderen und laeuft daher
/// ueberall. Genau das ist die Owner-Aussage "Basis = CPU-only (baut/fuehrt langsame Programme aus)".
template <class... Ms>
using MetaMetaSet = mp::mp_list<Ms...>;

/// Die Basis-Identitaet, explizit benannt (lesbarer als mp_list<> an den Verwendungsstellen).
using CpuOnlyIdentity = MetaMetaSet<>;

namespace detail {
/// Praedikat-Fabrik: "ist X in Haystack?" -- als mp11-taugliches Quoted-Metafunction-Argument.
template <class Haystack>
struct contained_in {
    template <class X>
    using fn = mp::mp_contains<Haystack, X>;
};
} // namespace detail

/// subsumes<Machine, Program> -- traegt die Maschinen-Identitaet das Programm?
///
/// TRUE genau dann, wenn JEDER Meta-Meta des Programms auch auf der Maschine vorhanden ist
/// (Program ist Teilmenge von Machine). Damit gilt per Konstruktion:
///   - subsumes<S, CpuOnlyIdentity> == true fuer JEDES S   (Basis laeuft ueberall)
///   - subsumes<S, S>               == true                (reflexiv)
///   - GPU ausgebaut => subsumes<CpuOnly, {Gpu}> == false  (Abwaerts NICHT kompatibel)
/// Die Relation ist reflexiv und transitiv, also eine Halbordnung -- sie ist NICHT symmetrisch, und
/// genau diese Asymmetrie IST die "nur aufwaerts"-Doktrin.
template <class Machine, class Program>
inline constexpr bool subsumes_v = mp::mp_all_of_q<Program, detail::contained_in<Machine>>::value;

/// Concept-Schreibweise derselben Relation (fuer requires-Klauseln an Freigabe-Nahten).
template <class Program, class Machine>
concept RunnableOn = subsumes_v<Machine, Program>;

/// Anzahl der Meta-Metas einer Identitaet (Diagnose/Stempel-Reflexion; emergent, nicht deklariert).
template <class Set>
inline constexpr std::size_t meta_meta_count_v = mp::mp_size<Set>::value;

// ---------------------------------------------------------------------------------------------
// HALBORDNUNGS-WACHEN: die Doktrin ist hier compile-time zementiert, nicht nur kommentiert.
// Die Platzhalter-Typen sind LOKAL und dienen ausschliesslich dem Beweis (keine echten Achsen).
// ---------------------------------------------------------------------------------------------
namespace detail {
struct ProofSimd {};
struct ProofGpu {};
using ProofCpuOnly  = MetaMetaSet<>;
using ProofSimdOnly = MetaMetaSet<ProofSimd>;
using ProofSimdGpu  = MetaMetaSet<ProofSimd, ProofGpu>;
} // namespace detail

// Reflexiv: jede Identitaet traegt sich selbst.
static_assert(subsumes_v<detail::ProofCpuOnly, detail::ProofCpuOnly>);
static_assert(subsumes_v<detail::ProofSimdGpu, detail::ProofSimdGpu>);
// Basis laeuft ueberall (die Kern-Aussage "CPU-only-Programme laufen weiter").
static_assert(subsumes_v<detail::ProofSimdGpu, detail::ProofCpuOnly>);
static_assert(subsumes_v<detail::ProofSimdOnly, detail::ProofCpuOnly>);
// AUFWAERTS ja, ABWAERTS nein -- die Asymmetrie, die die Doktrin ausmacht.
static_assert(subsumes_v<detail::ProofSimdGpu, detail::ProofSimdOnly>);
static_assert(!subsumes_v<detail::ProofSimdOnly, detail::ProofSimdGpu>,
              "Abwaerts-Kompatibilitaet waere ein Doktrin-Bruch: GPU ausgebaut -> GPU-Programm laeuft NICHT.");
static_assert(!subsumes_v<detail::ProofCpuOnly, detail::ProofSimdOnly>);
// Transitiv (Halbordnung): CpuOnly <= SimdOnly <= SimdGpu.
static_assert(subsumes_v<detail::ProofSimdGpu, detail::ProofSimdOnly> &&
              subsumes_v<detail::ProofSimdOnly, detail::ProofCpuOnly> &&
              subsumes_v<detail::ProofSimdGpu, detail::ProofCpuOnly>);
// Emergente Groesse.
static_assert(meta_meta_count_v<detail::ProofCpuOnly> == 0);
static_assert(meta_meta_count_v<detail::ProofSimdGpu> == 2);

} // namespace comdare::cache_engine::measurement
