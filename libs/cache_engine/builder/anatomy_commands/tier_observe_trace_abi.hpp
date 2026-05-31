#pragma once
// V41.F.6.1.R6 Inkrement 2 — TierObserveTraceAbi: host-seitiger Fuellstands-Mess-Treiber ueber das
// ABI-stabile IObservableTier (Pfad B ueber die Modul-Binary-Grenze, Doku 24 §8.6/§8.7).
//
// Generalisiert drive_tier_observe_trace (in-process, AnatomyExecutionContext) auf das ABI-Interface:
// der host-seitige CacheEngineBuilder treibt das (in-process ODER als .dll geladene) Tier-Modul
// AUSSCHLIESSLICH ueber IObservableTier und erhebt pro Fuellstand-Checkpoint
//   (b) Tier-Wall-Clock — Roh-Samples GETRENNT nach read/write/delete (Doku 24 §2.1), UND
//   (a) den Observer-POD via tier_observe (Doku 24 §2.2), Wall-Clock-KORRELIERT (§8.7, Trigger-Modus
//       Zustands-Manipulation: ein Snapshot je Checkpoint).
// Damit zieht der Builder die IM Tier eingebauten Observer durch die Schnittstelle + kann sie (mit dem
// korrelierten Wall-Clock-Kontext) persistieren — ohne den konkreten Composition-Typ zu kennen (nur das
// Gattungs-ABI). KEIN Runtime-Switch; reine Interface-Indirektion.

#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp>   // V5-I7: Zwei-Phasen-Treiber (tier_save_all/tier_rollback_all)

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace an = ::comdare::cache_engine::anatomy;

/// Konfiguration des ABI-Fuellstands-Treibers (deterministisch via seed).
struct AbiTierTraceConfig {
    std::vector<std::uint64_t> fill_checkpoints{10, 100, 1000};   // Element-Fuellstaende (Kurven-Stuetzpunkte)
    std::uint64_t lookups_per_checkpoint = 2000;
    std::uint64_t deletes_per_checkpoint = 200;                   // erase+reinsert → Fuellstand stabil
    std::uint64_t seed = 11;
    // Robustheit (2026-05-31): max. aufeinanderfolgende tier_insert-Versuche OHNE Fuellstand-Zuwachs, bevor
    // die WRITE-Phase abbricht. Ohne diese Schranke laeuft `while (tier_size() < target)` ENDLOS, sobald
    // tier_insert den Fuellstand nicht mehr erhoeht (Fixed-Capacity-Store voll / Key-Kollision) — ein Fill
    // ueber die effektive Tier-Kapazitaet wuerde sonst haengen (entdeckt via f15_compare --observe).
    std::uint64_t max_insert_stagnation = 4096;
};

/// Eine Fuellstands-Stufe: r/w/d-Wall-Clock-Roh-Samples + der Wall-Clock-korrelierte Observer-POD.
struct AbiFillLevelSnapshot {
    std::uint64_t             fill_level = 0;     // = tier.tier_size() am Checkpoint
    std::vector<std::int64_t> read_ns{};          // Tier-Wall-Clock je Operation, GETRENNT (§2.1)
    std::vector<std::int64_t> write_ns{};
    std::vector<std::int64_t> delete_ns{};
    std::uint64_t             read_sink = 0;       // Anti-Wegoptimierungs-Senke
    an::ComdareTierObserverSnapshotV1 observer{};  // §8.7: EIN Observer-POD je Checkpoint, korreliert
    std::int64_t              observe_wall_ns = 0;  // §8.7: Wall-Clock-Zeitstempel (relativ zum Trace-Start)
                                                    // im Moment des tier_observe → explizite (t ↔ Observer)-Korrelation
};

/// Tier-Mess-Trace ueber das ABI-Interface (Akkumulation von Fuellstands-Stufen).
struct AbiTierObserveTrace {
    std::vector<AbiFillLevelSnapshot> checkpoints{};
};

namespace detail {
[[nodiscard]] inline std::int64_t abi_dur_ns(std::chrono::steady_clock::time_point a,
                                             std::chrono::steady_clock::time_point b) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
}

/// Nearest-Rank-Perzentil (p ∈ [0,1]) über eine Roh-ns-Stichprobe (Kopie + sort; leere Stichprobe → 0).
[[nodiscard]] inline std::int64_t nearest_rank_p(std::vector<std::int64_t> v, double p) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    std::size_t rank = static_cast<std::size_t>(p * static_cast<double>(v.size() - 1) + 0.5);
    if (rank >= v.size()) rank = v.size() - 1;
    return v[rank];
}

/// V5-I7 Zwei-Phasen-Messung EINER Operation (User-Direktive 2026-05-31, Mess-Architektur §4):
///   save-all → op (Phase 1: Warmup, Cache/Branch-Predictor heizen, Timing VERWORFEN)
///   → rollback-all (Vor-Zustand exakt wiederhergestellt)
///   → op (Phase 2: Messung — warm, aber logisch gegen DENSELBEN Vor-Zustand wie der Warmup).
/// `timed_op()` führt die Operation aus UND liefert ihre Wall-Clock-ns zurück; wird bei verfügbarem
/// Rollback ZWEIMAL gerufen (Warmup verworfen, Messung behalten), sonst EINMAL (Kalt-Messung, Fallback
/// für alte Module / nicht-kopierbare Organe). Da jede Phase 1 exakt zurückgerollt wird, macht NUR die
/// gemessene Phase-2-Op logischen Fortschritt ⇒ End-Zustand + Observer-Zähler identisch zur Einphasen-Messung.
/// V5-Audit-Härtung: EMPIRISCHE Rollback-Exaktheits-Probe über das ABI (KEINE Interface-Erweiterung nötig).
/// save-all(leer) → Mutation → rollback-all → ist der leere Zustand exakt wiederhergestellt? Adressiert die
/// Audit-Lücke „two_phase_measure prüft nur rb!=nullptr, nicht Exaktheit": nicht-exakt-rollbackbare Organe
/// (z.B. nicht-kopierbare, deren save/rollback zu no-op degradiert) werden so erkannt → Kalt-Messung statt
/// stiller Warmup-Verfälschung. Läuft EINMAL je Tier VOR der Messung; lässt den Tier geleert zurück.
[[nodiscard]] inline bool
rollback_is_empirically_exact(::comdare::cache_engine::anatomy::IObservableTier& tier,
                              ::comdare::cache_engine::anatomy::IRollbackableTier* rb) noexcept {
    if (rb == nullptr) return false;
    tier.tier_clear();
    rb->tier_save_all();                          // Memento des LEEREN Zustands
    (void)tier.tier_insert(0xDEADBEEFu, 1u);      // Mutation (insert/peak werden bei exaktem Rollback zurückgerollt)
    bool const mutated = (tier.tier_size() == 1); // Mutation hat gewirkt
    rb->tier_rollback_all();                       // muss den leeren Zustand exakt zurückrollen
    bool const exact = mutated && (tier.tier_size() == 0);   // NUR tier_size() — KEIN tier_lookup (das würde
                                                  // lk_/miss_-Observer-Stats außerhalb des Mementos erhöhen).
    tier.tier_clear();                             // Aufräumen (falls nicht exakt: Probe-Key entfernen)
    return exact;
}

template <class TimedOp>
[[nodiscard]] std::int64_t two_phase_measure(::comdare::cache_engine::anatomy::IRollbackableTier* rb,
                                             TimedOp&& timed_op) {
    if (rb != nullptr) {
        rb->tier_save_all();
        (void)timed_op();        // Phase 1: Warmup (verworfen)
        rb->tier_rollback_all();  // Vor-Zustand exakt zurück
    }
    return timed_op();           // Phase 2: Messung (bzw. Kalt-Messung wenn rb==nullptr)
}
}  // namespace detail

/// Treibt ein IObservableTier ueber die Fuellstands-Checkpoints (Pfad B; Trigger Zustands-Manipulation
/// §8.7b) + erhebt pro Checkpoint r/w/d-Wall-Clock + tier_observe-POD (Wall-Clock-korreliert). Der
/// uebergebene Tier wird VOR jedem Lauf NICHT geleert (der Aufrufer steuert den Start-Zustand); die
/// Checkpoints muessen monoton steigen.
[[nodiscard]] inline AbiTierObserveTrace
drive_tier_observe_trace_abi(an::IObservableTier& tier, AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    std::mt19937_64 rng{cfg.seed};
    std::uint64_t next_key = 0;
    AbiTierObserveTrace trace;
    auto const trace_start = clock::now();   // §8.7: Nullpunkt der Wall-Clock-Korrelations-Achse

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        AbiFillLevelSnapshot snap;

        // WRITE-Phase: bis Fuellstand == target; jede tier_insert() Wall-Clock-umklammert.
        // Robustheit (2026-05-31): bricht ab, wenn der Fuellstand ueber max_insert_stagnation Versuche NICHT
        // mehr waechst (Fixed-Capacity-Store voll / Key-Kollision) — sonst Endlosschleife bei target > Kapazitaet.
        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill = tier.tier_size();
        while (tier.tier_size() < target) {
            auto const t0 = clock::now();
            (void)tier.tier_insert(next_key, next_key * 2u + 1u);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));
            ++next_key;
            std::uint64_t const cur_fill = tier.tier_size();
            if (cur_fill > last_fill) { last_fill = cur_fill; write_stagnation = 0; }
            else if (++write_stagnation >= cfg.max_insert_stagnation) break;  // effektive Tier-Kapazitaet erreicht
        }

        // READ-Phase: deterministische ~50%-Hit/Miss-Lookups ueber das Interface.
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const k = (next_key != 0) ? (rng() % (next_key * 2u)) : 0u;
            std::uint64_t out = 0;
            auto const t0 = clock::now();
            bool const hit = tier.tier_lookup(k, &out);
            auto const t1 = clock::now();
            snap.read_sink += hit ? out : 0u;
            snap.read_ns.push_back(detail::abi_dur_ns(t0, t1));
        }

        // DELETE-Phase: erase + reinsert → Fuellstand bleibt == target.
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && tier.tier_size() > 0; ++i) {
            std::uint64_t const dk = (next_key != 0) ? (rng() % next_key) : 0u;
            auto const t0 = clock::now();
            bool const ok = tier.tier_erase(dk);
            auto const t1 = clock::now();
            snap.delete_ns.push_back(detail::abi_dur_ns(t0, t1));
            if (ok) (void)tier.tier_insert(dk, dk * 2u + 1u);   // Fuellstand wiederherstellen
        }

        snap.fill_level = tier.tier_size();
        // §8.7: EIN Observer-POD am Checkpoint, mit explizitem Wall-Clock-Zeitstempel korreliert (der
        // Builder persistiert die (Wall-Clock ↔ Observer)-Zuordnung).
        tier.tier_observe(&snap.observer);
        snap.observe_wall_ns = detail::abi_dur_ns(trace_start, clock::now());
        trace.checkpoints.push_back(std::move(snap));
    }
    return trace;
}

/// V5-I7 — ZWEI-PHASEN-Treiber (User-Direktive 2026-05-31, Mess-Architektur §4): identisch zur Füllstands-
/// Trajektorie von drive_tier_observe_trace_abi, ABER jede GEMESSENE Operation läuft als zwei-phasiger Op
/// (save-all → op-warmup [verworfen] → rollback-all → op-measure), sofern `rollback != nullptr`. Dadurch wird
/// jeder gemessene Op warm (Cache/Branch-Predictor geheizt durch den Warmup), aber logisch gegen DENSELBEN
/// Vor-Zustand gemessen wie der Warmup. Da jede Warmup-Phase exakt zurückgerollt wird, macht nur die Mess-Op
/// logischen Fortschritt ⇒ End-Füllstand + Observer-Zähler IDENTISCH zu drive_tier_observe_trace_abi.
///
/// `rollback`: das memento_all-Sub-Interface der GLEICHEN Tier-Instanz (Loader-`dynamic_cast`; nullptr ⇒ altes
/// Modul / nicht-exakt-rollbackbares Organ ⇒ graziöser Fallback auf Einphasen-Kalt-Messung pro Op). Die
/// nicht-gemessenen Zustands-Operationen (DELETE-reinsert) bleiben einphasig (reine Füllstand-Restauration).
[[nodiscard]] inline AbiTierObserveTrace
drive_two_phase_tier_trace_abi(an::IObservableTier& tier,
                               an::IRollbackableTier* rollback,
                               AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    // V5-Audit-Härtung: nur zwei-phasig messen, wenn der Rollback EMPIRISCH exakt ist (sonst stille
    // Warmup-Verfälschung). Nicht-exakt → nullptr → Einphasen-Kalt-Messung pro Op.
    if (rollback != nullptr && !detail::rollback_is_empirically_exact(tier, rollback)) rollback = nullptr;
    std::mt19937_64 rng{cfg.seed};
    std::uint64_t next_key = 0;
    AbiTierObserveTrace trace;
    auto const trace_start = clock::now();

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        AbiFillLevelSnapshot snap;

        // WRITE-Phase zwei-phasig: Warmup-insert(k) → rollback (k weg) → Mess-insert(k) (k da, echter Fortschritt).
        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill = tier.tier_size();
        while (tier.tier_size() < target) {
            std::uint64_t const k = next_key;
            auto const ns = detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                auto const t0 = clock::now();
                (void)tier.tier_insert(k, k * 2u + 1u);
                auto const t1 = clock::now();
                return detail::abi_dur_ns(t0, t1);
            });
            snap.write_ns.push_back(ns);
            ++next_key;
            std::uint64_t const cur_fill = tier.tier_size();
            if (cur_fill > last_fill) { last_fill = cur_fill; write_stagnation = 0; }
            else if (++write_stagnation >= cfg.max_insert_stagnation) break;
        }

        // READ-Phase zwei-phasig: Warmup-lookup heizt, rollback restauriert Observer-Stats, Mess-lookup zählt.
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const k = (next_key != 0) ? (rng() % (next_key * 2u)) : 0u;
            std::uint64_t measured_out = 0;
            bool          measured_hit = false;
            auto const ns = detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                std::uint64_t out = 0;
                auto const t0 = clock::now();
                bool const hit = tier.tier_lookup(k, &out);
                auto const t1 = clock::now();
                measured_hit = hit; measured_out = out;   // nach two_phase_measure spiegelt dies die MESS-Phase
                return detail::abi_dur_ns(t0, t1);
            });
            snap.read_sink += measured_hit ? measured_out : 0u;
            snap.read_ns.push_back(ns);
        }

        // DELETE-Phase zwei-phasig (Mess-Op = erase); die Füllstand-Restauration (reinsert) ist NICHT gemessen.
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && tier.tier_size() > 0; ++i) {
            std::uint64_t const dk = (next_key != 0) ? (rng() % next_key) : 0u;
            bool measured_ok = false;
            auto const ns = detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                auto const t0 = clock::now();
                bool const ok = tier.tier_erase(dk);
                auto const t1 = clock::now();
                measured_ok = ok;
                return detail::abi_dur_ns(t0, t1);
            });
            snap.delete_ns.push_back(ns);
            if (measured_ok) (void)tier.tier_insert(dk, dk * 2u + 1u);   // Füllstand wiederherstellen (einphasig)
        }

        snap.fill_level = tier.tier_size();
        tier.tier_observe(&snap.observer);
        snap.observe_wall_ns = detail::abi_dur_ns(trace_start, clock::now());
        trace.checkpoints.push_back(std::move(snap));
    }
    return trace;
}

/// Persistiert den Pfad-B-Trace als CSV (Doku 24 §8.6 Schritt 6: der Builder persistiert die korrelierten
/// (Wall-Clock ↔ Observer)-Messergebnisse). Eine Zeile je Füllstand-Checkpoint; der Wall-Clock-Zeitstempel
/// (observe_wall_ns) korreliert die Per-Achsen-Observer-Zähler mit der Latenz-Phase. Header + r/w/d-Sample-
/// Zahlen (die Roh-ns-Kurven bleiben im Trace für eine separate Perzentil-Auswertung).
[[nodiscard]] inline std::string serialize_abi_tier_trace_csv(AbiTierObserveTrace const& trace) {
    std::ostringstream os;
    os << "checkpoint,observe_wall_ns,fill_level,write_samples,read_samples,delete_samples,"
          "search_insert,search_lookup,search_hit,search_miss,search_erase,search_peak_occupancy,"
          "alloc_bytes_in_use,alloc_alloc_count,observable_axes\n";
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        os << i << ',' << cp.observe_wall_ns << ',' << cp.fill_level << ','
           << cp.write_ns.size() << ',' << cp.read_ns.size() << ',' << cp.delete_ns.size() << ','
           << o.search_insert_count << ',' << o.search_lookup_count << ',' << o.search_hit_count << ','
           << o.search_miss_count << ',' << o.search_erase_count << ',' << o.search_peak_occupancy << ','
           << o.alloc_bytes_in_use << ',' << o.alloc_allocation_count << ',' << o.observable_axis_count << '\n';
    }
    return os.str();
}

/// Persistiert den Pfad-B-Trace als JSON-Array (eine Objekt-Zeile je Checkpoint). Zusätzlich zur CSV trägt
/// die JSON die **Perzentile** (p50/p99) der r/w/d-Roh-ns-Kurven — die Tier-Wall-Clock-Detail-Auswertung
/// (Doku 24 §2.1) korreliert mit den Observer-Zählern. Robust gegen Wall-Clock-Ausreisser (p50, vgl. Doku 22 §3.3).
[[nodiscard]] inline std::string serialize_abi_tier_trace_json(AbiTierObserveTrace const& trace) {
    std::ostringstream os;
    os << '[';
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        if (i != 0) os << ',';
        os << "{\"checkpoint\":" << i
           << ",\"observe_wall_ns\":" << cp.observe_wall_ns
           << ",\"fill_level\":" << cp.fill_level
           << ",\"write_p50_ns\":"  << detail::nearest_rank_p(cp.write_ns,  0.5)
           << ",\"write_p99_ns\":"  << detail::nearest_rank_p(cp.write_ns,  0.99)
           << ",\"read_p50_ns\":"   << detail::nearest_rank_p(cp.read_ns,   0.5)
           << ",\"read_p99_ns\":"   << detail::nearest_rank_p(cp.read_ns,   0.99)
           << ",\"delete_p50_ns\":" << detail::nearest_rank_p(cp.delete_ns, 0.5)
           << ",\"search_insert\":" << o.search_insert_count
           << ",\"search_lookup\":" << o.search_lookup_count
           << ",\"search_hit\":"    << o.search_hit_count
           << ",\"search_miss\":"   << o.search_miss_count
           << ",\"search_peak_occupancy\":" << o.search_peak_occupancy
           << ",\"alloc_bytes_in_use\":"    << o.alloc_bytes_in_use
           << ",\"observable_axes\":"       << o.observable_axis_count << '}';
    }
    os << ']';
    return os.str();
}

}  // namespace comdare::cache_engine::builder::anatomy_commands
