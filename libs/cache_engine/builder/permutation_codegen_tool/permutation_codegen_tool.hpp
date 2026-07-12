#pragma once
// #25 Teil B — permutation_codegen_tool Library-API (C++23-Port von codegen.cmake)
//
// User-Direktive (#25 B): den Permutations-Codegen von CMake-Script (tools/permutation_codegen/
// codegen.cmake) treu nach C++23 portieren. Ziel = BYTE-IDENTISCHE `permutations.cmake` (das
// add_library/comdare_permutations_all-Registrierungs-Snippet des 320-DLL-Baus) fuer dieselben
// Eingaben COMDARE_TARGET_ISA × COMDARE_PROFILE × COMDARE_MODE × COMDARE_OUTPUT.
//
// Sicherheits-Rahmen (KERN-BUILD-KRITISCH): Dieses Tool ist ein NEUES opt-in-Backend `cpp`.
// Das Default-Backend bleibt `cmake` (cmake -P codegen.cmake) UNVERAENDERT; der reale golden-320-
// Bau ist damit garantiert unberuehrt. Die Byte-Identitaet der `permutations.cmake` ist der Beweis
// der treuen Ersetzung (siehe tests/unit/perm_codegen_byte_identity.cmake).
//
// KEIN Python (Talos-OS-Direktive). Reines C++23 + CMake.
//
// Portierungs-Umfang (VOLLER Bau-Ersatz — alle vier Artefakte von codegen.cmake):
//   * render_permutations_cmake()  -> byte-identisch zu codegen.cmake Zeile 530-670 (file(WRITE OUTPUT))
//   * render_manifest()            -> byte-identisch zu codegen.cmake Zeile 521-525 (permutations_manifest.txt)
//   * render_wrapper_source()      -> byte-identisch zu codegen.cmake Zeile 258-502 (perm_src/perm_<id>.cpp)
//   * perm_versions/*.version      -> byte-identisch zu codegen.cmake Zeile 504-509 AUSSER dem
//                                     Feld `last_codegen` (Z.508 = ${CMAKE_CURRENT_LIST_FILE}, backend-
//                                     identifizierend): das cpp-Tool schreibt ehrlich seinen EIGENEN
//                                     Marker (cpp_last_codegen_marker()), statt codegen.cmake vorzugeben.
// generate() erzeugt alle vier Artefakte inkl. der V36.E selective-rebuild-Maschinerie (Z.179-514).

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::permutation_codegen {

// ─────────────────────────────────────────────────────────────────────────────
// Profile-Filter — Subset-Auswahl der Achsen-Matrix (codegen.cmake Zeile 60-72)
// ─────────────────────────────────────────────────────────────────────────────
enum class Profile : std::uint8_t { Smoke = 0, Medium = 1, Full = 2 };

[[nodiscard]] std::optional<Profile> parse_profile(std::string_view value) noexcept;
[[nodiscard]] std::string_view       profile_name(Profile profile) noexcept;

// ─────────────────────────────────────────────────────────────────────────────
// Inputs — spiegelt die vier CMake-Cache-Variablen von codegen.cmake
// ─────────────────────────────────────────────────────────────────────────────
struct Inputs {
    std::string           target_isa = "auto";               ///< COMDARE_TARGET_ISA
    Profile               profile    = Profile::Smoke;       ///< COMDARE_PROFILE
    std::string           mode       = "on_build_on_demand"; ///< COMDARE_MODE (Header + selective-rebuild)
    std::filesystem::path output;                            ///< COMDARE_OUTPUT (Pfad der permutations.cmake)
    /// Pfad zu axes_versions.txt (codegen.cmake Z.142). Leer => alle Achsen-Versionen "v0"
    /// (identisch zu codegen.cmake bei fehlender Datei). Fuer Wrapper-Byte-Identitaet noetig.
    std::filesystem::path axes_versions;
};

// ─────────────────────────────────────────────────────────────────────────────
// Permutation — ein Punkt im Achsen-Kreuzprodukt (codegen.cmake Zeile 114/128)
// ─────────────────────────────────────────────────────────────────────────────
struct Permutation {
    std::string simd;             ///< Achse 12
    std::string layout;           ///< Achse 4
    std::string alloc;            ///< Achse 6
    bool        extended = false; ///< true = medium/full (Achsen 1+8 aktiv)
    std::string node;             ///< Achse 1 (nur extended)
    std::string concur;           ///< Achse 8 (nur extended)

    /// perm_id: "|"-Join durch "_" ersetzt (codegen.cmake Zeile 180/548).
    [[nodiscard]] std::string id() const;
    /// Achsen-Ordnerpfad simd_.../layout_.../alloc_...[/node_.../concur_...] (codegen.cmake Zeile 553/558).
    [[nodiscard]] std::string axis_path() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Kreuzprodukt-Generator + ConstraintFilter (codegen.cmake Zeile 46-134)
// ─────────────────────────────────────────────────────────────────────────────

/// host_supports_isa — 1:1-Port von _perm_host_supports_isa (codegen.cmake Zeile 78-96).
[[nodiscard]] bool host_supports_isa(std::string_view isa, std::string_view target_isa) noexcept;

/// enumerate_permutations — verschachtelte foreach ueber die Profile-Achsen-Wertmengen
/// (codegen.cmake Zeile 106-134), inkl. ISA-Filter auf der SIMD-Achse.
[[nodiscard]] std::vector<Permutation> enumerate_permutations(Profile profile, std::string_view target_isa);

// ─────────────────────────────────────────────────────────────────────────────
// Emitter — byte-identische Text-Erzeugung
// ─────────────────────────────────────────────────────────────────────────────

/// render_permutations_cmake — byte-identisch zu codegen.cmake Zeile 530-670.
/// `perms` = Ergebnis von enumerate_permutations(inputs.profile, inputs.target_isa).
[[nodiscard]] std::string render_permutations_cmake(Inputs const& inputs, std::vector<Permutation> const& perms);

/// render_manifest — byte-identisch zu codegen.cmake Zeile 521-525.
[[nodiscard]] std::string render_manifest(Inputs const& inputs, std::vector<Permutation> const& perms);

// ─────────────────────────────────────────────────────────────────────────────
// V36.E — per-Permutation Wrapper-Quelle + Versionierung (codegen.cmake Z.140-514)
// ─────────────────────────────────────────────────────────────────────────────

/// axis_version_lookup — 1:1-Port von _perm_axis_version (codegen.cmake Z.149-156).
/// Sucht "axis_<axis_key>=<value>" im axes_versions.txt-Inhalt; default "v0".
[[nodiscard]] std::string axis_version_lookup(std::string_view axes_versions_content, std::string_view axis_key);

/// current_axes_signature — codegen.cmake Z.197-207: "simd=..;layout=..;alloc=..;node=..;concur=..".
[[nodiscard]] std::string current_axes_signature(std::string_view axes_versions_content, Permutation const& perm);

/// bump_minor — 1:1-Port von _perm_bump_minor (codegen.cmake Z.159-165): 0.1.0 -> 0.2.0.
[[nodiscard]] std::string bump_minor(std::string_view in_version);

/// render_wrapper_source — byte-identisch zu codegen.cmake Z.258-502 (perm_src/perm_<id>.cpp).
[[nodiscard]] std::string render_wrapper_source(Inputs const& inputs, Permutation const& perm,
                                                std::string_view stored_version, std::string_view current_axes_sig);

/// render_version_file — codegen.cmake Z.504-509. `last_codegen` traegt den EIGENEN cpp-Marker
/// (nicht den codegen.cmake-Pfad) — ehrliche Backend-Kennzeichnung.
[[nodiscard]] std::string render_version_file(Permutation const& perm, std::string_view stored_version,
                                              std::string_view current_axes_sig);

/// cpp_last_codegen_marker — der ehrliche last_codegen-Wert des cpp-Backends. Der Byte-Identitaets-
/// Test schliesst GENAU die `last_codegen=`-Zeile aus (backend-identifizierend).
[[nodiscard]] std::string_view cpp_last_codegen_marker() noexcept;

// ─────────────────────────────────────────────────────────────────────────────
// generate — schreibt permutations.cmake (inputs.output) + permutations_manifest.txt
// (Schwester-Datei im selben Verzeichnis, wie codegen.cmake Zeile 33).
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] bool generate(Inputs const& inputs, std::string* error_out = nullptr);

} // namespace comdare::cache_engine::builder::permutation_codegen
