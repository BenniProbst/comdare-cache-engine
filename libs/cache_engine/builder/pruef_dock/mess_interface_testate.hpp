#pragma once
// mess_interface_testate.hpp -- W3-KERN (2026-08-05, Owner-GO R3): POST-COMPILE-TESTATE der MESS-Interfaces
// am Pruef-Dock.
//
// SIBLING von conformance_gate.hpp, NICHT dessen Erweiterung: das Gate prueft die FUNKTIONALE
// std::map-Huellen-Konformitaet (IDriveableTier gegen das std::map-Orakel) und bleibt byte-unberuehrt.
// Diese Datei prueft die messungs-gegateten Sub-Interfaces, die der SearchAlgorithmAbiAdapter unter
// COMDARE_MEASUREMENT_ON ZUSAETZLICH erbt (abi_adapter.hpp:238-245):
//
//   IObservableTier          -- tier_observe-POD-Pull + tier_reset_statistics
//   IMeasurableWorkloadV2    -- run_workload_segmented (4 instrumentierte Achsen)
//   IMeasurableWorkloadV3    -- run_workload_segmented_v2 (ALLE 18 Achsen-Segmente)
//   IRollbackableTier        -- memento_all (tier_save_all/tier_rollback_all)
//   IMigratableTier          -- tier_migrate_step (2-Ebenen-Move)
//   IResourceControllableTier-- RC-POD-Konsum (Caps/apply)
//   IAllocatorProxyTier      -- tier_get_allocator (non-dagger Proxy)
//
// REIHENFOLGE (bindend, wie am Dock): import -> GATE (run_conformance_gate) -> erst dann diese Testate.
// Eine Huelle, die das funktionale Gate nicht besteht, wird gar nicht erst vermessen.
//
// FORM: je Interface EINE freie, noexcept-Pruef-Funktion, die INTERFACE-REFERENZEN nimmt (keine Templates,
// kein DLL-Wissen, kein Loader). Daraus folgen drei Dinge, die der W3-Schnitt braucht:
//   (a) die Nach-Abgabe-Integration in den CEB-Pruefstand-Batch ist rein ADDITIV -- dieselben Funktionen,
//       gerufen aus measure()/pruef_only, ohne dass hier etwas umgebaut wird;
//   (b) der beidseitige BISS ist billig -- ein Broken-Fake je Testat genuegt, kein DLL-Bau;
//   (c) das Ergebnis ist eine QUOTE (ConformanceResult, wiederverwendet), kein bool: eine leere Population
//       (cases_total == 0) macht jede Aussage wahr und wird darum vom Aufrufer als Fehler gewertet
//       (passed() fordert cases_total > 0 -- dieselbe Anti-Leerlauf-Lesart wie am Gate).
//
// ANTI-LEERLAUF ist DURCHGAENGIGES DESIGN, kein Zusatz: keine Pruefung ist ein blosser Praesenz-Check.
// Jedes Testat treibt ECHTE Ops ueber IDriveableTier bzw. den Mess-Einstiegspunkt des Moduls und
// vergleicht VORHER/NACHHER. Ein Modul, dessen Zaehler sich nicht bewegen, faellt durch.
//
// EHRLICHKEIT (honest-0, keine erfundenen Werte): wo eine Komposition eine Faehigkeit strategiebedingt
// NICHT traegt (NoMigration bewegt nichts; ein Allocator ohne Statistik-Route meldet keine Bytes), wird
// der NULLPUNKT als solcher GEPRUEFT (Flag geloescht UND Zaehler 0), nicht stillschweigend uebersprungen.
//
// DOKTRIN: header-only C++23, ASCII-Kommentare, nur stdlib + die ABI-Header der anatomy-Welt.
// KEINE Beruehrung von abi_adapter/PODs/Wire -- diese Datei LIEST die ABI, sie erweitert sie nicht.

#include "conformance_gate.hpp" // ConformanceResult (Quoten-Form) + probe_allocator_proxy (WIEDERVERWENDUNG)

#include <anatomy/allocator_proxy_tier.hpp>
#include <anatomy/idriveable_tier.hpp>
#include <anatomy/measurable_workload.hpp>
#include <anatomy/observable_tier.hpp> // IObservableTier + IMigratableTier + kV3AxisSchema + Snapshot-POD
#include <anatomy/resource_controllable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace comdare::cache_engine::builder::pruef_dock {

namespace mess_testat_detail {

/// Quoten-Sammler im Gate-Stil: EIN check() fuehrt cases_total/cases_passed/first_fail UND den optionalen
/// Report. Bewusst dieselbe Ergebnis-Struktur wie run_conformance_gate, damit der kuenftige Pruefstand-Batch
/// alle Quoten gleichfoermig aggregieren kann.
struct Quote {
    ConformanceResult r{};
    std::FILE*        report = nullptr;

    void check(bool ok, char const* what) noexcept {
        ++r.cases_total;
        if (ok) {
            ++r.cases_passed;
        } else if (r.first_fail == 0) {
            r.first_fail = r.cases_total;
        }
        if (report != nullptr) std::fprintf(report, "    %s: %s\n", what, ok ? "OK" : "FAIL");
    }
    void note(char const* what, unsigned long long value) const noexcept {
        if (report != nullptr) std::fprintf(report, "    (info) %s = %llu\n", what, value);
    }
    void note_i(char const* what, long long value) const noexcept {
        if (report != nullptr) std::fprintf(report, "    (info) %s = %lld\n", what, value);
    }
};

/// Der Schluessel-Raum aller Testate. Fix und klein, damit der Vorher/Nachher-Vergleich vollstaendig
/// (jeder Schluessel, nicht eine Stichprobe) und ohne Allokation laufen kann.
inline constexpr std::uint64_t kKeySpace = 512;

/// Deterministischer Op-Treiber (xorshift64 statt <random>: reproduzierbar OHNE Abhaengigkeit von der
/// libstdc++-Version, denn Host und Modul koennen mit verschiedenen Standard-Bibliotheken gebaut sein).
/// Fuehrt insert/lookup/erase in fester Mischung. Rueckgabe = Zahl der Lookup-Treffer (Anti-Leerlauf-Zeuge).
[[nodiscard]] inline std::uint64_t drive_ops(anatomy::IDriveableTier& tier, std::uint64_t n_ops,
                                             std::uint64_t seed) noexcept {
    std::uint64_t x       = (seed == 0) ? 0x9E3779B97F4A7C15ull : seed;
    std::uint64_t treffer = 0;
    for (std::uint64_t i = 0; i < n_ops; ++i) {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        std::uint64_t const k = x % kKeySpace;
        if ((i & 7u) == 7u) {
            (void)tier.tier_erase(k);
        } else {
            (void)tier.tier_insert(k, k * 7u + 1u);
        }
        std::uint64_t v = 0;
        if (tier.tier_lookup(k, &v)) ++treffer;
    }
    return treffer;
}

/// Vollstaendiger Zustands-Abzug ueber den Schluessel-Raum (Treffer-Flag + Wert je Schluessel).
struct ZustandsAbzug {
    std::array<std::uint8_t, kKeySpace>  treffer{};
    std::array<std::uint64_t, kKeySpace> wert{};
    std::uint64_t                        groesse = 0;
    std::uint64_t                        belegt  = 0; // Zahl der Treffer -- Anti-Leerlauf-Zeuge
};

[[nodiscard]] inline ZustandsAbzug abzug_von(anatomy::IDriveableTier const& tier) noexcept {
    ZustandsAbzug z{};
    for (std::uint64_t k = 0; k < kKeySpace; ++k) {
        std::uint64_t v                        = 0;
        bool const    hit                      = tier.tier_lookup(k, &v);
        z.treffer[static_cast<std::size_t>(k)] = hit ? std::uint8_t{1} : std::uint8_t{0};
        z.wert[static_cast<std::size_t>(k)]    = hit ? v : 0;
        if (hit) ++z.belegt;
    }
    z.groesse = tier.tier_size();
    return z;
}

[[nodiscard]] inline bool abzuege_gleich(ZustandsAbzug const& a, ZustandsAbzug const& b) noexcept {
    if (a.groesse != b.groesse) return false;
    for (std::size_t i = 0; i < kKeySpace; ++i) {
        if (a.treffer[i] != b.treffer[i]) return false;
        if (a.treffer[i] != 0 && a.wert[i] != b.wert[i]) return false;
    }
    return true;
}

/// Summe ALLER im Schema BENANNTEN Observer-Felder. Schema-getrieben (kV3AxisSchema), nichts hartkodiert:
/// waechst das Schema, waechst diese Summe automatisch mit.
[[nodiscard]] inline std::uint64_t benannte_feld_summe(anatomy::ComdareTierObserverSnapshot const& s) noexcept {
    std::uint64_t sum = 0;
    for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t)
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f)
            if (anatomy::kV3AxisSchema[t].names[f] != nullptr) sum += s.axis_stats[t][f];
    return sum;
}

/// Zahl der Achsen mit mindestens EINEM belegten BENANNTEN Feld (schema-getrieben).
[[nodiscard]] inline std::size_t belegte_achsen(anatomy::ComdareTierObserverSnapshot const& s) noexcept {
    std::size_t n = 0;
    for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f) {
            if (anatomy::kV3AxisSchema[t].names[f] != nullptr && s.axis_stats[t][f] != 0) {
                ++n;
                break;
            }
        }
    }
    return n;
}

} // namespace mess_testat_detail

// ---------------------------------------------------------------------------------------------------------------
// (1) IObservableTier -- tier_observe-POD-Pull, SCHEMA-getriebene Feld-Wache, Anti-Leerlauf, Reset-Semantik
// ---------------------------------------------------------------------------------------------------------------
//
// SCHEMA-WACHE (der Kern dieses Testats): kV3AxisSchema IST der Vertrag zwischen dem Schreiber in der DLL
// (fill_observer_v3) und den CSV-Spaltennamen im Host. Ein Slot OHNE Schema-Namen darf darum NICHT belegt
// sein -- ein Wert dort waere eine Zahl ohne Spalte, also ein stiller Messwert-Verlust an der CSV-Naht
// (dieselbe Fehlerklasse wie der A8-S1-Befund seg_ns[17]). Geprueft wird ueber die Tabelle, NICHT ueber
// hartkodierte Achsen-/Feld-Zahlen: eine neue Achse oder ein neues Feld zieht die Wache automatisch mit.
//
// ANTI-LEERLAUF: zwei Mess-Fenster verschiedener Groesse. tier_observe nullt die auto-gekoppelten Organe an
// seinem Ende (Q1-Sequenz SCHRITT 2/4), das zweite Fenster misst also GENAU seine eigenen Ops -- ein Fenster
// mit dreifacher Op-Zahl MUSS eine groessere benannte Feld-Summe liefern. Ein eingefrorener Zaehler faellt hier.
[[nodiscard]] inline ConformanceResult testat_observable_tier(anatomy::IObservableTier& obs,
                                                              std::FILE*                report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        obs.tier_clear();
        obs.tier_observe(nullptr); // Vertrags-Randfall: nullptr ist ein No-Op, kein Absturz

        constexpr std::uint64_t kFenster1 = 400;
        (void)d::drive_ops(obs, kFenster1, /*seed=*/0xA5A5);
        anatomy::ComdareTierObserverSnapshot s1{};
        obs.tier_observe(&s1);

        // -- Schema-Wache: KEIN unbenannter Slot traegt einen Wert (je Achse EIN Fall).
        for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
            bool leer = true;
            for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f)
                if (anatomy::kV3AxisSchema[t].names[f] == nullptr && s1.axis_stats[t][f] != 0) leer = false;
            q.check(leer, "unbenannter Schema-Slot bleibt 0 (kV3AxisSchema-getrieben)");
        }

        // -- Meta-Felder: der Snapshot beschreibt DIESES Modul, nicht eine Vorlage.
        q.check(s1.observable_axis_count > 0, "observable_axis_count > 0");
        q.check(s1.tier_fill_level == obs.tier_size(), "tier_fill_level == tier_size (Meta konsistent)");
        q.check(s1.filled_axis_count > 0 && s1.filled_axis_count <= anatomy::kV3AxisCount,
                "filled_axis_count in (0, kV3AxisCount]");

        // -- Nicht-vakuose Population: mindestens eine Achse traegt reale Observer-Werte.
        std::size_t const   achsen1 = d::belegte_achsen(s1);
        std::uint64_t const summe1  = d::benannte_feld_summe(s1);
        q.note("belegte Achsen (Fenster 1)", static_cast<unsigned long long>(achsen1));
        q.note("benannte Feld-Summe (Fenster 1)", summe1);
        q.check(achsen1 > 0, "mindestens eine Achse traegt belegte benannte Felder (nicht vakuos)");
        q.check(summe1 > 0, "benannte Feld-Summe > 0 nach realen Ops");

        // -- ANTI-LEERLAUF: dreifaches Fenster -> groessere Summe.
        (void)d::drive_ops(obs, kFenster1 * 3u, /*seed=*/0x5A5A);
        anatomy::ComdareTierObserverSnapshot s2{};
        obs.tier_observe(&s2);
        std::uint64_t const summe2 = d::benannte_feld_summe(s2);
        q.note("benannte Feld-Summe (Fenster 2 = 3x Ops)", summe2);
        q.check(summe2 > summe1, "Zaehler bewegen sich REAL mit der Op-Zahl (3x Fenster > 1x Fenster)");

        // -- Pfad-B-Timing quert die Grenze mit: seg_ns wird im selben POD getragen.
        std::int64_t seg_sum = 0;
        for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) seg_sum += s2.seg_ns[t];
        q.note_i("Summe seg_ns (Pfad B, Fenster 2)", static_cast<long long>(seg_sum));
        q.check(seg_sum > 0, "Pfad-B-Segment-Timing im Snapshot > 0 (tier_observe treibt es real)");
        q.check(s2.batches_measured > 0, "batches_measured > 0 (Pfad-B-Lauf hat stattgefunden)");

        // -- tier_reset_statistics: nach dem Reset OHNE neue Ops sind die auto-gekoppelten Felder leer.
        //    Geprueft an T0 search_algo, der EINZIGEN Achse, deren Zaehler ausschliesslich aus
        //    tier_insert/tier_lookup/tier_erase stammen (Pfad-B-getriebene Achsen fuellen sich in
        //    tier_observe selbst wieder -- die duerfen hier NICHT gefordert werden, sonst pruefte das
        //    Testat eine Unwahrheit).
        obs.tier_reset_statistics();
        anatomy::ComdareTierObserverSnapshot s3{};
        obs.tier_observe(&s3);
        std::uint64_t t0_summe = 0;
        for (std::size_t f = 0; f < anatomy::kV3FieldCount; ++f)
            if (anatomy::kV3AxisSchema[0].names[f] != nullptr) t0_summe += s3.axis_stats[0][f];
        q.note("T0-Summe nach tier_reset_statistics (ohne neue Ops)", t0_summe);
        q.check(t0_summe == 0, "tier_reset_statistics nullt die auto-gekoppelte T0-Zeile");
        q.check(s3.tier_fill_level == obs.tier_size(), "Reset ist DATEN-erhaltend (Fuellstand unveraendert)");
    } catch (...) { q.check(false, "IObservableTier-Testat: unerwartete Ausnahme (ABI-noexcept-Vertrag verletzt)"); }
    return q.r;
}

// ---------------------------------------------------------------------------------------------------------------
// (2) IMeasurableWorkloadV2 -- run_workload_segmented (die 4 instrumentierten Achsen)
// ---------------------------------------------------------------------------------------------------------------
[[nodiscard]] inline ConformanceResult testat_measurable_workload_v2(anatomy::IMeasurableWorkloadV2& w,
                                                                     std::FILE* report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        constexpr std::uint64_t kOps = 256, kBatches = 8;

        // Vertrags-Randfaelle zuerst: sie duerfen nichts messen und nichts werfen.
        q.check(w.run_workload_segmented(kOps, kBatches, 1u, nullptr) == 0, "nullptr-out -> 0 Samples");
        anatomy::ComdareSegmentLatencyV1 leer{};
        q.check(w.run_workload_segmented(kOps, 0, 1u, &leer) == 0, "batches == 0 -> 0 Samples");
        q.check(leer == anatomy::ComdareSegmentLatencyV1{}, "0-Lauf laesst den POD unberuehrt (kein Phantom)");

        anatomy::ComdareSegmentLatencyV1 out{};
        std::uint64_t const              ret = w.run_workload_segmented(kOps, kBatches, /*seed=*/4242, &out);
        q.check(ret == kBatches, "Rueckgabe == angeforderte Batches");
        q.check(out.batches_measured == ret, "batches_measured == Rueckgabe (EINE Wahrheit)");
        q.check(out.batches_measured > 0, "ANTI-LEERLAUF: batches_measured > 0");

        // Die 4 instrumentierten Achsen treiben ausnahmslos reale Ops (Lookups / alloc-Churn / Feld-Scan /
        // Encode-Scan) -- keine ist eine None-Baseline. Ein 0-Segment hiesse: der Timer lief ins Leere.
        q.check(out.seg_search_algo_ns > 0, "Segment T0 search_algo > 0 ns");
        q.check(out.seg_allocator_ns > 0, "Segment T6 allocator > 0 ns");
        q.check(out.seg_memory_layout_ns > 0, "Segment T5 memory_layout > 0 ns");
        q.check(out.seg_serialization_ns > 0, "Segment T9 serialization > 0 ns");

        // Identitaet des POD: total_ns IST die Summe der vier Segmente (Konsistenz-Diagnose des Vertrags).
        std::int64_t const summe =
            out.seg_search_algo_ns + out.seg_allocator_ns + out.seg_memory_layout_ns + out.seg_serialization_ns;
        q.note_i("Summe der 4 Segmente (ns)", static_cast<long long>(summe));
        q.note_i("total_ns", static_cast<long long>(out.total_ns));
        q.check(out.total_ns == summe, "total_ns == Summe der 4 Segment-ns (exakte Identitaet)");

        // ANTI-LEERLAUF ueber die Skala: der doppelte Lauf zaehlt doppelt so viele Batches.
        anatomy::ComdareSegmentLatencyV1 out2{};
        std::uint64_t const              ret2 = w.run_workload_segmented(kOps, kBatches * 2u, 4242, &out2);
        q.check(ret2 == kBatches * 2u && out2.batches_measured == ret2,
                "doppelte Batch-Zahl -> doppelt gezaehlte Batches (Skala wirkt)");
    } catch (...) { q.check(false, "IMeasurableWorkloadV2-Testat: unerwartete Ausnahme (noexcept-Vertrag verletzt)"); }
    return q.r;
}

// ---------------------------------------------------------------------------------------------------------------
// (3) IMeasurableWorkloadV3 -- run_workload_segmented_v2 (ALLE Achsen-Segmente) + die KERN-INVARIANTE
// ---------------------------------------------------------------------------------------------------------------
//
// KERN-TESTAT: Sum(seg_ns) + seg_framework_ns == seg_run_total_ns, EXAKT.
// Die Invariante ist NICHT hier erfunden, sondern die erforschte P-MD3-Coverage-Versoehnung: seg_run_total_ns
// ist die aeussere Wall-Clock des Segment-Laufs (der kommensurable Nenner), seg_framework_ns der BENANNTE
// Rest zwischen den Segment-Timern. Bricht die Gleichung, ist Mess-Zeit verloren gegangen, ohne dass es
// jemand sieht -- genau der Defekt, den der A8-S1-Befund (seg_ns[17] fiel still in den Rest) aufgedeckt hat.
// Deshalb wird ueber kV3AxisCount summiert und NIE ueber ein Literal.
[[nodiscard]] inline ConformanceResult testat_measurable_workload_v3(anatomy::IMeasurableWorkloadV3& w,
                                                                     std::FILE* report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        constexpr std::uint64_t kOps = 256, kBatches = 8;

        q.check(w.run_workload_segmented_v2(kOps, kBatches, 1u, nullptr) == 0, "nullptr-out -> 0 Samples");
        anatomy::ComdareSegmentLatencyV2 leer{};
        q.check(w.run_workload_segmented_v2(0, kBatches, 1u, &leer) == 0, "ops_per_batch == 0 -> 0 Samples");
        q.check(leer == anatomy::ComdareSegmentLatencyV2{}, "0-Lauf laesst den POD unberuehrt (kein Phantom)");

        anatomy::ComdareSegmentLatencyV2 out{};
        std::uint64_t const              ret = w.run_workload_segmented_v2(kOps, kBatches, /*seed=*/4242, &out);
        q.check(ret == kBatches, "Rueckgabe == angeforderte Batches");
        q.check(out.batches_measured == ret, "batches_measured == Rueckgabe (EINE Wahrheit)");

        std::int64_t seg_sum = 0;
        std::size_t  belegt  = 0;
        for (std::size_t t = 0; t < anatomy::kV3AxisCount; ++t) {
            seg_sum += out.seg_ns[t];
            if (out.seg_ns[t] > 0) ++belegt;
        }
        q.note("Segmente mit ns > 0", static_cast<unsigned long long>(belegt));
        q.note_i("Summe seg_ns", static_cast<long long>(seg_sum));
        q.note_i("seg_framework_ns", static_cast<long long>(out.seg_framework_ns));
        q.note_i("seg_run_total_ns", static_cast<long long>(out.seg_run_total_ns));

        q.check(belegt > 0, "ANTI-LEERLAUF: mindestens ein Achsen-Segment traegt reale ns");
        q.check(out.total_ns == seg_sum, "total_ns == Summe seg_ns ueber kV3AxisCount (kein Literal-Rand)");
        q.check(out.seg_run_total_ns > 0, "seg_run_total_ns > 0 (aeussere Wall-Clock lief)");
        q.check(seg_sum + out.seg_framework_ns == out.seg_run_total_ns,
                "KERN: Sum(seg_ns) + seg_framework_ns == seg_run_total_ns (exakte Identitaet)");
        q.check(out.seg_framework_ns >= 0, "seg_framework_ns ist ein benannter Rest, nie negativ");
    } catch (...) { q.check(false, "IMeasurableWorkloadV3-Testat: unerwartete Ausnahme (noexcept-Vertrag verletzt)"); }
    return q.r;
}

// ---------------------------------------------------------------------------------------------------------------
// (4) IRollbackableTier -- save -> mutierende Ops -> rollback, gegen einen mitgefuehrten Zustands-Abzug
// ---------------------------------------------------------------------------------------------------------------
//
// NICHT-VAKUOS per Konstruktion: die Gegenprobe faehrt DIESELBEN mutierenden Ops OHNE rollback und fordert,
// dass der Zustand dann ABWEICHT. Ohne diese zweite Haelfte waere ein rollback, das gar nichts tut, gruen,
// sobald die Mutation zufaellig nichts aenderte.
[[nodiscard]] inline ConformanceResult testat_rollbackable_tier(anatomy::IRollbackableTier& rb,
                                                                anatomy::IDriveableTier&    drv,
                                                                std::FILE*                  report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        constexpr std::uint64_t kBasis   = 400;
        constexpr std::uint64_t kMutiere = 300;

        drv.tier_clear();
        (void)d::drive_ops(drv, kBasis, /*seed=*/0xBEEF);
        d::ZustandsAbzug const vor = d::abzug_von(drv);
        q.note("belegte Schluessel vor save", vor.belegt);
        q.check(vor.belegt > 0, "ANTI-LEERLAUF: der Vor-Zustand ist nicht leer");

        rb.tier_save_all();
        (void)d::drive_ops(drv, kMutiere, /*seed=*/0xF00D);
        d::ZustandsAbzug const mutiert = d::abzug_von(drv);
        q.check(!d::abzuege_gleich(vor, mutiert), "die mutierenden Ops haben den Zustand REAL veraendert");

        rb.tier_rollback_all();
        d::ZustandsAbzug const zurueck = d::abzug_von(drv);
        q.note("belegte Schluessel nach rollback", zurueck.belegt);
        q.check(d::abzuege_gleich(vor, zurueck), "rollback stellt JEDEN Schluessel/Wert exakt wieder her");
        q.check(zurueck.groesse == vor.groesse, "rollback stellt die Groesse exakt wieder her");

        // Idempotenz des Vertrags: ein zweiter rollback nach EINEM save rollt auf denselben Stand.
        rb.tier_rollback_all();
        q.check(d::abzuege_gleich(vor, d::abzug_von(drv)), "zweiter rollback nach EINEM save ist idempotent");

        // GEGENPROBE (macht den Vergleich nicht-vakuos): dieselbe Mutation OHNE rollback MUSS abweichen.
        (void)d::drive_ops(drv, kMutiere, /*seed=*/0xF00D);
        q.check(!d::abzuege_gleich(vor, d::abzug_von(drv)),
                "GEGENPROBE: ohne rollback weicht der Zustand ab (Vergleich ist nicht-vakuos)");
    } catch (...) { q.check(false, "IRollbackableTier-Testat: unerwartete Ausnahme (noexcept-Vertrag verletzt)"); }
    return q.r;
}

// ---------------------------------------------------------------------------------------------------------------
// (5) IMigratableTier -- tier_migrate_step
// ---------------------------------------------------------------------------------------------------------------
//
// EHRLICHKEITS-SCHNITT (Ist-Befund am Objekt, hier deklariert): ALLE benannten Kompositionen des Repos
// tragen migration_policy = NoMigration (compositions/*.hpp, ausnahmslos). An einem geladenen Modul dieser
// Bauart ist "tier_moves > 0" darum NICHT erreichbar -- eine solche Forderung waere ein Testat, das die
// Wirklichkeit nicht kennt. Geprueft wird deshalb der Vertrag, der fuer BEIDE Strategien gilt:
//   * moved <= max_moves (die Obergrenze wird eingehalten),
//   * FUNKTIONALE AEQUIVALENZ -- ein Migrations-Schritt verschiebt Records zwischen Ebenen, er darf keinen
//     einzigen verlieren oder veraendern (das ist die eigentliche Gefahr eines 2-Ebenen-Moves),
//   * und der NULLPUNKT wird als solcher benannt: bei moved == 0 muss der Zustand BYTE-gleich bleiben
//     (honest-0 der None-Strategie), bei moved > 0 muss er es EBENFALLS (dann ist der Move echt und dennoch
//     verlustfrei) -- die Aussage ist in beiden Aesten scharf.
// Die Forderung "tier_moves REAL > 0" gehoert damit an die Stelle, an der sie beweisbar ist: an einen
// aktiven Migrations-Vertreter im Biss-Testat (dort steuert die Fixture die Strategie).
[[nodiscard]] inline ConformanceResult testat_migratable_tier(anatomy::IMigratableTier& mig,
                                                              anatomy::IDriveableTier&  drv,
                                                              std::FILE*                report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        constexpr std::uint64_t kBasis    = 400;
        constexpr std::uint64_t kMaxMoves = 64;

        drv.tier_clear();
        (void)d::drive_ops(drv, kBasis, /*seed=*/0xC0FFEE);
        d::ZustandsAbzug const vor = d::abzug_von(drv);
        q.note("belegte Schluessel vor migrate", vor.belegt);
        q.check(vor.belegt > 0, "ANTI-LEERLAUF: es gibt ueberhaupt Records zu migrieren");

        std::uint64_t const moved = mig.tier_migrate_step(kMaxMoves);
        q.note("tier_migrate_step -> bewegte Records", moved);
        q.check(moved <= kMaxMoves, "moved <= max_moves (Obergrenze eingehalten)");

        d::ZustandsAbzug const nach = d::abzug_von(drv);
        q.check(d::abzuege_gleich(vor, nach), "funktionale AEQUIVALENZ: kein Record geht durch die Migration verloren");
        q.check(nach.groesse == vor.groesse, "tier_size ueber die Migration unveraendert");
        if (moved == 0) {
            q.check(nach.belegt == vor.belegt, "honest-0: ohne bewegte Records ist der Schritt ein exakter No-Op");
        } else {
            std::uint64_t const zweiter = mig.tier_migrate_step(kMaxMoves);
            q.note("zweiter Migrations-Schritt -> bewegte Records", zweiter);
            q.check(zweiter <= kMaxMoves, "zweiter Schritt haelt die Obergrenze ebenfalls ein");
            q.check(d::abzuege_gleich(vor, d::abzug_von(drv)), "auch der zweite Schritt ist verlustfrei");
        }
    } catch (...) { q.check(false, "IMigratableTier-Testat: unerwartete Ausnahme (noexcept-Vertrag verletzt)"); }
    return q.r;
}

// ---------------------------------------------------------------------------------------------------------------
// (6) IResourceControllableTier -- Caps-Quere + RC-POD-Konsum je Feld
// ---------------------------------------------------------------------------------------------------------------
//
// DER KERN ist die ADDITIVITAETS-INVARIANTE: wird jedes der fuenf RC-Felder EINZELN angewandt und werden die
// fuenf Ergebnisse addiert, muss die Summe exakt dem Ergebnis des VOLLEN POD entsprechen. Damit ist der
// per-Feld-Konsum gemessen statt behauptet -- und zwar OHNE eine einzige hartkodierte Erwartung darueber,
// welche Achse diese Komposition real annimmt (das ist komposition-abhaengig und darf es bleiben).
// Zusaetzlich der T8-Phantom-Guard: thread_count ist label-only (der In-Prozess-Tier konsumiert es nicht),
// also darf es NIE als applied gezaehlt werden -- dieselbe Aussage, die test_d13 ueber die DLL-Grenze haelt.
[[nodiscard]] inline ConformanceResult testat_resource_controllable_tier(anatomy::IResourceControllableTier& rc,
                                                                         anatomy::IObservableTier*           obs,
                                                                         std::FILE* report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        anatomy::ComdareResourceControlV1 caps{};
        rc.tier_query_resource_caps(&caps);

        std::uint64_t const cap_felder = (caps.thread_count != 0 ? 1u : 0u) + (caps.prefetch_distance != 0 ? 1u : 0u) +
                                         (caps.pool_budget_bytes != 0 ? 1u : 0u) + (caps.batch_size != 0 ? 1u : 0u) +
                                         (caps.inline_threshold_bytes != 0 ? 1u : 0u);
        q.note("Caps mit Wert != 0", cap_felder);
        q.note("controllable_axis_count", caps.controllable_axis_count);
        q.check(cap_felder > 0, "ANTI-LEERLAUF: die Caps-Quere liefert ueberhaupt steuerbare Achsen");
        q.check(caps.controllable_axis_count == cap_felder,
                "controllable_axis_count == Zahl der Caps-Felder mit Wert (Meta konsistent)");

        // Vertrags-Randfaelle.
        q.check(rc.tier_apply_resource_control(nullptr) == 0, "nullptr-in -> 0 angewandte Achsen");
        anatomy::ComdareResourceControlV1 const null_pod{};
        q.check(rc.tier_apply_resource_control(&null_pod) == 0, "Null-POD -> 0 angewandte Achsen (0 == unveraendert)");

        // Voller POD innerhalb der Caps.
        anatomy::ComdareResourceControlV1 voll{};
        voll.thread_count                = caps.thread_count;
        voll.prefetch_distance           = caps.prefetch_distance;
        voll.pool_budget_bytes           = caps.pool_budget_bytes;
        voll.batch_size                  = caps.batch_size;
        voll.inline_threshold_bytes      = caps.inline_threshold_bytes;
        std::uint64_t const applied_voll = rc.tier_apply_resource_control(&voll);
        q.note("applied (voller POD)", applied_voll);
        q.check(applied_voll > 0, "ANTI-LEERLAUF: der volle POD wird real konsumiert (applied > 0)");
        q.check(applied_voll <= caps.controllable_axis_count, "applied <= controllable_axis_count");
        q.check(rc.tier_apply_resource_control(&voll) == applied_voll, "apply ist deterministisch (gleicher POD)");

        // Je Feld EINZELN -- die gemessene Grundlage der Additivitaet.
        auto einzeln = [&rc](std::uint64_t anatomy::ComdareResourceControlV1::* feld, std::uint64_t wert) noexcept {
            anatomy::ComdareResourceControlV1 p{};
            p.*feld = wert;
            return rc.tier_apply_resource_control(&p);
        };
        std::uint64_t const a_thread = einzeln(&anatomy::ComdareResourceControlV1::thread_count, caps.thread_count);
        std::uint64_t const a_pf =
            einzeln(&anatomy::ComdareResourceControlV1::prefetch_distance, caps.prefetch_distance);
        std::uint64_t const a_pool =
            einzeln(&anatomy::ComdareResourceControlV1::pool_budget_bytes, caps.pool_budget_bytes);
        std::uint64_t const a_batch = einzeln(&anatomy::ComdareResourceControlV1::batch_size, caps.batch_size);
        std::uint64_t const a_inline =
            einzeln(&anatomy::ComdareResourceControlV1::inline_threshold_bytes, caps.inline_threshold_bytes);
        q.note("applied (nur thread_count)", a_thread);
        q.note("applied (nur prefetch_distance)", a_pf);
        q.note("applied (nur pool_budget_bytes)", a_pool);
        q.note("applied (nur batch_size)", a_batch);
        q.note("applied (nur inline_threshold_bytes)", a_inline);

        q.check(a_thread == 0, "T8-Phantom-Guard: thread_count ist label-only und zaehlt NIE als applied");
        q.check(a_batch == 1, "batch_size ist genus-invariant adapter-konsumiert (Fenster-Semantik) -> genau 1");
        q.check(a_pf <= 1 && a_pool <= 1 && a_inline <= 1, "je Einzel-Feld hoechstens eine angewandte Achse");
        q.check(a_thread + a_pf + a_pool + a_batch + a_inline == applied_voll,
                "ADDITIVITAET: Summe der Einzel-Felder == applied des vollen POD (per-Feld-Konsum gemessen)");

        // Klammer-Semantik: Werte OBERHALB der Caps werden geklammert, nicht abgelehnt.
        anatomy::ComdareResourceControlV1 ueber = voll;
        ueber.prefetch_distance                 = caps.prefetch_distance * 4u + 1u;
        ueber.batch_size                        = caps.batch_size * 4u + 1u;
        ueber.inline_threshold_bytes            = caps.inline_threshold_bytes * 4u + 1u;
        q.check(rc.tier_apply_resource_control(&ueber) == applied_voll,
                "ueber-Cap-Werte werden GEKLAMMERT, nicht abgewiesen (applied unveraendert)");

        // Anti-Leerlauf ueber das Verhalten: nach der angewandten Steuerung liefert der Observer weiterhin
        // reale Zaehler -- die Steuerung legt das Modul nicht still.
        if (obs != nullptr) {
            obs->tier_clear();
            (void)d::drive_ops(*obs, 400, /*seed=*/0x1234);
            anatomy::ComdareTierObserverSnapshot s{};
            obs->tier_observe(&s);
            std::uint64_t const summe = d::benannte_feld_summe(s);
            q.note("benannte Feld-Summe unter angewandter Steuerung", summe);
            q.check(summe > 0, "unter angewandter RC-Steuerung bewegen sich die Observer-Zaehler weiterhin");
        }
    } catch (...) {
        q.check(false, "IResourceControllableTier-Testat: unerwartete Ausnahme (noexcept-Vertrag verletzt)");
    }
    return q.r;
}

// ---------------------------------------------------------------------------------------------------------------
// (7) IAllocatorProxyTier -- tier_get_allocator (non-dagger Proxy)
// ---------------------------------------------------------------------------------------------------------------
//
// Bis W3 gab es hier NUR probe_allocator_proxy: eine REPORT-Probe ganz ohne Zusicherung (conformance_gate.hpp).
// Dieses Testat WIEDERVERWENDET die Probe (kein Duplikat des Flag-Wissens) und macht aus dem Bericht eine
// ASSERTION: Interface vorhanden, Identitaet vorhanden, Format-Version korrekt -- und, wo die Statistik-Route
// deklariert ist, bewegen sich ihre Zaehler unter Last. Ist sie NICHT deklariert, wird der Nullpunkt geprueft
// (Zaehler exakt 0) statt uebersprungen: eine geloeschte Route, die trotzdem Zahlen meldet, waere Phantom.
[[nodiscard]] inline ConformanceResult testat_allocator_proxy_tier(anatomy::IDriveableTier& drv,
                                                                   std::FILE*               report = nullptr) noexcept {
    namespace d = mess_testat_detail;
    d::Quote q{};
    q.report = report;
    try {
        drv.tier_clear();
        (void)d::drive_ops(drv, 200, /*seed=*/0x2468);
        AllocatorProxyProbeResult const p1 = probe_allocator_proxy(drv);

        q.check(p1.interface_present, "IAllocatorProxyTier ist am geladenen Modul vorhanden (ASSERTIERT)");
        q.check(p1.identity_present, "identity_present-Flag gesetzt (die Achsen-Identitaet ist ehrlich belegt)");
        q.check(p1.proxy.format_version == anatomy::kAllocatorProxyFormatVersion,
                "format_version == kAllocatorProxyFormatVersion");
        q.note("allocator_family_id", p1.proxy.allocator_family_id);
        q.note("flags", p1.proxy.flags);
        q.note("stats_route_present", p1.stats_route_present ? 1u : 0u);
        q.note("total_bytes_allocated (Last 1)", p1.proxy.total_bytes_allocated);
        q.note("allocation_count (Last 1)", p1.proxy.allocation_count);

        // ANTI-LEERLAUF unter zusaetzlicher Last.
        (void)d::drive_ops(drv, 1200, /*seed=*/0x1357);
        AllocatorProxyProbeResult const p2 = probe_allocator_proxy(drv);
        q.check(p2.interface_present && p2.stats_route_present == p1.stats_route_present,
                "die Proxy-Deklaration ist ueber die Last stabil (kein Flag-Flackern)");
        q.note("total_bytes_allocated (Last 2)", p2.proxy.total_bytes_allocated);
        q.note("allocation_count (Last 2)", p2.proxy.allocation_count);

        if (p1.stats_route_present) {
            bool const bewegt = (p2.proxy.allocation_count > p1.proxy.allocation_count) ||
                                (p2.proxy.total_bytes_allocated > p1.proxy.total_bytes_allocated) ||
                                (p2.proxy.live_nodes != p1.proxy.live_nodes);
            q.check(bewegt, "stats_route deklariert -> die Zaehler bewegen sich unter Last (nicht eingefroren)");
        } else {
            // honest-0: die Route ist NICHT deklariert -- dann darf sie auch keine Zahlen liefern.
            bool const still = p2.proxy.total_bytes_allocated == 0 && p2.proxy.total_bytes_in_use == 0 &&
                               p2.proxy.allocation_count == 0 && p2.proxy.live_nodes == 0;
            q.check(still, "honest-0: ohne stats_route bleiben ALLE Statistik-Felder exakt 0 (kein Phantom)");
        }
    } catch (...) { q.check(false, "IAllocatorProxyTier-Testat: unerwartete Ausnahme (noexcept-Vertrag verletzt)"); }
    return q.r;
}

} // namespace comdare::cache_engine::builder::pruef_dock
