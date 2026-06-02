#pragma once
// Container-Gattung-Bau-Brücke (2026-06-02, User-Option-B Schritt 3) — minimale, ECHTE Container/Queue-Anatomie
// über die queuing-Achse Q1 (buffer_strategy). Die 2. Gattungs-Instanz neben SearchAlgorithmAnatomy.
//
// 22-vs-17 (Doc 27 §0.1): queuing q1/q2 sind eine EIGENE Gattung (Container, Tier-Metapher Adapter = Wrapper über
// Inner-Container, Doku 14 §27.2) — NICHT in die SearchAlgorithm-17-Komposition (Cross-Genus type-unmöglich,
// Doku 14 §32). Diese Anatomie hält das echte Q1-Buffer-Organ, treibt es (put/get) und liefert einen EIGENEN
// Container-Observer (NICHT ObserverAggregate<17>). So baut der EINE Experiment-Baum auch die Container-Gattung.
//
// Minimal: 1 Slot (Q1 buffer_strategy). Erweiterbar um Q2 (flush_policy) + geteilte Organe (allocator/concurrency).
// C++23, header-only.

#include "anatomy_base.hpp"   // AnatomyGenus

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// Container-Observer (queuing-Gattung) — EIGENER flacher Observer-POD, getrennt vom SearchAlgorithm-
/// ObserverAggregate<17> (gattungs-korrekt). Nur uint64 → standard_layout.
struct ContainerObserverSnapshot {
    std::uint64_t put_count          = 0;
    std::uint64_t get_count          = 0;
    std::uint64_t current_occupancy  = 0;
    std::uint64_t peak_occupancy     = 0;
};

/// ContainerComposition<Q1Buffer> — die Container-Gattungs-Komposition (Queue-Buffer-Strategy als Kern-Organ).
template <class Q1Buffer>
struct ContainerComposition {
    using buffer_strategy = Q1Buffer;
    static constexpr std::string_view name     = "ContainerComposition";
    static constexpr std::string_view paper_id = "P00 Container Gattung (Queue)";
};

/// ContainerAnatomy<Composition> — die Container-GATTUNG (genus()==Adapter). Hält das echte Q1-Buffer-Organ,
/// treibt es über die Gattungs-API (put/get/size/clear) und liefert observe_all → ContainerObserverSnapshot.
template <class Composition>
class ContainerAnatomy {
public:
    using composition_t = Composition;
    using buffer_t      = typename Composition::buffer_strategy;
    using element_type  = typename buffer_t::element_type;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id()         noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus()            noexcept { return AnatomyGenus::Adapter; }  // Queue = Container-Adapter
    static constexpr std::size_t      organ_count()      noexcept { return 1; }                      // minimal: buffer_strategy

    // ── Gattungs-API (Container/Queue) — treibt das ECHTE Q1-Organ + aktualisiert den Observer ──
    void put(element_type v) {
        buffer_.put(v);
        ++obs_.put_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(buffer_.size());
        if (obs_.current_occupancy > obs_.peak_occupancy) obs_.peak_occupancy = obs_.current_occupancy;
    }
    [[nodiscard]] std::optional<element_type> get() {
        auto r = buffer_.get();
        if (r) { ++obs_.get_count; obs_.current_occupancy = static_cast<std::uint64_t>(buffer_.size()); }
        return r;
    }
    [[nodiscard]] std::size_t size() const noexcept { return buffer_.size(); }
    void clear() noexcept { buffer_.clear(); obs_.current_occupancy = 0; }

    /// observe_all() — EIGENER Container-Gattungs-Observer (NICHT der 17-Achsen-SearchAlgorithm-Aggregate).
    [[nodiscard]] ContainerObserverSnapshot observe_all() const noexcept { return obs_; }

private:
    buffer_t                  buffer_{};
    ContainerObserverSnapshot obs_{};
};

}  // namespace comdare::cache_engine::anatomy
