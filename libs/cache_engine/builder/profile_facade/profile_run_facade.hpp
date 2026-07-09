#pragma once
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────
// profile_run_facade — die SCHLANKE, umbrella-FREIE Produktiv-Fassade um die CEB-Eintritts-API run_profile
// (E4-XML-Vollvision-Roadmap, Dossier 17 Phase 1 / #230: "run_profile aus dem Test-Harness in produktive
// Lib/App heben; messung_driver ruft es → die #229-Kette laeuft produktiv (auf golden-320)").
//
// PROBLEM (der zu isolierende Kern): profile_run_entry.hpp::run_profile ist die EINE deklarative XML-getriebene
// CacheEngineBuilder-Eintritts-API — ABER umbrella-schwer: es zieht source_catalog.hpp → pilot_source_map →
// all_axes_umbrella (alle 19 Achsen-Typen compile-time). Der eigene Kopf-Kommentar (profile_run_entry.hpp:20-21)
// haelt fest: "Katalog-/Umbrella-schwer → gehoert in die HARNESS-/Test-.cpp, NICHT in den engine-agnostischen
// Treiber-Header." Es DARF also NICHT direkt in main.cpp (messung_driver) inkludiert werden.
//
// LOESUNG (Lehrbuch-Pattern FACADE, GoF): diese Fassade kapselt run_profile in GENAU EINER .cpp
// (profile_run_facade.cpp — die den Umbrella zieht) und bietet dem Aufrufer NUR diesen schlanken Header mit
// POD-Argumenten: KEINE Umbrella-Typen, KEINE ex::CompileFn, KEIN boost::mp11 in der Signatur. Der messung_driver
// inkludiert ausschliesslich diesen Header und linkt die Fassaden-Lib (comdare::profile_run_facade), die den
// Umbrella hinter der Uebersetzungseinheits-Grenze isoliert.
//
// ADDITIV & golden/ABI-NEUTRAL: rein additive Naht. Keine Aenderung an permutation_axes.xml,
// golden_fullpilot_320, POD-sizeof, GenusBindingTraits oder ABI-MAJOR. Der String→Typ-Dispatch bleibt
// compile-time IM Umbrella hinter der .cpp-Grenze (genau der Grund fuer die Fassade — kein Runtime-Switch,
// keine vtable im Hot-Path). C++23.
// ─────────────────────────────────────────────────────────────────────────────────────────────────────────────

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace comdare::cache_engine::builder::profile_facade {

// ── POD-Eingabe: ALLES, was NICHT aus dem Profil kommt (Pfade/Toolchain/Output/Overrides). Bewusst nur STL-Typen
//    (kein ex::CompileFn, keine wd::WorkloadConfig-Registry in der Signatur) — die Fassade baut die CompileFn
//    (g++-16 @rsp) + die Lastprofil-Registry INTERN. Feld-fuer-Feld-Spiegel der genutzten Teilmenge von
//    tlz::RunProfileArgs (profile_run_entry.hpp:51-77). Die WHAT-Konfiguration (Lebewesen/Achsen/Sweeps/SOTA/
//    Working-Set/run_options) liest run_profile selbst aus dem Profil. ──
struct ProfileRunArgs {
    std::filesystem::path profile_path;     // das comdare_thesis_profile (z.B. m3v2_study.profile.xml)
    std::filesystem::path out_csv;          // Ziel-CSV (Header genau EINMAL, alle Paesse darunter)
    std::filesystem::path src_dir;          // perm_<id>.cpp-Ausgabe (per-Binary-Subdir-Basis)
    std::filesystem::path dll_dir;          // perm_<id>.so-Ausgabe (per-Binary-Subdir-Basis)
    std::filesystem::path load_profile_dir; // optional: Verzeichnis der XML-Lastprofile (Achse 2). Leer = keine.

    std::uint64_t n_ops               = 10000;  // Mess-Workload je dyn-Setting
    std::size_t   max_binaries        = 0;      // 0 ⇒ <run_options>.cap aus dem Profil; sonst Override
    std::string   build_version       = "m3v2"; // Resume-Marke (.version-Sidecar)
    std::uint32_t n_repeats           = 3;      // Wiederholungen je (Binary×Setting)
    std::size_t   cores_per_build     = 4;      // KF-16b Default
    double        min_free_gb         = 0.0;    // RAM-Admission (0 = aus)
    bool          resume_override_set = false;  // true ⇒ resume kommt aus `resume`, nicht aus <run_options>
    bool          resume              = true;   // Mess-Resume (#139)
    bool          run_sota_series     = true;   // die <sota_series>-Paesse mitfahren (false = nur Basis)
    std::uint64_t working_set_override = 0;     // >0 ⇒ EIN N statt Profil-<working_set_sweep>
    std::string   sweep_axis;                   // leer = Basis-Selektion; sonst ein deklarierter <axis_sweep>
    std::string   platform_override;            // leer ⇒ <run_options>.platform; sonst CSV-Tag-Override
    std::string   build_version_tag_override;   // leer ⇒ <run_options>.build_version; sonst CSV-Tag-Override
};

// ── POD-Ergebnis (rein zaehlend; die CSV ist die massgebliche Mess-Ausgabe). Spiegel der tlz::RunProfileResult. ──
struct ProfileRunResult {
    int           exit_code        = 1; // 0 = mind. 1 (Binary×Setting) gemessen/resumiert UND CSV fehlerfrei
    std::size_t   basis_rows       = 0; // CSV-Zeilen aus dem Basis-Pass (frisch+resumiert)
    std::size_t   sota_rows        = 0; // CSV-Zeilen aus den SOTA-Reihen-Paessen
    std::size_t   basis_binary_ids = 0; // distinkte Basis-binary_ids dieses Laufs
    std::size_t   sota_binary_ids  = 0; // distinkte SOTA-Reihen-binary_ids
    std::uint64_t measured         = 0; // real gemessene (Binary×Setting)
    std::uint64_t resumed          = 0; // resumierte (Binary×Setting)
};

/// run_profile_facade — der EINE produktive Einstieg: parst das XML-Profil und faehrt die volle XML-getriebene
/// CacheEngineBuilder-Kette (Basis-320 ∪ SOTA-Reihen → EINE CSV) ueber tlz::run_profile. Kapselt den Umbrella +
/// den realen g++-16-@rsp-Compiler-Aufruf. Der Aufrufer (messung_driver/main.cpp) sieht KEINE Umbrella-Typen.
[[nodiscard]] ProfileRunResult run_profile_facade(ProfileRunArgs const& args);

} // namespace comdare::cache_engine::builder::profile_facade
