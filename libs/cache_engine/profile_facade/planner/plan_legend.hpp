#pragma once
// -----------------------------------------------------------------------------
// PAKET W10-A (2026-07-19 nachts, Ledger §42/§42.b) — plan_legend: die EINE Formatierungs-Single-Source
// des dreistufigen Legenden-Namensschemas der CE-gesteuerten Pipeline-Kette.
//
// Ledger §42 (User-Direktive verbatim): „Mess-Achse[a,b,c] -> [a,b,c]CEB-Typ -> CEB definiert System-Achsen
// [d,e,f] -> [d,e,f]Pipeline-fuer-Build-freigegebenen-CEB-Raum -> CEB-Raum permutiert System-Achsen[d,e,f] und
// Organ-Achsen[g,h,i] -> [d,e,f,g,h,i]Tier-Binary". §42.b: „[d,e,f,g,h,i] benennt nur die HAUPT-Achsen" —
// Unter-Achsen sind reine LAUFZEIT-Permutation und stehen in KEINER Bau-Job-Legende (Bau=Haupt-only-Gate).
//
// Alle emittierten Job-/Pipeline-Namen der Kette laufen ausschliesslich ueber DIESE Funktionen — so ist das
// Namensschema an EINER Stelle definiert (Anti-Drift), deterministisch und YAML-quote-sicher. Das Format der
// Achsen-Arrays ist kurz (`[a,b,c]`), damit die Job-Namen Legenden bleiben und nicht zu Rauschen werden
// (2^17 Organ-Einzel-Jobs waeren keine Legende — deshalb buendelt die Bau-Ebene als `chunk<k>`, §42.b).
//
// Die drei Job-Ebenen (Legenden-Formen):
//   Stufe 1 (Planer-Rolle, --dump-ci):     "ceb:build:[a,b,c]" / "ceb:emit:[a,b,c]" / "ceb:trigger:[a,b,c]"
//   Stufe 2 (CEB-Rolle,   --emit-tier-ci): "tier:build:[a,b,c][d,e,f]:chunk<k>"
//   Stufe 3 (Mess-Job, GN-11/320er-gated): "measure:[a,b,c][d,e,f][g,h,i]"
//
// GOLDEN/HOST-NEUTRAL: reine String-Formatierung, keine Bau-/Mess-Semantik, keine Host-Werte.
// header-only, C++23.
// -----------------------------------------------------------------------------

#include <builder/experiment_tree/axis_path_serialization.hpp> // ex::kCompositionAxisNames (Organ-Haupt-Achsen-Namen, Single-Source)
#include <cache_engine/measurement/measurement_axis_registry.hpp> // kMeasurementAxisRegistry (16 Mess-Kategorien, Single-Source)

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::planner::legend {

namespace ex = ::comdare::cache_engine::builder::experiment;
namespace cm = ::comdare::cache_engine::measurement;

// Wie viele Organ-Haupt-Achsen die GATED Mess-Job-Legende [g,h,i] als Referenz benennt (illustrativ 3 —
// die reale Organ-Permutation eines Tier-Binaries ist der volle binary_id; die Mess-Job-Ebene ist
// GN-11/320er-gated und nennt hier die FUEHRENDEN Organ-Haupt-Achsen als Referenz-Positionen, §42.b).
inline constexpr std::size_t kOrganReferenceAxisCount = 3;

// ── Token-Sanitisierung: ein Legenden-Token darf die Trennzeichen der Array-/Job-Syntax nicht enthalten. ──
// Achsen-ids/Werte sind [A-Za-z0-9_.] — defensiv wird alles andere (inkl. '[',']',',',':') auf '_' gefaltet,
// damit die YAML-gequoteten Job-Namen kollisionsfrei und byte-deterministisch bleiben.
[[nodiscard]] inline std::string sanitize_token(std::string_view t) {
    std::string out;
    out.reserve(t.size());
    for (char const c : t) {
        bool const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' ||
                        c == '.' || c == '-';
        out += ok ? c : '_';
    }
    return out;
}

// ── Das kurze Achsen-Array `[t0,t1,...]` (deterministisch; leere Liste => "[]"). ──
[[nodiscard]] inline std::string axis_array(std::vector<std::string> const& tokens) {
    std::string out = "[";
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (i != 0) out += ',';
        out += sanitize_token(tokens[i]);
    }
    out += ']';
    return out;
}

// ── [a,b,c] — die Mess-Achsen-Kombination (CEB-Typ). Aus der Anwender-XML (<measurement_categories>). ──
// Kanonische Typ-Identitaet: dedupliziert + sortiert (Reihenfolge-unabhaengig => derselbe CEB-Typ). Deckt die
// Auswahl ALLE 16 System-Kategorien ab (oder ist leer = kein Subset deklariert), kollabiert die Legende zum
// kurzen, ehrlichen Sentinel `[all]` (das volle Mess-System). Single-Source der 16 = kMeasurementAxisRegistry.
[[nodiscard]] inline std::string measurement_combo(std::vector<std::string> const& categories) {
    std::vector<std::string> canon;
    canon.reserve(categories.size());
    for (auto const& c : categories) {
        std::string const s = sanitize_token(c);
        if (std::find(canon.begin(), canon.end(), s) == canon.end()) canon.push_back(s);
    }
    std::sort(canon.begin(), canon.end());
    if (canon.empty() || canon.size() >= cm::kMeasurementAxisCount) return "[all]";
    return axis_array(canon);
}

// ── [d,e,f] — die System-Achsen-Permutation (opt x simd des Director-Walks; die HAUPT-System-Achsen). ──
[[nodiscard]] inline std::string system_perm(std::string const& opt_id, std::string const& simd_id) {
    return axis_array({opt_id, simd_id});
}

// ── [g,h,i] — die GATED Organ-Referenz der Mess-Job-Legende (fuehrende Organ-Haupt-Achsen, Single-Source
//    ex::kCompositionAxisNames). Referenz-Positionen, KEIN konkreter binary_id: die reale Mess-Job-Auffaecherung
//    ist EIN Job je Organ-Haupt-Achsen-Permutation (2^17), GN-11/320er-gated (§42.b). ──
[[nodiscard]] inline std::string organ_reference() {
    std::vector<std::string> names;
    std::size_t const        n = std::min<std::size_t>(kOrganReferenceAxisCount, ex::kCompositionAxisNames.size());
    for (std::size_t i = 0; i < n; ++i) names.emplace_back(ex::kCompositionAxisNames[i]);
    return axis_array(names);
}

// ── Job-Namen (Doppelpunkt-Legenden-Form; in der YAML gequotet => Doppelpunkte im Schluessel eindeutig). ──

// Stufe 1 (Planer-Rolle): je Mess-Kombination die drei CEB-Jobs.
[[nodiscard]] inline std::string ceb_build_job(std::string const& combo) { return "ceb:build:" + combo; }
[[nodiscard]] inline std::string ceb_emit_job(std::string const& combo) { return "ceb:emit:" + combo; }
[[nodiscard]] inline std::string ceb_trigger_job(std::string const& combo) { return "ceb:trigger:" + combo; }

// Stufe 2 (CEB-Rolle): je System-Perm die Tier-Chunk-Bau-Jobs (Organ-Raum als chunk<k> gebuendelt, §42.b —
// KEINE Organ-Haupt-Achse und KEINE Unter-Achse in der Bau-Job-Legende).
[[nodiscard]] inline std::string tier_build_job(std::string const& combo, std::string const& perm, std::size_t chunk) {
    return "tier:build:" + combo + perm + ":chunk" + std::to_string(chunk);
}

// Stufe 3 (Mess-Job, GN-11/320er-gated): die volle Haupt-Achsen-Legende inkl. Organ-Referenz.
[[nodiscard]] inline std::string measure_job(std::string const& combo, std::string const& perm,
                                             std::string const& organ) {
    return "measure:" + combo + perm + organ;
}

// ── CMake-Target-Slugs (Identifier-sicher: nur [A-Za-z0-9_]; die [..]-Legende steht im COMMENT/echo). ──
[[nodiscard]] inline std::string cmake_slug(std::string_view raw) {
    std::string out;
    out.reserve(raw.size());
    for (char const c : raw) {
        bool const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        out += ok ? c : '_';
    }
    return out;
}

} // namespace comdare::cache_engine::planner::legend
