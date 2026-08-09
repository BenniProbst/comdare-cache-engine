#pragma once

#include <cstdint>
#include <cstdio>
#include <span>
#include <utility>

extern "C" {
#include <hdr/hdr_histogram.h>
}

namespace comdare::cache_engine::measurement {

// == D5-5 (2026-08-09): DER VERWORFENE ANTEIL IST EINE ZAHL, KEIN SCHWEIGEN ========================
// SELBSTCHECK DIESES BLOCKS
//   ZUSICHERT: jede vorgelegte Probe landet in GENAU EINEM Topf -- aufgezeichnet, null, negativ oder
//              ausserhalb. vorgelegt() == count() + verworfen_null() + verworfen_negativ() +
//              verworfen_ausserhalb(), solange das Histogramm bereit ist.
//   ZUSICHERT NICHT: nichts darueber, ob ein Verwurf schlimm ist -- das entscheidet die Auswertung.
//
// WARUM VIER TOEPFE UND NICHT EIN "verworfen"-ZAEHLER: es sind vier verschiedene Diagnosen.
//   * 0 ns      = REALE Messung unterhalb der Uhr-Aufloesung. Erwartbar, aber sie verzerrt jedes
//                 Perzentil nach oben, wenn sie stillschweigend fehlt.
//   * negativ   = unmoegliche Dauer -> DEFEKT der Zeitnahme (Uhr-Ruecksprung, falsche Subtraktion).
//   * ausserhalb= Wert ueber highest_trackable_value -> die Histogramm-Spanne ist zu klein gewaehlt.
//                 hdr_record_value meldet das per Rueckgabewert; der wurde bis hierher weggeworfen.
//   * nicht bereit = hdr_init fehlgeschlagen -> ALLES geht verloren, nicht nur einzelne Proben.
// Ein Sammelzaehler wuerde diese vier Ursachen zu einer unbrauchbaren Zahl verschmelzen.
class LatencyHdrHistogram {
    hdr_histogram* h_                    = nullptr;
    std::int64_t   verworfen_null_       = 0;
    std::int64_t   verworfen_negativ_    = 0;
    std::int64_t   verworfen_ausserhalb_ = 0;

public:
    /// Die signifikanten Stellen, mit denen dieser Wrapper seine Histogramme baut. OEFFENTLICH,
    /// damit die Auswertung ihre Toleranz aus GENAU DIESER Zahl herleiten kann statt aus einer
    /// zweiten, die auseinanderdriften wuerde (s. HdrGeometrie in hdr_perzentil_auswertung.hpp).
    static constexpr int kSignifikanteStellen = 3;

    explicit LatencyHdrHistogram(std::int64_t lowest = 1, std::int64_t highest = 3'600'000'000'000LL,
                                 int significant_figures = kSignifikanteStellen) noexcept {
        if (hdr_init(lowest, highest, significant_figures, &h_) != 0) h_ = nullptr;
    }

    ~LatencyHdrHistogram() {
        if (h_ != nullptr) hdr_close(h_);
    }

    // Die Zaehler wandern MIT dem Histogramm. Blieben sie zurueck, waere der Verwurf nach einem
    // move nicht mehr abfragbar -- also wieder still, nur eine Ebene hoeher.
    LatencyHdrHistogram(LatencyHdrHistogram&& other) noexcept
        : h_(std::exchange(other.h_, nullptr)), verworfen_null_(std::exchange(other.verworfen_null_, 0)),
          verworfen_negativ_(std::exchange(other.verworfen_negativ_, 0)),
          verworfen_ausserhalb_(std::exchange(other.verworfen_ausserhalb_, 0)) {}

    LatencyHdrHistogram& operator=(LatencyHdrHistogram&& other) noexcept {
        if (this != &other) {
            if (h_ != nullptr) hdr_close(h_);
            h_                    = std::exchange(other.h_, nullptr);
            verworfen_null_       = std::exchange(other.verworfen_null_, 0);
            verworfen_negativ_    = std::exchange(other.verworfen_negativ_, 0);
            verworfen_ausserhalb_ = std::exchange(other.verworfen_ausserhalb_, 0);
        }
        return *this;
    }

    LatencyHdrHistogram(LatencyHdrHistogram const&)            = delete;
    LatencyHdrHistogram& operator=(LatencyHdrHistogram const&) = delete;

    /// Nimmt eine Probe auf ODER zaehlt, warum nicht. Es gibt keinen dritten Ausgang.
    void record(std::int64_t ns) noexcept {
        if (h_ == nullptr) return; // nicht bereit -- bereit() meldet das, ein Zaehler waere sinnlos
        if (ns < 0) {
            ++verworfen_negativ_;
            return;
        }
        if (ns == 0) {
            ++verworfen_null_;
            return;
        }
        // Der Rueckgabewert von hdr_record_value ist die EINZIGE Stelle, an der ein Wert ueber der
        // Histogramm-Obergrenze auffaellt. Ihn wegzuwerfen hiess, die zu klein gewaehlte Spanne nie
        // zu erfahren.
        if (!hdr_record_value(h_, ns)) ++verworfen_ausserhalb_;
    }

    /// false = hdr_init ist fehlgeschlagen; dann geht JEDE Probe verloren, nicht nur einzelne.
    [[nodiscard]] bool bereit() const noexcept { return h_ != nullptr; }

    /// Proben unterhalb der Uhr-Aufloesung (0 ns) -- reale Messungen, die das Histogramm nicht fasst.
    [[nodiscard]] std::int64_t verworfen_null() const noexcept { return verworfen_null_; }
    /// Proben mit unmoeglicher Dauer (< 0 ns) -- ein Defekt der Zeitnahme, keine Messung.
    [[nodiscard]] std::int64_t verworfen_negativ() const noexcept { return verworfen_negativ_; }
    /// Proben ueber highest_trackable_value -- die Histogramm-Spanne ist zu klein gewaehlt.
    [[nodiscard]] std::int64_t verworfen_ausserhalb() const noexcept { return verworfen_ausserhalb_; }

    /// Der NENNER: alle vorgelegten Proben. Ohne ihn ist jede Verwurf-Zahl unlesbar.
    [[nodiscard]] std::int64_t vorgelegt() const noexcept {
        return count() + verworfen_null_ + verworfen_negativ_ + verworfen_ausserhalb_;
    }

    [[nodiscard]] std::int64_t value_at(double q) const noexcept {
        if (h_ == nullptr || h_->total_count == 0) return 0;
        if (q < 0.0) q = 0.0;
        if (q > 1.0) q = 1.0;
        return hdr_value_at_percentile(h_, q * 100.0);
    }

    [[nodiscard]] std::int64_t p50() const noexcept { return value_at(0.50); }
    [[nodiscard]] std::int64_t p95() const noexcept { return value_at(0.95); }
    [[nodiscard]] std::int64_t p99() const noexcept { return value_at(0.99); }

    [[nodiscard]] std::int64_t min() const noexcept {
        return (h_ != nullptr && h_->total_count != 0) ? hdr_min(h_) : 0;
    }

    [[nodiscard]] std::int64_t max() const noexcept {
        return (h_ != nullptr && h_->total_count != 0) ? hdr_max(h_) : 0;
    }

    [[nodiscard]] std::int64_t count() const noexcept { return h_ != nullptr ? h_->total_count : 0; }

    [[nodiscard]] double mean() const noexcept { return (h_ != nullptr && h_->total_count != 0) ? hdr_mean(h_) : 0.0; }

    /// Schreibt die klassische HdrHistogram-Perzentil-Verteilung (.hgrm) in einen BEREITS OFFENEN Strom.
    ///
    /// WARUM DER STROM VON AUSSEN KOMMT: die persistierte Datei traegt mehr als die Verteilung -- sie
    /// braucht davor den Verwurf-Kopf mit seinem Nenner (s. hdr_lauf_persistieren). Beides in EINEN
    /// Strom zu schreiben ist die einzige Weise, die beiden Teile untrennbar zu halten; ein zweiter,
    /// getrennt geoeffneter Strom koennte die Verteilung ohne ihren Nenner hinterlassen.
    /// Der Strom gehoert dem Aufrufer: diese Methode oeffnet, schliesst und flusht ihn NICHT (so auch
    /// die ausdrueckliche Zusage der vendorierten Quelle, vendor/include/hdr/hdr_histogram.h:460).
    ///
    /// false = nicht bereit (hdr_init fehlgeschlagen), kein Strom, oder der C-Kern meldete einen
    /// Schreibfehler. Der Rueckgabewert von hdr_percentiles_print ist die EINZIGE Stelle, an der ein
    /// EIO auffaellt -- ihn wegzuwerfen hiesse, eine halb geschriebene Verteilung fuer voll zu nehmen.
    [[nodiscard]] bool verteilung_schreiben(std::FILE* strom, std::int32_t ticks_pro_halbdistanz = 5,
                                            double wert_skala = 1.0) const noexcept {
        if (h_ == nullptr || strom == nullptr) return false;
        // h_ ist in einer const-Methode "hdr_histogram* const" -- der Pointee bleibt nicht-const, die
        // C-Signatur passt also ohne const_cast. hdr_percentiles_print iteriert nur.
        return hdr_percentiles_print(h_, strom, ticks_pro_halbdistanz, wert_skala, CLASSIC) == 0;
    }

    [[nodiscard]] static LatencyHdrHistogram from_samples(std::span<std::int64_t const> ns) noexcept {
        LatencyHdrHistogram hist;
        for (std::int64_t const value : ns) hist.record(value);
        return hist;
    }
};

} // namespace comdare::cache_engine::measurement