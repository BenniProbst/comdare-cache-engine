#pragma once
// XML-Config-Parser - Liest 4 Konfigurations-Saetze (REV 7 §5.2)
//
//   cache_engine_permutations.xml
//   search_algorithm_permutations.xml
//   allocator_permutations.xml
//   test_data_sets.xml
//
// Minimaler XML-Reader fuer Phase 6.4 Skelett. Phase 7 ersetzt durch
// echten XML-Parser (tinyxml2 oder eigene Implementation).
//
// REV 7.6 V8.6 — Modi 'defined' (vorhandene SOTA-Profile aus
// algorithm_profiles/sota/) vs. 'full' (alle Achsen-Permutationen aus
// permutation_axes.xml). User-Direktive 2026-05-13/14 + Habich-Antwort
// 2026-05-14: "immer so vollstaendig wie moeglich".

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace comdare::builder::xml {

struct PermutationEntry {
    std::string id;
    std::unordered_map<std::string, std::string> attributes;
};

// REV 7.6 V8.6 — Mess-Modi
enum class MessreihenMode {
    Defined,  // nur referenzierte SOTA-Profile aus algorithm_profiles/sota/
    Full      // alle Permutationen aus permutation_axes.xml (Cartesian product)
};

[[nodiscard]] inline MessreihenMode parse_mode(std::string_view s) noexcept {
    if (s == "full") return MessreihenMode::Full;
    return MessreihenMode::Defined;  // default
}

[[nodiscard]] inline std::string_view mode_to_string(MessreihenMode m) noexcept {
    return (m == MessreihenMode::Full) ? "full" : "defined";
}

// REV 7.6 V8.6 — Ein Profil aus algorithm_profiles/sota/<id>.profile.xml
// REV 7.6 V19.1 — expected_workload Tag (optional Override fuer V11.2-Heuristik)
// REV 7.7 V29.A — allocator_override Tag (optional Override fuer axes/allocator)
struct AlgorithmProfile {
    std::string id;          // z.B. "art", "hot", "masstree"
    std::string paper_ref;   // z.B. "P01", "P02"
    std::unordered_map<std::string, std::string> axes;  // page, node, traversal, ...
    std::string key_types;
    std::string value_types;
    std::string expected_workload;   // V19.1: optional, z.B. "YCSB_A".."YCSB_F" oder leer
    std::string allocator_override;  // V29.A: optional, Allokator-Profile-id (z.B. "mimalloc")
};

// REV 7.6 V8.6 — Eine Messreihe aus messreihe-XML
struct Messreihe {
    std::string id;
    MessreihenMode mode = MessreihenMode::Defined;
    // defined-mode: referenzierte sota-Profile (per id)
    std::vector<std::string> sota_profile_refs;
    // full-mode: keine Refs noetig — Builder enumeriert alle Achsen-Permutationen
};

struct CacheEngineConfig {
    std::vector<PermutationEntry> cache_engine_permutations;
    std::vector<PermutationEntry> search_algorithm_permutations;
    std::vector<PermutationEntry> allocator_permutations;
    std::vector<PermutationEntry> test_data_sets;

    // REV 7.6 V8.6 — geladene Profile + Messreihen
    std::vector<AlgorithmProfile> sota_profiles;
    std::vector<Messreihe>        messreihen;
};

// ─────────────────────────────────────────────────────────────────────────────
// KF-2/KF-1 (2026-06-02): comdare_thesis_profile — Diplomarbeit-Cache-Line-Konfigurator.
// Schema: algorithm_profiles/thesis_profiles/SCHEMA.md.
// ─────────────────────────────────────────────────────────────────────────────
struct ThesisTier {           // Paper-Original = statisches Achsen-Tupel
    std::string id;
    std::string profile_ref;  // relativer Pfad zu sota/*.profile.xml
    std::string paper_ref;
};

struct ThesisAxisSpec {                       // eine permutierte (dynamische) Achse
    std::string ref;                          // z.B. "layout","isa","cacheline"
    std::vector<std::string> values;          // leer = volle Liste aus permutation_axes.xml
    // Nur fuer ref=="cacheline" (per-Organ Cache-Line-Unterachse, KF-3):
    std::vector<std::string> per_organ;        // Organe mit JE eigener Einstellung
    std::vector<std::string> line_sizes;       // 64/128/256
    std::vector<std::string> alignments;       // none/cacheline_aligned/padded
    std::vector<std::string> sw_prefetch_hints;// none/T0/T1/T2/NTA
};

struct ThesisMode {                           // einer der 3 Permutationsmodi
    std::string name;
    std::string merge;                        // Stufe1_CeOnly / Stufe2_PrueflingReplace / Stufe3_FullJoin
    std::vector<std::string> active_axes;     // Achsen-Teilmenge dieses Modus
    std::string pruefling;                    // optional (Stufe 2/3)
    std::vector<std::string> replaces_axes;   // optional (Stufe 2)
};

struct ThesisProfile {
    std::string id;
    int schema_version = 0;
    std::vector<ThesisTier>     base_tiers;
    std::vector<ThesisAxisSpec> permute_axes;
    // compile_dims
    std::vector<std::string>    workloads;        // YCSB A..F
    std::string                 telemetry_mode;   // on/off/leaf_sampled/all
    bool                        telemetry_silent = false;
    // runtime_dynamic (OS-seitig, CacheEngineBuilder-Durchlauf)
    std::vector<std::string>    thread_counts;    // 1/2/4
    std::vector<std::string>    hw_prefetcher;    // all_on/adjacent_off/all_off
    std::map<std::string, std::string> fixed_conditions;  // turbo/smt/aslr/numa/governor
    // repetitions
    int  repetitions             = 3;
    bool repetitions_interpolate = false;
    bool repetitions_overlay     = true;
    // modes + static
    std::vector<ThesisMode> modes;
    std::string             static_axes_from;     // "base_tier"
    std::string key_types, value_types;
};

class XmlConfigParser {
public:
    [[nodiscard]] CacheEngineConfig parse(std::filesystem::path const& root_dir) const;
    [[nodiscard]] std::vector<PermutationEntry> parse_one(std::filesystem::path const& xml_file) const;

    // REV 7.6 V8.6 — algorithm_profiles/-Loader
    [[nodiscard]] std::vector<AlgorithmProfile> load_sota_profiles(
        std::filesystem::path const& sota_dir) const;
    [[nodiscard]] AlgorithmProfile parse_profile(std::filesystem::path const& profile_xml) const;
    [[nodiscard]] std::vector<Messreihe> load_messreihen(
        std::filesystem::path const& messreihen_xml) const;

    // KF-1 (2026-06-02): comdare_thesis_profile-Parser (self-contained XML-DOM, xml_reader.hpp).
    // Liefert nullopt bei fehlender/fehlerhafter Datei oder falschem Wurzel-Tag.
    [[nodiscard]] std::optional<ThesisProfile> parse_thesis_profile(
        std::filesystem::path const& xml_file) const;
};

}  // namespace comdare::builder::xml
