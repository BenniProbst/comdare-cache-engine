#pragma once
// L-76b (2026-06-03, Doc 24 §8.8) — Sequence-Gattungs-Prüf-Dock: die CacheEngineBuilder-Seite für die
// SEQUENCE-Gattung (Reptil, genus()==Sequence), analog ContainerDock/SearchAlgorithmDock.
//
// Doc 24 §8.8: per-Gattung Mess-Übergang — (a) Tier über die Gattungs-API treiben, (b) Observer messen, (c)
// persistieren. Hier in-process über die SequenceAnatomy (push_back/at-Workload → SequenceObserverSnapshot → CSV).
// Treibt REAL die axis_growth-Policy der Komposition (growth_events). Gattungs-Constraint: genus()==Sequence
// (Cross-Genus type-unmöglich, Doku 14 §32). Der DLL-Pfad (AnatomyModuleLoader) ist ein Folgeschritt. C++23, header-only.
//
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

#include "anatomy/sequence_anatomy.hpp"   // SequenceAnatomy / SequenceObserverSnapshot
#include "anatomy/anatomy_base.hpp"       // AnatomyGenus

#include <cstdint>
#include <string>

namespace comdare::cache_engine::builder::pruef_dock {

/// SequenceDock<SequenceAnatomyT> — treibt ein Sequence-Tier (V-indexed) über die Gattungs-API (push_back/at) +
/// misst den eingebauten Sequence-Observer (inkl. growth_events der axis_growth-Policy) + persistiert.
template <class SequenceAnatomyT>
class SequenceDock {
public:
    struct MeasureResult {
        ::comdare::cache_engine::anatomy::SequenceObserverSnapshot observer{};
        std::uint64_t total_ops = 0;
    };

    /// Diese Dock-Seite bedient die Sequence-Gattung (Doc 24 §8.8 Gattungs-Bindung).
    [[nodiscard]] static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus() noexcept {
        return ::comdare::cache_engine::anatomy::AnatomyGenus::Sequence;
    }

    /// Treibt das Sequence-Tier: erst n_pushes push_back(0..n_pushes-1) (treibt die axis_growth-Policy → reserve →
    /// growth_events), dann n_reads at(0..n_reads-1) (i<n_pushes = in-bounds, sonst OOB). Zieht den Sequence-Observer.
    [[nodiscard]] MeasureResult measure(std::uint64_t n_pushes, std::uint64_t n_reads) const {
        SequenceAnatomyT tier;
        using elem_t = typename SequenceAnatomyT::element_type;
        for (std::uint64_t i = 0; i < n_pushes; ++i) tier.push_back(static_cast<elem_t>(i));
        for (std::uint64_t i = 0; i < n_reads;  ++i) (void)tier.at(i);
        return MeasureResult{ tier.observe_all(), n_pushes + n_reads };
    }

    /// Persistierung (Doc 24 §8.8 Schritt c): eine CSV-Zeile mit den korrelierten Sequence-Observer-Werten.
    [[nodiscard]] static std::string serialize_csv(MeasureResult const& r) {
        std::string s = "genus,total_ops,push_count,at_count,at_oob,current_size,peak_size,growth_events\n";
        s += "Sequence," + std::to_string(r.total_ops) + ","
           + std::to_string(r.observer.push_count) + "," + std::to_string(r.observer.at_count) + ","
           + std::to_string(r.observer.at_oob_count) + "," + std::to_string(r.observer.current_size) + ","
           + std::to_string(r.observer.peak_size) + "," + std::to_string(r.observer.growth_events) + "\n";
        return s;
    }
};

}  // namespace comdare::cache_engine::builder::pruef_dock
