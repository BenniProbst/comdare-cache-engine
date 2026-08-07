#pragma once
// Querschnitt M -- SystemAxis-Wurzel fuer host-seitige Messachsen.
//
// Keine ABI-Erweiterung: diese Header-only-Wurzel dockt nur lesend an die bestehenden
// Host-PODs (ComdareTierObserverSnapshot, PmcCounters) an.

#include <cache_engine/measurement/axis_error.hpp>
#include <cache_engine/measurement/measurement_category.hpp>

#include "../../../anatomy/observable_tier.hpp"
#include <cache_engine/measurement/pmc_source.hpp>
#include "../../../topics/axis.hpp"

#include <array>
#include <concepts>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace comdare::cache_engine::measurement {

enum class MeasurementRegime : std::uint8_t {
    TimeObserver,
    PmcCounter,
};

inline constexpr std::array<MeasurementCategory, 16> kAllMeasurementCategories{
    MeasurementCategory::CLU,           MeasurementCategory::CACHE_MISS_L1,
    MeasurementCategory::CACHE_MISS_L2, MeasurementCategory::CACHE_MISS_L3,
    MeasurementCategory::DTLB_MISS,     MeasurementCategory::MEMORY_FOOTPRINT,
    MeasurementCategory::BRANCH_MISS,   MeasurementCategory::IPC_CPI,
    MeasurementCategory::LATENCY_MEAN,  MeasurementCategory::LATENCY_P50,
    MeasurementCategory::LATENCY_P95,   MeasurementCategory::LATENCY_P99,
    MeasurementCategory::LATENCY_P999,  MeasurementCategory::THROUGHPUT,
    MeasurementCategory::ENERGY_J,      MeasurementCategory::FILL_BUFFER_OCCUPANCY,
};

inline constexpr std::array<MeasurementCategory, 7> kPmcCounterCategories{
    MeasurementCategory::CACHE_MISS_L1, MeasurementCategory::CACHE_MISS_L2, MeasurementCategory::CACHE_MISS_L3,
    MeasurementCategory::DTLB_MISS,     MeasurementCategory::BRANCH_MISS,   MeasurementCategory::IPC_CPI,
    MeasurementCategory::ENERGY_J,
};

inline constexpr std::array<MeasurementCategory, 9> kTimeObserverCategories{
    MeasurementCategory::CLU,
    MeasurementCategory::MEMORY_FOOTPRINT,
    MeasurementCategory::LATENCY_MEAN,
    MeasurementCategory::LATENCY_P50,
    MeasurementCategory::LATENCY_P95,
    MeasurementCategory::LATENCY_P99,
    MeasurementCategory::LATENCY_P999,
    MeasurementCategory::THROUGHPUT,
    MeasurementCategory::FILL_BUFFER_OCCUPANCY,
};

[[nodiscard]] constexpr MeasurementRegime regime_of(MeasurementCategory category) noexcept {
    switch (category) {
        // Thesis 03_messsystem_prtart.tex:382-386: zeit-/observer-basierte Kategorien laufen mit eigener
        // Apparatur immer; 06_evaluation_methodology.tex:119-126: nicht vom privilegierten PMC-Regime gated.
        case MeasurementCategory::CLU:
        case MeasurementCategory::MEMORY_FOOTPRINT:
        case MeasurementCategory::LATENCY_MEAN:
        case MeasurementCategory::LATENCY_P50:
        case MeasurementCategory::LATENCY_P95:
        case MeasurementCategory::LATENCY_P99:
        case MeasurementCategory::LATENCY_P999:
        case MeasurementCategory::THROUGHPUT:
        case MeasurementCategory::FILL_BUFFER_OCCUPANCY: return MeasurementRegime::TimeObserver;

        // Thesis 03_messsystem_prtart.tex:382-386: PMC-/zaehlerbasierte Kategorien sind getrennt;
        // 06_evaluation_methodology.tex:119-126 nennt Cache-, dTLB-, Branch-Misses und IPC/CPI.
        // ENERGY_J ist code-verankert ueber PmcCounters::energy_micro_joules.
        case MeasurementCategory::CACHE_MISS_L1:
        case MeasurementCategory::CACHE_MISS_L2:
        case MeasurementCategory::CACHE_MISS_L3:
        case MeasurementCategory::DTLB_MISS:
        case MeasurementCategory::BRANCH_MISS:
        case MeasurementCategory::IPC_CPI:
        case MeasurementCategory::ENERGY_J: return MeasurementRegime::PmcCounter;
    }
    return MeasurementRegime::TimeObserver;
}

namespace detail {

[[nodiscard]] constexpr bool contains(std::array<MeasurementCategory, 7> const& values,
                                      MeasurementCategory                       category) noexcept {
    for (auto const value : values) {
        if (value == category) return true;
    }
    return false;
}

[[nodiscard]] constexpr bool contains(std::array<MeasurementCategory, 9> const& values,
                                      MeasurementCategory                       category) noexcept {
    for (auto const value : values) {
        if (value == category) return true;
    }
    return false;
}

[[nodiscard]] constexpr bool regime_mapping_is_complete() noexcept {
    std::size_t pmc_count           = 0;
    std::size_t time_observer_count = 0;
    for (auto const category : kAllMeasurementCategories) {
        MeasurementRegime const regime = regime_of(category);
        if (regime == MeasurementRegime::PmcCounter) {
            if (!contains(kPmcCounterCategories, category)) return false;
            ++pmc_count;
        } else {
            if (!contains(kTimeObserverCategories, category)) return false;
            ++time_observer_count;
        }
    }
    return pmc_count == kPmcCounterCategories.size() && time_observer_count == kTimeObserverCategories.size() &&
           (pmc_count + time_observer_count) == kAllMeasurementCategories.size();
}

template <class Derived>
[[nodiscard]] consteval MeasurementRegime regime_for_axis() {
    constexpr auto categories = Derived::do_categories();
    static_assert(categories.size() > 0, "SystemAxis braucht mindestens eine MeasurementCategory");
    MeasurementRegime const regime = regime_of(categories[0]);
    for (auto const category : categories) {
        if (regime_of(category) != regime) throw "SystemAxis darf keine MeasurementRegimes mischen";
    }
    return regime;
}

} // namespace detail

static_assert(kAllMeasurementCategories.size() == 16);
static_assert(detail::regime_mapping_is_complete(),
              "Jede MeasurementCategory muss genau einem MeasurementRegime zugeordnet sein");

/// FK-2: der Zell-Traeger der host-seitigen System-Achsen. Das fruehere `bool valid` konnte nur
/// "Zahl" von "keine Zahl" unterscheiden und hat damit drei grundverschiedene Aussagen in einen
/// Wert gefaltet: "diese Kategorie ist hier sinnlos" (n/a, kein Fehler), "die Mess-Quelle war nicht
/// da" (n/a) und "die Messung ist gescheitert" (failed). SampleStatus traegt genau diese Trennung
/// (axis_error.hpp, D2) und ist mit 1 Byte groessen-neutral zum abgeloesten bool (Layout-Pin unten).
struct SystemAxisSample {
    MeasurementCategory category = MeasurementCategory::CLU;
    std::uint64_t       value    = 0;
    // FK-2/K1 -- FAIL-SAFE-Default: eine default-konstruierte, nie collectete Sample ist NIE gueltig.
    // Der Default ist BEWUSST nicht Ok; die Ok==0-Zusicherung in axis_error.hpp gilt ausschliesslich
    // fuer den PermResult-Wire-POD. Waere er hier Ok, laese ein vergessener collect()-Aufruf als
    // "gueltige Messung mit dem Wert 0" -- exakt die Blindstelle "Messung nie als Nullen".
    // SourceUnavailable ist die ehrliche Aussage VOR jeder Erhebung: die Quelle wurde nie gefragt.
    SampleStatus status = SampleStatus::SourceUnavailable;

    /// Gueltig heisst AUSSCHLIESSLICH Ok. n/a, fehlende Quelle und failed sind alle drei nicht
    /// gueltig -- aber untereinander unterscheidbar (genau das konnte das bool nicht).
    [[nodiscard]] constexpr bool valid() const noexcept { return status == SampleStatus::Ok; }

    /// Erfolgspfad, EXPLIZIT: Wert und Status wandern nur gemeinsam. Kein "value gesetzt, Status vergessen".
    constexpr void mark_ok(std::uint64_t measured) noexcept {
        value  = measured;
        status = SampleStatus::Ok;
    }
    /// Die Kategorie ist fuer diese Achse/Binary sinnlos -> ehrliches "n/a", KEIN Fehler.
    constexpr void mark_not_applicable() noexcept {
        value  = 0;
        status = SampleStatus::NotApplicable;
    }
    /// Die Mess-QUELLE (PMC-Zugang, Snapshot, Schema-Spalte) ist nicht da oder nicht befragbar -> "n/a".
    constexpr void mark_source_unavailable() noexcept {
        value  = 0;
        status = SampleStatus::SourceUnavailable;
    }
    /// Echter Mess-Fehler (unplausibler Rohwert, Gate-Fail) -> Zelle "failed" + Log, NIE eine Null.
    constexpr void mark_failed() noexcept {
        value  = 0;
        status = SampleStatus::Failed;
    }
};

static_assert(std::is_standard_layout_v<SystemAxisSample>);
static_assert(std::is_trivially_copyable_v<SystemAxisSample>);
// FK-2-LAYOUT-PIN (VOR der Status-Hebung gemessen und festgeschrieben, Auflage aus dem Design-Dossier):
// die anstehende Hebung `bool valid` -> `SampleStatus status` (beide 1 Byte) muss GROESSEN-NEUTRAL sein.
// Der Pin steht bewusst hier und nicht erst nach dem Umbau: nur so beweist er, dass sich nichts bewegt
// hat. Bricht er, ist ein stiller Layout-Drift im host-seitigen Sample passiert -- genau der Fall, den
// die Risiko-Liste des Pakets benennt.
static_assert(sizeof(SystemAxisSample) == 24,
              "FK-2: SystemAxisSample-Groesse ist gepinnt (die Status-Hebung ist groessen-neutral)");
static_assert(alignof(SystemAxisSample) == 8, "FK-2: SystemAxisSample-Ausrichtung ist gepinnt");

/// System-Achsen sind bei eingeschalteter Messung immer host-seitig praesent (Blut-Direktive), unabhaengig von
/// Tier-Permutation und E2/E3-Baum. Sie sind keine Organ-Taxonomie und kein austauschbarer Achsen-Slot.
[[nodiscard]] constexpr bool system_axes_always_present() noexcept { return true; }

template <class Derived>
struct SystemAxis : topics::Axis<Derived> {
    [[nodiscard]] static constexpr topics::AxisKind axis_kind() noexcept {
        return topics::AxisKind::system_measurement;
    }
    [[nodiscard]] static constexpr auto              categories() noexcept { return Derived::do_categories(); }
    [[nodiscard]] static constexpr MeasurementRegime regime() noexcept { return detail::regime_for_axis<Derived>(); }

    [[nodiscard]] constexpr bool available() const noexcept {
        if constexpr (requires(Derived const& derived) {
                          { derived.do_available() } -> std::convertible_to<bool>;
                      }) {
            return static_cast<bool>(derived().do_available());
        } else {
            return true;
        }
    }

    constexpr void collect(SystemAxisSample& sample) const noexcept {
        if (!available()) {
            // Die Achse meldet ihre eigene Quelle als nicht verfuegbar (z.B. PmcCounters ohne Zugang):
            // ehrliches "n/a", kein Mess-Fehler und keine stille Null.
            sample.mark_source_unavailable();
            return;
        }
        derived().do_collect(sample);
    }

protected:
    constexpr SystemAxis() noexcept = default;

    /// DEPRECATED (FK-2): konflatierte "nicht anwendbar", "Quelle nicht da" und "Messung gescheitert"
    /// zu einem einzigen "nicht gueltig". Bleibt ERHALTEN (Kanon: deprecaten statt loeschen) und
    /// delegiert auf die konservativste der drei Aussagen. Neuer Code benutzt die benannten Helfer
    /// SystemAxisSample::mark_not_applicable/mark_source_unavailable/mark_failed direkt.
    [[deprecated("FK-2: konflatiert n/a vs. fehlende Quelle vs. Mess-Fehler -- "
                 "SystemAxisSample::mark_not_applicable()/mark_source_unavailable()/mark_failed() benutzen")]]
    static constexpr void invalidate(SystemAxisSample& sample) noexcept {
        sample.mark_source_unavailable();
    }

private:
    [[nodiscard]] constexpr Derived const& derived() const noexcept { return static_cast<Derived const&>(*this); }
};

template <class A>
concept SystemAxisConcept =
    topics::AxisConcept<A> && std::derived_from<A, SystemAxis<A>> && std::is_empty_v<SystemAxis<A>> &&
    (!std::is_polymorphic_v<SystemAxis<A>>) && requires(A const& axis, SystemAxisSample& sample) {
        { A::categories() };
        { A::regime() } -> std::same_as<MeasurementRegime>;
        { axis.available() } -> std::same_as<bool>;
        { axis.collect(sample) } -> std::same_as<void>;
    };

struct WallClockSystemAxis final : SystemAxis<WallClockSystemAxis> {
    std::int64_t  total_ns = 0;
    std::uint64_t op_count = 0;

    constexpr WallClockSystemAxis(std::int64_t total_ns_, std::uint64_t op_count_) noexcept
        : total_ns(total_ns_), op_count(op_count_) {}

    [[nodiscard]] static constexpr auto do_categories() noexcept {
        return std::array{MeasurementCategory::LATENCY_MEAN, MeasurementCategory::THROUGHPUT};
    }

    constexpr void do_collect(SystemAxisSample& sample) const noexcept {
        switch (sample.category) {
            case MeasurementCategory::LATENCY_MEAN:
                // FK-2: die alte Sammel-Bedingung (op_count==0 || total_ns<0) hat zwei verschiedene
                // Zustaende zu einem "invalid" gefaltet -- hier einzeln eingeordnet.
                if (total_ns < 0) {
                    // Eine NEGATIVE Dauer ist kein "nicht anwendbar", sondern ein unplausibler Rohwert
                    // der Zeitquelle: echter Mess-Fehler, Zelle "failed", nie eine Null.
                    sample.mark_failed();
                    return;
                }
                if (op_count == 0) {
                    // Kein einziger Operations-Zaehler: die Quelle hat nichts geliefert, ein Mittelwert
                    // ist nicht bildbar. Ehrliches "n/a", kein Fehler des Algorithmus.
                    sample.mark_source_unavailable();
                    return;
                }
                sample.mark_ok(static_cast<std::uint64_t>(total_ns) / op_count);
                return;

            case MeasurementCategory::THROUGHPUT:
                if (total_ns < 0) {
                    sample.mark_failed(); // s.o.: negative Dauer = unplausibler Rohwert, kein n/a
                    return;
                }
                if (total_ns == 0) {
                    sample.mark_source_unavailable(); // keine gemessene Zeitspanne -> keine Rate bildbar
                    return;
                }
                sample.mark_ok(static_cast<std::uint64_t>((static_cast<long double>(op_count) * 1'000'000'000.0L) /
                                                          static_cast<long double>(total_ns)));
                return;

            case MeasurementCategory::LATENCY_P50:
            case MeasurementCategory::LATENCY_P95:
            case MeasurementCategory::LATENCY_P99:
            case MeasurementCategory::LATENCY_P999:
                // honest-0, FK-2-Einordnung NotApplicable: total_ns/op_count ist ein MITTELWERT -- ihn als
                // Perzentil zu etikettieren waere ein Phantomwert. Es fehlt keine Quelle (die Zeitwerte
                // liegen vor), die KATEGORIE ist fuer diese Achse schlicht sinnlos. Echte Perzentile
                // liefert das HdrHistogramm (AP-8/#242).
                sample.mark_not_applicable();
                return;

            // Kategorie gehoert nicht zum Vertrag dieser Achse (do_categories) -> n/a, kein Fehler.
            default: sample.mark_not_applicable(); return;
        }
    }
};

struct ObserverSnapshotSystemAxis final : SystemAxis<ObserverSnapshotSystemAxis> {
    using snapshot_t = ::comdare::cache_engine::anatomy::ComdareTierObserverSnapshot;

    snapshot_t const* snapshot = nullptr;

    explicit constexpr ObserverSnapshotSystemAxis(snapshot_t const& snapshot_) noexcept : snapshot(&snapshot_) {}

    [[nodiscard]] static constexpr auto do_categories() noexcept { return std::array{MeasurementCategory::CLU}; }

    constexpr void do_collect(SystemAxisSample& sample) const noexcept {
        if (snapshot == nullptr) {
            // Kein Snapshot angeheftet -> die Mess-Quelle selbst fehlt (nicht: Kategorie sinnlos).
            sample.mark_source_unavailable();
            return;
        }

        switch (sample.category) {
            case MeasurementCategory::CLU: {
                // Cache-Line-AUSLASTUNG (Thesis 03:383) = field_bytes / (cache_lines * line_bytes), als Prozent.
                // Der rohe cache_lines-Zaehler waere die INVERSE Metrik (Review wf_f1604ba3, CONFIRMED-major).
                //
                // B14-NB4 (2026-08-06) -- HIER STAND DAS LITERAL 64, und es war der Landeblocker von B14.
                // B14-NB3 hat die EINHEIT von cache_lines geaendert: der Zaehler laeuft seitdem in Linien
                // DER cacheline-Unterachse (32/64/128/256), nicht mehr in 64-B-Linien. Der Nenner blieb
                // stehen -- mit dem Ergebnis, dass dieselbe, physisch UNVERAENDERTE Messung je Line-Groesse
                // eine andere Auslastung gemeldet haette. Nachgerechnet (Codex und Opus unabhaengig, mit
                // n=1024, record_size=48, field_bytes=8192, wahre beruehrte Bytes konstant 49152):
                // B32 -> 8 %, B64 -> 16 %, B128 -> 33 %, B256 -> 66 %, wo durchgaengig ~16 % richtig ist.
                // Eine Messgroesse zu korrigieren, ohne ihren Verbraucher mitzuziehen, ist schlimmer als
                // der Ausgangszustand: der Fehler wandert von der Einheit in den WERT.
                //
                // WARUM DER NENNER JETZT AUS DEM SNAPSHOT KOMMT und nicht "ueber dieselbe Achse" gezogen
                // wird: diese Achse liest einen POD ueber die Modul-ABI-Grenze (eine geladene Tier-.so).
                // Die Line-Groesse ist eine COMPILE-ZEIT-Eigenschaft der DLL; im Host existiert der
                // Layout-Typ nicht einmal. Ein Achsen-Zugriff hier waere entweder ein zweites Literal oder
                // eine Annahme ueber fremden Code. Der Produzent legt sie deshalb NEBEN den Zaehler
                // (axis_stats[5][5], reservierter Slot, sizeof unveraendert).
                std::uint64_t const field_bytes = snapshot->axis_stats[5][2];
                std::uint64_t const cache_lines = snapshot->axis_stats[5][3];
                std::uint64_t const line_bytes  = snapshot->axis_stats[5][5];
                if (cache_lines == 0 || line_bytes == 0) {
                    // Der Snapshot liegt vor, traegt aber keinen brauchbaren Nenner: entweder hat die
                    // QUELLE nichts geliefert (cache_lines == 0) oder sie nennt die Einheit ihres Zaehlers
                    // nicht eindeutig (line_bytes == 0; der Observer vergiftet den Wert, wenn zwei
                    // Produzenten mit verschiedenen Linien in denselben Zaehler geschrieben haben).
                    // Kein Fehler des Algorithmus, aber auch keine bildbare Auslastung -> n/a.
                    // FAIL-CLOSED und mit Absicht: eine Prozentzahl aus zwei Einheiten waere ein Phantom.
                    sample.mark_source_unavailable();
                    return;
                }
                sample.mark_ok((field_bytes * 100u) / (cache_lines * line_bytes));
                return;
            }
            case MeasurementCategory::MEMORY_FOOTPRINT:
                // honest-0, FK-2/K6-Einordnung SourceUnavailable: der Thesis-Kanon verlangt bytes_in_use_peak
                // (05_evaluation.tex:94-95); das T6-Schema traegt nur den Momentanwert bytes_in_use -- als
                // Footprint etikettiert verzerrte er CoW-lastige Layouts (Peak >> Endstand). Die Kategorie ist
                // fuer diese Achse SINNVOLL, es fehlt die QUELLE (die peak-Spalte). Sobald ein golden-neutraler
                // END-Append die Spalte liefert, wird hier gemessen -- deshalb NICHT NotApplicable.
                sample.mark_source_unavailable();
                return;
            case MeasurementCategory::FILL_BUFFER_OCCUPANCY:
                // honest-0, FK-2/K6-Einordnung NotApplicable: T17 queuing_q1.peak_size ist ein SOFTWARE-
                // Operations-Puffer; die Thesis-Kategorie meint den Hardware-Line-Fill-Buffer (P25 Mahling,
                // 03_state_of_the_art:543) -- physisch unverwandt, kein Proxy. Hier fehlt keine Spalte, die
                // Kategorie ist ueber diese Achse grundsaetzlich nicht erhebbar.
                sample.mark_not_applicable();
                return;
            // Kategorie gehoert nicht zum Vertrag dieser Achse (do_categories) -> n/a, kein Fehler.
            default: sample.mark_not_applicable(); return;
        }
    }
};

// Compile-time-Bindung an den Schema-Vertrag (kV3AxisSchema IST der Vertrag — keine stille Index-Drift).
static_assert(std::string_view{::comdare::cache_engine::anatomy::kV3AxisSchema[5].names[2]} == "field_bytes");
static_assert(std::string_view{::comdare::cache_engine::anatomy::kV3AxisSchema[5].names[3]} == "cache_lines");
// B14-NB4: der Nenner-Slot. Er ist Teil DESSELBEN Vertrags -- faellt er weg oder wandert er, muss der
// Build brechen und nicht die CLU stillschweigend wieder auf eine Fremd-Einheit rechnen.
static_assert(std::string_view{::comdare::cache_engine::anatomy::kV3AxisSchema[5].names[5]} == "line_bytes");

struct PmcSystemAxis final : SystemAxis<PmcSystemAxis> {
    using counters_t = ::comdare::cache_engine::measurement::PmcCounters;

    counters_t const* counters = nullptr;

    explicit constexpr PmcSystemAxis(counters_t const& counters_) noexcept : counters(&counters_) {}

    [[nodiscard]] static constexpr auto do_categories() noexcept { return kPmcCounterCategories; }

    [[nodiscard]] constexpr bool do_available() const noexcept { return counters != nullptr && counters->available; }

    constexpr void do_collect(SystemAxisSample& sample) const noexcept {
        if (!do_available()) {
            // FK-2/K6: PMC nicht angeheftet ODER unprivilegiert (counters->available==false) -> der ZUGANG
            // zur Mess-Quelle fehlt. Das ist kein Urteil ueber die Kategorie und kein Mess-Fehler -> n/a.
            sample.mark_source_unavailable();
            return;
        }

        // M-3a (2026-08-07) -- FEINKOERNIG STATT ZEILEN-WEIT. Bis hierher stempelte JEDE dieser Kategorien
        // mark_ok(), sobald counters->available galt. Das ist die falsche Frage: `available` sagt nur
        // "mindestens ein Zaehler hat geliefert", nicht "DIESER Zaehler". Auf AMD Zen5 ist available==true
        // (L1D oeffnete), waehrend cache_misses_l3 mangels generischem LL-Event beim POD-Default 0 bleibt --
        // mark_ok(0) haette diese 0 als gueltigen Messwert testiert. Die vier flag-tragenden Kategorien
        // fragen deshalb ihre EIGENE Quelle; ohne sie ist es derselbe FK-2/K6-Fall wie oben (der ZUGANG zur
        // Quelle fehlt, kein Urteil ueber die Kategorie) -> SourceUnavailable, nicht 0.
        // l1/dtlb bleiben zeilen-gebunden: fuer sie fuehrt der Bestands-POD kein eigenes Flag (identisch zur
        // WIDE-CSV, wo sie ueber `zelle` statt `pmc_zelle` gehen) -- hier wird kein Flag erfunden.
        auto flag_gated = [&sample](std::uint64_t value, bool source_available) noexcept {
            if (source_available)
                sample.mark_ok(value);
            else
                sample.mark_source_unavailable();
        };
        switch (sample.category) {
            case MeasurementCategory::CACHE_MISS_L1: sample.mark_ok(counters->cache_misses_l1); return;
            case MeasurementCategory::CACHE_MISS_L2:
                flag_gated(counters->cache_misses_l2, counters->cache_misses_l2_source_available);
                return;
            case MeasurementCategory::CACHE_MISS_L3:
                flag_gated(counters->cache_misses_l3, counters->cache_misses_l3_source_available);
                return;
            case MeasurementCategory::DTLB_MISS: sample.mark_ok(counters->dtlb_misses); return;
            case MeasurementCategory::BRANCH_MISS:
                flag_gated(counters->branch_misses, counters->branch_misses_source_available);
                return;
            case MeasurementCategory::ENERGY_J:
                flag_gated(counters->energy_micro_joules, counters->energy_micro_joules_source_available);
                return;
            case MeasurementCategory::IPC_CPI:
                // honest-0, FK-2/K6-Einordnung SourceUnavailable: das aktuelle PmcCounters-POD enthaelt keine
                // instructions/cycles-Spalten. IPC/CPI ist eine ECHTE PMC-Kategorie dieser Achse (sie steht in
                // kPmcCounterCategories) -- es fehlen die Zaehler-QUELLEN, nicht der Sinn. Kein Phantomwert.
                sample.mark_source_unavailable();
                return;
            // Kategorie gehoert nicht zum Vertrag dieser Achse (kPmcCounterCategories) -> n/a, kein Fehler.
            default: sample.mark_not_applicable(); return;
        }
    }
};

static_assert(SystemAxisConcept<WallClockSystemAxis>);
static_assert(SystemAxisConcept<ObserverSnapshotSystemAxis>);
static_assert(SystemAxisConcept<PmcSystemAxis>);

} // namespace comdare::cache_engine::measurement
