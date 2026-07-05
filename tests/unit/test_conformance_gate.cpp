// #223 (AUDIT-A3 K9, 2026-06-28) — REGRESSIONS-TEST, der das Konformitaets-Gate (import -> GATE -> messen)
// FESTNAGELT. Beweist literal, dass run_conformance_gate (builder/pruef_dock/conformance_gate.hpp) eine
// IDriveableTier-Huelle gegen das std::map<uint64,uint64>-Orakel treibt UND konforme von nicht-konformen Huellen
// UNTERSCHEIDET. Damit kann keine nicht-std::map-konforme Huelle als gueltige Mess-Zeile durchrutschen.
//
// Standalone (plain int main, KEIN gtest, KEIN Anatomie-/Boost-/generated-Include) — wie test_winsorized_mean.
// Braucht nur den cache_engine-Include-Root: conformance_gate.hpp zieht nur reine ABI-Header + std.

#include <anatomy/idriveable_tier.hpp>
#include <anatomy/scannable_tier.hpp>
#include <builder/pruef_dock/conformance_gate.hpp>
// #188-4a: das reale KAryTraversal-Organ ueber den Pilot-Store (RawSlotStore) — reine lib-Header (CI-tauglich, kein
// cl/anatomy/boost/generated, exakt der #223-Standalone-Stil) → beweist KAryTraversal == std::map-konform (Weg-A).
#include <axes/lookup/composable/composable_search.hpp>
#include <axes/lookup/composable/k_ary_traversal_organ.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <vector>

namespace dock = comdare::cache_engine::builder::pruef_dock;
namespace anat = comdare::cache_engine::anatomy;
namespace cmp  = comdare::cache_engine::lookup::composable;

namespace {

int  g_fail = 0;
void check(char const* what, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what);
    if (!ok) ++g_fail;
}

// ── KONFORM: eine korrekte std::map-Huelle (MUSS das Gate bestehen), inkl. optionalem geordnetem Scan. ──
class MapTier : public anat::IDriveableTier, public anat::IScannableTier {
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

    [[nodiscard]] std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                                          std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0;
        std::uint64_t sum     = 0;
        for (auto it = m_.lower_bound(start_key); it != m_.end() && visited < max_count; ++it) {
            sum += it->second;
            ++visited;
        }
        if (out_checksum != nullptr) *out_checksum += sum;
        return visited;
    }

protected:
    std::map<std::uint64_t, std::uint64_t> m_;
};

// ── NICHT-KONFORM #1: tier_lookup meldet IMMER Miss → RF2 (insert->Lookup-Hit erwartet) muss scheitern. ──
class LookupBrokenTier final : public MapTier {
public:
    [[nodiscard]] bool tier_lookup(std::uint64_t, std::uint64_t*) const noexcept override { return false; } // DEFEKT
};

// ── NICHT-KONFORM #2: tier_insert meldet das NEU-Flag falsch (immer true, auch bei Update) → RF3 muss scheitern. ──
class InsertFlagBrokenTier final : public MapTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        (void)MapTier::tier_insert(k, v);
        return true; // DEFEKT: meldet immer "neu" (auch beim Update)
    }
};

// ── NICHT-KONFORM #3: tier_size meldet IMMER 0 → RF2 (Groesse == oracle.size() nach Insert) muss scheitern. ──
class SizeBrokenTier final : public MapTier {
public:
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return 0; } // DEFEKT
};

// ── NICHT-KONFORM #4: synthetisches try_emplace sieht einen vorhandenen Key faelschlich als fehlend und ueberschreibt
//    dadurch den alten Wert. Das Gate muss den Erhalt des ALT-Werts gegen std::map::try_emplace fangen. ──
class BrokenTryEmplaceTier final : public MapTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        auto const inserted = MapTier::tier_insert(k, v);
        if (k == 19u && v == 190u) {
            try_key_present_      = true;
            try_key_lookup_count_ = 0;
        }
        return inserted;
    }

    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        if (k == 19u && try_key_present_) {
            ++try_key_lookup_count_;
            if (try_key_lookup_count_ == 2u) return false; // DEFEKT: vorhandener try_emplace-Key wirkt fehlend.
        }
        return MapTier::tier_lookup(k, out);
    }

private:
    bool                  try_key_present_      = false;
    mutable std::uint64_t try_key_lookup_count_ = 0;
};

// ── NICHT-KONFORM #5: empty-Synthese ueber size luegt genau bei leerem Zustand. ──
class BrokenEmptyTier final : public MapTier {
public:
    [[nodiscard]] std::uint64_t tier_size() const noexcept override {
        auto const n = MapTier::tier_size();
        return n == 0u ? 1u : n; // DEFEKT: empty() waere false, obwohl keine Keys existieren.
    }
};

// ── NICHT-KONFORM #6: Scan liefert gefilterte Records in Insert-Reihenfolge statt in Key-Reihenfolge. ──
class BrokenOrderTier final : public MapTier {
public:
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        bool const inserted = MapTier::tier_insert(k, v);
        if (inserted) insertion_order_.push_back(k);
        return inserted;
    }
    [[nodiscard]] bool tier_erase(std::uint64_t k) noexcept override {
        bool const erased = MapTier::tier_erase(k);
        if (erased) {
            insertion_order_.erase(std::remove(insertion_order_.begin(), insertion_order_.end(), k),
                                   insertion_order_.end());
        }
        return erased;
    }
    void tier_clear() noexcept override {
        MapTier::tier_clear();
        insertion_order_.clear();
    }
    [[nodiscard]] std::uint64_t tier_scan(std::uint64_t start_key, std::uint64_t max_count,
                                          std::uint64_t* out_checksum) const noexcept override {
        std::uint64_t visited = 0;
        std::uint64_t sum     = 0;
        for (auto const key : insertion_order_) {
            if (key < start_key || visited >= max_count) continue;
            std::uint64_t value = 0;
            if (MapTier::tier_lookup(key, &value)) {
                sum += value;
                ++visited;
            }
        }
        if (out_checksum != nullptr) *out_checksum += sum;
        return visited;
    }

private:
    std::vector<std::uint64_t> insertion_order_;
};

// ── #188-4a / #188-4a-C: KAryTraversal<Arity> ueber den realen Pilot-Store (ComposedSearch<KAryTraversal<Arity>,
//    RawSlotStore>). K = COMPILE-TIME-Permutation. Treibt das ECHTE k-Wege-Organ durch das std::map-Orakel. ──
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

// Compile-time per-Arity-Gate: jede Arity ist ein eigener Organ-Typ (StaticAxisNode) -> eigener Konformitaets-Lauf.
template <unsigned Arity>
void run_kary_arity_gate() {
    KAryComposedTier<Arity> t;
    std::FILE*              report = nullptr;
    if constexpr (Arity == 2u) { report = stdout; }
    auto const r = dock::run_conformance_gate(t, 42u, 2000u, false, report);
    char       lbl[96];
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
        auto const r = dock::run_conformance_gate(t, 42u, 2000u, false, stdout);
        check("konforme std::map-Huelle: passed()==true", r.passed());
        check("konform: cases_total > 0 (Gate lief wirklich)", r.cases_total > 0);
        check("konform: cases_passed == cases_total", r.cases_passed == r.cases_total);
        check("konform: first_fail == 0 (keine Verletzung)", r.first_fail == 0);
        std::printf("    konform: cases=%llu/%llu first_fail=%llu\n", static_cast<unsigned long long>(r.cases_passed),
                    static_cast<unsigned long long>(r.cases_total), static_cast<unsigned long long>(r.first_fail));
    }

    // (2) NICHT-KONFORM (Lookup-Miss) → Gate FAENGT es.
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

    // (5) NICHT-KONFORM (try_emplace ALT-Wert wird ueberschrieben) → !passed.
    {
        BrokenTryEmplaceTier t;
        auto const           r = dock::run_conformance_gate(t);
        check("try_emplace-defekt: passed()==false", !r.passed());
        check("try_emplace-defekt: first_fail > 0", r.first_fail > 0);
    }

    // (6) NICHT-KONFORM (empty/size luegt bei leer) → !passed.
    {
        BrokenEmptyTier t;
        auto const      r = dock::run_conformance_gate(t);
        check("empty-defekt: passed()==false", !r.passed());
        check("empty-defekt: first_fail > 0", r.first_fail > 0);
    }

    // (7) NICHT-KONFORM (Scan unsortiert) → Ordnungs-Dagger muessen scheitern.
    {
        BrokenOrderTier t;
        auto const      r = dock::run_conformance_gate(t);
        check("order-defekt: passed()==false", !r.passed());
        check("order-defekt: first_fail > 0", r.first_fail > 0);
    }

    // (8) #188-4a / #188-4a-C COMPILE-TIME ARITY: je Arity in {2,4,8,16} ist ein DISTINKTER Organ-Typ.
    run_kary_arity_gate<2u>();
    run_kary_arity_gate<4u>();
    run_kary_arity_gate<8u>();
    run_kary_arity_gate<16u>();

    std::printf("== test_conformance_gate: %s ==\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
