#pragma once
// V41 Roadmap-3 (Doku 24 §2.1/§2.2/§5.3) — TierObserveTrace: Builder-seitiger Fuellstands-Mess-Treiber.
//
// Vollendet Saeule 2 IN-PROCESS (die §5.2-Luecke ist Builder-seitig bereits geschlossen, Roadmap-1;
// observe_all durch die .dll-Grenze bleibt R6). Treibt den AnatomyExecutionContext<Composition> ueber
// einen Fuellstands-Workload und erhebt pro Checkpoint BEIDE Mess-Dimensionen aus EINEM Lauf:
//   (a) Achsen-Observer (Doku 24 §2.2): observe_all() → search_algo + allocator REAL (search_algo_at_checkpoint
//       + allocator_at_checkpoint), plus RAM-Proxy = allocator.total_bytes_in_use (portabel, KEINE OS-API).
//   (b) Tier-Wall-Clock (Doku 24 §2.1): Latenz als AKKUMULATION von Detail-Kurven — Roh-Samples pro Operation
//       GETRENNT nach read/write/delete, ueber den Element-Fuellstand (Checkpoints 10/100/1000).
// Auswertung p50/p99 ueber das bestehende builder::commands::stats::percentile_ns (kein neuer Perzentil-Code).
//
// KEIN Runtime-Switch (Praeprozessor + Templates); STATISTICS-abhaengige Felder unter #ifdef.

#include "anatomy_execution_context.hpp"
#include <anatomy/composition_concept.hpp>
#ifdef COMDARE_CE_ENABLE_STATISTICS
#include <topics/traversal/axis_03a_search_algo/concepts/axis_03a_search_algo_cache_engine_permutation_concept.hpp>  // SearchAlgoStatistics
#include <topics/allocator/axis_06_allocator/concepts/axis_06_allocator_cache_engine_permutation_concept.hpp>        // AllocationStatistics
#endif

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

namespace comdare::cache_engine::builder::anatomy_commands {

namespace ana = ::comdare::cache_engine::anatomy;

enum class TierOpType { Read, Write, Delete };

/// Konfiguration des Fuellstands-Treibers (deterministisch via seed).
struct TierTraceConfig {
    std::vector<std::size_t> fill_checkpoints{10, 100, 1000};   // Element-Fuellstaende (Kurven-Stuetzpunkte)
    std::uint64_t lookups_per_checkpoint = 2000;
    std::uint64_t deletes_per_checkpoint = 200;                 // erase+reinsert → Fuellstand stabil
    std::uint64_t seed = 11;
};

/// Eine Fuellstands-Stufe: Tier-Wall-Clock (r/w/d getrennt) + Achsen-Observer-Snapshot + RAM-Proxy.
struct FillLevelSnapshot {
    std::size_t fill_level  = 0;       // = ctx.size() am Checkpoint
    double      load_factor = 0.0;     // fill_level / groesster Checkpoint
    // (b) Tier-Wall-Clock — Roh-Samples (ns) je Operation, GETRENNT (p50/p99 via stats::percentile_ns).
    std::vector<std::int64_t> read_ns{};
    std::vector<std::int64_t> write_ns{};
    std::vector<std::int64_t> delete_ns{};
    std::uint64_t read_sink = 0;       // Anti-Wegoptimierungs-Senke fuer lookup-Resultate
    // (a) Achsen-Observer + RAM-Proxy.
    std::uint64_t ram_bytes_in_use = 0;
#ifdef COMDARE_CE_ENABLE_STATISTICS
    ::comdare::cache_engine::traversal::axis_03a_search_algo::concepts::SearchAlgoStatistics search_algo_at_checkpoint{};
    ::comdare::cache_engine::allocator::axis_06_allocator::concepts::AllocationStatistics    allocator_at_checkpoint{};
#endif
};

/// Tier-Mess-Trace eines ganzen Suchalgorithmus (Composition) als Akkumulation von Fuellstands-Stufen.
struct TierObserveTrace {
    std::string_view               composition_name{};
    std::vector<FillLevelSnapshot> checkpoints{};
};

namespace detail {
[[nodiscard]] inline std::int64_t tier_dur_ns(std::chrono::steady_clock::time_point a,
                                              std::chrono::steady_clock::time_point b) noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count();
}
}  // namespace detail

/// Treibt den AnatomyExecutionContext<Composition> ueber die Fuellstands-Checkpoints und erhebt pro
/// Checkpoint Tier-Wall-Clock (r/w/d) + observe_all-Trace (search_algo+allocator) + RAM. In-Process.
template <ana::IsComposition Composition>
[[nodiscard]] TierObserveTrace drive_tier_observe_trace(TierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    AnatomyExecutionContext<Composition> ctx;
    std::mt19937_64 rng{cfg.seed};
    std::uint64_t next_key = 0;

    TierObserveTrace trace;
    trace.composition_name = AnatomyExecutionContext<Composition>::composition_name();
    std::size_t const cap = cfg.fill_checkpoints.empty() ? std::size_t{1} : cfg.fill_checkpoints.back();

    for (std::size_t const target : cfg.fill_checkpoints) {
        FillLevelSnapshot snap;

        // WRITE-Phase: bis Fuellstand == target; jede insert() Wall-Clock-umklammert.
        while (ctx.size() < target) {
            auto const t0 = clock::now();
            ctx.insert(next_key, next_key * 2u + 1u);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::tier_dur_ns(t0, t1));
            ++next_key;
        }

        // READ-Phase: deterministische ~50%-Hit/Miss-Lookups; Resultat in read_sink akkumuliert.
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const k = (next_key != 0) ? (rng() % (next_key * 2u)) : 0u;
            auto const t0 = clock::now();
            auto const v  = ctx.lookup(k);
            auto const t1 = clock::now();
            snap.read_sink += v.has_value() ? *v : 0u;
            snap.read_ns.push_back(detail::tier_dur_ns(t0, t1));
        }

        // DELETE-Phase: erase + reinsert → Fuellstand bleibt == target.
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && ctx.size() > 0; ++i) {
            std::uint64_t const dk = (next_key != 0) ? (rng() % next_key) : 0u;
            auto const t0 = clock::now();
            bool const ok = ctx.erase(dk);
            auto const t1 = clock::now();
            snap.delete_ns.push_back(detail::tier_dur_ns(t0, t1));
            if (ok) ctx.insert(dk, dk * 2u + 1u);   // Fuellstand wiederherstellen
        }

        snap.fill_level  = ctx.size();
        snap.load_factor = static_cast<double>(snap.fill_level) / static_cast<double>(cap);
#ifdef COMDARE_CE_ENABLE_STATISTICS
        // (a) EIN observe_all() am Checkpoint → search_algo + allocator REAL (R5B_ObserveMultiAxes-Muster).
        auto const agg = ctx.observe_all();
        snap.search_algo_at_checkpoint = agg.search_algo;
        snap.allocator_at_checkpoint   = agg.allocator;
        snap.ram_bytes_in_use          = agg.allocator.total_bytes_in_use;   // portabler RAM-Proxy
#endif
        trace.checkpoints.push_back(std::move(snap));
    }
    return trace;
}

}  // namespace comdare::cache_engine::builder::anatomy_commands
