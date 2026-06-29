// #223 (AUDIT-A3 K9, 2026-06-28) — REGRESSIONS-TEST, der das Konformitaets-Gate (import -> GATE -> messen)
// FESTNAGELT. Beweist literal, dass run_conformance_gate (builder/pruef_dock/conformance_gate.hpp) eine
// IDriveableTier-Huelle gegen das std::map<uint64,uint64>-Orakel (RF1-7 + 2000 deterministische Zufalls-Ops)
// treibt UND konforme von nicht-konformen Huellen UNTERSCHEIDET. Damit kann keine nicht-std::map-konforme
// Huelle als gueltige Mess-Zeile durchrutschen (Meta-Lehre #3: nur konform-verifizierte Lebewesen liefern
// gueltige Mess-Daten; der Aufrufer perm_runner gated VOR der Messung — run_observable_perm:154-155 /
// run_workload_perm:233-234 -> gate_failed_result_ = genullte Zeile, two_phase_valid=false).
//
// WARUM dieser Test existiert: der Dossier-Vorbefund B3-2 ("die Voll-Lauf-Treiber-Funktion ruft das Gate
// NICHT") war STALE/FALSCH (das Gate IST verdrahtet). Dieser Test nagelt die Gate-DISKRIMINIERUNG gegen
// Regression fest. CI-erzwungen ueber die contract-Stage (reines C++/std::map, Linux-g++-tauglich, kein cl/
// Harness) -> schliesst die E4-Harness-CI-Verifikationsluecke fuer genau diesen Konformitaets-Contract.
//
// Standalone (plain int main, KEIN gtest, KEIN Anatomie-/Boost-/generated-Include) — wie test_winsorized_mean.
// Braucht nur den cache_engine-Include-Root: conformance_gate.hpp zieht nur <anatomy/idriveable_tier.hpp> +
// <map>/<random>/<cstdint> (alles std bzw. ein reiner ABI-Header).

#include <builder/pruef_dock/conformance_gate.hpp>
#include <anatomy/idriveable_tier.hpp>
// #188-4a: das reale KAryTraversal-Organ ueber den Pilot-Store (RawSlotStore) — reine lib-Header (CI-tauglich, kein
// cl/anatomy/boost/generated, exakt der #223-Standalone-Stil) → beweist KAryTraversal == std::map-konform (Weg-A).
#include <axes/lookup/composable/composable_search.hpp>
#include <axes/lookup/composable/k_ary_traversal_organ.hpp>

#include <cstdint>
#include <cstdio>
#include <map>

namespace dock = comdare::cache_engine::builder::pruef_dock;
namespace anat = comdare::cache_engine::anatomy;
namespace cmp  = comdare::cache_engine::lookup::composable;

namespace {

int  g_fail = 0;
void check(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

// ── KONFORM: eine korrekte std::map-Huelle (MUSS das Gate bestehen). ──
class MapTier final : public anat::IDriveableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        return m_.insert_or_assign(k, v).second; // true = NEUER Key (false = Update), exakt der tier_insert-Vertrag
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        auto const it = m_.find(k);
        if (it == m_.end()) return false;
        if (out != nullptr) *out = it->second;
        return true;
    }
    [[nodiscard]] bool          tier_erase(std::uint64_t k) noexcept override { return m_.erase(k) != 0; }
    void                        tier_clear() noexcept override { m_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return m_.size(); }

private:
    std::map<std::uint64_t, std::uint64_t> m_;
};

// ── NICHT-KONFORM #1: tier_lookup meldet IMMER Miss → RF2 (insert->Lookup-Hit erwartet) muss scheitern. ──
class LookupBrokenTier final : public anat::IDriveableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        return m_.insert_or_assign(k, v).second;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t, std::uint64_t*) const noexcept override { return false; } // DEFEKT
    [[nodiscard]] bool tier_erase(std::uint64_t k) noexcept override { return m_.erase(k) != 0; }
    void               tier_clear() noexcept override { m_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return m_.size(); }

private:
    std::map<std::uint64_t, std::uint64_t> m_;
};

// ── NICHT-KONFORM #2: tier_insert meldet das NEU-Flag falsch (immer true, auch bei Update) → RF3 (Duplikat
//    -> false erwartet) muss scheitern. ──
class InsertFlagBrokenTier final : public anat::IDriveableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        m_.insert_or_assign(k, v);
        return true; // DEFEKT: meldet immer "neu" (auch beim Update)
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        auto const it = m_.find(k);
        if (it == m_.end()) return false;
        if (out != nullptr) *out = it->second;
        return true;
    }
    [[nodiscard]] bool          tier_erase(std::uint64_t k) noexcept override { return m_.erase(k) != 0; }
    void                        tier_clear() noexcept override { m_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return m_.size(); }

private:
    std::map<std::uint64_t, std::uint64_t> m_;
};

// ── NICHT-KONFORM #3: tier_size meldet IMMER 0 → RF2 (Groesse == oracle.size() nach Insert) muss scheitern. ──
class SizeBrokenTier final : public anat::IDriveableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        return m_.insert_or_assign(k, v).second;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        auto const it = m_.find(k);
        if (it == m_.end()) return false;
        if (out != nullptr) *out = it->second;
        return true;
    }
    [[nodiscard]] bool          tier_erase(std::uint64_t k) noexcept override { return m_.erase(k) != 0; }
    void                        tier_clear() noexcept override { m_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return 0; } // DEFEKT
private:
    std::map<std::uint64_t, std::uint64_t> m_;
};

// ── #188-4a / #188-4a-C: KAryTraversal<Arity> ueber den realen Pilot-Store (ComposedSearch<KAryTraversal<Arity>,
//    RawSlotStore>). K = COMPILE-TIME-Permutation (StaticAxisNode — eigene Tier-Binary je Arity; User-Entscheid
//    2026-06-29), KEIN Laufzeit-Kanal. Je Arity ein DISTINKTER Organ-Typ. Treibt das ECHTE k-Wege-Organ durch das
//    std::map-Orakel → beweist Weg-A-Store-Traversierbarkeit + std::map-Konformitaet (lookup_in bit-identisch zu
//    SortedBinary auf dem sortierten Store) fuer JEDE compile-time Arity. MUSS bestehen. ──
template <unsigned Arity>
class KAryComposedTier final : public anat::IDriveableTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool const was_new = !s_.lookup(k).has_value(); // NEU-Flag wie tier_insert-Vertrag (true = neuer Key)
        s_.insert(k, v);
        return was_new;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        auto const r = s_.lookup(k);
        if (!r) return false;
        if (out != nullptr) *out = *r;
        return true;
    }
    [[nodiscard]] bool          tier_erase(std::uint64_t k) noexcept override { return s_.erase(k); }
    void                        tier_clear() noexcept override { s_.clear(); }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return s_.occupied_count(); }

private:
    cmp::ComposedSearch<cmp::KAryTraversal<Arity>, cmp::RawSlotStore> s_;
};

// Compile-time per-Arity-Gate: jede Arity ist ein eigener Organ-Typ (StaticAxisNode) -> eigener Konformitaets-Lauf
// gegen das std::map-Orakel. Beweist, dass die k-ary-Achse fuer JEDE compile-time gewaehlte Arity std::map-konform ist.
template <unsigned Arity>
void run_kary_arity_gate() {
    KAryComposedTier<Arity> t;
    auto const              r = dock::run_conformance_gate(t);
    char                    lbl[96];
    std::snprintf(lbl, sizeof(lbl), "k_ary<Arity=%u> (compile-time KAryTraversal<%u>/RawSlotStore): passed()==true",
                  Arity, Arity);
    check(lbl, r.passed());
    std::snprintf(lbl, sizeof(lbl), "k_ary<Arity=%u>: cases_total > 0 (Gate lief wirklich)", Arity);
    check(lbl, r.cases_total > 0);
    std::snprintf(lbl, sizeof(lbl), "k_ary<Arity=%u>: first_fail == 0 (keine Verletzung)", Arity);
    check(lbl, r.first_fail == 0);
    std::printf("    k_ary<Arity=%u>: cases=%llu/%llu first_fail=%llu\n", Arity,
                static_cast<unsigned long long>(r.cases_passed), static_cast<unsigned long long>(r.cases_total),
                static_cast<unsigned long long>(r.first_fail));
}

} // namespace

int main() {
    std::printf("== test_conformance_gate (#223 — Konformitaets-Gate festnageln) ==\n");

    // (1) KONFORM → passed(); jede Einzel-Zusicherung bestanden; kein first_fail.
    {
        MapTier    t;
        auto const r = dock::run_conformance_gate(t);
        check("konforme std::map-Huelle: passed()==true", r.passed());
        check("konform: cases_total > 0 (Gate lief wirklich)", r.cases_total > 0);
        check("konform: cases_passed == cases_total", r.cases_passed == r.cases_total);
        check("konform: first_fail == 0 (keine Verletzung)", r.first_fail == 0);
        std::printf("    konform: cases=%llu/%llu first_fail=%llu\n", static_cast<unsigned long long>(r.cases_passed),
                    static_cast<unsigned long long>(r.cases_total), static_cast<unsigned long long>(r.first_fail));
    }

    // (2) NICHT-KONFORM (Lookup-Miss) → Gate FAENGT es: !passed, first_fail>0, nicht alle cases bestanden.
    {
        LookupBrokenTier t;
        auto const       r = dock::run_conformance_gate(t);
        check("Lookup-defekt: passed()==false (Gate faengt die Nicht-Konformitaet)", !r.passed());
        check("Lookup-defekt: first_fail > 0", r.first_fail > 0);
        check("Lookup-defekt: cases_passed < cases_total", r.cases_passed < r.cases_total);
        std::printf("    lookup-defekt: first_fail=%llu (cases %llu/%llu)\n",
                    static_cast<unsigned long long>(r.first_fail), static_cast<unsigned long long>(r.cases_passed),
                    static_cast<unsigned long long>(r.cases_total));
    }

    // (3) NICHT-KONFORM (insert NEU-Flag) → !passed (RF3: Duplikat-Insert muss false liefern).
    {
        InsertFlagBrokenTier t;
        auto const           r = dock::run_conformance_gate(t);
        check("insert-Flag-defekt: passed()==false", !r.passed());
        check("insert-Flag-defekt: first_fail > 0", r.first_fail > 0);
    }

    // (4) NICHT-KONFORM (size==0) → !passed (RF2: Groesse muss oracle.size() folgen).
    {
        SizeBrokenTier t;
        auto const     r = dock::run_conformance_gate(t);
        check("size-defekt: passed()==false", !r.passed());
        check("size-defekt: first_fail > 0", r.first_fail > 0);
    }

    // (5) #188-4a / #188-4a-C COMPILE-TIME ARITY: je Arity in {2,4,8,16} ist ein DISTINKTER, compile-time-spezialisierter
    //     KAryTraversal<Arity>-Organ-Typ (StaticAxisNode — eigene Tier-Binary; User-Entscheid 2026-06-29: k-ary-Arity =
    //     compile-time-Permutation, KEIN Laufzeit-Kanal). Jeder MUSS das std::map-Orakel bestehen → die Arity-Variation
    //     ist REAL (anderer compile-time-Separator-Pfad in lookup_in) UND korrekt (Meta-Lehre #3). Organ stateless.
    run_kary_arity_gate<2u>();
    run_kary_arity_gate<4u>();
    run_kary_arity_gate<8u>();
    run_kary_arity_gate<16u>();

    std::printf("== test_conformance_gate: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
