#pragma once
// KF-8 (2026-06-02) — CebGenerator: C++23-Codegen aus dem Experiment-B+-Baum (KEIN Python).
//
// ⚠️ ZWEI PFADE — KLARE TRENNUNG (D3 / L-77, 2026-06-02; korrigiert die irreführende Lesart von Task #70):
//
//   (1) generate_perm_source / generate_all = STRING-getriebenes PFAD-MANIFEST + DIAGNOSE-SHELL. Aus dem
//       serialisierten Binary-Pfad (for_each_binary, BELIEBIGE Achsentiefe — bricht den 5-Achsen-Deckel von
//       tools/permutation_codegen) wird je Binary ein perm_<id>.cpp mit Pfad-#defines + GÜLTIGEM Default-
//       `perm_run`-Stub erzeugt. Das ist KEINE reale Anatomie — `perm_run` misst nichts (Default 0.0); die
//       #defines werden von keiner Anatomie konsumiert. Zweck: Pfad↔Datei-Korrelation, Diagnose, Achsen-Manifest.
//
//   (2) generate_all_real<Engine> = REALER BR-4-ANATOMIE-PFAD. Delegiert an codegen::emit_adhoc_modules<Engine>
//       (TYP-getrieben via Engine::for_each_composition_type): je Komposition-TYP ein Modul-.cpp mit
//       #include all_axes_umbrella.hpp + COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<19 FQ-Typen>). DAS ist der reale,
//       baubare, ladbare Anatomie-Emitter (Gate-3/BR-4). Schreibt zusätzlich das Pfad/Index-Manifest dazu.
//
// Merksatz: Der String-Pfad (1) kennt nur Namen, kann daraus keinen C++-TYP auflösen → bewusst KEIN
// String→Typ-Dispatch (Direktive „kein Runtime-Switch"); die reale Anatomie kommt typ-getrieben über (2).
// Host-Werkzeug: baut den Baum zur Laufzeit; die erzeugten Binaries bleiben compile-time-statisch. Doc 26 §7 KF-8.

#include "experiment_tree.hpp"
#include "../codegen/adhoc_emitter.hpp"   // (2) realer Anatomie-Emitter (emit_adhoc_modules<Engine>)

#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Helfer: Pfad-Parsing + Bezeichner-Sanitisierung ──
[[nodiscard]] inline std::string ceb_sanitize(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out += (std::isalnum(static_cast<unsigned char>(c)) ? c : '_');
    return out;
}
[[nodiscard]] inline std::string ceb_upper(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) out += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return out;
}
/// Zerlegt einen Binary-Pfad ("axis=value/axis.sub=value/…") in (achse, wert)-Paare.
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>> ceb_parse_path(std::string_view path) {
    std::vector<std::pair<std::string, std::string>> out;
    std::size_t i = 0;
    while (i < path.size()) {
        std::size_t slash = path.find('/', i);
        std::string_view seg = path.substr(i, (slash == std::string_view::npos ? path.size() : slash) - i);
        std::size_t eq = seg.find('=');
        if (eq != std::string_view::npos)
            out.emplace_back(std::string{seg.substr(0, eq)}, std::string{seg.substr(eq + 1)});
        if (slash == std::string_view::npos) break;
        i = slash + 1;
    }
    return out;
}

/// (1) DIAGNOSE-SHELL (KEIN reale-Anatomie-Emitter): C++23-Quelltext für genau EINEN Binary-Pfad (beliebige
/// Achsentiefe). Selbst-konsistente Shell (Pfad-#defines + Descriptor + extern-"C"-Entry); `perm_run` ist ein
/// GÜLTIGER DEFAULT-STUB (gibt 0.0 zurück, misst NICHTS) — die #defines werden von keiner Anatomie konsumiert.
/// Zweck = Pfad↔Datei-Korrelation/Diagnose. Reale, messbare Anatomie ⇒ generate_all_real<Engine> (BR-4).
[[nodiscard]] inline std::string generate_perm_source(std::string const& binary_id) {
    std::string const id = ceb_sanitize(binary_id);
    auto const axes = ceb_parse_path(binary_id);

    std::string s;
    s += "// AUTO-GENERATED durch CebGenerator (KF-8, C++23). Binary-Pfad: " + binary_id + "\n";
    s += "// Achsen-Wahl je Ebene aus dem Experiment-B+-Baum-Pfad (beliebige Tiefe, kein 5-Achsen-Deckel).\n\n";
    for (auto const& [axis, value] : axes)
        s += "#define COMDARE_PERM_" + ceb_upper(ceb_sanitize(axis)) + "_IS_" + ceb_upper(ceb_sanitize(value)) + " 1\n";
    s += "\n#include <cstdint>\n\n";
    s += "#if defined(_WIN32) || defined(__CYGWIN__)\n";
    s += "  #define COMDARE_PERM_EXPORT __declspec(dllexport)\n";
    s += "#elif defined(__GNUC__) || defined(__clang__)\n";
    s += "  #define COMDARE_PERM_EXPORT __attribute__((visibility(\"default\")))\n";
    s += "#else\n  #define COMDARE_PERM_EXPORT\n#endif\n\n";
    s += "#define COMDARE_PERM_ID   \"" + id + "\"\n";
    s += "#define COMDARE_PERM_PATH \"" + binary_id + "\"\n\n";
    s += "struct PermDescriptor {\n";
    s += "    const char* id;\n    const char* path;\n    const char* axes;\n";
    s += "    int (*run)(unsigned long, double*);\n};\n\n";
    s += "// run(): achsen-spezifischer Workload (KF-5/KF-6 weben die Organ-Komposition ein); gültiger Default.\n";
    s += "extern \"C\" COMDARE_PERM_EXPORT int perm_" + id + "_run(unsigned long n_ops, double* out_micros_per_op) {\n";
    s += "    if (out_micros_per_op == nullptr || n_ops == 0) return -1;\n";
    s += "    *out_micros_per_op = 0.0;\n    return 0;\n}\n\n";
    s += "static constexpr char kAxes_" + id + "[] = COMDARE_PERM_PATH;\n";
    s += "static constexpr PermDescriptor kDesc_" + id + " {\n";
    s += "    COMDARE_PERM_ID, COMDARE_PERM_PATH, kAxes_" + id + ", &perm_" + id + "_run\n};\n\n";
    s += "extern \"C\" COMDARE_PERM_EXPORT const PermDescriptor* comdare_perm_descriptor() {\n";
    s += "    return &kDesc_" + id + ";\n}\n";
    return s;
}

/// (1) STRING-PFAD-MANIFEST/DIAGNOSE: je Binary (for_each_binary) ein DIAGNOSE-perm_<id>.cpp + Manifest.
/// ⚠️ KEINE reale Anatomie (perm_run misst nichts) — für baubare, messbare Module: generate_all_real<Engine>.
/// ⚠️ O(∏) (for_each_binary) — nur auf handhabbaren (Pilot-)Bäumen aufrufen.
[[nodiscard]] inline std::size_t generate_all(ExperimentTree const& tree, std::filesystem::path const& out_dir) {
    std::filesystem::create_directories(out_dir);
    std::ofstream manifest{out_dir / "perm_manifest.txt"};
    std::size_t count = 0;
    tree.for_each_binary([&](std::string const& binary_id, std::string const& /*pinned*/, TreeNode const& /*leaf*/) {
        std::string const id = ceb_sanitize(binary_id);
        std::ofstream f{out_dir / ("perm_" + id + ".cpp")};
        f << generate_perm_source(binary_id);
        if (manifest) manifest << "perm_" << id << "  " << binary_id << "\n";
        ++count;
    });
    return count;
}

/// (2) REALER BR-4-ANATOMIE-PFAD (D3 / L-77): delegiert TYP-getrieben an codegen::emit_adhoc_modules<Engine>
/// (Engine::for_each_composition_type) — je Komposition-TYP ein Modul-.cpp mit #include all_axes_umbrella.hpp +
/// COMDARE_DEFINE_ANATOMY_MODULE_ADHOC(<19 FQ-Typen>). DIESE Module sind baubar/ladbar/observierbar (Gate-3/BR-4).
/// Schreibt zusätzlich ein anatomy_manifest.txt (idx → Dateiname). Liefert die geschriebenen .cpp-Pfade.
/// Damit erfüllt KF-8 den realen-Anatomie-Anspruch an EINER Stelle (der String-Pfad (1) bleibt Diagnose).
template <class Engine>
[[nodiscard]] std::vector<std::filesystem::path>
generate_all_real(std::filesystem::path const& out_dir) {
    auto files = codegen::emit_adhoc_modules<Engine>(out_dir);
    std::ofstream manifest{out_dir / "anatomy_manifest.txt"};
    if (manifest)
        for (std::size_t i = 0; i < files.size(); ++i)
            manifest << i << "  " << files[i].filename().string() << "\n";
    return files;
}

}  // namespace comdare::cache_engine::builder::experiment
