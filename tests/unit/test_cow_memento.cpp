// #133 Rev. 2 Copy-on-Write-Memento (2026-06-11) — SEMANTIK-VERIFIKATION in-process über reale Referenz-Kompositionen.
//
// Belegt literal (je Komposition Art/Hot/Masstree):
//   (1) AKTIV:        der lazy CoW-Pfad ist compile-time aktiv (tier_memento_is_copy_on_write()): save=O(1)-
//                     Stat-Snapshots, Daten-Vollkopie erst bei der ersten Warmup-Mutation (Writes/clear).
//   (2) DATEN-EXAKT:  save → {insert-neu | insert-update | erase | lookup | tier_clear} → rollback stellt
//                     tier_size UND Inhalte exakt wieder her (tier_clear über die Eskalations-Vollkopie).
//   (3) IDEMPOTENT:   doppeltes tier_rollback_all → derselbe Stand (IRollbackableTier-Vertrag).
//   (4) COUNTER-CLEAN (zwei_phase-Vertrag tier_observe_trace_abi.hpp: „End-Zustand + Observer-Zähler
//                     identisch zur Einphasen-Messung"): Einphasen-Lauf und Zwei-Phasen-Lauf derselben
//                     deterministischen Op-Sequenz enden mit identischem tier_size und ELEMENTWEISE
//                     identischen T0-(search)- und T6-(allocator)-axis_stats — exakt die Achsen, die das
//                     Memento abdeckt (die auto-gekoppelten Instanz-Achsen T1/T2/T3/T7/T8/T10/T17/T18
//                     zählen Warmup+Messung doppelt — Status-quo-identisch zum Copy-Memento, NICHT Teil
//                     des Memento-Vertrags; die Scan-Achsen T4/T5/T9/T11..T16 sind idempotent).
//   (5) FORTSCHRITT:  die Phase-2-Mess-Op bleibt erhalten (kein Doppel-Undo über Perioden hinweg —
//                     Recording ist nach rollback_all disarmed).
//   (6) HOST-GATE:    rollback_is_empirically_exact(...) == true (die empirische Probe des Mess-Pfads).
//
// Build (standalone, analog test_obs_phaseA):
//   cl /std:c++latest /EHsc /Od /bigobj /DCOMDARE_MEASUREMENT_ON=1 /DCOMDARE_CE_ENABLE_STATISTICS=1
//      + voller ADHOC-Include-Satz (Harness-Include-Liste, build_and_measure_150_tiere.ps1).

#include <anatomy/abi_adapter.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>
#include <anatomy/search_algorithm_anatomy.hpp>
#include <builder/anatomy_commands/tier_observe_trace_abi.hpp> // detail::two_phase_measure + rollback_is_empirically_exact

#include <compositions/art_reference.hpp>
#include <compositions/hot_reference.hpp>
#include <compositions/masstree_reference.hpp>

#include <cstdint>
#include <iostream>
#include <string>

namespace an   = ::comdare::cache_engine::anatomy;
namespace comp = ::comdare::cache_engine::compositions;
namespace acd  = ::comdare::cache_engine::builder::anatomy_commands::detail;

static int  g_fail = 0;
static void tr(std::string const& w, bool c) {
    std::cout << (c ? "  [OK]  " : "  [ERR] ") << w << "\n";
    if (!c) ++g_fail;
}

// Deterministische gemischte Op-Sequenz (identisch je Lauf): LOAD 60 + 240 Ops über 6 Op-Arten.
// rb != nullptr → jede Op zwei-phasig (save → warmup → rollback → measure), exakt wie run_workload_profile.
static void run_sequence(an::IObservableTier& t, an::IRollbackableTier* rb) {
    t.tier_clear();
    for (std::uint64_t k = 1; k <= 60; ++k) (void)t.tier_insert(k, k * 3u); // LOAD (ungemessen, wie perm_runner)
    for (std::uint64_t i = 0; i < 240; ++i) {
        auto op = [&]() -> std::int64_t {
            switch (i % 6u) {
                case 0: (void)t.tier_insert(1000u + i, i); break;              // insert NEU
                case 1: (void)t.tier_insert(1u + (i % 60u), 7777u + i); break; // insert UPDATE
                case 2: (void)t.tier_erase(1u + (i * 7u) % 60u); break;        // erase
                case 3: {
                    std::uint64_t o = 0;
                    (void)t.tier_lookup(1u + (i % 60u), &o);
                    break;
                } // lookup hit-lastig
                case 4: {
                    std::uint64_t o = 0;
                    (void)t.tier_lookup(99999u + i, &o);
                    break;
                } // lookup miss
                case 5: {
                    std::uint64_t c   = 0; // RMW (lookup→upsert, 1 Periode)
                    bool const    hit = t.tier_lookup(1u + (i % 60u), &c);
                    (void)t.tier_insert(1u + (i % 60u), (hit ? c : 0u) ^ (i + 1u));
                    break;
                }
            }
            return 0;
        };
        if (rb != nullptr)
            (void)acd::two_phase_measure(rb, op);
        else
            (void)op();
    }
}

template <class C>
static void check_composition(char const* name) {
    using Anatomy = an::SearchAlgorithmAnatomy<C>;
    using Adapter = an::SearchAlgorithmAbiAdapter<Anatomy>;
    std::cout << "== " << name << " ==\n";

    // (1) CoW-Pfad compile-time AKTIV (kein stiller Rückfall auf das eager Copy-Memento je Op).
    static_assert(Adapter::tier_memento_is_copy_on_write(),
                  "#133 Rev. 2: Referenz-Komposition muss den lazy Copy-on-Write-Memento-Pfad nutzen");
    tr(std::string(name) + ": tier_memento_is_copy_on_write() == true (compile-time)",
       Adapter::tier_memento_is_copy_on_write());

    // ── (4) COUNTER-CLEAN: Einphasen-Lauf vs Zwei-Phasen-Lauf derselben Sequenz ───────────────────────
    Adapter one_phase, two_phase;
    auto*   t1 = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&one_phase));
    auto*   t2 = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&two_phase));
    auto*   rb = dynamic_cast<an::IRollbackableTier*>(static_cast<an::IAnatomyBase*>(&two_phase));
    tr(std::string(name) + ": Schnittstellen castbar (IObservableTier ×2, IRollbackableTier)",
       t1 != nullptr && t2 != nullptr && rb != nullptr);
    if (t1 == nullptr || t2 == nullptr || rb == nullptr) return;

    run_sequence(*t1, nullptr);
    run_sequence(*t2, rb);
    tr(std::string(name) + ": tier_size identisch (1-phasig == 2-phasig)", t1->tier_size() == t2->tier_size());

    an::ComdareTierObserverSnapshot s1{}, s2{};
    t1->tier_observe(&s1);
    t2->tier_observe(&s2);
    bool t0_eq = true, t6_eq = true;
    for (std::size_t f = 0; f < an::kV3FieldCount; ++f) {
        if (s1.axis_stats[0][f] != s2.axis_stats[0][f]) t0_eq = false;
        if (s1.axis_stats[6][f] != s2.axis_stats[6][f]) t6_eq = false;
    }
    tr(std::string(name) + ": T0 search-axis_stats ELEMENTWEISE identisch (counter-clean)", t0_eq);
    tr(std::string(name) + ": T6 allocator-axis_stats ELEMENTWEISE identisch (deriviert exakt)", t6_eq);
    if (!t0_eq || !t6_eq) {
        for (std::size_t f = 0; f < an::kV3FieldCount; ++f)
            std::cout << "    f" << f << ": T0 " << s1.axis_stats[0][f] << " vs " << s2.axis_stats[0][f] << " | T6 "
                      << s1.axis_stats[6][f] << " vs " << s2.axis_stats[6][f] << "\n";
    }
    // Inhalts-Stichproben NACH tier_observe (Lookups hier verfälschen keinen Vergleich mehr).
    bool same_content = true;
    for (std::uint64_t k = 1; k <= 60; ++k) {
        std::uint64_t a = 0, b = 0;
        bool const    ha = t1->tier_lookup(k, &a), hb = t2->tier_lookup(k, &b);
        if (ha != hb || (ha && a != b)) {
            same_content = false;
            break;
        }
    }
    tr(std::string(name) + ": Inhalte 1..60 identisch (1-phasig == 2-phasig)", same_content);

    // ── (2)/(3)/(5) Einzelfall-Semantik direkt am IRollbackableTier ────────────────────────────────────
    Adapter unit;
    auto*   t = dynamic_cast<an::IObservableTier*>(static_cast<an::IAnatomyBase*>(&unit));
    auto*   r = dynamic_cast<an::IRollbackableTier*>(static_cast<an::IAnatomyBase*>(&unit));
    t->tier_clear();
    for (std::uint64_t k = 1; k <= 80; ++k) (void)t->tier_insert(k, k * 3u);
    std::uint64_t v = 0;

    r->tier_save_all();
    (void)t->tier_insert(200u, 7u);
    r->tier_rollback_all(); // insert NEU
    tr(std::string(name) + ": rollback nach insert-neu (size 80, key 200 weg)",
       t->tier_size() == 80 && !t->tier_lookup(200u, &v));

    r->tier_save_all();
    (void)t->tier_insert(40u, 999u);
    r->tier_rollback_all(); // insert UPDATE
    v = 0;
    tr(std::string(name) + ": rollback nach insert-update (40 → alter Wert 120)", t->tier_lookup(40u, &v) && v == 120u);

    r->tier_save_all();
    (void)t->tier_erase(40u);
    r->tier_rollback_all(); // erase
    v = 0;
    tr(std::string(name) + ": rollback nach erase (40 wieder da, Wert 120)", t->tier_lookup(40u, &v) && v == 120u);

    r->tier_save_all();
    v = 0;
    (void)t->tier_lookup(40u, &v);
    r->tier_rollback_all(); // read-only
    tr(std::string(name) + ": rollback nach lookup (size unverändert 80)", t->tier_size() == 80);

    r->tier_save_all();
    t->tier_clear();
    r->tier_rollback_all(); // clear → Eskalation
    v                       = 0;
    bool const    clear_ok  = (t->tier_size() == 80) && t->tier_lookup(1u, &v) && v == 3u;
    std::uint64_t v80       = 0;
    bool const    clear_ok2 = t->tier_lookup(80u, &v80) && v80 == 240u;
    tr(std::string(name) + ": rollback nach tier_clear (CoW-Vollkopie: 80 Einträge zurück)", clear_ok && clear_ok2);

    r->tier_save_all();
    (void)t->tier_insert(201u, 1u);
    r->tier_rollback_all();
    r->tier_rollback_all(); // Idempotenz
    tr(std::string(name) + ": doppeltes rollback_all idempotent (size 80, 201 weg)",
       t->tier_size() == 80 && !t->tier_lookup(201u, &v));

    // (5) FORTSCHRITT: Mess-Op bleibt; die NÄCHSTE Periode rollt nur ihre eigene Warmup-Op zurück.
    r->tier_save_all();
    (void)t->tier_insert(202u, 5u);
    r->tier_rollback_all();         // Periode 1: warmup+rollback
    (void)t->tier_insert(202u, 5u); // Periode 1: MESS-Op (bleibt)
    r->tier_save_all();
    (void)t->tier_insert(203u, 6u);
    r->tier_rollback_all(); // Periode 2: warmup+rollback
    v = 0;
    tr(std::string(name) + ": Mess-Fortschritt bleibt (202 da, 203 weg)",
       t->tier_lookup(202u, &v) && v == 5u && !t->tier_lookup(203u, &v) && t->tier_size() == 81);

    // ── (6) HOST-GATE: empirische Exaktheits-Probe (lässt das Tier geleert zurück → zuletzt) ──────────
    tr(std::string(name) + ": rollback_is_empirically_exact == true", acd::rollback_is_empirically_exact(*t, r));
}

int main() {
    check_composition<comp::ArtComposition>("ArtComposition");
    check_composition<comp::HotComposition>("HotComposition");
    check_composition<comp::MasstreeComposition>("MasstreeComposition");
    std::cout << (g_fail == 0 ? "==== #133 CoW-Memento: ALLE OK ====" : "==== #133 CoW-Memento: FEHLER ====")
              << " (fails=" << g_fail << ")\n";
    return g_fail == 0 ? 0 : 1;
}
