#pragma once
// STRANG A KORRIGIERT — Increment 5 / S6a + S6b (2026-06-18, profil-getrieben). sota_catalog: die
// MATERIALISIERUNGS-DOMÄNE der benannten SOTA-Kompositionen + PRT-ART + der 3 Kompositionalen Joins
// (Reihen A/B/C). Parallel zu source_catalog.hpp (der die 320 Basis-Permutationen materialisiert), aber
// für die im comdare_thesis_profile DEKLARIERTEN <sota_series>-Lebewesen.
//
// Plan: docs/sessions/20260618-STRANG-A-KORRIGIERT-PROFIL-GETRIEBEN-PLAN.md (S6). KEINE neue Selektion —
// die Selektion (WELCHE Reihen/Lebewesen) kommt aus dem Profil (<sota_series_set>). Dieser Katalog liefert
// NUR zu einem (Reihe, Lebewesen)-Paar die REALE Modul-Quelle (binary_id → render_*-Source). Der Treiber
// fragt den Katalog über das profil-selektierte Paar ab (kein String→Typ-Dispatch im Treiber).
//
// S6a — SOTA/PRT-ART im Katalog baubar: die benannten KnownReferenceCompositions (art/hot/masstree/surf/
//   start/wormhole, known_compositions_list.hpp) + PRT-ART (prt_art_reference.hpp) sind reale
//   AdHocComposition-Ausprägungen (je 1 Punkt). Materialisiert als COMDARE_DEFINE_ANATOMY_MODULE(<C>) —
//   derselbe Modul-Quelltext, den der CMake-Codegen (comdare_codegen_anatomy_module) für hot/wormhole
//   bereits emittiert (build/.../generated/anatomy_modules/). Lazy-Compile (1 DLL = 1 TU) bleibt.
//
// S6b — sota_series A/B/C via pruefling_merge: je (Reihe × Lebewesen) eine reale Lebewesen-DLL:
//   • Reihe A = Stufe1_CeOnly         → das Lebewesen ISOLIERT (PrtArtComposition / die 6 SOTA selbst).
//   • Reihe B = Stufe2_PrueflingReplace → PRT-ART ERSETZT den path_compression-Slot einer SOTA-Host-
//                                         Komposition (HotPrtStufe2ReplaceComposition). Fallback via
//                                         HasPruefling_v (pruefling_merge.hpp).
//   • Reihe C = Stufe3_FullJoin        → Union (non-redundant); je 1 Punkt = Pruefling-Repräsentant der
//                                         Union (MasstreePrtStufe3FullJoinComposition).
//   Die Slot-Auswahl folgt mechanisch aus MergeStrategy (KEINE Code-Selektion); die Gattung ist via
//   assert_pruefling_slot_genus garantiert (prt_art_merge_reference.hpp). PRT-ART wird über die
//   IPrueflingFactory-Form (Abstract-Factory-Slot, i_pruefling_factory.hpp) als Slot eingebracht.
//
// EHRLICHKEIT (Plan-Direktive): falls ein Lebewesen real NICHT baubar ist, liefert der Katalog für sein
// (Reihe,Lebewesen)-Paar eine LEERE Quelle → der Orchestrator markiert die DLL als nicht baubar (sichtbar,
// nicht versteckt). Der Klein-Pilot (test_sota_series_pilot) baut je Reihe A/B/C ≥1 reale DLL real mit cl.
//
// ⚠️ Umbrella-/Komposition-schwer → gehört in die HARNESS-/Test-.cpp, NICHT in den engine-agnostischen
//    Treiber-Header. C++23, header-only.

#include "xml_config_parser/xml_config_parser.hpp"                // ThesisSotaSeries / ThesisProfile

#include <compositions/known_compositions_list.hpp>               // KnownReferenceCompositions (6 SOTA)
#include <compositions/prt_art_reference.hpp>                     // PrtArtComposition (Reihe A Prüfling)
#include <compositions/prt_art_merge_reference.hpp>               // Hot/Masstree-PRT-Merge (Reihe B/C)

#include <boost/mp11.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace cmp  = ::comdare::cache_engine::compositions;
namespace cx   = ::comdare::builder::xml;
namespace mp   = boost::mp11;

// ── Der binary_id der SOTA/PRT-ART-Reihen. EIGENER Namensraum-Präfix "sota::<series>::<name>" → kollidiert
//    GARANTIERT NICHT mit dem Basis-320-Raum (der "search_algo=.../..."-Pfade nutzt). Stabil + distinkt je
//    (Reihe × Komposition). (Die SOTA-Organe tragen kein wrapper-name() → kein serialize_composition_path.) ──
[[nodiscard]] inline std::string sota_binary_id(std::string const& series, std::string const& comp_name) {
    return "sota::" + series + "::" + comp_name;
}

// ── Die Reihe-A-Lebewesen (Stufe1_CeOnly): ISOLIERT je 1 reale Komposition. Single-Source 1:1 zu den
//    Profil-<sota_series id="A" lebewesen=..>-Namen (m3v2_study.profile.xml). ──
struct SotaModule {
    std::string lebewesen;     // "art" / "hot" / ... / "prt_art"
    std::string binary_id;     // serialize_composition_from_slots<C>() (== Baum-binary_id-Format)
    std::string composition_type;  // FQ-Typ-Name (für COMDARE_DEFINE_ANATOMY_MODULE)
    std::string header;        // Include-Header der Composition
};

/// render_sota_module_source — der kompilierbare Modul-.cpp-Quelltext einer SOTA/PRT-ART-Komposition.
/// IDENTISCH zum CMake-Codegen-Modul (comdare_codegen_anatomy_module): ABI-Header + Composition-Header +
/// COMDARE_DEFINE_ANATOMY_MODULE(<FQ-Typ>). Real-baubar via cl (probe-belegt für alle 6 SOTA + PRT-ART + B/C).
[[nodiscard]] inline std::string render_sota_module_source(std::string const& fq_type,
                                                           std::string const& header) {
    return "// AUTO-GENERATED (STRANG-A Inc5 / S6) SOTA/PRT-ART Modul — DO NOT EDIT\n"
           "#define COMDARE_ANATOMY_MODULE_BUILD 1\n"
           "#include <cache_engine/abi/anatomy_module_abi_v1.hpp>\n"
           "#include <" + header + ">\n"
           "COMDARE_DEFINE_ANATOMY_MODULE(" + fq_type + ")\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// S6a — der KATALOG der materialisierbaren Lebewesen (binary_id → reale Modul-Quelle). compile-time
//   über die Composition-Typen iteriert; jeder Eintrag ist eine reale AdHocComposition (NICHT Selektion).
// ─────────────────────────────────────────────────────────────────────────────

/// build_sota_series_a_modules — die Reihe-A-Lebewesen (PRT-ART + die 6 KnownReferenceCompositions),
/// je als isolierte reale Komposition (Stufe1_CeOnly). binary_id = serialize_composition_from_slots.
[[nodiscard]] inline std::vector<SotaModule> build_sota_series_a_modules() {
    std::vector<SotaModule> out;
    // PRT-ART (Thesis-eigenes Prüfling-Lebewesen, Reihe A isoliert).
    out.push_back(SotaModule{
        "prt_art",
        sota_binary_id("A", std::string{cmp::PrtArtComposition::name}),
        "::comdare::cache_engine::compositions::PrtArtComposition",
        "compositions/prt_art_reference.hpp"});
    // Die 6 SOTA über die zentrale mp_list (Tool-Iteration, known_compositions_list.hpp). Wir nehmen die
    // ersten 6 (CE-Reimpl) als die im Profil benannten Lebewesen art/hot/wormhole/surf/masstree/start.
    auto add_sota = [&]<class Entry>() {
        using C = typename Entry::composition;
        std::string const sn{Entry::short_name};
        // Nur die 6 Basis-SOTA (kein _pb-PaperBinding in diesem Pilot — die Profil-Namen sind die 6 Basis).
        if (sn == "art" || sn == "hot" || sn == "wormhole" || sn == "surf" || sn == "masstree" || sn == "start") {
            out.push_back(SotaModule{
                sn,
                sota_binary_id("A", std::string{C::name}),
                std::string{C::cpp_type_name},    // FQ-Typ-Name aus HasCompositionLocation (R5.G)
                std::string{C::header_include}});
        }
    };
    mp::mp_for_each<cmp::KnownReferenceCompositions>([&]<class Entry>(Entry) { add_sota.template operator()<Entry>(); });
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// S6b — die 3 Kompositionalen Joins als reale Reihen-Module (pruefling_merge). Reihe B/C tragen PRT-ART als
//   Pruefling-Slot in einer SOTA-Host-Komposition. binary_id distinkt (verschiedene Hosts + Patricia).
// ─────────────────────────────────────────────────────────────────────────────

/// sota_module_for — das reale Lebewesen-Modul für ein (Reihe, Lebewesen)-Paar des Profils. nullopt, wenn
/// das Paar (in diesem Pilot) NICHT real baubar ist (ehrlich → leere Quelle beim Caller). Mechanik:
///   Reihe A → das isolierte Lebewesen (build_sota_series_a_modules).
///   Reihe B → HotPrtStufe2ReplaceComposition  (Stufe2: PRT-ART ersetzt den path_compression-Slot).
///   Reihe C → MasstreePrtStufe3FullJoinComposition (Stufe3: Pruefling-Repräsentant der Union).
/// (Im Klein-Pilot ist B/C an EINEN Host gebunden; der Voll-Lauf [HELD] fächert über alle 7 Hosts.)
[[nodiscard]] inline std::optional<SotaModule> sota_module_for(std::string const& series,
                                                              std::string const& lebewesen) {
    if (series == "A") {
        for (auto const& m : build_sota_series_a_modules())
            if (m.lebewesen == lebewesen) return m;
        return std::nullopt;
    }
    if (series == "B") {
        return SotaModule{
            lebewesen + "+prt(B)",
            sota_binary_id("B", std::string{cmp::HotPrtStufe2ReplaceComposition::name}),
            "::comdare::cache_engine::compositions::HotPrtStufe2ReplaceComposition",
            "compositions/prt_art_merge_reference.hpp"};
    }
    if (series == "C") {
        return SotaModule{
            lebewesen + "+prt(C)",
            sota_binary_id("C", std::string{cmp::MasstreePrtStufe3FullJoinComposition::name}),
            "::comdare::cache_engine::compositions::MasstreePrtStufe3FullJoinComposition",
            "compositions/prt_art_merge_reference.hpp"};
    }
    return std::nullopt;
}

/// build_sota_source_map(profile) — die profil-getriebene SourceGenFn-Quelle: für jedes im Profil
/// deklarierte <sota_series id lebewesen merge> ein Eintrag binary_id → reale Modul-Quelle. KEINE
/// Selektion (die Reihen/Lebewesen kommen aus dem Profil); je Eintrag genau EIN realer perm-DLL-Quelltext.
[[nodiscard]] inline std::map<std::string, std::string>
build_sota_source_map(cx::ThesisProfile const& tp) {
    std::map<std::string, std::string> by_id;
    for (auto const& s : tp.sota_series) {
        if (auto m = sota_module_for(s.id, s.lebewesen))
            by_id.emplace(m->binary_id, render_sota_module_source(m->composition_type, m->header));
    }
    return by_id;
}

// ─────────────────────────────────────────────────────────────────────────────
// S7 (Inc6) — die VERDRAHTUNG der SOTA-Reihen in den profil-getriebenen Treiber.
// ─────────────────────────────────────────────────────────────────────────────

// Der STATISCHE Achsen-Name, unter dem ein SOTA-Lebewesen als (einwertiger) Tier-Baum dem Treiber-View
// vorgelegt wird. Eine SOTA-Reihe = EIN binary_id = EINE einwertige AxisLevel "sota_tier=<sota::S::name>".
// Der StaticBinaryView::operator[] serialisiert das zu binary_id == "sota_tier=<sota::S::name>" — ein
// EIGENER, disjunkter Namensraum gegen die Basis-320 ("search_algo=.../..."). Damit ist die Quellen-
// Vereinigung (S7a) eine einfache disjunkte UNION (kein Schlüssel-Konflikt).
inline constexpr char const* kSotaTierAxis = "sota_tier";

/// sota_view_binary_id — der binary_id, den der StaticBinaryView eines SOTA-Einzel-Tier-Baums erzeugt
/// (== "sota_tier=<sota::S::name>"). Single-Source mit build_sota_view_source_map (gleiche Key-Bildung).
[[nodiscard]] inline std::string sota_view_binary_id(std::string const& sota_bid) {
    return std::string{kSotaTierAxis} + "=" + sota_bid;
}

// Ein profil-deklariertes SOTA-Reihen-Lebewesen, aufbereitet als EIN Treiber-Pass (Reihe-Tag + view-binary_id).
struct SotaPass {
    std::string series;       // "A" / "B" / "C" (CSV-Tag row_series)
    std::string lebewesen;    // Profil-Lebewesen-Name (Doku/Log)
    std::string sota_bid;     // der rohe sota::S::name (AxisLevel-Wert der einwertigen "sota_tier"-Ebene)
    std::string view_binary_id;  // == "sota_tier=<sota::S::name>" (== StaticBinaryView-binary_id dieses Passes)
};

/// build_sota_passes(profile) — die Liste der SOTA-Reihen-Pässe AUS DEM PROFIL (1 Eintrag je real baubarem
/// <sota_series>). Reihenfolge = Profil-Reihenfolge (stabil/resumierbar). Ein nicht baubares (Reihe,Lebewesen)-
/// Paar (sota_module_for == nullopt) wird ausgelassen (ehrlich: kein Phantom-Pass). KEINE Selektion — das
/// Profil bestimmt, WELCHE Reihen/Lebewesen laufen.
[[nodiscard]] inline std::vector<SotaPass> build_sota_passes(cx::ThesisProfile const& tp) {
    std::vector<SotaPass> out;
    out.reserve(tp.sota_series.size());
    for (auto const& s : tp.sota_series) {
        if (auto m = sota_module_for(s.id, s.lebewesen))
            out.push_back(SotaPass{s.id, s.lebewesen, m->binary_id, sota_view_binary_id(m->binary_id)});
    }
    return out;
}

/// build_sota_view_source_map(profile) — wie build_sota_source_map, ABER der Schlüssel ist der VIEW-binary_id
/// ("sota_tier=<sota::S::name>") statt des rohen sota::S::name. Das ist GENAU die Form, die der Treiber-View
/// (einwertige AxisLevel "sota_tier") je SOTA-Pass erzeugt → die SourceGenFn-Vereinigung (S7a) findet die Quelle
/// direkt über den View-binary_id (kein Re-Mapping im Treiber). Disjunkt zum Basis-320-Schlüsselraum.
[[nodiscard]] inline std::map<std::string, std::string>
build_sota_view_source_map(cx::ThesisProfile const& tp) {
    std::map<std::string, std::string> by_id;
    for (auto const& s : tp.sota_series) {
        if (auto m = sota_module_for(s.id, s.lebewesen))
            by_id.emplace(sota_view_binary_id(m->binary_id),
                          render_sota_module_source(m->composition_type, m->header));
    }
    return by_id;
}

}  // namespace comdare::cache_engine::thesis_lazy
