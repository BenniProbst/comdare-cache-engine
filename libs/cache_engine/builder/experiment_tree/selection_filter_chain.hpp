#pragma once
// selection_filter_chain.hpp — S4 (mess-getriebene Reduktionsstufe): Chain of Responsibility (GoF) im
// CacheEngineBuilder fuer die Kontrolle der Tier-Binary-GENERIERUNG (Doc 20 §C). Schliesst die heute fehlende
// Feedback-Kante Auswertung (best_binary_selector) -> Generierung (build_orchestrator provision_all): aus der
// aktuellen Kandidaten-BuildSelection + den Messergebnissen (MeasurementRow, join ueber binary_id) entsteht eine
// reduzierte BuildSelection fuer die naechste Bau-Runde.
//
// @topic builder @schicht experiment_tree @pattern Chain of Responsibility (GoF, cold-path/build-time)
//
// COLD-PATH: die Kette laeuft zwischen Mess-Runde und dem naechsten provision_all-Aufruf, NICHT im gemessenen
// Hot-Path (kein do_batch) -> klassische GoF-CoR mit Handler-Objekten zulaessig (keine vtable/Runtime-Switch-
// Sorge; die compile-time-strikt-Doktrin bindet nur den gemessenen Hot-Path). Nur benannte Patterns: CoR (Kette)
// + Strategy (Kriterien, bestehend RankingCriterion). Provenance-Audit fortgeschrieben (kein stilles Truncaten).
//
// SCOPE (Slice 1): der CoR-Mechanismus + EIN korrekt-gerichteter, doc-belegter Kern-Handler (ResumeFilter,
// Resume KF-16b + Mess-Ehrlichkeit Doc 20 §D). Die thesis-payoff-Handler sind bewusst DEFERRED (additiv als
// weitere Kettenglieder): PaperComparison (Doc 20 §D-Schritt-4, Wall-Clock-Vorrang) braucht eine Paper-Wall-
// Clock-Referenzquelle = data-gated (sota_catalog.hpp ist ein Materialisierungs-Katalog, keine Messreferenz);
// DominanceHandler/Pareto (§B) braucht das Messkurven-Typsystem-Schluessel (framework×workload×op×size), das
// noch nicht gebaut ist. Reihenfolge-Vorrang Paper-vor-Pareto (§D) wird beim Nachziehen dieser Glieder kodiert.

#include <builder/best_binary_selector/best_binary_selector.hpp> // best_binary::MeasurementRow (Auswertungs-Zeile)
#include <builder/experiment_tree/coverage_selection.hpp>        // experiment::BuildSelection (S1/S2/S3-Selektion)

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

/// Verdikt eines Filter-Glieds (GoF-CoR). Survive-Regel (WOHLDEFINIERT): ein Kandidat UEBERLEBT (kommt in die
/// reduzierte BuildSelection) genau dann, wenn die Kette KEIN Reject liefert. Pass = terminal behalten (Override,
/// ueberspringt Rest); PassToNext = kein Einwand -> naechstes Glied fragen (am Ketten-Ende ohne Reject = ueberlebt).
struct FilterVerdict {
    enum class Decision { Pass, Reject, PassToNext };
    Decision    decision = Decision::PassToNext;
    std::string reason;
};

/// Ein Kandidat der Generierungs-Steuerung: ein View-Index (je -> BinarySpec/binary_id) + optionale Auswertung.
struct PermutationCandidate {
    std::size_t                                view_index = 0;
    std::string                                binary_id; ///< 17-Achsen-Join-Schluessel (Auswertung <-> Generierung)
    std::optional<best_binary::MeasurementRow> row;       ///< Auswertung (falls schon gemessen), sonst nullopt
};

/// Abstraktes CoR-Glied mit Nachfolger-Verkettung (GoF Chain of Responsibility).
class FilterHandler {
public:
    FilterHandler()                                = default;
    FilterHandler(FilterHandler const&)            = default;
    FilterHandler& operator=(FilterHandler const&) = default;
    virtual ~FilterHandler()                       = default;

    void set_next(FilterHandler const* n) noexcept { next_ = n; }
    /// Verkettete Auswertung: Reject/Pass terminal; PassToNext -> naechstes Glied (bzw. am Ende PassToNext).
    [[nodiscard]] FilterVerdict dispatch(PermutationCandidate const& c) const {
        FilterVerdict v = handle(c);
        if (v.decision == FilterVerdict::Decision::PassToNext && next_ != nullptr) return next_->dispatch(c);
        return v;
    }

protected:
    [[nodiscard]] virtual FilterVerdict handle(PermutationCandidate const&) const = 0;

private:
    FilterHandler const* next_ = nullptr;
};

/// ResumeFilter (Resume KF-16b + Mess-Ehrlichkeit Doc 20 §D): eine Permutation, die BEREITS eine two_phase_valid-
/// Messung traegt, wird NICHT neu gebaut/gemessen (Reject aus dem Bau-Set) — nur unmess/invalide Kandidaten
/// passieren (PassToNext -> bauen). Das ist die eigentliche evaluation->generation-Feedback-Kante (Doc 20 §C),
/// korrekt-gerichtet (valide = fertig; invalide = neu messen) und NICHT-duplikativ zum DLL-Cache-Skip des
/// Orchestrators (der ueber DLL-Existenz/Version entscheidet, NICHT ueber Mess-Validitaet).
class ResumeFilter final : public FilterHandler {
protected:
    [[nodiscard]] FilterVerdict handle(PermutationCandidate const& c) const override {
        if (c.row.has_value() && c.row->two_phase_valid)
            return {FilterVerdict::Decision::Reject, "resume-skip: bereits two_phase_valid gemessen"};
        return {FilterVerdict::Decision::PassToNext, "keine valide Messung -> bauen/messen"};
    }
};

/// Fuehrt die GEORDNETE CoR ueber die Kandidaten und liefert die reduzierte BuildSelection (Ueberlebende =
/// dispatch-Verdikt != Reject). Leere Kette -> alle ueberleben (identitaet). provenance wird fortgeschrieben.
[[nodiscard]] inline BuildSelection run_selection_filter_chain(std::vector<PermutationCandidate> const& candidates,
                                                               std::vector<FilterHandler*> const&       ordered,
                                                               std::string const& upstream_provenance = "") {
    for (std::size_t i = 0; i + 1 < ordered.size(); ++i) ordered[i]->set_next(ordered[i + 1]); // successor-Link
    FilterHandler const* head = ordered.empty() ? nullptr : ordered.front();

    BuildSelection out;
    for (PermutationCandidate const& c : candidates) {
        FilterVerdict const v =
            (head != nullptr) ? head->dispatch(c) : FilterVerdict{FilterVerdict::Decision::PassToNext, ""};
        if (v.decision != FilterVerdict::Decision::Reject) out.indices.push_back(c.view_index);
    }
    out.provenance = upstream_provenance.empty() ? "filtered:cor" : (upstream_provenance + "|filtered:cor");
    return out;
}

} // namespace comdare::cache_engine::builder::experiment
