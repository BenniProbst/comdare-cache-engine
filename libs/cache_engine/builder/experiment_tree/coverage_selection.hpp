#pragma once
// D1 / L-SEL (Goal-V6, 2026-06-02) — BuildSelection: Übersetzung der ∏-lazy StaticBinaryView in eine ENDLICHE,
// reale Build-Index-Liste. Doc 28 §5 („reale Cluster-Build-Menge ≪ ∏; BUILD-PROFIL/Enabled-Flags/Coverage wählt"),
// Doc 22 §4 (1-wise: max(counts) statt ∏), messarchitektur_v5_drei_profile §1.
//
// PROBLEM: binary_count() == ∏ mp_size ist astronomisch (1.4e14). Niemals alle ∏ als Index-Vektor materialisieren
// (OOM, Doc 26 §2). Diese Utility liefert die EINE endliche `BuildSelection.indices`, die provision_all (O(K)),
// der SLURM-Array-Generator und der E2E-Treiber konsumieren — statt der ganzen View.
//
// 3 orthogonale Reduktionsstufen (kombinierbar): S1 BUILD-PROFIL (Enabled-Listen, configure-time → kleines ∏)
// ⊥ S2 Coverage-Sampling (1-wise über die Lazy-View, hier) ⊥ S3 explizite/pinned Selektion. Header-only, C++23.

#include "experiment_tree.hpp"
#include "../../anatomy/combinatorial_coverage.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Die EINE endliche, gewählte Index-Liste in den statischen Teilbaum (View-Indizes, je → BinarySpec via view[i]).
/// `provenance` dokumentiert die Quelle/Reduktionsstufe (Audit: kein stilles Truncaten — Doc 28 §0).
struct BuildSelection {
    std::vector<std::size_t> indices;       ///< gewählte View-Indizes (endlich, ≪ ∏)
    std::string              provenance;     ///< "full" / "one_wise" / "explicit" / "pinned:<sig>" / "*-REFUSED-..."
    [[nodiscard]] std::size_t size()  const noexcept { return indices.size(); }
    [[nodiscard]] bool        empty() const noexcept { return indices.empty(); }
};

/// Sicherheits-Default: select_full verweigert Materialisierung über dieser Grenze (OOM-Schutz; Doc 26 §2).
inline constexpr std::size_t kFullSelectMaxDefault = std::size_t{1} << 20;  // ~1.05 Mio Binaries

/// S0 (nur Pilot/handhabbar): ALLE View-Indizes 0..size()-1. Über `max_full` wird NICHT materialisiert (kein OOM) —
/// stattdessen leere Selektion mit Provenance-Markierung (ehrlicher Audit-Hinweis statt stillem Truncaten).
[[nodiscard]] inline BuildSelection select_full(StaticBinaryView const& view,
                                                std::size_t max_full = kFullSelectMaxDefault) {
    BuildSelection sel;
    if (view.size() > max_full) {
        sel.provenance = "full-REFUSED-too-large(" + std::to_string(view.size()) + ">" + std::to_string(max_full) + ")";
        return sel;  // leer — Aufrufer muss one_wise/explicit/pinned wählen
    }
    sel.provenance = "full";
    sel.indices.reserve(view.size());
    for (std::size_t i = 0; i < view.size(); ++i) sel.indices.push_back(i);
    return sel;
}

/// S2 (DEFAULT für riesige Inventare): 1-wise-Überdeckung ("each-value") über die Lazy-View. Größe = max(level_size)
/// statt ∏ → OOM-sicher auch bei 1e14-View (nur max(counts) Zeilen). Jede Variante JEDER Achse kommt ≥1× vor.
/// Mappt jedes Coverage-Tupel via view.flat_index() auf einen flachen View-Index.
[[nodiscard]] inline BuildSelection select_one_wise(StaticBinaryView const& view) {
    BuildSelection sel;
    sel.provenance = "one_wise";
    std::vector<std::size_t> counts;
    counts.reserve(view.level_count());
    for (std::size_t d = 0; d < view.level_count(); ++d) counts.push_back(view.level_size(d));
    auto const rows = anatomy::one_wise_cover_sample(counts);  // O(max(counts)) Index-Tupel
    sel.indices.reserve(rows.size());
    for (auto const& tuple : rows) sel.indices.push_back(view.flat_index(tuple));
    return sel;
}

/// S3a: explizite Index-Liste (z.B. gepinnte Pfade aus einem Manifest / Resume-Restmenge).
[[nodiscard]] inline BuildSelection select_explicit(std::vector<std::size_t> ids) {
    BuildSelection sel;
    sel.provenance = "explicit";
    sel.indices    = std::move(ids);
    return sel;
}

/// S3b: alle Binaries, deren gepinnte Signatur exakt `signature` ist (Paper-Wiedererkennung, KF-15). LAZY-Scan
/// über view[i].pinned_signature — O(∏) ZEIT, daher `max_scan`-Cap (verweigert über der Grenze, kein Endlos-Scan).
/// Für riesige Inventare zuerst per S1 (Enabled) eindampfen.
[[nodiscard]] inline BuildSelection select_by_pinned_signature(StaticBinaryView const& view,
                                                               std::string const& signature,
                                                               std::size_t max_scan = kFullSelectMaxDefault) {
    BuildSelection sel;
    if (view.size() > max_scan) {
        sel.provenance = "pinned-REFUSED-scan-too-large(" + std::to_string(view.size()) + ">" + std::to_string(max_scan) + ")";
        return sel;
    }
    sel.provenance = "pinned:" + signature;
    for (std::size_t i = 0; i < view.size(); ++i)
        if (view[i].pinned_signature == signature) sel.indices.push_back(i);
    return sel;
}

}  // namespace comdare::cache_engine::builder::experiment
