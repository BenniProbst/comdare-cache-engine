#pragma once
// STRANG A KORRIGIERT — Increment 5 / S6b (2026-06-18). prt_art_merge_reference: die SOTA-Reihen B + C
// als REALE Lebewesen-Kompositionen über pruefling_merge.hpp (die 3 Kompositionalen Joins, Doku 14 §18-§19).
//
// Reihe A = Stufe1_CeOnly → Pruefling/SOTA ISOLIERT (PrtArtComposition / die 6 SOTA selbst, prt_art_reference.hpp
//           + known_compositions_list.hpp). KEINE Merge-Komposition nötig (jede ist 1 reale DLL).
// Reihe B = Stufe2_PrueflingReplace → PRT-ART ERSETZT einen Slot (path_compression) einer SOTA-Host-Komposition
//           mit Fallback (HasPruefling_v). Ergebnis: Host-SOTA + Redirect-Organ = distinkte DLL.
// Reihe C = Stufe3_FullJoin → Union (non-redundant) aus Host-Default + Pruefling-Varianten; je 1 Punkt =
//           das erste Element der gemergten mp_list (AdHocComposition konsumiert genau EIN Tupel pro DLL).
//
// Die Slot-Auswahl ist KEINE neue Code-Selektion: sie folgt mechanisch aus MergeStrategy (pruefling_merge.hpp).
// Die Gattung wird per assert_pruefling_slot_genus (Cross-Genus-Join type-system-unmöglich) garantiert.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §18-§19 (3 Kompositionale Joins)
// @related pruefling_merge.hpp (MergeStrategy/MergeAxis) · i_pruefling_factory.hpp (Abstract Factory)

#include "prt_art_reference.hpp"            // PrtArtComposition + PrtArtPathCompressionOrgan (das Redirect-Organ)
#include "art_reference.hpp"                // ArtComposition (Default-Varianten-Quelle der Host-PC-Achse)
#include "hot_reference.hpp"                // HotComposition (Host für Reihe B)
#include "masstree_reference.hpp"           // MasstreeComposition (Host für Reihe C)

#include "../anatomy/pruefling_merge.hpp"   // MergeStrategy / MergeAxis / PrueflingSlotConcept / slot_genus
#include "../anatomy/search_algorithm_permutation_engine.hpp"   // assert_pruefling_slot_genus (Gattungs-Constraint)
#include "../anatomy/composition_concept.hpp"

#include <boost/mp11.hpp>
#include <string_view>

namespace comdare::cache_engine::compositions {

namespace pf = ::comdare::cache_engine::anatomy::pruefling;
namespace mp = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// (1) Der PRT-ART-Pruefling-SLOT für die path_compression-Achse (Abstract-Factory-/Slot-Pattern, E11).
//     PrueflingVariants = das Redirect-Organ; has_pruefling=true; genus EXPLIZIT SearchAlgorithm.
// ─────────────────────────────────────────────────────────────────────────────
struct PrtArtPathCompressionSlot {
    using PrueflingVariants = mp::mp_list<PrtArtPathCompressionOrgan>;
    static constexpr bool has_pruefling = true;
    static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus =
        ::comdare::cache_engine::anatomy::AnatomyGenus::SearchAlgorithm;
};

// Die Default-Variante der path_compression-Achse einer Host-SOTA (hier: ART/HOT tragen PathCompressionNone).
using HostDefaultPathCompressionVariants =
    mp::mp_list<nodes::axis_02_path_compression::PathCompressionNone>;

// ── MergeAxis Stufe2/Stufe3 → die gemergte Varianten-Liste der path_compression-Achse. EIN Punkt je DLL =
//    mp_front (AdHocComposition konsumiert genau ein Tupel). Stufe2 = Pruefling ERSETZT (Front = Redirect-Organ);
//    Stufe3 = FullJoin Union (Front = Host-Default, das Redirect-Organ ist als 2. Element non-redundant enthalten —
//    der Beleg, dass FullJoin BEIDE trägt, ist die Listen-Größe, siehe static_asserts unten). ──
using Stufe2MergedPC = pf::MergeAxis<pf::MergeStrategy::Stufe2_PrueflingReplace,
                                     HostDefaultPathCompressionVariants, PrtArtPathCompressionSlot>;
using Stufe3MergedPC = pf::MergeAxis<pf::MergeStrategy::Stufe3_FullJoin,
                                     HostDefaultPathCompressionVariants, PrtArtPathCompressionSlot>;

static_assert(mp::mp_size<Stufe2MergedPC>::value == 1,
              "Stufe2 ersetzt die Host-Default-Variante komplett (genau die Pruefling-Variante).");
static_assert(mp::mp_size<Stufe3MergedPC>::value == 2,
              "Stufe3 FullJoin trägt Host-Default UND Pruefling-Variante (non-redundant).");

// Stufe2: ERSETZT → der EINE Punkt ist die Pruefling-Variante (mp_front der 1-elementigen Replace-Liste).
using Stufe2PathCompressionOrgan = mp::mp_front<Stufe2MergedPC>;   // = PrtArtPathCompressionOrgan (Patricia, replace)
// Stufe3: FullJoin trägt BEIDE (Host-Default + Pruefling). Der gemessene Punkt je DLL ist der Pruefling-
// REPRÄSENTANT der Union (mp_back = die hinzugefügte Pruefling-Variante) — die Reihe-C-DLL belegt damit
// nachweislich den Join-Pruefling (nicht den Host-Default, der schon in Reihe A als reines SOTA gemessen wird).
using Stufe3PathCompressionOrgan = mp::mp_back<Stufe3MergedPC>;    // = PrtArtPathCompressionOrgan (Patricia, join-rep)

// ─────────────────────────────────────────────────────────────────────────────
// (2) Die Host-Komposition mit dem gemergten path_compression-Slot. Host-Achsen = die SOTA-Host-Komposition
//     (Template-Parameter Host); die path_compression-Achse kommt aus MergeAxis (Stufe 2 oder 3). Verschiedene
//     Hosts je Reihe (HOT für B, MASSTREE für C) ⇒ die binary_ids von A/B/C sind GARANTIERT distinkt
//     (verschiedene search_algo-Organe + Patricia statt PathCompressionNone ⇒ ≠ A-SOTA und ≠ untereinander).
// ─────────────────────────────────────────────────────────────────────────────
template <class Host, class MergedPathCompression>
struct HostPrtMergeComposition {
    using search_algo        = typename Host::search_algo;
    using cache_traversal    = typename Host::cache_traversal;
    using mapping            = typename Host::mapping;
    using path_compression   = MergedPathCompression;        // ← der gemergte Pruefling-Slot (Stufe 2/3)
    using node_type          = typename Host::node_type;
    using memory_layout      = typename Host::memory_layout;
    using allocator          = typename Host::allocator;
    using prefetch           = typename Host::prefetch;
    using concurrency        = typename Host::concurrency;
    using serialization      = typename Host::serialization;
    using telemetry          = typename Host::telemetry;
    using value_handle       = typename Host::value_handle;
    using isa                = typename Host::isa;
    using index_organization = typename Host::index_organization;
    using io_dispatch        = typename Host::io_dispatch;
    using migration_policy   = typename Host::migration_policy;
    using filter             = typename Host::filter;
    using queuing_q1         = typename Host::queuing_q1;
    using queuing_q2         = typename Host::queuing_q2;

    static constexpr std::string_view paper_id    = "PRT-ART merge (SOTA host + PRT redirect organ)";
    static constexpr std::string_view paper_title = "SOTA host composition with PRT-ART path_compression slot (3 compositional joins)";
};

/// Reihe B (Stufe2_PrueflingReplace): HOT-Host, path_compression = das PRT-Redirect-Organ (ersetzt PathCompressionNone).
struct HotPrtStufe2ReplaceComposition : HostPrtMergeComposition<HotComposition, Stufe2PathCompressionOrgan> {
    static constexpr std::string_view name = "HotPrtStufe2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::HotPrtStufe2ReplaceComposition",
        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe C (Stufe3_FullJoin): MASSTREE-Host, path_compression = Pruefling-Repräsentant der Union (non-redundant).
struct MasstreePrtStufe3FullJoinComposition : HostPrtMergeComposition<MasstreeComposition, Stufe3PathCompressionOrgan> {
    static constexpr std::string_view name = "MasstreePrtStufe3FullJoinComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION(
        "::comdare::cache_engine::compositions::MasstreePrtStufe3FullJoinComposition",
        "compositions/prt_art_merge_reference.hpp");
};

// ─────────────────────────────────────────────────────────────────────────────
// (3) GATTUNGS-CONSTRAINT (assert_pruefling_slot_genus). Compile-Zeit-Beleg, dass der PRT-ART-Slot zur
//     SearchAlgorithm-Gattung gehört (Cross-Genus-Join type-system-mathematisch unmöglich, Doku 14 §32).
// ─────────────────────────────────────────────────────────────────────────────
namespace detail {
    using GenusGuardEngine = ::comdare::cache_engine::anatomy::SearchAlgorithmPermutationEngine<>;
    inline constexpr void assert_prt_art_slot_genus() noexcept {
        GenusGuardEngine::assert_pruefling_slot_genus<PrtArtPathCompressionSlot>();
    }
    static_assert(GenusGuardEngine::slots_match_genus_v<PrtArtPathCompressionSlot>,
                  "PRT-ART-Pruefling-Slot muss zur SearchAlgorithm-Gattung gehören (Gattungs-Constraint).");
}  // namespace detail

}  // namespace comdare::cache_engine::compositions
