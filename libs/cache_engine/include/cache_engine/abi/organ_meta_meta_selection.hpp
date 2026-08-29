#pragma once
// abi/organ_meta_meta_selection.hpp -- E-10/#38a2: die Selektion der Organ-Meta-Metas JE COMPOSITION
// (Designplan 20260825-DESIGNPLAN-E10-38a2-ORG19 Schritt 1c + VERIFY-Fix FIX-1, 26.08.2026).
//
// ENTSCHEID D-1: Traeger der Selektion sind die 18 VORHANDENEN Slot-Aliase der Composition -- KEIN
// 19. Comp-Member (der prt-art-Vertrag bleibt 18 Aliase, compositions/prt_art_reference.hpp). Die Bindung
// Strategie-Typ -> Meta-Meta liegt HIER in abi/ und NICHT im Strategie-Header: organ_axes/ darf
// measurement/ nicht einbinden (Layer-Inversion), abi/ bindet bereits system_axes/ + mess_axes/ ein.
//
// FIX-1 (VERIFY-STEMPEL-IDENTITAET, Ein-Renderer-Doktrin / Mehrtraeger-Byteform): es gibt ZWEI Ausgabe-
// Formen mit EINER Grammatik-Quelle --
//   * organ_meta_meta_entries_for_variant<W>()  UNGEWRAPPT ("disk_io=code@1.0.0.c"; ';'-gefuegt bei
//     mehreren Gliedern). DIESE Form traegt die 1d-Tabelle (axis_variant_version_table); Schritt 4b
//     verkettert die Inner-Entries einer Comp und wrappt GENAU EINMAL (wrap_meta_meta_entries) -- sonst
//     entstuende im Mehrtraeger-Fall "[a=..];[b=..]" statt "[a=..;b=..]" (O-8-Schritt-12-Falle).
//   * organ_meta_meta_suffix_for_variant<W>()   GEWRAPPT ueber DENSELBEN Renderer wie der Mock-Pfad
//     (meta_meta_stamp_suffix_from_members) -- leere Selektion -> leerer Anhang (kein "[]").
//
// Die VOLLMENGEN-WACHE an der STEMPEL-STELLE (static_assert subsumiert an organ_stamp_line/4a) faehrt in
// Schritt 4; hier stehen die Wachen-Traits (subsumiert + duplikatfrei), die der k1-Test direkt beweist.
// BOOST-FREI (dieselbe Hermetik-Auflage wie meta_meta_stamp_suffix.hpp: Generator-Targets ohne Boost-Link);
// die mp11-Kreuzprobe gegen subsumes_v (meta_meta_identity.hpp) laeuft im Test, nicht hier.

#include <cache_engine/abi/meta_meta_stamp_suffix.hpp>          // EIN Renderer + wrap (detail) -- keine 2. Grammatik
#include <cache_engine/measurement/hardware_meta_meta_axis.hpp> // MetaMetaMembers + for_each_meta_meta
#include <organ_axes/organ_meta_meta/axis_disk_io_organ_meta_meta.hpp>              // ORG-19-IO (1a)
#include <organ_axes/persistence_target/axis_persistence_target_disk_writeback.hpp> // Traeger-Strategie

#include <string>
#include <type_traits>

namespace comdare::cache_engine::abi {

/// Bindung Strategie-Typ -> Meta-Meta-Glieder. Default: KEIN Traeger (leere Liste) -- damit ist jede
/// heutige Flotten-Strategie automatisch anhangfrei und der Default-Fall kostet 0 Byte.
template <class Strategy>
struct organ_meta_meta_binding {
    using type = ::comdare::cache_engine::measurement::MetaMetaMembers<>;
};

/// D-1: der EINE heutige Traeger. DiskWritebackTarget existiert vollstaendig, ist aber enabled=false
/// (Q-1 Fall B) -- genau dafuer ist die Selektion da: sobald der Baustein per option() aufgeschaltet
/// wird, stempelt sein Anhang, ohne dass eine Zeile Emitter-Code angefasst wird.
template <>
struct organ_meta_meta_binding<::comdare::cache_engine::persistence_target::DiskWritebackTarget> {
    using type = ::comdare::cache_engine::measurement::MetaMetaMembers<
        ::comdare::cache_engine::organ_meta_meta::DiskIoOrganMetaMeta>;
};

// Bindung == Single-Source (D-1): der Traeger-Wert der Meta-Meta ist EXAKT der Strategie-name().
static_assert(::comdare::cache_engine::persistence_target::DiskWritebackTarget::name() ==
                  ::comdare::cache_engine::organ_meta_meta::DiskIoOrganMetaMeta::kCarrierValues[0],
              "E-10 D-1: kCarrierValues[0] der Meta-Meta und DiskWritebackTarget::name() sind "
              "auseinandergelaufen -- die Comp-Bindung haette zwei Wahrheiten.");
static_assert(::comdare::cache_engine::organ_meta_meta::DiskIoOrganMetaMeta::kCarrierAxis ==
                  std::string_view{"persistence_target"},
              "E-10 D-1: der Traeger-Slot der IO-Meta-Meta ist persistence_target.");

namespace detail {

/// Pack-Verkettung ueber MetaMetaMembers (boost-frei; Spiegel des measurement-eigenen concat_members --
/// bewusst HIER dupliziert statt measurement::detail zu ziehen: fremde detail-Namespaces sind kein API).
template <class... Lists>
struct omm_concat {
    using type = ::comdare::cache_engine::measurement::MetaMetaMembers<>;
};
template <class... As>
struct omm_concat<::comdare::cache_engine::measurement::MetaMetaMembers<As...>> {
    using type = ::comdare::cache_engine::measurement::MetaMetaMembers<As...>;
};
template <class... As, class... Bs, class... Rest>
struct omm_concat<::comdare::cache_engine::measurement::MetaMetaMembers<As...>,
                  ::comdare::cache_engine::measurement::MetaMetaMembers<Bs...>, Rest...>
    : omm_concat<::comdare::cache_engine::measurement::MetaMetaMembers<As..., Bs...>, Rest...> {};

/// Vorkommens-Zaehler eines Typs in einem Pack (Traeger der Duplikatfrei-Wache, FIX-1 (iii)).
template <class X, class... Ms>
inline constexpr std::size_t omm_zaehl_vorkommen = (std::size_t{0} + ... + (std::is_same_v<X, Ms> ? 1u : 0u));

} // namespace detail

/// organ_meta_metas_of_t<Comp> -- die Meta-Meta-Glieder einer Composition, verkettet ueber die 18
/// benannten Slot-Aliase in kCompositionAxisNames-Ordnung (== organ_stamp_line-Entry-Ordnung). Damit ist
/// die Anhang-Reihenfolge im (deklarierten, R-11) Mehrtraeger-Fall die Slot-Ordnung -- kein zweiter
/// Ableitungsweg.
template <class Comp>
using organ_meta_metas_of_t =
    typename detail::omm_concat<typename organ_meta_meta_binding<typename Comp::search_algo>::type,
                                typename organ_meta_meta_binding<typename Comp::cache_traversal>::type,
                                typename organ_meta_meta_binding<typename Comp::mapping>::type,
                                typename organ_meta_meta_binding<typename Comp::path_compression>::type,
                                typename organ_meta_meta_binding<typename Comp::node_type>::type,
                                typename organ_meta_meta_binding<typename Comp::memory_layout>::type,
                                typename organ_meta_meta_binding<typename Comp::allocator>::type,
                                typename organ_meta_meta_binding<typename Comp::prefetch>::type,
                                typename organ_meta_meta_binding<typename Comp::concurrency>::type,
                                typename organ_meta_meta_binding<typename Comp::serialization>::type,
                                typename organ_meta_meta_binding<typename Comp::value_handle>::type,
                                typename organ_meta_meta_binding<typename Comp::index_organization>::type,
                                typename organ_meta_meta_binding<typename Comp::io_dispatch>::type,
                                typename organ_meta_meta_binding<typename Comp::migration_policy>::type,
                                typename organ_meta_meta_binding<typename Comp::filter>::type,
                                typename organ_meta_meta_binding<typename Comp::queuing_q1>::type,
                                typename organ_meta_meta_binding<typename Comp::queuing_q2>::type,
                                typename organ_meta_meta_binding<typename Comp::persistence_target>::type>::type;

/// Teilmengen-Wache (boost-freie Schwester von subsumes_v, meta_meta_identity.hpp -- der k1-Test haelt
/// beide per Kreuzprobe deckungsgleich): traegt die Vollmenge jedes Glied der Comp-Selektion?
template <class Voll, class Teil>
struct organ_meta_meta_subsumiert;
template <class... V, class... T>
struct organ_meta_meta_subsumiert<::comdare::cache_engine::measurement::MetaMetaMembers<V...>,
                                  ::comdare::cache_engine::measurement::MetaMetaMembers<T...>>
    : std::bool_constant<(::comdare::cache_engine::measurement::MetaMetaMembers<V...>::template contains<T> && ...)> {};

/// FIX-1 (iii): zwei Slots duerfen dasselbe Glied nicht doppelt stempeln -- die CT-Duplikatfrei-Wache am
/// Trait. Schritt 4 (4a) haengt sie zusammen mit der Subsumtions-Wache an die Stempel-Stelle.
template <class Members>
struct organ_meta_metas_sind_duplikatfrei;
template <class... Ms>
struct organ_meta_metas_sind_duplikatfrei<::comdare::cache_engine::measurement::MetaMetaMembers<Ms...>>
    : std::bool_constant<((detail::omm_zaehl_vorkommen<Ms, Ms...> == 1u) && ...)> {};

/// FIX-1 (i): die UNGEWRAPPTE Entry-Form der Glieder EINER Varianten-Strategie ("disk_io=code@1.0.0.c";
/// "" = kein Traeger). DIESE Form traegt die 1d-Tabelle; der Renderer je Entry ist DERSELBE wie im
/// Mock-Pfad (detail::render_meta_meta_entry -- eine Grammatik, ein Engpass, beide Versions-Wachen).
template <class W>
[[nodiscard]] inline std::string organ_meta_meta_entries_for_variant() {
    std::string inner;
    ::comdare::cache_engine::measurement::for_each_meta_meta(typename organ_meta_meta_binding<W>::type{},
                                                             [&inner](auto tag) {
                                                                 using M = typename decltype(tag)::type;
                                                                 if (!inner.empty()) inner += ';';
                                                                 inner += detail::render_meta_meta_entry<M>();
                                                             });
    return inner;
}

/// FIX-1 (ii): der EINE oeffentliche Wrap-Punkt fuer 4b (statt detail:: zu ziehen). Leerer Inhalt ->
/// leerer Anhang (kein "[]") -- dieselbe Stelle wrap_meta_meta_group, keine zweite Klammer-Grammatik.
[[nodiscard]] inline std::string wrap_meta_meta_entries(std::string const& inner) {
    return detail::wrap_meta_meta_group(inner);
}

/// Der GEWRAPPTE Anhang einer Varianten-Strategie -- byte-gleich zum Mock-Pfad-Renderer (EIN Renderer,
/// Plan 1c): "[disk_io=code@1.0.0.c]" fuer den IO-Traeger, "" fuer alle anderen.
template <class W>
[[nodiscard]] inline std::string organ_meta_meta_suffix_for_variant() {
    return meta_meta_stamp_suffix_from_members<typename organ_meta_meta_binding<W>::type>();
}

} // namespace comdare::cache_engine::abi
