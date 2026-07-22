#pragma once
// profile_facade/g1_binary_version_stamp.hpp -- K7b-4 (Section 62-B, B6-Auflage): der G1-Je-Binary-Selbst-Stempel des
// Treiber-Binary (Planer- + CEB-Rolle -- EIN Binary, zwei Rollen). Header-only, rein-lesend, KEIN Bau.
//
// Vier gelabelte Zeilen, jede non-empty:
//   planner@X.Y.Z isa=<isa> os=<os>   -- der Planer-Selbst-Stempel (planner_version_stamp(), selbst-gelabelt)
//   ceb-contract=<MAJOR>.<minor>      -- die CEB-Contract-Version (ABI-Major automatisch, codegen-Minor manuell)
//   build-type=<Debug|Release>        -- die Compile-Einstellung (immer non-empty; Default Release)
//   build-version=<system-suffix>     -- die volle System-Achsen-build_version (+ext/+cxx/+opt/+ceb[/+bt/...])
//
// Der system_build_version-Anteil (Single-Source system_axes_version_suffix, TU-lokal im Facade-.cpp) wird
// HEREINGEREICHT, damit dieser Header .cpp-frei + leicht unit-testbar bleibt (kein Link gegen die schwere Facade-Lib).
// Rein additiv, golden-/binary_id-neutral (ein reiner --version-Ausgabe-Stempel, kein Cache-Key, kein Bau-Input).

#include <cache_engine/abi/anatomy_module_abi_v1_decl.hpp> // COMDARE_ANATOMY_ABI_MAJOR + kCebContractCodegenMinor
#include <profile_facade/build_type_stamp.hpp>             // build_type_version_suffix()
#include <profile_facade/planner/planner_version.hpp>      // planner_version_stamp()

#include <string>
#include <string_view>

namespace comdare::cache_engine::builder::profile_facade {

/// g1_ceb_contract_version() -- "MAJOR.minor" der CEB-Contract-Ebene (ABI-Major automatisch ueber
/// COMDARE_ANATOMY_ABI_MAJOR, codegen-Minor manuell ueber kCebContractCodegenMinor). Immer non-empty (enthaelt '.').
[[nodiscard]] inline std::string g1_ceb_contract_version() {
    return std::to_string(COMDARE_ANATOMY_ABI_MAJOR) + "." +
           std::to_string(::comdare::cache_engine::abi::kCebContractCodegenMinor);
}

/// g1_build_type_label() -- "Debug" bei COMDARE_BUILD_TYPE == "Debug", sonst "Release". Immer non-empty (macht das
/// leere build_type_version_suffix()-Default explizit sichtbar).
[[nodiscard]] inline std::string g1_build_type_label() {
    return ::comdare::cache_engine::thesis_lazy::build_type_version_suffix().empty() ? std::string{"Release"}
                                                                                     : std::string{"Debug"};
}

/// g1_binary_version_block(system_build_version) -- die vier gelabelten Stempel-Zeilen des Treiber-Binary (je "\n"-
/// terminiert). system_build_version = die volle System-Achsen-build_version (Single-Source system_axes_version_suffix;
/// enthaelt bereits +ceb/+bt/+ext/+cxx/+opt) -- hereingereicht, damit der Header .cpp-frei bleibt.
[[nodiscard]] inline std::string g1_binary_version_block(std::string_view system_build_version) {
    std::string out;
    out += ::comdare::cache_engine::planner::planner_version_stamp(); // "planner@X.Y.Z isa=.. os=.." (selbst-gelabelt)
    out += '\n';
    out += "ceb-contract=" + g1_ceb_contract_version() + "\n";
    out += "build-type=" + g1_build_type_label() + "\n";
    out += "build-version=";
    out += system_build_version;
    out += '\n';
    return out;
}

} // namespace comdare::cache_engine::builder::profile_facade
