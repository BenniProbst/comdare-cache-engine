#pragma once
// L-76c (2026-06-03, Doc 24 §8.8) — View-Gattungs-Prüf-Dock: die CacheEngineBuilder-Seite für die VIEW-Gattung
// (Pflanze, genus()==View, non-owning), analog AdapterDock/SearchAlgorithmDock.
//
// Doc 24 §8.8: per-Gattung Mess-Übergang — (a) Tier über die Gattungs-API treiben, (b) Observer messen, (c)
// persistieren. Hier in-process über die ViewAnatomy (bind/read-Workload → ViewObserverSnapshot → CSV). Die View
// ist non-owning → der gemessene Puffer lebt WÄHREND der Messung im Dock (das Tier referenziert ihn nur). Treibt
// REAL die axis_layout/axis_accessor-Policies (read über index_of/access). Gattungs-Constraint: genus()==View
// (Cross-Genus type-unmöglich, Doku 14 §32). Der DLL-Pfad (AnatomyModuleLoader) ist ein Folgeschritt. C++23, header-only.
//
// @related [[anatomie-gattungen]] [[gattungs-constraint-pruefling-merge]]

#include "anatomy/view_anatomy.hpp"   // ViewAnatomy / ViewObserverSnapshot
#include "anatomy/anatomy_base.hpp"   // AnatomyGenus

#include <cstdint>
#include <string>
#include <vector>

namespace comdare::cache_engine::builder::pruef_dock {

/// ViewDock<ViewAnatomyT> — bindet einen lokalen Puffer (non-owning View) + treibt n_reads read() über die
/// axis_layout/axis_accessor-Policy + misst den eingebauten View-Observer + persistiert.
template <class ViewAnatomyT>
class ViewDock {
public:
    struct MeasureResult {
        ::comdare::cache_engine::anatomy::ViewObserverSnapshot observer{};
        std::uint64_t total_ops = 0;
    };

    /// Diese Dock-Seite bedient die View-Gattung (Doc 24 §8.8 Gattungs-Bindung).
    [[nodiscard]] static constexpr ::comdare::cache_engine::anatomy::AnatomyGenus genus() noexcept {
        return ::comdare::cache_engine::anatomy::AnatomyGenus::View;
    }

    /// Bindet einen lokalen Puffer der Größe buffer_size (Werte 0..buffer_size-1) an das View-Tier und treibt
    /// n_reads read(0..n_reads-1) (i<buffer_size = in-bounds über layout/accessor, sonst OOB). Der Puffer lebt
    /// während der gesamten Messung (non-owning View referenziert ihn). Zieht den eingebauten View-Observer.
    [[nodiscard]] MeasureResult measure(std::uint64_t buffer_size, std::uint64_t n_reads) const {
        using elem_t = typename ViewAnatomyT::element_type;
        std::vector<elem_t> buf(static_cast<std::size_t>(buffer_size));
        for (std::uint64_t i = 0; i < buffer_size; ++i) buf[static_cast<std::size_t>(i)] = static_cast<elem_t>(i);
        ViewAnatomyT tier;
        tier.bind(buf.data(), buf.size());
        for (std::uint64_t i = 0; i < n_reads; ++i) (void)tier.read(i);
        return MeasureResult{ tier.observe_all(), n_reads };
    }

    /// Persistierung (Doc 24 §8.8 Schritt c): eine CSV-Zeile mit den korrelierten View-Observer-Werten.
    [[nodiscard]] static std::string serialize_csv(MeasureResult const& r) {
        std::string s = "genus,total_ops,read_count,read_oob,bound_size,bind_count\n";
        s += "View," + std::to_string(r.total_ops) + ","
           + std::to_string(r.observer.read_count) + "," + std::to_string(r.observer.read_oob_count) + ","
           + std::to_string(r.observer.bound_size) + "," + std::to_string(r.observer.bind_count) + "\n";
        return s;
    }
};

}  // namespace comdare::cache_engine::builder::pruef_dock
