#pragma once
// AXIS_ALGO_VERSION: 1
// heuristik/workload_cluster_offline.hpp -- die OFFLINE-Haelfte der Workload-Cluster-Bildung (T-10).
//
// QUELLE (autoritativ, NICHT erfunden): super `docs/audits/20260709-workload-cluster-research-BEFUND.md`
//   Abschnitt 2.4 Empfehlung (1): "Offline-Auswertungsmodus -> agglomerative Hierarchie als
//   Primaer-Verfahren, k-Means als Verfeinerung. ... Auf dem im Dendrogramm gewaehlten k k-Means
//   nachlagern, um je Cluster den zentrums-naechsten Repraesentanten zu bestimmen
//   (OtterTune-/Subsetting-Muster)" (:130-140; OtterTune SIGMOD 2017, Subsetting arXiv 1409.0792).
//   Abschnitt 3.2: k-Wahl konsensuell (Silhouette und Gap als Haupt, Elbow als Plausibilitaet) mit
//   Domaenen-Anker ~6..14 aus den LP-Archetypen (:167-175). Abschnitt 5.2: Online-Zuordnung =
//   Nearest-Cluster-Zentroid (OtterTune-Mapping), "noise" -> sicherer Default (:239-244).
//
// WAS DIESER SCHNITT LIEFERT (und was bewusst NICHT -- ehrlich):
//   GEBAUT:  Dendrogramm (agglomerativ, Lance-Williams-Rekurrenz), Schnitt bei k, k-Means-Verfeinerung
//            (deterministisch aus den Schnitt-Zentroiden gesaeht), zentrums-naechster Repraesentant je
//            Cluster, Diagnostik WCSS (Elbow-Datenbasis) + mittlere Silhouette (Rousseeuw 1987),
//            Nearest-Centroid-Zuordnung mit explizitem noise-Schwellwert (Default-Fallback, :239-244).
//   NICHT GEBAUT (zweiter Schnitt / Owner): das modifizierte Online-DBSCAN ueber Ankunftsraten-
//            Zeitreihen (QueryBot 5000, :141-144) + Konzept-Drift/ADWIN (:254-261) -- braucht den
//            E1->E4-Mess-Rueckkanal; die Gap-Statistik und BIC (:159-162) -- brauchen eine
//            Zufalls-Referenz bzw. GMM (Seed-/Verfahrens-Entscheid beim Owner).
//
// OFFENE FESTLEGUNGEN (Mechanik mit explizitem Parameter, KEINE geratenen Konstanten):
//   * LINKAGE-Kriterium: der BEFUND sagt nur "agglomerative Hierarchie", nennt KEIN Kriterium ->
//     Nicht-Typ-Template-Parameter OHNE Default; der Aufrufer (letztlich der Owner) MUSS waehlen.
//   * k: Owner/Verfahren nach Abschnitt 3.2 (hier: Parameter + Diagnostik, kein eingebauter Wert).
//   * noise-Schwellwert der Online-Zuordnung: Parameter ohne Default (:243 "faellt die Last in kein
//     Cluster" -- WIE weit "kein Cluster" ist, legt der BEFUND nicht fest).
//   * kmeans_max_iterations: mechanischer Sicherheits-Deckel gegen Zyklen, explizit vom Aufrufer.
//   * k-Means-Saat = Zentroide des Dendrogramm-Schnitts: deterministische Operationalisierung von
//     "nachlagern" (:136-138); der BEFUND nennt keine Initialisierung -- diese Lesart vermeidet jede
//     Zufallsquelle und ist dokumentiert, nicht behauptet.
//
// DETERMINISMUS (Implementierungs-Regeln, dokumentiert -- keine BEFUND-Aussagen):
//   * Distanz = euklidisch (BEFUND :83 "euklidisch/distanzbasiert"); Ward intern auf Quadrat-Distanzen
//     (Lance-Williams-Standardform), Dendrogramm-Hoehe dort als sqrt (uebliche Konvention).
//   * Gleichstand bei der Paar-Wahl -> kleinstes Positions-Paar in Erzeugungs-Reihenfolge.
//   * Gleichstand bei der Zuordnung -> kleinster Zentroid-Index. Leerer Cluster im k-Means-Schritt ->
//     Zentroid bleibt stehen (ehrlich sichtbar als cluster_size 0, KEIN stilles Neu-Saehen).
//   * Cluster-Etiketten deterministisch nach kleinstem Mitglieds-Index umnummeriert.
//
// HONEST-EMPTY-Doktrin: leerer Korpus, k==0, k>n oder nicht-endliche Koordinaten -> std::nullopt.
//   Degenerierter Korpus (alle Punkte identisch) liefert ein EHRLICHES Ergebnis: leere Cluster bleiben
//   leer sichtbar, Silhouette verweigert sich (std::nullopt), nichts stuerzt ab und nichts wird erfunden.
//
// O(n^2)-Speicher/O(n^3)-Zeit der Hierarchie sind laut BEFUND :114-116 fuer das Klein-n dieses
// Merkmalsraums (14 LP x 6 Datensaetze x Fuellstaende) IRRELEVANT -- Klarheit schlaegt Mikro-Optimierung.
// Header-only, C++23, KEINE Fremdbibliothek (ext/ traegt keine Clustering-Bibliothek), kein Python.

#include "workload_feature_vector.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace comdare::cache_engine::heuristik {

/// Ein normalisierter Merkmals-Punkt fester Dimension (Default-Anwendung: kWorkloadFeatureCount).
template <std::size_t Dim>
using FeaturePoint = std::array<double, Dim>;

/// Agglomerations-Kriterium. Der BEFUND legt KEINES fest (nur "agglomerative Hierarchie", :108-116) --
/// darum traegt dieser Parameter bewusst KEINEN Default: die Wahl ist ein offener Owner-Entscheid.
enum class HierarchicalLinkage : std::uint8_t {
    Single   = 0, ///< d(A u B, C) = min  -- kettenbildend
    Complete = 1, ///< d(A u B, C) = max  -- kompakte Cluster
    Average  = 2, ///< groessen-gewichtetes Mittel (UPGMA)
    Ward     = 3, ///< minimale Varianz-Zunahme (Lance-Williams auf Quadrat-Distanzen)
};

/// Ein Verschmelzungs-Schritt des Dendrogramms. Blaetter tragen die ids 0..n-1, der Schritt t erzeugt
/// die id n+t (scipy-Konvention). `height` ist die Verschmelzungs-Distanz des gewaehlten Kriteriums.
struct DendrogramMerge {
    std::size_t left        = 0;   ///< kleinere der beiden verschmolzenen Cluster-ids
    std::size_t right       = 0;   ///< groessere der beiden verschmolzenen Cluster-ids
    double      height      = 0.0; ///< Verschmelzungs-Distanz (Ward: sqrt der Lance-Williams-Groesse)
    std::size_t merged_size = 0;   ///< Punktzahl des neuen Clusters
};

namespace detail {

template <std::size_t Dim>
[[nodiscard]] inline double squared_distance(FeaturePoint<Dim> const& a, FeaturePoint<Dim> const& b) noexcept {
    double s = 0.0;
    for (std::size_t d = 0; d < Dim; ++d) {
        double const diff = a[d] - b[d];
        s += diff * diff;
    }
    return s;
}

template <std::size_t Dim>
[[nodiscard]] inline double euclidean_distance(FeaturePoint<Dim> const& a, FeaturePoint<Dim> const& b) noexcept {
    return std::sqrt(squared_distance<Dim>(a, b));
}

template <std::size_t Dim>
[[nodiscard]] inline bool all_finite(std::span<FeaturePoint<Dim> const> pts) noexcept {
    for (FeaturePoint<Dim> const& p : pts)
        for (double v : p)
            if (!std::isfinite(v)) return false;
    return true;
}

} // namespace detail

// ---------------------------------------------------------------------------------------------------
// Schritt 1 (BEFUND 2.4-1a): das Dendrogramm. Lance-Williams-Rekurrenz, Kriterium compile-time per
// `if constexpr` (kein Laufzeit-Zweig, kein std::variant).
// ---------------------------------------------------------------------------------------------------
template <HierarchicalLinkage Linkage, std::size_t Dim>
[[nodiscard]] std::optional<std::vector<DendrogramMerge>>
agglomerate_dendrogram(std::span<FeaturePoint<Dim> const> pts) {
    static_assert(Dim > 0);
    if (!detail::all_finite<Dim>(pts)) return std::nullopt;
    std::size_t const            n = pts.size();
    std::vector<DendrogramMerge> merges;
    if (n < 2) return merges; // 0/1 Punkt: ehrlich leeres Dendrogramm (kein Fehler, nichts zu verschmelzen)
    merges.reserve(n - 1);

    constexpr bool kSquared = (Linkage == HierarchicalLinkage::Ward);

    std::vector<std::size_t>         ids(n);   // Cluster-id je aktiver Position (Erzeugungs-Reihenfolge)
    std::vector<std::size_t>         sizes(n); // Punktzahl je aktiver Position
    std::vector<std::vector<double>> dist(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        ids[i]   = i;
        sizes[i] = 1;
        for (std::size_t j = i + 1; j < n; ++j) {
            double const d = kSquared ? detail::squared_distance<Dim>(pts[i], pts[j])
                                      : detail::euclidean_distance<Dim>(pts[i], pts[j]);
            dist[i][j]     = d;
            dist[j][i]     = d;
        }
    }

    std::size_t next_id = n;
    while (ids.size() > 1) {
        // Kleinstes Paar; Gleichstand -> kleinstes Positions-Paar (deterministisch, dokumentiert).
        std::size_t bi = 0, bj = 1;
        double      best = dist[0][1];
        for (std::size_t i = 0; i < ids.size(); ++i)
            for (std::size_t j = i + 1; j < ids.size(); ++j)
                if (dist[i][j] < best) {
                    best = dist[i][j];
                    bi   = i;
                    bj   = j;
                }

        std::size_t const si  = sizes[bi];
        std::size_t const sj  = sizes[bj];
        double const      dij = dist[bi][bj];
        merges.push_back(DendrogramMerge{std::min(ids[bi], ids[bj]), std::max(ids[bi], ids[bj]),
                                         kSquared ? std::sqrt(std::max(dij, 0.0)) : dij, si + sj});

        // Lance-Williams-Distanz des neuen Clusters zu jedem verbliebenen k.
        std::vector<double> dnew;
        dnew.reserve(ids.size() - 2);
        for (std::size_t k = 0; k < ids.size(); ++k) {
            if (k == bi || k == bj) continue;
            double const dik = dist[bi][k];
            double const djk = dist[bj][k];
            double       d   = 0.0;
            if constexpr (Linkage == HierarchicalLinkage::Single) {
                d = std::min(dik, djk);
            } else if constexpr (Linkage == HierarchicalLinkage::Complete) {
                d = std::max(dik, djk);
            } else if constexpr (Linkage == HierarchicalLinkage::Average) {
                d = (static_cast<double>(si) * dik + static_cast<double>(sj) * djk) //
                    / static_cast<double>(si + sj);
            } else { // Ward auf Quadrat-Distanzen
                double const sk = static_cast<double>(sizes[k]);
                double const a  = static_cast<double>(si) + sk;
                double const b  = static_cast<double>(sj) + sk;
                double const c  = static_cast<double>(si + sj) + sk;
                d               = (a * dik + b * djk - sk * dij) / c;
            }
            dnew.push_back(d);
        }

        // Positionen bi/bj entfernen, neuen Cluster am ENDE anhaengen (Erzeugungs-Reihenfolge bleibt
        // die Tie-Break-Ordnung). Matrix entsprechend verdichten.
        std::vector<std::size_t> keep;
        keep.reserve(ids.size() - 2);
        for (std::size_t k = 0; k < ids.size(); ++k)
            if (k != bi && k != bj) keep.push_back(k);

        std::vector<std::size_t> const   old_ids   = ids;
        std::vector<std::size_t> const   old_sizes = sizes;
        std::size_t const                m         = keep.size() + 1;
        std::vector<std::size_t>         nids(m);
        std::vector<std::size_t>         nsizes(m);
        std::vector<std::vector<double>> nd(m, std::vector<double>(m, 0.0));
        for (std::size_t a = 0; a < keep.size(); ++a) {
            nids[a]   = old_ids[keep[a]];
            nsizes[a] = old_sizes[keep[a]];
            for (std::size_t b = a + 1; b < keep.size(); ++b) {
                nd[a][b] = dist[keep[a]][keep[b]];
                nd[b][a] = nd[a][b];
            }
            nd[a][m - 1] = dnew[a];
            nd[m - 1][a] = dnew[a];
        }
        nids[m - 1]   = next_id++;
        nsizes[m - 1] = si + sj;
        ids           = std::move(nids);
        sizes         = std::move(nsizes);
        dist          = std::move(nd);
    }
    return merges;
}

// ---------------------------------------------------------------------------------------------------
// Schritt 2: Schnitt des Dendrogramms bei k ("im Dendrogramm gewaehltes k", BEFUND :136-137).
// Etiketten 0..k-1, deterministisch nach kleinstem Mitglieds-Index geordnet.
// ---------------------------------------------------------------------------------------------------
[[nodiscard]] inline std::optional<std::vector<std::size_t>>
cut_dendrogram(std::size_t n_points, std::vector<DendrogramMerge> const& merges, std::size_t k) {
    if (n_points == 0 || k == 0 || k > n_points) return std::nullopt;
    if (merges.size() + 1 != n_points) return std::nullopt; // fremdes/verstuemmeltes Dendrogramm
    constexpr std::size_t    kNoParent = std::numeric_limits<std::size_t>::max();
    std::size_t const        applied   = n_points - k; // die ersten n-k Verschmelzungen gelten
    std::vector<std::size_t> parent(n_points + applied, kNoParent);
    for (std::size_t t = 0; t < applied; ++t) {
        std::size_t const merged = n_points + t;
        if (merges[t].left >= merged || merges[t].right >= merged) return std::nullopt; // inkonsistent
        parent[merges[t].left]  = merged;
        parent[merges[t].right] = merged;
    }
    std::vector<std::size_t> root_label(n_points + applied, kNoParent);
    std::vector<std::size_t> labels(n_points, 0);
    std::size_t              next_label = 0;
    for (std::size_t leaf = 0; leaf < n_points; ++leaf) {
        std::size_t r = leaf;
        while (parent[r] != kNoParent) r = parent[r];
        if (root_label[r] == kNoParent) root_label[r] = next_label++; // erstes Auftreten = kleinstes Mitglied
        labels[leaf] = root_label[r];
    }
    if (next_label != k) return std::nullopt; // Wache: Schnitt muss exakt k Komponenten ergeben
    return labels;
}

// ---------------------------------------------------------------------------------------------------
// Schritt 3 (BEFUND 2.4-1b): k-Means-Verfeinerung (Lloyd), deterministisch aus uebergebenen Zentroiden.
// ---------------------------------------------------------------------------------------------------
template <std::size_t Dim>
struct KMeansRefinement {
    std::vector<std::size_t>       labels;        ///< finale Zuordnung je Punkt
    std::vector<FeaturePoint<Dim>> centroids;     ///< finale Zentroide (leer gebliebene stehen unveraendert)
    std::vector<std::size_t>       cluster_sizes; ///< Punktzahl je Cluster (0 = ehrlich leer)
    std::size_t                    iterations = 0;
    bool                           converged  = false;
};

template <std::size_t Dim>
[[nodiscard]] std::optional<KMeansRefinement<Dim>> kmeans_refine(std::span<FeaturePoint<Dim> const> pts,
                                                                 std::vector<FeaturePoint<Dim>>     centroids,
                                                                 std::size_t                        max_iterations) {
    static_assert(Dim > 0);
    if (pts.empty() || centroids.empty() || centroids.size() > pts.size() || max_iterations == 0) return std::nullopt;
    if (!detail::all_finite<Dim>(pts) || !detail::all_finite<Dim>(std::span<FeaturePoint<Dim> const>(centroids)))
        return std::nullopt;

    std::size_t const        k = centroids.size();
    KMeansRefinement<Dim>    r;
    std::vector<std::size_t> prev;
    r.labels.assign(pts.size(), 0);
    r.cluster_sizes.assign(k, 0);
    for (std::size_t it = 0; it < max_iterations; ++it) {
        r.iterations = it + 1;
        // Zuordnung: naechster Zentroid; Gleichstand -> kleinster Index (deterministisch).
        for (std::size_t p = 0; p < pts.size(); ++p) {
            std::size_t best_c = 0;
            double      best_d = detail::squared_distance<Dim>(pts[p], centroids[0]);
            for (std::size_t c = 1; c < k; ++c) {
                double const d = detail::squared_distance<Dim>(pts[p], centroids[c]);
                if (d < best_d) {
                    best_d = d;
                    best_c = c;
                }
            }
            r.labels[p] = best_c;
        }
        if (r.labels == prev) {
            r.converged = true;
            break;
        }
        prev = r.labels;
        // Update: Zentroid = Mitglieds-Mittel; leerer Cluster -> Zentroid bleibt stehen (ehrlich, kein
        // stilles Neu-Saehen -- der leere Cluster bleibt im Ergebnis als cluster_size 0 sichtbar).
        std::vector<FeaturePoint<Dim>> sums(k, FeaturePoint<Dim>{});
        std::vector<std::size_t>       cnt(k, 0);
        for (std::size_t p = 0; p < pts.size(); ++p) {
            for (std::size_t d = 0; d < Dim; ++d) sums[r.labels[p]][d] += pts[p][d];
            ++cnt[r.labels[p]];
        }
        for (std::size_t c = 0; c < k; ++c)
            if (cnt[c] != 0)
                for (std::size_t d = 0; d < Dim; ++d) centroids[c][d] = sums[c][d] / static_cast<double>(cnt[c]);
    }
    std::fill(r.cluster_sizes.begin(), r.cluster_sizes.end(), 0);
    for (std::size_t lbl : r.labels) ++r.cluster_sizes[lbl];
    r.centroids = std::move(centroids);
    return r;
}

// ---------------------------------------------------------------------------------------------------
// Schritt 4 (OtterTune-/Subsetting-Muster, BEFUND :101-106/:137-139): der zentrums-naechste
// Repraesentant je Cluster -- der Stellvertreter, der als wiederverwendbares XML-Lastprofil persistiert
// wird. Leerer Cluster -> ehrlich std::nullopt (KEIN Phantom-Repraesentant).
// ---------------------------------------------------------------------------------------------------
template <std::size_t Dim>
[[nodiscard]] std::vector<std::optional<std::size_t>>
centroid_nearest_representatives(std::span<FeaturePoint<Dim> const> pts, std::vector<std::size_t> const& labels,
                                 std::vector<FeaturePoint<Dim>> const& centroids) {
    std::vector<std::optional<std::size_t>> rep(centroids.size());
    std::vector<double>                     best(centroids.size(), std::numeric_limits<double>::infinity());
    for (std::size_t p = 0; p < pts.size() && p < labels.size(); ++p) {
        std::size_t const c = labels[p];
        if (c >= centroids.size()) continue;
        double const d = detail::squared_distance<Dim>(pts[p], centroids[c]);
        if (d < best[c]) { // striktes '<': Gleichstand behaelt den kleineren Punkt-Index
            best[c] = d;
            rep[c]  = p;
        }
    }
    return rep;
}

// ---------------------------------------------------------------------------------------------------
// Diagnostik fuer die k-Wahl (BEFUND 3.2): WCSS als Elbow-Datenbasis, mittlere Silhouette
// (Rousseeuw 1987) als Haupt-Kriterium. Gap-Statistik/BIC bewusst NICHT hier (siehe Kopf).
// ---------------------------------------------------------------------------------------------------
template <std::size_t Dim>
[[nodiscard]] double wcss(std::span<FeaturePoint<Dim> const> pts, std::vector<std::size_t> const& labels,
                          std::vector<FeaturePoint<Dim>> const& centroids) {
    double s = 0.0;
    for (std::size_t p = 0; p < pts.size() && p < labels.size(); ++p)
        if (labels[p] < centroids.size()) s += detail::squared_distance<Dim>(pts[p], centroids[labels[p]]);
    return s;
}

/// Mittlere Silhouette ueber alle Punkte. Definiert nur fuer 2 <= (nicht-leere Cluster) <= n-1
/// (Rousseeuw 1987) -- sonst ehrlich std::nullopt. Einpunkt-Cluster: s(i)=0 (Rousseeuw-Konvention);
/// a==b==0 (identische Punkte): s(i)=0 statt 0/0.
template <std::size_t Dim>
[[nodiscard]] std::optional<double> mean_silhouette(std::span<FeaturePoint<Dim> const> pts,
                                                    std::vector<std::size_t> const&    labels) {
    std::size_t const n = pts.size();
    if (n == 0 || labels.size() != n) return std::nullopt;
    std::size_t const        k_slots = 1 + *std::max_element(labels.begin(), labels.end());
    std::vector<std::size_t> count(k_slots, 0);
    for (std::size_t lbl : labels) ++count[lbl];
    std::size_t nonempty = 0;
    for (std::size_t c : count)
        if (c > 0) ++nonempty;
    if (nonempty < 2 || nonempty > n - 1) return std::nullopt;

    double              acc = 0.0;
    std::vector<double> mean_to(k_slots, 0.0);
    for (std::size_t i = 0; i < n; ++i) {
        if (count[labels[i]] == 1) continue; // s(i)=0 fuer Einpunkt-Cluster -> traegt 0 bei
        std::fill(mean_to.begin(), mean_to.end(), 0.0);
        for (std::size_t j = 0; j < n; ++j) {
            if (j == i) continue;
            mean_to[labels[j]] += detail::euclidean_distance<Dim>(pts[i], pts[j]);
        }
        double const a = mean_to[labels[i]] / static_cast<double>(count[labels[i]] - 1);
        double       b = std::numeric_limits<double>::infinity();
        for (std::size_t c = 0; c < k_slots; ++c) {
            if (c == labels[i] || count[c] == 0) continue;
            b = std::min(b, mean_to[c] / static_cast<double>(count[c]));
        }
        double const m = std::max(a, b);
        acc += (m <= 0.0) ? 0.0 : (b - a) / m;
    }
    return acc / static_cast<double>(n);
}

// ---------------------------------------------------------------------------------------------------
// DIE Offline-Pipeline (BEFUND 2.4-1 komplett): Dendrogramm -> Schnitt bei k -> k-Means-Verfeinerung ->
// Repraesentanten -> Diagnostik. Ein Aufruf, ein deterministisches Ergebnis.
// ---------------------------------------------------------------------------------------------------
template <std::size_t Dim>
struct OfflineWorkloadClustering {
    std::vector<DendrogramMerge>            dendrogram;       ///< volle Verschmelzungs-Folge (Thesis-Abbildung)
    std::vector<std::size_t>                hierarchy_labels; ///< Schnitt bei k VOR der Verfeinerung
    std::vector<std::size_t>                labels;           ///< finale Zuordnung nach k-Means
    std::vector<FeaturePoint<Dim>>          centroids;        ///< finale Zentroide (normalisierter Raum)
    std::vector<std::size_t>                cluster_sizes;    ///< Punktzahl je Cluster (0 = ehrlich leer)
    std::vector<std::optional<std::size_t>> representatives;  ///< zentrums-naechster Punkt je Cluster
    double                                  wcss = 0.0;       ///< Elbow-Datenbasis (BEFUND 3.1)
    std::optional<double>                   silhouette;       ///< mittlere Silhouette; nullopt wo undefiniert
    std::size_t                             kmeans_iterations = 0;
    bool                                    kmeans_converged  = false;
};

/// Fuehrt die Offline-Haelfte aus. `k` und `Linkage` sind bewusst Pflicht-Entscheidungen des Aufrufers
/// (BEFUND 3.2: k konsensuell waehlen, Domaenen-Anker ~6..14 -- hier wird NICHTS davon hart verdrahtet).
template <HierarchicalLinkage Linkage, std::size_t Dim>
[[nodiscard]] std::optional<OfflineWorkloadClustering<Dim>>
cluster_offline(std::span<FeaturePoint<Dim> const> pts, std::size_t k, std::size_t kmeans_max_iterations) {
    if (pts.empty() || k == 0 || k > pts.size()) return std::nullopt;
    std::optional<std::vector<DendrogramMerge>> merges = agglomerate_dendrogram<Linkage, Dim>(pts);
    if (!merges) return std::nullopt;
    std::optional<std::vector<std::size_t>> const cut = cut_dendrogram(pts.size(), *merges, k);
    if (!cut) return std::nullopt;

    // Saat der Verfeinerung: Zentroide des Dendrogramm-Schnitts (deterministisch, keine Zufallsquelle).
    std::vector<FeaturePoint<Dim>> seed(k, FeaturePoint<Dim>{});
    std::vector<std::size_t>       cnt(k, 0);
    for (std::size_t p = 0; p < pts.size(); ++p) {
        for (std::size_t d = 0; d < Dim; ++d) seed[(*cut)[p]][d] += pts[p][d];
        ++cnt[(*cut)[p]];
    }
    for (std::size_t c = 0; c < k; ++c) // jeder Schnitt-Cluster hat >= 1 Mitglied (cut_dendrogram-Wache)
        for (std::size_t d = 0; d < Dim; ++d) seed[c][d] /= static_cast<double>(cnt[c]);

    std::optional<KMeansRefinement<Dim>> km = kmeans_refine<Dim>(pts, std::move(seed), kmeans_max_iterations);
    if (!km) return std::nullopt;

    OfflineWorkloadClustering<Dim> out;
    out.dendrogram        = std::move(*merges);
    out.hierarchy_labels  = *cut;
    out.labels            = std::move(km->labels);
    out.centroids         = std::move(km->centroids);
    out.cluster_sizes     = std::move(km->cluster_sizes);
    out.kmeans_iterations = km->iterations;
    out.kmeans_converged  = km->converged;
    out.representatives   = centroid_nearest_representatives<Dim>(pts, out.labels, out.centroids);
    out.wcss              = wcss<Dim>(pts, out.labels, out.centroids);
    out.silhouette        = mean_silhouette<Dim>(pts, out.labels);
    return out;
}

// ---------------------------------------------------------------------------------------------------
// Online-Bruecke (BEFUND 5.2): Nearest-Cluster-Zuordnung im OtterTune-Muster -- KEIN Re-Clustering im
// Hot-Path. Liegt die Last weiter als `noise_distance_threshold` vom naechsten Zentroid, ist sie "noise"
// -> std::nullopt -> der Aufrufer faellt auf den Common-Denominator-Default zurueck (:243-244).
// Der Schwellwert ist bewusst OHNE Default: seine Kalibrierung legt der BEFUND nicht fest (Owner).
// ---------------------------------------------------------------------------------------------------
struct NearestClusterAssignment {
    std::size_t cluster  = 0;   ///< Index des naechsten Zentroids
    double      distance = 0.0; ///< euklidische Distanz dorthin (normalisierter Raum)
};

template <std::size_t Dim>
[[nodiscard]] std::optional<NearestClusterAssignment> nearest_cluster(std::span<FeaturePoint<Dim> const> centroids,
                                                                      FeaturePoint<Dim> const&           x,
                                                                      double noise_distance_threshold) {
    if (centroids.empty() || !std::isfinite(noise_distance_threshold)) return std::nullopt;
    for (double v : x)
        if (!std::isfinite(v)) return std::nullopt;
    std::size_t best_c = 0;
    double      best_d = detail::squared_distance<Dim>(x, centroids[0]);
    for (std::size_t c = 1; c < centroids.size(); ++c) {
        double const d = detail::squared_distance<Dim>(x, centroids[c]);
        if (d < best_d) { // striktes '<': Gleichstand behaelt den kleineren Index
            best_d = d;
            best_c = c;
        }
    }
    double const d = std::sqrt(best_d);
    if (d > noise_distance_threshold) return std::nullopt; // "noise" -> sicherer Default (BEFUND :243)
    return NearestClusterAssignment{best_c, d};
}

} // namespace comdare::cache_engine::heuristik
