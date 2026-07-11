#pragma once
// STRANG A KORRIGIERT — Increment 5 / S6a + S6b (2026-06-18, profil-getrieben). sota_catalog: die
// MATERIALISIERUNGS-DOMÄNE der benannten SOTA-Kompositionen + PRT-ART + der 3 Kompositionalen Joins
// (Reihen A/B; Reihe C ist build-übergreifend). Parallel zu source_catalog.hpp (der die 320 Basis-Permutationen materialisiert), aber
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
// S6b — sota_series via pruefling_merge: je (Stufe × Lebewesen) eine reale Lebewesen-DLL. Die 3 Stufen
//   (MergeStrategy) sind die mechanische Slot-Wahl (KEINE Code-Selektion); die Gattung ist via
//   assert_pruefling_slot_genus garantiert (prt_art_merge_reference.hpp); PRT-ART kommt über die
//   IPrueflingFactory-Form (Abstract-Factory-Slot, i_pruefling_factory.hpp) in den Slot:
//   • Stufe1_CeOnly           → das Lebewesen ISOLIERT (PrtArtComposition / die 6 SOTA selbst).
//   • Stufe2_PrueflingReplace → PRT-ART ERSETZT den path_compression-Slot einer SOTA-Host-Komposition
//                               (HotPrtStufe2ReplaceComposition); Fallback via HasPruefling_v.
//   • Stufe3_FullJoin         → Union (non-redundant); je SOTA-Host ein Pruefling-Repräsentant
//                               (<Host>PrtStufe3FullJoinComposition), prt_art-als-Host degeneriert zu nullopt.
//
//   STUFE→REIHE-ZUORDNUNG — EINGEFROREN per Thesis ch4 §4.8 / Tab. tab:stage-series
//   (kapitel/de/04_concept_architecture.tex Z.314-331) + Treiber-Enum MessreiheKind (02_messung_driver/
//   main.cpp:108-118 — der ist BEREITS korrekt):
//     Stufe1 ∪ Stufe2 → Reihe A (Prüfling vs SOTA) · Stufe3 → Reihe B (systematische Variation) ·
//     Reihe C (Merge/Regression alt↔neu) = BUILD-ÜBERGREIFEND, an KEINE Stufe gebunden.
//   ✅ #178 ERLEDIGT (2026-06-28): die Reihe wird MECHANISCH aus der Stufe (merge) abgeleitet — sota_module_for()
//   dispatcht auf `merge` (Stufe), und stufe_to_reihe(merge) liefert den Reihen-Tag (Stufe1_CeOnly/
//   Stufe2_PrueflingReplace→"A", Stufe3_FullJoin→"B"). Die Profile m3v2_study/_smoke/_sota_pilot führen den
//   `id`-Tag NUR noch als (zur Stufe konsistente) Selbst-Doku; Reihe C ist build-übergreifend und wird von
//   KEINER Stufe erzeugt (kein <sota_series id="C">). CSV-row_series + Treiber-Enum MessreiheKind stimmen damit
//   überein (kein falsches 1:1 A=St1/B=St2/C=St3 mehr).
//
// EHRLICHKEIT (Plan-Direktive): falls ein Lebewesen real NICHT baubar ist, liefert der Katalog für sein
// (Reihe,Lebewesen)-Paar eine LEERE Quelle → der Orchestrator markiert die DLL als nicht baubar (sichtbar,
// nicht versteckt). Der Klein-Pilot (test_sota_series_pilot) baut je Stufe ≥1 reale DLL real mit cl und
// belegt Stufe3/Reihe B zusätzlich per 6 SOTA-Hosts.
//
// ⚠️ Umbrella-/Komposition-schwer → gehört in die HARNESS-/Test-.cpp, NICHT in den engine-agnostischen
//    Treiber-Header. C++23, header-only.

#include "xml_config_parser/xml_config_parser.hpp" // ThesisSotaSeries / ThesisProfile

#include <compositions/known_compositions_list.hpp> // KnownReferenceCompositions (6 SOTA)
#include <compositions/prt_art_reference.hpp>       // PrtArtComposition (Reihe A Prüfling)
#include <compositions/prt_art_merge_reference.hpp> // PRT-Merge-Kompositionen (Stufe2/Reihe A, Stufe3/Reihe B)

#include <boost/mp11.hpp>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::thesis_lazy {

namespace cmp = ::comdare::cache_engine::compositions;
namespace cx  = ::comdare::builder::xml;
namespace mp  = boost::mp11;

// ── Der binary_id der SOTA/PRT-ART-Reihen. EIGENER Namensraum-Präfix "sota::<series>::<name>" → kollidiert
//    GARANTIERT NICHT mit dem Basis-320-Raum (der "search_algo=.../..."-Pfade nutzt). Stabil + distinkt je
//    (Reihe × Komposition). (Die SOTA-Organe tragen kein wrapper-name() → kein serialize_composition_path.) ──
[[nodiscard]] inline std::string sota_binary_id(std::string const& series, std::string const& comp_name) {
    return "sota::" + series + "::" + comp_name;
}

// ── #178 / Thesis ch4 §4.8 (tab:stage-series): die EINGEFRORENE Stufe→Reihe-Ableitung (mechanisch, Single-Source).
//    Stufe1 ∪ Stufe2 → Reihe A (Prüfling vs SOTA) · Stufe3 → Reihe B (systematische Variation). Reihe C
//    (Merge/Regression alt↔neu) ist BUILD-ÜBERGREIFEND, an KEINE Stufe gebunden → wird hier NIE erzeugt. ──
[[nodiscard]] inline std::string stufe_to_reihe(std::string const& merge) {
    if (merge == "Stufe1_CeOnly") return "A";
    if (merge == "Stufe2_PrueflingReplace") return "A";
    if (merge == "Stufe3_FullJoin") return "B";
    return "-"; // unbekannte/leere Stufe → kein Phantom-Reihen-Tag
}

// ── Die Reihe-A-Lebewesen (Stufe1_CeOnly): ISOLIERT je 1 reale Komposition. Single-Source 1:1 zu den
//    Profil-<sota_series id="A" lebewesen=..>-Namen (m3v2_study.profile.xml). ──
struct SotaModule {
    std::string lebewesen;        // "art" / "hot" / ... / "prt_art"
    std::string binary_id;        // serialize_composition_from_slots<C>() (== Baum-binary_id-Format)
    std::string composition_type; // FQ-Typ-Name (für COMDARE_DEFINE_ANATOMY_MODULE)
    std::string header;           // Include-Header der Composition
};

/// render_sota_module_source — der kompilierbare Modul-.cpp-Quelltext einer SOTA/PRT-ART-Komposition.
/// IDENTISCH zum CMake-Codegen-Modul (comdare_codegen_anatomy_module): ABI-Header + Composition-Header +
/// COMDARE_DEFINE_ANATOMY_MODULE(<FQ-Typ>). Real-baubar via cl (probe-belegt für alle 6 SOTA + PRT-ART + Stufe3/B).
[[nodiscard]] inline std::string render_sota_module_source(std::string const& fq_type, std::string const& header) {
    return "// AUTO-GENERATED (STRANG-A Inc5 / S6) SOTA/PRT-ART Modul — DO NOT EDIT\n"
           "#define COMDARE_ANATOMY_MODULE_BUILD 1\n"
           "#include <cache_engine/abi/anatomy_module_abi_v1.hpp>\n"
           "#include <" +
           header +
           ">\n"
           "COMDARE_DEFINE_ANATOMY_MODULE(" +
           fq_type + ")\n";
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
    out.push_back(SotaModule{"prt_art", sota_binary_id("A", std::string{cmp::PrtArtComposition::name}),
                             "::comdare::cache_engine::compositions::PrtArtComposition",
                             "compositions/prt_art_reference.hpp"});
    // Die 6 SOTA über die zentrale mp_list (Tool-Iteration, known_compositions_list.hpp). Wir nehmen die
    // ersten 6 (CE-Reimpl) als die im Profil benannten Lebewesen art/hot/wormhole/surf/masstree/start.
    auto add_sota = [&]<class Entry>() {
        using C = typename Entry::composition;
        std::string const sn{Entry::short_name};
        // Nur die 6 Basis-SOTA (kein _pb-PaperBinding in diesem Pilot — die Profil-Namen sind die 6 Basis).
        if (sn == "art" || sn == "hot" || sn == "wormhole" || sn == "surf" || sn == "masstree" || sn == "start") {
            out.push_back(SotaModule{sn, sota_binary_id("A", std::string{C::name}),
                                     std::string{C::cpp_type_name}, // FQ-Typ-Name aus HasCompositionLocation (R5.G)
                                     std::string{C::header_include}});
        }
    };
    mp::mp_for_each<cmp::KnownReferenceCompositions>(
        [&]<class Entry>(Entry) { add_sota.template operator()<Entry>(); });
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// S6b — die 3 Kompositionalen Joins als reale Reihen-Module (pruefling_merge). #178: Stufe2 (→Reihe A) + Stufe3
//   (→Reihe B) tragen PRT-ART als Pruefling-Slot in einer SOTA-Host-Komposition. binary_id distinkt.
// ─────────────────────────────────────────────────────────────────────────────

/// sota_module_for — das reale Lebewesen-Modul für ein (Stufe, Lebewesen)-Paar des Profils. nullopt, wenn das
/// Paar (in diesem Pilot) NICHT real baubar ist (ehrlich → leere Quelle beim Caller). #178: dispatcht auf die
/// STUFE (merge) — die mechanische Wahrheit — NICHT auf den Reihen-id; der Reihen-Tag wird via stufe_to_reihe(merge)
/// abgeleitet (binary_id-Namensraum). Mechanik:
///   Stufe1_CeOnly           → das isolierte Lebewesen (build_sota_series_a_modules) → Reihe A.
///   Stufe2_PrueflingReplace → HotPrtStufe2ReplaceComposition (PRT-ART ersetzt den path_compression-Slot) → Reihe A.
///   Stufe3_FullJoin         → <Host>PrtStufe3FullJoinComposition je SOTA-Host → Reihe B; prt_art-Host → nullopt.
/// AP-4-Follow: Stufe2/Reihe-A-Parallelfall ist weiterhin der bestehende HOT-Pilot und wird separat gefächert.
[[nodiscard]] inline std::optional<SotaModule> sota_module_for(std::string const& merge, std::string const& lebewesen) {
    if (merge == "Stufe1_CeOnly") {
        for (auto const& m : build_sota_series_a_modules())
            if (m.lebewesen == lebewesen) return m;
        return std::nullopt;
    }
    std::string const reihe = stufe_to_reihe(merge); // #178: Reihe mechanisch aus der Stufe (A für St2, B für St3)
    if (merge == "Stufe2_PrueflingReplace") {
        // AP-4-Follow: Stufe2/Reihe-A-Parallelfall bleibt absichtlich der bestehende HOT-Pilot.
        return SotaModule{lebewesen + "+prt(St2)",
                          sota_binary_id(reihe, std::string{cmp::HotPrtStufe2ReplaceComposition::name}),
                          "::comdare::cache_engine::compositions::HotPrtStufe2ReplaceComposition",
                          "compositions/prt_art_merge_reference.hpp"};
    }
    if (merge == "Stufe3_FullJoin") {
        // AP-4/#238: per-Host-FullJoin (analog build_sota_series_a_modules) — je SOTA-Host eine distinkte
        // <Host>PrtStufe3FullJoinComposition. prt_art-als-Host ist degeneriert (prt_art ist in Reihe A/Stufe1
        // bereits isoliert) → nullopt (ehrlich, keine Reihe-A-Duplikation unter B-Label).
        struct B {
            std::string_view type;
            std::string_view name;
        };
        auto pick = [&](std::string const& l) -> std::optional<B> {
            if (l == "art")
                return B{"::comdare::cache_engine::compositions::ArtPrtStufe3FullJoinComposition",
                         cmp::ArtPrtStufe3FullJoinComposition::name};
            if (l == "hot")
                return B{"::comdare::cache_engine::compositions::HotPrtStufe3FullJoinComposition",
                         cmp::HotPrtStufe3FullJoinComposition::name};
            if (l == "masstree")
                return B{"::comdare::cache_engine::compositions::MasstreePrtStufe3FullJoinComposition",
                         cmp::MasstreePrtStufe3FullJoinComposition::name};
            if (l == "surf")
                return B{"::comdare::cache_engine::compositions::SurfPrtStufe3FullJoinComposition",
                         cmp::SurfPrtStufe3FullJoinComposition::name};
            if (l == "start")
                return B{"::comdare::cache_engine::compositions::StartPrtStufe3FullJoinComposition",
                         cmp::StartPrtStufe3FullJoinComposition::name};
            if (l == "wormhole")
                return B{"::comdare::cache_engine::compositions::WormholePrtStufe3FullJoinComposition",
                         cmp::WormholePrtStufe3FullJoinComposition::name};
            return std::nullopt; // prt_art (degeneriert) + unbekannte
        };
        if (auto b = pick(lebewesen))
            return SotaModule{lebewesen + "+prt(St3)", sota_binary_id(reihe, std::string{b->name}),
                              std::string{b->type}, "compositions/prt_art_merge_reference.hpp"};
        return std::nullopt;
    }
    return std::nullopt;
}

/// build_sota_source_map(profile) — die profil-getriebene SourceGenFn-Quelle: für jedes im Profil
/// deklarierte <sota_series id lebewesen merge> ein Eintrag binary_id → reale Modul-Quelle. KEINE
/// Selektion (die Reihen/Lebewesen kommen aus dem Profil); je Eintrag genau EIN realer perm-DLL-Quelltext.
[[nodiscard]] inline std::map<std::string, std::string> build_sota_source_map(cx::ThesisProfile const& tp) {
    std::map<std::string, std::string> by_id;
    for (auto const& s : tp.sota_series) {
        if (auto m = sota_module_for(s.merge, s.lebewesen)) // #178: dispatch auf die Stufe (merge), nicht den id
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

// ─────────────────────────────────────────────────────────────────────────────
// #171 (Text-Agent AP-X2/TODO-2, 2026-06-20) — die Pruefling-Auspraegung "full" vs "abstract" als 1:1-Sicht auf
//   die BESTEHENDE MergeStrategy/sota_series-Mechanik (KEINE neue Achse, KEINE neue Selektion):
//     • full     == "Originalkonfiguration"/self-contained == Reihe A == merge=Stufe1_CeOnly: das Lebewesen
//                   fuellt ALLE 19 Achsen mit EIGENEN Organen (PrtArtComposition / die 6 SOTA isoliert).
//                   Beleg: pruefling_merge.hpp:93-95 StufeOneAxis=DefaultList; prt_art_reference.hpp:61-80.
//     • abstract == Teilmenge + Host-Fallback == Stufe2/Reihe A oder Stufe3/Reihe B:
//                   der Pruefling fuellt NUR einen Slot (path_compression), 18 Achsen via Host-Fallback.
//                   Beleg: pruefling_merge.hpp:113-118 conditional_t Ersetzt-mit-Fallback; prt_art_merge_reference.hpp.
//   Quelle der cacheline-doc §0 Z.11 / §1.2 Z.19 ("Paper-Algorithmen als Basis-Lebewesen in Originalkonfiguration").
//   Die Ableitung ist DETERMINISTISCH aus merge (bzw. series-id als Fallback); ein explizites
//   <sota_series pruefling_type=".."/> uebersteuert sie. Single-Source hier → kein verstreuter String-Vergleich.
[[nodiscard]] inline std::string derive_pruefling_type(std::string const& series_id, std::string const& merge,
                                                       std::string const& explicit_override) {
    if (!explicit_override.empty()) return explicit_override; // Forscher-Override gewinnt
    // Primaer aus merge (die mechanische Wahrheit): Stufe1 = self-contained = full; Stufe2/3 = Teilmenge = abstract.
    if (merge == "Stufe1_CeOnly") return "full";
    if (merge == "Stufe2_PrueflingReplace") return "abstract";
    if (merge == "Stufe3_FullJoin") return "abstract";
    // Fallback wenn merge leer/unbekannt: aus der Reihen-id (A=full, B/C=abstract) — kongruent zur merge-Map.
    if (series_id == "A") return "full";
    if (series_id == "B" || series_id == "C") return "abstract";
    return "-"; // weder ableitbar noch gesetzt (kein Phantom-Typ)
}

// Ein profil-deklariertes SOTA-Reihen-Lebewesen, aufbereitet als EIN Treiber-Pass (Reihe-Tag + view-binary_id).
struct SotaPass {
    std::string
        series; // #178: Reihe aus Stufe abgeleitet (stufe_to_reihe): "A" (St1∪St2) / "B" (St3); C=build-übergr. (CSV-Tag row_series)
    std::string lebewesen;      // Profil-Lebewesen-Name (Doku/Log)
    std::string sota_bid;       // der rohe sota::S::name (AxisLevel-Wert der einwertigen "sota_tier"-Ebene)
    std::string view_binary_id; // == "sota_tier=<sota::S::name>" (== StaticBinaryView-binary_id dieses Passes)
    std::string pruefling_type; // #171: "full" (Reihe A self-contained) / "abstract" (Stufe2/3 Teilmenge+Fallback)
};

/// build_sota_passes(profile) — die Liste der SOTA-Reihen-Pässe AUS DEM PROFIL (1 Eintrag je real baubarem
/// <sota_series>). Reihenfolge = Profil-Reihenfolge (stabil/resumierbar). Ein nicht baubares (Reihe,Lebewesen)-
/// Paar (sota_module_for == nullopt) wird ausgelassen (ehrlich: kein Phantom-Pass). KEINE Selektion — das
/// Profil bestimmt, WELCHE Reihen/Lebewesen laufen.
[[nodiscard]] inline std::vector<SotaPass> build_sota_passes(cx::ThesisProfile const& tp) {
    std::vector<SotaPass> out;
    out.reserve(tp.sota_series.size());
    for (auto const& s : tp.sota_series) {
        if (auto m = sota_module_for(s.merge, s.lebewesen)) {  // #178: dispatch auf die Stufe (merge)
            std::string const reihe = stufe_to_reihe(s.merge); // #178: Reihen-Tag mechanisch aus der Stufe
            out.push_back(SotaPass{reihe, s.lebewesen, m->binary_id, sota_view_binary_id(m->binary_id),
                                   derive_pruefling_type(reihe, s.merge, s.pruefling_type)}); // #171: full/abstract
        }
    }
    return out;
}

/// build_sota_view_source_map(profile) — wie build_sota_source_map, ABER der Schlüssel ist der VIEW-binary_id
/// ("sota_tier=<sota::S::name>") statt des rohen sota::S::name. Das ist GENAU die Form, die der Treiber-View
/// (einwertige AxisLevel "sota_tier") je SOTA-Pass erzeugt → die SourceGenFn-Vereinigung (S7a) findet die Quelle
/// direkt über den View-binary_id (kein Re-Mapping im Treiber). Disjunkt zum Basis-320-Schlüsselraum.
[[nodiscard]] inline std::map<std::string, std::string> build_sota_view_source_map(cx::ThesisProfile const& tp) {
    std::map<std::string, std::string> by_id;
    for (auto const& s : tp.sota_series) {
        if (auto m = sota_module_for(s.merge, s.lebewesen)) // #178: dispatch auf die Stufe (merge), nicht den id
            by_id.emplace(sota_view_binary_id(m->binary_id), render_sota_module_source(m->composition_type, m->header));
    }
    return by_id;
}

} // namespace comdare::cache_engine::thesis_lazy
