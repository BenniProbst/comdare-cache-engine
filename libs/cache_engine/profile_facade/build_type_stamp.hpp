#pragma once
// (i) §61-STUFEN Compile-Kennzeichnung -- die build_version-Suffix-Naht fuer den Compile-Einstellungs-Stempel.
// KONTEXT: die Modi bauen stufenweise (Debug -> Messung -> Release); Messung/Release duerfen Debug-Artefakte NUR
// wiederverwenden, wenn mit Release-Compile gebaut. Damit dll_is_current/Sidecar/Resume Debug-vs-Release SCHARF
// trennen, traegt JEDE Nicht-Default-(Debug-)Binary ein +bt=Debug am build_version. Der Mess-/Provision-Job
// exportiert COMDARE_BUILD_TYPE=Debug (Director, Emissions-Seite, NUR bei debug-Profilen); run_profile liest es hier
// und haengt +bt=Debug an perm_suffix / system_axes_version_suffix. ENV UNGESETZT ODER != "Debug" (Release/measure =
// Default) => "" => build_version BYTE-IDENTISCH (golden/Sidecar/Resume/dll_is_current unberuehrt). Der IMMER-
// explizite Array-Stempel (jede Binary traegt ihre volle Compile-Einstellung) bleibt K7b/G1 (#38) -- hier NUR die
// identitaets-stabile Minimalform (Nicht-Default). Winziger Header (nur <cstdlib>/<string>) -> isoliert testbar.

#include <cstdlib>
#include <string>
#include <string_view>

namespace comdare::cache_engine::thesis_lazy {

/// build_type_version_suffix() -- "+bt=Debug", wenn COMDARE_BUILD_TYPE == "Debug"; sonst "" (byte-identischer
/// Default-Pfad). ASCII-/literal-sicher (nur =+ und alnum), an das build_version anhaengbar ohne Escaping.
[[nodiscard]] inline std::string build_type_version_suffix() {
    if (char const* e = std::getenv("COMDARE_BUILD_TYPE"); e != nullptr && std::string_view{e} == "Debug")
        return "+bt=Debug";
    return {};
}

} // namespace comdare::cache_engine::thesis_lazy
