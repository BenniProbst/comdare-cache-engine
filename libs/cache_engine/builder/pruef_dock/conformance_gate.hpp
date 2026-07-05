#pragma once
// V5-I4 — Konformitäts-Gate (User-Direktive 2026-05-31): JEDE geladene Tier-Binary muss VOR der Messung die
// std::map-Hüllen-Konformität ihrer Gattung über ALLE Randfälle bestehen. Egal wie die Hülle innen gebaut ist,
// die Funktionalität muss valide sein; das Experiment misst nur Performance (Doku: messarchitektur_v5_design.md §6,
// messarchitektur_v5_entscheidungen.md §8 — IDriveableTier-Vollständigkeit je Gattung = volle std::map-Hülle).
//
// Treibt IDriveableTier (der funktionale Gattungs-Antrieb — IMMER vorhanden, auch in Release-/funktional-only-DLLs)
// gegen std::map<uint64,uint64> als Oracle. Reihenfolge bindend: import → GATE → (nur bei pass) messen.
//
// SearchAlgorithm-Gattung. Heute auf den 5 Kern-Ops (insert/lookup/erase/clear/size); mit der Drive-Voll-API-
// Erweiterung (V5-I-Drive-Vollausbau) wächst die Randfall-Menge auf die volle std::map-Schnittstelle.

#include <anatomy/idriveable_tier.hpp>
#include <anatomy/scannable_tier.hpp>

#include <array>
#include <cstdio>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <type_traits>

namespace comdare::cache_engine::builder::pruef_dock {

static_assert(std::is_same_v<std::map<std::uint64_t, std::uint64_t>::key_compare, std::less<std::uint64_t>>,
              "conformance gate oracle key_comp must stay std::less<uint64_t>");

/// Ergebnis eines Konformitäts-Laufs — Quoten statt nur bool (V5-Design §6: „Konformitäts-Quoten").
struct ConformanceResult {
    std::uint64_t      cases_total  = 0; ///< Anzahl geprüfter Einzel-Zusicherungen
    std::uint64_t      cases_passed = 0; ///< davon bestanden
    std::uint64_t      first_fail   = 0; ///< 1-basierter Index der ersten verletzten Zusicherung (0 = keine)
    [[nodiscard]] bool passed() const noexcept { return cases_total > 0 && cases_passed == cases_total; }
};

/// run_conformance_gate — deterministische Randfall- + Zufallssequenz gegen std::map-Oracle.
/// `tier` wird VORHER geleert. Je Op verglichen: Rückgabe-Semantik (neu?/Treffer?/existierte?), Lookup-Wert, Größe.
/// noexcept: eine werfende Hülle verletzt den ABI-noexcept-Vertrag → gilt als nicht-konform.
[[nodiscard]] inline ConformanceResult run_conformance_gate(anatomy::IDriveableTier& tier, std::uint64_t seed = 42,
                                                            std::uint64_t n_random = 2000, bool wide_keys = false,
                                                            std::FILE* report = nullptr) noexcept {
    ConformanceResult r{};
    auto              check = [&](bool ok) noexcept {
        ++r.cases_total;
        if (ok) {
            ++r.cases_passed;
        } else if (r.first_fail == 0) {
            r.first_fail = r.cases_total;
        }
    };
    auto report_block = [&](char const* name, std::uint64_t before_total, std::uint64_t before_passed) noexcept {
        if (report == nullptr) return;
        auto const total_delta  = r.cases_total - before_total;
        auto const passed_delta = r.cases_passed - before_passed;
        std::fprintf(report, "    %s: %s (%llu/%llu)\n", name, (total_delta == passed_delta) ? "OK" : "FAIL",
                     static_cast<unsigned long long>(passed_delta), static_cast<unsigned long long>(total_delta));
    };
    auto report_skip = [&](char const* name, char const* why) noexcept {
        if (report == nullptr) return;
        std::fprintf(report, "    %s: SKIP (%s)\n", name, why);
    };
    try {
        std::map<std::uint64_t, std::uint64_t> oracle;
        tier.tier_clear();

        // RF1 dagger-empty/find/count/contains: leeres Tier -> Lookup-Miss, Groesse 0, empty/count/contains false.
        auto const    rf1_total  = r.cases_total;
        auto const    rf1_passed = r.cases_passed;
        std::uint64_t v          = 123;
        check(tier.tier_lookup(7, &v) == false);
        check(tier.tier_size() == 0);
        check((tier.tier_size() == 0) == oracle.empty());
        check(tier.tier_lookup(7, &v) == oracle.contains(7));
        check((tier.tier_lookup(7, &v) ? 1u : 0u) == oracle.count(7));
        report_block("RF1 dagger-empty/find/count/contains", rf1_total, rf1_passed);

        // RF2 dagger-insert_or_assign/find: insert neu -> true; Lookup-Hit + korrekter Wert; Groesse 1.
        auto const rf2_total  = r.cases_total;
        auto const rf2_passed = r.cases_passed;
        check(tier.tier_insert(7, 70) == true);
        oracle.insert_or_assign(7, 70);
        check(tier.tier_lookup(7, &v) == true && v == 70);
        check(tier.tier_size() == oracle.size());
        report_block("RF2 dagger-insert_or_assign/find(new)", rf2_total, rf2_passed);

        // RF3 dagger-insert_or_assign: insert Duplikat (Update) -> false; Wert aktualisiert; Groesse unveraendert.
        auto const rf3_total  = r.cases_total;
        auto const rf3_passed = r.cases_passed;
        check(tier.tier_insert(7, 700) == false);
        oracle.insert_or_assign(7, 700);
        check(tier.tier_lookup(7, &v) == true && v == 700);
        check(tier.tier_size() == oracle.size());
        report_block("RF3 dagger-insert_or_assign(update)", rf3_total, rf3_passed);

        // RF4 dagger-erase/find/empty: erase existierend -> true; danach Miss; Groesse dekrementiert; empty nach letztem
        // erase.
        auto const rf4_total  = r.cases_total;
        auto const rf4_passed = r.cases_passed;
        check(tier.tier_erase(7) == true);
        oracle.erase(7);
        check(tier.tier_lookup(7, &v) == false);
        check(tier.tier_size() == oracle.size());
        check((tier.tier_size() == 0) == oracle.empty());
        check(oracle.find(7) == oracle.end());
        report_block("RF4 dagger-erase/find/empty", rf4_total, rf4_passed);

        // RF5 dagger-erase: erase nicht-existent -> false; Groesse unveraendert.
        auto const rf5_total  = r.cases_total;
        auto const rf5_passed = r.cases_passed;
        check(tier.tier_erase(999) == false);
        check(tier.tier_size() == oracle.size());
        report_block("RF5 dagger-erase(miss)", rf5_total, rf5_passed);

        auto lookup_matches_oracle = [&](std::uint64_t key) noexcept {
            auto const    it       = oracle.find(key);
            std::uint64_t got      = 0;
            bool const    tier_hit = tier.tier_lookup(key, &got);
            check(tier_hit == (it != oracle.end()));
            if (it != oracle.end()) { check(got == it->second); }
        };
        auto insert_if_absent = [&](std::uint64_t key, std::uint64_t value) noexcept {
            std::uint64_t old = 0;
            if (tier.tier_lookup(key, &old)) return false;
            return tier.tier_insert(key, value);
        };

        // RF6 dagger-contains/count/find: Hit- und Miss-Faelle explizit gegen std::map::contains/count/find.
        auto const rf6_total  = r.cases_total;
        auto const rf6_passed = r.cases_passed;
        check(tier.tier_insert(11, 110) == true);
        oracle.insert_or_assign(11, 110);
        check(tier.tier_lookup(11, &v) == oracle.contains(11));
        check((tier.tier_lookup(11, &v) ? 1u : 0u) == oracle.count(11));
        lookup_matches_oracle(11);
        check(tier.tier_lookup(12, &v) == oracle.contains(12));
        check((tier.tier_lookup(12, &v) ? 1u : 0u) == oracle.count(12));
        lookup_matches_oracle(12);
        check(tier.tier_erase(11) == true);
        oracle.erase(11);
        lookup_matches_oracle(11);
        report_block("RF6 dagger-contains/count/find", rf6_total, rf6_passed);

        // RF7 dagger-insert/emplace/try_emplace: host-seitig als lookup -> ggf. insert; vorhandene Werte bleiben alt.
        auto const rf7_total  = r.cases_total;
        auto const rf7_passed = r.cases_passed;
        check(insert_if_absent(17, 170) == oracle.insert({17, 170}).second);
        lookup_matches_oracle(17);
        check(insert_if_absent(17, 171) == oracle.insert({17, 171}).second);
        lookup_matches_oracle(17);
        check(insert_if_absent(18, 180) == oracle.emplace(18, 180).second);
        lookup_matches_oracle(18);
        check(insert_if_absent(18, 181) == oracle.emplace(18, 181).second);
        lookup_matches_oracle(18);
        check(insert_if_absent(19, 190) == oracle.try_emplace(19, 190).second);
        lookup_matches_oracle(19);
        check(insert_if_absent(19, 191) == oracle.try_emplace(19, 191).second);
        lookup_matches_oracle(19);
        check(tier.tier_size() == oracle.size());
        report_block("RF7 dagger-insert/emplace/try_emplace", rf7_total, rf7_passed);

        // RF8 dagger-insert_or_assign/operator[]/at: direkte Upserts, Default-Insert 0 und at-hit/miss.
        auto const rf8_total  = r.cases_total;
        auto const rf8_passed = r.cases_passed;
        check(tier.tier_insert(20, 200) == oracle.insert_or_assign(20, 200).second);
        lookup_matches_oracle(20);
        check(tier.tier_insert(20, 201) == oracle.insert_or_assign(20, 201).second);
        lookup_matches_oracle(20);

        auto tier_bracket_value = [&](std::uint64_t key, std::uint64_t& out) noexcept {
            if (!tier.tier_lookup(key, &out)) { (void)tier.tier_insert(key, 0); }
            return tier.tier_lookup(key, &out);
        };
        std::uint64_t bracket = 999;
        auto const    op_new  = oracle[21];
        check(tier_bracket_value(21, bracket) && bracket == op_new);
        check(tier.tier_size() == oracle.size());
        check(tier.tier_insert(22, 220) == oracle.insert_or_assign(22, 220).second);
        auto const op_old = oracle[22];
        check(tier_bracket_value(22, bracket) && bracket == op_old);
        check(tier.tier_size() == oracle.size());

        std::uint64_t at_hit = 0;
        check(tier.tier_lookup(22, &at_hit) && at_hit == oracle.at(22));
        bool oracle_at_miss_threw = false;
        try {
            (void)oracle.at(222);
        } catch (std::out_of_range const&) { oracle_at_miss_threw = true; }
        check(!tier.tier_lookup(222, &at_hit) && oracle_at_miss_threw);
        report_block("RF8 dagger-insert_or_assign/operator[]/at", rf8_total, rf8_passed);

        // RF9: Zufallssequenz ueber begrenzten Key-Raum (Kollisionen/Updates/Erases), je Schritt Lookup+Groesse.
        auto const      rf9_total  = r.cases_total;
        auto const      rf9_passed = r.cases_passed;
        std::mt19937_64 rng{seed};
        for (std::uint64_t i = 0; i < n_random; ++i) {
            std::uint64_t const key = rng() % 256;
            switch (rng() % 3) {
                case 0: { // insert/update
                    std::uint64_t const val      = rng();
                    bool const          was_new  = (oracle.find(key) == oracle.end());
                    bool const          tier_new = tier.tier_insert(key, val);
                    oracle.insert_or_assign(key, val);
                    check(tier_new == was_new);
                    break;
                }
                case 1: { // lookup
                    auto const    it       = oracle.find(key);
                    std::uint64_t got      = 0;
                    bool const    tier_hit = tier.tier_lookup(key, &got);
                    check(tier_hit == (it != oracle.end()));
                    if (it != oracle.end()) { check(got == it->second); }
                    break;
                }
                default: { // erase
                    bool const existed      = (oracle.find(key) != oracle.end());
                    bool const tier_existed = tier.tier_erase(key);
                    oracle.erase(key);
                    check(tier_existed == existed);
                    break;
                }
            }
            check(tier.tier_size() == oracle.size());
        }
        report_block("RF9 random-core(insert/find/erase/size)", rf9_total, rf9_passed);

        // RF10 dagger-begin/end/lower_bound/upper_bound/equal_range/key_comp: geordnete Host-Synthese ueber
        // IScannableTier, falls das geladene Tier diese additive ABI-faehigkeit wirklich traegt.
        tier.tier_clear();
        oracle.clear();
        constexpr std::array<std::uint64_t, 5> order_keys{40u, 10u, 30u, 20u, 50u};
        for (auto const key : order_keys) {
            (void)tier.tier_insert(key, key * 10u + 3u);
            oracle.insert_or_assign(key, key * 10u + 3u);
        }

        auto* scan = dynamic_cast<anatomy::IScannableTier*>(&tier);
        if (scan == nullptr) {
            report_skip("RF10 dagger-begin/end", "ordnungs-Ops übersprungen (kein IScannableTier)");
            report_skip("RF11 dagger-lower_bound/upper_bound/equal_range",
                        "ordnungs-Ops übersprungen (kein IScannableTier)");
        } else {
            std::uint64_t probe_sum   = 0;
            auto const    probe_count = scan->tier_scan(0u, oracle.size(), &probe_sum);
            if (probe_count == 0 && !oracle.empty()) {
                report_skip("RF10 dagger-begin/end",
                            "ordnungs-Ops übersprungen (IScannableTier liefert keine Records)");
                report_skip("RF11 dagger-lower_bound/upper_bound/equal_range",
                            "ordnungs-Ops übersprungen (IScannableTier liefert keine Records)");
            } else {
                auto scan_matches = [&](std::uint64_t start_key, std::uint64_t max_count) noexcept {
                    std::uint64_t got_sum   = 0;
                    auto const    got_count = scan->tier_scan(start_key, max_count, &got_sum);

                    std::uint64_t expected_count = 0;
                    std::uint64_t expected_sum   = 0;
                    for (auto it = oracle.lower_bound(start_key); it != oracle.end() && expected_count < max_count;
                         ++it) {
                        expected_sum += it->second;
                        ++expected_count;
                    }
                    check(got_count == expected_count);
                    check(got_sum == expected_sum);
                };

                auto const rf10_total  = r.cases_total;
                auto const rf10_passed = r.cases_passed;
                for (std::uint64_t prefix = 0; prefix <= oracle.size() + 1u; ++prefix) { scan_matches(0u, prefix); }
                scan_matches(60u, 4u); // end(): groesser als max -> leere Range
                report_block("RF10 dagger-begin/end", rf10_total, rf10_passed);

                auto const rf11_total  = r.cases_total;
                auto const rf11_passed = r.cases_passed;
                for (auto const key : std::array<std::uint64_t, 6>{5u, 10u, 25u, 30u, 50u, 60u}) {
                    scan_matches(key, 1u); // lower_bound(key)
                    if (key != std::numeric_limits<std::uint64_t>::max()) {
                        auto const upper_start = oracle.contains(key) ? key + 1u : key;
                        scan_matches(upper_start, 1u); // upper_bound/equal_range second fuer uint64-Probe-Keys
                    }
                }
                report_block("RF11 dagger-lower_bound/upper_bound/equal_range", rf11_total, rf11_passed);
            }
        }
        auto const rf12_total  = r.cases_total;
        auto const rf12_passed = r.cases_passed;
        auto const comp        = oracle.key_comp();
        check(comp(10u, 20u));
        check(!comp(20u, 10u));
        check(!comp(10u, 10u));
        report_block("RF12 dagger-key_comp(std::less<uint64_t>)", rf12_total, rf12_passed);

        // RF13 dagger-clear/empty/find: clear -> Groesse 0; beliebiger Lookup-Miss; empty wahr.
        auto const rf13_total  = r.cases_total;
        auto const rf13_passed = r.cases_passed;
        tier.tier_clear();
        oracle.clear();
        check(tier.tier_size() == 0);
        check(tier.tier_lookup(42, &v) == false);
        check((tier.tier_size() == 0) == oracle.empty());
        report_block("RF13 dagger-clear/empty/find", rf13_total, rf13_passed);

        if (wide_keys) {
            auto const                             rf14_total  = r.cases_total;
            auto const                             rf14_passed = r.cases_passed;
            constexpr std::array<std::uint64_t, 4> keys{0u, 65'536u, (1ull << 32u),
                                                        std::numeric_limits<std::uint64_t>::max()};
            for (auto const key : keys) {
                auto const val = key ^ 0xA5A5A5A5A5A5A5A5ull;
                check(tier.tier_insert(key, val) == oracle.insert_or_assign(key, val).second);
                lookup_matches_oracle(key);
            }
            report_block("RF14 wide_keys opt-in", rf14_total, rf14_passed);
        } else {
            report_skip("RF14 wide_keys opt-in", "wide_keys=false");
        }
    } catch (...) {
        check(false); // werfende Hülle = nicht-konform
    }
    return r;
}

} // namespace comdare::cache_engine::builder::pruef_dock
