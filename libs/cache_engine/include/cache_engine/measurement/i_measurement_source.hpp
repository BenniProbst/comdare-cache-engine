#pragma once
// Phase-6-Vorbau (2026-07-10): vendor-neutrale Mess-Quellen-Abstraktion nach der Design-Quelle
// docs/sessions/20260531-mess-abstraktion-cross-platform-architektur-plan.md §2.2.
// Bewusst HOST-seitig (nicht ueber die DLL-Grenze): PMC-Zugriff braucht OS-Privilegien/Treiber,
// die ein Lebewesen-Modul nicht haben soll; die echte Erhebung umklammert host-seitig den
// run_workload-/tier_*-Aufruf am Dock. Runner-/Pruef-Dock-Verdrahtung = Folge-Increment (E1-Verweis).
//
// Review wf_c99a2132 (CONFIRMED-major, gefixt): read_delta zielt auf ein EIGENES, vollstaendiges
// MeasuredDelta (alle 10 MeasuredEvents + per-Event-Gueltigkeit) — measurement::PmcCounters (TABU,
// Bestand) kann Cycles/Instructions/MemStall strukturell nicht ausdruecken und waere fuer die
// Folge-Verdrahtung eine Sackgasse gewesen. Der ABI-heilige Snapshot-POD bleibt unberuehrt.

#include <cache_engine/measurement/pmc_source.hpp> // measurement::IPmcSource/PmcCounters (Bestands-Familie, nur Adapter)

#include <array>
#include <chrono>
#include <cstdint>
#include <span>
#include <string_view>

namespace comdare::cache_engine::measurement {

/// Vendor-neutrale Ereignis-SEMANTIK (nicht Event-Code) — Plan §2.2.
enum class MeasuredEvent : std::uint32_t {
    Cycles         = 0,
    Instructions   = 1,
    L1dMiss        = 2,
    L2Miss         = 3,
    L3Miss         = 4,
    DtlbMiss       = 5,
    BranchMiss     = 6,
    MemStall       = 7,
    CoherenceInval = 8,
    EnergyUj       = 9,
};
inline constexpr std::size_t kMeasuredEventCount = 10;

/// errno-style Verfuegbarkeits-Status (wie PruefDock) — Plan §2.2.
enum class SourceStatus : int {
    Available         = 0,
    NotImplemented    = 1,
    DriverMissing     = 2,
    PermissionDenied  = 3,
    EventUnsupported  = 4,
    MultiplexRequired = 5,
};

/// Was diese Quelle KANN (Gate-Info fuer den Mess-Planer) — Plan §2.2.
/// event_supported = STRUKTURELL moegliche Events; die per-MESSUNG-Wahrheit liefert
/// MeasuredDelta::valid (Review wf_c99a2132: Caps duerfen keine Mess-Garantie behaupten).
struct MeasurementSourceCaps {
    std::uint32_t                         max_concurrent_events = 0;     ///< z.B. Intel Skylake GP: 4 -> Multiplex-Flag
    bool                                  has_energy            = false; ///< RAPL/uProf vorhanden?
    bool                                  needs_admin       = false; ///< ZIH-Gate: true -> auf Rechenknoten gesperrt
    bool                                  hybrid_core_aware = false; ///< P/E-Core separate Event-Codes
    std::array<bool, kMeasuredEventCount> event_supported{};         ///< pro MeasuredEvent (strukturell)
};

/// Vollstaendiges, vendor-neutrales Mess-Delta: Wert + Gueltigkeit JE Event (honest-Doktrin —
/// value 0 mit valid=true ist eine echte Null-Messung, valid=false heisst NICHT gemessen).
struct MeasuredDelta {
    std::array<std::uint64_t, kMeasuredEventCount> value{};
    std::array<bool, kMeasuredEventCount>          valid{};

    [[nodiscard]] constexpr std::uint64_t of(MeasuredEvent e) const noexcept {
        return value[static_cast<std::size_t>(e)];
    }
    [[nodiscard]] constexpr bool is_valid(MeasuredEvent e) const noexcept { return valid[static_cast<std::size_t>(e)]; }
};

/// Vendor-neutrale Mess-Quelle; die PMC-Familie (IPmcSource) ist EINE Implementierungs-Familie davon.
class IMeasurementSource {
public:
    virtual ~IMeasurementSource() = default;
    [[nodiscard]] virtual std::string_view
    vendor_id() const noexcept = 0; ///< "intel-pcm"/"amd-uprof"/"arm-papi"/"wallclock"
    [[nodiscard]] virtual MeasurementSourceCaps capabilities() const noexcept = 0;
    /// Event-Auswahl + Multiplex-Plan. Review wf_c99a2132: MUSS die Event-Liste gegen die Caps
    /// pruefen — nicht unterstuetzte Events => EventUnsupported (keine stillen Leer-Zusagen).
    virtual SourceStatus open(std::span<MeasuredEvent const> events) noexcept = 0;
    virtual void         begin() noexcept                                     = 0; ///< Counter-Snapshot vor Workload
    virtual void         end() noexcept                                       = 0; ///< Counter-Snapshot nach Workload
    virtual void         read_delta(MeasuredDelta* out) const noexcept        = 0; ///< (end-begin) je Event
    virtual void         close() noexcept                                     = 0;
};

namespace detail {
/// Gemeinsame open()-Validierung: jedes angefragte Event muss strukturell unterstuetzt sein.
[[nodiscard]] inline SourceStatus validate_events(MeasurementSourceCaps const&   caps,
                                                  std::span<MeasuredEvent const> events) noexcept {
    for (MeasuredEvent e : events)
        if (!caps.event_supported[static_cast<std::size_t>(e)]) return SourceStatus::EventUnsupported;
    return SourceStatus::Available;
}
} // namespace detail

/// Garantierter Fallback (Plan §2.3 letzte Zeile): immer verfuegbar; liefert den Plan-geforderten
/// Cycles-PROXY aus der Wall-Clock (Delta in Nanosekunden, valid nur fuer Cycles) — explizit
/// degraded source, keine HW-Misses (Review wf_c99a2132: vorher war es wieder der Platzhalter).
class WallClockSource final : public IMeasurementSource {
public:
    [[nodiscard]] std::string_view      vendor_id() const noexcept override { return "wallclock"; }
    [[nodiscard]] MeasurementSourceCaps capabilities() const noexcept override {
        MeasurementSourceCaps caps{};
        caps.max_concurrent_events                                            = 1;
        caps.event_supported[static_cast<std::size_t>(MeasuredEvent::Cycles)] = true; // ns-Proxy
        return caps;
    }
    SourceStatus open(std::span<MeasuredEvent const> events) noexcept override {
        return detail::validate_events(capabilities(), events);
    }
    void begin() noexcept override { begin_ = std::chrono::steady_clock::now(); }
    void end() noexcept override { end_ = std::chrono::steady_clock::now(); }
    void read_delta(MeasuredDelta* out) const noexcept override {
        if (out == nullptr) return;
        *out = MeasuredDelta{};
        if (end_ >= begin_) {
            auto const ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - begin_);
            out->value[static_cast<std::size_t>(MeasuredEvent::Cycles)] = static_cast<std::uint64_t>(ns.count());
            out->valid[static_cast<std::size_t>(MeasuredEvent::Cycles)] = true; // Proxy, dokumentiert
        }
    }
    void close() noexcept override { begin_ = end_ = {}; }

private:
    std::chrono::steady_clock::time_point begin_{};
    std::chrono::steady_clock::time_point end_{};
};

/// GoF-ADAPTER: wickelt die Bestands-PMC-Familie (measurement::IPmcSource) in das vendor-neutrale
/// Interface — pmc_source.hpp bleibt byte-unberuehrt. EHRLICHKEITS-GRENZE (Review wf_c99a2132):
/// IPmcSource kennt weder per-Event-Caps noch per-Event-Gueltigkeit (EIN available-Flag); der
/// Adapter bewirbt deshalb nur die 7 STRUKTURELL ausdrueckbaren Kanaele und stempelt valid je
/// Kanal grob-granular mit counters.available. Feinere Wahrheit braucht eine IPmcSource-API-
/// Erweiterung (P4/Vendor-Impls) = Folge-Increment.
class PmcSourceAdapter final : public IMeasurementSource {
public:
    explicit PmcSourceAdapter(measurement::IPmcSource& source) noexcept : source_{source} {}

    [[nodiscard]] std::string_view      vendor_id() const noexcept override { return source_.name(); }
    [[nodiscard]] MeasurementSourceCaps capabilities() const noexcept override {
        MeasurementSourceCaps caps{};
        if (source_.available()) {
            caps.max_concurrent_events = 1;    // konservativ; echte Caps liefert die Vendor-Impl (P4)
            caps.has_energy            = true; // strukturell (energy_micro_joules-Feld); Mess-Wahrheit via valid
            for (auto e :
                 {MeasuredEvent::L1dMiss, MeasuredEvent::L2Miss, MeasuredEvent::L3Miss, MeasuredEvent::DtlbMiss,
                  MeasuredEvent::BranchMiss, MeasuredEvent::CoherenceInval, MeasuredEvent::EnergyUj})
                caps.event_supported[static_cast<std::size_t>(e)] = true;
        }
        return caps;
    }
    SourceStatus open(std::span<MeasuredEvent const> events) noexcept override {
        last_ = measurement::PmcCounters{}; // stale-Delta-Schutz (Review wf_c99a2132)
        if (!source_.available()) return SourceStatus::DriverMissing;
        return detail::validate_events(capabilities(), events);
    }
    void begin() noexcept override {
        last_ = measurement::PmcCounters{};
        source_.begin();
    }
    void end() noexcept override { last_ = source_.end(); }
    void read_delta(MeasuredDelta* out) const noexcept override {
        if (out == nullptr) return;
        *out           = MeasuredDelta{};
        auto const ok  = last_.available; // grob-granular (s. Klassen-Kommentar)
        auto       set = [&](MeasuredEvent e, std::uint64_t v) {
            out->value[static_cast<std::size_t>(e)] = v;
            out->valid[static_cast<std::size_t>(e)] = ok;
        };
        set(MeasuredEvent::L1dMiss, last_.cache_misses_l1);
        set(MeasuredEvent::L2Miss, last_.cache_misses_l2);
        set(MeasuredEvent::L3Miss, last_.cache_misses_l3);
        set(MeasuredEvent::DtlbMiss, last_.dtlb_misses);
        set(MeasuredEvent::BranchMiss, last_.branch_misses);
        set(MeasuredEvent::CoherenceInval, last_.coherence_invalidations);
        set(MeasuredEvent::EnergyUj, last_.energy_micro_joules);
    }
    void close() noexcept override { last_ = measurement::PmcCounters{}; }

private:
    measurement::IPmcSource& source_;
    measurement::PmcCounters last_{}; ///< Delta des letzten begin/end-Intervalls
};

} // namespace comdare::cache_engine::measurement
