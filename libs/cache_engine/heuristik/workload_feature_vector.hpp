#pragma once
// AXIS_ALGO_VERSION: 1
// heuristik/workload_feature_vector.hpp -- DER Cluster-Merkmalsvektor der Workload-Cluster-Bildung (T-10).
//
// QUELLE (autoritativ, NICHT erfunden): super `docs/audits/20260709-workload-cluster-research-BEFUND.md`
//   Abschnitt 1 "Merkmalsraum-Empfehlung" -- Merkmalsvektor-Tabelle :66-75, Normalisierung :82-89.
//   Grundprinzip :57-64: geclustert wird ueber Last-BESCHREIBER, NIE ueber die Achsen-Belegung (die ist
//   die Zielgroesse des spaeteren Klassifikators; sie hier aufzunehmen machte die Aufgabe zirkulaer).
//
// DIE SECHS FEATURE-GRUPPEN DES BEFUNDS (Tabelle :69-74), hier als 13 double-Dimensionen:
//   Op-Mix (5, Simplex) | Zugriffsverteilung (theta) | Negativ-Anteil | Working-Set |
//   Datensatz-Beschaffenheit (Schluessellaenge, Alphabet, Praefix-Entropie, Wert-Groesse) | Nebenlaeufigkeit.
//   EHRLICHE MARKE (:76-80): die Datensatz-Beschaffenheit ist im heutigen WorkloadProfileAggregate NICHT
//   erfasst -- sie ist ein Erweiterungs-Feature. Der Typ traegt die Felder; Quellen ohne diese Messung
//   fuellen honest-0 (from_op_mix_aggregate dokumentiert das je Feld).
//
// NORMALISIERUNG (Pflicht, nicht Kosmetik -- BEFUND :82-89 [LIT] PLOS One 2024): alle Verfahren sind
//   euklidisch/distanzbasiert; ohne Skalierung dominiert das Feature mit der groessten Spannweite
//   (Working-Set in Millionen >>> Op-Anteile in [0,1]). Der BEFUND legt fest: Op-Mix bleibt Simplex-Anteil;
//   Working-Set und Alphabet/Laenge log-transformiert dann z-normiert; Skew als kontinuierlicher
//   theta-Parameter. Der Rest der Zuordnung in kPlanFeatureScaling ist eine LESART der z-Score-Pflicht
//   und je Eintrag als solche markiert -- die Zuordnung ist ein PARAMETER von fit(), der Owner kann jede
//   Lesart ueberstimmen, ohne dass hier Code angefasst wird.
//
// OFFEN, NICHT ERFUNDEN (Owner-Entscheide, siehe Markierungen an kPlanFeatureScaling):
//   * z-Score auf theta / neg% / value_size / thread_count: der BEFUND nennt die Skala (:70-74), aber
//     nicht woertlich die Transformation je Einzel-Feature. Hier steht die naheliegende Lesart; sie ist
//     ueberstimmbar. * log1p statt log: Implementierungs-Notwendigkeit fuer den Wert 0 (thread_count=0,
//     working_set=0 sind reale honest-0-Werte), KEINE inhaltliche Festlegung.
//
// HONEST-EMPTY-Doktrin (analog axis_spline.hpp): leerer Korpus -> kein Normalizer (std::nullopt);
//   nicht-endliche oder fuer log unzulaessige (negative) Werte -> std::nullopt, NIE ein Phantom-Wert.
//   Null-Varianz eines z-Features -> das Feature traegt ehrlich 0 bei (konstant = trennt nichts), nie NaN.
//
// KEIN Runtime-Switch wo Compile-Time moeglich: Dimensionszahl, Namen und die Plan-Skalierung sind
// constexpr-Tabellen mit static_assert-Wachen. Header-only, C++23, KEINE Fremdbibliothek, kein Python.

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::heuristik {

/// Zahl der Merkmals-Dimensionen (BEFUND-Tabelle :69-74, sechs Gruppen -> 13 Skalare).
inline constexpr std::size_t kWorkloadFeatureCount = 13;

/// Index je Dimension -- Single-Source fuer Namen (kWorkloadFeatureNames) und Skalierung (kPlanFeatureScaling).
enum class WorkloadFeatureIndex : std::size_t {
    OpInsertShare       = 0,  ///< Op-Mix: Anteil Einfuegen           (:69, Simplex)
    OpLookupShare       = 1,  ///< Op-Mix: Anteil Lookup              (:69, Simplex)
    OpEraseShare        = 2,  ///< Op-Mix: Anteil Loeschen            (:69, Simplex)
    OpScanShare         = 3,  ///< Op-Mix: Anteil Scan                (:69, Simplex)
    OpRmwShare          = 4,  ///< Op-Mix: Anteil Read-Modify-Write   (:69, Simplex)
    AccessSkewTheta     = 5,  ///< Zugriffsverteilung als Zipf-theta, kontinuierlich statt kategorial (:70,:88-89)
    NegativeLookupShare = 6,  ///< neg% -- Anteil erfolgloser Lookups (:71, Skala [0,1])
    WorkingSetSize      = 7,  ///< Records bzw. key-range-Breite      (:72, log-skaliert)
    KeyLengthBytes      = 8,  ///< Datensatz: Schluessellaenge        (:73, Erweiterungs-Feature)
    AlphabetSize        = 9,  ///< Datensatz: Alphabet-Groesse        (:73, Erweiterungs-Feature)
    PrefixEntropyBits   = 10, ///< Datensatz: Praefix-Entropie        (:73, Erweiterungs-Feature)
    ValueSizeBytes      = 11, ///< Datensatz: Wert-Groesse            (:73, Erweiterungs-Feature)
    ThreadCount         = 12, ///< Nebenlaeufigkeit: Thread-Zahl      (:74, log/ordinal; heute honest-0)
};

/// Stabile Diagnose-Namen je Dimension (Reihenfolge == WorkloadFeatureIndex).
inline constexpr std::array<std::string_view, kWorkloadFeatureCount> kWorkloadFeatureNames = {
    "op_insert_share",     "op_lookup_share",       "op_erase_share",   "op_scan_share",    "op_rmw_share",
    "access_skew_theta",   "negative_lookup_share", "working_set_size", "key_length_bytes", "alphabet_size",
    "prefix_entropy_bits", "value_size_bytes",      "thread_count",
};

/// DER Merkmalsvektor EINER Last (roh, unnormalisiert). Reihenfolge und Bedeutung == WorkloadFeatureIndex.
struct WorkloadFeatureVector {
    double op_insert_share       = 0.0; ///< [0,1], Simplex mit den vier folgenden Anteilen
    double op_lookup_share       = 0.0; ///< [0,1]
    double op_erase_share        = 0.0; ///< [0,1]
    double op_scan_share         = 0.0; ///< [0,1]
    double op_rmw_share          = 0.0; ///< [0,1]
    double access_skew_theta     = 0.0; ///< Zipf-theta (0 = uniform), kontinuierlich (:88-89)
    double negative_lookup_share = 0.0; ///< [0,1] Anteil erfolgloser Lookups
    double working_set_size      = 0.0; ///< Records bzw. key-range-Breite (roh; log erst im Normalizer)
    double key_length_bytes      = 0.0; ///< Erweiterungs-Feature (:76-80); Quelle ohne Messung -> honest-0
    double alphabet_size         = 0.0; ///< Erweiterungs-Feature; honest-0 ohne Messung
    double prefix_entropy_bits   = 0.0; ///< Erweiterungs-Feature; honest-0 ohne Messung
    double value_size_bytes      = 0.0; ///< Erweiterungs-Feature; honest-0 ohne Messung
    double thread_count          = 0.0; ///< honest-0, solange keine Nebenlaeufigkeit ausgewiesen ist

    /// Roh-Werte als Array in WorkloadFeatureIndex-Reihenfolge (Eingang des Normalizers).
    [[nodiscard]] constexpr std::array<double, kWorkloadFeatureCount> to_array() const noexcept {
        return {op_insert_share,     op_lookup_share,       op_erase_share,   op_scan_share,    op_rmw_share,
                access_skew_theta,   negative_lookup_share, working_set_size, key_length_bytes, alphabet_size,
                prefix_entropy_bits, value_size_bytes,      thread_count};
    }
};

// ---------------------------------------------------------------------------------------------------
// Skalierung je Dimension (BEFUND :82-89). Die Transformation haengt an der DIMENSION, nicht am Korpus.
// ---------------------------------------------------------------------------------------------------
enum class FeatureScaling : std::uint8_t {
    SimplexShare = 0, ///< Wert geht unveraendert ein (bereits Anteil in [0,1] -- BEFUND :87)
    ZScore       = 1, ///< z-Standardisierung ueber den Korpus (Mittel 0, Std 1 -- BEFUND :85-86)
    Log1pZScore  = 2, ///< erst log(1+x) (0 zulaessig), dann z-Standardisierung (BEFUND :87-88 "log ... dann z")
};

/// Die BEFUND-verankerte Skalierungs-Zuordnung. Eintraege mit LESART sind nicht woertlich festgelegt und
/// per fit()-Parameter ueberstimmbar (Owner-Entscheid steht aus; siehe Kopf-Kommentar).
inline constexpr std::array<FeatureScaling, kWorkloadFeatureCount> kPlanFeatureScaling = {
    FeatureScaling::SimplexShare, // op_insert_share       :87 "Op-Mix bleibt als Simplex-Anteil"
    FeatureScaling::SimplexShare, // op_lookup_share       :87
    FeatureScaling::SimplexShare, // op_erase_share        :87
    FeatureScaling::SimplexShare, // op_scan_share         :87
    FeatureScaling::SimplexShare, // op_rmw_share          :87
    FeatureScaling::ZScore,       // access_skew_theta     :88-89 theta kontinuierlich; z = LESART der z-Pflicht
    FeatureScaling::SimplexShare, // negative_lookup_share :71 Skala [0,1]; Anteil wie Op-Mix = LESART
    FeatureScaling::Log1pZScore,  // working_set_size      :87-88 "log-transformiert dann z-normiert"
    FeatureScaling::Log1pZScore,  // key_length_bytes      :88 "Alphabet/Laenge"
    FeatureScaling::Log1pZScore,  // alphabet_size         :88 "Alphabet/Laenge"
    FeatureScaling::ZScore,       // prefix_entropy_bits   :73 "gemischt, je z-normiert"
    FeatureScaling::ZScore,       // value_size_bytes      :73 "je z-normiert"; ohne log = LESART
    FeatureScaling::Log1pZScore,  // thread_count          :74 "log/ordinal"; log+z = LESART
};

static_assert(kWorkloadFeatureNames.size() == kWorkloadFeatureCount);
static_assert(kPlanFeatureScaling.size() == kWorkloadFeatureCount);
static_assert(static_cast<std::size_t>(WorkloadFeatureIndex::ThreadCount) + 1 == kWorkloadFeatureCount,
              "WorkloadFeatureIndex und kWorkloadFeatureCount muessen dieselbe Dimensionszahl tragen");
// Wache: genau die fuenf Op-Mix-Dimensionen und neg% sind Simplex (BEFUND :87 + Lesart :71).
static_assert([] {
    std::size_t simplex = 0;
    for (FeatureScaling s : kPlanFeatureScaling)
        if (s == FeatureScaling::SimplexShare) ++simplex;
    return simplex == 6;
}());

namespace detail {

/// Vor-Transformation EINES Rohwerts. std::nullopt bei nicht-endlichem Wert oder negativem log-Eingang --
/// Messfehler werden abgewiesen, nie stillschweigend zu Phantom-Punkten verrechnet.
[[nodiscard]] inline std::optional<double> pre_transform(double v, FeatureScaling s) noexcept {
    if (!std::isfinite(v)) return std::nullopt;
    if (s == FeatureScaling::Log1pZScore) {
        if (v < 0.0) return std::nullopt; // negatives Working-Set/Thread-Count ist ein Messfehler
        return std::log1p(v);
    }
    return v;
}

} // namespace detail

// ---------------------------------------------------------------------------------------------------
// WorkloadFeatureNormalizer -- fit() lernt Mittel/Streuung des Korpus, apply() liefert den normalisierten
// Punkt fuer Offline-Clusterung UND Online-Zuordnung (BEFUND :239-244: derselbe Merkmalsraum fuer beide).
// Streuung ist die POPULATIONS-Standardabweichung (Teiler n, deterministisch, dokumentiert).
// ---------------------------------------------------------------------------------------------------
class WorkloadFeatureNormalizer {
public:
    using Point = std::array<double, kWorkloadFeatureCount>;

    /// Lernt die Normalisierung aus dem Korpus. HONEST-EMPTY: leerer Korpus oder unzulaessige Werte ->
    /// std::nullopt. `scaling` default = BEFUND-Zuordnung; der Owner kann jede Lesart ueberstimmen.
    [[nodiscard]] static std::optional<WorkloadFeatureNormalizer>
    fit(std::span<WorkloadFeatureVector const>                   corpus,
        std::array<FeatureScaling, kWorkloadFeatureCount> const& scaling = kPlanFeatureScaling) {
        if (corpus.empty()) return std::nullopt;
        WorkloadFeatureNormalizer nz;
        nz.scaling_ = scaling;
        nz.n_       = corpus.size();
        std::vector<double> t(corpus.size(), 0.0);
        for (std::size_t d = 0; d < kWorkloadFeatureCount; ++d) {
            for (std::size_t r = 0; r < corpus.size(); ++r) {
                std::optional<double> const v = detail::pre_transform(corpus[r].to_array()[d], scaling[d]);
                if (!v) return std::nullopt; // Messfehler im Korpus -> ehrlich kein Normalizer
                t[r] = *v;
            }
            double sum = 0.0;
            for (double v : t) sum += v;
            double const mean = sum / static_cast<double>(t.size());
            double       sq   = 0.0;
            for (double v : t) sq += (v - mean) * (v - mean);
            nz.mean_[d]   = mean;
            nz.stddev_[d] = std::sqrt(sq / static_cast<double>(t.size())); // Population (Teiler n)
        }
        return nz;
    }

    /// Normalisiert EINEN Rohvektor. Simplex-Dimensionen gehen unveraendert ein; z-Dimensionen als
    /// (t - mean)/std; Null-Varianz -> ehrlich 0 (konstantes Feature trennt nichts, nie NaN).
    [[nodiscard]] std::optional<Point> apply(WorkloadFeatureVector const& v) const {
        Point                                           out{};
        std::array<double, kWorkloadFeatureCount> const raw = v.to_array();
        for (std::size_t d = 0; d < kWorkloadFeatureCount; ++d) {
            std::optional<double> const t = detail::pre_transform(raw[d], scaling_[d]);
            if (!t) return std::nullopt;
            if (scaling_[d] == FeatureScaling::SimplexShare) {
                out[d] = *t;
            } else {
                out[d] = (stddev_[d] <= kTinyStddev) ? 0.0 : (*t - mean_[d]) / stddev_[d];
            }
        }
        return out;
    }

    /// Bequemlichkeit: ganzen Korpus normalisieren. Ein unzulaessiger Vektor -> std::nullopt (gesamt).
    [[nodiscard]] std::optional<std::vector<Point>> apply_corpus(std::span<WorkloadFeatureVector const> c) const {
        std::vector<Point> out;
        out.reserve(c.size());
        for (WorkloadFeatureVector const& v : c) {
            std::optional<Point> const p = apply(v);
            if (!p) return std::nullopt;
            out.push_back(*p);
        }
        return out;
    }

    [[nodiscard]] std::size_t    corpus_size() const noexcept { return n_; }
    [[nodiscard]] double         mean(std::size_t d) const noexcept { return mean_[d]; }
    [[nodiscard]] double         stddev(std::size_t d) const noexcept { return stddev_[d]; }
    [[nodiscard]] FeatureScaling scaling(std::size_t d) const noexcept { return scaling_[d]; }

    /// Unterhalb dieser Streuung gilt ein Feature als konstant (Schutz gegen Division durch ~0).
    static constexpr double kTinyStddev = 1e-12;

private:
    WorkloadFeatureNormalizer() = default;
    std::array<FeatureScaling, kWorkloadFeatureCount> scaling_{};
    std::array<double, kWorkloadFeatureCount>         mean_{};
    std::array<double, kWorkloadFeatureCount>         stddev_{};
    std::size_t                                       n_ = 0;
};

// ---------------------------------------------------------------------------------------------------
// Naht zum bestehenden Heuristik-Kern (BEFUND :45-48): das WorkloadProfileAggregate
// (builder/cacheline_policy/cacheline_policy_selector.hpp:61-108) ist eine reale Quelle des
// Merkmalsvektors. Concept statt Include -- heuristik/ bleibt frei von builder-Abhaengigkeiten
// (Meta-getriebene Concept-Haertung; der Test belegt die Passung gegen den echten Typ).
// ---------------------------------------------------------------------------------------------------
template <class A>
concept WorkloadOpMixAggregateLike = requires(A const& a) {
    { a.op_insert_n } -> std::convertible_to<std::uint64_t>;
    { a.op_lookup_n } -> std::convertible_to<std::uint64_t>;
    { a.op_erase_n } -> std::convertible_to<std::uint64_t>;
    { a.op_scan_n } -> std::convertible_to<std::uint64_t>;
    { a.op_rmw_n } -> std::convertible_to<std::uint64_t>;
    { a.working_set_n } -> std::convertible_to<std::uint64_t>;
};

/// Baut den Merkmalsvektor aus einem Op-Mix-Aggregat. Fuellt NUR Op-Mix + Working-Set; alle uebrigen
/// Dimensionen bleiben honest-0, weil das Aggregat sie nicht traegt (BEFUND :76-80 "nicht erfasst" --
/// KEINE erfundenen Werte). Leere Op-Summe -> alle Anteile ehrlich 0.
template <WorkloadOpMixAggregateLike A>
[[nodiscard]] constexpr WorkloadFeatureVector from_op_mix_aggregate(A const& a) noexcept {
    WorkloadFeatureVector v{};
    auto const            ins = static_cast<std::uint64_t>(a.op_insert_n);
    auto const            lk  = static_cast<std::uint64_t>(a.op_lookup_n);
    auto const            er  = static_cast<std::uint64_t>(a.op_erase_n);
    auto const            sc  = static_cast<std::uint64_t>(a.op_scan_n);
    auto const            rmw = static_cast<std::uint64_t>(a.op_rmw_n);
    std::uint64_t const   tot = ins + lk + er + sc + rmw;
    if (tot != 0) {
        auto share        = [tot](std::uint64_t n) { return static_cast<double>(n) / static_cast<double>(tot); };
        v.op_insert_share = share(ins);
        v.op_lookup_share = share(lk);
        v.op_erase_share  = share(er);
        v.op_scan_share   = share(sc);
        v.op_rmw_share    = share(rmw);
    }
    v.working_set_size = static_cast<double>(static_cast<std::uint64_t>(a.working_set_n));
    return v;
}

} // namespace comdare::cache_engine::heuristik
