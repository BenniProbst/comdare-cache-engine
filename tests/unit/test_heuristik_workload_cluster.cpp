// T-10 (2026-08-07): Unit-Test der OFFLINE-Haelfte der Workload-Cluster-Bildung.
// Plan-Quelle: super docs/audits/20260709-workload-cluster-research-BEFUND.md (Abschnitte 1, 2.4, 3.2, 5.2).
//
// Beweist (deterministische Fixtures, bekannte Cluster-Struktur):
//   (1) KORREKTE ZUORDNUNG: 9 Punkte in 3 klar getrennten Gruppen -> exakte Etiketten, analytische
//       Zentroide, zentrums-naechste Repraesentanten {0,3,6}, WCSS analytisch 0.04, Silhouette > 0.9.
//   (2) BISSPROBE k-MEANS: bewusst falsch gesetzte Saat-Zentroide -> Lloyd iteriert auf die exakten
//       Mitglieds-Mittel (eine No-Op-"Verfeinerung" fiele hier durch).
//   (3) BISSPROBE NORMALISIERUNG (BEFUND :82-89): identischer Korpus, identischer Algorithmus --
//       Plan-Skalierung clustert nach Lastprofil-Archetyp, Roh-Skalen clustern nach Working-Set-
//       Groessenordnung (genau die Dominanz-Falle, vor der der BEFUND warnt).
//   (4) LINKAGE TRAEGT VERHALTEN: Single- vs Complete-Linkage liefern auf einer Ketten-Fixture
//       verschiedene Partitionen (der CT-Parameter ist nicht dekorativ).
//   (5) GEGENPROBE DEGENERIERT: leer / k=0 / k>n / NaN -> std::nullopt; Einzelpunkt -> triviales,
//       ehrliches Ergebnis; alle Punkte identisch -> leerer Cluster BLEIBT sichtbar leer
//       (cluster_size 0, Repraesentant nullopt, Silhouette nullopt) -- kein Absturz, nichts erfunden.
//   (6) Online-Bruecke (BEFUND 5.2): Nearest-Centroid mit noise-Schwellwert -> Zuordnung bzw.
//       Default-Fallback (std::nullopt).
//   (7) Merkmalsvektor-Naht: Concept-Passung gegen das REALE WorkloadProfileAggregate (nur gelesen).

#include "heuristik/workload_cluster_offline.hpp"
#include "heuristik/workload_feature_vector.hpp"

#include <builder/cacheline_policy/cacheline_policy_selector.hpp> // WorkloadProfileAggregate (NUR Concept-Beleg)

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace h = ::comdare::cache_engine::heuristik;

namespace {

using P2  = h::FeaturePoint<2>;
using P1  = h::FeaturePoint<1>;
using P13 = h::FeaturePoint<h::kWorkloadFeatureCount>;

// Fixture (1): drei klar getrennte Gruppen, Abstand innerhalb <= 0.15, zwischen ~10.
std::vector<P2> three_group_points() {
    return {P2{0.0, 0.0},  P2{0.1, 0.0},  P2{0.0, 0.1},    // Gruppe um (0,0)   -> Cluster 0
            P2{10.0, 0.0}, P2{10.1, 0.0}, P2{10.0, -0.1},  // Gruppe um (10,0)  -> Cluster 1
            P2{0.0, 10.0}, P2{0.0, 10.1}, P2{-0.1, 10.0}}; // Gruppe um (0,10) -> Cluster 2
}

// --------------------------------------------------------------------------------------------------
// (1) Bekannte Cluster-Struktur -> exakte Zuordnung, analytische Zentroide/WCSS, Repraesentanten.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, ThreeGroupsExactAssignment) {
    std::vector<P2> const pts = three_group_points();
    auto const            res = h::cluster_offline<h::HierarchicalLinkage::Average, 2>(pts, 3, 100);
    ASSERT_TRUE(res.has_value());

    std::vector<std::size_t> const expected = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    EXPECT_EQ(res->labels, expected);
    EXPECT_EQ(res->hierarchy_labels, expected); // der Schnitt trifft schon exakt; k-Means bestaetigt ihn
    EXPECT_EQ(res->cluster_sizes, (std::vector<std::size_t>{3, 3, 3}));
    EXPECT_TRUE(res->kmeans_converged);
    ASSERT_EQ(res->dendrogram.size(), 8u); // n-1 Verschmelzungen

    // Analytische Zentroide: Mittel der Gruppen ((1/30, 1/30) usw.).
    ASSERT_EQ(res->centroids.size(), 3u);
    EXPECT_NEAR(res->centroids[0][0], 0.1 / 3.0, 1e-12);
    EXPECT_NEAR(res->centroids[0][1], 0.1 / 3.0, 1e-12);
    EXPECT_NEAR(res->centroids[1][0], 30.1 / 3.0, 1e-12);
    EXPECT_NEAR(res->centroids[1][1], -0.1 / 3.0, 1e-12);
    EXPECT_NEAR(res->centroids[2][0], -0.1 / 3.0, 1e-12);
    EXPECT_NEAR(res->centroids[2][1], 30.1 / 3.0, 1e-12);

    // Zentrums-naechste Repraesentanten (OtterTune-Muster): je Gruppe der unverrueckte Eckpunkt.
    ASSERT_EQ(res->representatives.size(), 3u);
    EXPECT_EQ(res->representatives[0], std::optional<std::size_t>{0});
    EXPECT_EQ(res->representatives[1], std::optional<std::size_t>{3});
    EXPECT_EQ(res->representatives[2], std::optional<std::size_t>{6});

    // WCSS analytisch: je Gruppe 12/900 -> gesamt 36/900 = 0.04.
    EXPECT_NEAR(res->wcss, 0.04, 1e-12);

    // Klar getrennte Gruppen -> hohe mittlere Silhouette.
    ASSERT_TRUE(res->silhouette.has_value());
    EXPECT_GT(*res->silhouette, 0.9);

    // Dendrogramm-Hoehen steigen monoton (Average-Linkage auf dieser Fixture).
    for (std::size_t t = 1; t < res->dendrogram.size(); ++t)
        EXPECT_GE(res->dendrogram[t].height, res->dendrogram[t - 1].height);
}

TEST(HeuristikWorkloadClusterT10, PipelineIsDeterministic) {
    std::vector<P2> const pts = three_group_points();
    auto const            a   = h::cluster_offline<h::HierarchicalLinkage::Average, 2>(pts, 3, 100);
    auto const            b   = h::cluster_offline<h::HierarchicalLinkage::Average, 2>(pts, 3, 100);
    ASSERT_TRUE(a.has_value());
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(a->labels, b->labels);
    EXPECT_EQ(a->centroids, b->centroids);
    ASSERT_EQ(a->dendrogram.size(), b->dendrogram.size());
    for (std::size_t t = 0; t < a->dendrogram.size(); ++t) {
        EXPECT_EQ(a->dendrogram[t].left, b->dendrogram[t].left);
        EXPECT_EQ(a->dendrogram[t].right, b->dendrogram[t].right);
        EXPECT_EQ(a->dendrogram[t].height, b->dendrogram[t].height); // bitgleich, keine Zufallsquelle
    }
}

// --------------------------------------------------------------------------------------------------
// (2) BISSPROBE k-Means: falsche Saat (4,0)/(10,0) -> Lloyd konvergiert auf die exakten Mittel
// (0.5,0)/(9.5,0). Eine "Verfeinerung", die nur die Saat zurueckgibt, faellt hier durch.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, KMeansRefinementActuallyIterates) {
    std::vector<P2> const pts   = {P2{0.0, 0.0}, P2{1.0, 0.0}, P2{9.0, 0.0}, P2{10.0, 0.0}};
    std::vector<P2>       seeds = {P2{4.0, 0.0}, P2{10.0, 0.0}};
    auto const            res   = h::kmeans_refine<2>(pts, seeds, 100);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->labels, (std::vector<std::size_t>{0, 0, 1, 1}));
    ASSERT_EQ(res->centroids.size(), 2u);
    EXPECT_NEAR(res->centroids[0][0], 0.5, 1e-12); // NICHT die Saat 4.0 -- der Beweis der Iteration
    EXPECT_NEAR(res->centroids[1][0], 9.5, 1e-12);
    EXPECT_TRUE(res->converged);
    EXPECT_GE(res->iterations, 2u);
}

// --------------------------------------------------------------------------------------------------
// (3) BISSPROBE Normalisierung (BEFUND :82-89): zwei Lastprofil-Archetypen (scan-lastig vs punkt-
// lastig), je bei Working-Set 1e3 und 1e6. MIT Plan-Skalierung trennen die Last-BESCHREIBER
// (Op-Mix/theta/neg%) -> Cluster = Archetypen. OHNE Skalierung (Roh-Werte) dominiert die Working-Set-
// Spannweite (~1e6) alles -> Cluster = Groessenordnungen, die Archetypen werden vermischt.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, NormalizationDecidesClusterStructure) {
    h::WorkloadFeatureVector scan_a{};
    scan_a.op_insert_share   = 0.05;
    scan_a.op_lookup_share   = 0.10;
    scan_a.op_scan_share     = 0.80;
    scan_a.op_rmw_share      = 0.05;
    scan_a.access_skew_theta = 0.99;
    h::WorkloadFeatureVector point_b{};
    point_b.op_insert_share       = 0.05;
    point_b.op_lookup_share       = 0.85;
    point_b.op_scan_share         = 0.05;
    point_b.op_rmw_share          = 0.05;
    point_b.negative_lookup_share = 0.3;

    h::WorkloadFeatureVector a_small = scan_a, a_large = scan_a, b_small = point_b, b_large = point_b;
    a_small.working_set_size                           = 1e3;
    a_large.working_set_size                           = 1e6;
    b_small.working_set_size                           = 1e3;
    b_large.working_set_size                           = 1e6;
    std::vector<h::WorkloadFeatureVector> const corpus = {a_small, a_large, b_small, b_large};

    // MIT Plan-Skalierung: Cluster folgen den Archetypen {A,A,B,B}.
    auto const nz = h::WorkloadFeatureNormalizer::fit(corpus);
    ASSERT_TRUE(nz.has_value());
    auto const normalized = nz->apply_corpus(corpus);
    ASSERT_TRUE(normalized.has_value());
    auto const plan = h::cluster_offline<h::HierarchicalLinkage::Average, h::kWorkloadFeatureCount>( //
        *normalized, 2, 100);
    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->labels, (std::vector<std::size_t>{0, 0, 1, 1}));

    // OHNE Skalierung (alle Dimensionen roh): Working-Set dominiert -> {klein, gross} statt {A, B}.
    std::array<h::FeatureScaling, h::kWorkloadFeatureCount> raw{};
    raw.fill(h::FeatureScaling::SimplexShare); // SimplexShare == unveraenderter Durchgriff
    auto const nz_raw = h::WorkloadFeatureNormalizer::fit(corpus, raw);
    ASSERT_TRUE(nz_raw.has_value());
    auto const raw_points = nz_raw->apply_corpus(corpus);
    ASSERT_TRUE(raw_points.has_value());
    auto const unscaled = h::cluster_offline<h::HierarchicalLinkage::Average, h::kWorkloadFeatureCount>( //
        *raw_points, 2, 100);
    ASSERT_TRUE(unscaled.has_value());
    EXPECT_EQ(unscaled->labels, (std::vector<std::size_t>{0, 1, 0, 1})); // Archetypen vermischt
}

// --------------------------------------------------------------------------------------------------
// (4) Linkage traegt Verhalten: Ketten-Fixture {0, 1, 2.1, 3.3} -- Single kettet {0,1,2.1} zusammen,
// Complete schneidet {0,1} | {2.1,3.3}. Der Template-Parameter ist beweisbar nicht dekorativ.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, LinkageChoiceChangesPartition) {
    std::vector<P1> const pts      = {P1{0.0}, P1{1.0}, P1{2.1}, P1{3.3}};
    auto const            single   = h::cluster_offline<h::HierarchicalLinkage::Single, 1>(pts, 2, 100);
    auto const            complete = h::cluster_offline<h::HierarchicalLinkage::Complete, 1>(pts, 2, 100);
    ASSERT_TRUE(single.has_value());
    ASSERT_TRUE(complete.has_value());
    EXPECT_EQ(single->hierarchy_labels, (std::vector<std::size_t>{0, 0, 0, 1}));
    EXPECT_EQ(complete->hierarchy_labels, (std::vector<std::size_t>{0, 0, 1, 1}));
}

TEST(HeuristikWorkloadClusterT10, WardSeparatesTwoBlocks) {
    std::vector<P2> const pts = {P2{0.0, 0.0}, P2{0.0, 1.0}, P2{10.0, 0.0}, P2{10.0, 1.0}};
    auto const            res = h::cluster_offline<h::HierarchicalLinkage::Ward, 2>(pts, 2, 100);
    ASSERT_TRUE(res.has_value());
    EXPECT_EQ(res->labels, (std::vector<std::size_t>{0, 0, 1, 1}));
    ASSERT_EQ(res->dendrogram.size(), 3u);
    EXPECT_GT(res->dendrogram[2].height, res->dendrogram[0].height); // Block-Fusion ist teuerste Fusion
}

// --------------------------------------------------------------------------------------------------
// (5) GEGENPROBE degenerierter Input: ehrliche Ergebnisse, kein Absturz, keine Phantome.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, DegenerateInputsAreHonest) {
    std::vector<P2> const empty;
    // Doppel-Klammer: die Komma-Trennung der Template-Argumente ist sonst ein Makro-Argument-Trenner.
    EXPECT_FALSE((h::cluster_offline<h::HierarchicalLinkage::Average, 2>(empty, 1, 10).has_value()));

    std::vector<P2> const one = {P2{3.5, -1.25}};
    EXPECT_FALSE((h::cluster_offline<h::HierarchicalLinkage::Average, 2>(one, 0, 10).has_value())); // k=0
    EXPECT_FALSE((h::cluster_offline<h::HierarchicalLinkage::Average, 2>(one, 2, 10).has_value())); // k>n

    auto const trivial = h::cluster_offline<h::HierarchicalLinkage::Average, 2>(one, 1, 10);
    ASSERT_TRUE(trivial.has_value());
    EXPECT_EQ(trivial->labels, (std::vector<std::size_t>{0}));
    EXPECT_TRUE(trivial->dendrogram.empty());
    EXPECT_EQ(trivial->centroids[0], (P2{3.5, -1.25}));
    EXPECT_EQ(trivial->representatives[0], std::optional<std::size_t>{0});
    EXPECT_EQ(trivial->wcss, 0.0);
    EXPECT_FALSE(trivial->silhouette.has_value()); // ein Cluster -> Silhouette undefiniert, kein Fake-Wert

    // NaN im Korpus -> ehrlich kein Ergebnis.
    std::vector<P2> const poisoned = {P2{0.0, 0.0}, P2{std::numeric_limits<double>::quiet_NaN(), 1.0}};
    EXPECT_FALSE((h::cluster_offline<h::HierarchicalLinkage::Average, 2>(poisoned, 1, 10).has_value()));
}

TEST(HeuristikWorkloadClusterT10, AllIdenticalPointsKeepEmptyClusterVisible) {
    std::vector<P2> const pts(5, P2{3.5, -1.25});
    auto const            res = h::cluster_offline<h::HierarchicalLinkage::Average, 2>(pts, 2, 100);
    ASSERT_TRUE(res.has_value());
    // Es GIBT keine zwei Cluster in diesen Daten: k-Means zieht alles auf den Index 0, der zweite
    // Cluster bleibt EHRLICH leer sichtbar -- kein stilles Neu-Saehen, kein erfundener zweiter Cluster.
    EXPECT_EQ(res->labels, (std::vector<std::size_t>{0, 0, 0, 0, 0}));
    EXPECT_EQ(res->cluster_sizes, (std::vector<std::size_t>{5, 0}));
    EXPECT_EQ(res->representatives[0], std::optional<std::size_t>{0});
    EXPECT_FALSE(res->representatives[1].has_value()); // leerer Cluster -> KEIN Phantom-Repraesentant
    EXPECT_EQ(res->wcss, 0.0);
    EXPECT_FALSE(res->silhouette.has_value()); // effektiv 1 Cluster -> undefiniert
}

// --------------------------------------------------------------------------------------------------
// (6) Online-Bruecke (BEFUND 5.2): Nearest-Centroid-Zuordnung + noise -> Default-Fallback.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, NearestClusterAssignsOrFallsBack) {
    std::vector<P2> const centroids = {P2{0.0, 0.0}, P2{10.0, 0.0}};

    auto const near0 = h::nearest_cluster<2>(centroids, P2{1.0, 0.0}, 2.0);
    ASSERT_TRUE(near0.has_value());
    EXPECT_EQ(near0->cluster, 0u);
    EXPECT_NEAR(near0->distance, 1.0, 1e-12);

    auto const near1 = h::nearest_cluster<2>(centroids, P2{9.5, 0.0}, 2.0);
    ASSERT_TRUE(near1.has_value());
    EXPECT_EQ(near1->cluster, 1u);
    EXPECT_NEAR(near1->distance, 0.5, 1e-12);

    // Zwischen den Clustern, weiter als der Schwellwert -> "noise" -> Default-Fallback.
    EXPECT_FALSE(h::nearest_cluster<2>(centroids, P2{5.0, 0.0}, 2.0).has_value());
    // Leere Zentroid-Menge / NaN-Last -> ehrlich keine Zuordnung.
    EXPECT_FALSE(h::nearest_cluster<2>(std::vector<P2>{}, P2{0.0, 0.0}, 2.0).has_value());
    EXPECT_FALSE(h::nearest_cluster<2>(centroids, P2{std::numeric_limits<double>::quiet_NaN(), 0.0}, 2.0).has_value());
}

// --------------------------------------------------------------------------------------------------
// Merkmalsvektor + Normalizer: analytische z-Werte, Simplex-Durchgriff, Null-Varianz -> 0, honest-empty.
// --------------------------------------------------------------------------------------------------
TEST(HeuristikWorkloadClusterT10, NormalizerAnalyticValues) {
    h::WorkloadFeatureVector v1{};
    v1.op_scan_share     = 0.25;
    v1.access_skew_theta = 1.0;
    v1.working_set_size  = 0.0;
    h::WorkloadFeatureVector v2{};
    v2.op_scan_share                                   = 0.25;
    v2.access_skew_theta                               = 3.0;
    v2.working_set_size                                = std::expm1(2.0); // log1p davon == 2.0 (analytisch)
    std::vector<h::WorkloadFeatureVector> const corpus = {v1, v2};

    auto const nz = h::WorkloadFeatureNormalizer::fit(corpus);
    ASSERT_TRUE(nz.has_value());
    auto const p1 = nz->apply(v1);
    ASSERT_TRUE(p1.has_value());
    auto const scan_i  = static_cast<std::size_t>(h::WorkloadFeatureIndex::OpScanShare);
    auto const theta_i = static_cast<std::size_t>(h::WorkloadFeatureIndex::AccessSkewTheta);
    auto const ws_i    = static_cast<std::size_t>(h::WorkloadFeatureIndex::WorkingSetSize);
    auto const key_i   = static_cast<std::size_t>(h::WorkloadFeatureIndex::KeyLengthBytes);
    EXPECT_EQ((*p1)[scan_i], 0.25);           // Simplex: unveraenderter Durchgriff (BEFUND :87)
    EXPECT_NEAR((*p1)[theta_i], -1.0, 1e-12); // theta {1,3}: Mittel 2, Pop-Std 1 -> z = -1
    EXPECT_NEAR((*p1)[ws_i], -1.0, 1e-12);    // log1p {0,2}: Mittel 1, Pop-Std 1 -> z = -1
    EXPECT_EQ((*p1)[key_i], 0.0);             // Null-Varianz -> ehrlich 0, nie NaN
    auto const p2 = nz->apply(v2);
    ASSERT_TRUE(p2.has_value());
    EXPECT_NEAR((*p2)[theta_i], 1.0, 1e-12);
    EXPECT_NEAR((*p2)[ws_i], 1.0, 1e-12);

    // honest-empty: leerer Korpus / NaN / negativer log-Eingang.
    EXPECT_FALSE(h::WorkloadFeatureNormalizer::fit({}).has_value());
    h::WorkloadFeatureVector bad_nan{};
    bad_nan.access_skew_theta                         = std::numeric_limits<double>::quiet_NaN();
    std::vector<h::WorkloadFeatureVector> const c_nan = {bad_nan};
    EXPECT_FALSE(h::WorkloadFeatureNormalizer::fit(c_nan).has_value());
    h::WorkloadFeatureVector bad_neg{};
    bad_neg.working_set_size = -1.0; // negatives Working-Set ist ein Messfehler, kein log-Eingang
    std::vector<h::WorkloadFeatureVector> const c_neg = {bad_neg};
    EXPECT_FALSE(h::WorkloadFeatureNormalizer::fit(c_neg).has_value());
}

// --------------------------------------------------------------------------------------------------
// (7) Naht zum bestehenden Heuristik-Kern: das REALE WorkloadProfileAggregate erfuellt das Concept
// (BEFUND :45-48 verankert den Merkmalsraum an genau diesem Typ). Nur gelesen, nichts veraendert.
// --------------------------------------------------------------------------------------------------
static_assert(
    h::WorkloadOpMixAggregateLike<::comdare::cache_engine::builder::cacheline_policy::WorkloadProfileAggregate>);

TEST(HeuristikWorkloadClusterT10, FromOpMixAggregateFillsSharesHonestly) {
    struct LocalAggregate { // Concept-Minimalform (beweist die Naht ohne builder-Abhaengigkeit im Header)
        std::uint64_t op_insert_n   = 1;
        std::uint64_t op_lookup_n   = 2;
        std::uint64_t op_erase_n    = 1;
        std::uint64_t op_scan_n     = 4;
        std::uint64_t op_rmw_n      = 2;
        std::uint64_t working_set_n = 777;
    };
    static_assert(h::WorkloadOpMixAggregateLike<LocalAggregate>);
    h::WorkloadFeatureVector const v = h::from_op_mix_aggregate(LocalAggregate{});
    EXPECT_NEAR(v.op_insert_share, 0.1, 1e-12);
    EXPECT_NEAR(v.op_lookup_share, 0.2, 1e-12);
    EXPECT_NEAR(v.op_erase_share, 0.1, 1e-12);
    EXPECT_NEAR(v.op_scan_share, 0.4, 1e-12);
    EXPECT_NEAR(v.op_rmw_share, 0.2, 1e-12);
    EXPECT_EQ(v.working_set_size, 777.0);
    // Erweiterungs-Features (BEFUND :76-80 "nicht erfasst") bleiben honest-0 -- NICHT erfunden.
    EXPECT_EQ(v.key_length_bytes, 0.0);
    EXPECT_EQ(v.prefix_entropy_bits, 0.0);
    EXPECT_EQ(v.thread_count, 0.0);

    struct EmptyAggregate {
        std::uint64_t op_insert_n = 0, op_lookup_n = 0, op_erase_n = 0, op_scan_n = 0, op_rmw_n = 0, working_set_n = 0;
    };
    h::WorkloadFeatureVector const e = h::from_op_mix_aggregate(EmptyAggregate{});
    EXPECT_EQ(e.op_lookup_share, 0.0); // leere Op-Summe -> ehrlich 0, keine Division durch 0
}

// Fremdes/verstuemmeltes Dendrogramm wird abgewiesen (Wache in cut_dendrogram).
TEST(HeuristikWorkloadClusterT10, CutRejectsForeignDendrogram) {
    std::vector<P2> const pts    = {P2{0.0, 0.0}, P2{1.0, 0.0}, P2{2.0, 0.0}};
    auto const            merges = h::agglomerate_dendrogram<h::HierarchicalLinkage::Average, 2>(pts);
    ASSERT_TRUE(merges.has_value());
    ASSERT_EQ(merges->size(), 2u);
    EXPECT_FALSE(h::cut_dendrogram(4, *merges, 2).has_value()); // n passt nicht zur Verschmelzungs-Folge
    EXPECT_TRUE(h::cut_dendrogram(3, *merges, 2).has_value());
}

} // namespace
