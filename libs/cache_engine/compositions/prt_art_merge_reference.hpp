#pragma once
// STRANG A KORRIGIERT — Increment 5 / S6b (2026-06-18). prt_art_merge_reference: die SOTA-Reihen A + B
// als REALE Lebewesen-Kompositionen über pruefling_merge.hpp (die 3 Kompositionalen Joins, Doku 14 §18-§19).
//
// Reihe A = Verbund1_CeOnly -> Pruefling/SOTA ISOLIERT (PrtArtComposition / die 6 SOTA selbst, prt_art_reference.hpp
//           + known_compositions_list.hpp) UND Verbund2_Replace -> PRT-ART ERSETZT einen Slot
//           (path_compression) einer SOTA-Host-Komposition mit Fallback (HasPruefling_v).
// Reihe B = Verbund3_Union -> Union (non-redundant) aus Host-Default + Pruefling-Varianten; je 1 Punkt =
//           der Pruefling-Repräsentant der gemergten mp_list (AdHocComposition konsumiert genau EIN Tupel pro DLL).
// Reihe C = build-übergreifende Merge/Regression alt↔neu; keine Stufe in diesem Header.
//
// Die Slot-Auswahl ist KEINE neue Code-Selektion: sie folgt mechanisch aus PrueflingVerbundStrategy
// (pruefling_merge.hpp).
// Die Gattung wird per assert_pruefling_slot_genus (Cross-Genus-Join type-system-unmöglich) garantiert.
//
// @doku docs/architektur/14_achsen_komposition_organ_metapher.md §18-§19 (3 Kompositionale Joins)
// @related pruefling_merge.hpp (PrueflingVerbundStrategy/MergeAxis) * i_pruefling_factory.hpp (Abstract Factory)

#include "prt_art_reference.hpp"  // PrtArtComposition + PrtArtPathCompressionOrgan (das Redirect-Organ)
#include "art_reference.hpp"      // ArtComposition (Default-Varianten-Quelle der Host-PC-Achse)
#include "hot_reference.hpp"      // HotComposition (Host fuer Verbund2/Reihe A und Verbund3/Reihe B)
#include "masstree_reference.hpp" // MasstreeComposition (Host fuer Verbund3/Reihe B)
#include "surf_reference.hpp"     // SurfComposition (Host fuer Verbund3/Reihe B)
#include "start_reference.hpp"    // StartComposition (Host fuer Verbund3/Reihe B)
#include "wormhole_reference.hpp" // WormholeComposition (Host fuer Verbund3/Reihe B)

#include "../anatomy/pruefling_merge.hpp" // PrueflingVerbundStrategy / MergeAxis / PrueflingSlotConcept / slot_genus
#include "../anatomy/search_algorithm_permutation_engine.hpp" // assert_pruefling_slot_genus (Gattungs-Constraint)
#include "../anatomy/composition_concept.hpp"

#include <boost/mp11.hpp>
#include <string_view>
#include <organ_axes/persistence_target/axis_persistence_target_memory_only.hpp> // STRUKT-R ORG-18

namespace comdare::cache_engine::compositions {

namespace pf = ::comdare::cache_engine::anatomy::pruefling;
namespace mp = boost::mp11;

// ─────────────────────────────────────────────────────────────────────────────
// (1) Der PRT-ART-Pruefling-SLOT für die path_compression-Achse (Abstract-Factory-/Slot-Pattern, E11).
//     PrueflingVariants = das Redirect-Organ; has_pruefling=true; genus EXPLIZIT SearchAlgorithm.
// ─────────────────────────────────────────────────────────────────────────────
struct PrtArtPathCompressionSlot {
    using PrueflingVariants = mp::mp_list<PrtArtPathCompressionOrgan>;
    static constexpr bool                                           has_pruefling = true;
    static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus =
        ::comdare::cache_engine::anatomy::AnatomyGenus::SearchAlgorithm;
};

// Die Default-Variante der path_compression-Achse einer Host-SOTA (hier: ART/HOT tragen PathCompressionNone).
using HostDefaultPathCompressionVariants = mp::mp_list<nodes::axis_02_path_compression::PathCompressionNone>;

// -- MergeAxis Verbund2/Verbund3 -> die gemergte Varianten-Liste der path_compression-Achse. EIN Punkt je DLL =
//    mp_front (AdHocComposition konsumiert genau ein Tupel). Verbund2 = Pruefling ERSETZT (Front = Redirect-Organ);
//    Verbund3 = Union (Front = Host-Default, das Redirect-Organ ist als 2. Element non-redundant enthalten --
//    der Beleg, dass die Union BEIDE traegt, ist die Listen-Groesse, siehe static_asserts unten). --
using Verbund2MergedPC = pf::MergeAxis<pf::PrueflingVerbundStrategy::Verbund2_Replace,
                                       HostDefaultPathCompressionVariants, PrtArtPathCompressionSlot>;
using Verbund3MergedPC = pf::MergeAxis<pf::PrueflingVerbundStrategy::Verbund3_Union, HostDefaultPathCompressionVariants,
                                       PrtArtPathCompressionSlot>;

static_assert(mp::mp_size<Verbund2MergedPC>::value == 1,
              "Verbund2 ersetzt die Host-Default-Variante komplett (genau die Pruefling-Variante).");
static_assert(mp::mp_size<Verbund3MergedPC>::value == 2,
              "Verbund3 Union traegt Host-Default UND Pruefling-Variante (non-redundant).");

// Verbund2: ERSETZT -> der EINE Punkt ist die Pruefling-Variante (mp_front der 1-elementigen Replace-Liste).
using Verbund2PathCompressionOrgan = mp::mp_front<Verbund2MergedPC>; // = PrtArtPathCompressionOrgan (Patricia, replace)
// Verbund3: die Union traegt BEIDE (Host-Default + Pruefling). Der gemessene Punkt je DLL ist der Pruefling-
// REPRÄSENTANT der Union (mp_back = die hinzugefügte Pruefling-Variante) — die Reihe-B-DLL belegt damit
// nachweislich den Join-Pruefling (nicht den Host-Default, der schon in Reihe A als reines SOTA gemessen wird).
using Verbund3PathCompressionOrgan = mp::mp_back<Verbund3MergedPC>; // = PrtArtPathCompressionOrgan (Patricia, join-rep)

// ─────────────────────────────────────────────────────────────────────────────
// (2) Die Host-Komposition mit dem gemergten path_compression-Slot. Host-Achsen = die SOTA-Host-Komposition
//     (Template-Parameter Host); die path_compression-Achse kommt aus MergeAxis (Stufe 2 oder 3). Verschiedene
//     Hosts in Verbund3/Reihe B liefern distinkte Kompositionen; Verbund2/Reihe A bleibt der bestehende HOT-Pilot.
//     (verschiedene search_algo-Organe + Patricia statt PathCompressionNone ⇒ ≠ A-SOTA und ≠ untereinander).
// ─────────────────────────────────────────────────────────────────────────────
template <class Host, class MergedPathCompression>
struct HostPrtMergeComposition {
    using search_algo        = typename Host::search_algo;
    using cache_traversal    = typename Host::cache_traversal;
    using mapping            = typename Host::mapping;
    using path_compression   = MergedPathCompression; // ← der gemergte Pruefling-Slot (Stufe 2/3)
    using node_type          = typename Host::node_type;
    using memory_layout      = typename Host::memory_layout;
    using allocator          = typename Host::allocator;
    using prefetch           = typename Host::prefetch;
    using concurrency        = typename Host::concurrency;
    using serialization      = typename Host::serialization;
    using value_handle       = typename Host::value_handle;
    using index_organization = typename Host::index_organization;
    using io_dispatch        = typename Host::io_dispatch;
    using migration_policy   = typename Host::migration_policy;
    using filter             = typename Host::filter;
    using queuing_q1         = typename Host::queuing_q1;
    using queuing_q2         = typename Host::queuing_q2;
    // STRUKT-R ORG-18: 18. Organ-Slot (Pflicht, kein Default). MemoryOnlyTarget = Durchreich-Wert:
    // kein Rueckschreib-Pfad. VOLL qualifiziert, weil der Member-Alias den Namespace sonst verdeckt.
    using persistence_target = ::comdare::cache_engine::persistence_target::MemoryOnlyTarget;

    static constexpr std::string_view paper_id = "PRT-ART merge (SOTA host + PRT redirect organ)";
    static constexpr std::string_view paper_title =
        "SOTA host composition with PRT-ART path_compression slot (3 compositional joins)";
};

/// Reihe A (Verbund2_Replace): HOT-Host, path_compression = das PRT-Redirect-Organ (ersetzt PathCompressionNone).
struct HotPrtVerbund2ReplaceComposition : HostPrtMergeComposition<HotComposition, Verbund2PathCompressionOrgan> {
    static constexpr std::string_view name = "HotPrtVerbund2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::HotPrtVerbund2ReplaceComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

// -- M-CE-10 (per-Host-Verbund2, 2026-07-14, Ledger :1134 "per-Host-Verbund2-Kompositionen noetig", bau-relevant mit
//    F/G): der HOT-Pilot war die EINE Verbund2-Binary; damit erzeugten N <sota_series merge="Verbund2_.." lebewesen=X>
//    N Paesse auf DERSELBEN HOT-Binary (Dedup fing es ab, aber die per-Host-Distinktheit fehlte). ANALOG zur
//    Verbund3-Reihe (unten) faechert Verbund2 jetzt je SOTA-Host eine EIGENE <Host>PrtVerbund2ReplaceComposition: der
//    Host stellt 16 Achsen (search_algo/node_type/... je Repository VERSCHIEDEN), path_compression = das PRT-Redirect-
//    Organ (Verbund2PathCompressionOrgan). Damit sind die Verbund2-binary_ids GENUINE per-Host distinkt (KEIN
//    Fake-id fuer byte-identischen HOT-Code -- ArtHost != HotHost != ...): der frueher zurecht kritisierte
//    Anti-Phantom-Fall ("N FAKE-distinkte-ids fuer byte-identischen HOT-Code") entfaellt, weil die Kompositionen
//    real verschieden sind. prt_art-als-Host bleibt degeneriert (in Reihe A/Verbund1 bereits isoliert) -> im Katalog
//    nullopt. Die Abstraktheit des Merge-Slots bleibt unberuehrt (path_compression-Slot, 18 Host-Achsen, W4 :969).
/// Reihe A (Verbund2_Replace): ART-Host, path_compression = das PRT-Redirect-Organ.
struct ArtPrtVerbund2ReplaceComposition : HostPrtMergeComposition<ArtComposition, Verbund2PathCompressionOrgan> {
    static constexpr std::string_view name = "ArtPrtVerbund2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::ArtPrtVerbund2ReplaceComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe A (Verbund2_Replace): MASSTREE-Host, path_compression = das PRT-Redirect-Organ.
struct MasstreePrtVerbund2ReplaceComposition
    : HostPrtMergeComposition<MasstreeComposition, Verbund2PathCompressionOrgan> {
    static constexpr std::string_view name = "MasstreePrtVerbund2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::MasstreePrtVerbund2ReplaceComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe A (Verbund2_Replace): SuRF-Host, path_compression = das PRT-Redirect-Organ.
struct SurfPrtVerbund2ReplaceComposition : HostPrtMergeComposition<SurfComposition, Verbund2PathCompressionOrgan> {
    static constexpr std::string_view name = "SurfPrtVerbund2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::SurfPrtVerbund2ReplaceComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe A (Verbund2_Replace): START-Host, path_compression = das PRT-Redirect-Organ.
struct StartPrtVerbund2ReplaceComposition : HostPrtMergeComposition<StartComposition, Verbund2PathCompressionOrgan> {
    static constexpr std::string_view name = "StartPrtVerbund2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::StartPrtVerbund2ReplaceComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe A (Verbund2_Replace): Wormhole-Host, path_compression = das PRT-Redirect-Organ.
struct WormholePrtVerbund2ReplaceComposition
    : HostPrtMergeComposition<WormholeComposition, Verbund2PathCompressionOrgan> {
    static constexpr std::string_view name = "WormholePrtVerbund2ReplaceComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::WormholePrtVerbund2ReplaceComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe B (Verbund3_Union): MASSTREE-Host, path_compression = Pruefling-Repraesentant der Union (non-redundant).
struct MasstreePrtVerbund3UnionComposition
    : HostPrtMergeComposition<MasstreeComposition, Verbund3PathCompressionOrgan> {
    static constexpr std::string_view name = "MasstreePrtVerbund3UnionComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::MasstreePrtVerbund3UnionComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe B (Verbund3_Union): ART-Host, path_compression = Pruefling-Repraesentant der Union (non-redundant).
struct ArtPrtVerbund3UnionComposition : HostPrtMergeComposition<ArtComposition, Verbund3PathCompressionOrgan> {
    static constexpr std::string_view name = "ArtPrtVerbund3UnionComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::ArtPrtVerbund3UnionComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe B (Verbund3_Union): HOT-Host, path_compression = Pruefling-Repraesentant der Union (non-redundant).
struct HotPrtVerbund3UnionComposition : HostPrtMergeComposition<HotComposition, Verbund3PathCompressionOrgan> {
    static constexpr std::string_view name = "HotPrtVerbund3UnionComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::HotPrtVerbund3UnionComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe B (Verbund3_Union): SuRF-Host, path_compression = Pruefling-Repraesentant der Union (non-redundant).
struct SurfPrtVerbund3UnionComposition : HostPrtMergeComposition<SurfComposition, Verbund3PathCompressionOrgan> {
    static constexpr std::string_view name = "SurfPrtVerbund3UnionComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::SurfPrtVerbund3UnionComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe B (Verbund3_Union): START-Host, path_compression = Pruefling-Repraesentant der Union (non-redundant).
struct StartPrtVerbund3UnionComposition : HostPrtMergeComposition<StartComposition, Verbund3PathCompressionOrgan> {
    static constexpr std::string_view name = "StartPrtVerbund3UnionComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::StartPrtVerbund3UnionComposition",
                                        "compositions/prt_art_merge_reference.hpp");
};

/// Reihe B (Verbund3_Union): Wormhole-Host, path_compression = Pruefling-Repraesentant der Union (non-redundant).
struct WormholePrtVerbund3UnionComposition
    : HostPrtMergeComposition<WormholeComposition, Verbund3PathCompressionOrgan> {
    static constexpr std::string_view name = "WormholePrtVerbund3UnionComposition";
    COMDARE_DEFINE_COMPOSITION_LOCATION("::comdare::cache_engine::compositions::WormholePrtVerbund3UnionComposition",
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
} // namespace detail

} // namespace comdare::cache_engine::compositions
