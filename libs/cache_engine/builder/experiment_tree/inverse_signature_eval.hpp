#pragma once
// KF-15 (2026-06-02) — inverse Auswertung via STATISCHER SIGNATUR (NICHT Fingerprints) + read-only Tree-Interface.
//
// Doc 26 §3 (maßgeblich): Ein Paper-Tier = statische Rekombination über die GEPINNTEN Achsen; seine „statische
// Signatur" (Array der gepinnten Achsen-Werte) ist der Wiedererkennungswert. Die inverse Auswertung = Projektion
// der real gemessenen Blätter auf die Paper-Sichten per Signatur-Filter (LINEARE Traversierung, „für Suche immer
// Bäume"). Mehrere Paper können DIESELBE Signatur teilen → multimap<Signatur, Paper> (Doppelerkennung).
//
// Schicht-Trennung (Doc 26 §0): Dies ist die READ-ONLY-Seite, über die die DIPLOMARBEIT die Ergebnisse liest
// (das WAS) — sie traversiert den Baum lesend, greift NICHT in Tiere/Organe/Baum ein. C++23, header-only.

#include "experiment_tree.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::experiment {

// ── Paper-Wiedererkennung: multimap<gepinnte Signatur, Paper-ID> (Doc 26 §3, Doppelerkennung) ──
class PaperSignatureIndex {
public:
    /// Registriert ein Paper unter seiner gepinnten Signatur (z.B. P01 → "traversal=ART").
    void add(std::string paper_id, std::string pinned_signature) {
        sig_to_paper_.emplace(std::move(pinned_signature), std::move(paper_id));
    }
    /// Alle Paper, die EXAKT diese Signatur teilen (mehrere möglich → Doppelerkennung).
    [[nodiscard]] std::vector<std::string> papers_for(std::string const& signature) const {
        std::vector<std::string> out;
        auto [lo, hi] = sig_to_paper_.equal_range(signature);
        for (auto it = lo; it != hi; ++it) out.push_back(it->second);
        return out;
    }
    /// Die Signatur(en) eines Papers (rückwärts; i.d.R. genau eine).
    [[nodiscard]] std::vector<std::string> signatures_for(std::string const& paper_id) const {
        std::vector<std::string> out;
        for (auto const& [sig, pid] : sig_to_paper_) if (pid == paper_id) out.push_back(sig);
        return out;
    }
    [[nodiscard]] std::size_t size() const noexcept { return sig_to_paper_.size(); }
    [[nodiscard]] std::multimap<std::string, std::string> const& raw() const noexcept { return sig_to_paper_; }

private:
    std::multimap<std::string, std::string> sig_to_paper_;
};

// ── Read-only Tree-Interface (Diplomarbeit liest Ergebnisse via Baum-Traversal, projiziert per Signatur) ──
class ReadOnlyResultView {
public:
    explicit ReadOnlyResultView(ExperimentTree const& tree)
        : tree_{tree}, sig_to_binary_{tree.pinned_signature_index()} {}

    /// Alle Binaries (read-only), deren gepinnte Signatur == `signature` — das Teilexperiment EINES Papers
    /// (bzw. mehrerer, die die Signatur teilen). Lineare Projektion (equal_range über die Signatur-multimap).
    [[nodiscard]] std::vector<std::string> binaries_with_signature(std::string const& signature) const {
        std::vector<std::string> out;
        auto [lo, hi] = sig_to_binary_.equal_range(signature);
        for (auto it = lo; it != hi; ++it) out.push_back(it->second);
        return out;
    }

    /// Alle distinkten gepinnten Signaturen im Baum (= die unterscheidbaren Paper-Sichten).
    [[nodiscard]] std::vector<std::string> signatures() const {
        std::set<std::string> uniq;
        for (auto const& [sig, _bin] : sig_to_binary_) uniq.insert(sig);
        return {uniq.begin(), uniq.end()};
    }

    /// INVERSE AUSWERTUNG: projiziert die gemessenen Blätter auf die Sicht EINES Papers — alle Binaries, deren
    /// Signatur einer der Paper-Signaturen entspricht (Doc 26 §3). Dedupliziert.
    [[nodiscard]] std::vector<std::string> binaries_for_paper(
        std::string const& paper_id, PaperSignatureIndex const& papers) const {
        std::set<std::string> bins;
        for (auto const& sig : papers.signatures_for(paper_id))
            for (auto const& b : binaries_with_signature(sig)) bins.insert(b);
        return {bins.begin(), bins.end()};
    }

    /// Aggregierte Mess-Auswertung (NodeValue) über alle Binaries einer Signatur — read-only Projektion der
    /// per-node persistierten Observer-Statistics (Doc 26 §2). measured_setting_count zählt die Binaries.
    [[nodiscard]] NodeValue aggregate_for_signature(std::string const& signature) const {
        NodeValue agg;
        tree_.for_each_binary([&](std::string const&, std::string const& pin, TreeNode const& leaf) {
            if (pin != signature) return;
            agg.measured_setting_count += (leaf.value.has_result ? leaf.value.measured_setting_count : 1);
            agg.sum_total_cycles       += leaf.value.sum_total_cycles;
            agg.sum_op_count           += leaf.value.sum_op_count;
            agg.has_result              = agg.has_result || leaf.value.has_result;
        });
        return agg;
    }

private:
    ExperimentTree const&                    tree_;
    std::multimap<std::string, std::string>  sig_to_binary_;  // Signatur → binary_id (Doc 26 §3)
};

}  // namespace comdare::cache_engine::builder::experiment
