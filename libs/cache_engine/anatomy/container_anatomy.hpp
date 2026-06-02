#pragma once
// Container-Gattung-Bau-Brücke (2026-06-02, User-Option-B Schritt 3) — minimale, ECHTE Container/Queue-Anatomie
// über die queuing-Achse Q1 (buffer_strategy). Die 2. Gattungs-Instanz neben SearchAlgorithmAnatomy.
//
// 22-vs-17 (Doc 27 §0.1): queuing q1/q2 sind eine EIGENE Gattung (Container, Tier-Metapher Adapter = Wrapper über
// Inner-Container, Doku 14 §27.2) — NICHT in die SearchAlgorithm-17-Komposition (Cross-Genus type-unmöglich,
// Doku 14 §32). Diese Anatomie hält das echte Q1-Buffer-Organ, treibt es (put/get) und liefert einen EIGENEN
// Container-Observer (NICHT ObserverAggregate<17>). So baut der EINE Experiment-Baum auch die Container-Gattung.
//
// D4 / L-75 (2026-06-02): 2 Slots — Q1 (buffer_strategy) + Q2 (flush_policy). Erweiterbar um geteilte Organe.
// C++23, header-only. ENTKOPPELT von der q2-Topic (kein Boost/generated-Include): FlushDecision wird gespiegelt
// (ABI-Konvention 0/1/2), die ECHTE FlushPolicy (z.B. WatermarkFlush) setzt der Aufrufer als Q2Flush ein.

#include "anatomy_base.hpp"   // AnatomyGenus

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace comdare::cache_engine::anatomy {

/// Gespiegelt von queuing::axis_q2_queuing::concepts::FlushDecision — ENTKOPPELT (container_anatomy bleibt
/// leichtgewichtig, kein q2-topic-Include). Numerische Werte IDENTISCH (ABI-Konvention): NoFlush=0/Partial=1/Full=2.
/// ContainerAnatomy vergleicht den should_flush-Rückgabewert numerisch (static_cast<uint8_t>) → enum-agnostisch,
/// funktioniert für die echte FlushPolicy (axis_q2) UND den internen Default. Konvention-Guard: Test prüft ==2.
enum class ContainerFlushDecision : std::uint8_t { NoFlush = 0, PartialFlush = 1, FullFlush = 2 };

/// Default-Q2 (kein Auto-Flush): erfüllt die FlushPolicy-Duck-API (should_flush + on_flush_complete) ohne
/// q2-topic-Abhängigkeit. So bleibt ContainerComposition<Q1> rückwärtskompatibel (organ_count==2, aber nie Flush).
struct ContainerNoFlushPolicy {
    [[nodiscard]] ContainerFlushDecision should_flush(std::size_t /*fill*/, std::size_t /*cap*/) const noexcept {
        return ContainerFlushDecision::NoFlush;
    }
    void on_flush_complete() noexcept {}
};

/// Container-Observer (queuing-Gattung) — EIGENER flacher Observer-POD, getrennt vom SearchAlgorithm-
/// ObserverAggregate<17> (gattungs-korrekt). Nur uint64 → standard_layout. D4: + Flush-Felder (Q2).
struct ContainerObserverSnapshot {
    std::uint64_t put_count                 = 0;
    std::uint64_t get_count                 = 0;
    std::uint64_t current_occupancy         = 0;
    std::uint64_t peak_occupancy            = 0;
    // ── Q2 flush_policy (D4) ──
    std::uint64_t flush_decisions_evaluated = 0;
    std::uint64_t full_flush_count          = 0;
    std::uint64_t no_flush_count            = 0;
    std::uint64_t flush_complete_count      = 0;
};

/// ContainerComposition<Q1Buffer, Q2Flush> — die Container-Gattungs-Komposition: Q1 (Buffer) + Q2 (Flush-Policy).
/// Q2Flush defaultet auf ContainerNoFlushPolicy → bestehende ContainerComposition<Q1> bleibt gültig (kein Flush).
template <class Q1Buffer, class Q2Flush = ContainerNoFlushPolicy>
struct ContainerComposition {
    using buffer_strategy = Q1Buffer;
    using flush_policy    = Q2Flush;
    static constexpr std::string_view name     = "ContainerComposition";
    static constexpr std::string_view paper_id = "P00 Container Gattung (Queue)";
};

/// ContainerAnatomy<Composition> — die Container-GATTUNG (genus()==Adapter). Hält das echte Q1-Buffer-Organ
/// UND das Q2-Flush-Organ, treibt beide über die Gattungs-API (put/get/size/clear; put→should_flush) und liefert
/// observe_all → ContainerObserverSnapshot (inkl. Flush-Felder).
///
/// capacity_: Flush-Bezugsgröße für die Policy. Default 0 → should_flush(fill, 0) → NoFlush (kein Auto-Flush,
/// rückwärtskompatibel). capacity_ > 0 (ctor) → die Policy entscheidet (z.B. Watermark 75% → FullFlush → clear()).
template <class Composition>
class ContainerAnatomy {
public:
    using composition_t = Composition;
    using buffer_t      = typename Composition::buffer_strategy;
    using flush_t       = typename Composition::flush_policy;
    using element_type  = typename buffer_t::element_type;

    static constexpr std::string_view composition_name() noexcept { return Composition::name; }
    static constexpr std::string_view paper_id()         noexcept { return Composition::paper_id; }
    static constexpr AnatomyGenus     genus()            noexcept { return AnatomyGenus::Adapter; }  // Queue = Container-Adapter
    static constexpr std::size_t      organ_count()      noexcept { return 2; }                      // buffer_strategy + flush_policy

    ContainerAnatomy() = default;
    /// capacity > 0 aktiviert die Flush-Policy (Bezugsgröße fill/cap); 0 = nie auto-flushen.
    explicit ContainerAnatomy(std::size_t capacity) noexcept : capacity_{capacity} {}

    // ── Gattungs-API (Container/Queue) — treibt BEIDE Organe (Q1 buffer + Q2 flush) + aktualisiert den Observer ──
    void put(element_type v) {
        buffer_.put(v);
        ++obs_.put_count;
        obs_.current_occupancy = static_cast<std::uint64_t>(buffer_.size());
        if (obs_.current_occupancy > obs_.peak_occupancy) obs_.peak_occupancy = obs_.current_occupancy;
        // Q2: die Flush-Policy entscheidet (numerischer Vergleich → enum-agnostisch, s. ContainerFlushDecision).
        auto const raw = static_cast<std::uint8_t>(flush_.should_flush(buffer_.size(), capacity_));
        ++obs_.flush_decisions_evaluated;
        if (raw == static_cast<std::uint8_t>(ContainerFlushDecision::FullFlush)) {
            buffer_.clear();
            obs_.current_occupancy = 0;
            flush_.on_flush_complete();
            ++obs_.full_flush_count;
            ++obs_.flush_complete_count;
        } else {
            ++obs_.no_flush_count;   // PartialFlush wird vorerst wie NoFlush behandelt (Teil-Spülung = Folge-Feature)
        }
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
    flush_t                   flush_{};
    std::size_t               capacity_ = 0;   // 0 = kein Auto-Flush (rückwärtskompatibel)
    ContainerObserverSnapshot obs_{};
};

}  // namespace comdare::cache_engine::anatomy
