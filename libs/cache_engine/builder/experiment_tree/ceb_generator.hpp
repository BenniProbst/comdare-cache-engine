#pragma once
// KF-8 (2026-06-02) — CebGenerator: C++23-Codegen aus dem Experiment-B+-Baum (KEIN Python).
//
// Nutzt ExperimentTree::for_each_binary (KF-9): je STATISCHEM Binary-Pfad (= eine zu ladende Tier-Binary)
// wird ein perm_<id>.cpp erzeugt. Die Achsen-Wahl je Ebene wird aus dem serialisierten Pfad abgeleitet —
// BELIEBIGE Achsentiefe (bricht den hartcodierten 5-Achsen-Deckel von tools/permutation_codegen/codegen.cmake).
// Host-Werkzeug: baut den Baum zur Laufzeit; die erzeugten Binaries bleiben compile-time-statisch. Doc architecture/26.

#include "experiment_tree.hpp"

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

/// Erzeugt den C++23-Quelltext einer Permutations-DLL für genau EINEN Binary-Pfad (beliebige Achsentiefe).
/// Selbst-konsistente Shell (Descriptor + extern-"C"-Entry); der achsen-spezifische run()-Body wird beim
/// Einweben der Organe (KF-5/KF-6) gefüllt — hier ein gültiger, kompilierbarer Default.
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

/// Generiert je Binary (for_each_binary) ein perm_<id>.cpp in out_dir + ein Manifest. Liefert die Anzahl.
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

}  // namespace comdare::cache_engine::builder::experiment
