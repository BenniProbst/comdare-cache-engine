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

#include <chrono>
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
        while (tier.tier_size() < target) {
            auto const t0 = clock::now();
            (void)tier.tier_insert(next_key, next_key * 2u + 1u);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));
            ++next_key;
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

}  // namespace comdare::cache_engine::builder::anatomy_commands
