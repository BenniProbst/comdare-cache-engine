#pragma once
// eta_estimator.hpp -- G3 / #46b Lagerhaltung, Scheibe B4 (Ledger §62-B P1 / §62-B-NACHTRAG-2).
//
// REINE Funktionen fuer die ETA- und avg_size-Kalibrierung eines Bau-Batches (N-9-ETA-Formel,
// literal testbar, keine Zeit-/IO-Abhaengigkeit). Die CEB kalibriert einen ersten Mini-Batch
// (Groesse ~ N_threads), leitet daraus ETA + avg_size ab und traegt beide je Batch-Block ins
// Bestandslog ein (§62-B-NACHTRAG-2: avg_size ist der ZWEITE Log-Wert neben ETA).
//
// ETA-FORMEL (Design §1.5): ETA = Sum(t_i) / N_threads, mit HARTER Untergrenze max(t_i) -- selbst
// mit unbegrenzt vielen Threads kann ein Batch nie schneller sein als sein laengster einzelner
// Compile. avg_size = arithmetisches Mittel der Binary-Groessen des Blocks.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare (§ erlaubt), nur stdlib. Keine vtables, kein Runtime-
// Switch -- reine Arithmetik.

#include <cstdint>
#include <span>

namespace comdare::cache_engine::builder::bestandslog {

// Ergebnis einer Mini-Batch-Kalibrierung: geschaetzte Restzeit + durchschnittliche Binary-Groesse.
struct EtaResult {
    double        eta_s          = 0.0;
    std::uint64_t avg_size_bytes = 0;

    friend bool operator==(EtaResult const&, EtaResult const&) = default;
};

// ETA eines Batches aus den gemessenen per-Binary-Compile-Zeiten (Sekunden) bei N_threads parallel:
//   ETA = max( Sum(t_i) / N_threads , max(t_i) ).
// n_threads==0 wird als 1 behandelt (Division-Schutz). Leere Eingabe -> 0.
[[nodiscard]] inline double estimate_eta_s(std::span<double const> times_s, unsigned n_threads) noexcept {
    if (times_s.empty()) return 0.0;
    unsigned const n   = (n_threads == 0) ? 1u : n_threads;
    double         sum = 0.0;
    double         mx  = 0.0;
    for (double t : times_s) {
        sum += t;
        if (t > mx) mx = t;
    }
    double const parallel = sum / static_cast<double>(n);
    return (parallel > mx) ? parallel : mx;
}

// Arithmetisches Mittel der Binary-Groessen eines Blocks (leere Eingabe -> 0).
[[nodiscard]] inline std::uint64_t average_size_bytes(std::span<std::uint64_t const> sizes) noexcept {
    if (sizes.empty()) return 0;
    std::uint64_t sum = 0;
    for (auto s : sizes) sum += s; // 4096 x ~428KB ~ 1.7GB << uint64
    return sum / static_cast<std::uint64_t>(sizes.size());
}

// Projiziert die ETA fuer das GANZE noch offene Slice-Fenster aus der Mini-Batch-Kalibrierung:
//   ETA = max( remaining_count * avg_time_s / N_threads , longest_seen_s ).
// avg_time_s = mittlere per-Binary-Zeit der Kalibrierung; longest_seen_s haelt die Untergrenze
// (der bisher laengste beobachtete Compile). n_threads==0 -> 1.
[[nodiscard]] inline double project_slice_eta_s(double avg_time_s, std::uint64_t remaining_count, unsigned n_threads,
                                                double longest_seen_s) noexcept {
    unsigned const n         = (n_threads == 0) ? 1u : n_threads;
    double const   projected = (static_cast<double>(remaining_count) * avg_time_s) / static_cast<double>(n);
    return (projected > longest_seen_s) ? projected : longest_seen_s;
}

// Kombinierte Kalibrierung: aus den Mini-Batch-Zeiten + Groessen den EtaResult fuer den Block bilden
// (ETA fuer die vermessenen Binaries selbst; die Slice-Projektion nutzt project_slice_eta_s).
[[nodiscard]] inline EtaResult calibrate_block(std::span<double const> times_s, std::span<std::uint64_t const> sizes,
                                               unsigned n_threads) noexcept {
    return EtaResult{estimate_eta_s(times_s, n_threads), average_size_bytes(sizes)};
}

} // namespace comdare::cache_engine::builder::bestandslog
