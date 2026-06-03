#pragma once
// L-74b (2026-06-03, Doc 27 §3) — build_variant_definition_reader: zieht die per-Knoten getragene Build-Achsen-
// DEFINITION (page_type/09b/12) in einen NodeValue. Gegenstück zu node_value_measurement.hpp (BR-3), das den
// Laufzeit-Observer (search_algo/allocator) zieht — HIER die DEFINITION der 3 Build-KONSTANTEN-Achsen.
//
// Die 3 Achsen haben keinen Laufzeit-Zustand (alle Properties static constexpr, Doc 27 §3) → ein „Observer" wäre
// Etikettenschwindel. Stattdessen trägt der GEMESSENE Knoten ihre reale, ABI-gezogene Build-Identität
// (BuildVariantDefinitionV1) + markiert build_def_real=true; ungemessene Knoten bleiben build_def_real=false
// (SPARSE-Kontrast, exakt wie observer_real). C++23, header-only.

#include "experiment_tree.hpp"                       // NodeValue
#include "anatomy/build_variant_definition.hpp"      // build_variant_definition<PT,SE,HW>()

namespace comdare::cache_engine::builder::experiment {

/// read_build_variant<PT,SE,HW>(nv) — füllt die per-Knoten Build-Achsen-Definition (page_type/simd/hw) aus den
/// static-constexpr-Properties der 3 Build-Achsen (compile-time, KEIN Treiben) + markiert build_def_real.
/// Für GEMESSENE Knoten aufzurufen; ungemessene behalten build_def_real=false (read-only Definition, kein Observer).
template <class PT, class SE, class HW>
inline void read_build_variant(NodeValue& nv) noexcept {
    nv.build_def      = ::comdare::cache_engine::anatomy::build_variant_definition<PT, SE, HW>();
    nv.build_def_real = true;
}

}  // namespace comdare::cache_engine::builder::experiment
