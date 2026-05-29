#pragma once
// V41.F.6.1 R5.D — Kombinatorische Coverage des Permutations-Raums (Sampling-Grundlage).
//
// @topic anatomy
//
// **Problem:** der volle kartesische Achsen-Raum (Produkt aller Achsen-Varianten-Zahlen) ist
// kombinatorisch riesig (≈1e15+, vgl. G.1-Hierarchie-Summary) und KANN NICHT vollstaendig zu DLLs
// gebaut/gemessen werden. Statt "alles bauen" braucht es eine PRINZIPIELLE STICHPROBE mit
// definierter Abdeckungs-Garantie (kombinatorisches Testen / Covering Arrays).
//
// **Diese Utility** quantifiziert den Raum und liefert eine traktable **1-wise-Ueberdeckung**
// ("each-value"): eine Stichprobe, in der JEDE Variante JEDER Achse mindestens einmal vorkommt —
// Groesse = max(Achsen-Varianten-Zahl) statt Produkt. Damit wird "voller kartesischer Raum"
// von intraktabel zu beweisbar-abgedeckt-per-Sampling. Zusaetzlich die Pairwise-Untergrenze
// (Produkt der zwei groessten Achsen) als Richtwert fuer 2-wise-Coverage.
//
// Rein wertbasiert (Achsen-Varianten-Zahlen als Eingabe) → unabhaengig von den konkreten Achsen-Typen,
// per Unit-Test verifizierbar. Keine Allokation im Hot-Path noetig (Analyse zur Mess-/Build-Planung).

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace comdare::cache_engine::anatomy {

/// Sentinel: voller Raum ueberschreitet uint64 → als "gesaettigt" markiert (statt Overflow-UB).
inline constexpr std::uint64_t kCoverageSaturated = ~std::uint64_t{0};

struct CoverageReport {
    std::size_t   axis_count {0};            ///< Zahl der Achsen
    std::uint64_t full_space {1};            ///< Produkt aller Varianten-Zahlen (saturiert bei Overflow)
    bool          full_space_saturated {false};
    std::size_t   one_wise_cover {0};        ///< 1-wise-Stichprobengroesse = max(counts)
    std::uint64_t pairwise_lower_bound {0};  ///< Untergrenze 2-wise = Produkt der zwei groessten counts
    bool          any_axis_empty {false};    ///< eine Achse hat 0 Varianten → Raum leer
};

/// Analysiert den Permutations-Raum aus den Achsen-Varianten-Zahlen.
[[nodiscard]] inline CoverageReport analyze_coverage(std::span<const std::size_t> counts) noexcept {
    CoverageReport r {};
    r.axis_count = counts.size();
    if (counts.empty()) { r.full_space = 0; return r; }

    std::size_t max1 = 0, max2 = 0;   // zwei groesste counts (fuer Pairwise-Untergrenze)
    for (std::size_t c : counts) {
        if (c == 0) { r.any_axis_empty = true; }
        r.one_wise_cover = std::max(r.one_wise_cover, c);
        if (c > max1) { max2 = max1; max1 = c; }
        else if (c > max2) { max2 = c; }
        // full_space *= c mit Overflow-Saettigung
        if (!r.full_space_saturated) {
            if (c == 0) { r.full_space = 0; }
            else if (r.full_space > kCoverageSaturated / c) { r.full_space_saturated = true; r.full_space = kCoverageSaturated; }
            else { r.full_space *= c; }
        }
    }
    r.pairwise_lower_bound = static_cast<std::uint64_t>(max1) * static_cast<std::uint64_t>(max2);
    if (r.any_axis_empty) { r.full_space = 0; }
    return r;
}

/// Erzeugt eine **1-wise-Ueberdeckungs-Stichprobe**: pro Zeile r (0..max-1) waehlt Achse i die
/// Variante (r mod counts[i]). Da r alle Werte 0..max-1 (>= counts[i]-1) durchlaeuft, kommt jede
/// Variante jeder Achse mindestens einmal vor. Rueckgabe: Vektor von Index-Tupeln (Zeile → Achse → Variante).
/// Leerer Rueckgabewert wenn eine Achse 0 Varianten hat (Raum leer).
[[nodiscard]] inline std::vector<std::vector<std::size_t>>
one_wise_cover_sample(std::span<const std::size_t> counts) {
    std::vector<std::vector<std::size_t>> rows;
    if (counts.empty()) return rows;
    std::size_t max_c = 0;
    for (std::size_t c : counts) { if (c == 0) return rows; max_c = std::max(max_c, c); }
    rows.reserve(max_c);
    for (std::size_t r = 0; r < max_c; ++r) {
        std::vector<std::size_t> tuple(counts.size());
        for (std::size_t i = 0; i < counts.size(); ++i) tuple[i] = r % counts[i];
        rows.push_back(std::move(tuple));
    }
    return rows;
}

}  // namespace comdare::cache_engine::anatomy
