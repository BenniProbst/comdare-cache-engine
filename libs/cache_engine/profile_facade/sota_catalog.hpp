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
// (test_sota_series_pilot: Pilot-Test entfernt 2026-07-11; Host/Emitter heute Code/02_messung_driver, E4-XML)
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
#include <set>
#include <string>
#include <string_view>
#include <utility>
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
    // M-CE-10 (Voll-Review 2026-07-13; per-Host-Auffächerung 2026-07-14): das HOST-Lebewesen der Komposition — das
    // Original-Quell-Repository, dessen H2-Code-Qualitäts-Score dieses Modul trägt (host-dominant, #171: "abstract" =
    // der Host füllt 16/17 Achsen (INC-2d), der Prüfling nur den path_compression-Slot). Stufe1 (isoliert) == lebewesen;
    // Stufe2 UND Stufe3 (per-Host) == dem angefragten Lebewesen (das IST der Host). Damit trägt h2_score_for das
    // reale Host-Lebewesen (nach der per-Host-Auffächerung ist das = das angefragte lebewesen, kein FIX "hot" mehr).
    std::string host_lebewesen;
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
    // PRT-ART (Thesis-eigenes Prüfling-Lebewesen, Reihe A isoliert). host==prt_art (KEIN Original-Repo → H2 n/a).
    out.push_back(SotaModule{"prt_art", sota_binary_id("A", std::string{cmp::PrtArtComposition::name}),
                             "::comdare::cache_engine::compositions::PrtArtComposition",
                             "compositions/prt_art_reference.hpp", "prt_art"});
    // Die 6 SOTA über die zentrale mp_list (Tool-Iteration, known_compositions_list.hpp). Wir nehmen die
    // ersten 6 (CE-Reimpl) als die im Profil benannten Lebewesen art/hot/wormhole/surf/masstree/start.
    auto add_sota = [&]<class Entry>() {
        using C = typename Entry::composition;
        std::string const sn{Entry::short_name};
        // Nur die 6 Basis-SOTA (kein _pb-PaperBinding in diesem Pilot — die Profil-Namen sind die 6 Basis).
        if (sn == "art" || sn == "hot" || sn == "wormhole" || sn == "surf" || sn == "masstree" || sn == "start") {
            out.push_back(SotaModule{sn, sota_binary_id("A", std::string{C::name}),
                                     std::string{C::cpp_type_name}, // FQ-Typ-Name aus HasCompositionLocation (R5.G)
                                     std::string{C::header_include},
                                     sn}); // host_lebewesen == sn (Stufe1 isoliert: der Host IST das Lebewesen)
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
///   Stufe2_PrueflingReplace → <Host>PrtStufe2ReplaceComposition je SOTA-Host (PRT-ART ersetzt den path_compression-
///                             Slot) → Reihe A; prt_art-Host → nullopt (M-CE-10 per-Host, 2026-07-14).
///   Stufe3_FullJoin         → <Host>PrtStufe3FullJoinComposition je SOTA-Host → Reihe B; prt_art-Host → nullopt.
/// M-CE-10 (2026-07-14): Stufe2 ist per-Host aufgefächert (der frühere HOT-Pilot war die EINE St2-Binary) — jetzt
/// symmetrisch zu Stufe3 (je SOTA-Host eine distinkte reale Komposition, host_lebewesen == lebewesen).
[[nodiscard]] inline std::optional<SotaModule> sota_module_for(std::string const& merge, std::string const& lebewesen) {
    if (merge == "Stufe1_CeOnly") {
        for (auto const& m : build_sota_series_a_modules())
            if (m.lebewesen == lebewesen) return m;
        return std::nullopt;
    }
    std::string const reihe = stufe_to_reihe(merge); // #178: Reihe mechanisch aus der Stufe (A für St2, B für St3)
    if (merge == "Stufe2_PrueflingReplace") {
        // M-CE-10 (per-Host-Stufe2, 2026-07-14, Ledger :1134 — bau-relevant mit F/G): ANALOG zu Stufe3 (unten)
        // fächert Stufe2 jetzt je SOTA-Host eine distinkte <Host>PrtStufe2ReplaceComposition (Host stellt 16 Achsen, INC-2d,
        // PRT-ART ersetzt den path_compression-Slot). Die binary_id ist damit GENUINE per-Host distinkt (verschiedene
        // Hosts = verschiedene Kompositionen — KEIN Fake-id für byte-identischen HOT-Code, der alte Anti-Phantom-
        // Fall entfällt). host_lebewesen == lebewesen (der reale Host trägt den H2-Score). prt_art-als-Host ist
        // degeneriert (in Reihe A/Stufe1 bereits isoliert) → nullopt (ehrlich, keine Stufe1-Duplikation unter St2).
        struct B {
            std::string_view type;
            std::string_view name;
        };
        auto pick = [&](std::string const& l) -> std::optional<B> {
            if (l == "art")
                return B{"::comdare::cache_engine::compositions::ArtPrtStufe2ReplaceComposition",
                         cmp::ArtPrtStufe2ReplaceComposition::name};
            if (l == "hot")
                return B{"::comdare::cache_engine::compositions::HotPrtStufe2ReplaceComposition",
                         cmp::HotPrtStufe2ReplaceComposition::name};
            if (l == "masstree")
                return B{"::comdare::cache_engine::compositions::MasstreePrtStufe2ReplaceComposition",
                         cmp::MasstreePrtStufe2ReplaceComposition::name};
            if (l == "surf")
                return B{"::comdare::cache_engine::compositions::SurfPrtStufe2ReplaceComposition",
                         cmp::SurfPrtStufe2ReplaceComposition::name};
            if (l == "start")
                return B{"::comdare::cache_engine::compositions::StartPrtStufe2ReplaceComposition",
                         cmp::StartPrtStufe2ReplaceComposition::name};
            if (l == "wormhole")
                return B{"::comdare::cache_engine::compositions::WormholePrtStufe2ReplaceComposition",
                         cmp::WormholePrtStufe2ReplaceComposition::name};
            return std::nullopt; // prt_art (degeneriert) + unbekannte
        };
        if (auto b = pick(lebewesen))
            return SotaModule{lebewesen + "+prt(St2)", sota_binary_id(reihe, std::string{b->name}),
                              std::string{b->type}, "compositions/prt_art_merge_reference.hpp",
                              lebewesen}; // host_lebewesen == lebewesen: der per-Host-Replace hat DIESEN Host
        return std::nullopt;
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
                              std::string{b->type}, "compositions/prt_art_merge_reference.hpp",
                              lebewesen}; // host_lebewesen == lebewesen: der per-Host-FullJoin hat DIESEN Host
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
//                   fuellt ALLE 17 Achsen (INC-2d) mit EIGENEN Organen (PrtArtComposition / die 6 SOTA isoliert).
//                   Beleg: pruefling_merge.hpp:93-95 StufeOneAxis=DefaultList; prt_art_reference.hpp:61-80.
//     • abstract == Teilmenge + Host-Fallback == Stufe2/Reihe A oder Stufe3/Reihe B:
//                   der Pruefling fuellt NUR einen Slot (path_compression), 16 Achsen via Host-Fallback (INC-2d).
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
    // GO-5 Fork 6 (2026-07-12, Thesis §sec:fairness): der deklarierte Fairness-Modus der Reihe —
    // "common_denominator" / "native" / "-" (ungesetzt = heutiges Verhalten). REINE Reihen-Metadaten
    // (CSV-Tag fairness_mode + Resume-Stamp), NICHT Teil der binary_id (Tags verschmutzen die
    // Round-Trip-Identitaet nie, LazyRunConfig-Doktrin). Die Kompositions-Pinnung des
    // common_denominator-Falls (value_handle_external + PRT-Spezialpfade aus) ist DATEN-gated
    // (#156/#162-Fenster) und aendert DANN die Komposition selbst (⇒ natuerlicherweise eigene binary_id).
    std::string fairness_mode = "-";
    // M-CE-10 (Voll-Review 2026-07-13; per-Host-Auffächerung 2026-07-14): das Lebewesen, dessen H2-Code-Qualitäts-
    // Score dieser Pass trägt — host-dominant (== SotaModule::host_lebewesen). Für St1/St2/St3 identisch zu
    // `lebewesen` (nach der per-Host-Auffächerung ist auch die St2-Komposition per-Host, host_lebewesen == lebewesen).
    // Der Treiber löst den CSV-Score über DIESES Feld auf (h2_score_for) — prt_art/fehlende Akte ⇒ honest "n/a".
    std::string h2_lebewesen = "-";
};

/// make_sota_pass — die EINE Stelle, die aus einem (merge, lebewesen [, series_id, pruefling_type-Override,
/// fairness])-Eintrag GENAU EINEN SotaPass baut (Single-Source der Pass-Bildung). nullopt, wenn das
/// (merge,lebewesen)-Paar in diesem Pilot NICHT real baubar ist (sota_module_for == nullopt → ehrlich
/// ausgelassen, KEIN Phantom-Pass). Die DEDUP der Pass-Liste bleibt beim Aufrufer (er iteriert die
/// Eintrags-/Reihen-Liste und führt den (view_binary_id, fairness_mode)-Guard). Genutzt von BEIDEN
/// Eingangsformen: build_sota_passes(ThesisProfile) (die <sota_series>-Reihen) UND build_sota_passes(pairs)
/// (die Brücken-(merge×lebewesen)-Paare, E5 / Fork-5 (b), Ledger:377). #178: dispatch auf die STUFE (merge).
[[nodiscard]] inline std::optional<SotaPass> make_sota_pass(std::string const& merge, std::string const& lebewesen,
                                                            std::string const& series_id,
                                                            std::string const& pruefling_type_override,
                                                            std::string const& fairness_raw) {
    auto const m = sota_module_for(merge, lebewesen);   // #178: dispatch auf die Stufe (merge)
    if (!m) return std::nullopt;                        // nicht baubares Paar (ehrlich: kein Phantom-Pass)
    std::string const reihe    = stufe_to_reihe(merge); // #178: Reihen-Tag mechanisch aus der Stufe
    std::string const view_bid = sota_view_binary_id(m->binary_id);
    std::string const fair     = fairness_raw.empty() ? std::string{"-"} : fairness_raw; // GO-5 Fork 6
    return SotaPass{reihe, lebewesen, m->binary_id, view_bid,
                    // #171: full/abstract. F25 (WP-4, 2026-07-16): der 1. derive_pruefling_type-Parameter ist laut
                    // Funktions-Vertrag die PROFIL-series-id (Fallback bei leerem/unbekanntem merge). Byte-
                    // verhaltensgleich fuer alle erreichbaren Pfade: dieser Punkt ist nur mit gueltigem merge
                    // erreichbar (sota_module_for != nullopt) → der merge-Primaerzweig greift, series_id ist dann
                    // ungenutzt (die Brücken-Paare reichen "" herein, ThesisProfile reicht s.id herein).
                    derive_pruefling_type(series_id, merge, pruefling_type_override), fair,
                    // M-CE-10 (c): H2 host-dominant. KORREKTUR F23 (2026-07-16): seit der per-Host-Auffaecherung
                    // (2026-07-14) ist host_lebewesen == lebewesen (Gate: test_sota_st2_dedup, 19 Paesse).
                    m->host_lebewesen};
}

/// build_sota_passes(profile) — die Liste der SOTA-Reihen-Pässe AUS DEM PROFIL (1 Eintrag je real baubarem
/// <sota_series>). Reihenfolge = Profil-Reihenfolge (stabil/resumierbar). Ein nicht baubares (Reihe,Lebewesen)-
/// Paar (sota_module_for == nullopt) wird ausgelassen (ehrlich: kein Phantom-Pass). KEINE Selektion — das
/// Profil bestimmt, WELCHE Reihen/Lebewesen laufen.
[[nodiscard]] inline std::vector<SotaPass> build_sota_passes(cx::ThesisProfile const& tp) {
    std::vector<SotaPass> out;
    out.reserve(tp.sota_series.size());
    // ── M-CE-10 (Voll-Review 2026-07-13; per-Host-Auffächerung 2026-07-14) — SEMANTIK-ENTSCHEIDUNG ─────────────
    //   FRÜHER (2026-07-13) war Stufe2_PrueflingReplace KONZEPTIONELL EINE Binary (der HOT-Pilot): sota_module_for
    //   bildete die binary_id OHNE das angefragte lebewesen, mehrere <sota_series merge="Stufe2_.."> mit
    //   VERSCHIEDENEN lebewesen materialisierten DIESELBE Messung → Dedup kollabierte sie zu 1 Pass. Der damalige
    //   Anti-Phantom-Vorbehalt ("lebewesen in die binary_id zu ziehen = N FAKE-distinkte-ids für byte-identischen
    //   HOT-Code") galt, WEIL nur der HOT-Host existierte. JETZT (F/G-relevant, Ledger :1134) faechert Stufe2 je
    //   SOTA-Host eine EIGENE reale <Host>PrtStufe2ReplaceComposition (prt_art_merge_reference.hpp) — die binary_ids
    //   sind damit GENUINE per-Host distinkt (verschiedene Hosts = verschiedene Kompositionen, KEIN Fake-id), genau
    //   wie Stufe3. Die Mechanik bleibt:
    //     (a) DEDUP der Pässe auf (view_binary_id, fairness_mode): kollabiert nur noch WIRKLICH identische Messungen
    //         (gleicher Host + gleicher fairness_mode). Die LEGITIMEN fairness-Varianten (gleiche binary_id,
    //         verschiedener fairness_mode — DATEN-gated #156-Pinnung) bleiben getrennt; per-Host-St2 sind distinkt.
    //     (c) H2-ATTRIBUTION host-dominant: der Pass trägt den H2-Score seines HOST-Lebewesens (m->host_lebewesen);
    //         nach der Auffächerung ist der Host == das angefragte lebewesen (konsistent zu St1/St3).
    //   (b) Der Zähler res.sota_binary_ids zählt im Treiber (profile_run_entry) weiterhin DISTINKTE binary_ids.
    // ──────────────────────────────────────────────────────────────────────────────────────────────────────────
    std::set<std::pair<std::string, std::string>> seen_pass; // Dedup-Schlüssel: (view_binary_id, fairness_mode)
    for (auto const& s : tp.sota_series) {
        // Single-Source der Pass-Bildung (make_sota_pass): #178 dispatch auf die Stufe (merge), ehrlich-nullopt
        // bei nicht baubarem Paar, F25 series-id-Fallback (s.id) + F23 host-dominante H2 leben im Helfer.
        auto p = make_sota_pass(s.merge, s.lebewesen, s.id, s.pruefling_type, s.fairness);
        if (!p) continue;                                                   // nicht baubar (kein Phantom-Pass)
        if (!seen_pass.emplace(p->view_binary_id, p->fairness_mode).second) // M-CE-10 (a): identische Messung
            continue;                                                       //   ⇒ genau 1 Pass
        out.push_back(std::move(*p));
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

// ─────────────────────────────────────────────────────────────────────────────
// BRÜCKE / E5 (Fork-5 (b), Ledger:377 „schmale (merge,lebewesen)-Overloads in sota_catalog") — die SCHMALEN
//   (merge×lebewesen)-Paar-Overloads. Sie SPIEGELN die ThesisProfile-Bausteine (build_sota_passes /
//   build_sota_view_source_map), aber der Eingang ist eine explizite (merge×lebewesen)-Paar-Liste statt einer
//   <sota_series_set>-Sicht. Damit projiziert die comdare_experiment-Brücke OHNE synthetische ThesisProfile-
//   Sicht auf denselben SOTA-Pass-Unterbau (dieselben Primitive make_sota_pass / sota_module_for /
//   sota_view_binary_id / render_sota_module_source). ADDITIV — die ThesisProfile-Signaturen bleiben unberührt.
// ─────────────────────────────────────────────────────────────────────────────

/// SotaMergeLebewesen — ein schmales (merge,lebewesen)-Paar, die Brücken-Eingangsform. Trägt NUR die 2
/// mechanisch nötigen Felder; series_id/pruefling_type werden (wie im ThesisProfile-Pfad) aus `merge`
/// ABGELEITET (derive_pruefling_type), fairness bleibt "-" (die Experiment-Phase deklariert keinen Fairness-Modus).
struct SotaMergeLebewesen {
    std::string merge;     // Stufe1_CeOnly / Stufe2_PrueflingReplace / Stufe3_FullJoin
    std::string lebewesen; // prt_art/art/hot/masstree/surf/start/wormhole
};

/// build_sota_passes(pairs) — Overload: die SOTA-Reihen-Pässe AUS einer expliziten (merge×lebewesen)-Paar-Liste
/// (Brücken-Eingang). 1 Eintrag je real baubarem Paar (make_sota_pass != nullopt); nicht baubare Paare
/// (z.B. prt_art unter Stufe2/Stufe3) werden EHRLICH ausgelassen (kein Phantom-Pass). Dedup IDENTISCH zum
/// ThesisProfile-Pfad: (view_binary_id, fairness_mode). Reihenfolge = Paar-Reihenfolge (stabil).
[[nodiscard]] inline std::vector<SotaPass> build_sota_passes(std::vector<SotaMergeLebewesen> const& pairs) {
    std::vector<SotaPass>                         out;
    std::set<std::pair<std::string, std::string>> seen_pass; // Dedup-Schlüssel: (view_binary_id, fairness_mode)
    out.reserve(pairs.size());
    for (auto const& pr : pairs) {
        // series_id/pruefling_type-Override "" ⇒ derive_pruefling_type leitet rein aus `merge` ab (full/abstract);
        // fairness "" ⇒ CSV-Tag "-". Identische Bau-Semantik wie der ThesisProfile-Pfad (make_sota_pass).
        auto p = make_sota_pass(pr.merge, pr.lebewesen, /*series_id=*/"", /*pruefling_type_override=*/"",
                                /*fairness_raw=*/"");
        if (!p) continue;                                                   // nicht baubar (kein Phantom-Pass)
        if (!seen_pass.emplace(p->view_binary_id, p->fairness_mode).second) // identische Messung ⇒ genau 1 Pass
            continue;
        out.push_back(std::move(*p));
    }
    return out;
}

/// build_sota_view_source_map(pairs) — Overload: die Quellen-Map (view_binary_id → reale Modul-Quelle) AUS einer
/// (merge×lebewesen)-Paar-Liste. Byte-gleich zum ThesisProfile-Pfad, nur die Eingangsform unterscheidet sich.
/// Nicht baubare Paare (sota_module_for == nullopt) erzeugen KEINEN Key (ehrlich, kein Phantom-Key).
[[nodiscard]] inline std::map<std::string, std::string>
build_sota_view_source_map(std::vector<SotaMergeLebewesen> const& pairs) {
    std::map<std::string, std::string> by_id;
    for (auto const& pr : pairs)
        if (auto m = sota_module_for(pr.merge, pr.lebewesen))
            by_id.emplace(sota_view_binary_id(m->binary_id), render_sota_module_source(m->composition_type, m->header));
    return by_id;
}

// ─────────────────────────────────────────────────────────────────────────────
// BRÜCKE I3 — die PROJEKTION ExperimentProfile → (merge×lebewesen)-Pässe/Quellen-Map. REINER Enumerations-/
//   Render-Schritt: KEIN DLL-Bau, KEINE Messung, KEIN run_lazy_static_then_dynamic. Je <phase> das Kreuzprodukt
//   phase.merge × profile.lebewesen; je real baubares Paar EIN SotaPass (Muster build_sota_passes) + die
//   Quellen-Map keyed view_binary_id (Muster build_sota_view_source_map). Die dünne Lauf-Fassade (I4) hängt
//   hier den bestehenden run_profile-Unterbau (echte DLLs, EINE CSV) an — I3 liefert NUR die Projektion.
// ─────────────────────────────────────────────────────────────────────────────

/// Die Projektion EINER ExperimentPhase auf den SOTA-Pass-Unterbau. passes und source_by_view_id sind
/// über den view_binary_id 1:1 verknüpft (jeder Pass-view_binary_id ist genau ein Map-Key).
struct ExperimentPhaseProjection {
    std::string                        phase_name;        // ExperimentPhase.name (Provenienz/Log)
    std::string                        merge;             // ExperimentPhase.merge (die Stufe dieser Phase)
    std::vector<SotaPass>              passes;            // je (phase.merge × lebewesen) real baubares Paar EIN Pass
    std::map<std::string, std::string> source_by_view_id; // view_binary_id → render_sota_module_source
};

/// project_experiment_to_sota_passes — die Brücken-Projektion (I3). Je <phase> wird das Kreuzprodukt
/// phase.merge × profile.lebewesen gebildet und über die schmalen (merge,lebewesen)-Overloads auf Pässe +
/// Quellen-Map abgebildet. nullopt-Paare (z.B. prt_art unter Stufe2/Stufe3) werden EHRLICH ausgelassen (kein
/// Phantom-Key/-Pass — Muster build_sota_passes:323). Reihenfolge = Phasen-Reihenfolge × lebewesen-Reihenfolge
/// (stabil/resumierbar). KEIN Bau, KEIN Lauf.
[[nodiscard]] inline std::vector<ExperimentPhaseProjection>
project_experiment_to_sota_passes(cx::ExperimentProfile const& ep) {
    std::vector<ExperimentPhaseProjection> out;
    out.reserve(ep.phases.size());
    for (auto const& phase : ep.phases) {
        std::vector<SotaMergeLebewesen> pairs;
        pairs.reserve(ep.lebewesen.size());
        for (auto const& l : ep.lebewesen) pairs.push_back(SotaMergeLebewesen{phase.merge, l});
        out.push_back(ExperimentPhaseProjection{phase.name, phase.merge, build_sota_passes(pairs),
                                                build_sota_view_source_map(pairs)});
    }
    return out;
}

} // namespace comdare::cache_engine::thesis_lazy
