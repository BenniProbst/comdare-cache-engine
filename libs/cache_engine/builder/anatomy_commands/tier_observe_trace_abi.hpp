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

// D5-4: die EINE Feldliste der Trace-Ausgabe. Sie zieht den D5-1-Perzentil-Kanon
// (builder/commands/latency_stats.hpp) selbst mit -- diese Datei rechnet kein Perzentil mehr und
// braucht latency_stats deshalb nicht mehr direkt.
#include "tier_trace_schema.hpp"

#include <anatomy/observable_tier.hpp>
#include <anatomy/rollbackable_tier.hpp> // V5-I7: Zwei-Phasen-Treiber (tier_save_all/tier_rollback_all)

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
// D5-4: der Alias `st` (= builder::commands::stats) stand hier, solange die Serialisierer die
// Perzentile SELBST riefen. Sie tun es nicht mehr -- der Alias ist mit dem Feldblock gegangen, damit
// niemand ihn versehentlich fuer eine zehnte, hausgemachte Kennzahl wiederbelebt.
namespace ts = trace_schema; // D5-4 Feldliste + geteilte Serialisierer

/// Konfiguration des ABI-Fuellstands-Treibers (deterministisch via seed).
struct AbiTierTraceConfig {
    std::vector<std::uint64_t> fill_checkpoints{10, 100, 1000}; // Element-Fuellstaende (Kurven-Stuetzpunkte)
    std::uint64_t              lookups_per_checkpoint = 2000;
    std::uint64_t              deletes_per_checkpoint = 200; // erase+reinsert → Fuellstand stabil
    std::uint64_t              seed                   = 11;
    // Robustheit (2026-05-31): max. aufeinanderfolgende tier_insert-Versuche OHNE Fuellstand-Zuwachs, bevor
    // die WRITE-Phase abbricht. Ohne diese Schranke laeuft `while (tier_size() < target)` ENDLOS, sobald
    // tier_insert den Fuellstand nicht mehr erhoeht (Fixed-Capacity-Store voll / Key-Kollision) — ein Fill
    // ueber die effektive Tier-Kapazitaet wuerde sonst haengen (entdeckt via f15_compare --observe).
    std::uint64_t max_insert_stagnation = 4096;
    // NAHT-1 (Owner-KERN 09.08.2026): was die BAU-SEITE ueber die Mess-Ausstattung des Moduls
    // BEHAUPTET -- die Host-Vorhersage des neunten Preimage-Glieds (mess_gates_glied_for_legend,
    // profile_facade/mess_achsen_naht.hpp). Sie ist die eine Haelfte des Widerspruchs-Befunds; die
    // andere ist die Probe am geladenen Modul.
    //   -1 = unbekannt (nichts wird behauptet -> ein Widerspruch kann nicht festgestellt werden)
    //    0 = Legende sagt: dieses Modul hat KEINE Messung eingebaut
    //    1 = Legende sagt: dieses Modul HAT Messung eingebaut
    // OFFEN, ausdruecklich: die automatische Ableitung dieses Werts aus dem Modul-Stempel ist NICHT
    // verdrahtet -- der Aufrufer setzt ihn. Solange er -1 bleibt, verhaelt sich das Dock exakt wie
    // vorher (Status 3 bei fehlender Flaeche). Eine ehrliche Wissensluecke, kein stiller Default.
    int mess_legende_erwartung = -1;
};

/// Eine Fuellstands-Stufe: r/w/d-Wall-Clock-Roh-Samples + der Wall-Clock-korrelierte Observer-POD.
struct AbiFillLevelSnapshot {
    std::uint64_t                   fill_level = 0; // = tier.tier_size() am Checkpoint
    std::vector<std::int64_t>       read_ns{};      // Tier-Wall-Clock je Operation, GETRENNT (§2.1)
    std::vector<std::int64_t>       write_ns{};
    std::vector<std::int64_t>       delete_ns{};
    std::uint64_t                   read_sink = 0; // Anti-Wegoptimierungs-Senke
    an::ComdareTierObserverSnapshot observer{}; // §8.7: EIN konsolidierter Observer-POD je Checkpoint, korreliert (I1)
    std::int64_t                    observe_wall_ns = 0; // §8.7: Wall-Clock-Zeitstempel (relativ zum Trace-Start)
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

// SELBSTCHECK (D5-1, ergaenzt D5-4, beide 2026-08-09)
//   ZUSICHERT: in dieser Datei existiert KEINE Perzentil-Formel mehr -- und seit D5-4 auch keine
//              Feldliste. Die p-Felder ihrer Serialisierer kommen ueber trace_schema::kLatenzFelder
//              und rechnen dort mit stats::percentile_ns.
//   ZUSICHERT NICHT: dass Altdaten weiter gelten -- sie tun es nicht (andere Definition).
//
// D5-1 (2026-08-09): hier stand `detail::nearest_rank_p` -- eine ZWEITE Perzentil-Formel
// (rank = round(p*(n-1))), die den Namen "Nearest-Rank" zu Unrecht trug und bei 1..100 p50=51
// lieferte. Sie ist ERSATZLOS geloescht, ohne Alias und ohne Weiterleitung: jede uebersehene
// Aufrufstelle sollte compile-time brechen, nicht still weiterrechnen. Der EINE Kanon steht in
// builder/commands/latency_stats.hpp (stats::nearest_rank_index / stats::percentile_ns).

/// V5-I7 Zwei-Phasen-Messung EINER Operation (User-Direktive 2026-05-31, Mess-Architektur §4):
///   save-all → op (Phase 1: Warmup, Cache/Branch-Predictor heizen, Timing VERWORFEN)
///   → rollback-all (Vor-Zustand exakt wiederhergestellt)
///   → op (Phase 2: Messung — warm, aber logisch gegen DENSELBEN Vor-Zustand wie der Warmup).
/// `timed_op()` führt die Operation aus UND liefert ihre Wall-Clock-ns zurück; wird bei verfügbarem
/// Rollback ZWEIMAL gerufen (Warmup verworfen, Messung behalten), sonst EINMAL (Kalt-Messung, Fallback
/// für alte Module / nicht-kopierbare Organe). Da jede Phase 1 exakt zurückgerollt wird, macht NUR die
/// gemessene Phase-2-Op logischen Fortschritt ⇒ End-Zustand + Observer-Zähler identisch zur Einphasen-Messung.
/// QUALIFIZIERUNG (G5-Audit w289llo0o): „identisch" gilt für die nicht-auto-gekoppelten Achsen-Zähler (z.B. T0/T6).
/// Die 8 auto-gekoppelten Sekundär-Achsen zählen je logischer Op ZWEIMAL (Warmup-Op + Mess-Op tragen je in deren
/// gekoppelte Zähler ein, da der Rollback nur den materialisierten container_algorithm_-Zustand zurückrollt, nicht
/// die gekoppelten Observer-Counter) → für sie ist der Zählerstand NICHT bit-identisch zur Einphasen-Messung. Vgl. Doc 33 §2.
/// V5-Audit-Härtung: EMPIRISCHE Rollback-Exaktheits-Probe über das ABI (KEINE Interface-Erweiterung nötig).
/// save-all(leer) → Mutation → rollback-all → ist der leere Zustand exakt wiederhergestellt? Adressiert die
/// Audit-Lücke „two_phase_measure prüft nur rb!=nullptr, nicht Exaktheit": nicht-exakt-rollbackbare Organe
/// (z.B. nicht-kopierbare, deren save/rollback zu no-op degradiert) werden so erkannt → Kalt-Messung statt
/// stiller Warmup-Verfälschung. Läuft EINMAL je Tier VOR der Messung; lässt den Tier geleert zurück.
[[nodiscard]] inline bool
rollback_is_empirically_exact(::comdare::cache_engine::anatomy::IObservableTier&   tier,
                              ::comdare::cache_engine::anatomy::IRollbackableTier* rb) noexcept {
    if (rb == nullptr) return false;
    tier.tier_clear();
    rb->tier_save_all();                          // Memento des LEEREN Zustands
    (void)tier.tier_insert(0xDEADBEEFu, 1u);      // Mutation (insert/peak werden bei exaktem Rollback zurückgerollt)
    bool const mutated = (tier.tier_size() == 1); // Mutation hat gewirkt
    rb->tier_rollback_all();                      // muss den leeren Zustand exakt zurückrollen
    bool const exact = mutated && (tier.tier_size() == 0); // NUR tier_size() — KEIN tier_lookup (das würde
                                                           // lk_/miss_-Observer-Stats außerhalb des Mementos erhöhen).
    tier.tier_clear();                                     // Aufräumen (falls nicht exakt: Probe-Key entfernen)
    return exact;
}

template <class TimedOp>
[[nodiscard]] std::int64_t two_phase_measure(::comdare::cache_engine::anatomy::IRollbackableTier* rb,
                                             TimedOp&&                                            timed_op) {
    if (rb != nullptr) {
        rb->tier_save_all();
        (void)timed_op();        // Phase 1: Warmup (verworfen)
        rb->tier_rollback_all(); // Vor-Zustand exakt zurück
    }
    return timed_op(); // Phase 2: Messung (bzw. Kalt-Messung wenn rb==nullptr)
}
} // namespace detail

/// Treibt ein IObservableTier ueber die Fuellstands-Checkpoints (Pfad B; Trigger Zustands-Manipulation
/// §8.7b) + erhebt pro Checkpoint r/w/d-Wall-Clock + tier_observe-POD (Wall-Clock-korreliert). Der
/// uebergebene Tier wird VOR jedem Lauf NICHT geleert (der Aufrufer steuert den Start-Zustand); die
/// Checkpoints muessen monoton steigen.
[[nodiscard]] inline AbiTierObserveTrace drive_tier_observe_trace_abi(an::IObservableTier&      tier,
                                                                      AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    std::mt19937_64     rng{cfg.seed};
    std::uint64_t       next_key = 0;
    AbiTierObserveTrace trace;
    auto const          trace_start = clock::now(); // §8.7: Nullpunkt der Wall-Clock-Korrelations-Achse

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        AbiFillLevelSnapshot snap;

        // WRITE-Phase: bis Fuellstand == target; jede tier_insert() Wall-Clock-umklammert.
        // Robustheit (2026-05-31): bricht ab, wenn der Fuellstand ueber max_insert_stagnation Versuche NICHT
        // mehr waechst (Fixed-Capacity-Store voll / Key-Kollision) — sonst Endlosschleife bei target > Kapazitaet.
        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill        = tier.tier_size();
        while (tier.tier_size() < target) {
            auto const t0 = clock::now();
            (void)tier.tier_insert(next_key, next_key * 2u + 1u);
            auto const t1 = clock::now();
            snap.write_ns.push_back(detail::abi_dur_ns(t0, t1));
            ++next_key;
            std::uint64_t const cur_fill = tier.tier_size();
            if (cur_fill > last_fill) {
                last_fill        = cur_fill;
                write_stagnation = 0;
            } else if (++write_stagnation >= cfg.max_insert_stagnation)
                break; // effektive Tier-Kapazitaet erreicht
        }

        // READ-Phase: deterministische ~50%-Hit/Miss-Lookups ueber das Interface.
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const k   = (next_key != 0) ? (rng() % (next_key * 2u)) : 0u;
            std::uint64_t       out = 0;
            auto const          t0  = clock::now();
            bool const          hit = tier.tier_lookup(k, &out);
            auto const          t1  = clock::now();
            snap.read_sink += hit ? out : 0u;
            snap.read_ns.push_back(detail::abi_dur_ns(t0, t1));
        }

        // DELETE-Phase: erase + reinsert → Fuellstand bleibt == target.
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && tier.tier_size() > 0; ++i) {
            std::uint64_t const dk = (next_key != 0) ? (rng() % next_key) : 0u;
            auto const          t0 = clock::now();
            bool const          ok = tier.tier_erase(dk);
            auto const          t1 = clock::now();
            snap.delete_ns.push_back(detail::abi_dur_ns(t0, t1));
            if (ok) (void)tier.tier_insert(dk, dk * 2u + 1u); // Fuellstand wiederherstellen
        }

        snap.fill_level = tier.tier_size();
        // §8.7: EIN Observer-POD am Checkpoint, mit explizitem Wall-Clock-Zeitstempel korreliert (der
        // Builder persistiert die (Wall-Clock ↔ Observer)-Zuordnung).
        // NAHT-1 (Owner-KERN 09.08.2026): DIE UEBERGABE. Die CEB stellt die Senke auf und reicht
        // sie durch das Genus-Interface HINEIN; das Tier berichtet in sie. Vorher stand hier der
        // Sidecar-Pull (tier_observe), bei dem der Host von aussen einen POD abschabte.
        // Die Senke ist ein gewoehnlicher Typ; typgeloescht wird nur der MessEdge, auf dem Stack,
        // fuer die Dauer des Aufrufs.
        an::SnapshotSink               mess_senke{&snap.observer};
        an::MessEdge<an::SnapshotSink> mess_kante{mess_senke};
        tier.tier_measure_accept(mess_kante);
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
/// logischen Fortschritt ⇒ End-Füllstand IDENTISCH zu drive_tier_observe_trace_abi. (G5-Audit w289llo0o: „Observer-Zähler
/// IDENTISCH" gilt nur für die nicht-auto-gekoppelten Achsen, z.B. T0/T6 — die 8 auto-gekoppelten Sekundär-Achsen zählen
/// je logischer Op ZWEIMAL [Warmup + Messung], da der Rollback ihre gekoppelten Counter nicht zurückrollt; vgl. Doc 33 §2.)
///
/// `rollback`: das memento_all-Sub-Interface der GLEICHEN Tier-Instanz (Loader-`dynamic_cast`; nullptr ⇒ altes
/// Modul / nicht-exakt-rollbackbares Organ ⇒ graziöser Fallback auf Einphasen-Kalt-Messung pro Op). Die
/// nicht-gemessenen Zustands-Operationen (DELETE-reinsert) bleiben einphasig (reine Füllstand-Restauration).
[[nodiscard]] inline AbiTierObserveTrace drive_two_phase_tier_trace_abi(an::IObservableTier&      tier,
                                                                        an::IRollbackableTier*    rollback,
                                                                        AbiTierTraceConfig const& cfg = {}) {
    using clock = std::chrono::steady_clock;
    // V5-Audit-Härtung: nur zwei-phasig messen, wenn der Rollback EMPIRISCH exakt ist (sonst stille
    // Warmup-Verfälschung). Nicht-exakt → nullptr → Einphasen-Kalt-Messung pro Op.
    if (rollback != nullptr && !detail::rollback_is_empirically_exact(tier, rollback)) rollback = nullptr;
    std::mt19937_64     rng{cfg.seed};
    std::uint64_t       next_key = 0;
    AbiTierObserveTrace trace;
    auto const          trace_start = clock::now();

    for (std::uint64_t const target : cfg.fill_checkpoints) {
        AbiFillLevelSnapshot snap;

        // WRITE-Phase zwei-phasig: Warmup-insert(k) → rollback (k weg) → Mess-insert(k) (k da, echter Fortschritt).
        std::uint64_t write_stagnation = 0;
        std::uint64_t last_fill        = tier.tier_size();
        while (tier.tier_size() < target) {
            std::uint64_t const k  = next_key;
            auto const          ns = detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                auto const t0 = clock::now();
                (void)tier.tier_insert(k, k * 2u + 1u);
                auto const t1 = clock::now();
                return detail::abi_dur_ns(t0, t1);
            });
            snap.write_ns.push_back(ns);
            ++next_key;
            std::uint64_t const cur_fill = tier.tier_size();
            if (cur_fill > last_fill) {
                last_fill        = cur_fill;
                write_stagnation = 0;
            } else if (++write_stagnation >= cfg.max_insert_stagnation)
                break;
        }

        // READ-Phase zwei-phasig: Warmup-lookup heizt, rollback restauriert Observer-Stats, Mess-lookup zählt.
        for (std::uint64_t i = 0; i < cfg.lookups_per_checkpoint; ++i) {
            std::uint64_t const k            = (next_key != 0) ? (rng() % (next_key * 2u)) : 0u;
            std::uint64_t       measured_out = 0;
            bool                measured_hit = false;
            auto const          ns           = detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                std::uint64_t out = 0;
                auto const    t0  = clock::now();
                bool const    hit = tier.tier_lookup(k, &out);
                auto const    t1  = clock::now();
                measured_hit      = hit;
                measured_out      = out; // nach two_phase_measure spiegelt dies die MESS-Phase
                return detail::abi_dur_ns(t0, t1);
            });
            snap.read_sink += measured_hit ? measured_out : 0u;
            snap.read_ns.push_back(ns);
        }

        // DELETE-Phase zwei-phasig (Mess-Op = erase); die Füllstand-Restauration (reinsert) ist NICHT gemessen.
        for (std::uint64_t i = 0; i < cfg.deletes_per_checkpoint && tier.tier_size() > 0; ++i) {
            std::uint64_t const dk          = (next_key != 0) ? (rng() % next_key) : 0u;
            bool                measured_ok = false;
            auto const          ns          = detail::two_phase_measure(rollback, [&]() -> std::int64_t {
                auto const t0 = clock::now();
                bool const ok = tier.tier_erase(dk);
                auto const t1 = clock::now();
                measured_ok   = ok;
                return detail::abi_dur_ns(t0, t1);
            });
            snap.delete_ns.push_back(ns);
            if (measured_ok) (void)tier.tier_insert(dk, dk * 2u + 1u); // Füllstand wiederherstellen (einphasig)
        }

        snap.fill_level = tier.tier_size();
        // NAHT-1 (Owner-KERN 09.08.2026): DIE UEBERGABE. Die CEB stellt die Senke auf und reicht
        // sie durch das Genus-Interface HINEIN; das Tier berichtet in sie. Vorher stand hier der
        // Sidecar-Pull (tier_observe), bei dem der Host von aussen einen POD abschabte.
        // Die Senke ist ein gewoehnlicher Typ; typgeloescht wird nur der MessEdge, auf dem Stack,
        // fuer die Dauer des Aufrufs.
        an::SnapshotSink               mess_senke{&snap.observer};
        an::MessEdge<an::SnapshotSink> mess_kante{mess_senke};
        tier.tier_measure_accept(mess_kante);
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
    // D5-4: Zeit-Kopf + Zeit-Zellen aus dem GETEILTEN Schema (ts::), nicht mehr als Literal-Abschrift.
    os << ts::kGeteilterCsvKopf
       << ",search_insert,search_lookup,search_hit,search_miss,search_erase,search_peak_occupancy,"
          "alloc_bytes_in_use,alloc_alloc_count,observable_axes\n";
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        ts::schreibe_geteilte_csv_zellen(os, i, cp);
        os << ',' << o.axis_stats[0][3] << ',' << o.axis_stats[0][0] << ',' << o.axis_stats[0][1] << ','
           << o.axis_stats[0][2] << ',' << o.axis_stats[0][4] << ',' << o.axis_stats[0][5] << ',' << o.axis_stats[6][1]
           << ',' << o.axis_stats[6][2] << ',' << o.observable_axis_count << '\n';
    }
    return os.str();
}

/// Persistiert den Pfad-B-Trace als JSON-Array (eine Objekt-Zeile je Checkpoint). Zusätzlich zur CSV trägt
/// die JSON die **Perzentile** (p50/p95/p99) der r/w/d-Roh-ns-Kurven — die Tier-Wall-Clock-Detail-Auswertung
/// (Doku 24 §2.1) korreliert mit den Observer-Zählern. Robust gegen Wall-Clock-Ausreisser (p50, vgl. Doku 22 §3.3).
///
/// SELBSTCHECK (D5-4, 2026-08-09)
///   ZUSICHERT: der Latenz-Feldblock steht NICHT MEHR in dieser Datei. Er kommt aus
///              trace_schema::kLatenzFelder und wird von derselben Funktion gerendert wie bei den vier
///              Container-Gattungen -- ein Feld kann hier nicht mehr fehlen, ohne dort zu fehlen.
///   ZUSICHERT NICHT: Vergleichbarkeit mit Traces vor dem 2026-08-09 (D5-1 hat die Perzentil-Definition
///              gewechselt; D5-4 ergaenzt delete_p99_ns). Alte Traces sind ungueltig, nicht geloescht.
[[nodiscard]] inline std::string serialize_abi_tier_trace_json(AbiTierObserveTrace const& trace) {
    std::ostringstream os;
    os << '[';
    for (std::size_t i = 0; i < trace.checkpoints.size(); ++i) {
        auto const& cp = trace.checkpoints[i];
        auto const& o  = cp.observer;
        if (i != 0) os << ',';
        ts::schreibe_geteilten_json_vorspann(os, i, cp);
        os << ",\"search_insert\":" << o.axis_stats[0][3] << ",\"search_lookup\":" << o.axis_stats[0][0]
           << ",\"search_hit\":" << o.axis_stats[0][1] << ",\"search_miss\":" << o.axis_stats[0][2]
           << ",\"search_peak_occupancy\":" << o.axis_stats[0][5] << ",\"alloc_bytes_in_use\":" << o.axis_stats[6][1]
           << ",\"observable_axes\":" << o.observable_axis_count << '}';
    }
    os << ']';
    return os.str();
}

} // namespace comdare::cache_engine::builder::anatomy_commands
