// Phase-6-Vorbau: Contract-Gates fuer IMeasurementSource (vendor-neutral), die compile-time
// Mess-Achsen-Registry und das E4'-Kurven-Fit-Skeleton (honest-empty-Doktrin).

#include <cache_engine/measurement/i_measurement_source.hpp>
#include <cache_engine/measurement/measurement_axis_registry.hpp>

#include "../../libs/cache_engine/builder/curve_fit/curve_fit.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

namespace mm = ::comdare::cache_engine::measurement;
namespace cb = ::comdare::cache_engine::builder;
namespace cf = ::comdare::cache_engine::builder::curve_fit;

namespace {

int g_fail = 0;

void check_true(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << "\n";
    if (!ok) ++g_fail;
}

template <class A, class B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what;
    if (!ok) std::cout << "  (abweichend)";
    std::cout << "\n";
    if (!ok) ++g_fail;
}

} // namespace

int main() {
    std::cout << "==== Phase-6-Vorbau: IMeasurementSource + Registry + Kurven-Skeleton ====\n";

    // ── (1) Registry: compile-time-Gates laufen als static_assert; hier die Laufzeit-Sicht. ──
    check_eq("Registry: 16 Achsen", mm::kMeasurementAxisRegistry.size(), std::size_t{16});
    std::size_t visited = 0, pmc_axes = 0;
    mm::for_each_measurement_axis([&](mm::MeasurementAxisInfo const& info) {
        ++visited;
        if (info.regime == mm::MeasurementRegime::PmcCounter) ++pmc_axes;
    });
    check_eq("Registry: for_each besucht 16", visited, std::size_t{16});
    check_true("Registry: beide Regimes vertreten (Zweiteilung M-Wurzel)", pmc_axes > 0 && pmc_axes < 16);
    check_true("Registry: axis_info(CLU).name == CLU",
               mm::axis_info(mm::MeasurementCategory::CLU).name == std::string_view{"CLU"});
    check_true("Registry: ENERGY_J ist PmcCounter-Regime (regime_of-Single-Source)",
               mm::axis_info(mm::MeasurementCategory::ENERGY_J).regime == mm::MeasurementRegime::PmcCounter);

    // ── (2) WallClockSource: garantierter degraded Fallback (Plan §2.3). ──
    mm::WallClockSource wall;
    mm::MeasuredEvent   want[] = {mm::MeasuredEvent::Cycles};
    check_true("WallClock: open(Cycles) == Available", wall.open(want) == mm::SourceStatus::Available);
    mm::MeasuredEvent unsupported[] = {mm::MeasuredEvent::L3Miss};
    check_true("WallClock: open(L3Miss) == EventUnsupported (Caps-Gate, wf_c99a2132)",
               wall.open(unsupported) == mm::SourceStatus::EventUnsupported);
    check_true("WallClock: vendor_id == wallclock", wall.vendor_id() == std::string_view{"wallclock"});
    wall.begin();
    wall.end();
    mm::MeasuredDelta delta;
    wall.read_delta(&delta);
    check_true("WallClock: Cycles-Proxy valid (Plan §2.2)", delta.is_valid(mm::MeasuredEvent::Cycles));
    check_true("WallClock: HW-Kanaele NICHT valid (ehrlich degraded)",
               !delta.is_valid(mm::MeasuredEvent::L1dMiss) && !delta.is_valid(mm::MeasuredEvent::EnergyUj));
    check_true("WallClock: needs_admin == false (immer verfuegbar)", !wall.capabilities().needs_admin);

    // ── (3) PmcSourceAdapter um NullPmcSource: ehrliche DriverMissing-Meldung. ──
    cb::NullPmcSource    null_pmc;
    mm::PmcSourceAdapter adapter{null_pmc};
    check_true("Adapter(NullPmc): open == DriverMissing", adapter.open(want) == mm::SourceStatus::DriverMissing);
    adapter.begin();
    adapter.end();
    mm::MeasuredDelta null_delta;
    null_delta.valid[0] = true; // muss von read_delta ueberschrieben werden
    adapter.read_delta(&null_delta);
    check_true("Adapter(NullPmc): kein Kanal valid (ehrlich, kein Schein-0)",
               !null_delta.is_valid(mm::MeasuredEvent::L1dMiss) && !null_delta.is_valid(mm::MeasuredEvent::Cycles));
    adapter.close();
    mm::MeasuredDelta after_close;
    adapter.read_delta(&after_close);
    check_true("Adapter: nach close kein stale Delta (wf_c99a2132)",
               after_close.of(mm::MeasuredEvent::L1dMiss) == 0 && !after_close.is_valid(mm::MeasuredEvent::L1dMiss));

    // ── (4) fit_log_linear: honest-empty + deterministische Mathematik. ──
    cf::MeasurementCurve empty;
    check_true("Fit: 0 Punkte => NoData", cf::fit_log_linear(empty).status == cf::FitStatus::NoData);
    cf::MeasurementCurve one;
    one.points.push_back({1024, 1.0, 1});
    check_true("Fit: 1 Punkt => InsufficientPoints",
               cf::fit_log_linear(one).status == cf::FitStatus::InsufficientPoints);
    cf::MeasurementCurve zero_x;
    zero_x.points.push_back({0, 5.0, 1});
    zero_x.points.push_back({1024, 10.0, 1});
    check_true("Fit: x==0 => InvalidData, NIE Ok/NaN (wf_c99a2132 critical)",
               cf::fit_log_linear(zero_x).status == cf::FitStatus::InvalidData);
    cf::MeasurementCurve same_x;
    for (int i = 0; i < 7; ++i) same_x.points.push_back({1000, static_cast<double>(i), 1});
    check_true("Fit: 7x identisches x => InsufficientPoints (Varianz-Guard statt det==0)",
               cf::fit_log_linear(same_x).status == cf::FitStatus::InsufficientPoints);
    // Synthetische log-Gerade y = 2*log2(x) + 3 ueber 4 Working-Set-Groessen.
    cf::MeasurementCurve line;
    for (std::uint64_t x :
         {std::uint64_t{1} << 10, std::uint64_t{1} << 14, std::uint64_t{1} << 17, std::uint64_t{1} << 23})
        line.points.push_back({x, 2.0 * std::log2(static_cast<double>(x)) + 3.0, 1});
    cf::FitResult const fit = cf::fit_log_linear(line);
    check_true("Fit: 4-Punkte-log-Gerade => Ok", fit.status == cf::FitStatus::Ok);
    check_true("Fit: a == 2 (1e-9)", std::fabs(fit.a - 2.0) < 1e-9);
    check_true("Fit: b == 3 (1e-9)", std::fabs(fit.b - 3.0) < 1e-9);
    check_true("Fit: residual ~ 0", fit.residual_rms < 1e-9);

    // ── (5) parse_wide_csv_column: REALER Bestands-Dialekt (Semikolon, lazy_csv_header-Namen),
    //        CRLF, Quotes, nicht-numerische Zeilen — alles wf_c99a2132-Befunde. ──
    std::string const csv_text = "binary_id;working_set_n;op_search_lookup_p99_ns\r\n"
                                 "id_a;1024;11.5\r\n"
                                 "\"id,mit,kommas\";4096;13.5\r\n"
                                 "id_c;n/a;n/a\r\n";
    {
        std::istringstream         csv{csv_text};
        cf::MeasurementCurve const c = cf::parse_wide_csv_column(csv, "working_set_n", "op_search_lookup_p99_ns",
                                                                 mm::MeasurementCategory::LATENCY_P99);
        check_eq("CSV: 2 gueltige Punkte (Semikolon+CRLF+Quotes)", c.points.size(), std::size_t{2});
        check_true("CSV: x/y des 2. Punkts korrekt (gequotete id verschiebt nichts)",
                   c.points.size() == 2 && c.points[1].x_working_set_bytes == 4096 &&
                       std::fabs(c.points[1].y_value - 13.5) < 1e-12);
        check_eq("CSV: n/a-Zeile gezaehlt uebersprungen (kein Phantom-Punkt)", c.skipped_rows, std::uint64_t{1});
    }
    {
        std::istringstream         csv{csv_text};
        cf::MeasurementCurve const missing =
            cf::parse_wide_csv_column(csv, "working_set_n", "SPALTE_GIBT_ES_NICHT", mm::MeasurementCategory::CLU);
        check_true("CSV: fehlende Spalte => ehrlich leere Kurve", missing.points.empty());
        check_true("CSV: leerer Fit => NoData", cf::fit_log_linear(missing).status == cf::FitStatus::NoData);
    }

    std::cout << "\n==== Phase-6-Vorbau: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER"))
              << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
