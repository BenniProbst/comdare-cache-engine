#pragma once
// §23-QUARANTAENE (2026-07-18): variant-basierter Cluster -> fuer statische/Organ-Achsen VERBOTEN (Runtime-
// Overhead + Code-Bloat, §23). NICHT in den Live-Codegen-Pfad verdrahten; der Live-Weg sind monomorphe
// CRTP+Concept-Achsen. Guard: tests/unit/test_striktheit_metaprog_guard.cpp (Block D).
// ---------------------------------------------------------------------------
// TEST-ONLY (Marker 2026-07-17 F6): REV7-Baustein-Cluster
// (resolve_baustein.hpp -> baustein_variants.hpp -> algorithm_baustein.hpp).
// Konsumiert AUSSCHLIESSLICH von tests/unit/test_abi_interface.cpp (via
// #include + comdare::has_member_baustein) sowie clusterintern; KEIN Lib-/App-
// Konsument (repo-weiter grep verifiziert, auch symbol-level: resolve_baustein_t
// / eleven_axes_permutation / algorithm_axis / baustein:: nirgends sonst).
// NICHT test-only ist dagegen der ABI-Kern module_abi_v1.hpp (mehrere reale
// Lib-Konsumenten, u.a. experiment_demo/result_aggregator/workload_generator)
// -- NICHT anfassen. Rein additiver Marker; NICHT loeschen, Struktur unveraendert.
// ---------------------------------------------------------------------------
// resolve_baustein - Compile-time-Fallback fuer PRT-ART → CacheEngine (REV 7 §6.2 / V10.2)
//
// PRT-ART deklariert seine Bausteine in configuration_permutation_type.
// Wenn ein angefragter Bausteine-Typ im PRT-ART-Stack NICHT vorhanden ist
// → Compile-time-Fallback auf den CacheEngine-Stack.
//
// REV 7.6 V10.2 — operationalisiert mit konkreten Tag-Specializations
// fuer alle 11 Achsen aus baustein_variants.hpp. Stand der Fallback-
// Hierarchie:
//   1. Pruefling-Stack (Algo::template baustein_t<Tag>)        — Vorrang
//   2. cache_engine::baustein_t<Tag>::type (per Specialization) — Fallback
//   3. void                                                    — Default

#include "baustein_variants.hpp"

#include <type_traits>

namespace comdare {

// Concept: hat Algo eine member `baustein_t<Tag>`?
template <typename Algo, typename BausteineTag>
concept has_member_baustein = requires { typename Algo::template baustein_t<BausteineTag>; };

// CacheEngine-Default-Baustein-Lookup (Fallback-Quelle)
namespace cache_engine {
template <typename BausteineTag>
struct baustein_t {
    // Default-Wert: void → "kein default-Baustein" → Pruefling muss eigenen liefern
    using type = void;
};

// REV 7.6 V10.2 — Konkrete Specializations pro 11 Achsen
// (verwendet die Tag-Strukturen aus baustein_variants.hpp)
namespace tag {
struct PageAxisTag {};
struct NodeAxisTag {};
struct TraversalAxisTag {};
struct ValueHandleAxisTag {};
struct ConcurrencyAxisTag {};
struct AllocatorAxisTag {};
struct PrefetchAxisTag {};
struct TelemetryAxisTag {};
struct IsaAxisTag {};
struct LayoutAxisTag {};
struct ReclamationAxisTag {};
} // namespace tag

template <>
struct baustein_t<tag::PageAxisTag> {
    using type = baustein::PageAxis;
};
template <>
struct baustein_t<tag::NodeAxisTag> {
    using type = baustein::NodeAxis;
};
template <>
struct baustein_t<tag::TraversalAxisTag> {
    using type = baustein::TraversalAxis;
};
template <>
struct baustein_t<tag::ValueHandleAxisTag> {
    using type = baustein::ValueHandleAxis;
};
template <>
struct baustein_t<tag::ConcurrencyAxisTag> {
    using type = baustein::ConcurrencyAxis;
};
template <>
struct baustein_t<tag::AllocatorAxisTag> {
    using type = baustein::AllocatorAxis;
};
template <>
struct baustein_t<tag::PrefetchAxisTag> {
    using type = baustein::PrefetchAxis;
};
template <>
struct baustein_t<tag::TelemetryAxisTag> {
    using type = baustein::TelemetryAxis;
};
template <>
struct baustein_t<tag::IsaAxisTag> {
    using type = baustein::IsaAxis;
};
template <>
struct baustein_t<tag::LayoutAxisTag> {
    using type = baustein::LayoutAxis;
};
template <>
struct baustein_t<tag::ReclamationAxisTag> {
    using type = baustein::ReclamationAxis;
};
} // namespace cache_engine

// Resolve-Helper: liefere den Baustein-Typ entweder aus Algo (PRT-ART)
// oder per Fallback aus cache_engine
template <typename Algo, typename BausteineTag>
struct resolve_baustein {
private:
    template <typename A>
    static auto choose() {
        if constexpr (has_member_baustein<A, BausteineTag>) {
            return typename A::template baustein_t<BausteineTag>{};
        } else {
            return typename cache_engine::baustein_t<BausteineTag>::type{};
        }
    }

public:
    using type = decltype(choose<Algo>());
};

template <typename Algo, typename BausteineTag>
using resolve_baustein_t = typename resolve_baustein<Algo, BausteineTag>::type;

} // namespace comdare
