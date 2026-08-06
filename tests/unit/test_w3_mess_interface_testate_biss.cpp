// W3-KERN BISS (2026-08-05) -- der BEIDSEITIGE Biss der Mess-Interface-Testate.
//
// Ein Testat, das nur an korrekten Huellen gruen wird, beweist nichts: es koennte tautologisch sein.
// Diese TU faehrt darum JEDE Testat-Funktion aus builder/pruef_dock/mess_interface_testate.hpp ZWEIMAL --
// gegen eine KORREKTE Huelle (muss bestehen) und gegen eine gezielt DEFEKTE (muss ROT beissen). Muster:
// test_conformance_gate.cpp ("6 Broken-Huellen fangen"), hier auf die Mess-Interfaces uebertragen.
//
// Der Biss ist billig, weil die Testat-Funktionen INTERFACE-REFERENZEN nehmen: es braucht keinen DLL-Bau,
// nur je eine kleine Huelle im selben Prozess. Genau das war der Grund fuer diesen Schnitt (die Substanz
// liegt in der Header-Funktion, nicht in der Loader-TU).
//
// Zusaetzlich traegt diese TU den Beweis, den das Dock-Testat am geladenen Modul NICHT fuehren kann:
// **tier_moves REAL > 0**. Alle benannten Kompositionen des Repos tragen migration_policy = NoMigration
// (compositions/*.hpp, ausnahmslos gegriffen) -- ein aktiver Migrations-Vertreter existiert als geladenes
// Modul nicht. Hier steuert die Fixture die Strategie und kann den aktiven Ast scharf pruefen.
//
// Standalone (plain int main, KEIN gtest, KEIN Anatomie-Umbrella/Boost/generated-Include): der Testat-Header
// zieht nur die reinen ABI-Header + std.

#include <builder/pruef_dock/mess_interface_testate.hpp>

#include <anatomy/allocator_proxy_tier.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp>
#include <anatomy/resource_controllable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>

#include <cstdint>
#include <cstdio>
#include <map>

namespace ana  = ::comdare::cache_engine::anatomy;
namespace dock = ::comdare::cache_engine::builder::pruef_dock;

namespace {

int  g_fail = 0;
void check(char const* was, bool ok) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", was);
    if (!ok) ++g_fail;
}

/// Die Abnahme eines Biss-Paares: die korrekte Huelle MUSS bestehen, die defekte MUSS durchfallen.
void biss(char const* etikett, dock::ConformanceResult const& gut, dock::ConformanceResult const& kaputt) {
    std::printf("  -- %s: gut %llu/%llu (first_fail=%llu) | kaputt %llu/%llu (first_fail=%llu)\n", etikett,
                static_cast<unsigned long long>(gut.cases_passed), static_cast<unsigned long long>(gut.cases_total),
                static_cast<unsigned long long>(gut.first_fail), static_cast<unsigned long long>(kaputt.cases_passed),
                static_cast<unsigned long long>(kaputt.cases_total),
                static_cast<unsigned long long>(kaputt.first_fail));
    check("korrekte Huelle: cases_total > 0 (nicht-leere Population)", gut.cases_total > 0);
    check("korrekte Huelle: Testat bestanden", gut.passed());
    check("defekte Huelle: Testat BEISST (passed()==false)", !kaputt.passed());
    check("defekte Huelle: first_fail > 0 (der Biss ist lokalisiert)", kaputt.first_fail > 0);
}

// ---------------------------------------------------------------------------------------------------------
// KORREKTE Referenz-Huelle: implementiert dieselbe Interface-Familie wie der SearchAlgorithmAbiAdapter unter
// MESSUNG-AN und erfuellt deren Vertraege ehrlich (Fenster-Semantik der Observer-Zaehler, exakte Segment-
// Identitaeten, verlustfreie Migration, klammernde RC-Semantik, wachsende Allocator-Statistik).
// ---------------------------------------------------------------------------------------------------------
class KorrekteHuelle : public ana::IObservableTier,
                       public ana::IMeasurableWorkloadV2,
                       public ana::IMeasurableWorkloadV3,
                       public ana::IRollbackableTier,
                       public ana::IMigratableTier,
                       public ana::IResourceControllableTier,
                       public ana::IAllocatorProxyTier {
public:
    // -- IDriveableTier (ueber IObservableTier) -------------------------------------------------------------
    [[nodiscard]] bool tier_insert(std::uint64_t k, std::uint64_t v) noexcept override {
        ++f_insert_;
        ++alloc_ops_;
        (void)kalt_.erase(k);
        return heiss_.insert_or_assign(k, v).second;
    }
    [[nodiscard]] bool tier_lookup(std::uint64_t k, std::uint64_t* out) const noexcept override {
        ++f_lookup_;
        if (auto const it = heiss_.find(k); it != heiss_.end()) {
            if (out != nullptr) *out = it->second;
            ++f_hit_;
            return true;
        }
        if (auto const it = kalt_.find(k); it != kalt_.end()) { // die kalte 2. Ebene bleibt sichtbar
            if (out != nullptr) *out = it->second;
            ++f_hit_;
            return true;
        }
        return false;
    }
    [[nodiscard]] bool tier_erase(std::uint64_t k) noexcept override {
        ++f_erase_;
        return (heiss_.erase(k) + kalt_.erase(k)) != 0;
    }
    void tier_clear() noexcept override {
        heiss_.clear();
        kalt_.clear();
    }
    [[nodiscard]] std::uint64_t tier_size() const noexcept override { return heiss_.size() + kalt_.size(); }

    // -- IObservableTier -----------------------------------------------------------------------------------
    // FENSTER-Semantik wie im echten Adapter (Q1-Sequenz SCHRITT 4): die auto-gekoppelten Zaehler werden nach
    // dem Lesen genullt -- ein Snapshot beschreibt GENAU die Ops seit dem letzten Snapshot.
    void tier_observe(ana::ComdareTierObserverSnapshot* out) const noexcept override {
        if (out == nullptr) return;
        *out = ana::ComdareTierObserverSnapshot{};
        fuelle_schema_(out);
        out->observable_axis_count = 1;
        out->tier_fill_level       = tier_size();
        out->filled_axis_count     = 1;
        out->batches_measured      = 4;
        out->seg_ns[0]             = 1200;
        out->seg_ns[5]             = 800;
        out->seg_framework_ns      = 400;
        out->seg_run_total_ns      = 1200 + 800 + 400;
        f_lookup_ = f_hit_ = f_insert_ = f_erase_ = 0;
    }
    void tier_reset_statistics() noexcept override { f_lookup_ = f_hit_ = f_insert_ = f_erase_ = 0; }

    // -- IMeasurableWorkloadV2 -----------------------------------------------------------------------------
    [[nodiscard]] std::uint64_t run_workload_segmented(std::uint64_t ops, std::uint64_t batches, std::uint64_t,
                                                       ana::ComdareSegmentLatencyV1* out) noexcept override {
        if (out == nullptr) return 0;
        *out = ana::ComdareSegmentLatencyV1{};
        if (ops == 0 || batches == 0) return 0;
        out->seg_search_algo_ns   = static_cast<std::int64_t>(batches * 11u);
        out->seg_allocator_ns     = static_cast<std::int64_t>(batches * 7u);
        out->seg_memory_layout_ns = static_cast<std::int64_t>(batches * 5u);
        out->seg_serialization_ns = static_cast<std::int64_t>(batches * 3u);
        out->total_ns =
            out->seg_search_algo_ns + out->seg_allocator_ns + out->seg_memory_layout_ns + out->seg_serialization_ns;
        out->batches_measured = batches;
        return batches;
    }

    // -- IMeasurableWorkloadV3 -----------------------------------------------------------------------------
    [[nodiscard]] std::uint64_t run_workload_segmented_v2(std::uint64_t ops, std::uint64_t batches, std::uint64_t,
                                                          ana::ComdareSegmentLatencyV2* out) noexcept override {
        if (out == nullptr) return 0;
        *out = ana::ComdareSegmentLatencyV2{};
        if (ops == 0 || batches == 0) return 0;
        std::int64_t summe = 0;
        for (std::size_t t = 0; t < ana::kV3AxisCount; ++t) {
            out->seg_ns[t] = static_cast<std::int64_t>(batches * (t + 1u));
            summe += out->seg_ns[t];
        }
        out->total_ns         = summe;
        out->batches_measured = batches;
        out->seg_framework_ns = static_cast<std::int64_t>(batches * 3u);
        out->seg_run_total_ns = summe + out->seg_framework_ns;
        return batches;
    }

    // -- IRollbackableTier ---------------------------------------------------------------------------------
    void tier_save_all() noexcept override {
        try {
            gesichert_heiss_ = heiss_;
            gesichert_kalt_  = kalt_;
        } catch (...) {} // noexcept-Vertrag
    }
    void tier_rollback_all() noexcept override {
        try {
            heiss_ = gesichert_heiss_;
            kalt_  = gesichert_kalt_;
        } catch (...) {}
    }

    // -- IMigratableTier (AKTIVE Strategie: bewegt REAL, verliert nichts) ----------------------------------
    [[nodiscard]] std::uint64_t tier_migrate_step(std::uint64_t max_moves) noexcept override {
        std::uint64_t bewegt = 0;
        try {
            auto it = heiss_.begin();
            while (it != heiss_.end() && (max_moves == 0 || bewegt < max_moves)) {
                kalt_.insert_or_assign(it->first, it->second); // Move heiss -> kalt, kein Verlust
                it = heiss_.erase(it);
                ++bewegt;
            }
        } catch (...) { return bewegt; }
        return bewegt;
    }

    // -- IResourceControllableTier (Caps + Klammer-Semantik wie im Adapter) --------------------------------
    void tier_query_resource_caps(ana::ComdareResourceControlV1* out) const noexcept override {
        if (out == nullptr) return;
        *out                         = ana::ComdareResourceControlV1{};
        out->thread_count            = 64;
        out->prefetch_distance       = 64;
        out->pool_budget_bytes       = (std::uint64_t{1} << 30);
        out->batch_size              = 4096;
        out->inline_threshold_bytes  = 256;
        out->controllable_axis_count = 5;
    }
    [[nodiscard]] std::uint64_t tier_apply_resource_control(ana::ComdareResourceControlV1 const* in) noexcept override {
        if (in == nullptr) return 0;
        ana::ComdareResourceControlV1 caps{};
        tier_query_resource_caps(&caps);
        auto klammer = [](std::uint64_t v, std::uint64_t cap) noexcept {
            return (v == 0 || cap == 0 || v <= cap) ? v : cap;
        };
        // thread_count wird geklammert + gehalten, zaehlt aber NIE (label-only, T8-Phantom-Guard).
        rc_.thread_count           = klammer(in->thread_count, caps.thread_count);
        rc_.prefetch_distance      = klammer(in->prefetch_distance, caps.prefetch_distance);
        rc_.pool_budget_bytes      = klammer(in->pool_budget_bytes, caps.pool_budget_bytes);
        rc_.batch_size             = klammer(in->batch_size, caps.batch_size);
        rc_.inline_threshold_bytes = klammer(in->inline_threshold_bytes, caps.inline_threshold_bytes);
        return zaehle_angewandt_();
    }

    // -- IAllocatorProxyTier -------------------------------------------------------------------------------
    void tier_get_allocator(ana::ComdareAllocatorProxyV1* out) const noexcept override {
        if (out == nullptr) return;
        *out                       = ana::ComdareAllocatorProxyV1{};
        out->format_version        = ana::kAllocatorProxyFormatVersion;
        out->allocator_family_id   = 7;
        out->flags                 = (std::uint64_t{1} << 0u) | (std::uint64_t{1} << 1u); // identity + stats
        out->allocation_count      = alloc_ops_;
        out->total_bytes_allocated = alloc_ops_ * 32u;
        out->total_bytes_in_use    = tier_size() * 32u;
        out->live_nodes            = tier_size();
    }

protected:
    /// Nur BENANNTE Schema-Slots befuellen (das ist der Vertrag, den die Schema-Wache prueft).
    virtual void fuelle_schema_(ana::ComdareTierObserverSnapshot* out) const noexcept {
        out->axis_stats[0][0] = f_lookup_; // "lookup"
        out->axis_stats[0][1] = f_hit_;    // "hit"
        out->axis_stats[0][3] = f_insert_; // "insert"
        out->axis_stats[0][4] = f_erase_;  // "erase"
    }
    [[nodiscard]] virtual std::uint64_t zaehle_angewandt_() const noexcept {
        std::uint64_t n = 0;
        if (rc_.prefetch_distance != 0) ++n;
        if (rc_.pool_budget_bytes != 0) ++n;
        if (rc_.batch_size != 0) ++n;
        if (rc_.inline_threshold_bytes != 0) ++n;
        return n; // thread_count NIE
    }

    std::map<std::uint64_t, std::uint64_t> heiss_;
    std::map<std::uint64_t, std::uint64_t> kalt_;
    std::map<std::uint64_t, std::uint64_t> gesichert_heiss_;
    std::map<std::uint64_t, std::uint64_t> gesichert_kalt_;
    ana::ComdareResourceControlV1          rc_{};
    mutable std::uint64_t                  f_lookup_  = 0;
    mutable std::uint64_t                  f_hit_     = 0;
    mutable std::uint64_t                  f_insert_  = 0;
    mutable std::uint64_t                  f_erase_   = 0;
    std::uint64_t                          alloc_ops_ = 0;
};

// -- DEFEKT 1: Observer-Zaehler EINGEFROREN (konstant, unabhaengig von der Op-Zahl) ------------------------
class ObsEingefroren final : public KorrekteHuelle {
protected:
    void fuelle_schema_(ana::ComdareTierObserverSnapshot* out) const noexcept override {
        out->axis_stats[0][0] = 4242; // DEFEKT: bewegt sich nie
        out->axis_stats[0][3] = 4242;
    }
};

// -- DEFEKT 2: Schreiber belegt einen Slot OHNE Schema-Namen (stiller Messwert-Verlust an der CSV-Naht) ----
class ObsSchemaDrift final : public KorrekteHuelle {
protected:
    void fuelle_schema_(ana::ComdareTierObserverSnapshot* out) const noexcept override {
        KorrekteHuelle::fuelle_schema_(out);
        out->axis_stats[2][7] = 99; // DEFEKT: kV3AxisSchema[2].names[7] == nullptr
    }
};

// -- DEFEKT 3: V2 total_ns weicht von der Segment-Summe ab -------------------------------------------------
class V2SummenBruch final : public KorrekteHuelle {
public:
    [[nodiscard]] std::uint64_t run_workload_segmented(std::uint64_t ops, std::uint64_t batches, std::uint64_t seed,
                                                       ana::ComdareSegmentLatencyV1* out) noexcept override {
        std::uint64_t const r = KorrekteHuelle::run_workload_segmented(ops, batches, seed, out);
        if (out != nullptr && r > 0) out->total_ns += 1; // DEFEKT: Identitaet gebrochen
        return r;
    }
};

// -- DEFEKT 4: V3 verliert Mess-Zeit im unbenannten Rest (die P-MD3-Identitaet bricht) ---------------------
class V3IdentitaetsBruch final : public KorrekteHuelle {
public:
    [[nodiscard]] std::uint64_t run_workload_segmented_v2(std::uint64_t ops, std::uint64_t batches, std::uint64_t seed,
                                                          ana::ComdareSegmentLatencyV2* out) noexcept override {
        std::uint64_t const r = KorrekteHuelle::run_workload_segmented_v2(ops, batches, seed, out);
        if (out != nullptr && r > 0) out->seg_framework_ns = 0; // DEFEKT: der Rest wird verschwiegen
        return r;
    }
};

// -- DEFEKT 5: rollback ist ein No-Op ---------------------------------------------------------------------
class RollbackNoOp final : public KorrekteHuelle {
public:
    void tier_rollback_all() noexcept override {} // DEFEKT
};

// -- DEFEKT 6: Migration VERLIERT Records (der eigentliche Schaden eines 2-Ebenen-Moves) ------------------
class MigrationVerlust final : public KorrekteHuelle {
public:
    [[nodiscard]] std::uint64_t tier_migrate_step(std::uint64_t max_moves) noexcept override {
        std::uint64_t bewegt = 0;
        try {
            auto it = heiss_.begin();
            while (it != heiss_.end() && (max_moves == 0 || bewegt < max_moves)) {
                it = heiss_.erase(it); // DEFEKT: geloescht statt bewegt
                ++bewegt;
            }
        } catch (...) { return bewegt; }
        return bewegt;
    }
};

// -- DEFEKT 7: RC zaehlt thread_count als angewandt (Phantom-applied, T8) ---------------------------------
class RcPhantom final : public KorrekteHuelle {
protected:
    [[nodiscard]] std::uint64_t zaehle_angewandt_() const noexcept override {
        std::uint64_t n = KorrekteHuelle::zaehle_angewandt_();
        if (rc_.thread_count != 0) ++n; // DEFEKT
        return n;
    }
};

// -- DEFEKT 8: Allocator-Proxy deklariert die stats-Route, liefert aber eingefrorene Zaehler --------------
class AllocEingefroren final : public KorrekteHuelle {
public:
    void tier_get_allocator(ana::ComdareAllocatorProxyV1* out) const noexcept override {
        KorrekteHuelle::tier_get_allocator(out);
        if (out == nullptr) return;
        out->allocation_count      = 17; // DEFEKT: konstant trotz deklarierter stats-Route
        out->total_bytes_allocated = 544;
        out->live_nodes            = 3;
    }
};

} // namespace

int main() {
    std::printf("==== W3-BISS: jede Testat-Funktion beidseitig (korrekt besteht / defekt beisst) ====\n");

    // (1) IObservableTier -- zwei Kern-Aussagen, zwei Defekte.
    {
        KorrekteHuelle gut;
        ObsEingefroren kaputt;
        biss("IObservableTier / Zaehler eingefroren", dock::testat_observable_tier(gut),
             dock::testat_observable_tier(kaputt));
    }
    {
        KorrekteHuelle gut;
        ObsSchemaDrift kaputt;
        biss("IObservableTier / unbenannter Schema-Slot belegt", dock::testat_observable_tier(gut),
             dock::testat_observable_tier(kaputt));
    }

    // (2) IMeasurableWorkloadV2
    {
        KorrekteHuelle gut;
        V2SummenBruch  kaputt;
        biss("IMeasurableWorkloadV2 / total_ns != Summe der Segmente", dock::testat_measurable_workload_v2(gut),
             dock::testat_measurable_workload_v2(kaputt));
    }

    // (3) IMeasurableWorkloadV3 -- die KERN-Invariante
    {
        KorrekteHuelle     gut;
        V3IdentitaetsBruch kaputt;
        biss("IMeasurableWorkloadV3 / Sum(seg)+framework != run_total", dock::testat_measurable_workload_v3(gut),
             dock::testat_measurable_workload_v3(kaputt));
    }

    // (4) IRollbackableTier
    {
        KorrekteHuelle gut;
        RollbackNoOp   kaputt;
        biss("IRollbackableTier / rollback ist ein No-Op", dock::testat_rollbackable_tier(gut, gut),
             dock::testat_rollbackable_tier(kaputt, kaputt));
    }

    // (5) IMigratableTier -- hier zugleich der Beweis 'tier_moves REAL > 0' (aktive Fixture-Strategie)
    {
        KorrekteHuelle   gut;
        MigrationVerlust kaputt;
        biss("IMigratableTier / Migration verliert Records", dock::testat_migratable_tier(gut, gut),
             dock::testat_migratable_tier(kaputt, kaputt));

        KorrekteHuelle aktiv;
        for (std::uint64_t k = 0; k < 64; ++k) (void)aktiv.tier_insert(k, k * 3u + 1u);
        std::uint64_t const bewegt = aktiv.tier_migrate_step(32);
        std::printf("    -> aktive Migrations-Strategie: tier_migrate_step(32) bewegte %llu Records\n",
                    static_cast<unsigned long long>(bewegt));
        check("aktive Strategie: tier_moves REAL > 0 (P4-#123-Doktrin, hier beweisbar)", bewegt > 0);
        check("aktive Strategie: moved <= max_moves", bewegt <= 32);
        check("aktive Strategie: kein Record verloren (tier_size unveraendert)", aktiv.tier_size() == 64);
    }

    // (6) IResourceControllableTier
    {
        KorrekteHuelle gut;
        RcPhantom      kaputt;
        biss("IResourceControllableTier / thread_count als Phantom-applied gezaehlt",
             dock::testat_resource_controllable_tier(gut, &gut),
             dock::testat_resource_controllable_tier(kaputt, &kaputt));
    }

    // (7) IAllocatorProxyTier
    {
        KorrekteHuelle   gut;
        AllocEingefroren kaputt;
        biss("IAllocatorProxyTier / stats-Route deklariert, Zaehler eingefroren",
             dock::testat_allocator_proxy_tier(gut), dock::testat_allocator_proxy_tier(kaputt));
    }

    std::printf("\n==== W3-BISS: %s ====\n", g_fail == 0 ? "ALLE OK" : "FEHLER");
    return g_fail == 0 ? 0 : 1;
}
