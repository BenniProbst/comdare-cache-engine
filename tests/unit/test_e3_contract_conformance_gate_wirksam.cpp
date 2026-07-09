// Schicht E3 Paket A: #223-Beweis, dass perm_runner.hpp:130-148 das Konformitaets-Gate vor die Messung schaltet.
//
// ORIGIN B3-2 vom 2026-06-28 war stale: Audit K9/V5-I4 hat den Pfad import -> GATE -> nur bei pass messen bereits
// geheilt. Dieser Test haelt den Produktionspfad fest: run_observable_perm() muss bei nicht-konformer
// IObservableTier-Huelle vor jeder Performance-Messung abbrechen und eine genullte Formatzeile liefern.

#include <anatomy/observable_tier.hpp>
#include <builder/experiment_tree/perm_runner.hpp>

#include <cstdint>
#include <iostream>
#include <map>
#include <string>

namespace ex  = ::comdare::cache_engine::builder::experiment;
namespace ana = ::comdare::cache_engine::anatomy;

namespace {

int g_fail = 0;

void check(char const* what, bool ok) {
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << '\n';
    if (!ok) ++g_fail;
}

template <typename A, typename B>
void check_eq(char const* what, A const& got, B const& want) {
    bool const ok = (got == want);
    std::cout << (ok ? "  [OK]  " : "  [ERR] ") << what << " = " << got;
    if (!ok) std::cout << "  (erwartet: " << want << ")";
    std::cout << '\n';
    if (!ok) ++g_fail;
}

class MapObservableTier : public ana::IObservableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t key, std::uint64_t value) noexcept override {
        auto const [it, inserted] = data_.insert_or_assign(key, value);
        (void)it;
        ++inserts_;
        return inserted;
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        ++lookups_;
        auto const it = data_.find(key);
        if (it == data_.end()) return false;
        ++hits_;
        if (out_value != nullptr) *out_value = it->second;
        return true;
    }

    [[nodiscard]] bool tier_erase(std::uint64_t key) noexcept override { return data_.erase(key) != 0; }

    void tier_clear() noexcept override {
        data_.clear();
        reset_counters();
    }

    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return data_.size(); }

    void tier_observe(ana::ComdareTierObserverSnapshot* out) const noexcept override {
        if (out == nullptr) return;
        out->axis_stats[0][0]      = lookups_;
        out->axis_stats[0][1]      = hits_;
        out->axis_stats[0][3]      = inserts_;
        out->observable_axis_count = 1;
        out->tier_fill_level       = data_.size();
        out->filled_axis_count     = 1;
    }

    void tier_reset_statistics() noexcept override { reset_counters(); }

protected:
    void reset_counters() const noexcept {
        inserts_ = 0;
        lookups_ = 0;
        hits_    = 0;
    }

    std::map<std::uint64_t, std::uint64_t> data_;
    mutable std::uint64_t                  inserts_ = 0;
    mutable std::uint64_t                  lookups_ = 0;
    mutable std::uint64_t                  hits_    = 0;
};

// Absichtlich nicht std::map-konform: Hits liefern true, aber einen falschen Wert. Das muss das Gate vor der
// Messphase fangen; tier_observe schreibt einen Sentinel, falls der Short-Circuit versehentlich entfernt wird.
class WrongValueObservableTier final : public MapObservableTier {
public:
    [[nodiscard]] bool tier_lookup(std::uint64_t key, std::uint64_t* out_value) const noexcept override {
        auto const it = data_.find(key);
        if (it == data_.end()) return false;
        if (out_value != nullptr) *out_value = it->second + 1u;
        return true;
    }

    void tier_observe(ana::ComdareTierObserverSnapshot* out) const noexcept override {
        ++observe_calls;
        if (out == nullptr) return;
        out->axis_stats[0][0]      = 999999u;
        out->observable_axis_count = 99u;
        out->tier_fill_level       = 99u;
        out->filled_axis_count     = 99u;
    }

    mutable std::uint64_t observe_calls = 0;
};

void positive_conforming_tier_measures() {
    std::cout << "== Positiv: konforme IObservableTier-Huelle ==\n";
    MapObservableTier tier;
    std::string const bid = "search_algo=map_ok/node_type=mock";

    ex::PermResult const pr = ex::run_observable_perm(tier, bid, /*n_ops=*/64);

    check("Gate lief (cases_total > 0)", pr.conformance_cases_total > 0);
    check("Gate bestanden", pr.conformance_passed);
    check_eq("Gate: alle Faelle bestanden", pr.conformance_cases_passed, pr.conformance_cases_total);
    check_eq("Gate: first_fail == 0", pr.conformance_first_fail, std::uint64_t{0});
    check("Mess-Snapshot ist real", pr.unified_real);
    check("Formatzeile ist nicht leer", !pr.line.empty());
    check("Formatzeile beginnt mit binary_id", pr.line.rfind(bid + ";", 0) == 0);
    check("Formatzeile entspricht dem beobachteten Snapshot", pr.line == ex::format_perm_result(bid, pr.unified));
    check_eq("n_ops gesetzt", pr.n_ops, std::uint64_t{64});
    check_eq("timed_ops = insert+lookup", pr.timed_ops, std::uint64_t{128});
    check_eq("Observer: lookups", pr.unified.axis_stats[0][0], std::uint64_t{64});
    check_eq("Observer: hits", pr.unified.axis_stats[0][1], std::uint64_t{64});
    check_eq("Observer: inserts", pr.unified.axis_stats[0][3], std::uint64_t{64});
    check_eq("Observer: tier_fill_level", pr.unified.tier_fill_level, std::uint64_t{64});
}

void negative_non_conforming_tier_is_gated() {
    std::cout << "== Negativ: nicht-konforme IObservableTier-Huelle ==\n";
    WrongValueObservableTier tier;
    std::string const        bid       = "search_algo=wrong_value/node_type=mock";
    auto const               zero_snap = ana::ComdareTierObserverSnapshot{};
    std::string const        zero_line = ex::format_perm_result(bid, zero_snap);

    ex::PermResult const pr = ex::run_observable_perm(tier, bid, /*n_ops=*/64);

    check("Gate lief (cases_total > 0)", pr.conformance_cases_total > 0);
    check("Gate NICHT bestanden", !pr.conformance_passed);
    check("Gate: first_fail gesetzt", pr.conformance_first_fail > 0);
    check("Gate: cases_passed < cases_total", pr.conformance_cases_passed < pr.conformance_cases_total);
    check("Keine Zwei-Phasen-Gueltigkeit", !pr.two_phase_valid);
    check("Kein realer Mess-Snapshot", !pr.unified_real);
    check("tier_observe wurde nicht gerufen", tier.observe_calls == 0);
    check_eq("total_ns bleibt 0", pr.total_ns, std::int64_t{0});
    check_eq("n_ops bleibt 0", pr.n_ops, std::uint64_t{0});
    check_eq("timed_ops bleibt 0", pr.timed_ops, std::uint64_t{0});
    check("Snapshot bleibt genullt", pr.unified == zero_snap);
    check("Formatzeile ist die genullte Default-Zeile", pr.line == zero_line);
}

// Review-Fix (wf_6e518da1, CONFIRMED-major): der Voll-Lauf dispatcht bei aktiver Workload-Dimension auf
// run_workload_perm (cache_engine_builder_iterator.hpp:731-735) mit EIGENEM Gate-Aufruf (perm_runner.hpp:245-246).
// Dieser Fall beweist den Gate-Short-Circuit auch fuer DIESEN Zweig: Registry-aufgeloestes Profil (kein
// profile_by_name-Fallback auf run_observable_perm), nicht-konformes Tier => genullte Zeile, keine Load-/Run-Phase.
void negative_non_conforming_tier_is_gated_on_workload_path() {
    std::cout << "== Negativ: nicht-konforme Huelle im run_workload_perm-Zweig ==\n";
    namespace wd = ::comdare::cache_engine::builder::workload_driver;

    WrongValueObservableTier tier;
    std::string const        bid       = "search_algo=wrong_value/node_type=mock";
    auto const               zero_snap = ana::ComdareTierObserverSnapshot{};

    std::map<std::string, wd::WorkloadConfig> registry;
    registry["e3_gate_profile"] = wd::WorkloadConfig{}; // Inhalt egal: der Gate-Fail-Return kommt VOR jeder Nutzung.

    ex::PermResult const pr = ex::run_workload_perm(tier, /*rollback=*/nullptr, /*scan=*/nullptr, bid,
                                                    "e3_gate_profile", /*n_ops=*/64, /*seed=*/42,
                                                    /*load_records=*/0, &registry);

    check("WL: Gate lief (cases_total > 0)", pr.conformance_cases_total > 0);
    check("WL: Gate NICHT bestanden", !pr.conformance_passed);
    check("WL: profile_name traegt den Achsen-Wert", pr.profile_name == "e3_gate_profile");
    check("WL: keine Zwei-Phasen-Gueltigkeit", !pr.two_phase_valid);
    check("WL: kein realer Mess-Snapshot", !pr.unified_real);
    check("WL: tier_observe wurde nicht gerufen", tier.observe_calls == 0);
    check_eq("WL: total_ns bleibt 0", pr.total_ns, std::int64_t{0});
    check_eq("WL: timed_ops bleibt 0", pr.timed_ops, std::uint64_t{0});
    check("WL: Snapshot bleibt genullt", pr.unified == zero_snap);
    check("WL: Load-Phase uebersprungen (Tier leer nach Gate)", tier.tier_size() == 0);
}

} // namespace

int main() {
    std::cout << "==== E3 Contract: Conformance-Gate wirkt im perm_runner (#223) ====\n";
    positive_conforming_tier_measures();
    negative_non_conforming_tier_is_gated();
    negative_non_conforming_tier_is_gated_on_workload_path();
    std::cout << "\n==== E3 Gate: " << (g_fail == 0 ? "ALLE OK" : (std::to_string(g_fail) + " FEHLER")) << " ====\n";
    return g_fail == 0 ? 0 : 1;
}
