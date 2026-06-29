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

#include <cstdint>
#include <map>
#include <random>

namespace comdare::cache_engine::builder::pruef_dock {

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
                                                            std::uint64_t n_random = 2000) noexcept {
    ConformanceResult r{};
    auto              check = [&](bool ok) noexcept {
        ++r.cases_total;
        if (ok) {
            ++r.cases_passed;
        } else if (r.first_fail == 0) {
            r.first_fail = r.cases_total;
        }
    };
    try {
        std::map<std::uint64_t, std::uint64_t> oracle;
        tier.tier_clear();

        // RF1: leeres Tier — Lookup-Miss, Größe 0.
        std::uint64_t v = 123;
        check(tier.tier_lookup(7, &v) == false);
        check(tier.tier_size() == 0);

        // RF2: insert neu → true; Lookup-Hit + korrekter Wert; Größe 1.
        check(tier.tier_insert(7, 70) == true);
        oracle[7] = 70;
        check(tier.tier_lookup(7, &v) == true && v == 70);
        check(tier.tier_size() == oracle.size());

        // RF3: insert Duplikat (Update) → false (kein NEUER Key); Wert aktualisiert; Größe unverändert.
        check(tier.tier_insert(7, 700) == false);
        oracle[7] = 700;
        check(tier.tier_lookup(7, &v) == true && v == 700);
        check(tier.tier_size() == oracle.size());

        // RF4: erase existierend → true; danach Miss; Größe dekrementiert.
        check(tier.tier_erase(7) == true);
        oracle.erase(7);
        check(tier.tier_lookup(7, &v) == false);
        check(tier.tier_size() == oracle.size());

        // RF5: erase nicht-existent → false; Größe unverändert.
        check(tier.tier_erase(999) == false);
        check(tier.tier_size() == oracle.size());

        // RF6: Zufallssequenz über begrenzten Key-Raum (Kollisionen/Updates/Erases), je Schritt Lookup+Größe.
        std::mt19937_64 rng{seed};
        for (std::uint64_t i = 0; i < n_random; ++i) {
            std::uint64_t const key = rng() % 256;
            switch (rng() % 3) {
                case 0: { // insert/update
                    std::uint64_t const val      = rng();
                    bool const          was_new  = (oracle.find(key) == oracle.end());
                    bool const          tier_new = tier.tier_insert(key, val);
                    oracle[key]                  = val;
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

        // RF7: clear → Größe 0; beliebiger Lookup-Miss.
        tier.tier_clear();
        oracle.clear();
        check(tier.tier_size() == 0);
        check(tier.tier_lookup(42, &v) == false);
    } catch (...) {
        check(false); // werfende Hülle = nicht-konform
    }
    return r;
}

} // namespace comdare::cache_engine::builder::pruef_dock
